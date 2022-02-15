//
// Created by ßËßË on 2022/1/26.
//

#ifndef _MQTT_H
#define _MQTT_H
#include "stm32f0xx_hal.h"

#define WAIT_TIME_IN  100 //µ¥Î»10ms
#define WAIT_TIME_OUT 500
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
#define MQTTString_initializer {NULL, {0, NULL}}

typedef struct
{
    /** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1
      */
    char clientID[10];
    uint16_t keepAliveInterval;
    char username[10];
    char password[10];
} MQTTPacket_connectData;
extern void mqttTask();
#endif
