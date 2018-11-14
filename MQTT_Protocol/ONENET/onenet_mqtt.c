/**
  * @file    onenet_mqtt.c
  * @author  Liang
  * @version V1.0.0
  * @date    2018-10-29
  * @brief	 适合ONENET MQTT接入的二次封装MQTT函数
	* 基本使用流程
		1. 定义MQTT上下文结构实例以及设置相应的协议参数
		2. MqttSample_Init()初始化MQTT
		3. MQTT连接
		4. MQTT订阅
		5. 发布消息
  **/

#include "onenet_mqtt.h"
#include "usart2.h"
#include "rtthread.h"
#include "debug.h"
#include "stdlib.h"
#include "string.h"

#define STRLEN 64
char g_cmdid[STRLEN];

/**
 * 发送http数据包
 * @param   
 * @return 
 * @brief 
 **/
void sendHttpPkt(char *phead, char *pbody)
{
    char sendBuf0[20];
    char sendBuf1[500];
    
    sprintf(sendBuf1, "%s%d\r\n\r\n%s", phead, strlen(pbody), pbody);
    
    sprintf(sendBuf0, "AT+CIPSEND=%d\r\n", strlen(sendBuf1));
    SendCmd(sendBuf0, ">", 500);
    USART2_Clear();
    

    /* EDP设备连接包，发送 */
    USART2_Write(USART2, (uint8_t*)sendBuf1, strlen(sendBuf1));    //串口发送
}

/**
 * 串口数据解析
 * @param   
 * @return 
 * @brief 
 **/
char *uartDataParse(char *buffer, int32_t *plen)
{
    char *p;
    char *pnum;
    int32_t len;
    if((p = strstr(buffer, "CLOSED")) != NULL)
    {
        rt_kprintf("tcp connection closed\r\n");
    }
    if((p = strstr(buffer, "WIFI DISCONNECT")) != NULL)
    {
        rt_kprintf("wifi disconnected\r\n");
    }
    if((p = strstr(buffer, "WIFI CONNECTED")) != NULL)
    {
        rt_kprintf("wifi connected\r\n");
    }
    if((p = strstr(buffer, "+IPD")) != NULL)
    {
        pnum = p + 5;       //跳过头部的 "+IPD,"，指向长度字段的首地址
        p = strstr(p, ":"); //指向长度字段末尾
        *(p++) = '\0';      //长度字段末尾添加结束符，p指向服务器下发的第一个字节
        len = atoi(pnum);
        *plen = len;
        return p;
    }
    return NULL;
}

/**
 * 非透传模式发送EDP报文
 * @param   
 * @return 
 * @brief 
 **/
void sendPkt(char *p, int len)
{
    char sendBuf[30] = {0};

    /* 非透传模式先发送AT+CIPSEND=X */
    sprintf(sendBuf, "AT+CIPSEND=%d\r\n", len);
    SendCmd(sendBuf, ">", 500);
    USART2_Clear();

    /* EDP设备连接包，发送 */
    USART2_Write(USART2, p, len);    //串口发送
    //hexdump(p, len);     //打印发送内容
}

/**
 * MQTT连接并保持活跃
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_Connect(struct MqttSampleContext *ctx, char *proid, \
				char *auth_info, const char *devid, int keep_alive, int clean_session)
{
	int err, flags;

	rt_kprintf("product id: %s\r\nsn: %s\r\ndeviceid: %s\r\nkeepalive: %d\r\ncleansession: %d\r\nQoS: %d\r\n", 
		proid, auth_info, devid, keep_alive, clean_session, MQTT_QOS_LEVEL0);
	
	err = Mqtt_PackConnectPkt(ctx->mqttbuf, keep_alive, devid,
                              clean_session, NULL,
                              NULL, 0,
                              MQTT_QOS_LEVEL0, 0, proid,
                              auth_info, strlen(auth_info));
    
	if(MQTTERR_NOERROR != err) {
        rt_kprintf("Failed to pack the MQTT CONNECT PACKET, errcode is %d.\n", err);
        return -1;
    }

    return 0;
}

/**
 * MQTT接收数据包
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_RecvPkt(void *arg, void *buf, uint32_t count)
{
	int i;
  int rcv_len = 0, onenetdata_len = 0;
  uint8_t buffer[128] = {0};
  char *p = NULL;
    
  /* 完成串口数据包的接收 */
  while ((rcv_len = USART2_GetRcvNum()) > 0)
  {
		rt_kprintf("rcv_len: %d\r\n", rcv_len);
    mDelay(500);
  }
    
	if(usart2_rcv_len != 0) //连同wifi的AT指令返回信息
	{
			USART2_GetRcvData(buffer, usart2_rcv_len);
			/* 串口数据分析，区分WIFI模块的推送信息与服务器的下发信息 */
			if((p = uartDataParse(buffer, &onenetdata_len)) == NULL)
			{
					/* 无有效数据，返回0 */
					return 0;	
			}
			
			/* 成功接收了rcv_len个字节的数据，将其写入recv_buf，用于解析 */
			memcpy(buf, p, onenetdata_len);

			if(onenetdata_len > 0)
			{
					printf("Rcv: \r\n");
					for(i=0; i<onenetdata_len; i++)
					{
							printf("%02X ", ((unsigned char *)buf)[i]);
					}
					printf("\r\n");
			}
	}
  return onenetdata_len;
}

/**
* MQTT发送数据包
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_SendPkt(void *arg, const struct iovec *iov, int iovcnt)
{
    char sendbuf[1024];
    int len = 0;
    int bytes;
    int i=0,j=0;
	
    for(i=0; i<iovcnt; ++i)
    {
        char *pkg = (char*)iov[i].iov_base;
        for(j=0; j<iov[i].iov_len; ++j)
        {
            rt_kprintf("%02X ", pkg[j]&0xFF);
        }
        rt_kprintf("\n");
        
        memcpy(sendbuf+len, iov[i].iov_base, iov[i].iov_len);
        len += iov[i].iov_len;
    }
    
    sendPkt(sendbuf, len);


    //bytes = sendmsg((int)(size_t)arg, &msg, 0);
    return bytes;
}

//------------------------------- packet handlers -------------------------------------------

/**
 * 连接确认
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandleConnAck(void *arg, char flags, char ret_code)
{
    rt_kprintf("Success to connect to the server, flags(%0x), code(%d).\n",
           flags, ret_code);
    return 0;
}

/**
 * Ping响应
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandlePingResp(void *arg)
{
    rt_kprintf("Recv the ping response.\n");
    return 0;
}

/**
 * 发布消息
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandlePublish(void *arg, uint16_t pkt_id, const char *topic,
                                    const char *payload, uint32_t payloadsize,
                                    int dup, enum MqttQosLevel qos)
{
    struct MqttSampleContext *ctx = (struct MqttSampleContext*)arg;
    ctx->pkt_to_ack = pkt_id;
    ctx->dup = dup;
    ctx->qos = qos;
    printf("dup: %d, qos: %d, id: %d\r\ntopic: %s\r\npayloadsize: %d  payload: %s\r\n",
           dup, qos, pkt_id, topic, payloadsize, payload);

	

    /*fix me : add response ?*/

    //get cmdid
    //$creq/topic_name/cmdid
    memset(g_cmdid, STRLEN, 0);
    if('$' == topic[0] &&
        'c' == topic[1] &&
        'r' == topic[2] &&
        'e' == topic[3] &&
        'q' == topic[4] &&
        '/' == topic[5]){
        int i=6;
        while(topic[i]!='/' && i<strlen(topic)){
            ++i;
        }
        if(i<strlen(topic))
            memcpy(g_cmdid, topic+i+1, strlen(topic+i+1));
    }
    return 0;
}

/**
 * 收到发布消息确认
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandlePubAck(void *arg, uint16_t pkt_id)
{
    printf("Recv the publish ack, packet id is %d.\n", pkt_id);
    return 0;
}

/**
 * 发布消息收到
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandlePubRec(void *arg, uint16_t pkt_id)
{
    struct MqttSampleContext *ctx = (struct MqttSampleContext*)arg;
    ctx->pkt_to_ack = pkt_id;
    printf("Recv the publish rec, packet id is %d.\n", pkt_id);
    return 0;
}

/**
 * 发布消息释放
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandlePubRel(void *arg, uint16_t pkt_id)
{
    struct MqttSampleContext *ctx = (struct MqttSampleContext*)arg;
    ctx->pkt_to_ack = pkt_id;
    printf("Recv the publish rel, packet id is %d.\n", pkt_id);
    return 0;
}

/**
 * 发布消息完成
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandlePubComp(void *arg, uint16_t pkt_id)
{
    printf("Recv the publish comp, packet id is %d.\n", pkt_id);
    return 0;
}

/**
 * 订阅确认
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandleSubAck(void *arg, uint16_t pkt_id, const char *codes, uint32_t count)
{
    uint32_t i;
    printf("Recv the subscribe ack, packet id is %d, return code count is %d:.\n", pkt_id, count);
    for(i = 0; i < count; ++i) {
        unsigned int code = ((unsigned char*)codes)[i];
        printf("   code%d=%02x\n", i, code);
    }

    return 0;
}

/**
 * 取消订阅确认
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandleUnsubAck(void *arg, uint16_t pkt_id)
{
    printf("Recv the unsubscribe ack, packet id is %d.\n", pkt_id);
    return 0;
}

/**
 * 命令
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_HandleCmd(void *arg, uint16_t pkt_id, const char *cmdid,
                                int64_t timestamp, const char *desc, const char *cmdarg,
                                uint32_t cmdarg_len, int dup, enum MqttQosLevel qos)
{
    uint32_t i;
    char cmd_str[100] = {0};
    struct MqttSampleContext *ctx = (struct MqttSampleContext*)arg;
    ctx->pkt_to_ack = pkt_id;
    strcpy(ctx->cmdid, cmdid);
    printf("Recv the command, packet id is %d, cmduuid is %s, qos=%d, dup=%d.\n",
           pkt_id, cmdid, qos, dup);

    if(0 != timestamp) {
        time_t seconds = timestamp / 1000;
        struct tm *st = localtime(&seconds);

        printf("    The timestampe is %04d-%02d-%02dT%02d:%02d:%02d.%03d.\n",
               st->tm_year + 1900, st->tm_mon + 1, st->tm_mday,
               st->tm_hour, st->tm_min, st->tm_sec, (int)(timestamp % 1000));
    }
    else {
        printf("    There is no timestamp.\n");
    }

    if(NULL != desc) {
        printf("    The description is: %s.\n", desc);
    }
    else {
        printf("    There is no description.\n");
    }

    printf("    The length of the command argument is %d, the argument is:", cmdarg_len);

    for(i = 0; i < cmdarg_len; ++i) {
        const char c = cmdarg[i];
        if(0 == i % 16) {
            printf("\n        ");
        }
        printf("%02X'%c' ", c, c);
    }
    printf("\n");
    memcpy(cmd_str, cmdarg, cmdarg_len);
    printf("cmd: %s\r\n", cmd_str);

	/* add your execution code here */

  return 0;
}

/**
 * 订阅请求
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_Subscribe(struct MqttSampleContext *ctx, char **topic, int num)
{
    int err;
//    char **topics;
//    int topics_len = 0;
    
//    topics = str_split(buf, ' ', &topics_len);

//    if (topics){
//        int i;
//        for (i = 0; *(topics + i); i++){
//                    printf("%s\n", *(topics + i));
//        }
//        printf("\n");
//     }

    //sprintf(topic, "%s/%s/45523/test-1", ctx->proid, ctx->apikey);
    err = Mqtt_PackSubscribePkt(ctx->mqttbuf, 2, MQTT_QOS_LEVEL0, topic, num);
    if(err != MQTTERR_NOERROR) {
        printf("Critical bug: failed to pack the subscribe packet.\n");
        return -1;
    }

    /*
    sprintf(topic, "%s/%s/45523/test-2", ctx->proid, ctx->apikey);
    err = Mqtt_AppendSubscribeTopic(ctx->mqttbuf, topic, MQTT_QOS_LEVEL1);
    if (err != MQTTERR_NOERROR) {
        printf("Critical bug: failed to pack the subscribe packet.\n");
        return -1;
    }
    */

    return 0;
}

/**
 * 取消订阅
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_Unsubscribe(struct MqttSampleContext *ctx, char **topics, int num)
{
    int err;
//     char **topics;
//     int topics_len = 0;
//     topics = str_split(buf, ' ', &topics_len);


//     printf("topic len %d\n", topics_len);
//     if (topics){
//         int i;
//         for (i = 0; *(topics + i); i++){
//                     printf("%s\n", *(topics + i));
//         }
//         printf("\n");
//     }

    err = Mqtt_PackUnsubscribePkt(ctx->mqttbuf, 3, topics, num);
    if(err != MQTTERR_NOERROR) {
        printf("Critical bug: failed to pack the unsubscribe packet.\n");
        return -1;
    }

    return 0;
}

/**
 * 
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_Savedata11(struct MqttSampleContext *ctx, int temp, int humi)
{
    int err;
    struct MqttExtent *ext;
	uint16_t pkt_id = 1; 

    char json[]="{\"datastreams\":[{\"id\":\"temp\",\"datapoints\":[{\"value\":%d}]},{\"id\":\"humi\",\"datapoints\":[{\"value\":%d}]}]}";
    char t_json[200];
    int payload_len;
    char *t_payload;
    unsigned short json_len;
    
    sprintf(t_json, json, temp, humi);
    payload_len = 1 + 2 + strlen(t_json)/sizeof(char);
    json_len = strlen(t_json)/sizeof(char);
    
    t_payload = (char *)malloc(payload_len);
    if(t_payload == NULL)
    {
        printf("<%s>: t_payload malloc error\r\n", __FUNCTION__);
        return 0;
    }

    //type
    t_payload[0] = '\x01';

    //length
    t_payload[1] = (json_len & 0xFF00) >> 8;
    t_payload[2] = json_len & 0xFF;

	//json
	memcpy(t_payload+3, t_json, json_len);

    if(ctx->mqttbuf->first_ext) {
        return MQTTERR_INVALID_PARAMETER;
    }
	
	printf("Topic: %s\r\nPakect ID: %d\r\nQoS: %d\r\nPayload: %s\r\n", 
		"$dp", pkt_id, MQTT_QOS_LEVEL1, t_json);
    err = Mqtt_PackPublishPkt(ctx->mqttbuf, pkt_id, "$dp", t_payload, payload_len, \
		MQTT_QOS_LEVEL1, 0, 1);
    
    free(t_payload);

    if(err != MQTTERR_NOERROR) {
        return err;
    }

    return 0;
}

/**
 * 
 * @param   
 * @return 
 * @brief 
 **/
int MqttSample_Savedata(struct MqttSampleContext *ctx, int temp, int humi)
{
    char opt;
    int Qos=1;
    int type = 1;
    //int i = 0;
    /*-q 0/1   ----> Qos0/Qos1
      -t 1/7   ----> json/float datapoint
    */


    printf("Qos: %d    Type: %d\r\n", Qos, type);
    MqttSample_Savedata11(ctx, temp, humi); // qos=1 type=1
}


/**
 * 发布消息
 * @param   
 * @return 
 * @brief 根据传感器数据类型修改合适的参数
 **/
int MqttSample_Publish(struct MqttSampleContext *ctx, int temp, int humi)
{
    int err;
    //int topics_len = 0;
    //struct MqttExtent *ext;
    //int i=0;
    
    char *topic = "key_press";
    char *payload1= "key pressed, temp: %d, humi: %d";
    char payload[100] = {0};
    int pkg_id = 1;

    sprintf(payload, payload1, temp, humi);
    logging_debug("public %s : %s\r\n",topic, payload);

		/*

				topics = str_split(buf, ' ', &topics_len);

			printf("topics_len: %d\r\n", topics_len);

				if (topics){
						int i;
						for (i = 0; *(topics + i); i++){
												printf("%s\n", *(topics + i));
						}
						printf("\n");
				}
				if(4 != topics_len){
						printf("usage:push_dp topicname payload pkg_id");
						return err;
				}

		 */
    if(ctx->mqttbuf->first_ext) {
        return MQTTERR_INVALID_PARAMETER;
    }

    /*
    std::string pkg_id_s(topics+3);
    int pkg_id = std::stoi(pkg_id_s);
    */
    
    err = Mqtt_PackPublishPkt(ctx->mqttbuf, pkg_id, topic, payload, strlen(payload), MQTT_QOS_LEVEL1, 0, 1);

    if(err != MQTTERR_NOERROR) {
        return err;
    }

    return 0;
}


/**
 * 回复命令
 * @param MqttSampleContext
 * @return 
 * @brief 
 **/
int MqttSample_RespCmd(struct MqttSampleContext *ctx, char *resp)
{
    int err;
    int Qos=0;

		logging_debug("QoS: %d\r\nCmdId: %s\r\n", Qos, ctx->cmdid);

    if(0==Qos)
    {
        err = Mqtt_PackCmdRetPkt(ctx->mqttbuf, 1, ctx->cmdid,
                                 resp, 11, MQTT_QOS_LEVEL0, 1);
    }
    else if(1==Qos)
    {
        err = Mqtt_PackCmdRetPkt(ctx->mqttbuf, 1, ctx->cmdid,
                                 resp, 11, MQTT_QOS_LEVEL1, 1);
    }

    if(MQTTERR_NOERROR != err) {
        logging_warning("Critical bug: failed to pack the cmd ret packet.\n");
        return -1;
    }

    return 0;
}

/**
 * MQTT协议初始化
 * @param   MqttSampleContext[MQTT实例结构体指针]
 * @return 
 * @brief 
 **/
int MqttSample_Init(struct MqttSampleContext *ctx)
{
    //struct epoll_event event;
    int err;

    ctx->host = MQTT_HOST;
    ctx->port = MQTT_PORT;
    ctx->sendedbytes = -1;
    
    ctx->devid = NULL;
    ctx->cmdid[0] = '\0';
    
    //    ctx->mqttfd = -1;

    /*
    ctx->host = "192.168.200.218";
    ctx->port = 6002;
    ctx->proid = "433223";
    ctx->devid = "45523";
    ctx->apikey = "Bs04OCJioNgpmvjRphRak15j7Z8=";
    */

    //初始化MQTT上下文,接受缓冲区1000字节
		err = Mqtt_InitContext(ctx->mqttctx, 1000);
    if(MQTTERR_NOERROR != err) {
        logging_error("Failed to init MQTT context errcode is %d", err);
        return -1;
    }

		//定义上下文操作函数
    ctx->mqttctx->read_func = MqttSample_RecvPkt;		//读数据包函数
		//ctx->mqttctx->read_func_arg =  (void*)(size_t)ctx->mqttfd;
		//ctx->mqttctx->writev_func_arg =  (void*)(size_t)ctx->mqttfd;
    ctx->mqttctx->writev_func = MqttSample_SendPkt; //写数据包函数

    ctx->mqttctx->handle_conn_ack = MqttSample_HandleConnAck;
    ctx->mqttctx->handle_conn_ack_arg = ctx;
    ctx->mqttctx->handle_ping_resp = MqttSample_HandlePingResp;
    ctx->mqttctx->handle_ping_resp_arg = ctx;
    ctx->mqttctx->handle_publish = MqttSample_HandlePublish;
    ctx->mqttctx->handle_publish_arg = ctx;
    ctx->mqttctx->handle_pub_ack = MqttSample_HandlePubAck;
    ctx->mqttctx->handle_pub_ack_arg = ctx;
    ctx->mqttctx->handle_pub_rec = MqttSample_HandlePubRec;
    ctx->mqttctx->handle_pub_rec_arg = ctx;
    ctx->mqttctx->handle_pub_rel = MqttSample_HandlePubRel;
    ctx->mqttctx->handle_pub_rel_arg = ctx;
    ctx->mqttctx->handle_pub_comp = MqttSample_HandlePubComp;
    ctx->mqttctx->handle_pub_comp_arg = ctx;
    ctx->mqttctx->handle_sub_ack = MqttSample_HandleSubAck;
    ctx->mqttctx->handle_sub_ack_arg = ctx;
    ctx->mqttctx->handle_unsub_ack = MqttSample_HandleUnsubAck;
    ctx->mqttctx->handle_unsub_ack_arg = ctx;
    ctx->mqttctx->handle_cmd = MqttSample_HandleCmd;
    ctx->mqttctx->handle_cmd_arg = ctx;

    //初始化mqtt数据缓冲区
		MqttBuffer_Init(ctx->mqttbuf);

//     ctx->epfd = epoll_create(10);
//     if(ctx->epfd < 0) {
//         printf("Failed to create the epoll instance.\n");
//         return -1;
//     }

//     if(fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) < 0) {
//         printf("Failed to set the stdin to nonblock mode, errcode is %d.\n", errno);
//         return -1;
//     }

//     event.data.fd = STDIN_FILENO;
//     event.events = EPOLLIN;
//     if(epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, STDIN_FILENO, &event) < 0) {
//         printf("Failed to add the stdin to epoll, errcode is %d.\n", errno);
//         return -1;
//     }

    return 0;
}

