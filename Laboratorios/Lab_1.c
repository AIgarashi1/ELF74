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

#define GPIO_PA0_U0RX           0x00000001
#define GPIO_PA1_U0TX           0x00000401

#define LED_PORTN      GPIO_PORTN_BASE
#define LED_PORTF      GPIO_PORTF_BASE
#define LED_PIN_1      GPIO_PIN_1
#define LED_PIN_0      GPIO_PIN_0
#define LED_PIN_4			 GPIO_PIN_4


volatile unsigned int SysTicks1ms;
volatile unsigned int passo;
uint32_t SysClock;
unsigned char rxbuffer[2];
volatile bool timer = false;
volatile unsigned int state = 0;

void SysTickIntHandler(void);
void UART_Interruption_Handler(void);
void SetupSystick(void);
void SetupUart(void);
void Config(void);
void Leds_A(void);
void Leds_H(void);


int main(void) {
	
	SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
	
	SetupSystick();
	
	Config();
	
	SetupUart();
	
	while(1) {
		state = 0;
		if(rxbuffer[0]=='H') {  //horario
			rxbuffer[0] = 0;
			passo = rxbuffer[1] - '0';
			passo = passo * 4;
			while(passo--){
				Leds_H();
			}
			
		}
		
		if(rxbuffer[0]=='A') {  // anti horario
			rxbuffer[0] = 0;
			passo = rxbuffer[1] - '0';
			passo = passo * 4;
			while(passo--){
				Leds_A();
			}
		}
		
	}
	
	
}



void SysTickIntHandler(void)
{
  SysTicks1ms++;
	if(SysTicks1ms>= 10000)
		timer = true;
}


void UART_Interruption_Handler(void) 
{
  uint8_t last;
  //limpar IRQ exec
  UARTIntClear(UART0_BASE,UARTIntStatus(UART0_BASE,true));
  // Ler o pr?ximo caractere na uart.
  last = (uint8_t)UARTCharGetNonBlocking(UART0_BASE);
  //rotacionar buffer circular
  rxbuffer[0]=rxbuffer[1];
  rxbuffer[1]=last;
}


void SetupSystick(void)
{
  SysTicks1ms=0;
  //desliga o SysTick para poder configurar
  SysTickDisable();
  //clock 120MHz <=> SysTick deve contar 1ms=120k - 1 do Systick_Counter - 12 trocas de contexto PP->IRQ - (1T Mov, 1T Movt, 3T LDR, 1T INC ... STR e IRQ->PP j? n?o contabilizam atrasos para a vari?vel)  
  SysTickPeriodSet(120000-1-12-6);
  //registra a fun??o de atendimento da interrup??o
  SysTickIntRegister(SysTickIntHandler);
  //liga o atendimento via interrup??o
  SysTickIntEnable();
  //liga novamente o SysTick
  SysTickEnable();
}


void SetupUart(void)
{
  //Habilitar porta serial a 115200 com interrup??o seguindo sequencia de inicializa??es abaixo:
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
  UARTConfigSetExpClk(UART0_BASE, SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  UARTFIFODisable(UART0_BASE);
  UARTIntEnable(UART0_BASE,UART_INT_RX);
  UARTIntRegister(UART0_BASE,UART_Interruption_Handler);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE,(GPIO_PIN_0|GPIO_PIN_1));
}


void Config(void){
    // Enable peripherals for LED and Button
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
	  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Configure LED_PIN as output
    GPIOPinTypeGPIOOutput(LED_PORTN, LED_PIN_1);
	  GPIOPinTypeGPIOOutput(LED_PORTN, LED_PIN_0);
		GPIOPinTypeGPIOOutput(LED_PORTF, LED_PIN_0);
	  GPIOPinTypeGPIOOutput(LED_PORTF, LED_PIN_4);
	
}

void Leds_A(void) {
	switch (state){
		case 0:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, LED_PIN_1);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, 0);
			state++;
			break;
		case 1:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, 0);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, LED_PIN_0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, 0);
			state++;
			break;
		case 2:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, 0);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, LED_PIN_4);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, 0);
			state++;
			break;
		case 3:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, 0);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, LED_PIN_0);
			state = 0;
		
	}		
}

void Leds_H(void) {
	switch (state){
		case 0:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, 0);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, LED_PIN_0);
			state++;
			break;
		case 1:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, 0);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, LED_PIN_4);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, 0);
			state++;
			break;
		case 2:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, 0);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, LED_PIN_0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, 0);
			state++;
			break;
		case 3:
			GPIOPinWrite(LED_PORTN, LED_PIN_1, LED_PIN_1);
			GPIOPinWrite(LED_PORTN, LED_PIN_0, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_4, 0);
			GPIOPinWrite(LED_PORTF, LED_PIN_0, 0);
			state = 0;
		
	}		
}