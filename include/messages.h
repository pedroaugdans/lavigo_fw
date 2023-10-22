
#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#define SIZEOF_DEPLOY_IMESSAGE      1024
#define SIZEOF_EXECUTE_IMESSAGE     256
#define SIZEOF_MONITOR_IMESSAGE     256
#define SIZEOF_UPDATE_IMESSAGE      256
#define SIZEOF_VALIDATION_IMESSAGE  256
#define INFO                        "info"
#define ERROR_FIELD                 "error"

typedef enum {
    Validation = 0,
    Monitoring = 1,
    Deployment = 2,
    Execution = 3,
    Update = 4,
    NOF_MSG_CHANNELS,
} messages_channel_t;

typedef enum {
    hubIdField = 0,
    resourceField,
    actionField,
    statusField,
    typeField,
    errorField,
    templateField,
    JSON_TOTAL_FIELDS
} jsonFieldNames;

typedef enum {
    monitoring_type_heartbeat,
    monitoring_type_reconnected,
    monitoring_type_recovery,
    monitoring_type_dump,
    monitoring_type_fallback,
    monitoring_type_report,
    monitoring_type_xio
} monitoring_types;

typedef enum {
    okMonitor = 0,
    failMonitor
} monitoringStates;

typedef enum {
    successDeploy,
    failDeploy
} deploymentResult;

typedef enum {
    succesExecution,
    failedExecution,
    triggerExecution
} msgResults;

typedef enum {
    deployment_msg_init,
    deployment_msg_fix,
    deployment_msg_clear,
    TOTAL_DEPLOYMENT_ACTIONS
} deployment_msg_actions;

typedef enum {
    registration_msg_validate,
    registration_msg_rotate,
    registration_msg_roleback,
    registration_msg_roleforward,
    MAX_REGISTRATION_MSGS
} registration_msg_actions;

typedef enum {
    type_result,
    type_event,
    type_action
} msgType;

typedef enum {
    validationSuccess,
    validationFailure,
    validationPending,
    TOTAL_VALIDATION_RESULTS,
} validationResults;

typedef enum {
    template_versionField,
    template_portsField,
    template_signalsField,
    template_actionsField,
    template_eventsField,
    TEMPLATE_TOTAL_FIELDS
} templateFields_t;

typedef enum {
    runningStatus,
    successStatus,
    triggerStatus,
    MAX_ACTION_STATUS
} actionStatus_t;

typedef enum {
    DRAM_monitoringField,
    IRAM_monitoringField
} monitoringVariables_field_t;

typedef enum {
    idle_recovery_field,
    enable_recovery_field,
    control_recovery_field,
    action_recovery_field,
    action_timestamp_field,
    resource_recovery_field,
    new_sequence_recovery_field,
    TOTAL_RECOVERY_FIELDS
} monitoring_recovery_fields_t;

typedef enum {
    config_mode_field,
    config_key_fields,
    config_fw_fields,
    config_ts_fields
} monitoring_config_fields_t;

typedef enum {
    fallback_port_field,
    fallback_level_field,
    fallback_tstamp_field
} fallback_field_t;

extern char *registration_actions[];
extern char *monitoring_config_fields[];
extern char *monitoring_recovery_fields[];
extern char *monitoring_fields[];
extern char *monitoringVariables[];
extern char *action_status[];
extern char *message_type[];
extern char *deploymentActions[];
extern char *msg_fields[];
extern char *monitoringStatus[];
extern char *execution_status[];
extern char *validation_status[];
extern char *deployment_status[];
extern char *deploymentCode[];
extern char *updateStatus[];
extern char *messages_templates[NOF_MSG_CHANNELS];
extern char *template_fields[];
extern char *loopback_messages_templates[];
extern char *monitoring_fallback_msg[];
extern char *monitoring_error_keys_fields[];
extern char *hub_mode_msg[];

#endif
