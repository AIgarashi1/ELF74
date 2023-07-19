//Universidade Tecnologica Federal do Parana
//Sistemas Embarcados
//Prof: Dr. Eduardo Nunes
//Laboratorio 2
//Autores:	Andre Vaz Igarashi
//		    Gianlucca Fiori Oliveira
//			Italo Ribeiro Fabiani



#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
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


#include "cmsis_os2.h" // CMSIS-RTOS

#define LED_PORTN      GPIO_PORTN_BASE
#define LED_PORTF      GPIO_PORTF_BASE
#define LED_PIN_1      GPIO_PIN_1
#define LED_PIN_0      GPIO_PIN_0
#define LED_PIN_4	     GPIO_PIN_4



osThreadId_t LED1_thread;                
osThreadId_t LED2_thread;                
osThreadId_t LED3_thread;    



	
void Config(void);
int main(void);
void led1(void *argument);
void led2(void *argument);
void led3(void *argument);


void led1(void *argument) {
  for (;;) {
    GPIOPinWrite(LED_PORTN, LED_PIN_1, LED_PIN_1);
    osDelay(500);                /* call common signal function   */
    GPIOPinWrite(LED_PORTN, LED_PIN_1, 0);
		osDelay(500);
  }
}


void led2(void *argument) {
  for (;;) {
    GPIOPinWrite(LED_PORTN, LED_PIN_0, LED_PIN_0);
    osDelay(250);                /* call common signal function   */
    GPIOPinWrite(LED_PORTN, LED_PIN_0, 0);
		osDelay(250);
  }
}


void led3(void *argument) {
  for (;;) {
    GPIOPinWrite(LED_PORTF, LED_PIN_4, LED_PIN_4);
    osDelay(250);                /* call common signal function   */
    GPIOPinWrite(LED_PORTF, LED_PIN_4, 0);
		osDelay(250);
  }
}








int main(void) {
	Config();
	osKernelInitialize();                 // Initialize CMSIS-RTOS
	
  LED1_thread = osThreadNew(led1, NULL, NULL);
  LED2_thread = osThreadNew(led2, NULL, NULL);
  LED3_thread = osThreadNew(led3, NULL, NULL);
  if (osKernelGetState() == osKernelReady) {
    osKernelStart();                    // Start thread execution
  }
	
       while(1);
}






void Config(void){
    // Configure LED_PIN as output
	
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	
    GPIOPinTypeGPIOOutput(LED_PORTN, LED_PIN_1);
	  GPIOPinTypeGPIOOutput(LED_PORTN, LED_PIN_0);
	  GPIOPinTypeGPIOOutput(LED_PORTF, LED_PIN_4);
	
}