/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "LTC2983.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef handle_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

LTC2983IfaceConfig_t ltc1IFaceConfig = {
		.hspi = &hspi2,
		.GpioChipSelect = {.Pin = LTC1_CS_Pin, .Port = LTC1_CS_GPIO_Port},
		.GpioReset = {.Pin = LTC1_RST_Pin, .Port = LTC1_RST_GPIO_Port},
		.GpioInterrupt = {.Pin = LTC1_EXTI_Pin, .Port = LTC1_EXTI_GPIO_Port}
};

const float rsenseResistorValue = 5050.0; // ohms
LTC2983ChannelConfig_t ltc1ChannelArray[5] = {
		{ // Rsense resistor
			.Channel = 	2,
			.Data = LTC2983_SENSOR_TYPE__SENSE_RESISTOR | (uint32_t)(rsenseResistorValue * 1024) // resolution per bit
		},
		{ // rtd1, sensor type and excitation current is an unknown
			.Channel = 4,
			.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
			| LTC2983_RTD_EXCITATION_CURRENT__100UA | LTC2983_RTD_STANDARD__EUROPEAN
		},
		{
				.Channel = 6,
				.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
				| LTC2983_RTD_EXCITATION_CURRENT__100UA | LTC2983_RTD_STANDARD__EUROPEAN
		},
		{
				.Channel = 8,
				.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
				| LTC2983_RTD_EXCITATION_CURRENT__100UA | LTC2983_RTD_STANDARD__EUROPEAN
		},
		{
				.Channel = 10,
				.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
				| LTC2983_RTD_EXCITATION_CURRENT__100UA | LTC2983_RTD_STANDARD__EUROPEAN
		}
};

LTC2983ChannelConfigs_t ltc1ChannelConfigs = {
		.Configs = ltc1ChannelArray,
		.Count = sizeof(ltc1ChannelArray) / sizeof(LTC2983ChannelConfig_t)
};

LTC2983ConvResult_t ltc1ConvResultArray[4] = {
		{
				.Channel = 4, .Status = 0, .Temperature = 0
		},
		{
				.Channel = 6, .Status = 0, .Temperature = 0
		},
		{
				.Channel = 8, .Status = 0, .Temperature = 0
		},
		{
				.Channel = 10, .Status = 0, .Temperature = 0
		}
};

LTC2983ConvResults_t ltc1ConvResults = {
		.Results = ltc1ConvResultArray,
		.Count = sizeof(ltc1ConvResultArray) / sizeof(LTC2983ConvResult_t)
};

LTC2983RuntimeState_t ltc1State = {0};

LTC2983Handle_t ltc1Handle = {
		.IfaceConfig = &ltc1IFaceConfig,
		.ChannelConfigs = &ltc1ChannelConfigs,
		.Results = &ltc1ConvResults,
		.State = &ltc1State,
		.BitMask = 1 << 3 | 1 << 5 | 1 << 7 | 1 << 9, // channel 4, 6, 8, 10
		.GlobalConfigurationRegister = LTC2983_REJECTION__50_60_HZ | LTC2983_TEMP_UNIT__C,
		.MuxConfigDelay = 10 // for 1ms delay, dunno why i just decided so
};

LTC2983Handle_t* handlesArray[1] = { &ltc1Handle };

LTC2983HandleRegistry_t handleRegistry = {
		.Handles = handlesArray,
		.Count = 1
};



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

	LTC2983_Init(&ltc1Handle);

	LTC2983_RegisterLTC2983HandleRegistry(&handleRegistry);

	int step = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  LTC2983RuntimeState_t * ltc1State = ltc1Handle.State;

	  if (ltc1State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		  LTC2983_StartUp(&ltc1Handle);
		  continue;
	  }

	  if (ltc1State->Status == LTC2983_DRIVER_STATUS_BUSY) {
		  continue;
	  }

	  switch (step){
	  	  case 0:{
	  		  LTC2983_WriteGlobalConfigReg(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 1:{
	  		  LTC2983_ReadGlobalConfigReg(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 2:{
	  		  LTC2983_WriteMuxConfigDelay(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 3:{
	  		  LTC2983_ReadMuxConfigDelay(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 4:{
	  		  LTC2983_WriteMeasMultiChannelsMask(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 5:{
	  		  LTC2983_ReadMeasMultiChannelsMask(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 6:{
	  		  LTC2983_WriteChannelsAssignmentData(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 7:{
	  		  LTC2983_ReadChannelsAssignmentData(&ltc1Handle);

	  		  step++;
	  		  break;
	  	  }
	  	  case 8:{
	  		  LTC2983_Convert(&ltc1Handle, 4);

	  		  step++;
	  		  break;
	  	  }
	  	  case 9:{
	  		  LTC2983_ReadTemperatureResults(&ltc1Handle, 4);

	  		  step++;
	  		  break;
	  	  }
	  	  case 10:{
	  		  LTC2983_Convert(&ltc1Handle, 0);

	  		  step++;
	  		  break;
	  	  }
	  	  case 11:{
	  		  LTC2983_ReadTemperatureResults(&ltc1Handle, 0);

	  		  step++;
	  		  break;
	  	  }
	  	  case 12:{
	  		  LTC2983_ReadTemperatureResults(&ltc1Handle, 0);

	  		  step++;
	  		  break;
	  	  }
	  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x7;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi2, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LTC1_CS_GPIO_Port, LTC1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LTC1_RST_GPIO_Port, LTC1_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LTC1_CS_Pin */
  GPIO_InitStruct.Pin = LTC1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LTC1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LTC1_EXTI_Pin */
  GPIO_InitStruct.Pin = LTC1_EXTI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LTC1_EXTI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LTC1_RST_Pin */
  GPIO_InitStruct.Pin = LTC1_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LTC1_RST_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void ltcResultsCallback(LTC2983Handle_t *handle) {
    if (handle->State->Status == LTC2983_DRIVER_STATUS_COMPLETE) {
		char msg[64];
		float t = ltc1Handle.Results->Results[0].Temperature;

		// Arduino-style výpis
		int len = snprintf(msg, sizeof(msg), "Teplota LTC: %.2f C\r\n", t);
		HAL_UART_Transmit(&huart1, (uint8_t*)msg, len, 100);
    }
}

void ltcDebugCallback(LTC2983Handle_t *handle) {
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
