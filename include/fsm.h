/**
 * @file fsm.h
 * @brief 二维表形式的有限状态机框架（MCU适用）
 * @version 2.0.0
 * @date 2024
 *
 * @details
 * 使用二维表实现FSM，支持：
 * - 状态转移表驱动（使用二维数组）
 * - 事件处理
 * - 状态进入/退出动作
 * - 无动态内存分配
 * - 线程安全设计（单线程使用）
 *
 * 遵循C99标准，无任何C++语法
 */

#ifndef FSM_H
#define FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*================================================================*/
/* 编译器兼容性宏                                                 */
/*================================================================*/
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    /* ARM Compiler 6 */
    #define FSM_PACKED_BEGIN __PACKED
    #define FSM_PACKED_END
    #define FSM_INLINE static __inline
    #define FSM_WEAK __attribute__((weak))
#elif defined(__ARMCC_VERSION)
    /* ARM Compiler 5 */
    #define FSM_PACKED_BEGIN __packed
    #define FSM_PACKED_END
    #define FSM_INLINE static __inline
    #define FSM_WEAK __attribute__((weak))
#elif defined(__ICCARM__)
    /* IAR Compiler */
    #define FSM_PACKED_BEGIN __packed
    #define FSM_PACKED_END
    #define FSM_INLINE static inline
    #define FSM_WEAK __weak
#elif defined(__GNUC__)
    /* GCC and Clang */
    #define FSM_PACKED_BEGIN
    #define FSM_PACKED_END __attribute__((packed))
    #define FSM_INLINE static inline
    #define FSM_WEAK __attribute__((weak))
#else
    /* Unknown compiler - use defaults */
    #define FSM_PACKED_BEGIN
    #define FSM_PACKED_END
    #define FSM_INLINE static
    #define FSM_WEAK
#endif

/*================================================================*/
/* 平台适配宏                                                     */
/*================================================================*/
#ifndef FSM_API
    #define FSM_API
#endif

#ifndef FSM_PRIVATE
    #define FSM_PRIVATE static
#endif

/*================================================================*/
/* 日志宏                                                         */
/*================================================================*/
/**
 * @brief 用户可通过定义 FSM_PRINTF 宏来重定向日志输出。
 *        例如：#define FSM_PRINTF(...)  printf(__VA_ARGS__)
 *        若未定义，则默认不输出任何日志。
 */
#ifndef FSM_PRINTF
    #define FSM_PRINTF(...)   /* 空实现 */
#endif

/**
 * @brief 日志级别控制宏。
 *        用户可通过定义 FSM_LOG_LEVEL 来控制输出哪些日志：
 *        0：关闭所有日志
 *        1：仅错误
 *        2：错误+警告
 *        3：错误+警告+信息
 *        4：错误+警告+信息+调试
 */
#ifndef FSM_LOG_LEVEL
    #define FSM_LOG_LEVEL 0
#endif

#if FSM_LOG_LEVEL >= 1
    #define FSM_LOG_ERR(fmt, ...) \
        FSM_PRINTF("[FSM_ERR] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_ERR(fmt, ...)
#endif

#if FSM_LOG_LEVEL >= 2
    #define FSM_LOG_WRN(fmt, ...) \
        FSM_PRINTF("[FSM_WRN] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_WRN(fmt, ...)
#endif

#if FSM_LOG_LEVEL >= 3
    #define FSM_LOG_INFO(fmt, ...) \
        FSM_PRINTF("[FSM_INFO] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_INFO(fmt, ...)
#endif

#if FSM_LOG_LEVEL >= 4
    #define FSM_LOG_DBG(fmt, ...) \
        FSM_PRINTF("[FSM_DBG] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_DBG(fmt, ...)
#endif

/*================================================================*/
/* 空动作宏                                                       */
/*================================================================*/
#define FSM_ACTION_NOP  NULL
#define FSM_GUARD_NOP   NULL

/*================================================================*/
/* 错误码                                                         */
/*================================================================*/
typedef enum
{
    FSM_OK                    = 0,      /*!< 操作成功 */
    FSM_ERR_INVALID_PARAM     = -1,     /*!< 参数无效 */
    FSM_ERR_INVALID_STATE     = -2,     /*!< 状态无效 */
    FSM_ERR_INVALID_EVENT     = -3,     /*!< 事件无效 */
    FSM_ERR_TRANSITION_FAILED = -4,     /*!< 状态转移失败 */
    FSM_ERR_NOT_INITIALIZED   = -5,     /*!< FSM未初始化 */
    FSM_ERR_HANDLER_NULL      = -6,     /*!< 处理函数为空 */
    FSM_ERR_NO_TRANSITION     = -7,     /*!< 无有效转移 */
    FSM_ERR_GUARD_REJECTED    = -8,     /*!< 守卫条件拒绝 */
} fsm_err_t;

/*================================================================*/
/* 类型定义                                                       */
/*================================================================*/
typedef uint16_t fsm_state_id_t;        /*!< 状态ID类型 */
typedef uint16_t fsm_event_id_t;        /*!< 事件ID类型 */

#ifndef FSM_MAX_STATES
#define FSM_MAX_STATES 32               /*!< 最大状态数 */
#endif

#ifndef FSM_MAX_EVENTS
#define FSM_MAX_EVENTS 32               /*!< 最大事件数 */
#endif

typedef void* fsm_handle_t;             /*!< FSM句柄 */

struct fsm_context;                     /*!< 前向声明 */

/* 函数指针类型定义 */
typedef fsm_err_t (*fsm_state_handler_t)(struct fsm_context* context,
                                          fsm_event_id_t event,
                                          void* event_data);   /*!< 状态处理函数 */

typedef void (*fsm_state_action_t)(struct fsm_context* context); /*!< 状态进入/退出动作 */

typedef bool (*fsm_guard_t)(struct fsm_context* context, void* event_data); /*!< 守卫条件 */

typedef void (*fsm_transition_action_t)(struct fsm_context* context,
                                         void* event_data); /*!< 转移动作 */

/*================================================================*/
/* 数据结构                                                       */
/*================================================================*/

/**
 * @brief 二维表转移项结构
 *        每个状态对应的事件处理项
 */
FSM_PACKED_BEGIN typedef struct
{
    fsm_state_id_t next_state;              /*!< 下一个状态 */
    fsm_guard_t guard;                       /*!< 守卫条件（可空） */
    fsm_transition_action_t action;          /*!< 转移动作（可空） */
} FSM_PACKED_END fsm_transition_item_t;

/**
 * @brief FSM状态描述结构
 */
FSM_PACKED_BEGIN typedef struct
{
    fsm_state_id_t id;                       /*!< 状态ID */
    const char* name;                         /*!< 状态名称（调试用） */
    fsm_state_handler_t handler;              /*!< 状态处理函数（可空） */
    fsm_state_action_t on_enter;              /*!< 进入状态时调用（可空） */
    fsm_state_action_t on_exit;                /*!< 退出状态时调用（可空） */
    uint32_t timeout_ms;                       /*!< 状态超时时间（0表示不超时） */
    fsm_state_id_t timeout_next_state;         /*!< 超时后转移到的状态 */
} FSM_PACKED_END fsm_state_t;

/**
 * @brief FSM配置结构（使用二维数组）
 */
typedef struct
{
    const fsm_state_t* states;                 /*!< 状态表 */
    uint16_t state_count;                       /*!< 状态数量 */
    uint16_t event_count;                        /*!< 事件数量 */

    /* 二维转移表：states × events
     * 结构：transition_table[state][event]
     */
    const fsm_transition_item_t* transition_table;  /*!< 转移表指针 */

    void* user_data;                             /*!< 用户数据 */
    const char* name;                             /*!< FSM名称（调试用） */
} fsm_config_t;

/**
 * @brief FSM上下文结构
 */
typedef struct fsm_context
{
    fsm_state_id_t current_state;          /*!< 当前状态ID */
    fsm_state_id_t previous_state;         /*!< 前一个状态ID */
    const fsm_config_t* config;            /*!< FSM配置 */
    uint32_t state_entry_time;             /*!< 状态进入时间（ms） */
    uint32_t event_count;                  /*!< 事件计数（调试用） */
    uint32_t transition_count;             /*!< 转移计数（调试用） */
    uint32_t guard_rejection_count;        /*!< 守卫拒绝计数 */
    uint32_t timeout_count;                 /*!< 超时计数 */
    bool initialized;                        /*!< 初始化标志 */
    bool processing;                         /*!< 正在处理标志 */
} fsm_context_t;

/**
 * @brief FSM统计信息结构
 */
typedef struct
{
    uint32_t event_count;                  /*!< 事件总数 */
    uint32_t transition_count;              /*!< 转移总数 */
    uint32_t error_count;                   /*!< 错误总数 */
    uint32_t state_timeouts;                 /*!< 状态超时次数 */
    uint32_t guard_rejections;               /*!< 守卫条件拒绝次数 */
} fsm_stats_t;

/*================================================================*/
/* 函数声明                                                       */
/*================================================================*/

/* 默认空动作函数声明 */
FSM_WEAK void fsm_action_nop(fsm_context_t* context, void* event_data);

/**
 * @brief 创建并初始化FSM
 *
 * @param config FSM配置
 * @param context FSM上下文（外部提供）
 * @return fsm_handle_t FSM句柄，失败返回NULL
 */
FSM_API fsm_handle_t fsm_create(const fsm_config_t* config, fsm_context_t* context);

/**
 * @brief 销毁FSM
 *
 * @param handle FSM句柄
 */
FSM_API void fsm_destroy(fsm_handle_t handle);

/**
 * @brief 处理FSM事件
 *
 * @param handle FSM句柄
 * @param event 事件ID
 * @param event_data 事件数据（可空）
 * @return fsm_err_t 错误码
 */
FSM_API fsm_err_t fsm_process_event(fsm_handle_t handle,
                                     fsm_event_id_t event,
                                     void* event_data);

/**
 * @brief 强制设置FSM状态（谨慎使用）
 *
 * @param handle FSM句柄
 * @param state 目标状态
 * @return fsm_err_t 错误码
 */
FSM_API fsm_err_t fsm_force_state(fsm_handle_t handle, fsm_state_id_t state);

/**
 * @brief 获取当前状态
 *
 * @param handle FSM句柄
 * @return fsm_state_id_t 当前状态ID
 */
FSM_API fsm_state_id_t fsm_get_current_state(fsm_handle_t handle);

/**
 * @brief 获取前一个状态
 *
 * @param handle FSM句柄
 * @return fsm_state_id_t 前一个状态ID
 */
FSM_API fsm_state_id_t fsm_get_previous_state(fsm_handle_t handle);

/**
 * @brief 获取状态名称
 *
 * @param handle FSM句柄
 * @param state 状态ID
 * @return const char* 状态名称，无效返回NULL
 */
FSM_API const char* fsm_get_state_name(fsm_handle_t handle, fsm_state_id_t state);

/**
 * @brief 检查是否在指定状态
 *
 * @param handle FSM句柄
 * @param state 状态ID
 * @return true 在指定状态
 * @return false 不在指定状态
 */
FSM_API bool fsm_is_in_state(fsm_handle_t handle, fsm_state_id_t state);

/**
 * @brief 处理超时检查
 *
 * @param handle FSM句柄
 * @param current_time 当前时间（ms）
 * @return fsm_err_t 错误码
 */
FSM_API fsm_err_t fsm_check_timeout(fsm_handle_t handle, uint32_t current_time);

/**
 * @brief 获取FSM统计信息
 *
 * @param handle FSM句柄
 * @param stats 统计信息输出
 * @return fsm_err_t 错误码
 */
FSM_API fsm_err_t fsm_get_stats(fsm_handle_t handle, fsm_stats_t* stats);

/**
 * @brief 重置FSM统计信息
 *
 * @param handle FSM句柄
 * @return fsm_err_t 错误码
 */
FSM_API fsm_err_t fsm_reset_stats(fsm_handle_t handle);

/**
 * @brief 获取FSM上下文
 *
 * @param handle FSM句柄
 * @return fsm_context_t* FSM上下文
 */
FSM_API fsm_context_t* fsm_get_context(fsm_handle_t handle);

/**
 * @brief 获取二维转移表项的指针
 *
 * @param config FSM配置
 * @param state 状态ID
 * @param event 事件ID
 * @return const fsm_transition_item_t* 转移项指针
 */
FSM_INLINE const fsm_transition_item_t* fsm_get_transition_item(
    const fsm_config_t* config,
    fsm_state_id_t state,
    fsm_event_id_t event)
{
    const fsm_transition_item_t* item = NULL;

    if ((config == NULL) || (config->transition_table == NULL))
    {
        return NULL;
    }

    if ((state >= config->state_count) || (event >= config->event_count))
    {
        return NULL;
    }

    /* 计算二维数组偏移量 */
    item = &config->transition_table[state * config->event_count + event];
    return item;
}

#ifdef FSM_ENABLE_DEBUG
/**
 * @brief 打印FSM信息（调试用）
 *
 * @param handle FSM句柄
 */
FSM_API void fsm_print_info(fsm_handle_t handle);

/**
 * @brief 打印状态转移表（调试用）
 *
 * @param handle FSM句柄
 */
FSM_API void fsm_print_transition_table(fsm_handle_t handle);
#endif /* FSM_ENABLE_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */
