/**
 * @file test_fsm.c
 * @brief FSM框架测试代码 - 使用二维数组版本（无超时）
 */

/*================================================================*/
/* 头文件包含 */
/*================================================================*/
#include "fsm.h"
#include <stdio.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#endif

/*================================================================*/
/* 宏定义 */
/*================================================================*/
/* 测试程序独立定义自己的日志宏，与框架解耦 */
#ifndef TEST_PRINTF
    #define TEST_PRINTF printf
#endif

#define TEST_LOG_INFO(fmt, ...) \
    TEST_PRINTF("[TEST_INFO] " fmt "\n", ##__VA_ARGS__)
#define TEST_LOG_DBG(fmt, ...) \
    TEST_PRINTF("[TEST_DBG] " fmt "\n", ##__VA_ARGS__)
#define TEST_LOG_ERR(fmt, ...) \
    TEST_PRINTF("[TEST_ERR] " fmt "\n", ##__VA_ARGS__)

#ifdef _WIN32
#define CLEAR_SCREEN() system("cls")
#else
#define CLEAR_SCREEN() system("clear")
#endif

/*================================================================*/
/* 类型定义 */
/*================================================================*/
/** @brief 测试状态枚举 */
typedef enum
{
    STATE_IDLE = 0,     /*!< 空闲状态 */
    STATE_ACTIVE,       /*!< 活跃状态 */
    STATE_PAUSED,       /*!< 暂停状态 */
    STATE_ERROR,        /*!< 错误状态 */
    STATE_COUNT
} test_state_t;

/** @brief 测试事件枚举 */
typedef enum
{
    EVENT_START = 0,    /*!< 启动事件 */
    EVENT_PAUSE,        /*!< 暂停事件 */
    EVENT_RESUME,       /*!< 恢复事件 */
    EVENT_STOP,         /*!< 停止事件 */
    EVENT_ERROR,        /*!< 错误事件 */
    EVENT_RESET,        /*!< 复位事件 */
    EVENT_COUNT
} test_event_t;

/** @brief 测试用户数据结构 */
typedef struct
{
    uint32_t counter;    /*!< 计数器 */
    char message[32];    /*!< 消息缓冲区 */
} test_user_data_t;

/*================================================================*/
/* 状态处理函数 */
/*================================================================*/
/**
 * @brief 空闲状态处理函数
 * @param context FSM上下文
 * @param event 事件ID
 * @param event_data 事件数据
 * @return 错误码，返回FSM_ERR_INVALID_EVENT让转移表处理
 */
FSM_PRIVATE fsm_err_t state_idle_handler(fsm_context_t* context,
                                          fsm_event_id_t event,
                                          void* event_data)
{
    (void)context;
    (void)event_data;

    TEST_LOG_DBG("State IDLE handling event: %u", event);
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief 活跃状态处理函数
 * @param context FSM上下文
 * @param event 事件ID
 * @param event_data 事件数据
 * @return 错误码，若事件为EVENT_ERROR则直接处理返回FSM_OK，否则返回FSM_ERR_INVALID_EVENT
 */
FSM_PRIVATE fsm_err_t state_active_handler(fsm_context_t* context,
                                            fsm_event_id_t event,
                                            void* event_data)
{
    fsm_err_t ret = FSM_ERR_INVALID_EVENT;

    (void)event_data;

    TEST_LOG_DBG("State ACTIVE handling event: %u", event);

    /* 直接处理错误事件，不触发转移 */
    if (event == EVENT_ERROR)
    {
        test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;
        if (user_data != NULL)
        {
            snprintf(user_data->message, sizeof(user_data->message),
                     "Error in active state");
        }
        ret = FSM_OK;
    }
    return ret;
}

/*================================================================*/
/* 状态进入/退出函数 */
/*================================================================*/

/**
 * @brief 进入ACTIVE状态回调
 * @param context FSM上下文
 */
FSM_PRIVATE void on_enter_active(fsm_context_t* context)
{
    test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;
    if (user_data != NULL)
    {
        user_data->counter++;
        snprintf(user_data->message, sizeof(user_data->message),
                 "Entered ACTIVE state");
    }
    TEST_LOG_DBG("-> Entering ACTIVE state");
}

/**
 * @brief 退出ACTIVE状态回调
 * @param context FSM上下文
 */
FSM_PRIVATE void on_exit_active(fsm_context_t* context)
{
    test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;
    if (user_data != NULL)
    {
        snprintf(user_data->message, sizeof(user_data->message),
                 "Exited ACTIVE state");
    }
    TEST_LOG_DBG("-> Exiting ACTIVE state");
}

/**
 * @brief 进入ERROR状态回调
 * @param context FSM上下文
 */
FSM_PRIVATE void on_enter_error(fsm_context_t* context)
{
    (void)context;
    TEST_LOG_DBG("-> Entering ERROR state");
}

/*================================================================*/
/* 守卫条件函数 */
/*================================================================*/
/**
 * @brief 检查是否可以暂停
 * @param context FSM上下文
 * @param event_data 事件数据
 * @return true 允许暂停，false 拒绝暂停
 */
FSM_PRIVATE bool guard_can_pause(fsm_context_t* context, void* event_data)
{
    bool result = true;
    (void)event_data;
    test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;

    if ((user_data != NULL) && (user_data->counter > 5))
    {
        TEST_LOG_DBG("Guard condition: Cannot pause, counter > 5");
        result = false;
    }
    return result;
}

/*================================================================*/
/* 转移动作函数 */
/*================================================================*/
/**
 * @brief 转移动作：记录日志
 * @param context FSM上下文
 * @param event_data 事件数据
 */
FSM_PRIVATE void action_log_transition(fsm_context_t* context, void* event_data)
{
    (void)event_data;
    test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;

    if (user_data != NULL)
    {
        TEST_LOG_DBG("Transition action executed. Counter: %u", user_data->counter);
    }
}

/**
 * @brief 转移动作：错误发生
 * @param context FSM上下文
 * @param event_data 事件数据
 */
FSM_PRIVATE void action_error_occurred(fsm_context_t* context, void* event_data)
{
    (void)event_data;
    test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;

    if (user_data != NULL)
    {
        snprintf(user_data->message, sizeof(user_data->message), "Error occurred");
    }
    TEST_LOG_DBG("Error action executed");
}

/*================================================================*/
/* 状态表 */
/*================================================================*/
static const fsm_state_t test_states[] =
{
    /* id,        name,       handler,           on_enter, on_exit */
    {STATE_IDLE,   "IDLE",    state_idle_handler, NULL,    NULL},
    {STATE_ACTIVE, "ACTIVE",  state_active_handler, on_enter_active, on_exit_active},
    {STATE_PAUSED, "PAUSED",  NULL,               NULL,    NULL},
    {STATE_ERROR,  "ERROR",   NULL,               on_enter_error, NULL},
};

/*================================================================*/
/* 二维转移表 */
/*================================================================*/
static const fsm_transition_item_t test_transition_table[STATE_COUNT][EVENT_COUNT] =
{
    /* 状态IDLE (0) 对所有事件的处理 */
    [STATE_IDLE] =
    {
        /* EVENT_START (0) */  {STATE_ACTIVE, FSM_GUARD_NOP, action_log_transition},
        /* EVENT_PAUSE (1) */  {STATE_IDLE,   FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_RESUME (2) */ {STATE_IDLE,   FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_STOP (3) */   {STATE_IDLE,   FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_ERROR (4) */  {STATE_ERROR,  FSM_GUARD_NOP, action_error_occurred},
        /* EVENT_RESET (5) */  {STATE_IDLE,   FSM_GUARD_NOP, FSM_ACTION_NOP},
    },

    /* 状态ACTIVE (1) 对所有事件的处理 */
    [STATE_ACTIVE] =
    {
        /* EVENT_START (0) */  {STATE_ACTIVE, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_PAUSE (1) */  {STATE_PAUSED, guard_can_pause, action_log_transition},
        /* EVENT_RESUME (2) */ {STATE_ACTIVE, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_STOP (3) */   {STATE_IDLE,   FSM_GUARD_NOP, action_log_transition},
        /* EVENT_ERROR (4) */  {STATE_ERROR,  FSM_GUARD_NOP, action_error_occurred},
        /* EVENT_RESET (5) */  {STATE_IDLE,   FSM_GUARD_NOP, action_log_transition},
    },

    /* 状态PAUSED (2) 对所有事件的处理 */
    [STATE_PAUSED] =
    {
        /* EVENT_START (0) */  {STATE_PAUSED, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_PAUSE (1) */  {STATE_PAUSED, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_RESUME (2) */ {STATE_ACTIVE, FSM_GUARD_NOP, action_log_transition},
        /* EVENT_STOP (3) */   {STATE_IDLE,   FSM_GUARD_NOP, action_log_transition},
        /* EVENT_ERROR (4) */  {STATE_ERROR,  FSM_GUARD_NOP, action_error_occurred},
        /* EVENT_RESET (5) */  {STATE_IDLE,   FSM_GUARD_NOP, action_log_transition},
    },

    /* 状态ERROR (3) 对所有事件的处理 */
    [STATE_ERROR] =
    {
        /* EVENT_START (0) */  {STATE_ERROR, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_PAUSE (1) */  {STATE_ERROR, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_RESUME (2) */ {STATE_ERROR, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_STOP (3) */   {STATE_ERROR, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_ERROR (4) */  {STATE_ERROR, FSM_GUARD_NOP, FSM_ACTION_NOP},
        /* EVENT_RESET (5) */  {STATE_IDLE,  FSM_GUARD_NOP, action_log_transition},
    },
};

/* FSM配置 */
static const fsm_config_t test_fsm_config =
{
    .states = test_states,
    .state_count = STATE_COUNT,
    .event_count = EVENT_COUNT,
    .transition_table = (const fsm_transition_item_t*)test_transition_table,
    .user_data = NULL,
    .name = "TestFSM"
};

/*================================================================*/
/* 测试函数 */
/*================================================================*/
/**
 * @brief 基本操作测试
 */
void test_basic_operations(void)
{
    fsm_context_t context;
    test_user_data_t user_data = {0};
    fsm_config_t config = test_fsm_config;
    fsm_handle_t fsm = NULL;
    fsm_state_id_t current_state = 0;
    fsm_err_t ret = FSM_OK;
    fsm_stats_t stats;

    TEST_LOG_INFO("=== Test Basic Operations ===");

    config.user_data = &user_data;

    fsm = fsm_create(&config, &context);
    if (fsm == NULL)
    {
        TEST_LOG_ERR("Failed to create FSM");
        return;
    }

    TEST_LOG_INFO("FSM created successfully");

    /* 测试1: 初始状态应为IDLE */
    current_state = fsm_get_current_state(fsm);
    TEST_LOG_INFO("Test 1 - Initial state: %u (expected: %u)",
                  current_state, STATE_IDLE);

    /* 测试2: 处理START事件，应该转移到ACTIVE状态 */
    TEST_LOG_INFO("Test 2 - Processing START event:");
    ret = fsm_process_event(fsm, EVENT_START, NULL);
    TEST_LOG_INFO("  Result: %d, Current state: %u (expected: %u)",
                  ret, fsm_get_current_state(fsm), STATE_ACTIVE);

    /* 测试3: 处理无效事件 */
    TEST_LOG_INFO("Test 3 - Processing invalid event:");
    ret = fsm_process_event(fsm, EVENT_COUNT, NULL);  /* 无效事件ID */
    TEST_LOG_INFO("  Result: %d (expected error)", ret);

    /* 测试4: 处理PAUSE事件 */
    TEST_LOG_INFO("Test 4 - Processing PAUSE event:");
    ret = fsm_process_event(fsm, EVENT_PAUSE, NULL);
    TEST_LOG_INFO("  Result: %d, Current state: %u", ret, fsm_get_current_state(fsm));

    /* 测试5: 处理RESUME事件 */
    TEST_LOG_INFO("Test 5 - Processing RESUME event:");
    ret = fsm_process_event(fsm, EVENT_RESUME, NULL);
    TEST_LOG_INFO("  Result: %d, Current state: %u", ret, fsm_get_current_state(fsm));

    /* 测试6: 强制状态转移 */
    TEST_LOG_INFO("Test 6 - Force state to ERROR:");
    ret = fsm_force_state(fsm, STATE_ERROR);
    TEST_LOG_INFO("  Result: %d, Current state: %u", ret, fsm_get_current_state(fsm));

    /* 测试7: 处理RESET事件 */
    TEST_LOG_INFO("Test 7 - Processing RESET event:");
    ret = fsm_process_event(fsm, EVENT_RESET, NULL);
    TEST_LOG_INFO("  Result: %d, Current state: %u", ret, fsm_get_current_state(fsm));

    /* 测试统计信息 */
    ret = fsm_get_stats(fsm, &stats);
    if (ret == FSM_OK)
    {
        TEST_LOG_INFO("Statistics:");
        TEST_LOG_INFO("  Events processed: %u", stats.event_count);
        TEST_LOG_INFO("  Transitions: %u", stats.transition_count);
        TEST_LOG_INFO("  Guard rejections: %u", stats.guard_rejections);
    }

    fsm_destroy(fsm);
    TEST_LOG_INFO("FSM destroyed");
}

/**
 * @brief 守卫条件测试
 */
void test_guard_conditions(void)
{
    fsm_context_t context;
    test_user_data_t user_data = {0};
    fsm_config_t config = test_fsm_config;
    fsm_handle_t fsm = NULL;
    fsm_err_t ret = FSM_OK;
    fsm_stats_t stats;

    TEST_LOG_INFO("=== Test Guard Conditions ===");

    config.user_data = &user_data;

    fsm = fsm_create(&config, &context);
    if (fsm == NULL)
    {
        TEST_LOG_ERR("Failed to create FSM");
        return;
    }

    /* 转移到ACTIVE状态 */
    fsm_process_event(fsm, EVENT_START, NULL);

    /* 设置计数器为3，应该可以通过守卫条件 */
    user_data.counter = 3;
    TEST_LOG_INFO("Test with counter = 3:");
    ret = fsm_process_event(fsm, EVENT_PAUSE, NULL);
    TEST_LOG_INFO("  Pause result: %d (0=OK, -8=Guard rejected)", ret);

    /* 恢复状态 */
    fsm_process_event(fsm, EVENT_RESUME, NULL);

    /* 设置计数器为10，应该被守卫条件拒绝 */
    user_data.counter = 10;
    TEST_LOG_INFO("Test with counter = 10:");
    ret = fsm_process_event(fsm, EVENT_PAUSE, NULL);
    TEST_LOG_INFO("  Pause result: %d (0=OK, -8=Guard rejected)", ret);

    /* 检查守卫拒绝计数 */
    if (fsm_get_stats(fsm, &stats) == FSM_OK)
    {
        TEST_LOG_INFO("  Guard rejections counted: %u", stats.guard_rejections);
    }

    fsm_destroy(fsm);
}

/* 超时测试函数已移除 */

/**
 * @brief 错误处理测试
 */
void test_error_handling(void)
{
    fsm_context_t context;
    test_user_data_t user_data = {0};
    fsm_config_t config = test_fsm_config;
    fsm_handle_t fsm = NULL;
    fsm_err_t ret = FSM_OK;

    TEST_LOG_INFO("=== Test Error Handling ===");

    /* 测试1: 无效参数 */
    TEST_LOG_INFO("Test 1 - Invalid parameters:");
    fsm = fsm_create(NULL, &context);
    TEST_PRINTF("  Create with NULL config: %s\n",
                fsm == NULL ? "FAILED (expected)" : "SUCCESS (unexpected)");

    /* 测试2: 未初始化的FSM */
    TEST_LOG_INFO("Test 2 - Uninitialized FSM:");
    ret = fsm_process_event(NULL, EVENT_START, NULL);
    TEST_PRINTF("  Process event on NULL handle: %d (expected: %d)\n",
                ret, FSM_ERR_NOT_INITIALIZED);

    /* 测试3: 处理事件后销毁 */
    TEST_LOG_INFO("Test 3 - Destroy after use:");
    config.user_data = &user_data;

    fsm = fsm_create(&config, &context);
    if (fsm != NULL)
    {
        fsm_process_event(fsm, EVENT_START, NULL);
        fsm_destroy(fsm);
        TEST_PRINTF("  Destroy successful\n");

        /* 尝试使用已销毁的FSM */
        ret = fsm_process_event(fsm, EVENT_STOP, NULL);
        TEST_PRINTF("  Process event after destroy: %d (expected: %d)\n",
                    ret, FSM_ERR_NOT_INITIALIZED);
    }
}

/**
 * @brief 二维数组访问测试
 */
void test_2d_array_access(void)
{
    const fsm_transition_item_t* item = NULL;

    TEST_LOG_INFO("=== Test 2D Array Access ===");

    item = fsm_get_transition_item(&test_fsm_config, STATE_IDLE, EVENT_START);
    TEST_PRINTF("IDLE + START -> ACTIVE? %s\n",
                (item != NULL && item->next_state == STATE_ACTIVE) ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_IDLE, EVENT_PAUSE);
    TEST_PRINTF("IDLE + PAUSE -> IDLE (self-loop)? %s\n",
                (item != NULL && item->next_state == STATE_IDLE) ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_ACTIVE, EVENT_PAUSE);
    TEST_PRINTF("ACTIVE + PAUSE guard exists? %s\n",
                (item != NULL && item->guard != FSM_GUARD_NOP) ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_ACTIVE, EVENT_STOP);
    TEST_PRINTF("ACTIVE + STOP action exists? %s\n",
                (item != NULL && item->action != FSM_ACTION_NOP) ? "YES" : "NO");

    /* 测试边界情况 */
    item = fsm_get_transition_item(&test_fsm_config, STATE_COUNT, EVENT_START);
    TEST_PRINTF("Invalid state access returns NULL? %s\n", item == NULL ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_IDLE, EVENT_COUNT);
    TEST_PRINTF("Invalid event access returns NULL? %s\n", item == NULL ? "YES" : "NO");
}

#ifdef _WIN32
/**
 * @brief 设置控制台颜色（Windows）
 * @param color 颜色值
 */
void set_console_color(int color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}
#endif

/**
 * @brief 打印测试标题
 */
void print_banner(void)
{
#ifdef _WIN32
    set_console_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#endif

    TEST_PRINTF("============================================\n");
    TEST_PRINTF("    FSM Framework Test - 2D Array Version   \n");
    TEST_PRINTF("============================================\n");

#ifdef _WIN32
    set_console_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
}

/*================================================================*/
/* 主函数 */
/*================================================================*/
int main(void)
{
    int ret = 0;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    system("title FSM Framework Test - 2D Array Version");
#endif

    print_banner();

    TEST_PRINTF("\nTesting FSM with 2D transition table...\n");

    test_basic_operations();
    TEST_PRINTF("\nPress Enter to continue to guard condition tests...");
    getchar();

    test_guard_conditions();
    TEST_PRINTF("\nPress Enter to continue to error handling tests...");
    getchar();

    test_error_handling();
    TEST_PRINTF("\nPress Enter to continue to 2D array access tests...");
    getchar();

    test_2d_array_access();

#ifdef _WIN32
    set_console_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#endif

    TEST_PRINTF("\n");
    TEST_PRINTF("============================================\n");
    TEST_PRINTF("          All Tests Completed!              \n");
    TEST_PRINTF("============================================\n");

#ifdef _WIN32
    set_console_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    TEST_PRINTF("Press Enter to exit...\n");
    getchar();
#endif

    return ret;
}
