#include "stm32f4_discovery.h"
#include "stm32f4xx.h"
#include <stdbool.h>

#define SYSTEM_CLOCK_FREQUENCY_MHZ 168

volatile uint32_t ticks;
volatile bool led3status;

void Delay(__IO uint32_t nCount) {
  while(nCount--) {
  }
}

void Delayms(__IO uint32_t ms) {
  Delay(ms * 1000 * SYSTEM_CLOCK_FREQUENCY_MHZ);
}

static void Error_Handler(void)
{
  BSP_LED_On(LED5);

  while (1) {};
}

void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* CAN1 TX GPIO pin configuration */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Alternate =  GPIO_AF9_CAN1;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* CAN1 RX GPIO pin configuration */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Alternate =  GPIO_AF9_CAN1;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void CAN_Configuration(CAN_HandleTypeDef *CAN_Handle) {
  static CAN_FilterConfTypeDef CAN_FilterConfStructure;
  static CanTxMsgTypeDef TxMessage;
  static CanRxMsgTypeDef RxMessage;

  /* CAN1 configuration */
  CAN_Handle->Instance = CAN1;
  CAN_Handle->pTxMsg = &TxMessage;
  CAN_Handle->pRxMsg = &RxMessage;

  CAN_Handle->Init.TTCM = DISABLE; // time-triggered communication mode = DISABLED
  CAN_Handle->Init.ABOM = DISABLE; // automatic bus-off management mode = DISABLED
  CAN_Handle->Init.AWUM = DISABLE; // automatic wake-up mode = DISABLED
  CAN_Handle->Init.NART = DISABLE; // non-automati1 retransmission mode = DISABLED
  CAN_Handle->Init.RFLM = DISABLE; // receive FIFO locked mode = DISABLED
  CAN_Handle->Init.TXFP = DISABLE; // transmit FIFO priority = DISABLED
  CAN_Handle->Init.Mode = CAN_MODE_LOOPBACK; // loopback CAN mode
  CAN_Handle->Init.SJW = CAN_SJW_1TQ; // synchronization jump width = 1
  CAN_Handle->Init.BS1 = CAN_BS1_6TQ; //6
  CAN_Handle->Init.BS2 = CAN_BS2_8TQ; //8
  CAN_Handle->Init.Prescaler = 2; // baudrate 500 kbps

  if(HAL_CAN_Init(CAN_Handle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  else
  {
    /* Turn the blue LED on to signal init success */
    BSP_LED_On(LED6);  
  }

  /* Default filter - accept all to CAN_FIFO */
  CAN_FilterConfStructure.BankNumber = 14;
  CAN_FilterConfStructure.FilterNumber = 0;
  CAN_FilterConfStructure.FilterFIFOAssignment = CAN_FIFO0;
  CAN_FilterConfStructure.FilterMode = CAN_FILTERMODE_IDMASK;
  CAN_FilterConfStructure.FilterScale = CAN_FILTERSCALE_32BIT;
  CAN_FilterConfStructure.FilterIdHigh = 0x0000;
  CAN_FilterConfStructure.FilterIdLow = 0x0000;
  CAN_FilterConfStructure.FilterMaskIdHigh = 0x0000;
  CAN_FilterConfStructure.FilterMaskIdLow = 0x0000;
  CAN_FilterConfStructure.FilterActivation = ENABLE;

  if(HAL_CAN_ConfigFilter(CAN_Handle, &CAN_FilterConfStructure) != HAL_OK)
  {
    /* Filter configuration Error */
    Error_Handler();
  }
}

HAL_StatusTypeDef CAN_TxMessage(CAN_HandleTypeDef *CAN_Handle) {
  /* CAN message to send */
  CAN_Handle->pTxMsg->StdId = 0x42;
  CAN_Handle->pTxMsg->RTR = CAN_RTR_DATA;
  CAN_Handle->pTxMsg->IDE = CAN_ID_STD;
  CAN_Handle->pTxMsg->DLC = 4;
  CAN_Handle->pTxMsg->Data[0] = 0xDE;
  CAN_Handle->pTxMsg->Data[1] = 0xAD;
  CAN_Handle->pTxMsg->Data[2] = 0xCA;
  CAN_Handle->pTxMsg->Data[3] = 0xFE;

  if(HAL_CAN_Transmit(CAN_Handle, 10) != HAL_OK)
  {
    /* Transmission Error */
    Error_Handler();
  }

  if(HAL_CAN_GetState(CAN_Handle) != HAL_CAN_STATE_READY)
  {
    return HAL_ERROR;
  }

  /* Blink the green LED if the transmission was successful */
  BSP_LED_On(LED3);
  Delayms(100);
  BSP_LED_Off(LED3);

  return HAL_OK;
}

HAL_StatusTypeDef CAN_RxMessage(CAN_HandleTypeDef *CAN_Handle)
{
  if (HAL_CAN_Receive(CAN_Handle, CAN_FIFO0, 10) != HAL_OK)
  {
    /* Reception Error */
    Error_Handler();
  }

  if (HAL_CAN_GetState(CAN_Handle) != HAL_CAN_STATE_READY)
  {
    return HAL_ERROR;
  }

  if (CAN_Handle->pRxMsg->StdId != 0x42)
  {
    return HAL_ERROR;  
  }

  if (CAN_Handle->pRxMsg->IDE != CAN_ID_STD)
  {
    return HAL_ERROR;
  }

  if (CAN_Handle->pRxMsg->DLC != 4)
  {
    return HAL_ERROR;  
  }

  if (*(uint32_t*)CAN_Handle->pRxMsg->Data != 0xFECAADDE)
  {
    BSP_LED_On(LED5);
    Delayms(100);
    BSP_LED_Off(LED5);
    return HAL_ERROR;
  }

  BSP_LED_On(LED4);
  Delayms(100);
  BSP_LED_Off(LED4);

  return HAL_OK; /* Test Passed */
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* Enable GPIO A clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Enable CAN1 clock */
  __HAL_RCC_CAN1_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

void SysTick_Handler(void)
{
  if (ticks % 10000 == 0) {
    led3status = !led3status;
  }

  HAL_IncTick();
  ticks++;
}

int main(void)
{
  static CAN_HandleTypeDef CAN_Handle;
  static WWDG_HandleTypeDef WWDG_Handle;

  HAL_Init();

  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);
  BSP_LED_Init(LED5);
  BSP_LED_Init(LED6);

  SystemClock_Config();

  WWDG_Handle.Instance = (WWDG_TypeDef*)WWDG_BASE; 
  CLEAR_BIT(WWDG_Handle.Instance->CR, WWDG_CR_WDGA);

  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI); 

  /* Initialize GPIO */
  GPIO_Configuration();

  /* Initialize CAN */
  CAN_Configuration(&CAN_Handle);

  while(1) {
    /* Transfer CAN message */
    CAN_TxMessage(&CAN_Handle);
    /* Receive CAN message */
    CAN_RxMessage(&CAN_Handle);

    Delayms(800);
  }
}
