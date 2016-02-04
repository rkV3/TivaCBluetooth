//----------------------------------------------------------------
//....Author: RAMA KRISHNA AKULA...................................
//----26,Nov 2014--------------------------------------------------
//...........................................................------
//..............BLUETOOTH ON TIVA C LAUNCHPAD---------------------
//You are free to modify and use, provided the code must follow
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI and I SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.

//Contact
//www.ramakrishnaakula.com
//ramakrishna.akula1@gmail.com

//Tiva -------- HC05 Bluetooth Module
//PD6   ------  TXD
//PC7   ------  RXD
//3.3v  ------  Vc
//gnd   ------  gnd




#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include <string.h>

#define redLED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define blueLED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define greenLED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

char rxChar[10];//for PC application,uncomment this
char txChar;
uint8_t  timeron=60;
uint16_t count=0;
char countstr[10];
uint8_t index=0;

void writeCharToUart0(char c)
{
	while (UART0_FR_R & UART_FR_TXFF); //wait till Transmitter is not full
	UART0_DR_R = c; //write to UART0
}


void writeStringToUart0(char* str)   //write a string to Uart0
{
	int i;
    for (i = 0; i < strlen(str); i++)
    	writeCharToUart0(str[i]);
}


void writeCharToUart3(char c)
{
	while (UART3_FR_R & UART_FR_TXFF); //wait till Transmitter is not full
	UART3_DR_R = c; //write to UART3
}


void writeStringToUart3(char* str)
{
	int i;
    for (i = 0; i < strlen(str); i++)
    	writeCharToUart3(str[i]);
}

void Uart2InterruptIsr()//this interrupt routine is for receiving data from bluetooth
{
    rxChar[index] = UART2_DR_R;
	index++;
		if(rxChar[index-1]==13)
	{
		rxChar[index-1]='\0';
		if(strcmp(rxChar,"red")==0)
			redLED^=1;
		if(strcmp(rxChar,"blue")==0)
					blueLED^=1;
		if(strcmp(rxChar,"green")==0)
					greenLED^=1;
		index=0;
	}

    UART2_ICR_R=UART_ICR_RXIC;//clear interrupt
}


void Uart3InterruptIsr() //this interrupt routine is to transmit data over bluetooth
{
	txChar = UART3_DR_R;
    UART3_ICR_R=UART_ICR_TXIC; //clear interrupt
}

//This timer interrupt fires every 100ms
void Timer0InterruptIsr(void)
{

	if(timeron>0)
		timeron--;
	if(timeron==0)
	{
		timeron=60; //i.e every 6 sec this block will execute
		sprintf(countstr,"%u",count);
		writeStringToUart3(countstr);
		writeStringToUart3("\n\r");
	    count++;
	}

	TIMER0_ICR_R=TIMER_ICR_TATOCINT;//clear the interrupt
}

void setUp()
{

	    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);//40Mhz clock


		SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF|SYSCTL_RCGC2_GPIOC|SYSCTL_RCGC2_GPIOA|SYSCTL_RCGC2_GPIOB|SYSCTL_RCGC2_GPIOE|SYSCTL_RCGC2_GPIOD;
		GPIO_PORTF_DIR_R |= 0x0E;
		GPIO_PORTF_DEN_R |= 0x0E;

	    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0; //Clock for UART0
	    GPIO_PORTA_DEN_R |= 3;
	    GPIO_PORTA_AFSEL_R |= 3;
	    GPIO_PORTA_PCTL_R = GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;

	    UART0_CTL_R = 0;
	    UART0_CC_R = UART_CC_CS_SYSCLK;
	    UART0_IBRD_R = 21;
	    UART0_FBRD_R = 45;    // set baud rate as 1152000
	    UART0_LCRH_R = UART_LCRH_WLEN_8;
	    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;

    	SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R3;      //Clock for UART3
        GPIO_PORTC_DEN_R |= 0xC0;
    	GPIO_PORTC_AFSEL_R |= 0xC0;
        GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC7_U3TX; //PC7 is transmit i.e RXD in bluetooth module

       	// Configure UART3 to 9600 baud, 8N1 format
        UART3_CTL_R = 0;
    	UART3_CC_R = UART_CC_CS_SYSCLK;                  // use system clock (40 MHz)
        UART3_IBRD_R = 260;                               // r = 40 MHz / (Nx9600Hz), set floor(r)=260, where N=16
        UART3_FBRD_R = 27;                               // round(fract(r)*64)=27
        UART3_LCRH_R = UART_LCRH_WLEN_8; // configure for 8N1 w/o FIFO
        UART3_CTL_R = UART_CTL_TXE | UART_CTL_UARTEN; // enable TX and module
    	UART3_IM_R = UART_IM_TXIM;                       // turn-on TX interrupt
    	NVIC_EN1_R = 1<<27;//enable interrupt


        SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R2;            // turn-on UART2, leave other uarts in same status
    	GPIO_PORTD_DEN_R |= 0x40;
    	GPIO_PORTD_AFSEL_R |= 0x40;
    	GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD6_U2RX;//PD6 is recieve. i.e TXD in bluetooth module

        // Configure UART2 to 9600 baud, 8N1 format
    	UART2_CTL_R = 0;
        UART2_CC_R = UART_CC_CS_SYSCLK;                  // use system clock (40 MHz)
    	UART2_IBRD_R = 260;                               // r = 40 MHz / (Nx9600Hz), set floor(r)=260, where N=16
    	UART2_FBRD_R = 27;                               // round(fract(r)*64)=27
    	UART2_LCRH_R = UART_LCRH_WLEN_8; // configure for 8N1 w/o FIFO
    	UART2_CTL_R = UART_CTL_RXE | UART_CTL_UARTEN; // enable RX, and module
    	UART2_IM_R = UART_IM_RXIM;                       // turn-on RX interrupt
    	NVIC_EN1_R = 1<<1;//enable interrupt

    	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;
        TIMER0_CTL_R &= ~TIMER_CTL_TAEN;//reset timer1
    	TIMER0_CFG_R =TIMER_CFG_32_BIT_TIMER;
    	TIMER0_TAMR_R =TIMER_TAMR_TAMR_PERIOD;
        TIMER0_TAILR_R=4000000;//40000000/4000000=10Hz 100ms configure
    	TIMER0_IMR_R= TIMER_IMR_TATOIM; // enable timer0 interrupt for every 100ms
    	TIMER0_CTL_R  |= 0x00000001;
    	NVIC_EN0_R|=1<<19; //enable interrupt

}


int main(void)
{


    setUp();

    while(1);
}
