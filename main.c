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
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2); //�����ж����ȼ�����
	SysTick_Init(72000);//1msִ��һ���ж�
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
	AliIOT_ConnetServer();//���Ӱ����Ʒ�����
	MQTT_connect_flag=MyMQTT_Connect(Ali_Buf.ClientID,Ali_Buf.UserName,Ali_Buf.PassWord);
	printf("MQTT_connect_flag=%d\r\n",MQTT_connect_flag);
	if(MQTT_connect_flag!=0)
		printf("����MQTTʧ��\r\n");
	else
	{
		printf("���Ӱ������豸�ɹ�\r\n");
		MyMQTT_Subscribe(Ali_Buf.topic_set);//������������
		MyMQTT_UnSubscribe(Ali_Buf.topic_post_reply);//ȡ�����ķ�����Ӧ
		MyMQTT_UnSubscribe(Ali_Buf.topic_post);//ȡ�����ķ���
		aliiot_connect_flag=1;
	}
	while(1)
	{
		
//		MQTT_Getdatas_Deal();
		if(aliiot_connect_flag)
		{		
			MQTT_Getdatas_Deal();   //�������ĵİ������·�����
		}
		if(Task_time[0]>=Task_time[1])//ʱ��Ƭ����ָʾ��
		{
			LED1_TOGGLE;
			Task_time[0]=0;
		}
		if(ALiData_time[0]>=ALiData_time[1])//ʱ��Ƭ���������ϴ�
		{
			
			DHT11_ReadData(&tmp,&hum);
			Ali_data.Hum = hum;
      Ali_data.Tem = tmp; 

			Ali_data.Light = adc_value.adc_data[0];
			Ali_data.Smoke = adc_value.adc_data[1];
			
			
     //2. ��������
			aliiot_cnt++;
			 
			if(aliiot_cnt == 5) //5s�ϴ�һ��
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
				if(MyMQTT_Publish(Ali_Buf.topic_post,Send_JsonBuf) != 0)//��������:ע����ȡ����Ӧ�Ķ��ģ��غ����ڲ����жϻش���ֱ���϶������ɹ�
					printf("��������ʧ��\r\n");
				else
					printf("�������ݳɹ�\r\n");	
			}
			
			 
			else 
			{	
				MQTT_Ping();    //MQTT����������
			}
			ALiData_time[0]=0;
		}
		if(ADC_time[0]>=ADC_time[1])//ʱ��Ƭ����
		{
			Relay_Getstatue();
			sprintf(buff,"����ǿ��%d",adc_value.adc_data[0]);
			Draw_Text_8_16_Str(0,0,WHITE,BLACK,buff,0);
			sprintf(buff,"����Ũ��%d",adc_value.adc_data[1]);
			Draw_Text_8_16_Str(0,16,WHITE,BLACK,buff,0);
			sprintf(buff,"�¶�%0.1f",tmp);
			Draw_Text_8_16_Str(0,32,WHITE,BLACK,buff,0);
			sprintf(buff,"ʪ��%0.1f",hum);
			Draw_Text_8_16_Str(0,48,WHITE,BLACK,buff,0);
			Draw_Text_8_16_Str(0,64,WHITE,BLACK,Relay_statue,0);
			ADC_time[0]=0;
		}
	}
}

