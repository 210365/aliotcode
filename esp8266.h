#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"

#define WIFI_PIN_PORT				GPIOB
#define WIFI_PIN_CLK				RCC_APB2Periph_GPIOB
#define WIFI_PIN_CLK_CMD		RCC_APB2PeriphClockCmd
#define WIFI_TX_PIN					GPIO_Pin_10
#define WIFI_RX_PIN					GPIO_Pin_11

#define WIFI_UART						USART3
#define WIFI_UART_CLK				RCC_APB1Periph_USART3
#define WIFI_UART_CLK_CMD		RCC_APB1PeriphClockCmd
#define WIFI_IRQn						USART3_IRQn
#define WIFI_IRQHandler			USART3_IRQHandler


#define ESP8266_EN_CLK     				RCC_APB2Periph_GPIOE
#define ESP8266_EN_CLK_INIT				RCC_APB2PeriphClockCmd

#define ESP8266_EN_PORT			GPIOE
#define ESP8266_EN_PIN			GPIO_Pin_6

#define ESP8266_ENABLE()	(GPIO_SetBits(ESP8266_EN_PORT, ESP8266_EN_PIN))
#define ESP8266_DISABLE()	(GPIO_ResetBits(ESP8266_EN_PORT, ESP8266_EN_PIN))


#define WIFI_MAX_LEN 1024

#define ID "ChinaNet-2.4G-2063"
#define PD "15193941304.jyc"
typedef struct{
	uint32_t rx_count;							//保存接收到的数据
	uint8_t rx_buff[WIFI_MAX_LEN];	//保存接收到的数据
	uint8_t rx_over;								//WiFi接收完成标志位
	uint16_t rx_time;								//接收超时计数
	uint8_t tx_buff[WIFI_MAX_LEN];
	uint32_t tx_count;
}__WIFI_MESSAGE;
extern __WIFI_MESSAGE wifi;

/*WIFI模块返回值*/
typedef enum {
	WIFI_RES_OK=0,
	WIFI_ACK_ERROR
}WIFI_RES_E;

void Usart3_Config(uint32_t brr);
void WIFI_SendStr(char * str);
void WIFI_SendBuff(uint8_t * buff, uint16_t len);
void WIFI_ClearData(void);
void WIFI_TimeTick(void);

uint8_t Wifi_Initialization(char * SSID, char * PWD);
uint8_t Wifi_OpenTransmission(void);
void Wifi_CloseTransmission(void);

uint8_t Wifi_ConnectServer(char* mode, char* ip, uint16_t port);
uint8_t WIFI_DisconnectServer(void);
void WIFI_ClearTxData(void);
#endif
