#ifndef _ONENET_MQTT_H
#define _ONENET_MQTT_H

#include "sys.h"
#include "mqtt.h"

//MQTT接入数据包结构体
struct MqttSampleContext
{
//    int epfd;
//    int mqttfd;
    uint32_t sendedbytes;
    struct MqttContext mqttctx[1];
    struct MqttBuffer mqttbuf[1];

    const char *host;
    unsigned short port;

    const char *proid;
    const char *devid;
    const char *apikey;

    int dup;
    enum MqttQosLevel qos;
    int retain;

    uint16_t pkt_to_ack;
    char cmdid[70];
};

#define PROD_ID     "70901"             //修改为自己的产品ID
#define SN          "201608160002"      //修改为自己的设备唯一序列号
#define REG_CODE    "6TM7OkhNsTjATvFx"  //修改为自己的产品注册码
#define API_ADDR    "api.heclouds.com"	//API接入地址

#define DEVICE_NAME     "kylin_"SN			//设备名称

#define REG_PKT_HEAD    "POST http://"API_ADDR"/register_de?register_code="REG_CODE" HTTP/1.1\r\n"\
                        "Host: "API_ADDR"\r\n"\
                        "Content-Length: "

#define REG_PKT_BODY    "{\"title\":\""DEVICE_NAME"\",\"sn\":\""SN"\"}"

#define STRLEN 64

void sendHttpPkt(char *phead, char *pbody);
char *uartDataParse(char *buffer, int32_t *plen);
void sendPkt(char *p, int len);
int MqttSample_Connect(struct MqttSampleContext *ctx, char *proid, \
				char *auth_info, const char *devid, int keep_alive, int clean_session);
int MqttSample_RecvPkt(void *arg, void *buf, uint32_t count);
int MqttSample_SendPkt(void *arg, const struct iovec *iov, int iovcnt);
int MqttSample_HandleConnAck(void *arg, char flags, char ret_code);
int MqttSample_HandlePingResp(void *arg);
int MqttSample_HandlePublish(void *arg, uint16_t pkt_id, const char *topic,
                                    const char *payload, uint32_t payloadsize,
                                    int dup, enum MqttQosLevel qos);
int MqttSample_HandlePubAck(void *arg, uint16_t pkt_id);
int MqttSample_HandlePubRel(void *arg, uint16_t pkt_id);
int MqttSample_HandlePubComp(void *arg, uint16_t pkt_id);
int MqttSample_HandleSubAck(void *arg, uint16_t pkt_id, const char *codes, uint32_t count);
int MqttSample_HandleUnsubAck(void *arg, uint16_t pkt_id);
int MqttSample_HandleCmd(void *arg, uint16_t pkt_id, const char *cmdid,
                                int64_t timestamp, const char *desc, const char *cmdarg,
                                uint32_t cmdarg_len, int dup, enum MqttQosLevel qos);
int MqttSample_Subscribe(struct MqttSampleContext *ctx, char **topic, int num);
int MqttSample_Unsubscribe(struct MqttSampleContext *ctx, char **topics, int num);
int MqttSample_Savedata11(struct MqttSampleContext *ctx, int temp, int humi);
int MqttSample_Savedata(struct MqttSampleContext *ctx, int temp, int humi);
int MqttSample_Publish(struct MqttSampleContext *ctx, int temp, int humi);
int MqttSample_RespCmd(struct MqttSampleContext *ctx, char *resp);
int MqttSample_Init(struct MqttSampleContext *ctx);
																
												
#endif