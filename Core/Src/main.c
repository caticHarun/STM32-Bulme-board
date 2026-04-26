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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SET GPIO_PIN_SET
#define RESET GPIO_PIN_RESET
#define HAL_MAX_TIMEOUT 1000
#define true 1
#define false 0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

struct button
{
	int state;
	uint32_t change_timestamp;
	GPIO_TypeDef *port;
	uint16_t pin;
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Helpers
int print(const char *fmt, ...)
{
	char buffer[256];

	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (len < 0)
	{
		return len;
	}

	if (len > sizeof(buffer))
	{
		len = sizeof(buffer);
	}

	HAL_UART_Transmit(&huart2, (uint8_t *)buffer, len, HAL_MAX_DELAY);
	return len;
}

int bigger(int a, int b)
{
	return a > b ? a : b;
}

int smaller(int a, int b)
{
	return a < b ? a : b;
}

// LED Edits
void toggle_left_LEDs(int even)
{
	HAL_GPIO_WritePin(LED_L1_GPIO_Port, LED_L1_Pin, even ? SET : RESET);
	HAL_GPIO_WritePin(LED_L2_GPIO_Port, LED_L2_Pin, !even ? SET : RESET);
	HAL_GPIO_WritePin(LED_L3_GPIO_Port, LED_L3_Pin, even ? SET : RESET);
	HAL_GPIO_WritePin(LED_L4_GPIO_Port, LED_L4_Pin, !even ? SET : RESET);
	HAL_GPIO_WritePin(LED_L5_GPIO_Port, LED_L5_Pin, even ? SET : RESET);
	HAL_GPIO_WritePin(LED_L6_GPIO_Port, LED_L6_Pin, !even ? SET : RESET);
	HAL_GPIO_WritePin(LED_L7_GPIO_Port, LED_L7_Pin, even ? SET : RESET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, !even ? SET : RESET);
}

void toggle_main_LED(int red, int green, int blue) // Percentages
{
	if (!red)
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
	else
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

	if (!green)
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
	else
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

	if (!blue)
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
	else
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

	int period = 999;
	int one = (period / 100);
	int r = period - one * red;
	int g = period - one * green;
	int b = period - one * blue;

	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, r);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, g);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, b);
}

void debug_left_LEDs(int on)
{
	HAL_GPIO_WritePin(LED_L1_GPIO_Port, LED_L1_Pin, on ? SET : RESET);
	HAL_GPIO_WritePin(LED_L2_GPIO_Port, LED_L2_Pin, on ? SET : RESET);
	HAL_GPIO_WritePin(LED_L3_GPIO_Port, LED_L3_Pin, on ? SET : RESET);
	HAL_GPIO_WritePin(LED_L4_GPIO_Port, LED_L4_Pin, on ? SET : RESET);
	HAL_GPIO_WritePin(LED_L5_GPIO_Port, LED_L5_Pin, on ? SET : RESET);
	HAL_GPIO_WritePin(LED_L6_GPIO_Port, LED_L6_Pin, on ? SET : RESET);
	HAL_GPIO_WritePin(LED_L7_GPIO_Port, LED_L7_Pin, on ? SET : RESET);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, on ? SET : RESET);
}

// Button helpers
struct button addButton(
	GPIO_TypeDef *port,
	uint16_t pin)
{
	struct button btn;

	btn.state = 0;
	btn.change_timestamp = HAL_GetTick();
	btn.port = port;
	btn.pin = pin;

	return btn;
}

/**
 * Easier since intellisense gets this
 *
 * @returns
 * 	0 -> no click |
 * 	1 -> down |
 *  2 -> up |
 *  3 -> double click |
 */
int button_click_check(struct button *btn)
{ // 0 -> no click, 1 -> down; 2 -> up; 3 -> double click
	int state = !!(HAL_GPIO_ReadPin(btn->port, btn->pin) == RESET);
	if (state == btn->state)
		return 0;

	uint32_t current = HAL_GetTick();
	uint32_t difference = current - btn->change_timestamp;

	btn->state = state;
	btn->change_timestamp = HAL_GetTick();

	if (state == 1)
	{
		if (difference < 150)
			return 3;
		else
			return 1;
	}
	else
		return 2;
}

void default_disco_lights(int disco_lights_hard[][3], int *disco_lights_length)
{
	*disco_lights_length = 7;
	int dpl = 20; // Disco light percentage

	int t[][3] = {
		{0, 0, dpl},
		{0, dpl, 0},
		{0, dpl, dpl},
		{dpl, 0, 0},
		{dpl, 0, dpl},
		{dpl, dpl, 0},
		{dpl, dpl, dpl},
	};

	for (int i = 0; i < *disco_lights_length; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			disco_lights_hard[i][j] = t[i][j];
		}
	}
}

void transition_disco_lights(int disco_lights_hard[][3], int *disco_lights_length, int speed)
{
	int index = 0;

	int max_w = 100 / speed;

	// Red -> Green
	for (int i = 0; i < max_w; i++)
	{
		disco_lights_hard[index][0] = max_w - i; // R
		disco_lights_hard[index][1] = i;		 // G
		disco_lights_hard[index][2] = 0;		 // B
		index++;
	}

	// Green -> Blue
	for (int i = 0; i < max_w; i++)
	{
		disco_lights_hard[index][0] = 0;		 // R
		disco_lights_hard[index][1] = max_w - i; // G
		disco_lights_hard[index][2] = i;		 // B
		index++;
	}

	// Blue -> Red
	for (int i = 0; i < max_w; i++)
	{
		disco_lights_hard[index][0] = i;		 // R
		disco_lights_hard[index][1] = 0;		 // G
		disco_lights_hard[index][2] = max_w - i; // B
		index++;
	}

	*disco_lights_length = index;
}

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
	MX_USART2_UART_Init();
	MX_TIM1_Init();
	MX_ADC1_Init();
	/* USER CODE BEGIN 2 */

	// Setters
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	const int disco_timeout = 100;
	int disco_speed = 1;
	int disco_lights_hard[256 * 3][3];
	int disco_lights_length = 0;
	default_disco_lights(disco_lights_hard, &disco_lights_length);
	uint32_t timestamp = HAL_GetTick();

	// Buttons
	struct button a1 = addButton(
		A1_Button_GPIO_Port,
		A1_Button_Pin);
	struct button a2 = addButton(
		A2_Button_GPIO_Port,
		A2_Button_Pin);

	// Disco
	int disco_rgb_mode = 0; // Count
	int disco_even = false;
	int on = true;
	int smooth_speed = 1;
	const int max_smooth_speed = 100;
	int smooth = false;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		uint32_t current = HAL_GetTick();

		if ((current - timestamp) > (disco_timeout * disco_speed) && on)
		{
			timestamp = current;
			if (!smooth)
				toggle_left_LEDs(disco_even);
			else
				transition_disco_lights(disco_lights_hard, &disco_lights_length, smooth_speed);

			toggle_main_LED(
				disco_lights_hard[disco_rgb_mode][0],
				disco_lights_hard[disco_rgb_mode][1],
				disco_lights_hard[disco_rgb_mode][2]);
			disco_rgb_mode = (disco_rgb_mode + 1) % disco_lights_length;
			disco_even = !disco_even;
		}

		// ON / OFF
		int a1_clicked = button_click_check(&a1);
		if (a1_clicked == 1) // Off/On Disco mode
		{
			on = !on;
			debug_left_LEDs(0);
			toggle_main_LED(0, 0, 0);
			disco_speed = 1;
			if (on)
			{
				smooth = false;
				default_disco_lights(disco_lights_hard, &disco_lights_length);
			}
		}

		// Speed and Smooth color transition
		int a2_clicked = button_click_check(&a2);
		if (a2_clicked == 1)
		{
			default_disco_lights(disco_lights_hard, &disco_lights_length);
			smooth = false;
			on = true;
			disco_speed = (disco_speed % 4) + 1;
		}
		if (a2_clicked == 3)
		{
			debug_left_LEDs(0);
			smooth = true;
			disco_speed = 1;
		}

		// Speed for color transition
		if (smooth)
		{
			HAL_ADC_Start(&hadc1);
			if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_TIMEOUT) == HAL_OK)
			{
				uint32_t pot_value = HAL_ADC_GetValue(&hadc1);

				int percentage = (pot_value / 4095.0) * 100;
				smooth_speed = bigger(max_smooth_speed / 100 * percentage, 1);
			}
			HAL_ADC_Stop(&hadc1);
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
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure LSE Drive Capability
	 */
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 16;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Enable MSI Auto calibration
	 */
	HAL_RCCEx_EnableMSIPLLMode();
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void)
{

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = {0};

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */

	/** Common config
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc1.Init.OversamplingMode = DISABLE;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure Regular Channel
	 */
	sConfig.Channel = ADC_CHANNEL_9;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */
}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void)
{

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 0;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 999;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
	{
		Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.BreakFilter = 0;
	sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
	sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
	sBreakDeadTimeConfig.Break2Filter = 0;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */
	HAL_TIM_MspPostInit(&htim1);
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */
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
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LED_L7_Pin | LED_L4_Pin | LED_L1_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LED_L2_Pin | LED_L3_Pin | LD3_Pin | LED_L6_Pin | LED_L5_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : A1_Button_Pin A2_Button_Pin */
	GPIO_InitStruct.Pin = A1_Button_Pin | A2_Button_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : LED_L7_Pin LED_L4_Pin LED_L1_Pin */
	GPIO_InitStruct.Pin = LED_L7_Pin | LED_L4_Pin | LED_L1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : LED_L2_Pin LED_L3_Pin LD3_Pin LED_L6_Pin
							 LED_L5_Pin */
	GPIO_InitStruct.Pin = LED_L2_Pin | LED_L3_Pin | LD3_Pin | LED_L6_Pin | LED_L5_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
