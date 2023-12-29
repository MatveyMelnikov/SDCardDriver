/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "sdcard_driver.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */
// HAL_StatusTypeDef receive_byte(uint8_t* data);
// HAL_StatusTypeDef receive_bytes(uint8_t* data, const uint8_t size);
// HAL_StatusTypeDef transmit_bytes(const uint8_t* data, const uint8_t size);
// void fast_boot(void);
// HAL_StatusTypeDef wait_sd_response(uint8_t* data);
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
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // sd_card_command cmd0 = sd_card_get_cmd(0, 0x0);
	// sd_card_command cmd8 = sd_card_get_cmd(8, (1 << 8) | 0x55);
  // uint8_t cmd0_response = 0;
  // sd_card_r7_response cmd8_response = { 0 };
  // uint8_t data[6] = { 0 };

  // fast_boot();

  // SELECT_SD();
	// HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi2, (uint8_t*)&cmd0, 6, HAL_MAX_DELAY);
	// // //status |= HAL_SPI_TransmitReceive(&hspi2, &cmd0_response, 1, HAL_MAX_DELAY);
  // // status |= receive_byte(&data);

  // //HAL_StatusTypeDef status = transmit_bytes((uint8_t*)&cmd0, 6);

  // while (data[0] == IN_IDLE_STATE)
  // {
  //   status |= receive_byte(&data);
  // }

  // DISELECT_SD();

  // SELECT_SD();
	// // status |= HAL_SPI_Transmit(&hspi2, (uint8_t*)&cmd8, 6, HAL_MAX_DELAY);
	// // //status |= receive_cmd_response(&hspi2, (uint8_t*)&cmd8_response, 5, HAL_MAX_DELAY);
  // // //status |= HAL_SPI_TransmitReceive(&hspi2, (uint8_t*)&cmd8_response, 5, HAL_MAX_DELAY);
  // // status |= receive_byte(&data);
  // // status |= receive_byte(&data + 1);
  // // status |= receive_byte(&data + 2);
  // // status |= receive_byte(&data + 3);
  // // status |= receive_byte(&data + 4);
  // // status |= receive_byte(&data + 5);

  // //status |= transmit_bytes((uint8_t*)&cmd8, 6);


  // // status |= HAL_SPI_Transmit(&hspi2, (uint8_t*)&cmd8, 6, HAL_MAX_DELAY);
  // // status |= receive_bytes(&data, 5); // r7

  // status |= HAL_SPI_Transmit(&hspi2, (uint8_t*)&cmd8, 6, HAL_MAX_DELAY);
  // status |= wait_sd_response(&data);
  // status |= receive_bytes(&data, 5); // r7

  // DISELECT_SD();
  

/*
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(&hspi2, (uint8_t*)&cmd0, buffer, 6, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
  */

  sd_card_reset(&hspi2);
  while (1)
  {
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// HAL_StatusTypeDef receive_byte(uint8_t* data)
// {
//   uint8_t dummy_data = 0xff;

//   return HAL_SPI_TransmitReceive(&hspi2, &dummy_data, data, 1, 1000);
// }

// HAL_StatusTypeDef receive_bytes(uint8_t* data, const uint8_t size)
// {
//   HAL_StatusTypeDef status = 0;

//   for (uint8_t i = 0; i < size; i++)
//     status += receive_byte(data + i);
  
//   return status;
// }

// HAL_StatusTypeDef transmit_bytes(const uint8_t* data, const uint8_t size)
// {
//   HAL_StatusTypeDef status = 0;

//   for (int16_t i = size - 1; i >= 0; i--)
//     status += HAL_SPI_Transmit(&hspi2, data + i, 1, 1000);
  
//   return status;
// }

// void fast_boot(void)
// {
//   uint8_t dummy_data = 0xff;

//   DISELECT_SD();

//   // Clock occurs only during transmission
//   for (uint8_t i = 0; i < 10; i++) // Need at least 74 ticks
//     HAL_SPI_Transmit(&hspi2, &dummy_data, 1, 1);
// }

// // We are trying to get a byte with the first zero bit.
// // The received byte is written to the argument
// HAL_StatusTypeDef wait_sd_response(uint8_t* data)
// {
//   HAL_StatusTypeDef status = 0;
//   uint32_t captured_tick = HAL_GetTick();

//   do
//   {
//     if ((HAL_GetTick() - captured_tick) > 500)
//       return HAL_TIMEOUT;

//     status |= receive_byte(&data);
//   } while (data == NULL); // ?

//   return status;
// }

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

#ifdef  USE_FULL_ASSERT
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
