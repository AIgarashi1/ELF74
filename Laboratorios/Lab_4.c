//Universidade Tecnologica Federal do Parana
//Sistemas Embarcados
//Prof: Dr. Eduardo Nunes
//Laboratorio 2
//Autores:	Andre Vaz Igarashi
//		    Gianlucca Fiori Oliveira
//			Italo Ribeiro Fabiani




#include <stdbool.h> 
#include <stdint.h>
#include <time.h>

#include "cmsis_os2.h" 
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"

#define GPIO_PA1_U0TX 0x00000401

#define UART_MODULE      UART0_BASE
#define UART_GPIO_PORT   GPIO_PORTA_BASE
#define UART_GPIO_RX_PIN GPIO_PIN_0
#define UART_GPIO_TX_PIN GPIO_PIN_1
#define UART_BAUDRATE    115200


uint32_t SysClock;

osThreadId_t  temperature_sensor;
osThreadId_t  luminosity_sensor;
osThreadId_t  transmiter_data;


typedef struct {
    int info;
		//uint8_t Idx;
}DATA;

osMessageQueueId_t mid_MsgQueue1;                // message queue id
osMessageQueueId_t mid_MsgQueue2;


void temperature_thread( void *argument);
void luminosity_thread( void *argument);
void sendData_thread( void *argument);
void SetupUart(void);
void UARTStringSend(const uint8_t *String, uint32_t tamanho);
void itoa(int value, char* result, int base);
int rand();
bool verify = false;
uint8_t data;
char string[20];


int main(void) {
	
		osKernelInitialize();                 // Initialize CMSIS-RTOS
		SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
		//SetupSystick();
		SetupUart();
		//UARTStringSend("Teste",5);
    //message_queue = osMessageCreate(osMessageQ(message_queue), NULL); 
		mid_MsgQueue1 = osMessageQueueNew(16, sizeof(DATA), NULL);
		mid_MsgQueue2 = osMessageQueueNew(16, sizeof(DATA), NULL);		
	
		temperature_sensor = osThreadNew(temperature_thread, NULL, NULL);
		luminosity_sensor = osThreadNew(luminosity_thread, NULL, NULL);
		transmiter_data = osThreadNew(sendData_thread, NULL, NULL);
		
		if (osKernelGetState() == osKernelReady) {
    osKernelStart();                    // Start thread execution
		}
	
       while(1);
}


void SetupUart(void)
{
  //Habilitar porta serial a 115200 com interrup??o seguindo sequencia de inicializa??es abaixo:
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
  UARTConfigSetExpClk(UART0_BASE, SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  UARTFIFODisable(UART0_BASE);
  //UARTIntEnable(UART0_BASE,UART_INT_RX);
  //UARTIntRegister(UART0_BASE,UART_Interruption_Handler);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
  //GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE,(GPIO_PIN_1));
}



//função para enviar string pela uart
void UARTStringSend(const uint8_t *String, uint32_t tamanho){
		while (tamanho--) UARTCharPut(UART0_BASE, *String++);
	}



void temperature_thread( void *argument){
DATA msg;
 
  while (1) {
    msg.info = rand();                                         // do some work...
		//msg.Idx = 1;
    osMessageQueuePut(mid_MsgQueue1, &msg, 0U, 0U);
		osDelay(25000);
  }

}

void luminosity_thread( void *argument){

DATA msg;
 
  while (1) {
    msg.info = rand();                                         // do some work...
		//msg.Idx    = 2;
    osMessageQueuePut(mid_MsgQueue2, &msg, 0U, 0U);
		osDelay(25000);
	}

}

void sendData_thread( void *argument){
		
		DATA msg;
		osStatus_t status1;
		osStatus_t status2;
 
  while (1) {
    status1 = osMessageQueueGet(mid_MsgQueue1, &msg, NULL, 0U);   // wait for message
    if (status1 == osOK) {
			itoa(msg.info, string, 10);
			UARTStringSend("Luminosidade: ",14);
      UARTStringSend(string,3); 
			UARTStringSend("\n",2);
			verify = true;
			osDelay(5000);
    }
		status2 = osMessageQueueGet(mid_MsgQueue2, &msg, NULL, 0U);   // wait for message
    if (status2 == osOK && verify) {
			itoa(msg.info, string, 10);
			UARTStringSend("Temperatura: ",13);
      UARTStringSend(string,3); 
			UARTStringSend("\n",2);
			verify = false;
			osDelay(5000);
    }
	}
	
}

void itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0';}

    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
}

int rand1() {
    static int previous = 0;
    int current;
    
    current = (previous * 1103515245 + 12345) & 0x7fffffff;
    previous = current;
    
    return current;
}

int rand2() {
    static int previous = 0;
    int current;
    
    current = (previous * 473489705 + 12345) & 0x7fffffff;
    previous = current;
    
    return current;
}
