/**
 * @file bms_example.c
 * @brief BMS管理系统示例（无超时版本）
 */

/*================================================================*/
/* 头文件包含 */
/*================================================================*/
#include "fsm.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define CLEAR_SCREEN() system("cls")
#else
#include <unistd.h>
#define CLEAR_SCREEN() system("clear")
#endif

/*================================================================*/
/* 宏定义 */
/*================================================================*/
/* 独立的示例日志宏 */
#ifndef DEMO_PRINTF
    #define DEMO_PRINTF printf
#endif

#define DEMO_LOG_INFO(fmt, ...) \
    DEMO_PRINTF("[INFO] " fmt "\n", ##__VA_ARGS__)
#define DEMO_LOG_DBG(fmt, ...) \
    DEMO_PRINTF("[DBG] " fmt "\n", ##__VA_ARGS__)
#define DEMO_LOG_WRN(fmt, ...) \
    DEMO_PRINTF("[WARN] " fmt "\n", ##__VA_ARGS__)
#define DEMO_LOG_ERR(fmt, ...) \
    DEMO_PRINTF("[ERR] " fmt "\n", ##__VA_ARGS__)

/*================================================================*/
/* 类型定义 */
/*================================================================*/
/** @brief BMS状态枚举 */
typedef enum
{
    STATE_UNINIT = 0,       /*!< 未初始化/混沌态 */
    STATE_INIT,             /*!< 初始化中 */
    STATE_OTA,              /*!< 系统更新(OTA) */
    STATE_IDLE,             /*!< 待机/就绪 */
    STATE_CHARGING,         /*!< 充电 */
    STATE_DISCHARGING,      /*!< 放电 */
    STATE_HEATING,          /*!< 加热 */
    STATE_ERROR,            /*!< 错误/故障 */
    STATE_SLEEP,            /*!< 休眠 */
    STATE_DEEP_SLEEP,       /*!< 深度休眠 */
    BMS_STATE_COUNT
} bms_state_t;

/** @brief BMS事件枚举 */
typedef enum
{
    EVENT_POWER_ON = 0,     /*!< 上电 */
    EVENT_INIT_DONE,        /*!< 初始化完成 */
    EVENT_OTA_START,        /*!< 开始OTA */
    EVENT_OTA_DONE,         /*!< OTA完成 */
    EVENT_OTA_FAIL,         /*!< OTA失败 */
    EVENT_CHARGE_PLUG,      /*!< 充电器插入 */
    EVENT_CHARGE_UNPLUG,    /*!< 充电器拔出 */
    EVENT_LOAD_ATTACH,      /*!< 负载接入 */
    EVENT_LOAD_DETACH,      /*!< 负载移除 */
    EVENT_TEMP_LOW,         /*!< 温度过低 */
    EVENT_TEMP_NORMAL,      /*!< 温度恢复正常 */
    EVENT_TEMP_HIGH,        /*!< 温度过高 */
    EVENT_FAULT,            /*!< 故障发生 */
    EVENT_FAULT_CLEAR,      /*!< 故障清除 */
    EVENT_SLEEP_TIMEOUT,    /*!< 休眠超时（需手动触发） */
    EVENT_WAKEUP,           /*!< 唤醒 */
    EVENT_DEEP_SLEEP_CMD,   /*!< 进入深度休眠命令 */
    BMS_EVENT_COUNT
} bms_event_t;

/** @brief 用户数据 - 模拟BMS环境参数 */
typedef struct
{
    bool charger_present;    /*!< 充电器是否插入 */
    bool load_present;       /*!< 负载是否接入 */
    int16_t temperature;     /*!< 电池温度（摄氏度） */
    uint32_t voltage;        /*!< 电池电压 (mV) */
    uint32_t current;        /*!< 电流 (mA) 正为充电，负为放电 */
    uint32_t soc;            /*!< 荷电状态 0-100% */
    uint32_t fault_code;     /*!< 故障码 */
    bool ota_in_progress;    /*!< OTA进行中标志 */

    uint32_t charger_power;  /*!< 充电器可用功率 (W) */
    uint32_t load_power;     /*!< 负载需求功率 (W) */
    uint32_t battery_power;  /*!< 电池当前允许功率 (W) */
} bms_user_data_t;

/*================================================================*/
/* 函数声明（状态处理） */
/*================================================================*/
static fsm_err_t state_uninit_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_init_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_ota_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_idle_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_charging_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_discharging_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_heating_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_error_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_sleep_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);
static fsm_err_t state_deep_sleep_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data);

/* 进入/退出动作 */
static void on_enter_uninit(fsm_context_t* ctx);
static void on_enter_init(fsm_context_t* ctx);
static void on_exit_init(fsm_context_t* ctx);
static void on_enter_ota(fsm_context_t* ctx);
static void on_exit_ota(fsm_context_t* ctx);
static void on_enter_idle(fsm_context_t* ctx);
static void on_exit_idle(fsm_context_t* ctx);
static void on_enter_charging(fsm_context_t* ctx);
static void on_exit_charging(fsm_context_t* ctx);
static void on_enter_discharging(fsm_context_t* ctx);
static void on_exit_discharging(fsm_context_t* ctx);
static void on_enter_heating(fsm_context_t* ctx);
static void on_exit_heating(fsm_context_t* ctx);
static void on_enter_error(fsm_context_t* ctx);
static void on_exit_error(fsm_context_t* ctx);
static void on_enter_sleep(fsm_context_t* ctx);
static void on_exit_sleep(fsm_context_t* ctx);
static void on_enter_deep_sleep(fsm_context_t* ctx);
static void on_exit_deep_sleep(fsm_context_t* ctx);

/* 守卫条件 */
static bool guard_can_charge(fsm_context_t* ctx, void* event_data);
static bool guard_can_discharge(fsm_context_t* ctx, void* event_data);
static bool guard_can_heat(fsm_context_t* ctx, void* event_data);
static bool guard_can_sleep(fsm_context_t* ctx, void* event_data);
static bool guard_can_deep_sleep(fsm_context_t* ctx, void* event_data);
static bool guard_can_ota(fsm_context_t* ctx, void* event_data);

/* 转移动作 */
static void action_log_transition(fsm_context_t* ctx, void* event_data);
static void action_start_charging(fsm_context_t* ctx, void* event_data);
static void action_stop_charging(fsm_context_t* ctx, void* event_data);
static void action_start_discharging(fsm_context_t* ctx, void* event_data);
static void action_stop_discharging(fsm_context_t* ctx, void* event_data);
static void action_start_heating(fsm_context_t* ctx, void* event_data);
static void action_stop_heating(fsm_context_t* ctx, void* event_data);
static void action_enter_sleep(fsm_context_t* ctx, void* event_data);
static void action_enter_deep_sleep(fsm_context_t* ctx, void* event_data);
static void action_ota_start(fsm_context_t* ctx, void* event_data);
static void action_ota_done(fsm_context_t* ctx, void* event_data);

/*================================================================*/
/* 状态表 */
/*================================================================*/
static const fsm_state_t bms_states[] =
{
    /* id,              name,           handler,                on_enter,           on_exit */
    {STATE_UNINIT,      "UNINIT",       state_uninit_handler,   on_enter_uninit,    NULL},
    {STATE_INIT,        "INIT",         state_init_handler,     on_enter_init,      on_exit_init},
    {STATE_OTA,         "OTA",          state_ota_handler,      on_enter_ota,       on_exit_ota},
    {STATE_IDLE,        "IDLE",         state_idle_handler,     on_enter_idle,      on_exit_idle},
    {STATE_CHARGING,    "CHARGING",     state_charging_handler, on_enter_charging,  on_exit_charging},
    {STATE_DISCHARGING, "DISCHARGING",  state_discharging_handler, on_enter_discharging, on_exit_discharging},
    {STATE_HEATING,     "HEATING",      state_heating_handler,  on_enter_heating,   on_exit_heating},
    {STATE_ERROR,       "ERROR",        state_error_handler,    on_enter_error,     on_exit_error},
    {STATE_SLEEP,       "SLEEP",        state_sleep_handler,    on_enter_sleep,     on_exit_sleep},
    {STATE_DEEP_SLEEP,  "DEEP_SLEEP",   state_deep_sleep_handler, on_enter_deep_sleep, on_exit_deep_sleep},
};

/*================================================================*/
/* 二维转移表 */
/*================================================================*/
static const fsm_transition_item_t bms_transition_table[BMS_STATE_COUNT][BMS_EVENT_COUNT] =
{
    /* 状态 UNINIT (0) */
    [STATE_UNINIT] =
    {
        /* EVENT_POWER_ON */        {STATE_INIT,        FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_INIT_DONE */       {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_OTA,         guard_can_ota,      action_ota_start},
        /* EVENT_OTA_DONE */        {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_UNPLUG */   {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_DETACH */     {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_UNINIT,      FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 INIT (1) */
    [STATE_INIT] =
    {
        /* EVENT_POWER_ON */        {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_IDLE,        FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_OTA_START */       {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_UNPLUG */   {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_DETACH */     {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_INIT,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 OTA (2) */
    [STATE_OTA] =
    {
        /* EVENT_POWER_ON */        {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_IDLE,        FSM_GUARD_NOP,      action_ota_done},
        /* EVENT_OTA_FAIL */        {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_CHARGE_PLUG */     {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_UNPLUG */   {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_DETACH */     {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_OTA,         FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 IDLE (3) */
    [STATE_IDLE] =
    {
        /* EVENT_POWER_ON */        {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_OTA,         guard_can_ota,      action_ota_start},
        /* EVENT_OTA_DONE */        {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_CHARGING,    guard_can_charge,   action_start_charging},
        /* EVENT_CHARGE_UNPLUG */   {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_DISCHARGING, guard_can_discharge, action_start_discharging},
        /* EVENT_LOAD_DETACH */     {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_HEATING,     guard_can_heat,     action_start_heating},
        /* EVENT_TEMP_NORMAL */     {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_SLEEP,       guard_can_sleep,    action_enter_sleep},
        /* EVENT_WAKEUP */          {STATE_IDLE,        FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_DEEP_SLEEP,  guard_can_deep_sleep, action_enter_deep_sleep},
    },

    /* 状态 CHARGING (4) */
    [STATE_CHARGING] =
    {
        /* EVENT_POWER_ON */        {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_UNPLUG */   {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_DISCHARGING, guard_can_discharge, action_stop_charging},
        /* EVENT_LOAD_DETACH */     {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_CHARGING,    FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 DISCHARGING (5) */
    [STATE_DISCHARGING] =
    {
        /* EVENT_POWER_ON */        {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_CHARGING,    guard_can_charge,   action_stop_discharging},
        /* EVENT_CHARGE_UNPLUG */   {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_DETACH */     {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_DISCHARGING, FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 HEATING (6) */
    [STATE_HEATING] =
    {
        /* EVENT_POWER_ON */        {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_IDLE,        FSM_GUARD_NOP,      action_stop_heating},
        /* EVENT_CHARGE_UNPLUG */   {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_IDLE,        FSM_GUARD_NOP,      action_stop_heating},
        /* EVENT_LOAD_DETACH */     {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_IDLE,        FSM_GUARD_NOP,      action_stop_heating},
        /* EVENT_TEMP_HIGH */       {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_HEATING,     FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 ERROR (7) */
    [STATE_ERROR] =
    {
        /* EVENT_POWER_ON */        {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_UNPLUG */   {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_DETACH */     {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_FAULT_CLEAR */     {STATE_IDLE,        FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_ERROR,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 SLEEP (8) */
    [STATE_SLEEP] =
    {
        /* EVENT_POWER_ON */        {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_UNPLUG */   {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_DETACH */     {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      action_enter_deep_sleep},
        /* EVENT_WAKEUP */          {STATE_IDLE,        FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_SLEEP,       FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },

    /* 状态 DEEP_SLEEP (9) */
    [STATE_DEEP_SLEEP] =
    {
        /* EVENT_POWER_ON */        {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_INIT_DONE */       {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_START */       {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_DONE */        {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_OTA_FAIL */        {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_PLUG */     {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_CHARGE_UNPLUG */   {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_ATTACH */     {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_LOAD_DETACH */     {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_LOW */        {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_NORMAL */     {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_TEMP_HIGH */       {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_FAULT */           {STATE_ERROR,       FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_FAULT_CLEAR */     {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_SLEEP_TIMEOUT */   {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
        /* EVENT_WAKEUP */          {STATE_INIT,        FSM_GUARD_NOP,      action_log_transition},
        /* EVENT_DEEP_SLEEP_CMD */  {STATE_DEEP_SLEEP,  FSM_GUARD_NOP,      FSM_ACTION_NOP},
    },
};

/* 配置结构 */
static const fsm_config_t bms_config =
{
    .states = bms_states,
    .state_count = BMS_STATE_COUNT,
    .event_count = BMS_EVENT_COUNT,
    .transition_table = (const fsm_transition_item_t*)bms_transition_table,
    .user_data = NULL,
    .name = "BMS_System"
};

/*================================================================*/
/* 状态处理函数实现 */
/*================================================================*/
/**
 * @brief 未初始化状态处理
 */
static fsm_err_t state_uninit_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_DBG("UNINIT: received event %d", evt);
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief 初始化状态处理
 */
static fsm_err_t state_init_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_DBG("INIT: processing event %d", evt);
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief OTA状态处理
 */
static fsm_err_t state_ota_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_DBG("OTA: in progress...");
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief 空闲状态处理
 */
static fsm_err_t state_idle_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_DBG("IDLE: waiting for commands");
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief 充电状态处理函数，实现边充边放逻辑
 */
static fsm_err_t state_charging_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    fsm_err_t ret = FSM_ERR_INVALID_EVENT;

    DEMO_LOG_DBG("CHARGING: processing event %d", evt);

    if (evt == EVENT_LOAD_ATTACH)
    {
        if (user != NULL)
        {
            if (user->charger_power >= user->load_power)
            {
                user->current = (user->charger_power - user->load_power) * 1000 / user->voltage;
                user->load_present = true;
                DEMO_LOG_INFO("Charger supplies load, battery continues charging");
                ret = FSM_OK;
            }
            else
            {
                DEMO_LOG_INFO("Charger power insufficient, switching to discharging");
                ret = FSM_ERR_INVALID_EVENT;
            }
        }
    }
    else if (evt == EVENT_LOAD_DETACH)
    {
        if (user != NULL)
        {
            user->current = user->charger_power * 1000 / user->voltage;
            user->load_present = false;
            DEMO_LOG_INFO("Load detached, back to normal charging");
            ret = FSM_OK;
        }
    }
    else if (evt == EVENT_CHARGE_UNPLUG)
    {
        if (user != NULL)
        {
            if (user->load_present)
            {
                DEMO_LOG_INFO("Charger unplugged with load attached, switching to discharging");
                fsm_force_state(ctx, STATE_DISCHARGING);
            }
            else
            {
                DEMO_LOG_INFO("Charger unplugged, no load, switching to idle");
                fsm_force_state(ctx, STATE_IDLE);
            }
        }
        ret = FSM_OK;
    }
    else if (evt == EVENT_TEMP_NORMAL)
    {
        DEMO_LOG_INFO("Temperature normal, continuing charging");
        ret = FSM_OK;
    }
    return ret;
}

/**
 * @brief 放电状态处理函数，实现边充边放逻辑
 */
static fsm_err_t state_discharging_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    fsm_err_t ret = FSM_ERR_INVALID_EVENT;

    DEMO_LOG_DBG("DISCHARGING: processing event %d", evt);

    if (evt == EVENT_CHARGE_PLUG)
    {
        if (user != NULL)
        {
            if (user->charger_power >= user->load_power)
            {
                DEMO_LOG_INFO("Charger plugged with enough power, switching to charging");
                ret = FSM_ERR_INVALID_EVENT;
            }
            else
            {
                user->current = -(user->load_power - user->charger_power) * 1000 / user->voltage;
                user->charger_present = true;
                DEMO_LOG_INFO("Charger insufficient, continuing discharging with reduced battery current");
                ret = FSM_OK;
            }
        }
    }
    else if (evt == EVENT_LOAD_DETACH)
    {
        if (user != NULL)
        {
            if (user->charger_present)
            {
                DEMO_LOG_INFO("Load detached, charger present, switching to charging");
                fsm_force_state(ctx, STATE_CHARGING);
            }
            else
            {
                DEMO_LOG_INFO("Load detached, no charger, switching to idle");
                fsm_force_state(ctx, STATE_IDLE);
            }
        }
        ret = FSM_OK;
    }
    else if (evt == EVENT_TEMP_NORMAL)
    {
        DEMO_LOG_INFO("Temperature normal, continuing discharging");
        ret = FSM_OK;
    }
    return ret;
}

/**
 * @brief 加热状态处理
 */
static fsm_err_t state_heating_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_DBG("HEATING: processing event %d", evt);
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief 错误状态处理
 */
static fsm_err_t state_error_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_ERR("ERROR: fault code=%u", ((bms_user_data_t*)ctx->config->user_data)->fault_code);
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief 休眠状态处理
 */
static fsm_err_t state_sleep_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_DBG("SLEEP: low power mode");
    return FSM_ERR_INVALID_EVENT;
}

/**
 * @brief 深度休眠状态处理
 */
static fsm_err_t state_deep_sleep_handler(fsm_context_t* ctx, fsm_event_id_t evt, void* data)
{
    (void)ctx; (void)evt; (void)data;
    DEMO_LOG_DBG("DEEP SLEEP: almost off");
    return FSM_ERR_INVALID_EVENT;
}

/*================================================================*/
/* 进入/退出动作函数 */
/*================================================================*/
/**
 * @brief 进入未初始化状态
 */
static void on_enter_uninit(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Entering UNINIT state");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->charger_present = false;
        user->load_present = false;
        user->temperature = 25;
        user->voltage = 12000;
        user->soc = 50;
        user->fault_code = 0;
        user->ota_in_progress = false;
        user->charger_power = 0;
        user->load_power = 0;
        user->battery_power = 60000;
    }
}

/**
 * @brief 进入初始化状态
 */
static void on_enter_init(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Entering INIT state - initializing hardware...");
}

/**
 * @brief 退出初始化状态
 */
static void on_exit_init(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Exiting INIT state - initialization done");
}

/**
 * @brief 进入OTA状态
 */
static void on_enter_ota(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Entering OTA state - firmware update started");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->ota_in_progress = true;
    }
}

/**
 * @brief 退出OTA状态
 */
static void on_exit_ota(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Exiting OTA state");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->ota_in_progress = false;
    }
}

/**
 * @brief 进入空闲状态
 */
static void on_enter_idle(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Entering IDLE state - ready");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->current = 0;
    }
}

/**
 * @brief 退出空闲状态
 */
static void on_exit_idle(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Exiting IDLE state");
}

/**
 * @brief 进入充电状态
 */
static void on_enter_charging(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Entering CHARGING state - charging started");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->charger_present = true;
        user->current = user->charger_power * 1000 / user->voltage;
    }
}

/**
 * @brief 退出充电状态
 */
static void on_exit_charging(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Exiting CHARGING state");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->charger_present = false;
    }
}

/**
 * @brief 进入放电状态
 */
static void on_enter_discharging(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Entering DISCHARGING state - discharging started");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->load_present = true;
        user->current = - (user->load_power * 1000 / user->voltage);
    }
}

/**
 * @brief 退出放电状态
 */
static void on_exit_discharging(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Exiting DISCHARGING state");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->load_present = false;
    }
}

/**
 * @brief 进入加热状态
 */
static void on_enter_heating(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Entering HEATING state - heating enabled");
}

/**
 * @brief 退出加热状态
 */
static void on_exit_heating(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Exiting HEATING state");
}

/**
 * @brief 进入错误状态
 */
static void on_enter_error(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_ERR("Entering ERROR state - fault detected");
}

/**
 * @brief 退出错误状态
 */
static void on_exit_error(fsm_context_t* ctx)
{
    DEMO_LOG_INFO("Exiting ERROR state - fault cleared");
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    if (user != NULL)
    {
        user->fault_code = 0;
    }
}

/**
 * @brief 进入休眠状态
 */
static void on_enter_sleep(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Entering SLEEP state");
}

/**
 * @brief 退出休眠状态
 */
static void on_exit_sleep(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Exiting SLEEP state");
}

/**
 * @brief 进入深度休眠状态
 */
static void on_enter_deep_sleep(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Entering DEEP SLEEP state");
}

/**
 * @brief 退出深度休眠状态
 */
static void on_exit_deep_sleep(fsm_context_t* ctx)
{
    (void)ctx;
    DEMO_LOG_INFO("Exiting DEEP SLEEP state");
}

/*================================================================*/
/* 守卫条件函数实现 */
/*================================================================*/
/**
 * @brief 检查是否可以充电
 */
static bool guard_can_charge(fsm_context_t* ctx, void* event_data)
{
    bool can = false;
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;

    (void)event_data;
    can = (user != NULL) && (!user->ota_in_progress) &&
          (user->fault_code == 0) && (user->soc < 100);
    if (!can)
    {
        DEMO_LOG_WRN("Guard: Cannot charge (ota=%d, fault=%u, soc=%u)",
                     user ? user->ota_in_progress : 0,
                     user ? user->fault_code : 0,
                     user ? user->soc : 0);
    }
    return can;
}

/**
 * @brief 检查是否可以放电
 */
static bool guard_can_discharge(fsm_context_t* ctx, void* event_data)
{
    bool can = false;
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;

    (void)event_data;
    can = (user != NULL) && (!user->ota_in_progress) &&
          (user->fault_code == 0) && (user->soc > 0);
    if (!can)
    {
        DEMO_LOG_WRN("Guard: Cannot discharge");
    }
    return can;
}

/**
 * @brief 检查是否可以加热
 */
static bool guard_can_heat(fsm_context_t* ctx, void* event_data)
{
    bool can = false;
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;

    (void)event_data;
    can = (user != NULL) && (!user->ota_in_progress) &&
          (user->fault_code == 0) && (user->temperature < 5);
    return can;
}

/**
 * @brief 检查是否可以进入休眠
 */
static bool guard_can_sleep(fsm_context_t* ctx, void* event_data)
{
    bool can = false;
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;

    (void)event_data;
    can = (user != NULL) && (!user->charger_present) &&
          (!user->load_present) && (user->fault_code == 0);
    return can;
}

/**
 * @brief 检查是否可以进入深度休眠
 */
static bool guard_can_deep_sleep(fsm_context_t* ctx, void* event_data)
{
    bool can = false;
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;

    (void)event_data;
    can = (user != NULL) && (!user->charger_present) &&
          (!user->load_present) && (user->fault_code == 0);
    return can;
}

/**
 * @brief 检查是否可以进行OTA
 */
static bool guard_can_ota(fsm_context_t* ctx, void* event_data)
{
    bool can = false;
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;

    (void)event_data;
    can = (user != NULL) && (!user->ota_in_progress) &&
          (user->fault_code == 0) && (!user->charger_present) &&
          (!user->load_present);
    return can;
}

/*================================================================*/
/* 转移动作函数实现 */
/*================================================================*/
static void action_log_transition(fsm_context_t* ctx, void* event_data)
{
    (void)ctx; (void)event_data;
    DEMO_LOG_DBG("Transition action: log only");
}

static void action_start_charging(fsm_context_t* ctx, void* event_data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    (void)event_data;
    DEMO_LOG_INFO("Action: start charging");
    if (user != NULL)
    {
        user->charger_present = true;
        user->current = user->charger_power * 1000 / user->voltage;
    }
}

static void action_stop_charging(fsm_context_t* ctx, void* event_data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    (void)event_data;
    DEMO_LOG_INFO("Action: stop charging");
    if (user != NULL)
    {
        user->charger_present = false;
    }
}

static void action_start_discharging(fsm_context_t* ctx, void* event_data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    (void)event_data;
    DEMO_LOG_INFO("Action: start discharging");
    if (user != NULL)
    {
        user->load_present = true;
        user->current = - (user->load_power * 1000 / user->voltage);
    }
}

static void action_stop_discharging(fsm_context_t* ctx, void* event_data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    (void)event_data;
    DEMO_LOG_INFO("Action: stop discharging");
    if (user != NULL)
    {
        user->load_present = false;
    }
}

static void action_start_heating(fsm_context_t* ctx, void* event_data)
{
    (void)ctx; (void)event_data;
    DEMO_LOG_INFO("Action: start heating");
}

static void action_stop_heating(fsm_context_t* ctx, void* event_data)
{
    (void)ctx; (void)event_data;
    DEMO_LOG_INFO("Action: stop heating");
}

static void action_enter_sleep(fsm_context_t* ctx, void* event_data)
{
    (void)ctx; (void)event_data;
    DEMO_LOG_INFO("Action: entering sleep mode");
}

static void action_enter_deep_sleep(fsm_context_t* ctx, void* event_data)
{
    (void)ctx; (void)event_data;
    DEMO_LOG_INFO("Action: entering deep sleep mode");
}

static void action_ota_start(fsm_context_t* ctx, void* event_data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    (void)event_data;
    DEMO_LOG_INFO("Action: OTA started");
    if (user != NULL)
    {
        user->ota_in_progress = true;
    }
}

static void action_ota_done(fsm_context_t* ctx, void* event_data)
{
    bms_user_data_t* user = (bms_user_data_t*)ctx->config->user_data;
    (void)event_data;
    DEMO_LOG_INFO("Action: OTA completed");
    if (user != NULL)
    {
        user->ota_in_progress = false;
    }
}

/*================================================================*/
/* 辅助函数 - 输入处理 */
/*================================================================*/
/**
 * @brief 跳过空白并读取功率值
 * @param input 输入字符串
 * @param user 用户数据
 * @param cmd 命令字符（'p'或'l'）
 */
static void skip_whitespace_and_read_power(char* input, bms_user_data_t* user, char cmd)
{
    char* p = input;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0')
    {
        DEMO_PRINTF("Enter power value: ");
        char buf[16];
        if (fgets(buf, sizeof(buf), stdin) != NULL)
        {
            if (cmd == 'p')
            {
                user->charger_power = (uint32_t)atoi(buf);
            }
            else
            {
                user->load_power = (uint32_t)atoi(buf);
            }
        }
    }
    else
    {
        if (cmd == 'p')
        {
            user->charger_power = (uint32_t)atoi(p);
        }
        else
        {
            user->load_power = (uint32_t)atoi(p);
        }
    }
}

/**
 * @brief 打印菜单
 */
static void print_menu(void)
{
    DEMO_PRINTF("\n");
    DEMO_PRINTF("===== BMS Demo Menu =====\n");
    DEMO_PRINTF(" 0: POWER_ON\n");
    DEMO_PRINTF(" 1: INIT_DONE\n");
    DEMO_PRINTF(" 2: OTA_START\n");
    DEMO_PRINTF(" 3: OTA_DONE\n");
    DEMO_PRINTF(" 4: OTA_FAIL\n");
    DEMO_PRINTF(" 5: CHARGE_PLUG\n");
    DEMO_PRINTF(" 6: CHARGE_UNPLUG\n");
    DEMO_PRINTF(" 7: LOAD_ATTACH\n");
    DEMO_PRINTF(" 8: LOAD_DETACH\n");
    DEMO_PRINTF(" 9: TEMP_LOW\n");
    DEMO_PRINTF("10: TEMP_NORMAL\n");
    DEMO_PRINTF("11: TEMP_HIGH\n");
    DEMO_PRINTF("12: FAULT\n");
    DEMO_PRINTF("13: FAULT_CLEAR\n");
    DEMO_PRINTF("14: SLEEP_TIMEOUT\n");
    DEMO_PRINTF("15: WAKEUP\n");
    DEMO_PRINTF("16: DEEP_SLEEP_CMD\n");
    DEMO_PRINTF(" p: set charger power (enter new value)\n");
    DEMO_PRINTF(" l: set load power (enter new value)\n");
    DEMO_PRINTF(" s: show status\n");
    DEMO_PRINTF(" h: show event definition table.\n");
    DEMO_PRINTF(" q: quit\n");
    DEMO_PRINTF("==========================\n");
}

/**
 * @brief 显示状态
 */
static void show_status(fsm_handle_t fsm, bms_user_data_t* user)
{
    fsm_state_id_t state = fsm_get_current_state(fsm);
    DEMO_PRINTF("\n--- BMS Status ---\n");
    DEMO_PRINTF("State: %s\n", fsm_get_state_name(fsm, state));
    DEMO_PRINTF("Charger: %s (%u W), Load: %s (%u W)\n",
                user->charger_present ? "Plugged" : "Unplugged", user->charger_power,
                user->load_present ? "Attached" : "Detached", user->load_power);
    DEMO_PRINTF("Temp: %d°C, SOC: %u%%, Current: %d mA\n",
                user->temperature, user->soc, user->current);
    DEMO_PRINTF("Fault code: %u\n", user->fault_code);
    DEMO_PRINTF("OTA in progress: %s\n", user->ota_in_progress ? "Yes" : "No");
}

/**
 * @brief 获取用户输入的事件或命令
 * @param fsm FSM句柄
 * @param user_data 用户数据指针
 * @return 有效事件号（0~BMS_EVENT_COUNT-1），-1表示退出
 */
static int get_bms_event(fsm_handle_t fsm, bms_user_data_t* user_data)
{
    char line[32];
    int evt = -1;

    while (1)
    {
        DEMO_PRINTF("\nCurrent state: %s > ",
                    fsm_get_state_name(fsm, fsm_get_current_state(fsm)));
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            /* 输入错误，视为退出 */
            return -1;
        }

        /* 去除末尾换行符 */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n')
        {
            line[len-1] = '\0';
        }

        if (line[0] == '\0')
            continue;

        /* 处理单字符命令 */
        char cmd = line[0];
        if (cmd == 'q' || cmd == 'Q')
        {
            return -1;  /* 退出 */
        }
        else if (cmd == 's' || cmd == 'S')
        {
            show_status(fsm, user_data);
            continue;  /* 继续等待输入 */
        }
        else if (cmd == 'p' || cmd == 'P')
        {
            skip_whitespace_and_read_power(line+1, user_data, 'p');
            DEMO_LOG_INFO("Charger power set to %u W", user_data->charger_power);
            continue;
        }
        else if (cmd == 'l' || cmd == 'L')
        {
            skip_whitespace_and_read_power(line+1, user_data, 'l');
            DEMO_LOG_INFO("Load power set to %u W", user_data->load_power);
            continue;
        }
        else if (cmd == 'h' || cmd == 'H' || cmd == '?')
        {
            /* 方便查看事件定义 */
            print_menu();
            continue;
        }

        /* 尝试解析整数事件号 */
        evt = atoi(line);
        if (evt == 0 && line[0] != '0')
        {
            DEMO_LOG_WRN("Unknown command: %s", line);
            continue;
        }

        if (evt >= 0 && evt < BMS_EVENT_COUNT)
        {
            return evt;  /* 返回有效事件号 */
        }
        else
        {
            DEMO_LOG_WRN("Invalid event number: %d", evt);
            continue;
        }
    }
}

/*================================================================*/
/* 主函数 */
/*================================================================*/
int main(int argc, const char* argv[])
{
    fsm_context_t fsm_context;
    bms_user_data_t user_data = {0};
    fsm_config_t config = bms_config;
    fsm_handle_t fsm = NULL;
    int ret = 0;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    system("title BMS Demo - FSM with Coexist Charging/Discharging");
#endif

    config.user_data = &user_data;

    fsm = fsm_create(&config, &fsm_context);
    if (fsm == NULL)
    {
        DEMO_LOG_ERR("Failed to create FSM!");
        ret = -1;
    }
    else
    {
        DEMO_LOG_INFO("BMS FSM created. Initial state: %s",
                      fsm_get_state_name(fsm, fsm_get_current_state(fsm)));

        print_menu();

        /* 主事件循环 */
        while (1)
        {
            int evt = get_bms_event(fsm, &user_data);
            if (evt == -1)
            {
                break;  /* 退出 */
            }

            fsm_err_t fsm_ret = fsm_process_event(fsm, (fsm_event_id_t)evt, NULL);
            if (fsm_ret != FSM_OK)
            {
                DEMO_LOG_ERR("Event %d failed: %d", evt, fsm_ret);
            }
        }

        fsm_destroy(fsm);
        DEMO_LOG_INFO("BMS Demo ended.");
    }

    return ret;
}
