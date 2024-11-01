#include "aliiot.h"
#include "esp8266.h"
#include "hmac/utils_hmac.h"
#include "mqttpacket.h"
#include "cJSON.h"
#include "usart.h"
#include "delay.h"
#include "beep.h"
#include "led.h"
_AliIOT_Connect Ali_Buf = {"\0"};
_AliData Ali_data={0};
Aliyun_Status_E ali_status = Aliyun_NOWifi;

uint8_t MQTT_GETJson_Parse(uint8_t *str);
/*******************************************************************************
函数名称：AliIOT_DataConfig
函数作用：阿里云数据初始化
*******************************************************************************/
void AliIOT_DataConfig(void)
{
	char Temp_buf[128] = "\0";
	memset(&Ali_Buf, 0, sizeof(_AliIOT_Connect));  
	
	//获取ClientID
	sprintf(Ali_Buf.ClientID, "%s|securemode=3,signmethod=hmacsha1|", Ali_DeviceName);
	Ali_Buf.ClientID_len = strlen(Ali_Buf.ClientID);
	//获取UserName
	sprintf(Ali_Buf.UserName, "%s&%s", Ali_DeviceName, Ali_ProductKey);
	Ali_Buf.UserName_len = strlen(Ali_Buf.UserName);
	//获取PassWord
	sprintf(Temp_buf, "clientId%sdeviceName%sproductKey%s", Ali_DeviceName, Ali_DeviceName, Ali_ProductKey); 
	utils_hmac_sha1(Temp_buf, strlen(Temp_buf), Ali_Buf.PassWord, Ali_DeviceSecret, strlen(Ali_DeviceSecret));
	Ali_Buf.PassWord_len = strlen(Ali_Buf.PassWord); 
	//获取服务器IP地址、端口号
	sprintf(Ali_Buf.ServerIP, "%s.iot-as-mqtt.cn-shanghai.aliyuncs.com", Ali_ProductKey);                  //构建服务器域名
	Ali_Buf.ServerPort = 1883;
	
	//Topic列表
	sprintf(Ali_Buf.topic_post, "/sys/%s/%s/thing/event/property/post", Ali_ProductKey, Ali_DeviceName);                 
	sprintf(Ali_Buf.topic_post_reply, "/sys/%s/%s/thing/event/property/post_reply", Ali_ProductKey, Ali_DeviceName);  
	sprintf(Ali_Buf.topic_set, "/sys/%s/%s/thing/service/property/set", Ali_ProductKey, Ali_DeviceName);  
	
	//串口输出调试信息	
	printf("服 务 器：%s:%d\r\n", Ali_Buf.ServerIP, Ali_Buf.ServerPort); 
	printf("客户端ID：%s\r\n", Ali_Buf.ClientID);               
	printf("用 户 名：%s\r\n", Ali_Buf.UserName);               
	printf("密    码：%s\r\n", Ali_Buf.PassWord);               
}

/*******************************************************************************
函数名称：AliIOT_ConnetServer
函数作用：阿里云链接服务器

*******************************************************************************/
uint8_t AliIOT_ConnetServer(void)
{
	uint8_t Timers = 2;
	AliIOT_DataConfig();
	
	Wifi_CloseTransmission();                   //多次连接需退出透传
  Delay_nop_nms(500);
	
	//连接服务器
	while(Timers--)
	{       
		if(Wifi_ConnectServer("TCP", Ali_Buf.ServerIP, Ali_Buf.ServerPort) == WIFI_RES_OK)
		{
			return 1;
		}
	}
	return 0;
}



/*******************************************************************************
函数名称：AliIOT_MQTTAckCheck
函数作用：检测MQTT协议回传数据
*******************************************************************************/
uint8_t AliIOT_MQTTAckCheck(uint8_t MQTTtype, uint16_t timeout)
{
	uint8_t res=2;
	while(timeout--)//等待串口接收到应答信号
	{
		Delay_nop_nms(1);
		if(wifi.rx_over == 1) 
		{
			if(wifi.rx_buff[0] == MQTTtype)
			{
				break;
			}
			else
				wifi.rx_over=0;
		}
	}
	
	if(timeout+1==0)
		return 1;//应答超时
	else
	{
		//进一步判断响应内容
		switch(MQTTtype)
		{
			case 0x20://打印测试：20 02 00 00
				if((wifi.rx_buff[2]==0)&&(wifi.rx_buff[3]==0))
					res = 0;
				else
					res = 2;
				break;
			case 0x90://打印测试：90 03 00 0A 01     描述：第4个数据为QOS质量
				if((wifi.rx_buff[4]==0)||(wifi.rx_buff[4]==1)||(wifi.rx_buff[4]==2))
					res = 0;
				else
					res = 2;
				break;
			case 0xB0://打印测试：B0 02 00 00
				res = 0;//有响应即可
				break;
			case 0x30:
				res = 0;//MQTT_PublishAck_Deal();//默认回传OK，也可使用函数解析下回传的数据，发布也回传，解析容易出错，建议最开始取消订阅回传
				break;
			case 0xD0:
				res = 0;
				break;
			default:res = 2;break;
				
		}
	}
	WIFI_ClearData();
	return res;
}



/*******************************************************************************
函数名称：MyMQTT_Connect
函数作用：MQTT连接
*******************************************************************************/
uint8_t MyMQTT_Connect(char *ClientID, char *UserName, char *PassWord)
{
	WIFI_ClearTxData();
	//1.创建MQTT CONNECT 结构体
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	//2.结构体赋值
	data.cleansession = 1;//清除会话
	data.clientID.cstring = ClientID;//初始化设备ID
	data.keepAliveInterval = 60;//保持建立时长60s
	data.MQTTVersion = 4;//使用MQTT 3.1.1
	
	data.username.cstring = UserName;//用户名
	data.password.cstring = PassWord;//用户密码
	//3.调用MQTTSerialize_connect把结构体中的数据构成二进制流
	wifi.tx_count = MQTTSerialize_connect(wifi.tx_buff, WIFI_MAX_LEN,&data);
	if(wifi.tx_count == 0)
		return 1;//构成二进制数据流失败
	//4.通过WIFI发送出去
	WIFI_ClearData();
	WIFI_SendBuff(wifi.tx_buff, wifi.tx_count);

	return AliIOT_MQTTAckCheck(0x20, 1500);//打印测试：20 02 00 00
}

/*******************************************************************************
函数名称：MyMQTT_DisConnect
函数作用：MQTT断开连接
*******************************************************************************/
uint8_t MyMQTT_DisConnect(void)
{
	WIFI_ClearTxData();
	//1.把DISCONNECT报文结构体，变为二进制数据存入发送缓冲区中
	wifi.tx_count = MQTTSerialize_disconnect(wifi.tx_buff, WIFI_MAX_LEN);
	if(wifi.tx_count == 0)
		return 1;
	//2.调用底层网络发送数据
	WIFI_SendBuff(wifi.tx_buff, wifi.tx_count);
	return 0;
}

/*******************************************************************************
函数名称：MyMQTT_Publish
函数作用：MQTT发布数据
*******************************************************************************/
uint8_t MyMQTT_Publish(char *topic, char *payload)
{
	uint32_t payload_len = 0;
	unsigned char dup = 0;
	int qos = 0;
	unsigned char retained = 0;
	unsigned short msgid = 0;
	
	WIFI_ClearTxData();
	
	//1.初始化相关参数
	MQTTString topicString = MQTTString_initializer;
	topicString.cstring = topic;//保存主题
	payload_len = strlen(payload);//有效荷载的长度
	//2.构成Publish的二进制数据流
	wifi.tx_count = MQTTSerialize_publish(wifi.tx_buff,WIFI_MAX_LEN,dup, qos, retained, msgid,\
												topicString,(uint8_t *)payload,payload_len);
	if(wifi.tx_count <= 0)
		return 1;
	//3.WIFI发送出去
	WIFI_SendBuff(wifi.tx_buff, wifi.tx_count);	
	return 0;//AliIOT_MQTTAckCheck(0x30,10000);//打印测试：回传发布是否成功的响应
}

/*******************************************************************************
函数名称：MyMQTT_Subscribe
函数作用：MQTT订阅主题
*******************************************************************************/
uint8_t MyMQTT_Subscribe(char *topic)
{
	#define TOPIC_COUNT 1
	unsigned char dup = 0;
	unsigned short msgid = 10;
	int count = TOPIC_COUNT;
	int req_qoss[TOPIC_COUNT] = {2};
	
	WIFI_ClearTxData();

	MQTTString topicStrings[TOPIC_COUNT] = { MQTTString_initializer };
	topicStrings[0].cstring = topic;
	wifi.tx_count = MQTTSerialize_subscribe(wifi.tx_buff, WIFI_MAX_LEN, dup, msgid, count, topicStrings, req_qoss);
	
	if(wifi.tx_count <= 0)
		return 1;
	//3.WIFI发送出去
	WIFI_SendBuff(wifi.tx_buff, wifi.tx_count);
	
	return AliIOT_MQTTAckCheck(0x90, 1000);//打印测试：90 03 00 0A 01
}

/*******************************************************************************
函数名称：MyMQTT_UnSubscribe
函数作用：MQTT取消订阅主题
*********************************************************************************/
uint8_t MyMQTT_UnSubscribe(char *topic)
{
	unsigned char dup = 0;
	unsigned short msgid = 0;
	WIFI_ClearTxData();

	MQTTString topicStrings = MQTTString_initializer ;
	
	topicStrings.cstring = topic;
	wifi.tx_count = MQTTSerialize_unsubscribe(wifi.tx_buff, WIFI_MAX_LEN, dup, msgid, 1, &topicStrings);
	
	if(wifi.tx_count <= 0)
		return 1;
	//3.WIFI发送出去
	WIFI_SendBuff(wifi.tx_buff, wifi.tx_count);
	
	return AliIOT_MQTTAckCheck(0xB0, 1000);//打印测试：B0 02 00 00
}



/*******************************************************************************
函数名称：MQTT_Getdata_Deal
函数作用：接收云端数据解析
*********************************************************************************/
void MQTT_Getdatas_Deal(void)
{
	if(wifi.rx_over)
	{
		uint32_t OverLen = 0;
		uint16_t BuffCount = 0;
		uint16_t TopicLen = 0;
		/*
		固定报头：30 99 01  剩余长度=153
		TopicLEN： 00 33 
		2F 73 79 73 2F 61 31 62 61 77 70 69 7A 69 6F 68 2F 49 4F 54 5F 32 30 36 2F 74 68 69 6E 67 2F 73 65 72 76 69 63 65 2F 70 72 6F 70 65 72 74 79 2F 73 65 74 
		7B 22 6D 65 74 68 6F 64 22 3A 22 74 68 69 6E 67 2E 73 65 72 76 69 63 65 2E 70 72 6F 70 65 72 74 79 2E 73 65 74 22 2C 22 69 64 22 3A 22 31 31 39 31 36 37 34 30 39 33 22 2C 22 70 61 72 61 6D 73 22 3A 7B 22 4C 45 44 53 77 69 74 63 68 22 3A 31 7D 2C 22 76 65 72 73 69 6F 6E 22 3A 22 31 2E 30 2E 30 22 7D 
		*/

		if(wifi.rx_buff[BuffCount++] == 0x30)
		{
			if(wifi.rx_buff[BuffCount] &0x80)
			{
				wifi.rx_buff[BuffCount] &=~(0x01<<7);
				OverLen = wifi.rx_buff[BuffCount+1]*128 +wifi.rx_buff[BuffCount];
				BuffCount += 2;
			}
			else
			{
				OverLen = wifi.rx_buff[BuffCount];
				BuffCount++;
			}
			TopicLen = (wifi.rx_buff[BuffCount]<<8)|wifi.rx_buff[BuffCount+1];
			BuffCount += 2;
			
			BuffCount+=TopicLen;

			MQTT_GETJson_Parse(&wifi.rx_buff[BuffCount]);
			
		}
		WIFI_ClearData();
	}
}

/*******************************************************************************
函数名称：MQTT_PublishAck_Deal
函数作用：发布响应解析
*********************************************************************************/
uint8_t MQTT_PublishAck_Deal(void)
{
	uint8_t result = 0x02;
	if(wifi.rx_over)
	{
		uint32_t OverLen = 0;
		uint16_t BuffCount = 0;
		uint16_t TopicLen = 0;
		/*
		固定报头：30 99 01  剩余长度=153
		TopicLEN： 00 33 
		2F 73 79 73 2F 61 31 62 61 77 70 69 7A 69 6F 68 2F 49 4F 54 5F 32 30 36 2F 74 68 69 6E 67 2F 73 65 72 76 69 63 65 2F 70 72 6F 70 65 72 74 79 2F 73 65 74 
		7B 22 6D 65 74 68 6F 64 22 3A 22 74 68 69 6E 67 2E 73 65 72 76 69 63 65 2E 70 72 6F 70 65 72 74 79 2E 73 65 74 22 2C 22 69 64 22 3A 22 31 31 39 31 36 37 34 30 39 33 22 2C 22 70 61 72 61 6D 73 22 3A 7B 22 4C 45 44 53 77 69 74 63 68 22 3A 31 7D 2C 22 76 65 72 73 69 6F 6E 22 3A 22 31 2E 30 2E 30 22 7D 
		*/
		
		if(wifi.rx_buff[BuffCount++] == 0x30)
		{
			if(wifi.rx_buff[BuffCount] &0x80)
			{
				wifi.rx_buff[BuffCount] &=~(0x01<<7);
				OverLen = wifi.rx_buff[BuffCount+1]*128 +wifi.rx_buff[BuffCount];
				BuffCount += 2;
			}
			else
			{
				OverLen = wifi.rx_buff[BuffCount];
				BuffCount++;
			}			
			
			TopicLen = (wifi.rx_buff[BuffCount]<<8)|wifi.rx_buff[BuffCount+1];
			BuffCount += 2;		

			BuffCount+=TopicLen;
			
			/*
			{
					"code":200,
					"data":{

					},
					"id":"230788029",
					"message":"success",
					"method":"thing.event.property.post",
					"version":"1.0"
			}
			*/
			cJSON *root = NULL,*json_results = NULL;
			//JSON解析返回的数据
			root = cJSON_Parse((char *)&wifi.rx_buff[BuffCount]);
			
			printf("解析字符串=%s\r\n",&wifi.rx_buff[BuffCount]);
			if(!root)
			{
				printf("root Error before: %s\n", cJSON_GetErrorPtr());
			}
			else
			{
				json_results = cJSON_GetObjectItem(root,"message");//获取参数对象
				if(!json_results)
				{
					printf("results Error before: %s\n", cJSON_GetErrorPtr());
				}
				else
				{
					printf("响应=%s\r\n",json_results->valuestring);
					result=0;
				}
			}
			cJSON_Delete(root);
		}
		WIFI_ClearData();
	}
	return result;
}
/*******************************************************************************
函数名称：MQTT_GETJson_Parse
函数作用：解析MQTT协议中的JSON数据
*********************************************************************************/
uint8_t MQTT_GETJson_Parse(uint8_t *str)
{
	uint8_t send_state = 0;
	uint8_t send_state1 = 0;//开关变量----------------------------------------------------------------
	uint8_t send_state2=0;//--------------------------------------------------------------------------
	
	cJSON *root = NULL,*json_results = NULL, *value = NULL;
	 
	cJSON *root1 = NULL,*json_results1 = NULL, *value1 = NULL;//我定义的-------------------------------------------------------------------------
	cJSON *root2 = NULL,*json_results2 = NULL, *value2 = NULL;//我定义的-------------------------------------------------------------------------
	//JSON解析返回的数据
	root = cJSON_Parse((char *)str);
	root1 = cJSON_Parse((char *)str);
	root2 = cJSON_Parse((char *)str);
//--------------------------------------------------------------------------------------------------
	if(!root)
	{
		printf("root Error before: %s\n", cJSON_GetErrorPtr());
		return 1;
	}
	else
	{
		json_results = cJSON_GetObjectItem(root,"params");//获取参数对象
		if(!json_results)
		{
			printf("results Error before: %s\n", cJSON_GetErrorPtr());
			return 2;
		}
		else
		{
			//XYD:因CJSON格式不好传输出去，特之间再次解析判断
			value = cJSON_GetObjectItem(json_results,"Beep");
			if(value)
			{
				send_state=1;
				Ali_data.Beep_State = value->valueint;
				
				
				if(Ali_data.Beep_State)	BEEP(ON);
				else                    BEEP(OFF);				
				printf("Beep_State=%d\r\n", value->valueint);
			}
			
				//回传响应给阿里云：
			if(send_state)
			{
				char Send_JsonBuf[512] = "\0";
				/*发布数据：*/
				sprintf((char *)Send_JsonBuf,
					"{\"method\":\"thing.service.property.post\",\
						\"id\":\"1111\",\
						\"params\":{\
						\"Humidity\":%d,\
						\"IndoorTemperature\":%d,\
                        \"LightLux\":%d,\
                        \"Smoke\":%d,\
                        \"Beep\":%d,\
				                 \"LEDSwitch\":%d,\
				                \"RGBTHREE\":%d,\
				}\
						,\"version\":\"1.0\"}",
						Ali_data.Hum, Ali_data.Tem,Ali_data.Light,Ali_data.Smoke,Ali_data.Beep_State,Ali_data.LEDSwitch_State ,Ali_data.RGBTHREE_State );
				if(MyMQTT_Publish(Ali_Buf.topic_post,Send_JsonBuf)!=0)
					printf("发布响应失败\r\n");
				else
					printf("发布响应成功\r\n");
				 
			}
		}
	}
	cJSON_Delete(root);
//	return 0;
//*********************************************************************************
 
//-----------------------------------------------------------------------------------------------------
if(!root1)
	{
		printf("root1 Error before: %s\n", cJSON_GetErrorPtr());
		return 1;
	}
	else
	{
		json_results1 = cJSON_GetObjectItem(root1,"params");//获取参数对象
		if(!json_results1)
		{
			printf("results1 Error before: %s\n", cJSON_GetErrorPtr());
			return 2;
		}
		else
		{
			//XYD:因CJSON格式不好传输出去，特之间再次解析判断
			value1 = cJSON_GetObjectItem(json_results1,"LEDSwitch");
			if(value1)
			{
				send_state1=1;
				Ali_data.LEDSwitch_State = value1->valueint;
				
				
				if(Ali_data.LEDSwitch_State)	LED2(1);
				else                    LED2(0);				
				printf("LEDSwitch_State=%d\r\n", value1->valueint);
			}
	//回传响应给阿里云：
			if(send_state1)
			{
				char Send_JsonBuf[512] = "\0";
				/*发布数据：*/
				sprintf((char *)Send_JsonBuf,
					"{\"method\":\"thing.service.property.post\",\
						\"id\":\"1111\",\
						\"params\":{\
						\"Humidity\":%d,\
						\"IndoorTemperature\":%d,\
                        \"LightLux\":%d,\
                        \"Smoke\":%d,\
                        \"Beep\":%d,\
				                 \"LEDSwitch\":%d,\
				                \"RGBTHREE\":%d,\
						}\
						,\"version\":\"1.0\"}",
						Ali_data.Hum, Ali_data.Tem,Ali_data.Light,Ali_data.Smoke,Ali_data.Beep_State,Ali_data.LEDSwitch_State ,Ali_data.RGBTHREE_State);
				if(MyMQTT_Publish(Ali_Buf.topic_post,Send_JsonBuf)!=0)
					printf("发布响应失败\r\n");
				else
					printf("发布响应成功\r\n");
				  
			}
		}
	}
	cJSON_Delete(root1);
//*************************************************************************************************
	if(!root2)
	{
		printf("root2 Error before: %s\n", cJSON_GetErrorPtr());
		return 1;
	}
	else
	{
		json_results2 = cJSON_GetObjectItem(root2,"params");//获取参数对象
		if(!json_results2)
		{
			printf("results2 Error before: %s\n", cJSON_GetErrorPtr());
			return 2;
		}
		else
		{
		
	//回传响应给阿里云：
			if(send_state2)
			{
				char Send_JsonBuf[512] = "\0";
				/*发布数据：*/
				sprintf((char *)Send_JsonBuf,
					"{\"method\":\"thing.service.property.post\",\
						\"id\":\"1111\",\
						\"params\":{\
						\"Humidity\":%d,\
						\"IndoorTemperature\":%d,\
                        \"LightLux\":%d,\
                        \"Smoke\":%d,\
                        \"Beep\":%d,\
				                \"LEDSwitch\":%d,\
				                \"RGBTHREE\":%d,\
						}\
						,\"version\":\"1.0\"}",
						Ali_data.Hum, Ali_data.Tem,Ali_data.Light,Ali_data.Smoke,Ali_data.Beep_State,Ali_data.LEDSwitch_State ,Ali_data.RGBTHREE_State);
				if(MyMQTT_Publish(Ali_Buf.topic_post,Send_JsonBuf)!=0)
					printf("发布响应失败\r\n");
				else
					printf("发布响应成功\r\n");
				  
			}
		}
	}
	cJSON_Delete(root2);
//*************************************************************************************************
	
	return 0;





}
uint8_t MQTT_Ping(void)
{
	WIFI_ClearTxData();
	wifi.tx_buff[wifi.tx_count++] = 0xC0;
	wifi.tx_buff[wifi.tx_count++] = 0x00;
	WIFI_SendBuff(wifi.tx_buff, wifi.tx_count);
	return AliIOT_MQTTAckCheck(0xD0, 2000);
}
