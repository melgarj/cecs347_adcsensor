// ADCTestMain.c
// Runs on LM4F120/TM4C123
// This program periodically samples ADC channel 1 and stores the
// result to a global variable that can be accessed with the JTAG
// debugger and viewed with the variable watch feature.
// Daniel Valvano
// October 20, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// input signal connected to PE2/AIN1

#include "ADCSWTrigger.h"
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "Nokia5110.h"

int period = 0; // set value that will give 40Hz / 25 ms
char sample = 0;
// {adc, distance}
// 70 - 10 cm
//int adcTable[] = {548,583,636,692,732,791,857,887,1059,1343,1730,2276,3385};
int adcTable[] = {4095, 3385, 2276, 1730, 1343, 1059, 887, 857, 791, 732, 692, 636, 583, 548, 0};
int distTable[] = {0, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 999};
float distance = 0;
	float calibration = 0;
  float a = 0;
	float b = 0;
	int ia = 0;
	int ib = 0;
  float m = 0;
  float l = 0;
int i;
int f;

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
void SysTick_Init(void);

volatile unsigned long ADCvalue;
// The digital number ADCvalue is a representation of the voltage on PE4 
// voltage  ADCvalue
// 0.00V     0
// 0.75V    1024
// 1.50V    2048
// 2.25V    3072
// 3.00V    4095
int main(void){unsigned long volatile delay;
  PLL_Init();                           // 80 MHz
  ADC0_InitSWTriggerSeq3_Ch1();         // ADC initialization PE2/AIN1
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // activate port F
  delay = SYSCTL_RCGC2_R;
  GPIO_PORTF_DIR_R |= 0x04;             // make PF2 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x04;          // disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x04;             // enable digital I/O on PF2
                                        // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF0FF)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;               // disable analog functionality on PF
	SysTick_Init();
	PLL_Init();                           // set system clock to 50 MHz
  Nokia5110_Init(); 
  Nokia5110_Clear();
	
  while(1){
    if(sample){
		sample = 0;	
    ADCvalue = ADC0_InSeq3(); // Ensure sampler works
	
		// Find distance
		for(i = 14; i < 0; i = i - 1){
			if(ADCvalue > adcTable[i]){
				a = adcTable[i];
				ia = i;
			}
			else{
				break;
			}
		}
		
		for(f = 0; f < 15; f = f + 1){
			if(ADCvalue < adcTable[f]){
				b = adcTable[f];
				ib = f;
			}
			else {
				break;
			}
		}
		 m = b - a;
		 l = b - ADCvalue;
		
		distance = distTable[ib] + (l/m * 5);
		calibration = (39629.72122/ADCvalue) - 4.15302219;
		
		

		GPIO_PORTF_DATA_R ^= 0x04;
		Nokia5110_SetCursor(0,0);
    Nokia5110_OutString("ADC Output:");
		Nokia5110_SetCursor(0,1);
		Nokia5110_OutUDec(ADCvalue);
		Nokia5110_SetCursor(0,2);
		Nokia5110_OutString("Distance (LU):");
		Nokia5110_SetCursor(0,3);
		Nokia5110_OutUDec(distance);
		Nokia5110_SetCursor(0,4);
		Nokia5110_OutString("Calibration:");
		Nokia5110_SetCursor(0,5);
		Nokia5110_OutUDec(calibration);
	}
  }
}

// Can sample at 20 or 10 Hz also
void SysTick_Init(void){
	NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = 2000000-1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                              // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = 0x07;
}

void SysTick_Handler(){
	sample = 1;
}

// Current equation: y = (39,629.72122/x) - 4.15302219
