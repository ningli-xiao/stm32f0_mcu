//
// Created by ���� on 2022/1/26.
//

#ifndef _MQTT_H
#define _MQTT_H

#define WAIT_TIME_IN  100 //��λ10ms
#define WAIT_TIME_OUT 500
#define BROKER_SITE "mqtt.testmqtt.com"
#define BROKER_PORT 1883
#define CLIENT_ID "cc"
#define CLIENT_USER "123"
#define CLIENT_PASS "1"
#define SUBSCRIBE_TOPIC "testsub"
#define PUBLISH_TOPIC "testpub"
#define PUBLISH_INTERVAL 300000
#define PUBLISH_PAYLOEAD "hello I'm from gprs module"

extern void mqttTask();
#endif
