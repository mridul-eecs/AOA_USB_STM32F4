/**
  ******************************************************************************
  * file    main.c
  * author  Kapil Sharma
  * version V1.0.0
  * date    2019/06/02
  * brief   Main function file
  ******************************************************************************
  */

/* Includes */

#define MEM_DEVICE_WRITE_ADDR 0x6a
#define MEM_DEVICE_READ_ADDR 0x6B
#define I2C_TIMEOUT_MAX 100000000

#include "stm32f4xx.h"
#include <stm32f4xx_tim.h>
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
#include "string.h"


/* Private macro */

char mftr[64]= "Lucky_Charge";
char mdl[64]= "PowerBank";
char desc[64]= "This is a custom message";
char ver[64]= "1.0";
char uri[]= "https://play.google.com/store/apps/details?id=com.jft.vts&hl=en_IN";
char sr[64]= "987654123";

/*
#define manufacture "Lucky_Charge"  
#define model "powerbank "    
#define description "This is a custom message"
#define version       "1.0"
#define uri "https://play.google.com/store/apps/details?id=com.jft.vts&hl=en_IN"
#define serial "987654123"*/



/* Private variables */
__IO uint32_t TimingDelay;
int firsttime = 0;
int i = 1;
int chargershut = 0;
uint8_t * i2creaddata;
uint32_t readdata;
char binaryreaddata[9] = "0";
char* hextostring;
float voltage = 0;
int charging = 0;
int idle_mincount = 0;
int idle_seccount = 0;
int push_button_tmr = 0;
int powerstate = 0;
int charge = 0;
int statemachine = 0;
uint16_t len;

/* Private function prototypes */
void InitializeTimer(void);
void EnableTimerInterrupt(void);
void chargercontrol_pins(void);
void Delay(__IO uint32_t nTime);
void TimingDelay_Decrement(void);
void readbatvoltage(void);
void poweron_adkinit(void);
void poweron(void);
void poweroff(void);
void ledshow(int led1, int led2, int led3, int led4);
extern uint8_t Write_24Cxx(uint16_t Addr, uint8_t Data);
extern uint8_t next_sno[64], prev_sno[64], sno_matched;
extern int timercount;
extern void USBH_ADK_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev, void *phost);
extern uint8_t sno_matched;

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
  uint8_t msg[2];
  RCC_ClocksTypeDef RCC_Clocks;

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
       
  /* Init ADK Library */
  I2C_LowLevel_Init();
	InitializeTimer();
	EnableTimerInterrupt();
	//poweron_adkinit();
  Delay(10);
  while (1){	   	
   /* Host Task handler */		
		/* Push button Monitor */
			if(STM_EVAL_PBGetState(BUTTON_USER) == 0){// && chargershut == 1 ){
				while(STM_EVAL_PBGetState(BUTTON_USER) == 0);
				if(1){//(push_button_tmr >= 2){
					while(STM_EVAL_PBGetState(BUTTON_USER) == 0);
					//toggle power
					if(powerstate == 0)
						powerstate=1;
					else powerstate=0;
					
					if(powerstate == 1){
						poweron_adkinit();
						Delay(100);}
					push_button_tmr = 0;
				}
			}
			else{
				push_button_tmr = 0;
			}
			//powerstate = 1;
			if(powerstate == 1){
				
			/* Read Analog Voltage */
				//readbatvoltage(); 

				/* Host Task handler */
				USBH_Process(&USB_OTG_Core_dev , &USB_Host);
				statemachine = USBH_ADK_getStatus();
							
				/* Accessory Mode enabled */
				if(statemachine == ADK_IDLE ){

					if(firsttime == 0){
						timercount=0;
						idle_mincount = 0;
					}
					firsttime = 1;
					memset(msg,0,strlen(msg));
					len = 0;					
					len = USBH_ADK_read(&USB_OTG_Core_dev, msg, sizeof(msg));
					if( len > 0 ){
						
						charge=1;   //charge =1 means application is installed						
					}
					//GPIO_ToggleBits(GPIOC, GPIO_Pin_14);			
					if(charge == 1){
						memcpy (prev_sno, next_sno, sizeof(next_sno));
					}

					//Delay(1000);
				}
				msg[0]=1;msg[1]=0;
				USBH_ADK_write(&USB_OTG_Core_dev, msg, sizeof(msg));			
			}else	poweroff();
						Delay(1);
						readbatvoltage();
						if (i++ >= 200){
							if(charging == 1){
							//led on for 500 ms
								ledshow(0,0,0,0);
								if(i >= 400)i = 0;
										}else if(charging == 0 && powerstate == 0){
									ledshow(1,1,1,1);   
									}		
						}	
				
					}
	}

	
void poweron_adkinit(){
	if(1){//chargershut != 1){
		USBH_ADK_InterfaceDeInit (&USB_OTG_Core_dev, &USB_Host);
		
		GPIO_WriteBit(GPIOA, GPIO_Pin_8, 1);   //ctl1
		GPIO_WriteBit(GPIOB, GPIO_Pin_15, 1);   //ctl2
		GPIO_WriteBit(GPIOB, GPIO_Pin_14, 1);   //ctl3     
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1);   //boost enable 
		USBH_DeInit(&USB_OTG_Core_dev, &USB_Host);
		Delay(100);	

		USBH_Init(&USB_OTG_Core_dev,USB_OTG_FS_CORE_ID,&USB_Host,&USBH_ADK_cb,&USR_Callbacks);
		USBH_ADK_InterfaceDeInit (&USB_OTG_Core_dev, &USB_Host);
		
		Delay(100);
		USBH_ADK_Init(mftr, mdl, desc, ver,uri,sr);	
		Delay(100);
	}
	chargershut = 0;
	timercount=0;
	idle_mincount = 0;
	charge = 0;
	firsttime = 0;	
}
void poweron(){
		GPIO_WriteBit(GPIOA, GPIO_Pin_8, 1);   //ctl1
		GPIO_WriteBit(GPIOB, GPIO_Pin_15, 1);   //ctl2
		GPIO_WriteBit(GPIOB, GPIO_Pin_14, 1);   //ctl3     
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1);   //boost enable 
		powerstate = 1;
}
void poweroff(){
		GPIO_WriteBit(GPIOA, GPIO_Pin_8, 0);   //ctl1
		GPIO_WriteBit(GPIOB, GPIO_Pin_15, 0);   //ctl2
		GPIO_WriteBit(GPIOB, GPIO_Pin_14, 0);   //ctl3     
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, 0);   //boost enable 
		USBH_ADK_InterfaceDeInit (&USB_OTG_Core_dev, &USB_Host);
		USBH_DeInit(&USB_OTG_Core_dev, &USB_Host);

		powerstate = 0;
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

//Initilize Timer2 
void InitializeTimer()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
 
    TIM_TimeBaseInitTypeDef timerInitStructure; 
    timerInitStructure.TIM_Prescaler = 45000;
    timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    timerInitStructure.TIM_Period = 2000;
    timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    timerInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &timerInitStructure);
    TIM_Cmd(TIM3, ENABLE);
		TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}
 
//configure interrupt for Timer2
void EnableTimerInterrupt()
{
    NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = TIM3_IRQn;
    nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
    nvicStructure.NVIC_IRQChannelSubPriority = 1;
    nvicStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicStructure);
}




void ledshow(int led1, int led2, int led3, int led4){
  GPIO_WriteBit(GPIOC, GPIO_Pin_14, led1);
  GPIO_WriteBit(GPIOC, GPIO_Pin_13, led2);
  GPIO_WriteBit(GPIOB, GPIO_Pin_8, led3);
  GPIO_WriteBit(GPIOB, GPIO_Pin_5, led4);

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
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_14 |  GPIO_Pin_15 |  GPIO_Pin_0 |  GPIO_Pin_5 |  GPIO_Pin_8;
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
  int ibin =1;
  int bitvalue = 0;
  voltage = 2.3;
  readdata = 0;
  readdata = I2C_RdData(0x6a, 0x02, i2creaddata, 1);
  //readdata = I2C_RdData(0x6a, 0x0e, i2creaddata, 1);
  //I2C_WrData(0x6b, 0x02, 0xf1);
  

  Write_24Cxx(0x02, 0xF1);
  
  readdata = I2C_RdData(0x6a, 0x02, i2creaddata, 1);
  readdata = I2C_RdData(0x6a, 0x0e, i2creaddata, 1);
  memset(binaryreaddata, 0, 9);
  while(max){
    binaryreaddata[max] = (readdata & ibin) ? 1 : 0;
    ibin = ibin << 1;
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

  
  if(voltage >= 2.9 && voltage < 3.225){
    if(charging==1 || powerstate==1)ledshow(1,1,1,0);
  }
  if(voltage >= 3.225 && voltage < 3.55){
    if(charging==1 || powerstate==1)ledshow(1,1,0,0);
  }
  if(voltage >= 3.55 && voltage < 3.875){
    if(charging==1 || powerstate==1)ledshow(1,0,0,0);
  }
  if(voltage >= 3.875 && voltage < 4.2){
    if(charging==1 || powerstate==1)ledshow(0,0,0,0);
  }
  
  readdata = I2C_RdData(0x6a, 0x0B, i2creaddata, 1);
  memset(binaryreaddata, 0, 9);
  max = 7;
  ibin =1;
  while(max){
    binaryreaddata[max] = (readdata & ibin) ? 1 : 0;
    ibin = ibin << 1;
  max--;}
  if(binaryreaddata[3] == 0 && binaryreaddata[4] == 0){
    charging = 0;
  }else{
    charging = 1;
    ledshow(1,1,1,1);
    charging = 1;

  }    
  //sprintf(binaryreaddata, "%b", readdata);
}







/***************************End of File**************************/

