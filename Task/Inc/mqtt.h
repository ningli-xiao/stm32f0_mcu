//
// Created by ���� on 2022/1/26.
//

#ifndef _MQTT_H
#define _MQTT_H
#include "stm32f0xx_hal.h"

#define MQTTString_initializer {NULL, {0, NULL}}

#define WAIT_TIME_IN  1000 //��λ1ms
#define WAIT_TIME_OUT 5000
//#define BROKER_SITE "mqtt.testmqtt.com"
//#define BROKER_PORT 1883
//#define CLIENT_ID "cc"
//#define CLIENT_USER "123"
//#define CLIENT_PASS "1"
//#define SUBSCRIBE_TOPIC "testsub"
//#define PUBLISH_TOPIC "testpub"
//#define PUBLISH_INTERVAL 300000
//#define PUBLISH_PAYLOEAD "hello I'm from gprs module"

#define BROKER_SITE "183.230.40.39"
#define BROKER_PORT 6002
#define CLIENT_ID "893582456"
#define CLIENT_USER "489121"
#define CLIENT_PASS "1"
#define SUBSCRIBE_TOPIC "testsub"
#define PUBLISH_TOPIC "testpub"
#define PUBLISH_INTERVAL 300000
#define PUBLISH_PAYLOEAD "hello I'm from gprs module"



typedef struct
{
    int len;
    char* data;
} MQTTLenString;

typedef struct
{
    char* cstring;
    MQTTLenString lenstring;
} MQTTString;
#define MQTTString_initializer {NULL, {0, NULL}}
/**
 * Defines the MQTT "Last Will and Testament" (LWT) settings for
 * the connect packet.
 */
typedef struct
{
    /** The eyecatcher for this structure.  must be MQTW. */
    char struct_id[4];
    /** The version number of this structure.  Must be 0 */
    int struct_version;
    /** The LWT topic to which the LWT message will be published. */
    MQTTString topicName;
    /** The LWT payload. */
    MQTTString message;
    /**
      * The retained flag for the LWT message (see MQTTAsync_message.retained).
      */
    unsigned char retained;
    /**
      * The quality of service setting for the LWT message (see
      * MQTTAsync_message.qos and @ref qos).
      */
    char qos;
} MQTTPacket_willOptions;
#define MQTTPacket_willOptions_initializer { {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }

typedef struct
{
    /** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1
      */
    unsigned char MQTTVersion;
    MQTTString clientID;
    unsigned short keepAliveInterval;
    unsigned char cleanSession;
//    unsigned char willFlag;
//    MQTTPacket_willOptions will;
    MQTTString username;
    MQTTString password;
} MQTTPacket_connectData;
#define MQTTPacket_connectData_initializer { 4, {NULL, {0, NULL}}, 60, 1, {NULL, {0, NULL}}, {NULL, {0, NULL}}}
extern void mqttTask();
#endif
