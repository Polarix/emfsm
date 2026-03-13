/**
 * @file test_fsm.c
 * @brief FSM框架测试代码 - 使用二维数组版本
 */

/* 测试程序独立定义自己的日志宏，与框架解耦 */
#ifndef TEST_PRINTF
    #define TEST_PRINTF printf
#endif

#define TEST_LOG_INFO(fmt, ...) \
    TEST_PRINTF("[TEST_INFO] " fmt "\r\n", ##__VA_ARGS__)
#define TEST_LOG_DBG(fmt, ...) \
    TEST_PRINTF("[TEST_DBG] " fmt "\r\n", ##__VA_ARGS__)
#define TEST_LOG_ERR(fmt, ...) \
    TEST_PRINTF("[TEST_ERR] " fmt "\r\n", ##__VA_ARGS__)

#include "fsm.h"
#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#define CLEAR_SCREEN() system("cls")
#else
#define CLEAR_SCREEN() system("clear")
#endif

/*============================================================================*
 *                                状态与事件定义                               *
 *============================================================================*/
typedef enum
{
    STATE_IDLE = 0,
    STATE_ACTIVE,
    STATE_PAUSED,
    STATE_ERROR,
    STATE_COUNT
} test_state_t;

typedef enum
{
    EVENT_START = 0,
    EVENT_PAUSE,
    EVENT_RESUME,
    EVENT_STOP,
    EVENT_ERROR,
    EVENT_RESET,
    EVENT_COUNT
} test_event_t;

typedef struct
{
    uint32_t counter;
    char message[32];
} test_user_data_t;

/*============================================================================*
 *                                状态处理函数                                 *
 *============================================================================*/
FSM_PRIVATE fsm_err_t state_idle_handler(fsm_context_t* context,
                                          fsm_event_id_t event,
                                          void* event_data)
{
    (void)context;
    (void)event_data;

    TEST_LOG_DBG("State IDLE handling event: %u", event);
    return FSM_ERR_INVALID_EVENT; /* 让转移表处理 */
}

FSM_PRIVATE fsm_err_t state_active_handler(fsm_context_t* context,
                                            fsm_event_id_t event,
                                            void* event_data)
{
    (void)event_data;

    TEST_LOG_DBG("State ACTIVE handling event: %u", event);

    /* 在这里可以处理一些不需要状态转移的事件 */
    if (event == EVENT_ERROR)
    {
        /* 直接处理错误事件 */
        test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;
        if (user_data != NULL)
        {
            snprintf(user_data->message, sizeof(user_data->message),
                     "Error in active state");
        }
        return FSM_OK;
    }

    return FSM_ERR_INVALID_EVENT; /* 让转移表处理 */
}

/*============================================================================*
 *                              状态进入/退出函数                              *
 *============================================================================*/
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

FSM_PRIVATE void on_enter_error(fsm_context_t* context)
{
    (void)context;
    TEST_LOG_DBG("-> Entering ERROR state");
}

/*============================================================================*
 *                                守卫条件函数                                 *
 *============================================================================*/
FSM_PRIVATE bool guard_can_pause(fsm_context_t* context, void* event_data)
{
    (void)event_data;
    test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;

    if ((user_data != NULL) && (user_data->counter > 5))
    {
        TEST_LOG_DBG("Guard condition: Cannot pause, counter > 5");
        return false;
    }

    return true;
}

/*============================================================================*
 *                                转移动作函数                                 *
 *============================================================================*/
FSM_PRIVATE void action_log_transition(fsm_context_t* context, void* event_data)
{
    (void)event_data;
    test_user_data_t* user_data = (test_user_data_t*)context->config->user_data;

    if (user_data != NULL)
    {
        TEST_LOG_DBG("Transition action executed. Counter: %u", user_data->counter);
    }
}

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

/*============================================================================*
 *                                状态表                                       *
 *============================================================================*/
static const fsm_state_t test_states[] =
{
    /* id,        name,       handler,           on_enter, on_exit, timeout_ms, timeout_next */
    {STATE_IDLE,   "IDLE",    state_idle_handler, NULL,    NULL,    0,          0},
    {STATE_ACTIVE, "ACTIVE",  state_active_handler, on_enter_active, on_exit_active, 5000, STATE_IDLE},
    {STATE_PAUSED, "PAUSED",  NULL,               NULL,    NULL,    0,          0},
    {STATE_ERROR,  "ERROR",   NULL,               on_enter_error, NULL, 0,      0},
};

/*============================================================================*
 *                                二维转移表                                   *
 *============================================================================*/
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

/*============================================================================*
 *                                测试函数                                     *
 *============================================================================*/
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

void test_timeout_functionality(void)
{
    fsm_context_t context;
    test_user_data_t user_data = {0};
    fsm_config_t config = test_fsm_config;
    fsm_handle_t fsm = NULL;
    uint32_t current_time = 0;
    int i = 0;
    fsm_state_id_t state = 0;

    TEST_LOG_INFO("=== Test Timeout Functionality ===");

    config.user_data = &user_data;

    fsm = fsm_create(&config, &context);
    if (fsm == NULL)
    {
        TEST_LOG_ERR("Failed to create FSM");
        return;
    }

    TEST_LOG_INFO("Starting in IDLE state");

    /* 转移到ACTIVE状态（有5秒超时） */
    fsm_process_event(fsm, EVENT_START, NULL);
    TEST_LOG_INFO("Moved to ACTIVE state (will timeout to IDLE after 5 seconds)");

    /* 模拟时间流逝 */
    TEST_LOG_INFO("Simulating time...");
    current_time = 0;

    for (i = 0; i < 7; i++)
    {
        current_time += 1000; /* 增加1秒 */
        fsm_check_timeout(fsm, current_time);

        state = fsm_get_current_state(fsm);
        TEST_PRINTF("  Time: %u ms, State: %u", current_time, state);

        if (i == 4)
        {
            TEST_PRINTF(" (Should still be ACTIVE)");
        }
        else if (i == 5)
        {
            TEST_PRINTF(" (Should be IDLE due to timeout)");
        }
        TEST_PRINTF("\r\n");
    }

    fsm_destroy(fsm);
}

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
    TEST_PRINTF("  Create with NULL config: %s\r\n",
                fsm == NULL ? "FAILED (expected)" : "SUCCESS (unexpected)");

    /* 测试2: 未初始化的FSM */
    TEST_LOG_INFO("Test 2 - Uninitialized FSM:");
    ret = fsm_process_event(NULL, EVENT_START, NULL);
    TEST_PRINTF("  Process event on NULL handle: %d (expected: %d)\r\n",
                ret, FSM_ERR_NOT_INITIALIZED);

    /* 测试3: 处理事件后销毁 */
    TEST_LOG_INFO("Test 3 - Destroy after use:");
    config.user_data = &user_data;

    fsm = fsm_create(&config, &context);
    if (fsm != NULL)
    {
        fsm_process_event(fsm, EVENT_START, NULL);
        fsm_destroy(fsm);
        TEST_PRINTF("  Destroy successful\r\n");

        /* 尝试使用已销毁的FSM */
        ret = fsm_process_event(fsm, EVENT_STOP, NULL);
        TEST_PRINTF("  Process event after destroy: %d (expected: %d)\r\n",
                    ret, FSM_ERR_NOT_INITIALIZED);
    }
}

void test_2d_array_access(void)
{
    const fsm_transition_item_t* item = NULL;

    TEST_LOG_INFO("=== Test 2D Array Access ===");

    item = fsm_get_transition_item(&test_fsm_config, STATE_IDLE, EVENT_START);
    TEST_PRINTF("IDLE + START -> ACTIVE? %s\r\n",
                (item != NULL && item->next_state == STATE_ACTIVE) ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_IDLE, EVENT_PAUSE);
    TEST_PRINTF("IDLE + PAUSE -> IDLE (self-loop)? %s\r\n",
                (item != NULL && item->next_state == STATE_IDLE) ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_ACTIVE, EVENT_PAUSE);
    TEST_PRINTF("ACTIVE + PAUSE guard exists? %s\r\n",
                (item != NULL && item->guard != FSM_GUARD_NOP) ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_ACTIVE, EVENT_STOP);
    TEST_PRINTF("ACTIVE + STOP action exists? %s\r\n",
                (item != NULL && item->action != FSM_ACTION_NOP) ? "YES" : "NO");

    /* 测试边界情况 */
    item = fsm_get_transition_item(&test_fsm_config, STATE_COUNT, EVENT_START);
    TEST_PRINTF("Invalid state access returns NULL? %s\r\n", item == NULL ? "YES" : "NO");

    item = fsm_get_transition_item(&test_fsm_config, STATE_IDLE, EVENT_COUNT);
    TEST_PRINTF("Invalid event access returns NULL? %s\r\n", item == NULL ? "YES" : "NO");
}

#ifdef _WIN32
void set_console_color(int color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}
#endif

void print_banner(void)
{
#ifdef _WIN32
    set_console_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#endif

    TEST_PRINTF("============================================\r\n");
    TEST_PRINTF("    FSM Framework Test - 2D Array Version   \r\n");
    TEST_PRINTF("============================================\r\n");

#ifdef _WIN32
    set_console_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
}

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    system("title FSM Framework Test - 2D Array Version");
#endif

    print_banner();

    TEST_PRINTF("\r\nTesting FSM with 2D transition table...\r\n");

    test_basic_operations();
    TEST_PRINTF("\r\nPress Enter to continue to guard condition tests...");
    getchar();

    test_guard_conditions();
    TEST_PRINTF("\r\nPress Enter to continue to timeout tests...");
    getchar();

    test_timeout_functionality();
    TEST_PRINTF("\r\nPress Enter to continue to error handling tests...");
    getchar();

    test_error_handling();
    TEST_PRINTF("\r\nPress Enter to continue to 2D array access tests...");
    getchar();

    test_2d_array_access();

#ifdef _WIN32
    set_console_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#endif

    TEST_PRINTF("\r\n============================================\r\n");
    TEST_PRINTF("          All Tests Completed!              \r\n");
    TEST_PRINTF("============================================\r\n");

#ifdef _WIN32
    set_console_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    TEST_PRINTF("\r\nPress Enter to exit...");
    getchar();
#endif

    return 0;
}
