/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f3xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "LiquidCrystal.h"
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
typedef enum
{
	false,
	true
} bool;
typedef struct
{
    uint16_t frequency;
    uint16_t duration;
} Tone;
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
int seven_segment_counter = 0;
int score = 0;
volatile uint32_t last_gpio_exit = 0;
unsigned char data[500] = "SALAM";
int volume_value = 0;
int8_t doodler_x = 0, doodler_y = 0;
int8_t last_doodler_x = 0, last_doodler_y = 0;
int8_t shot_x = -1, shot_y = -1;
bool game_started = false;
bool about_screen_showing = false;
bool show_menu = false;
bool finish_game = false;
bool first_run = true;
bool start_screen = true;
uint8_t jump_step = 0;
uint8_t map[20][4] = {0};
uint8_t replaced_stick = 0;
int scroll_times = 0;
uint8_t jump_number = 7;
char player_name[15] = "Doodler";
bool name_selected = true;
uint8_t uart_counter = 0;
uint16_t led_light = 0;
uint8_t probablity[10][6] = {
	// has_something, normal_stick, broken_stick, jump_stick, monster, black_hole
	{30, 100, 100, 100, 100, 100}, //{40, 100, 0, 0, 0, 0},
	{30, 90, 90, 100, 100, 100},   //{40, 90, 0, 10, 0, 0},
	{30, 80, 90, 100, 100, 100},   //{40, 80, 10, 10, 0, 0},
	{30, 80, 85, 100, 100, 100},   //{30, 80, 5, 15, 0, 0},
	{25, 70, 85, 100, 100, 100},   //{20, 70, 15, 15, 0, 0},
	{20, 70, 80, 90, 100, 100},	   //{20, 70, 10, 10, 10, 0},
	{30, 75, 80, 85, 95, 100},	   //{30, 75, 5, 5, 10, 5},
	{30, 70, 75, 80, 90, 100},	   //{30, 70, 5, 5, 10, 10},
	{30, 65, 70, 80, 90, 100},	   //{30, 65, 5, 10, 10, 10},
	{30, 55, 60, 70, 85, 100},	   //{30, 55, 5, 10, 15, 15},
};


extern TIM_HandleTypeDef htim1;
TIM_HandleTypeDef *pwm_timer = &htim1;	// Point to PWM Timer configured in CubeMX
uint32_t pwm_channel = TIM_CHANNEL_1;   // Select configured PWM channel number
const Tone *volatile melody_ptr;
volatile uint16_t melody_tone_count;
volatile uint16_t current_tone_number;
volatile uint32_t current_tone_end;
volatile uint16_t volume = 1;          // (0 - 1000)
volatile uint32_t last_button_press;
const Tone star_wars_melody[] = {
		{880, 642},
		{831, 481},
		{784, 160},
		{622, 160},
		{587, 160},
		{622, 321},
		{440, 321},
		{622, 642},
		{587, 481},
		{880, 642},
		{831, 481},
		{784, 160},
		{622, 160},
		{587, 160},
		{622, 321},
		{440, 321},
		{622, 642},
		{587, 481},
		{880, 642},
		{831, 481},
		{784, 160},
		{622, 160},
		{587, 160},
		{622, 321},
		{440, 321},
		{622, 642},
		{587, 481},
		{880, 642},
		{831, 481},
		{784, 160},
		{622, 160},
		{587, 160},
		{622, 321},
		{440, 321},
		{622, 642},
		{587, 481},
		{0, 321},
		{349, 321},
		{415, 642},
		{349, 481},
		{440, 240},
		{440, 642},
		{349, 481},
		{523, 160},
		{440, 1285},
	{   0,  0}	// <-- Disable PWM
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void numberToBCD(int i)
{
	int x1 = i & 1;
	int x2 = i & 2;
	int x3 = i & 4;
	int x4 = i & 8;
	if (x1 > 0)
		x1 = 1;
	if (x2 > 0)
		x2 = 1;
	if (x3 > 0)
		x3 = 1;
	if (x4 > 0)
		x4 = 1;

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, x4); // a
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, x3); // b
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, x2); // c
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, x1); // d
}
void PWM_Start()
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void PWM_Change_Tone(uint16_t pwm_freq, uint16_t volume) // pwm_freq (1 - 20000), volume (0 - 1000)
{
    if (pwm_freq == 0 || pwm_freq > 20000)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    }
    else
    {
        const uint32_t internal_clock_freq = HAL_RCC_GetSysClockFreq();
        const uint16_t prescaler = 1;
        const uint32_t timer_clock = internal_clock_freq / prescaler;
        const uint32_t period_cycles = timer_clock / pwm_freq;
        const uint32_t pulse_width = volume * period_cycles / 1000 / 2;

        pwm_timer->Instance->PSC = prescaler - 1;
        pwm_timer->Instance->ARR = period_cycles - 1;
        pwm_timer->Instance->EGR = TIM_EGR_UG;
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_width); // pwm_timer->Instance->CCR2 = pulse_width;
    }
}

void Change_Melody(const Tone *melody, uint16_t tone_count)
{
    melody_ptr = melody;
    melody_tone_count = tone_count;
    current_tone_number = 0;
}

void Update_Melody()
{
    if ((HAL_GetTick() > current_tone_end) && (current_tone_number < melody_tone_count) && finish_game)
    {
        const Tone active_tone = *(melody_ptr + current_tone_number);
        PWM_Change_Tone(active_tone.frequency, volume);
        current_tone_end = HAL_GetTick() + active_tone.duration;
        current_tone_number++;
    }
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */
extern RTC_HandleTypeDef hrtc;
extern RTC_TimeTypeDef mytime;
extern RTC_DateTypeDef mydate;

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
	while (1)
	{
	}
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
	if(finish_game) Update_Melody();

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F3xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f3xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line0 interrupt.
  */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */
		if (start_screen)
		{
			start_screen = false;
			show_menu = true;
			first_run = true;
		}
  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
  * @brief This function handles EXTI line3 interrupt.
  */
void EXTI3_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI3_IRQn 0 */

  /* USER CODE END EXTI3_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
  /* USER CODE BEGIN EXTI3_IRQn 1 */
	if ((HAL_GetTick() - last_gpio_exit) < 50)
	{
		return;
	}
	last_gpio_exit = HAL_GetTick();
	HAL_RTC_GetTime(&hrtc, &mytime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &mydate, RTC_FORMAT_BIN);
	char time_str[20];
	char date_str[20];
	sprintf(date_str, "20%02d-%02d-%02d", mydate.Year, mydate.Month, mydate.Date);
	sprintf(time_str, "%02d:%02d:%02d", mytime.Hours, mytime.Minutes, mytime.Seconds);
	for (int i = 0; i < 4; i++)
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 0);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, 0);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, 0);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, 0);

		if (i == 0)
		{
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 1);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3))
			{ // A button
				if (about_screen_showing)
				{ // az safhe about be menu
					about_screen_showing = false;
					show_menu = true;
					first_run = true;
				}
				else if (game_started)
				{
					doodler_x--;
					if (doodler_x < 0)
						doodler_x = 3;
					int n = sprintf(data, "%s %s-%s Left X=%d\n", date_str, time_str, player_name, doodler_x);
					HAL_UART_Transmit(&huart1, data, n, 1000);
				}
				if (start_screen)
				{
					start_screen = false;
					show_menu = true;
					first_run = true;
				}
			}
		}
		else if (i == 1)
		{
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, 1);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3))
			{ // B button
				if (show_menu)
				{ // az menu be about
					show_menu = false;
					first_run = true;
					about_screen_showing = true;
				}
				else if (finish_game)
				{
			        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

			        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
					__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
					__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
					led_light = 0;
					 PWM_Start();
					      Change_Melody(star_wars_melody, ARRAY_LENGTH(star_wars_melody));
					finish_game = false;
					show_menu = true;
					first_run = true;
				} else if(game_started && shot_x == -1 && shot_y == -1){
					shot_x = doodler_x;
					shot_y = doodler_y + 2;
					int n = sprintf(data, "%s %s-%s Hit X=%d\n", date_str, time_str, player_name, doodler_x);
					HAL_UART_Transmit(&huart1, data, n, 1000);
				}
			}
		}
		else if (i == 2)
		{
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, 1);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3))
			{ // C button
				if (show_menu)
				{ // az menu be bazi
					game_started = true;
					score = 0;
					finish_game = false;
					show_menu = false;
					first_run = true;
					srand(HAL_GetTick());
					for (int i = 0; i < 20; i++)
					{
						for (int j = 0; j < 4; j++)
						{
							uint8_t use_stick = rand() % 100;
							if (use_stick <= probablity[volume_value][0])
							{
								uint8_t type = rand() % 100;
								if (type <= probablity[volume_value][1])
									map[i][j] = 2;
								else if (type <= probablity[volume_value][2])
									map[i][j] = 3;
								else if (type <= probablity[volume_value][3])
									map[i][j] = 4;
								else if (type <= probablity[volume_value][4]){
									if(i<7) map[i][j] = 10;
									else map[i][j] = 5;
								}
								else if (type <= probablity[volume_value][5]){
									if(i<7) map[i][j] = 10;
									else map[i][j] = 6;
								}
							}
							else
							{
								map[i][j] = 10; // 10 means nothing in that place
							}
						}
					}
					map[0][0] = 10;
				}
			}
		}
		else if (i == 3)
		{
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, 1);
			if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3))
			{ // D button
				if (game_started)
				{
					doodler_x++;
					if (doodler_x > 3)
						doodler_x = 0;
					int n = sprintf(data, "%s %s-%s Right X=%d\n", date_str, time_str, player_name, doodler_x);
					HAL_UART_Transmit(&huart1, data, n, 1000);
				}
			}
		}
	}
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, 1);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, 1);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, 1);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, 1);
	while (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3))
		;

  /* USER CODE END EXTI3_IRQn 1 */
}

/**
  * @brief This function handles ADC1 and ADC2 interrupts.
  */
void ADC1_2_IRQHandler(void)
{
  /* USER CODE BEGIN ADC1_2_IRQn 0 */

  /* USER CODE END ADC1_2_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  /* USER CODE BEGIN ADC1_2_IRQn 1 */
	int x = HAL_ADC_GetValue(&hadc1);
	volume_value = x * 9 / 4095;
	HAL_ADC_Start_IT(&hadc1);
  /* USER CODE END ADC1_2_IRQn 1 */
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
	seven_segment_counter++;
	if (seven_segment_counter > 4)
		seven_segment_counter = 1;
	int temp = score;
	if (seven_segment_counter == 1)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, 0);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10, 1);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, 0);
		numberToBCD(temp % 10);
	}
	else if (seven_segment_counter == 2)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, 0);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11, 1);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, 0);
		numberToBCD((temp / 10) % 10);
	}
	else if (seven_segment_counter == 3)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_10 | GPIO_PIN_11, 1);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, 0);
		numberToBCD((temp / 100) % 10);
	}
	else if (seven_segment_counter == 4)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 0);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11, 1);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, 1);
		numberToBCD(volume_value % 10);
	}
  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */

  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */
	if (game_started)
	{
		if (last_doodler_x != doodler_x || last_doodler_y != doodler_y)
		{
			setCursor(last_doodler_y, last_doodler_x);
			if (replaced_stick == 0 || (replaced_stick == 3 && jump_step >= jump_number))
				print(" ");
			else
				write(replaced_stick);
		}
		setCursor(doodler_y, doodler_x);
		if (map[doodler_y][doodler_x] != 10)
		{
			replaced_stick = map[doodler_y][doodler_x];
			last_doodler_x = doodler_x;
			last_doodler_y = doodler_y;
			if (replaced_stick == 5 || replaced_stick == 6)
			{
				__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
				game_started = false;
				show_menu = false;
				finish_game = true;
				first_run = true;
				 PWM_Start();
				      Change_Melody(star_wars_melody, ARRAY_LENGTH(star_wars_melody));
				doodler_x = 0;
				doodler_y = 0;
				last_doodler_x = 0;
				last_doodler_y = 0;
				replaced_stick = 0;
				shot_x = -1;
				shot_y = -1;
				jump_step = 0;
				jump_number = 7;
				write(0);
				return;
			}
			if (replaced_stick == 2 || replaced_stick == 4)
			{
				if (jump_step >= jump_number)
				{
					jump_step = 0;
					if (replaced_stick == 2)
						jump_number = 7;
					else if (replaced_stick == 4)
						jump_number = 20;
				}
			}
			if (replaced_stick == 3)
			{
				if (jump_step >= jump_number)
					map[doodler_y][doodler_x] = 10;
			}
			write(1);
		}
		else
		{
			last_doodler_x = doodler_x;
			last_doodler_y = doodler_y;
			replaced_stick = 0;
			write(0);
		}
		if(shot_x != -1 && shot_y != -1){
			setCursor(shot_y-1, shot_x);
			if(map[shot_y-1][shot_x]!=10) write(map[shot_y-1][shot_x]);
			else print(" ");
			setCursor(shot_y, shot_x);
			print("*");
			if(map[shot_y][shot_x] == 5) {
				map[shot_y][shot_x] = 10;
				setCursor(shot_y, shot_x);
				print(" ");
				shot_y = -1;
				shot_x = -1;
			} else{
				shot_y++;
				if(shot_y > 19){
					setCursor(19, shot_x);
					if(map[19][shot_x]!=10) write(map[19][shot_x]);
					else print(" ");
					shot_x = -1;
					shot_y = -1;
				}
			}
		}
		if (first_run)
		{
			for (int i = 0; i < 4; i++)
			{
				setCursor(0, i);
				for (int j = 0; j < 20; j++)
				{
					if (i == doodler_x && j == doodler_y)
					{
						if (map[j][i] != 10)
							write(1);
						else
							write(0);
					}
					else if (map[j][i] == 10)
					{
						print(" ");
					}
					else
					{
						write(map[j][i]);
					}
				}
			}
			first_run = false;
		}
		jump_step++;
		if (jump_step <= jump_number)
		{
//			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 81);
			PWM_Change_Tone(3000, 1);
			doodler_y++;
			if (score <= 7)
				score++;
			if (doodler_y >= 10)
			{
				doodler_y = 10;
				if (score < 999)
					score += volume_value;
				replaced_stick = 0;
				for (int i = 0; i < 19; i++)
				{
					for (int j = 0; j < 4; j++)
					{
						if (map[i][j] == map[i + 1][j])
							continue;
						map[i][j] = map[i + 1][j];
						setCursor(i, j);
						if (map[i][j] == 10)
						{
							print(" ");
						}
						else
						{
							write(map[i][j]);
						}
					}
				}
				for (int j = 0; j < 4; j++)
				{
					uint8_t use_stick = rand() % 100;
					if (use_stick <= probablity[volume_value][0])
					{
						uint8_t type = rand() % 100;
						if (type <= probablity[volume_value][1])
							map[19][j] = 2;
						else if (type <= probablity[volume_value][2])
							map[19][j] = 3;
						else if (type <= probablity[volume_value][3])
							map[19][j] = 4;
						else if (type <= probablity[volume_value][4])
							map[19][j] = 5;
						else if (type <= probablity[volume_value][5])
							map[19][j] = 6;
					}
					else
					{
						map[19][j] = 10; // 10 means nothing in that place
					}
					setCursor(19, j);
					if (map[19][j] == 10)
					{
						print(" ");
					}
					else
					{
						write(map[19][j]);
					}
				}
			}
		}
		else
		{
//			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 165);
			PWM_Change_Tone(2000, 1);
			doodler_y--;
			if (doodler_y < 0)
			{
				__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
				game_started = false;
				show_menu = false;
				finish_game = true;
				first_run = true;
			 PWM_Start();
			  Change_Melody(star_wars_melody, ARRAY_LENGTH(star_wars_melody));
				doodler_x = 0;
				doodler_y = 0;
				last_doodler_x = 0;
				last_doodler_y = 0;
				shot_x = -1;
				shot_y = -1;
				replaced_stick = 0;
				jump_step = 0;
				jump_number = 7;
			}
		}
	}
	else if (about_screen_showing)
	{
		if (first_run)
		{
			clear();
			first_run = false;
			setCursor(0, 0);
			print("Ali Nakhaee Sharif");
			setCursor(0, 1);
			print("Pariya Abadeh");
			HAL_RTC_GetTime(&hrtc, &mytime, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &mydate, RTC_FORMAT_BIN);
			char date_str[20];

			sprintf(date_str, "20%02d-%02d-%02d", mydate.Year, mydate.Month, mydate.Date);
			setCursor(0, 3);
			print(date_str);
		}

		HAL_RTC_GetTime(&hrtc, &mytime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &mydate, RTC_FORMAT_BIN);
		char time_str[20];
		sprintf(time_str, "%02d:%02d:%02d", mytime.Hours, mytime.Minutes, mytime.Seconds);
		setCursor(12, 3);
		print(time_str);
	}
	else if (show_menu)
	{
		if (first_run)
		{
			clear();
			first_run = false;
			setCursor(5, 1);
			print("Start  Game");
			setCursor(8, 2);
			print("About");
		}
	}
	else if (finish_game)
	{
		if (first_run)
		{
			clear();
			first_run = false;
			setCursor(0, 0);
			print(player_name);
			setCursor(0, 1);
			char score_string[100] = "SALAM";
			int n = sprintf(score_string, "score: %d", score);
			print(score_string);
			n = sprintf(score_string, "%s-score:%d\n", player_name, score);
			HAL_UART_Transmit(&huart1, score_string, n, 1000);
		}
		if (led_light < 5000)
		{
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, led_light);
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, led_light);
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, led_light);
		}
		else if (led_light < 1000)
		{
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 10000 - led_light);
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 10000 - led_light);
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 10000 - led_light);
		}
		led_light += 1000;
		if (led_light > 10000)
			led_light = 0;
	}
	else if (!game_started && !show_menu && !about_screen_showing)
	{
		setCursor(5, 0);
		print("Doodle Jump");
		setCursor(0, 1);
		write(1);
		print("     ");
		write(5);
		setCursor(0, 2);
		write(2);
		write(2);
		write(3);
		print("   ");
		write(2);
		print(" ");
		write(4);
		setCursor(0, 3);
		write(2);
		print(" ");
		write(2);
		print(" ");
		write(4);
		print("  ");
		write(6);
	}

  /* USER CODE END TIM3_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt / USART1 wake-up interrupt through EXTI line 25.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */
	extern unsigned char data1[1];
	if (show_menu)
	{
		if (!name_selected)
		{
			uart_counter++;
			if (data1[0] == '\n' || uart_counter > 15)
			{
				name_selected = true;
				setCursor(0, 3);
//				player_name[uart_counter] = '\0';
//				for(int i=strlen(player_name);i<15;i++)
//				{
//					player_name[i] = ' ';
//				}
				print(player_name);
			} else if(data1[0] >= 48 && data1[0]<=152) player_name[uart_counter] = data1[0];
		}
		else
		{
			if (data1[0] != '\n')
			{
				for(int i=0;i<14;i++)
				{
				    player_name[i] = ' ';
				}
				player_name[14] = '\0';
				uart_counter = 0;
				name_selected = false;
				player_name[0] = data1[0];
			}
		}
	}
	HAL_UART_Receive_IT(&huart1, data1, sizeof(data1));

  /* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
