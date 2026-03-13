/**
 * @file led_example.c
 * @brief FSM框架简单示例 - LED灯光控制器（无超时）
 */

/*================================================================*/
/* 头文件包含 */
/*================================================================*/
#include "fsm.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

/*================================================================*/
/* 宏定义 */
/*================================================================*/
/* 独立的示例日志宏 */
#ifndef DEMO_PRINTF
    #define DEMO_PRINTF printf
#endif

#define DEMO_LOG_INFO(fmt, ...) \
    DEMO_PRINTF("[DEMO_INFO] " fmt "\r\n", ##__VA_ARGS__)
#define DEMO_LOG_DBG(fmt, ...) \
    DEMO_PRINTF("[DEMO_DBG] " fmt "\r\n", ##__VA_ARGS__)
#define DEMO_LOG_ERR(fmt, ...) \
    DEMO_PRINTF("[DEMO_ERR] " fmt "\r\n", ##__VA_ARGS__)

/*================================================================*/
/* 类型定义 */
/*================================================================*/
/** @brief LED状态枚举 */
typedef enum
{
    LIGHT_OFF = 0,      /*!< 灯光关闭 */
    LIGHT_ON,           /*!< 灯光常亮 */
    LIGHT_BLINKING,     /*!< 灯光闪烁 */
    LIGHT_STATE_COUNT
} light_state_t;

/** @brief LED事件枚举 */
typedef enum
{
    EVENT_TURN_ON = 0,  /*!< 打开灯光 */
    EVENT_TURN_OFF,     /*!< 关闭灯光 */
    EVENT_TOGGLE,       /*!< 切换灯光 */
    EVENT_SET_BLINK,    /*!< 设置为闪烁模式 */
    EVENT_TIMER_TICK,   /*!< 定时器滴答（用于闪烁） */
    LIGHT_EVENT_COUNT
} light_event_t;

/** @brief 用户数据结构 */
typedef struct
{
    uint32_t blink_counter;     /*!< 闪烁计数器 */
    uint32_t brightness;        /*!< 亮度值（0-100） */
    uint32_t blink_interval;    /*!< 闪烁间隔（ms） */
    bool manual_control;        /*!< 手动控制标志 */
    char description[32];       /*!< 描述信息 */
} light_user_data_t;

/*================================================================*/
/* 状态处理函数 */
/*================================================================*/
/**
 * @brief 关灯状态处理函数
 */
static fsm_err_t state_off_handler(fsm_context_t* context,
                                    fsm_event_id_t event,
                                    void* event_data)
{
    fsm_err_t ret = FSM_ERR_INVALID_EVENT;
    (void)event_data;
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;

    DEMO_LOG_DBG("[OFF] Handling event: ");

    switch (event)
    {
        case EVENT_TURN_ON:
            DEMO_LOG_DBG("TURN_ON -> Switching to ON state");
            if (user_data != NULL)
            {
                user_data->brightness = 100;
                snprintf(user_data->description, sizeof(user_data->description),
                         "Turned ON from OFF");
            }
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_TOGGLE:
            DEMO_LOG_DBG("TOGGLE -> Switching to ON state");
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_SET_BLINK:
            DEMO_LOG_DBG("SET_BLINK -> Switching to BLINKING state");
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_TIMER_TICK:
            DEMO_LOG_DBG("TIMER_TICK (ignored in OFF state)");
            ret = FSM_OK;
            break;

        default:
            DEMO_LOG_DBG("UNKNOWN event %u", event);
            ret = FSM_ERR_INVALID_EVENT;
            break;
    }
    return ret;
}

/**
 * @brief 开灯状态处理函数
 */
static fsm_err_t state_on_handler(fsm_context_t* context,
                                   fsm_event_id_t event,
                                   void* event_data)
{
    fsm_err_t ret = FSM_ERR_INVALID_EVENT;
    (void)event_data;
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;

    DEMO_LOG_DBG("[ON] Handling event: ");

    switch (event)
    {
        case EVENT_TURN_OFF:
            DEMO_LOG_DBG("TURN_OFF -> Switching to OFF state");
            if (user_data != NULL)
            {
                user_data->brightness = 0;
                snprintf(user_data->description, sizeof(user_data->description),
                         "Turned OFF from ON");
            }
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_TOGGLE:
            DEMO_LOG_DBG("TOGGLE -> Switching to OFF state");
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_SET_BLINK:
            DEMO_LOG_DBG("SET_BLINK -> Switching to BLINKING state");
            if (user_data != NULL)
            {
                snprintf(user_data->description, sizeof(user_data->description),
                         "Entering blink mode");
            }
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_TIMER_TICK:
            DEMO_LOG_DBG("TIMER_TICK (ignored in ON state)");
            ret = FSM_OK;
            break;

        default:
            DEMO_LOG_DBG("UNKNOWN event %u", event);
            ret = FSM_ERR_INVALID_EVENT;
            break;
    }
    return ret;
}

/**
 * @brief 闪烁状态处理函数
 */
static fsm_err_t state_blinking_handler(fsm_context_t* context,
                                         fsm_event_id_t event,
                                         void* event_data)
{
    fsm_err_t ret = FSM_ERR_INVALID_EVENT;
    (void)event_data;
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;

    DEMO_LOG_DBG("[BLINKING] Handling event: ");

    switch (event)
    {
        case EVENT_TURN_OFF:
            DEMO_LOG_DBG("TURN_OFF -> Switching to OFF state");
            if (user_data != NULL)
            {
                user_data->blink_counter = 0;
                user_data->brightness = 0;
                snprintf(user_data->description, sizeof(user_data->description),
                         "Blinking stopped, light OFF");
            }
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_TURN_ON:
            DEMO_LOG_DBG("TURN_ON -> Switching to ON state");
            if (user_data != NULL)
            {
                user_data->blink_counter = 0;
                user_data->brightness = 100;
                snprintf(user_data->description, sizeof(user_data->description),
                         "Blinking stopped, light ON");
            }
            ret = FSM_ERR_INVALID_EVENT;
            break;

        case EVENT_TIMER_TICK:
            /* 处理闪烁逻辑 */
            if (user_data != NULL)
            {
                user_data->blink_counter++;

                if (user_data->blink_counter % 2 == 0)
                {
                    user_data->brightness = 100;
                    DEMO_LOG_DBG("TIMER_TICK -> Light ON (blink %u)", user_data->blink_counter);
                }
                else
                {
                    user_data->brightness = 0;
                    DEMO_LOG_DBG("TIMER_TICK -> Light OFF (blink %u)", user_data->blink_counter);
                }

                /* 闪烁10次后自动停止 */
                if (user_data->blink_counter >= 20)
                {
                    DEMO_LOG_DBG("Auto-stopping blink after 10 cycles");
                    user_data->manual_control = false;
                    ret = FSM_ERR_INVALID_EVENT;  /* 触发自动转移 */
                }
                else
                {
                    ret = FSM_OK;  /* 已处理，不触发状态转移 */
                }
            }
            break;

        default:
            DEMO_LOG_DBG("UNKNOWN event %u", event);
            ret = FSM_ERR_INVALID_EVENT;
            break;
    }
    return ret;
}

/*================================================================*/
/* 状态进入/退出函数 */
/*================================================================*/
/**
 * @brief 进入关灯状态
 */
static void on_enter_off(fsm_context_t* context)
{
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;
    if (user_data != NULL)
    {
        user_data->brightness = 0;
        snprintf(user_data->description, sizeof(user_data->description),
                 "Light is OFF");
    }
    DEMO_LOG_DBG("-> Light turned OFF");
}

/**
 * @brief 进入开灯状态
 */
static void on_enter_on(fsm_context_t* context)
{
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;
    if (user_data != NULL)
    {
        user_data->brightness = 100;
        user_data->blink_counter = 0;
        snprintf(user_data->description, sizeof(user_data->description),
                 "Light is ON at %u%% brightness", user_data->brightness);
    }
    DEMO_LOG_DBG("-> Light turned ON");
}

/**
 * @brief 进入闪烁状态
 */
static void on_enter_blinking(fsm_context_t* context)
{
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;
    if (user_data != NULL)
    {
        user_data->blink_counter = 0;
        user_data->brightness = 100;
        user_data->manual_control = true;
        snprintf(user_data->description, sizeof(user_data->description),
                 "Light is BLINKING (interval: %ums)", user_data->blink_interval);
    }
    DEMO_LOG_DBG("-> Light started BLINKING");
}

/**
 * @brief 退出闪烁状态
 */
static void on_exit_blinking(fsm_context_t* context)
{
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;
    if (user_data != NULL)
    {
        DEMO_LOG_DBG("-> Exiting BLINKING state after %u blinks",
                     user_data->blink_counter / 2);
    }
}

/*================================================================*/
/* 守卫条件函数 */
/*================================================================*/
/**
 * @brief 检查是否可以进入闪烁模式
 */
static bool guard_can_blink(fsm_context_t* context, void* event_data)
{
    bool can = true;
    (void)event_data;
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;

    if ((user_data != NULL) && (user_data->blink_interval == 0))
    {
        DEMO_LOG_DBG("Guard: Cannot blink - interval is 0");
        can = false;
    }
    return can;
}

/*================================================================*/
/* 转移动作函数 */
/*================================================================*/
/**
 * @brief 转移动作：记录灯光变化
 */
static void action_log_light_change(fsm_context_t* context, void* event_data)
{
    (void)event_data;
    light_user_data_t* user_data = (light_user_data_t*)context->config->user_data;

    if (user_data != NULL)
    {
        DEMO_LOG_DBG("Action: Light state changed. Brightness: %u%%", user_data->brightness);
    }
}

/*================================================================*/
/* 状态表 */
/*================================================================*/
static const fsm_state_t light_states[] =
{
    /* id,          name,        handler,              on_enter,     on_exit */
    {LIGHT_OFF,     "OFF",       state_off_handler,    on_enter_off, NULL},
    {LIGHT_ON,      "ON",        state_on_handler,     on_enter_on,  NULL},
    {LIGHT_BLINKING,"BLINKING",  state_blinking_handler, on_enter_blinking, on_exit_blinking},
};

/*================================================================*/
/* 状态转移表 */
/*================================================================*/
static const fsm_transition_item_t light_transition_table[LIGHT_STATE_COUNT][LIGHT_EVENT_COUNT] =
{
    /* 状态OFF (0) */
    [LIGHT_OFF] =
    {
        /* EVENT_TURN_ON (0) */   {LIGHT_ON,      FSM_GUARD_NOP, action_log_light_change},
        /* EVENT_TURN_OFF (1) */  {LIGHT_OFF,     FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_TOGGLE (2) */    {LIGHT_ON,      FSM_GUARD_NOP, action_log_light_change},
        /* EVENT_SET_BLINK (3) */ {LIGHT_BLINKING, guard_can_blink, action_log_light_change},
        /* EVENT_TIMER_TICK (4) */{LIGHT_OFF,     FSM_GUARD_NOP, FSM_ACTION_NOP},
    },

    /* 状态ON (1) */
    [LIGHT_ON] =
    {
        /* EVENT_TURN_ON (0) */   {LIGHT_ON,      FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_TURN_OFF (1) */  {LIGHT_OFF,     FSM_GUARD_NOP, action_log_light_change},
        /* EVENT_TOGGLE (2) */    {LIGHT_OFF,     FSM_GUARD_NOP, action_log_light_change},
        /* EVENT_SET_BLINK (3) */ {LIGHT_BLINKING, guard_can_blink, action_log_light_change},
        /* EVENT_TIMER_TICK (4) */{LIGHT_ON,      FSM_GUARD_NOP, FSM_ACTION_NOP},
    },

    /* 状态BLINKING (2) */
    [LIGHT_BLINKING] =
    {
        /* EVENT_TURN_ON (0) */   {LIGHT_ON,      FSM_GUARD_NOP, action_log_light_change},
        /* EVENT_TURN_OFF (1) */  {LIGHT_OFF,     FSM_GUARD_NOP, action_log_light_change},
        /* EVENT_TOGGLE (2) */    {LIGHT_OFF,     FSM_GUARD_NOP, action_log_light_change},
        /* EVENT_SET_BLINK (3) */ {LIGHT_ON,      FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_TIMER_TICK (4) */{LIGHT_BLINKING,FSM_GUARD_NOP, FSM_ACTION_NOP},
    },
};

/*================================================================*/
/* 辅助函数 */
/*================================================================*/
/**
 * @brief 打印灯光信息
 * @param data 用户数据
 */
static void print_light_info(light_user_data_t* data)
{
    if (data != NULL)
    {
        DEMO_PRINTF("\r\n--- Light Info ---\r\n");
        DEMO_PRINTF("Brightness: %u%%\r\n", data->brightness);
        DEMO_PRINTF("Blink Counter: %u\r\n", data->blink_counter);
        DEMO_PRINTF("Blink Interval: %u ms\r\n", data->blink_interval);
        DEMO_PRINTF("Manual Control: %s\r\n", data->manual_control ? "Yes" : "No");
        DEMO_PRINTF("Description: %s\r\n", data->description);
        DEMO_PRINTF("-----------------\r\n");
    }
}

/*================================================================*/
/* 示例函数 */
/*================================================================*/
/**
 * @brief 基本使用示例
 */
void example_basic_usage(void)
{
    fsm_context_t fsm_context;
    light_user_data_t user_data =
    {
        .blink_counter = 0,
        .brightness = 0,
        .blink_interval = 500,
        .manual_control = false,
        .description = "Initial state"
    };
    fsm_config_t config =
    {
        .states = light_states,
        .state_count = LIGHT_STATE_COUNT,
        .event_count = LIGHT_EVENT_COUNT,
        .transition_table = (const fsm_transition_item_t*)light_transition_table,
        .user_data = &user_data,
        .name = "LightController"
    };
    fsm_handle_t light_fsm = NULL;
    fsm_stats_t stats;
    int i = 0;

    DEMO_LOG_INFO("=== Example 1: Basic Light Control ===");

    light_fsm = fsm_create(&config, &fsm_context);
    if (light_fsm == NULL)
    {
        DEMO_LOG_ERR("Failed to create FSM!");
        return;
    }

    DEMO_LOG_INFO("Light FSM created. Initial state: %s",
                  fsm_get_state_name(light_fsm, fsm_get_current_state(light_fsm)));
    print_light_info(&user_data);

    DEMO_LOG_INFO("Test 1: Turning light ON");
    fsm_process_event(light_fsm, EVENT_TURN_ON, NULL);
    print_light_info(&user_data);

    DEMO_LOG_INFO("Test 2: Toggling light");
    fsm_process_event(light_fsm, EVENT_TOGGLE, NULL);
    print_light_info(&user_data);

    DEMO_LOG_INFO("Test 3: Setting blink mode");
    fsm_process_event(light_fsm, EVENT_SET_BLINK, NULL);
    print_light_info(&user_data);

    DEMO_LOG_INFO("Test 4: Simulating timer ticks for blinking");
    for (i = 0; i < 5; i++)
    {
        DEMO_PRINTF("Timer tick %d: ", i + 1);
        fsm_process_event(light_fsm, EVENT_TIMER_TICK, NULL);
        print_light_info(&user_data);
        sleep_ms(300);
    }

    DEMO_LOG_INFO("Test 5: Turning light OFF during blinking");
    fsm_process_event(light_fsm, EVENT_TURN_OFF, NULL);
    print_light_info(&user_data);

    if (fsm_get_stats(light_fsm, &stats) == FSM_OK)
    {
        DEMO_LOG_INFO("FSM Statistics:");
        DEMO_LOG_INFO("  Events processed: %u", stats.event_count);
        DEMO_LOG_INFO("  Transitions: %u", stats.transition_count);
        DEMO_LOG_INFO("  Guard rejections: %u", stats.guard_rejections);
    }

    fsm_destroy(light_fsm);
    DEMO_LOG_INFO("Light FSM destroyed.");
}

/**
 * @brief 高级功能示例（守卫、强制转移等）
 */
void example_advanced_features(void)
{
    fsm_context_t fsm_context;
    light_user_data_t user_data =
    {
        .blink_counter = 0,
        .brightness = 0,
        .blink_interval = 100,
        .manual_control = false,
        .description = "Advanced example"
    };
    fsm_config_t config =
    {
        .states = light_states,
        .state_count = LIGHT_STATE_COUNT,
        .event_count = LIGHT_EVENT_COUNT,
        .transition_table = (const fsm_transition_item_t*)light_transition_table,
        .user_data = &user_data,
        .name = "AdvancedLightController"
    };
    fsm_handle_t light_fsm = NULL;
    fsm_stats_t stats;
    const fsm_transition_item_t* item = NULL;

    DEMO_LOG_INFO("=== Example 2: Advanced Features ===");

    light_fsm = fsm_create(&config, &fsm_context);
    if (light_fsm == NULL)
    {
        DEMO_LOG_ERR("Failed to create FSM!");
        return;
    }

    DEMO_LOG_INFO("Advanced Light FSM created.");

    DEMO_LOG_INFO("Test 1: Testing guard condition (invalid blink interval)");
    user_data.blink_interval = 0;
    fsm_process_event(light_fsm, EVENT_SET_BLINK, NULL);
    DEMO_PRINTF("Current state: %s (should still be OFF)\r\n",
                fsm_get_state_name(light_fsm, fsm_get_current_state(light_fsm)));

    if ((fsm_get_stats(light_fsm, &stats) == FSM_OK) && (stats.guard_rejections > 0))
    {
        DEMO_LOG_INFO("  Guard was rejected (expected)");
    }

    DEMO_LOG_INFO("Test 2: Testing force state");
    fsm_force_state(light_fsm, LIGHT_ON);
    DEMO_PRINTF("Forced state to ON. Current state: %s\r\n",
                fsm_get_state_name(light_fsm, fsm_get_current_state(light_fsm)));

    DEMO_PRINTF("Previous state: %s\r\n",
                fsm_get_state_name(light_fsm, fsm_get_previous_state(light_fsm)));

    DEMO_PRINTF("Is in ON state? %s\r\n",
                fsm_is_in_state(light_fsm, LIGHT_ON) ? "Yes" : "No");
    DEMO_PRINTF("Is in OFF state? %s\r\n",
                fsm_is_in_state(light_fsm, LIGHT_OFF) ? "Yes" : "No");

    DEMO_LOG_INFO("Test 3: Testing 2D array access");
    item = fsm_get_transition_item(&config, LIGHT_OFF, EVENT_TURN_ON);
    if ((item != NULL) && (item->next_state == LIGHT_ON))
    {
        DEMO_LOG_INFO("  OFF + TURN_ON -> ON: OK");
    }

    item = fsm_get_transition_item(&config, LIGHT_OFF, EVENT_SET_BLINK);
    if ((item != NULL) && (item->guard != FSM_GUARD_NOP))
    {
        DEMO_LOG_INFO("  OFF + SET_BLINK has guard: OK");
    }

    fsm_destroy(light_fsm);
    DEMO_LOG_INFO("Advanced Light FSM destroyed.");
}

/*================================================================*/
/* 主函数 */
/*================================================================*/
int main(int argc, const char* argv[])
{
    int ret = 0;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    system("title FSM Light Controller Demo - 2D Array Version");
#endif

    DEMO_PRINTF("=== FSM Framework Simple Example ===\r\n");
    DEMO_PRINTF("Light Controller State Machine Demo (2D Array Version)\r\n");

    example_basic_usage();
    DEMO_PRINTF("\r\nPress Enter to continue to advanced features...");
    getchar();

    example_advanced_features();

    DEMO_PRINTF("\r\n=== All Examples Completed ===\r\n");

#ifdef _WIN32
    DEMO_PRINTF("\r\nPress Enter to exit...");
    getchar();
#endif

    return ret;
}
