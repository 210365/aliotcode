/***********************************************************************************
	* __  ____    __ ______
	* \ \/ /\ \  / /|  __  \	�ٷ���վ�� www.edu118.com
	*  \  /  \_\/_/ | |  \ |					
	*  /  \   |  |  | |__/ |					
	* /_/\_\  |__|  |______/					
	*
	*��Ȩ��������ӯ��Ƽ����޹�˾��֣�ݷֲ�����������Ȩ
	*�ļ�����ALIIOT.h
	*���ߣ�XYD
	*�汾�ţ�v1.0
	*���ڣ�2021.01.16
	*������STM32 MQTTЭ��Ϊpaho��
	*��ע��
	*��ʷ��¼��
	*
***********************************************************************************/
#ifndef _ALIIOT_H_
#define _ALIIOT_H_

#include "main.h"



//��Ԫ��
#define Ali_ProductKey 		"k1wobqqvVoW"
#define Ali_DeviceName 		"TEST"
#define Ali_DeviceSecret 	"11c0ca172083efeebb11df6e092e9efe"


#define ALIBUF_SIZE 128
typedef struct{
	char ClientID[ALIBUF_SIZE];
	uint16_t ClientID_len;
	char UserName[ALIBUF_SIZE];
	uint16_t UserName_len;
	char PassWord[ALIBUF_SIZE];
	uint16_t PassWord_len;
	char ServerIP[ALIBUF_SIZE];
	uint16_t ServerPort;
	
	char topic_post[ALIBUF_SIZE];//����Topic
	char topic_post_reply[ALIBUF_SIZE];//���Ļش���Ӧ
	char topic_set[ALIBUF_SIZE];//����������Ϣ
}_AliIOT_Connect;

extern _AliIOT_Connect Ali_Buf;

/* ������ͨ�����ݷ�װ */
typedef struct
{
	uint16_t Tem;
	uint16_t Hum;
	uint16_t Light;
	uint16_t Smoke;
	uint8_t  Beep_State;
	uint8_t  LEDSwitch_State;
	uint8_t  RGBTHREE_State;
}_AliData;
extern _AliData Ali_data;

typedef enum{Aliyun_NOWifi=0,Aliyun_Connect_Error,Aliyun_MQTT_OK} Aliyun_Status_E;
extern Aliyun_Status_E ali_status;
void AliIOT_DataConfig(void);
uint8_t AliIOT_ConnetServer(void);
uint8_t MyMQTT_Connect(char *ClientID,char *UserName,char *PassWord);
uint8_t MyMQTT_DisConnect(void);
uint8_t MyMQTT_Publish(char *topic,char *payload);
uint8_t MyMQTT_Subscribe(char *topic);
uint8_t MyMQTT_UnSubscribe(char *topic);
void MQTT_Test(void);
void MQTT_Test_Ranye(void);

void MQTT_Getdatas_Deal(void);
void MQTT_Publishdata_Deal(void);
uint8_t MQTT_PublishAck_Deal(void);
uint8_t MQTT_Ping(void);
#endif

