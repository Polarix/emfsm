/**
 * @file fsm.h
 * @brief 二维表形式的有限状态机框架（MCU适用）- 无超时版本
 * @version 2.1.0
 */

#ifndef FSM_H
#define FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*============================================================================*
 *                                编译器兼容性宏                               *
 *============================================================================*/
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    #define FSM_PACKED_BEGIN __PACKED
    #define FSM_PACKED_END
    #define FSM_INLINE static __inline
    #define FSM_WEAK __attribute__((weak))
#elif defined(__ARMCC_VERSION)
    #define FSM_PACKED_BEGIN __packed
    #define FSM_PACKED_END
    #define FSM_INLINE static __inline
    #define FSM_WEAK __attribute__((weak))
#elif defined(__ICCARM__)
    #define FSM_PACKED_BEGIN __packed
    #define FSM_PACKED_END
    #define FSM_INLINE static inline
    #define FSM_WEAK __weak
#elif defined(__GNUC__)
    #define FSM_PACKED_BEGIN
    #define FSM_PACKED_END __attribute__((packed))
    #define FSM_INLINE static inline
    #define FSM_WEAK __attribute__((weak))
#else
    #define FSM_PACKED_BEGIN
    #define FSM_PACKED_END
    #define FSM_INLINE static
    #define FSM_WEAK
#endif

/*============================================================================*
 *                                平台适配宏                                   *
 *============================================================================*/
#ifndef FSM_API
    #define FSM_API
#endif

#ifndef FSM_PRIVATE
    #define FSM_PRIVATE static
#endif

/*============================================================================*
 *                                  日志宏                                     *
 *============================================================================*/
#ifndef FSM_PRINTF
    #define FSM_PRINTF(...)
#endif

#ifndef FSM_LOG_LEVEL
    #define FSM_LOG_LEVEL 0
#endif

#if FSM_LOG_LEVEL >= 1
    #define FSM_LOG_ERR(fmt, ...) FSM_PRINTF("[FSM_ERR] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_ERR(fmt, ...)
#endif

#if FSM_LOG_LEVEL >= 2
    #define FSM_LOG_WRN(fmt, ...) FSM_PRINTF("[FSM_WRN] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_WRN(fmt, ...)
#endif

#if FSM_LOG_LEVEL >= 3
    #define FSM_LOG_INFO(fmt, ...) FSM_PRINTF("[FSM_INFO] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_INFO(fmt, ...)
#endif

#if FSM_LOG_LEVEL >= 4
    #define FSM_LOG_DBG(fmt, ...) FSM_PRINTF("[FSM_DBG] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define FSM_LOG_DBG(fmt, ...)
#endif

/*============================================================================*
 *                                空动作宏                                     *
 *============================================================================*/
#define FSM_ACTION_NOP  NULL
#define FSM_GUARD_NOP   NULL

/*============================================================================*
 *                                 错误码                                      *
 *============================================================================*/
typedef enum {
    FSM_OK                    = 0,
    FSM_ERR_INVALID_PARAM     = -1,
    FSM_ERR_INVALID_STATE     = -2,
    FSM_ERR_INVALID_EVENT     = -3,
    FSM_ERR_TRANSITION_FAILED = -4,
    FSM_ERR_NOT_INITIALIZED   = -5,
    FSM_ERR_HANDLER_NULL      = -6,
    FSM_ERR_NO_TRANSITION     = -7,
    FSM_ERR_GUARD_REJECTED    = -8,
} fsm_err_t;

/*============================================================================*
 *                                类型定义                                     *
 *============================================================================*/
typedef uint16_t fsm_state_id_t;
typedef uint16_t fsm_event_id_t;

#ifndef FSM_MAX_STATES
#define FSM_MAX_STATES 32
#endif

#ifndef FSM_MAX_EVENTS
#define FSM_MAX_EVENTS 32
#endif

typedef void* fsm_handle_t;
struct fsm_context;

/* 函数指针类型定义 */
typedef fsm_err_t (*fsm_state_handler_t)(struct fsm_context* context,
                                          fsm_event_id_t event,
                                          void* event_data);

typedef void (*fsm_state_action_t)(struct fsm_context* context);

typedef bool (*fsm_guard_t)(struct fsm_context* context, void* event_data);

typedef void (*fsm_transition_action_t)(struct fsm_context* context,
                                         void* event_data);

/*============================================================================*
 *                                数据结构                                     *
 *============================================================================*/

/**
 * @brief 二维表转移项结构
 */
FSM_PACKED_BEGIN typedef struct {
    fsm_state_id_t next_state;
    fsm_guard_t guard;
    fsm_transition_action_t action;
} FSM_PACKED_END fsm_transition_item_t;

/**
 * @brief FSM状态描述结构（无超时字段）
 */
FSM_PACKED_BEGIN typedef struct {
    fsm_state_id_t id;
    const char* name;
    fsm_state_handler_t handler;
    fsm_state_action_t on_enter;
    fsm_state_action_t on_exit;
} FSM_PACKED_END fsm_state_t;

/**
 * @brief FSM配置结构（使用二维数组）
 */
typedef struct {
    const fsm_state_t* states;
    uint16_t state_count;
    uint16_t event_count;
    const fsm_transition_item_t* transition_table;
    void* user_data;
    const char* name;
} fsm_config_t;

/**
 * @brief FSM上下文结构（无超时相关字段）
 */
typedef struct fsm_context {
    fsm_state_id_t current_state;
    fsm_state_id_t previous_state;
    const fsm_config_t* config;
    uint32_t event_count;
    uint32_t transition_count;
    uint32_t guard_rejection_count;
    bool initialized;
    bool processing;
} fsm_context_t;

/**
 * @brief FSM统计信息结构（无超时统计）
 */
typedef struct {
    uint32_t event_count;
    uint32_t transition_count;
    uint32_t error_count;
    uint32_t guard_rejections;
} fsm_stats_t;

/*============================================================================*
 *                                函数声明                                     *
 *============================================================================*/

FSM_WEAK void fsm_action_nop(fsm_context_t* context, void* event_data);

FSM_API fsm_handle_t fsm_create(const fsm_config_t* config, fsm_context_t* context);

FSM_API void fsm_destroy(fsm_handle_t handle);

FSM_API fsm_err_t fsm_process_event(fsm_handle_t handle,
                                     fsm_event_id_t event,
                                     void* event_data);

FSM_API fsm_err_t fsm_force_state(fsm_handle_t handle, fsm_state_id_t state);

FSM_API fsm_state_id_t fsm_get_current_state(fsm_handle_t handle);

FSM_API fsm_state_id_t fsm_get_previous_state(fsm_handle_t handle);

FSM_API const char* fsm_get_state_name(fsm_handle_t handle, fsm_state_id_t state);

FSM_API bool fsm_is_in_state(fsm_handle_t handle, fsm_state_id_t state);

FSM_API fsm_err_t fsm_get_stats(fsm_handle_t handle, fsm_stats_t* stats);

FSM_API fsm_err_t fsm_reset_stats(fsm_handle_t handle);

FSM_API fsm_context_t* fsm_get_context(fsm_handle_t handle);

/**
 * @brief 获取二维转移表项的指针
 */
FSM_INLINE const fsm_transition_item_t* fsm_get_transition_item(
    const fsm_config_t* config,
    fsm_state_id_t state,
    fsm_event_id_t event)
{
    if (config == NULL || config->transition_table == NULL) {
        return NULL;
    }
    if (state >= config->state_count || event >= config->event_count) {
        return NULL;
    }
    return &config->transition_table[state * config->event_count + event];
}

#ifdef FSM_ENABLE_DEBUG
FSM_API void fsm_print_info(fsm_handle_t handle);
FSM_API void fsm_print_transition_table(fsm_handle_t handle);
#endif /* FSM_ENABLE_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */
