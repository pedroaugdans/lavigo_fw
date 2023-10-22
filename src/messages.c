
#include "messages.h"

char *message_type[] = {
    "result",
    "event",
    "action"
};

char *msg_fields[] = {
    "hub",
    "resource",
    "action",
    "status",
    "type",
    "code",
    "template"
};

char *template_fields[] = {
    "version",
    "ports",
    "signals",
    "actions",
    "events",
};

char *execution_status[] = {
    "success",
    "failure",
    "trigger"
};

char * action_status[] = {
    "running",
    "success",
    "trigger"
};

char *validation_status[] = {
    "success",
    "failure",
    "pending"
};

char *deployment_status[] = {
    "success",
    "failure"
};

char *deploymentActions[] = {
    "init",
    "fix",
    "clear"
};

char *deploymentCode[] = {
    "lolz"
};

char *updateStatus[] = {
    "invalid",
    "pending",
    "success",
    "failure"
};


char *monitoring_fields[] = {
    "heartbeat",
    "reconnect",
    "recover",
    "dump",
    "fback",
    "report",
    "xio"
};

char *monitoringStatus[] = {
    "OK",
    "failure"
};

char *monitoringVariables[] = {
    "DRAM",
    "IRAM",
};

char * monitoring_recovery_fields[] = {
    "idle",
    "enab",
    "ctrl",
    "actn",
    "time",
    "resource",
    "newsec"
};

char * monitoring_fallback_msg[] = {
    "port",
    "level",
    "timestamp"
};

char * monitoring_config_fields[] = {
    "mode",
    "key",
    "firmware",
    "timestamp"
};

char * registration_actions[] = {
  "validate",
  "rotate",
  "back",
  "forward"
};

char * hub_mode_msg[] = {
    "fallback",
    "bootup",
    "idle"
};

char * monitoring_error_keys_fields[] = {
  "uart",
  "wifi",
  "mqtt",
  "update",
  "rgstr",
  "connection",
  "validation",
  "monitoring",
  "deployment",
  "execution",
  "launcher",
  "console",
  "fallback"
};

char *messages_templates[NOF_MSG_CHANNELS] = {
    /* Validation */ "{\"hub\":\"hub-id\"}",
    /* Monitoring */ "{\"hub\":\"hub-id\"}",
    /* Deployment */ "{\"type\":\"type\",\"hub\":\"hub-id\",\"status\":\"success\",\"resource\":1,\"action\":\"init\"}",
    /* Execution  */ "{\"type\":\"type\",\"hub\":\"hub-id\",\"status\":\"status\",\"resource\":2,\"action\":\"action\"}",
    /* Update     */ "{\"hub\":\"hub-id\",\"status\":\"success\"}"
};

char * loopback_messages_templates[] = {
    /* Validation */ "",
    /* Monitoring */ "",
    /* Deployment */ "{ \"hub\": \"hub-VKXA5UMQEYCG\", \"resource\": 4, \"action\": \"init\", \"template\": { \"version\": 1, \"ports\": [{ \"letter\": \"A\", \"color\": \"orange\" }], \"signals\": [{ \"name\": \"S\", \"port\": 0, \"direction\": 0 }, { \"name\": \"A\", \"port\": 0, \"direction\": 1 }], \"actions\": [{ \"name\": \"start\", \"target\": 0, \"pattern\": \"A:l|S:L|S:H|A:h|%:10|S:L\", \"status\": \"running\", \"condition\": 1 }, { \"name\": \"testr\", \"target\": 0, \"pattern\": \"S:L|S:H|S:L\", \"status\": \"success\", \"condition\": 1 }, { \"name\": \"test\", \"target\": 0, \"pattern\": \"S:H|S:L\", \"status\": \"success\", \"condition\": 1 }, { \"name\": \"deny\", \"target\": 1, \"pattern\": \"A:H\", \"status\": \"success\", \"condition\": 1 }, { \"name\": \"allow\", \"target\": 1, \"pattern\": \"A:L\", \"status\": \"success\", \"condition\": 1 }], \"events\": [{ \"name\": \"start\", \"target\": 0, \"pattern\": \"A:l|S:l|S:h|A:h|S:l|A:l\", \"status\": \"success\", \"condition\": 0 }, { \"name\": \"test\", \"status\": \"trigger\", \"target\": 0, \"pattern\": \"S:l|S:h|S:l\", \"condition\": 0 }] } }",
    /* Execution  */ "{\"hub\":\"hub-id\",\"resource\":2,\"action\":\"action\"}",
    /* Update     */ ""
};
