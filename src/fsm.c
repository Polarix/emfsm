/**
 * @file fsm.c
 * @brief 二维表形式的有限状态机实现 - 无超时版本
 */

#include "fsm.h"
#include <stddef.h>

#ifdef FSM_ENABLE_DEBUG
#include <stdio.h>
#endif

/*============================================================================*
 *                                内部函数声明                                 *
 *============================================================================*/
FSM_PRIVATE const fsm_state_t* fsm_find_state(const fsm_config_t* config,
                                               fsm_state_id_t state_id);

FSM_PRIVATE fsm_err_t fsm_perform_transition(fsm_context_t* context,
                                              fsm_state_id_t next_state,
                                              fsm_transition_action_t action,
                                              void* event_data);

/*============================================================================*
 *                                默认弱函数                                   *
 *============================================================================*/

FSM_WEAK void fsm_action_nop(fsm_context_t* context, void* event_data)
{
    (void)context;
    (void)event_data;
}

FSM_WEAK fsm_err_t fsm_default_unhandled_event(fsm_context_t* context,
                                                fsm_event_id_t event,
                                                void* event_data)
{
    (void)context;
    (void)event;
    (void)event_data;
    return FSM_ERR_INVALID_EVENT;
}

/*============================================================================*
 *                                外部API实现                                  *
 *============================================================================*/

fsm_handle_t fsm_create(const fsm_config_t* config, fsm_context_t* context)
{
    const fsm_state_t* init_state = NULL;

    if (config == NULL || context == NULL) {
        FSM_LOG_ERR("fsm_create: invalid parameter");
        return NULL;
    }
    if (config->states == NULL || config->state_count == 0) {
        FSM_LOG_ERR("fsm_create: states table is empty");
        return NULL;
    }
    if (config->transition_table == NULL) {
        FSM_LOG_ERR("fsm_create: transition_table is NULL");
        return NULL;
    }
    if (config->state_count > FSM_MAX_STATES) {
        FSM_LOG_ERR("fsm_create: state count exceeds %u", FSM_MAX_STATES);
        return NULL;
    }

    (void)memset(context, 0, sizeof(fsm_context_t));

    context->current_state = 0;
    context->previous_state = 0;
    context->config = config;
    context->initialized = true;
    context->processing = false;

    init_state = fsm_find_state(config, context->current_state);
    if (init_state != NULL && init_state->on_enter != NULL) {
        init_state->on_enter(context);
    }

    FSM_LOG_INFO("fsm_create: FSM '%s' created", config->name ? config->name : "unnamed");
    return (fsm_handle_t)context;
}

void fsm_destroy(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* current_state = NULL;

    if (context == NULL || !context->initialized) {
        return;
    }

    current_state = fsm_find_state(context->config, context->current_state);
    if (current_state != NULL && current_state->on_exit != NULL) {
        current_state->on_exit(context);
    }

    (void)memset(context, 0, sizeof(fsm_context_t));
    FSM_LOG_INFO("fsm_destroy: FSM destroyed");
}

fsm_err_t fsm_process_event(fsm_handle_t handle, fsm_event_id_t event, void* event_data)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* current_state = NULL;
    const fsm_transition_item_t* trans_item = NULL;
    fsm_err_t ret = FSM_OK;

    if (context == NULL || !context->initialized) {
        ret = FSM_ERR_NOT_INITIALIZED;
        FSM_LOG_ERR("fsm_process_event: FSM not initialized");
        return ret;
    }

    if (context->processing) {
        ret = FSM_ERR_INVALID_PARAM;
        FSM_LOG_WRN("fsm_process_event: reentrant call blocked");
        return ret;
    }

    context->processing = true;
    context->event_count++;

    if (event >= context->config->event_count) {
        context->processing = false;
        ret = FSM_ERR_INVALID_EVENT;
        FSM_LOG_WRN("fsm_process_event: invalid event %u", event);
        return ret;
    }

    current_state = fsm_find_state(context->config, context->current_state);
    if (current_state == NULL) {
        context->processing = false;
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_process_event: current state %u not found", context->current_state);
        return ret;
    }

    if (current_state->handler != NULL) {
        ret = current_state->handler(context, event, event_data);
        if (ret == FSM_OK) {
            context->processing = false;
            return ret;
        }
    }

    trans_item = fsm_get_transition_item(context->config, context->current_state, event);
    if (trans_item == NULL) {
        ret = fsm_default_unhandled_event(context, event, event_data);
        context->processing = false;
        return ret;
    }

    if (trans_item->next_state >= context->config->state_count) {
        context->processing = false;
        ret = FSM_ERR_NO_TRANSITION;
        FSM_LOG_ERR("fsm_process_event: next state %u out of range", trans_item->next_state);
        return ret;
    }

    if (trans_item->next_state == context->current_state &&
        trans_item->guard == FSM_GUARD_NOP &&
        trans_item->action == FSM_ACTION_NOP) {
        context->processing = false;
        return FSM_OK;
    }

    if (trans_item->guard != FSM_GUARD_NOP) {
        if (!trans_item->guard(context, event_data)) {
            context->guard_rejection_count++;
            context->processing = false;
            ret = FSM_ERR_GUARD_REJECTED;
            FSM_LOG_WRN("fsm_process_event: guard rejected transition");
            return ret;
        }
    }

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

    if (context == NULL || !context->initialized) {
        ret = FSM_ERR_NOT_INITIALIZED;
        FSM_LOG_ERR("fsm_force_state: FSM not initialized");
        return ret;
    }

    if (state >= context->config->state_count) {
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_force_state: state %u out of range", state);
        return ret;
    }

    target_state = fsm_find_state(context->config, state);
    if (target_state == NULL) {
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_force_state: state %u not found", state);
        return ret;
    }

    if (context->current_state == state) {
        return FSM_OK;
    }

    current_state = fsm_find_state(context->config, context->current_state);
    if (current_state != NULL && current_state->on_exit != NULL) {
        current_state->on_exit(context);
    }

    context->previous_state = context->current_state;
    context->current_state = state;

    if (target_state->on_enter != NULL) {
        target_state->on_enter(context);
    }

    FSM_LOG_INFO("fsm_force_state: forced state to %u", state);
    return FSM_OK;
}

fsm_state_id_t fsm_get_current_state(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    if (context == NULL || !context->initialized) {
        return 0;
    }
    return context->current_state;
}

fsm_state_id_t fsm_get_previous_state(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    if (context == NULL || !context->initialized) {
        return 0;
    }
    return context->previous_state;
}

const char* fsm_get_state_name(fsm_handle_t handle, fsm_state_id_t state)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_state_t* state_desc = NULL;

    if (context == NULL || !context->initialized || context->config == NULL) {
        return NULL;
    }

    state_desc = fsm_find_state(context->config, state);
    if (state_desc == NULL) {
        return NULL;
    }
    return state_desc->name;
}

bool fsm_is_in_state(fsm_handle_t handle, fsm_state_id_t state)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    if (context == NULL || !context->initialized) {
        return false;
    }
    return (context->current_state == state);
}

fsm_err_t fsm_get_stats(fsm_handle_t handle, fsm_stats_t* stats)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    if (context == NULL || !context->initialized || stats == NULL) {
        return FSM_ERR_INVALID_PARAM;
    }

    stats->event_count = context->event_count;
    stats->transition_count = context->transition_count;
    stats->error_count = 0;
    stats->guard_rejections = context->guard_rejection_count;
    return FSM_OK;
}

fsm_err_t fsm_reset_stats(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    if (context == NULL || !context->initialized) {
        return FSM_ERR_NOT_INITIALIZED;
    }

    context->event_count = 0;
    context->transition_count = 0;
    context->guard_rejection_count = 0;
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

    if (context == NULL || !context->initialized) {
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
}

void fsm_print_transition_table(fsm_handle_t handle)
{
    fsm_context_t* context = (fsm_context_t*)handle;
    const fsm_config_t* config = NULL;
    uint16_t s, e;

    if (context == NULL || !context->initialized || context->config == NULL) {
        FSM_LOG_ERR("fsm_print_transition_table: FSM not initialized");
        return;
    }

    config = context->config;

    FSM_PRINTF("Transition Table for FSM: %s\r\n", config->name ? config->name : "Unnamed");
    FSM_PRINTF("State Count: %u, Event Count: %u\r\n",
               config->state_count, config->event_count);

    FSM_PRINTF("\r\nStates:\r\n");
    for (s = 0; s < config->state_count; s++) {
        const fsm_state_t* state = &config->states[s];
        FSM_PRINTF("  [%u] %s\r\n", state->id, state->name);
    }

    FSM_PRINTF("\r\nTransition Matrix (State x Event):\r\n");
    FSM_PRINTF("State \\ Event");
    for (e = 0; e < config->event_count; e++) {
        FSM_PRINTF(" | E%u", e);
    }
    FSM_PRINTF("\r\n");

    for (s = 0; s < config->state_count; s++) {
        FSM_PRINTF("S%u", s);
        for (e = 0; e < config->event_count; e++) {
            const fsm_transition_item_t* item = fsm_get_transition_item(config, s, e);
            if (item == NULL) {
                FSM_PRINTF(" |  -");
            } else if (item->next_state == s &&
                       item->guard == FSM_GUARD_NOP &&
                       item->action == FSM_ACTION_NOP) {
                FSM_PRINTF(" |  *");
            } else {
                FSM_PRINTF(" | %s%u",
                           item->guard != FSM_GUARD_NOP ? "G" : "",
                           item->next_state);
            }
        }
        FSM_PRINTF("\r\n");
    }

    FSM_PRINTF("\r\nDetailed Transitions:\r\n");
    for (s = 0; s < config->state_count; s++) {
        for (e = 0; e < config->event_count; e++) {
            const fsm_transition_item_t* item = fsm_get_transition_item(config, s, e);
            if (item != NULL && !(item->next_state == s &&
                                   item->guard == FSM_GUARD_NOP &&
                                   item->action == FSM_ACTION_NOP)) {
                const fsm_state_t* from_state = fsm_find_state(config, s);
                const fsm_state_t* to_state = fsm_find_state(config, item->next_state);

                FSM_PRINTF("  S%u(%s) + E%u -> S%u(%s)",
                           s, from_state ? from_state->name : "?",
                           e,
                           item->next_state, to_state ? to_state->name : "?");

                if (item->guard != FSM_GUARD_NOP) {
                    FSM_PRINTF(" [Guard]");
                }
                if (item->action != FSM_ACTION_NOP) {
                    FSM_PRINTF(" [Action]");
                }
                FSM_PRINTF("\r\n");
            }
        }
    }
}
#endif /* FSM_ENABLE_DEBUG */

/*============================================================================*
 *                                内部函数实现                                 *
 *============================================================================*/

FSM_PRIVATE const fsm_state_t* fsm_find_state(const fsm_config_t* config,
                                               fsm_state_id_t state_id)
{
    if (config == NULL || config->states == NULL) {
        return NULL;
    }
    if (state_id >= config->state_count) {
        return NULL;
    }
    return &config->states[state_id];
}

FSM_PRIVATE fsm_err_t fsm_perform_transition(fsm_context_t* context,
                                              fsm_state_id_t next_state,
                                              fsm_transition_action_t action,
                                              void* event_data)
{
    const fsm_state_t* next_state_desc = NULL;
    const fsm_state_t* current_state = NULL;
    fsm_err_t ret = FSM_OK;

    next_state_desc = fsm_find_state(context->config, next_state);
    if (next_state_desc == NULL) {
        ret = FSM_ERR_INVALID_STATE;
        FSM_LOG_ERR("fsm_perform_transition: next state %u not found", next_state);
        return ret;
    }

    if (context->current_state == next_state) {
        if (action != FSM_ACTION_NOP) {
            action(context, event_data);
        }
        return FSM_OK;
    }

    current_state = fsm_find_state(context->config, context->current_state);

    if (current_state != NULL && current_state->on_exit != NULL) {
        current_state->on_exit(context);
    }

    if (action != FSM_ACTION_NOP) {
        action(context, event_data);
    }

    context->previous_state = context->current_state;
    context->current_state = next_state;

    if (next_state_desc->on_enter != NULL) {
        next_state_desc->on_enter(context);
    }

    context->transition_count++;
    FSM_LOG_DBG("fsm_perform_transition: %u -> %u", context->previous_state, context->current_state);
    return FSM_OK;
}
