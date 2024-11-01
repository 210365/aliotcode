#include "esp8266.h"
#include "delay.h"
 #include "string.h"

__WIFI_MESSAGE wifi = {.rx_count = 0, .rx_over = 0, .rx_time = 0};

//���ڳ�ʼ��
void Usart3_Config(uint32_t brr)
{
	//��ʱ��
	WIFI_PIN_CLK_CMD(WIFI_PIN_CLK, ENABLE);
	WIFI_UART_CLK_CMD(WIFI_UART_CLK, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure = {0};
	
	ESP8266_EN_CLK_INIT(ESP8266_EN_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = ESP8266_EN_PIN;
	GPIO_Init(ESP8266_EN_PORT, &GPIO_InitStructure);
	ESP8266_DISABLE();
	Delay_nop_nms(1000);
	ESP8266_ENABLE();
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = WIFI_TX_PIN;
	GPIO_Init(WIFI_PIN_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = WIFI_RX_PIN;
	GPIO_Init(WIFI_PIN_PORT, &GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructuer = {0};
	USART_InitStructuer.USART_BaudRate = brr;
	USART_InitStructuer.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructuer.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructuer.USART_Parity = USART_Parity_No;
	USART_InitStructuer.USART_StopBits = USART_StopBits_1;
	USART_InitStructuer.USART_WordLength = USART_WordLength_8b;
	USART_Init(WIFI_UART, &USART_InitStructuer);
	//ʹ�ܽ����ж�
	USART_ITConfig(WIFI_UART, USART_IT_RXNE, ENABLE);
	//����NVIC
	NVIC_InitTypeDef NVIC_InitStructure = {0};
	NVIC_InitStructure.NVIC_IRQChannel = WIFI_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//ʹ�ܴ���
	USART_Cmd(WIFI_UART, ENABLE);	
}

void WIFI_SendByte(uint8_t data)
{
	while(USART_GetFlagStatus(WIFI_UART, USART_FLAG_TC) == RESET);
	USART_SendData(WIFI_UART, data);
}

void WIFI_SendStr(char * str)
{
	while(*str != '\0')
		WIFI_SendByte(*str++);
}

void WIFI_SendBuff(uint8_t * buff, uint16_t len)
{
	for(uint16_t i=0; i<len; i++)
		WIFI_SendByte(buff[i]);
}

void WIFI_TimeTick(void)
{
	//Ĭ��Ϊ0���������Ϊ0�������ڶ�ʱ���ж������ӣ�ÿ�δ��ڽ����жϵ������ı�ֵΪ1
	if(wifi.rx_time)	wifi.rx_time++;//������Ϊ0������
	if(wifi.rx_time > 5) {
		wifi.rx_over = 1;
		wifi.rx_time = 0;//�������0
	}
}

void WIFI_IRQHandler(void)
{
	uint8_t data = 0;
	if(USART_GetFlagStatus(WIFI_UART, USART_FLAG_RXNE) == SET)
	{
		data = USART_ReceiveData(WIFI_UART);//��������Ҳ����ζ�� ��ձ�־λ
		USART1->DR = data;//����
		wifi.rx_buff[wifi.rx_count++] = data;
		wifi.rx_count %= WIFI_MAX_LEN;//��ֹ����Խ��
		wifi.rx_time = 1;
	}
}

/**
 * ���ܣ������ַ������Ƿ������һ���ַ���
 * ������
 *       dest��������Ŀ���ַ���
 *       src������������
 *       timeout: ��ѯ��ʱʱ��
 * ����ֵ�����ҽ��  �����������ַ����������ַ����е�λ��
 *						 = NULL  û���ҵ��ַ���		����ʧ��
 */
char * FindStr(char* dest, char* src, uint32_t timeout)
{
	while(timeout--)
	{
		if(wifi.rx_over){
			if(strstr(dest, src) != NULL)	break;
			else wifi.rx_over = 0;		
		}
		Delay_nop_nms(1);
	}
	return strstr(dest, src);
}

void WIFI_ClearData(void)
{
	memset(wifi.rx_buff, 0, sizeof(wifi.rx_buff));
	wifi.rx_over = 0;
	wifi.rx_count = 0;
}

uint8_t WIFI_SendCmd_RecAck(char *str, char *ack, uint32_t timeout)
{
	WIFI_ClearData();
	WIFI_SendStr(str);
	WIFI_SendStr("\r\n");
	
	if(FindStr((char *)wifi.rx_buff, ack, timeout) != NULL)
		return WIFI_RES_OK;
	else
		return WIFI_ACK_ERROR;
}

//��͸��
uint8_t Wifi_OpenTransmission(void)
{
	return WIFI_SendCmd_RecAck("AT+CIPMODE=1\r\n", "OK", 1000);
}

//�ر�͸��
void Wifi_CloseTransmission(void)
{
	WIFI_SendStr("+++");
	Delay_nop_nms(500);
}


uint8_t Wifi_Initialization(char * SSID, char * PWD)
{
	Usart3_Config(115200);//���õײ㴮��
	Wifi_CloseTransmission();
	if(WIFI_SendCmd_RecAck("AT", "OK", 200) == WIFI_RES_OK)
		printf("��⵽ esp8266\r\n");
	else{
		printf("û�м�⵽ esp8266\r\n");
		return WIFI_ACK_ERROR;
	}
	
	if(WIFI_SendCmd_RecAck("AT+CWMODE=1", "OK", 200) == WIFI_RES_OK)
		printf("Station ���óɹ�\r\n");
	else{
		printf("Station ����ʧ��\r\n");
		return WIFI_ACK_ERROR;
	}	
	sprintf((char*)wifi.tx_buff, "AT+CWJAP=\"%s\",\"%s\"", SSID, PWD);
	if(WIFI_SendCmd_RecAck((char*)wifi.tx_buff, "OK", 10000) == WIFI_RES_OK)
		printf("WIFI ���ӳɹ�\r\n");
	else{
		printf("WIFI ����ʧ��\r\n");
		return WIFI_ACK_ERROR;
	}
	return WIFI_RES_OK;
}

uint8_t Wifi_ConnectServer(char* mode, char* ip, uint16_t port)
{
	Wifi_CloseTransmission();//�˳�͸��
	
	sprintf((char*)wifi.tx_buff, "AT+CIPSTART=\"%s\",\"%s\",%d", mode, ip, port);
	if(WIFI_SendCmd_RecAck((char*)wifi.tx_buff, "OK", 10000) == WIFI_RES_OK)
	{
		printf("���ӷ������ɹ�\r\n");
	}
	else
	{
		printf("���ӷ�����ʧ��\r\n");
		return WIFI_ACK_ERROR;
	}
	Delay_nop_nms(1000);
	//����͸��ģʽ
	if(Wifi_OpenTransmission() != WIFI_RES_OK) 
		return WIFI_ACK_ERROR;
	
	if(WIFI_SendCmd_RecAck("AT+CIPSEND", ">", 500) != WIFI_RES_OK)
		return WIFI_ACK_ERROR;
	printf("ESP8266����͸��ģʽ\r\n>\r\n");
	return WIFI_RES_OK;
}


uint8_t WIFI_DisconnectServer(void)
{
	uint8_t rev = 0;
	WIFI_ClearData();//���������
	//1.�˳�͸��ģʽ
	Wifi_CloseTransmission();
	
	rev = WIFI_SendCmd_RecAck("AT+CIPMODE=0", "OK", 500);
	if(rev != WIFI_RES_OK)
		return WIFI_ACK_ERROR;
	//3.AT+CIPCLOSEָ��˳�����������
	rev =WIFI_SendCmd_RecAck("AT+CIPCLOSE", "CLOSED", 500);
	if(rev != WIFI_RES_OK)
		return WIFI_ACK_ERROR;
	printf("ESP8266�Ͽ��������ɹ�\r\n");//��ʾ��Ϣ	
	return WIFI_RES_OK;
}


void WIFI_ClearTxData(void)
{
	memset(wifi.tx_buff, 0, sizeof(wifi.rx_buff));
	wifi.tx_count = 0;
}


