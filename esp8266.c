#include "esp8266.h"
#include "delay.h"
 #include "string.h"

__WIFI_MESSAGE wifi = {.rx_count = 0, .rx_over = 0, .rx_time = 0};

//串口初始化
void Usart3_Config(uint32_t brr)
{
	//开时钟
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
	//使能接收中断
	USART_ITConfig(WIFI_UART, USART_IT_RXNE, ENABLE);
	//配置NVIC
	NVIC_InitTypeDef NVIC_InitStructure = {0};
	NVIC_InitStructure.NVIC_IRQChannel = WIFI_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//使能串口
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
	//默认为0，接收完成为0，不会在定时器中断内增加；每次串口接收中断到来，改变值为1
	if(wifi.rx_time)	wifi.rx_time++;//计数不为0，增加
	if(wifi.rx_time > 5) {
		wifi.rx_over = 1;
		wifi.rx_time = 0;//计数清空0
	}
}

void WIFI_IRQHandler(void)
{
	uint8_t data = 0;
	if(USART_GetFlagStatus(WIFI_UART, USART_FLAG_RXNE) == SET)
	{
		data = USART_ReceiveData(WIFI_UART);//接收数据也就意味着 清空标志位
		USART1->DR = data;//调试
		wifi.rx_buff[wifi.rx_count++] = data;
		wifi.rx_count %= WIFI_MAX_LEN;//防止数组越界
		wifi.rx_time = 1;
	}
}

/**
 * 功能：查找字符串中是否包含另一个字符串
 * 参数：
 *       dest：待查找目标字符串
 *       src：待查找内容
 *       timeout: 查询超时时间
 * 返回值：查找结果  返回所查找字符串在整体字符串中的位置
 *						 = NULL  没有找到字符串		查找失败
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

//打开透传
uint8_t Wifi_OpenTransmission(void)
{
	return WIFI_SendCmd_RecAck("AT+CIPMODE=1\r\n", "OK", 1000);
}

//关闭透传
void Wifi_CloseTransmission(void)
{
	WIFI_SendStr("+++");
	Delay_nop_nms(500);
}


uint8_t Wifi_Initialization(char * SSID, char * PWD)
{
	Usart3_Config(115200);//配置底层串口
	Wifi_CloseTransmission();
	if(WIFI_SendCmd_RecAck("AT", "OK", 200) == WIFI_RES_OK)
		printf("检测到 esp8266\r\n");
	else{
		printf("没有检测到 esp8266\r\n");
		return WIFI_ACK_ERROR;
	}
	
	if(WIFI_SendCmd_RecAck("AT+CWMODE=1", "OK", 200) == WIFI_RES_OK)
		printf("Station 设置成功\r\n");
	else{
		printf("Station 设置失败\r\n");
		return WIFI_ACK_ERROR;
	}	
	sprintf((char*)wifi.tx_buff, "AT+CWJAP=\"%s\",\"%s\"", SSID, PWD);
	if(WIFI_SendCmd_RecAck((char*)wifi.tx_buff, "OK", 10000) == WIFI_RES_OK)
		printf("WIFI 连接成功\r\n");
	else{
		printf("WIFI 连接失败\r\n");
		return WIFI_ACK_ERROR;
	}
	return WIFI_RES_OK;
}

uint8_t Wifi_ConnectServer(char* mode, char* ip, uint16_t port)
{
	Wifi_CloseTransmission();//退出透传
	
	sprintf((char*)wifi.tx_buff, "AT+CIPSTART=\"%s\",\"%s\",%d", mode, ip, port);
	if(WIFI_SendCmd_RecAck((char*)wifi.tx_buff, "OK", 10000) == WIFI_RES_OK)
	{
		printf("连接服务器成功\r\n");
	}
	else
	{
		printf("连接服务器失败\r\n");
		return WIFI_ACK_ERROR;
	}
	Delay_nop_nms(1000);
	//设置透传模式
	if(Wifi_OpenTransmission() != WIFI_RES_OK) 
		return WIFI_ACK_ERROR;
	
	if(WIFI_SendCmd_RecAck("AT+CIPSEND", ">", 500) != WIFI_RES_OK)
		return WIFI_ACK_ERROR;
	printf("ESP8266进入透传模式\r\n>\r\n");
	return WIFI_RES_OK;
}


uint8_t WIFI_DisconnectServer(void)
{
	uint8_t rev = 0;
	WIFI_ClearData();//清接收数据
	//1.退出透传模式
	Wifi_CloseTransmission();
	
	rev = WIFI_SendCmd_RecAck("AT+CIPMODE=0", "OK", 500);
	if(rev != WIFI_RES_OK)
		return WIFI_ACK_ERROR;
	//3.AT+CIPCLOSE指令，退出服务器连接
	rev =WIFI_SendCmd_RecAck("AT+CIPCLOSE", "CLOSED", 500);
	if(rev != WIFI_RES_OK)
		return WIFI_ACK_ERROR;
	printf("ESP8266断开服务器成功\r\n");//提示信息	
	return WIFI_RES_OK;
}


void WIFI_ClearTxData(void)
{
	memset(wifi.tx_buff, 0, sizeof(wifi.rx_buff));
	wifi.tx_count = 0;
}


