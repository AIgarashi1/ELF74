#include <stdbool.h> 
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <utils/uartstdio.h>

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

#define GPIO_PA0_U0RX 0x00000001
#define GPIO_PA1_U0TX 0x00000401

#define UART_MODULE      UART0_BASE
#define UART_GPIO_PORT   GPIO_PORTA_BASE
#define UART_GPIO_RX_PIN GPIO_PIN_0
#define UART_GPIO_TX_PIN GPIO_PIN_1
#define UART_BAUDRATE    115200
#define DEMO_STACK_SIZE         1024
#define DEMO_QUEUE_SIZE         10

const osMutexAttr_t Uart_Mutex_attr = {
  "UartMutex",                	            // human readable mutex name
  osMutexRecursive | osMutexPrioInherit,    // attr_bits
  NULL,                                     // memory for control block   
  0U                                        // size for control block
  };

const osThreadAttr_t thread_attr = {
  .stack_size = 1024                            // Create the thread stack with a size of 1024 bytes
};
	
uint32_t SysClock;

osThreadId_t  elevador_esquerda;
osThreadId_t  elevador_centro;
osThreadId_t  elevador_direita;
osThreadId_t  receptor_uart;
osThreadId_t  main_thread;

osMessageQueueId_t queue_uart_esquerda;
osMessageQueueId_t queue_uart_centro;
osMessageQueueId_t queue_uart_direita;             
osMessageQueueId_t queue_interna_esquerda;
osMessageQueueId_t queue_interna_centro;            
osMessageQueueId_t queue_interna_direita;

osMutexId_t mutex_uart;


void SetupUart(void);
char floor_int_to_char(char decimal, char unidade);
void controle_elevador(void *local_elevador);
void uart_receive(void *argument);



int main(void) {
	
		osKernelInitialize();
		SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
		SetupUart();
		
		queue_uart_esquerda = osMessageQueueNew(DEMO_QUEUE_SIZE*8, 8, NULL);
		queue_uart_centro = osMessageQueueNew(DEMO_QUEUE_SIZE*8, 8, NULL);
		queue_uart_direita = osMessageQueueNew(DEMO_QUEUE_SIZE*8, 8, NULL);
		queue_interna_esquerda = osMessageQueueNew(DEMO_QUEUE_SIZE*8, 8, NULL);
		queue_interna_centro = osMessageQueueNew(DEMO_QUEUE_SIZE*8, 8, NULL);
		queue_interna_direita = osMessageQueueNew(DEMO_QUEUE_SIZE*8, 8, NULL);
		
		
		elevador_esquerda = osThreadNew(controle_elevador, "e", &thread_attr);
		while(osThreadGetState(elevador_esquerda) != osThreadReady );
		elevador_centro = osThreadNew(controle_elevador, "c", &thread_attr);
		while(osThreadGetState(elevador_centro) != osThreadReady );
		elevador_direita = osThreadNew(controle_elevador, "d", &thread_attr);
		while(osThreadGetState(elevador_direita) != osThreadReady );
		receptor_uart = osThreadNew(uart_receive, NULL, &thread_attr);
		while(osThreadGetState(receptor_uart) != osThreadReady );
		
		mutex_uart = osMutexNew(&Uart_Mutex_attr);
	
		if (osKernelGetState() == osKernelReady){
    osKernelStart();
		}
		while(1);
}

void SetupUart(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
  UARTConfigSetExpClk(UART0_BASE, SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  UARTFIFODisable(UART0_BASE);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE,(GPIO_PIN_0|GPIO_PIN_1));
	UARTFIFOEnable(UART0_BASE);
}


//converte o valor inteiro do andar (00 - 15) para caracter (a - p)
char floor_int_to_char(char decimal, char unidade){
	int floor = (decimal - '0') * 10 + (unidade - '0');
	
	switch (floor) {
		case 0:
			return 'a';
		case 1:
			return 'b';
		case 2:
			return 'c';
		case 3:
			return 'd';
		case 4:
			return 'e';
		case 5:
			return 'f';
		case 6:
			return 'g';
		case 7:
			return 'h';
		case 8:
			return 'i';
		case 9:
			return 'j';
		case 10:
			return 'k';
		case 11:
			return 'l';
		case 12:
			return 'm';
		case 13:
			return 'n';
		case 14:
			return 'o';
		case 15:
			return 'p';
		default:
			return NULL;
    }
}

//thread dos elevadores
void controle_elevador(void *local_elevador){
	char comando_recebido[8];
	char comando_atual[8];
	char andar_atual = 'a';
	char andar_destino ='a';
	osStatus_t status;
	int comandos_armazenados = 0;
	int esta_movendo = 0;
	int comando_executando = 0;
	char pos_elevador;
	//verifica para qual elevador e a thread
	if(local_elevador=="e")
			pos_elevador = 'e';
	else if(local_elevador=="c")
			pos_elevador = 'c';
	else if(local_elevador=="d")
			pos_elevador = 'd';
  
	//pega o mutex
	status =  osMutexAcquire (mutex_uart, osWaitForever);
	if (status != osOK)
    return;
  
	//inicializa o elevador
	UARTCharPut(UART0_BASE,pos_elevador);
	UARTCharPut(UART0_BASE,'r');
	UARTCharPut(UART0_BASE,'\r');
    
	//abre as portas
	UARTCharPut(UART0_BASE,pos_elevador);
	UARTCharPut(UART0_BASE,'a');
	UARTCharPut(UART0_BASE,'\r');
	
	//libera o mutex
	status =  osMutexRelease (mutex_uart);
	if (status != osOK)
		return;

  
	while(1){
		//limpa e armazena o comando recebido da queue
		memset(comando_recebido, 0, sizeof comando_recebido);
		//recebe o conteudo da fila de leitura da uart
		if(pos_elevador=='e')
			status = osMessageQueueGet (queue_uart_esquerda, comando_recebido, NULL, 2); 
		else if(pos_elevador=='c')
			status = osMessageQueueGet (queue_uart_centro, comando_recebido, NULL, 2);  
		else if(pos_elevador=='d')
			status = osMessageQueueGet (queue_uart_direita, comando_recebido, NULL, 2);  

    //verifica se recebeu a posicao atual do elevador
		if(comando_recebido[1]>='0' && comando_recebido[1]<='9'){
			if(comando_recebido[2]>='0' && comando_recebido[2]<='9')
				andar_atual = floor_int_to_char(comando_recebido[1], comando_recebido[2]);
			else
				andar_atual = floor_int_to_char('0', comando_recebido[1]);
		}
		//verifica se foi enviado algum comando do simulador
		if(comando_recebido[1]=='E' || comando_recebido[1]=='I'){
			if(comando_recebido[1]=='I'){
				//pega mutex
				status =  osMutexAcquire (mutex_uart, osWaitForever);
			if (status != osOK)
				break;
        
			//coloca comando para acender LED
			UARTCharPut(UART0_BASE,pos_elevador);
			UARTCharPut(UART0_BASE,'L');
			UARTCharPut(UART0_BASE,comando_recebido[2]);
			UARTCharPut(UART0_BASE,'\r');

			//libera mutex
			status =  osMutexRelease (mutex_uart);
			if (status != osOK)
				break;
			}

			comandos_armazenados ++;

			//coloca o comando na queue interna
			if(pos_elevador=='e'){
				status = osMessageQueuePut(queue_interna_esquerda, comando_recebido, NULL, osWaitForever);  
				if (status != osOK)
					break;
			}else if(pos_elevador=='c'){
				status = osMessageQueuePut(queue_interna_centro, comando_recebido, NULL, osWaitForever);
				if (status != osOK)
					break;
			}else if(pos_elevador=='d'){
				status = osMessageQueuePut(queue_interna_direita, comando_recebido, NULL, osWaitForever);
				if (status != osOK)
					break;
			}
		}

		//se tem algum comando armazenado e nao tem nenhum comando sendo executado, pega o próximo
		if(comandos_armazenados > 0 && comando_executando == 0){
			if(pos_elevador=='e'){
				status = osMessageQueueGet (queue_interna_esquerda, comando_atual, NULL, osWaitForever); 
				if (status != osOK)
					break;
			}else if(pos_elevador=='c'){
				status = osMessageQueueGet (queue_interna_centro, comando_atual, NULL, osWaitForever); 
				if (status != osOK)
					break;
			}else if(pos_elevador=='d'){
				status = osMessageQueueGet (queue_interna_direita, comando_atual, NULL, osWaitForever); 
				if (status != osOK)
					break;
			}
		comando_executando = 1;
		comandos_armazenados--;
		}

		//dependendo do comando, salva o andar destino, se for E (dezena + decimal) -> char referente ao andar
		if(comando_atual[1]=='E')
			andar_destino  = floor_int_to_char(comando_atual[2], comando_atual[3]);
		//se for I pega o proprio andar recebido como char
		else if(comando_atual[1]=='I')
			andar_destino = comando_atual[2];
		//verifica se o elevador deve subir
		if(andar_destino > andar_atual && esta_movendo == 0){
			osDelay(10000);
		
			//pega o mutex
			status =  osMutexAcquire (mutex_uart, osWaitForever);
			if (status != osOK)
				break;
      
			//fecha as portas
			UARTCharPut(UART0_BASE,pos_elevador);
			UARTCharPut(UART0_BASE,'f');
			UARTCharPut(UART0_BASE,'\r');
		
			//libera o mutex
			status =  osMutexRelease (mutex_uart);
			if (status != osOK)
				break;
			
			osDelay(4000);
			
			//pega o mutex
			status =  osMutexAcquire (mutex_uart, osWaitForever);
			if (status != osOK)
				break;
			
			//manda comando para subir o elevador
			UARTCharPut(UART0_BASE,pos_elevador);
			UARTCharPut(UART0_BASE,'s');
			UARTCharPut(UART0_BASE,'\r');
      
			//libera o mutex
			status =  osMutexRelease (mutex_uart);
			if (status != osOK)
				break;
		memset(comando_atual, 0, sizeof comando_atual);
		esta_movendo = 1;
			
		//verifica se o elevador deve descer 	
		}else if(andar_destino < andar_atual && esta_movendo == 0){
			osDelay(10000);
		
			//pega o mutex
			status =  osMutexAcquire (mutex_uart, osWaitForever);
			if (status != osOK)
				break;
      
			//fecha as portas
			UARTCharPut(UART0_BASE,pos_elevador);
			UARTCharPut(UART0_BASE,'f');
			UARTCharPut(UART0_BASE,'\r');
		
			//libera o mutex
			status =  osMutexRelease (mutex_uart);
			if (status != osOK)
				break;
			
			osDelay(4000);
			
			//pega o mutex
			status =  osMutexAcquire (mutex_uart, osWaitForever);
			if (status != osOK)
				break;
			
			//manda o comando para descer o elevador
			UARTCharPut(UART0_BASE,pos_elevador);
			UARTCharPut(UART0_BASE,'d');
			UARTCharPut(UART0_BASE,'\r');
      
			//libera o mutex
			status =  osMutexRelease (mutex_uart);

			if (status != osOK)
				break;
		
		memset(comando_atual, 0, sizeof comando_atual);
		esta_movendo = 1;
		}
		//verifica se o elevador chegou no andar destino
		if(andar_destino == andar_atual && comando_executando == 1){
		//limpa comando atual
		comando_executando = 0;
		
		esta_movendo = 0;
		//pega o mutex
			status =  osMutexAcquire (mutex_uart, osWaitForever);
			if (status != osOK)
				break;
      
		//manda comando para parar o elevador
		UARTCharPut(UART0_BASE,pos_elevador);
		UARTCharPut(UART0_BASE,'p');
		UARTCharPut(UART0_BASE,'\r');
		
		//apaga o led
		UARTCharPut(UART0_BASE,pos_elevador);
		UARTCharPut(UART0_BASE,'D');
		UARTCharPut(UART0_BASE,andar_destino);
		UARTCharPut(UART0_BASE,'\r');
			
		//abre as portas
		UARTCharPut(UART0_BASE,pos_elevador);
		UARTCharPut(UART0_BASE,'a');
		UARTCharPut(UART0_BASE,'\r');

      
		//libera o mutex
		status =  osMutexRelease (mutex_uart);
		if (status != osOK)
			break;
		}
    osDelay(1);
	}
}



void uart_receive(void *argument){
  
	char receiving_char ;
	char comando_simulador[8];
	int comando_encerrado = 0;
	int posicao_buffer_interno = 0;
	
  osStatus_t status;

  while(1){
  //verifica a FIFO para leitura enquanto houver caracteres
  while(UARTCharsAvail(UART0_BASE)){
      //guarda o char recebido 
			receiving_char = (uint8_t)UARTCharGetNonBlocking(UART0_BASE);
	//ignora o caracter de inicializacao
	if(receiving_char!=0xFE){
      //se esta recebendo um comando 
      if(receiving_char!='\n' && receiving_char!='\r'){
        // guarda o char no comando recebido para compor ele
       comando_simulador[posicao_buffer_interno] = receiving_char;
       posicao_buffer_interno++;
        //se for F ou A ignora e limpa o vetor
        if(receiving_char=='F' || receiving_char=='A'){
          memset(comando_simulador, 0, sizeof comando_simulador);
          posicao_buffer_interno = 0;
        }
      }
			//se o comando foi finalizado
      else{
        posicao_buffer_interno = 0;
        comando_encerrado = 1;
        }
			osDelay(5);
			}
}
      //se acabou o comando envia para a fila respectiva
      if(comando_encerrado){
        if(comando_simulador[0]=='e'){
		   status = osMessageQueuePut(queue_uart_esquerda, comando_simulador, NULL, osWaitForever);
          if (status != osOK)
              break;
        }
        if(comando_simulador[0]=='c'){
			status = osMessageQueuePut(queue_uart_centro, comando_simulador, NULL, osWaitForever);
          if (status != osOK)
              break;
        }
        if(comando_simulador[0]=='d'){
			status = osMessageQueuePut(queue_uart_direita, comando_simulador, NULL, osWaitForever);
          if (status != osOK)
              break;
        }
        //apos o envio, reseta o comando recebido
        memset(comando_simulador, 0, sizeof comando_simulador);
        comando_encerrado = 0;
      }
     
      osDelay(1);
    }
  }