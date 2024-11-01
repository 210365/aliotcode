#include "main.h"
#include "string.h"



int main(void)
{
	char buff[24]={0};
	float hum=0;
	float tmp=0;
	uint8_t aliiot_connect_flag = 0;
	uint8_t aliiot_cnt = 0;
	char Send_JsonBuf[512] = "\0";
	uint8_t MQTT_connect_flag=0;
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2); //设置中断优先级分组
	SysTick_Init(72000);//1ms执行一次中断
	Usart1_Config(115200);
	gpioc0init();
	led_config();
	beep_config();
	KEY_Config();
	LCD_Init();
	ADC_Config();
	rgb_config();
	Relay_Config();
	Motor_Config();
	Wifi_Initialization("ChinaNet-2.4G-2063", "15193941304.jyc");
	AliIOT_ConnetServer();//连接阿里云服务器
	MQTT_connect_flag=MyMQTT_Connect(Ali_Buf.ClientID,Ali_Buf.UserName,Ali_Buf.PassWord);
	printf("MQTT_connect_flag=%d\r\n",MQTT_connect_flag);
	if(MQTT_connect_flag!=0)
		printf("连接MQTT失败\r\n");
	else
	{
		printf("连接阿里云设备成功\r\n");
		MyMQTT_Subscribe(Ali_Buf.topic_set);//订阅属性设置
		MyMQTT_UnSubscribe(Ali_Buf.topic_post_reply);//取消订阅发布响应
		MyMQTT_UnSubscribe(Ali_Buf.topic_post);//取消订阅发布
		aliiot_connect_flag=1;
	}
	while(1)
	{
		
//		MQTT_Getdatas_Deal();
		if(aliiot_connect_flag)
		{		
			MQTT_Getdatas_Deal();   //解析订阅的阿里云下发数据
		}
		if(Task_time[0]>=Task_time[1])//时间片任务：指示灯
		{
			LED1_TOGGLE;
			Task_time[0]=0;
		}
		if(ALiData_time[0]>=ALiData_time[1])//时间片任务：数据上传
		{
			
			DHT11_ReadData(&tmp,&hum);
			Ali_data.Hum = hum;
      Ali_data.Tem = tmp; 

			Ali_data.Light = adc_value.adc_data[0];
			Ali_data.Smoke = adc_value.adc_data[1];
			
			
     //2. 发布数据
			aliiot_cnt++;
			 
			if(aliiot_cnt == 5) //5s上传一次
			{
				aliiot_cnt = 0;
				sprintf((char *)Send_JsonBuf,
					"{\"method\":\"thing.service.property.post\",\
						\"id\":\"1111\",\
						\"params\":{\
						\"temperature\":%d,\
						\"Humidity\":%d,\
                        \"LightLux\":%d,\
                        \"Smoke\":%d,\
                        \"Beep\":%d,\
				                \"LEDSwitch\":%d,\
				                \"RGBTHREE\":%d,\
						}\
						,\"version\":\"1.0\"}",
						Ali_data.Hum, Ali_data.Tem,Ali_data.Light,Ali_data.Smoke,Ali_data.Beep_State,Ali_data.LEDSwitch_State,Ali_data.RGBTHREE_State);
				if(MyMQTT_Publish(Ali_Buf.topic_post,Send_JsonBuf) != 0)//发布数据:注：已取消响应的订阅，特函数内部不判断回传，直接认定发布成功
					printf("发布数据失败\r\n");
				else
					printf("发布数据成功\r\n");	
			}
			
			 
			else 
			{	
				MQTT_Ping();    //MQTT发送心跳包
			}
			ALiData_time[0]=0;
		}
		if(ADC_time[0]>=ADC_time[1])//时间片任务
		{
			Relay_Getstatue();
			sprintf(buff,"光照强度%d",adc_value.adc_data[0]);
			Draw_Text_8_16_Str(0,0,WHITE,BLACK,buff,0);
			sprintf(buff,"烟雾浓度%d",adc_value.adc_data[1]);
			Draw_Text_8_16_Str(0,16,WHITE,BLACK,buff,0);
			sprintf(buff,"温度%0.1f",tmp);
			Draw_Text_8_16_Str(0,32,WHITE,BLACK,buff,0);
			sprintf(buff,"湿度%0.1f",hum);
			Draw_Text_8_16_Str(0,48,WHITE,BLACK,buff,0);
			Draw_Text_8_16_Str(0,64,WHITE,BLACK,Relay_statue,0);
			ADC_time[0]=0;
		}
	}
}

