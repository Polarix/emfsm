/**
 * @file fsm.c
 * @brief 二维表形式的有限状态机实现
 */

#include "fsm.h"
#include <stddef.h>

/*================================================================*/
/* 内部函数声明                                                   */
/*================================================================*/
/**
 * @brief 根据状态ID查找状态描述
 * @param config FSM配置
 * @param state_id 状态ID
 * @return const fsm_state_t* 状态描述指针，失败返回NULL
 */
FSM_PRIVATE const fsm_state_t* fsm_find_state(const fsm_config_t* config,
                                               fsm_state_id_t state_id);

/**
 * @brief 执行状态转移
 * @param context FSM上下文
 * @param next_state 目标状态ID
 * @param action 转移动作函数
 * @param event_data 事件数据
 * @return fsm_err_t 错误码
 */
FSM_PRIVATE fsm_err_t fsm_perform_transition(fsm_context_t* context,
                                              fsm_state_id_t next_state,
                                              fsm_transition_action_t action,
                                              void* event_data);

/*================================================================*/
/* 默认弱函数                                                     */
/*================================================================*/

/**
 * @brief 默认的空动作函数（弱符号）
 */
FSM_WEAK void fsm_action_nop(fsm_context_t* context, void* event_data)
{
    (void)context;
    (void)event_data;
    /* 空函数，什么也不做 */
}

/**
 * @brief 默认的未处理事件处理函数（弱符号）
 */
FSM_WEAK fsm_err_t fsm_default_unhandled_event(fsm_context_t* context,
                                                fsm_event_id_t event,
                                                void* event_data)
{
    (void)context;
    (void)event;
    (void)event_data;
    return FSM_ERR_INVALID_EVENT;
}

/*================================================================*/
/* 外部API实现                                                    */
/*================================================================*/

fsm_handle_t fsm_create(const fsm_config_t* config, fsm_context_t* context)
{
    const fsm_state_t* init_state = NULL;

    /* 参数检查 */
    if ((config == NULL) || (context == NULL))
    {
        FSM_LOG_ERR("fsm_create: invalid parameter (config or context is NULL)");
        return NULL;
    }

    if ((config->states == NULL) || (config->state_count == 0))
    {
        FSM_LOG_ERR("fsm_create: states table is empty");
        return NULL;
    }

    if (config->transition_table == NULL)
    {
        FSM_LOG_ERR("fsm_create: transition_table is NULL");
        return NULL;
    }

    if (config->state_count > FSM_MAX_STATES)
    {
        FSM_LOG_ERR("fsm_create: state count exceeds FSM_MAX_STATES (%u > %u)",
                    config->state_count, FSM_MAX_STATES);
        return NULL;
    }

    /* 初始化上下文 */
    (void)memset(context, 0, sizeof(fsm_context_t));

    context->current_state = 0;          /* 默认初始状态为第一个状态 */
    context->previous_state = 0;
    context->config = config;
    context->state_entry_time = 0;
    context->initialized = true;
    context->processing = false;

    /* 调用初始状态的进入函数（如果存在） */
    init_state = fsm_find_state(config, context->current_state);
    if ((init_state != NULL) && (init_state->on_enter != NULL))
    {
        init_state->on_enter(context);
    }

    FSM_LOG_INFO("fsm_create: FSM '%s' created, initial state=%u",
                 config->name ? config->name : "unnamed", context->current_state);

    return (fsm_handle_t)context;
}

void fsm_destroy(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* current_state = NULL;

    if ((context == NULL) || (!context->initialized))
    {
        return;
    }

    /* 调用当前状态的退出函数（如果存在） */
    current_state = fsm_find_state(context->config, context->current_state);
    if ((current_state != NULL) && (current_state->on_exit != NULL))
    {
        current_state->on_exit(context);
    }

    /* 清除上下文 */
    (void)memset(context, 0, sizeof(fsm_context_t));

    FSM_LOG_INFO("fsm_destroy: FSM destroyed");
}

fsm_err_t fsm_process_event(fsm_handle_t handle, fsm_event_id_t event, void* event_data)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* current_state = NULL;
    const fsm_transition_item_t* trans_item = NULL;
    fsm_err_t ret = FSM_OK;

    /* 参数检查 */
    if ((context == NULL) || (!context->initialized))
    {
        ret = FSM_ERR_NOT_INITIALIZED;
        FSM_LOG_ERR("fsm_process_event: FSM not initialized");
        return ret;
    }

    /* 防止重入 */
    if (context->processing)
    {
        ret = FSM_ERR_INVALID_PARAM;
        FSM_LOG_WRN("fsm_process_event: reentrant call blocked");
        return ret;
    }

    context->processing = true;
    context->event_count++;

    /* 检查事件ID是否有效 */
    if (event >= context->config->event_count)
    {
        context->processing = false;
        ret = FSM_ERR_INVALID_EVENT;
        FSM_LOG_WRN("fsm_process_event: invalid event %u", event);
        return ret;
    }

    /* 查找当前状态 */
    current_state = fsm_find_state(context->config, context->current_state);
    if (current_state == NULL)
    {
        context->processing = false;
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_process_event: current state %u not found",
                    context->current_state);
        return ret;
    }

    /* 首先尝试状态处理函数 */
    if (current_state->handler != NULL)
    {
        ret = current_state->handler(context, event, event_data);
        if (ret == FSM_OK)
        {
            /* 状态处理函数已处理事件 */
            FSM_LOG_DBG("fsm_process_event: state handler processed event %u", event);
            context->processing = false;
            return ret;
        }
    }

    /* 查找二维转移表 */
    trans_item = fsm_get_transition_item(context->config, context->current_state, event);
    if (trans_item == NULL)
    {
        /* 没有找到转移项，使用默认处理 */
        ret = fsm_default_unhandled_event(context, event, event_data);
        context->processing = false;
        FSM_LOG_DBG("fsm_process_event: no transition for event %u", event);
        return ret;
    }

    /* 检查下一个状态是否有效 */
    if (trans_item->next_state >= context->config->state_count)
    {
        context->processing = false;
        ret = FSM_ERR_NO_TRANSITION;
        FSM_LOG_ERR("fsm_process_event: next state %u out of range", trans_item->next_state);
        return ret;
    }

    /* 如果下一个状态是当前状态，且没有守卫和动作，直接返回成功（自循环） */
    if ((trans_item->next_state == context->current_state) &&
        (trans_item->guard == FSM_GUARD_NOP) &&
        (trans_item->action == FSM_ACTION_NOP))
    {
        context->processing = false;
        FSM_LOG_DBG("fsm_process_event: self-loop without guard/action");
        return FSM_OK;
    }

    /* 检查守卫条件 */
    if (trans_item->guard != FSM_GUARD_NOP)
    {
        if (!trans_item->guard(context, event_data))
        {
            context->guard_rejection_count++;
            context->processing = false;
            ret = FSM_ERR_GUARD_REJECTED;
            FSM_LOG_WRN("fsm_process_event: guard rejected transition from %u to %u",
                        context->current_state, trans_item->next_state);
            return ret;
        }
    }

    /* 执行状态转移 */
    ret = fsm_perform_transition(context, trans_item->next_state,
                                  trans_item->action, event_data);
    context->processing = false;
    return ret;
}

fsm_err_t fsm_force_state(fsm_handle_t handle, fsm_state_id_t state)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* target_state = NULL;
    const fsm_state_t* current_state = NULL;
    fsm_err_t ret = FSM_OK;

    if ((context == NULL) || (!context->initialized))
    {
        ret = FSM_ERR_NOT_INITIALIZED;
        FSM_LOG_ERR("fsm_force_state: FSM not initialized");
        return ret;
    }

    /* 检查状态ID是否有效 */
    if (state >= context->config->state_count)
    {
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_force_state: state %u out of range", state);
        return ret;
    }

    /* 查找目标状态 */
    target_state = fsm_find_state(context->config, state);
    if (target_state == NULL)
    {
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_force_state: state %u not found", state);
        return ret;
    }

    /* 如果已经在目标状态，直接返回 */
    if (context->current_state == state)
    {
        return FSM_OK;
    }

    /* 调用当前状态的退出函数 */
    current_state = fsm_find_state(context->config, context->current_state);
    if ((current_state != NULL) && (current_state->on_exit != NULL))
    {
        current_state->on_exit(context);
    }

    /* 更新状态 */
    context->previous_state = context->current_state;
    context->current_state = state;

    /* 调用目标状态的进入函数 */
    if (target_state->on_enter != NULL)
    {
        target_state->on_enter(context);
    }

    /* 重置状态进入时间 */
    context->state_entry_time = 0;

    FSM_LOG_INFO("fsm_force_state: forced state from %u to %u",
                 context->previous_state, context->current_state);

    return FSM_OK;
}

fsm_state_id_t fsm_get_current_state(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    fsm_state_id_t state = 0;

    if ((context == NULL) || (!context->initialized))
    {
        FSM_LOG_ERR("fsm_get_current_state: FSM not initialized");
        return 0;
    }

    state = context->current_state;
    return state;
}

fsm_state_id_t fsm_get_previous_state(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    fsm_state_id_t state = 0;

    if ((context == NULL) || (!context->initialized))
    {
        FSM_LOG_ERR("fsm_get_previous_state: FSM not initialized");
        return 0;
    }

    state = context->previous_state;
    return state;
}

const char* fsm_get_state_name(fsm_handle_t handle, fsm_state_id_t state)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* state_desc = NULL;

    if ((context == NULL) || (!context->initialized) || (context->config == NULL))
    {
        FSM_LOG_ERR("fsm_get_state_name: FSM not initialized");
        return NULL;
    }

    state_desc = fsm_find_state(context->config, state);
    if (state_desc == NULL)
    {
        FSM_LOG_WRN("fsm_get_state_name: state %u not found", state);
        return NULL;
    }

    return state_desc->name;
}

bool fsm_is_in_state(fsm_handle_t handle, fsm_state_id_t state)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    bool result = false;

    if ((context == NULL) || (!context->initialized))
    {
        FSM_LOG_ERR("fsm_is_in_state: FSM not initialized");
        return false;
    }

    result = (context->current_state == state);
    return result;
}

fsm_err_t fsm_check_timeout(fsm_handle_t handle, uint32_t current_time)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* current_state = NULL;
    fsm_err_t ret = FSM_OK;

    if ((context == NULL) || (!context->initialized))
    {
        ret = FSM_ERR_NOT_INITIALIZED;
        FSM_LOG_ERR("fsm_check_timeout: FSM not initialized");
        return ret;
    }

    /* 获取当前状态 */
    current_state = fsm_find_state(context->config, context->current_state);
    if (current_state == NULL)
    {
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_check_timeout: current state %u not found", context->current_state);
        return ret;
    }

    /* 检查是否需要超时处理 */
    if ((current_state->timeout_ms == 0) || (current_state->timeout_next_state == 0))
    {
        return FSM_OK;
    }

    /* 检查是否超时 */
    if ((current_time - context->state_entry_time) >= current_state->timeout_ms)
    {
        /* 执行超时转移 */
        context->timeout_count++;
        ret = fsm_force_state(handle, current_state->timeout_next_state);
        FSM_LOG_INFO("fsm_check_timeout: timeout occurred, forcing to state %u",
                     current_state->timeout_next_state);
    }

    return ret;
}

fsm_err_t fsm_get_stats(fsm_handle_t handle, fsm_stats_t* stats)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    fsm_err_t ret = FSM_OK;

    if ((context == NULL) || (!context->initialized) || (stats == NULL))
    {
        ret = FSM_ERR_INVALID_PARAM;
        FSM_LOG_ERR("fsm_get_stats: invalid parameter");
        return ret;
    }

    stats->event_count = context->event_count;
    stats->transition_count = context->transition_count;
    stats->error_count = 0;          /* 暂未统计具体错误次数 */
    stats->state_timeouts = context->timeout_count;
    stats->guard_rejections = context->guard_rejection_count;

    return FSM_OK;
}

fsm_err_t fsm_reset_stats(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    fsm_err_t ret = FSM_OK;

    if ((context == NULL) || (!context->initialized))
    {
        ret = FSM_ERR_NOT_INITIALIZED;
        FSM_LOG_ERR("fsm_reset_stats: FSM not initialized");
        return ret;
    }

    context->event_count = 0;
    context->transition_count = 0;
    context->guard_rejection_count = 0;
    context->timeout_count = 0;

    return FSM_OK;
}

fsm_context_t* fsm_get_context(fsm_handle_t handle)
{
    return (fsm_context_t*)handle;
}

#ifdef FSM_ENABLE_DEBUG
void fsm_print_info(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_config_t* config = NULL;
    const fsm_state_t* current_state = NULL;

    if ((context == NULL) || (!context->initialized))
    {
        FSM_LOG_ERR("fsm_print_info: FSM not initialized");
        return;
    }

    config = context->config;
    current_state = fsm_find_state(config, context->current_state);

    FSM_PRINTF("FSM: %s\r\n", config->name ? config->name : "Unnamed");
    FSM_PRINTF("  Current State: %s (ID: %u)\r\n",
               current_state ? current_state->name : "Unknown",
               context->current_state);
    FSM_PRINTF("  Previous State: %u\r\n", context->previous_state);
    FSM_PRINTF("  Events Processed: %u\r\n", context->event_count);
    FSM_PRINTF("  Transitions: %u\r\n", context->transition_count);
    FSM_PRINTF("  Guard Rejections: %u\r\n", context->guard_rejection_count);
    FSM_PRINTF("  Timeouts: %u\r\n", context->timeout_count);
}

void fsm_print_transition_table(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_config_t* config = NULL;
    uint16_t s = 0;
    uint16_t e = 0;

    if ((context == NULL) || (!context->initialized) || (context->config == NULL))
    {
        FSM_LOG_ERR("fsm_print_transition_table: FSM not initialized");
        return;
    }

    config = context->config;

    FSM_PRINTF("Transition Table for FSM: %s\r\n", config->name ? config->name : "Unnamed");
    FSM_PRINTF("State Count: %u, Event Count: %u\r\n",
               config->state_count, config->event_count);

    FSM_PRINTF("\r\nStates:\r\n");
    for (s = 0; s < config->state_count; s++)
    {
        const fsm_state_t* state = &config->states[s];
        FSM_PRINTF("  [%u] %s (Timeout: %u ms -> State %u)\r\n",
                   state->id, state->name, state->timeout_ms, state->timeout_next_state);
    }

    FSM_PRINTF("\r\nTransition Matrix (State x Event):\r\n");
    FSM_PRINTF("State \\ Event");
    for (e = 0; e < config->event_count; e++)
    {
        FSM_PRINTF(" | E%u", e);
    }
    FSM_PRINTF("\r\n");

    for (s = 0; s < config->state_count; s++)
    {
        FSM_PRINTF("S%u", s);
        for (e = 0; e < config->event_count; e++)
        {
            const fsm_transition_item_t* item = fsm_get_transition_item(config, s, e);
            if (item == NULL)
            {
                FSM_PRINTF(" |  -");
            }
            else if ((item->next_state == s) &&
                     (item->guard == FSM_GUARD_NOP) &&
                     (item->action == FSM_ACTION_NOP))
            {
                FSM_PRINTF(" |  *");  /* 自循环 */
            }
            else
            {
                FSM_PRINTF(" | %s%u",
                           item->guard != FSM_GUARD_NOP ? "G" : "",
                           item->next_state);
            }
        }
        FSM_PRINTF("\r\n");
    }

    /* 详细转移信息 */
    FSM_PRINTF("\r\nDetailed Transitions:\r\n");
    for (s = 0; s < config->state_count; s++)
    {
        for (e = 0; e < config->event_count; e++)
        {
            const fsm_transition_item_t* item = fsm_get_transition_item(config, s, e);
            if ((item != NULL) &&
                !((item->next_state == s) &&
                  (item->guard == FSM_GUARD_NOP) &&
                  (item->action == FSM_ACTION_NOP)))
            {
                const fsm_state_t* from_state = fsm_find_state(config, s);
                const fsm_state_t* to_state = fsm_find_state(config, item->next_state);

                FSM_PRINTF("  S%u(%s) + E%u -> S%u(%s)",
                           s, from_state ? from_state->name : "?",
                           e,
                           item->next_state, to_state ? to_state->name : "?");

                if (item->guard != FSM_GUARD_NOP)
                {
                    FSM_PRINTF(" [Guard]");
                }
                if (item->action != FSM_ACTION_NOP)
                {
                    FSM_PRINTF(" [Action]");
                }
                FSM_PRINTF("\r\n");
            }
        }
    }
}
#endif /* FSM_ENABLE_DEBUG */

/*================================================================*/
/* 内部函数实现                                                   */
/*================================================================*/
/**
 * @brief 根据状态ID查找状态描述
 * @param config FSM配置
 * @param state_id 状态ID
 * @return const fsm_state_t* 状态描述指针，失败返回NULL
 */
FSM_PRIVATE const fsm_state_t* fsm_find_state(const fsm_config_t* config,
                                               fsm_state_id_t state_id)
{
    const fsm_state_t* state = NULL;

    if ((config == NULL) || (config->states == NULL))
    {
        return NULL;
    }

    /* 假设状态ID是连续且从0开始，直接通过索引访问 */
    if (state_id >= config->state_count)
    {
        return NULL;
    }

    state = &config->states[state_id];
    return state;
}

/**
 * @brief 执行状态转移
 * @param context FSM上下文
 * @param next_state 目标状态ID
 * @param action 转移动作函数
 * @param event_data 事件数据
 * @return fsm_err_t 错误码
 */
FSM_PRIVATE fsm_err_t fsm_perform_transition(fsm_context_t* context,
                                              fsm_state_id_t next_state,
                                              fsm_transition_action_t action,
                                              void* event_data)
{
    const fsm_state_t* next_state_desc = NULL;
    const fsm_state_t* current_state = NULL;
    fsm_err_t ret = FSM_OK;

    /* 检查目标状态是否存在 */
    next_state_desc = fsm_find_state(context->config, next_state);
    if (next_state_desc == NULL)
    {
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_perform_transition: next state %u not found", next_state);
        return ret;
    }

    /* 如果当前状态和目标状态相同，不执行转移 */
    if (context->current_state == next_state)
    {
        /* 只执行动作（如果有） */
        if (action != FSM_ACTION_NOP)
        {
            action(context, event_data);
        }
        return FSM_OK;
    }

    /* 获取当前状态描述 */
    current_state = fsm_find_state(context->config, context->current_state);

    /* 执行当前状态的退出函数 */
    if ((current_state != NULL) && (current_state->on_exit != NULL))
    {
        current_state->on_exit(context);
    }

    /* 执行转移动作 */
    if (action != FSM_ACTION_NOP)
    {
        action(context, event_data);
    }

    /* 更新状态 */
    context->previous_state = context->current_state;
    context->current_state = next_state;

    /* 执行目标状态的进入函数 */
    if (next_state_desc->on_enter != NULL)
    {
        next_state_desc->on_enter(context);
    }

    /* 更新统计 */
    context->transition_count++;

    FSM_LOG_DBG("fsm_perform_transition: %u -> %u, total transitions=%u",
                context->previous_state, context->current_state,
                context->transition_count);

    return FSM_OK;
}
