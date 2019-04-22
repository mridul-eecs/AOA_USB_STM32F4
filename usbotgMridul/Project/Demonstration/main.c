/**
  ******************************************************************************
  * @file    main.c
  * @author  Yuuichi Akagawa
  * @version V1.0.0
  * @date    2012/03/05
  * @brief   Android Open Accessory implementation
  ******************************************************************************
  * @attention
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *      http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  * <h2><center>&copy; COPYRIGHT (C)2012 Yuuichi Akagawa</center></h2>
  *
  ******************************************************************************
  */

/* Includes */

#define MEM_DEVICE_WRITE_ADDR 0x6a
#define MEM_DEVICE_READ_ADDR 0x6B
#define I2C_TIMEOUT_MAX 100000000

#include "stm32f4xx.h"
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include "stm32f4_discovery.h"
#include "usb_bsp.h"
#include "usbh_core.h"
#include "usbh_usr.h"
#include "usbh_adk_core.h"
#include "uart_debug.h"
#include "I2CLib.h"


/* Private macro */
/* Private variables */
__IO uint32_t TimingDelay;
int firsttime = 0;
int i = 1;
int chargershut = 0;
uint8_t * i2creaddata;
uint32_t readdata;
char binaryreaddata[9] = 0;
char* hextostring;
float voltage = 0;

/* Private function prototypes */
void chargercontrol_pins(void);
void Delay(__IO uint32_t nTime);
void TimingDelay_Decrement(void);
void readbatvoltage(void);
extern uint8_t Write_24Cxx(uint16_t Addr, uint8_t Data);
/** @defgroup USBH_USR_MAIN_Private_Variables
* @{
*/
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USB_OTG_CORE_HANDLE           USB_OTG_Core_dev __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USBH_HOST                     USB_Host __ALIGN_END ;
I2C_InitTypeDef* I2C1_InitStruct;
/**
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/

void init_I2C1(void);
uint8_t Read_24Cxx(uint16_t Addr, uint8_t Mem_Type);

int main(void)
{
  int i = 0;
  int idle_mscount = 0;
  int charge = 0;
  uint8_t msg[10];
  uint16_t len;
  RCC_ClocksTypeDef RCC_Clocks;
  /* Initialize LEDs and User_Button on STM32F4-Discovery --------------------*/
  
  


  /* SysTick end of count event each 1ms */
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

#ifdef DEBUG
  /* Init Debug out setting(UART2) */
  uart_debug_init();
#endif

  /* Init Host Library */
  USBH_DeInit(&USB_OTG_Core_dev, &USB_Host);
  USBH_Init(&USB_OTG_Core_dev,
            USB_OTG_FS_CORE_ID,
            &USB_Host,            &USBH_ADK_cb,
            &USR_Callbacks);
  chargercontrol_pins();

  GPIO_WriteBit(GPIOA, GPIO_Pin_8, 1);   //ctl1
  GPIO_WriteBit(GPIOB, GPIO_Pin_15, 1);   //ctl2
  GPIO_WriteBit(GPIOB, GPIO_Pin_14, 1);   //ctl3     
  GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1);   //boost enable            
  /* Init ADK Library */
  I2C_LowLevel_Init();

 
  USBH_ADK_Init("LuckyCharger", "powerbank", "description: xxx", "1.0", "q",  "987654123");
   Delay(10);
  while (1)
  {
    /* Read Analog Voltage */
    readbatvoltage();       
    /* Push button Monitor */
    if(STM_EVAL_PBGetState(BUTTON_USER) == 0){// && chargershut == 1 ){
      NVIC_SystemReset();  
    }
    /* Host Task handler */
    USBH_Process(&USB_OTG_Core_dev , &USB_Host);
    /* Accessory Mode enabled */
    if( USBH_ADK_getStatus() == ADK_IDLE) {
      firsttime = 1;  
      len = USBH_ADK_read(&USB_OTG_Core_dev, msg, sizeof(msg));
    	if( len > 0 ){
        charge=1;
    	}else{
        idle_mscount++;
        if(idle_mscount > 30000   &&  charge==0){
          GPIO_WriteBit(GPIOA, GPIO_Pin_8, 0);    //ctl1
          GPIO_WriteBit(GPIOB, GPIO_Pin_15, 0);   //ctl2
          GPIO_WriteBit(GPIOB, GPIO_Pin_14, 0);   //ctl3         
          GPIO_WriteBit(GPIOB, GPIO_Pin_0, 0);    //boost enable   
          chargershut = 1;
        }
      }
      USBH_ADK_write(&USB_OTG_Core_dev, msg, sizeof(msg));
    }
    Delay(1);
    if (i++ == 100){
      GPIO_ToggleBits(GPIOC, GPIO_Pin_13);
      i = 0;
    }
  }
}

#ifdef USE_FULL_ASSERT
/**
* @brief  assert_failed
*         Reports the name of the source file and the source line number
*         where the assert_param error has occurred.
* @param  File: pointer to the source file name
* @param  Line: assert_param error line source number
* @retval None
*/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line
  number,ex: printf("Wrong parameters value: file %s on line %d\r\n",
  file, line) */

  /* Infinite loop */
  while (1)
  {}
}

#endif

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 1 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  {
    TimingDelay--;
  }
}


void chargercontrol_pins(void){
  
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA , ENABLE);  

  GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);  

  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB , ENABLE);  
  
  /* Configure SOF VBUS ID DM DP Pins */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_14 |  GPIO_Pin_15 |  GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOB, &GPIO_InitStructure);    
  
  GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOB, &GPIO_InitStructure);  


  
  
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC , ENABLE);  
  /* Configure SOF VBUS ID DM DP Pins */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOC, &GPIO_InitStructure);    
}

void hextobinary(char* hex, char* bin){
	int cou;
			cou=0;
		while(hex[cou]){
         switch(hex[cou]){
             case '0': strcat(bin,"0000"); break;
             case '1': strcat(bin,"0001"); break;
             case '2': strcat(bin,"0010"); break;
             case '3': strcat(bin,"0011"); break;
             case '4': strcat(bin,"0100"); break;
             case '5': strcat(bin,"0101"); break;
             case '6': strcat(bin,"0110"); break;
             case '7': strcat(bin,"0111"); break;
             case '8': strcat(bin,"1000"); break;
             case '9': strcat(bin,"1001"); break;
             case 'A': strcat(bin,"1010"); break;
             case 'B': strcat(bin,"1011"); break;
             case 'C': strcat(bin,"1100"); break;
             case 'D': strcat(bin,"1101"); break;
             case 'E': strcat(bin,"1110"); break;
             case 'F': strcat(bin,"1111"); break;
             case 'a': strcat(bin,"1010"); break;
             case 'b': strcat(bin,"1011"); break;
             case 'c': strcat(bin,"1100"); break;
             case 'd': strcat(bin,"1101"); break;
             case 'e': strcat(bin,"1110"); break;
             case 'f': strcat(bin,"1111"); break;
             default: ; 
         }
         cou++;
    }
}

void readbatvoltage(void){
  int max = 7;
  int i =1;
  int bitvalue = 0;
  voltage = 2.3;
  readdata = 0;
  readdata = I2C_RdData(0x6a, 0x02, i2creaddata, 1);
  //readdata = I2C_RdData(0x6a, 0x0e, i2creaddata, 1);
  //I2C_WrData(0x6b, 0x02, 0xf1);
  Write_24Cxx(0x02, 0xF1);
  
  readdata = I2C_RdData(0x6a, 0x02, i2creaddata, 1);
  readdata = I2C_RdData(0x6a, 0x0e, i2creaddata, 1);

  while(max){
    binaryreaddata[max] = (readdata & i) ? 1 : 0;
    i = i << 1;
  max--;}
    
  if(binaryreaddata[7] == 1)
    voltage += .020;
  if(binaryreaddata[6] == 1)
    voltage += .040;
  if(binaryreaddata[5] == 1)
    voltage += .080;
  if(binaryreaddata[4] == 1)
    voltage += .160;
  if(binaryreaddata[3] == 1)
    voltage += .320;
  if(binaryreaddata[2] == 1)
    voltage += .640;
  if(binaryreaddata[1] == 1)
    voltage += 1.280;  
//  if(binaryreaddata[7] == 1)
//    voltage += .465;
//  if(binaryreaddata[6] == 1)
//    voltage += .93;
//  if(binaryreaddata[5] == 1)
//    voltage += 1.86;
//  if(binaryreaddata[4] == 1)
//    voltage += 3.72;
//  if(binaryreaddata[3] == 1)
//    voltage += 7.44;
//  if(binaryreaddata[2] == 1)
//    voltage += 14.88;
//  if(binaryreaddata[1] == 1)
//    voltage += 29.76;

  binaryreaddata[0]= 1;
  
  
  //sprintf(binaryreaddata, "%b", readdata);
}





/* Reference
    	//USBH_ADK_write(&USB_OTG_Core_dev, msg, sizeof(msg));

    		if( msg[0] == 0x1){
            GPIO_WriteBit(GPIOC, GPIO_Pin_14, 0);
//          GPIO_WriteBit(GPIOA, GPIO_Pin_8, 1);   //ctl1
//          GPIO_WriteBit(GPIOB, GPIO_Pin_15, 1);   //ctl2
//          GPIO_WriteBit(GPIOB, GPIO_Pin_14, 1);   //ctl3      

    			if( msg[1] == 0x1) {
    				GPIO_WriteBit(GPIOC, GPIO_Pin_14, 1);
            GPIO_WriteBit(GPIOA, GPIO_Pin_8, 0);   //ctl1
            GPIO_WriteBit(GPIOB, GPIO_Pin_15, 0);   //ctl2
            GPIO_WriteBit(GPIOB, GPIO_Pin_14, 0);   //ctl3         
            GPIO_WriteBit(GPIOB, GPIO_Pin_0, 0);   //boost enable            
            chargershut = 1;
    			}else{
    				//STM_EVAL_LEDOff(LED5);
    			}
    		}

*/



  

