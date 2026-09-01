/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * EEE3096S 2026 - Practical 1B
  * Tasks 2 and 3: fast integer square root, TIM16 timing, optimisation flags
  *
  * Student 1 : <name>  < MGWXAB002>
  * Student 2 :
  * Date      : <date>
  *
  * Board pins used
  *   PC13 : scope pulse. Driven LOW for the timed section, HIGH otherwise.
  *          Broken out on Header P1.
  *   PB1  : pass or fail indicator. User LED 1. ON means all ten golden
  *          values matched.
  *
  * Search for TODO. Every TODO is a piece of work you have to complete.
  * Do not delete the USER CODE markers. STM32CubeIDE overwrites everything
  * outside them whenever you regenerate from the .ioc file.
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */
/* USER CODE BEGIN PD */
#define PULSE_PIN    13u          /* PC13 */
#define LED_PIN      1u           /* PB1  */
#define TEST_INPUT   987654321u   /* the input named in the Task 2 question */
#define LONG_RUN_N   20000u       /* calls in the wrap-around run           */
/* USER CODE END PD */
/* USER CODE BEGIN PV */
/* The ten inputs from Task 1. Do not change these. */
static const uint32_t golden_inputs[10] = {
    0u, 1u, 15u, 16u, 4095u, 65535u,
    123456789u, 987654321u, 4294836225u, 4294967295u
};
/*
 * TODO 1 – filled with YOUR Task 1 golden outputs
 */
static const uint32_t golden_outputs[10] = {
    0u, 1u, 3u, 4u, 63u, 255u,
    11111u, 31426u, 65535u, 65535u
};
/*
 * Results. Keep these volatile so the optimiser leaves them alone at -O1
 * and above. Read them in the STM32CubeIDE Live Expressions view.
 */
volatile uint8_t  pass_all          = 0u;   /* 1 means all ten matched      */
volatile uint32_t single_call_span  = 0u;   /* timer counts, one call       */
volatile uint32_t long_run_span     = 0u;   /* timer counts, LONG_RUN_N     */
volatile float    mean_us_per_call  = 0.0f; /* long run divided by N        */
/* Sink for the return value. Stops the optimiser deleting the call. */
static volatile uint32_t sink = 0u;
/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void     gpio_init(void);
static void     timing_timer_init(void);
static uint32_t isqrt(uint32_t x);
static uint32_t time_one_call(uint32_t x);
static uint32_t time_n_calls(uint32_t x, uint32_t n);
/* USER CODE END PFP */
/* USER CODE BEGIN 0 */
/* ---------------------------------------------------------------------------
 * Hardware initialisation
 * ------------------------------------------------------------------------ */
static void gpio_init(void)
{
    /* TODO 2: Enable GPIOC and GPIOB clocks (AHB bus) */
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOBEN;

    /* TODO 3: PC13 and PB1 as general-purpose outputs */
    GPIOC->MODER = (GPIOC->MODER & ~(3u << (PULSE_PIN * 2))) | (1u << (PULSE_PIN * 2));
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (LED_PIN   * 2))) | (1u << (LED_PIN   * 2));

    /* TODO 4: Idle states – PC13 HIGH, PB1 LOW */
    GPIOC->BSRR = (1UL << PULSE_PIN);          /* PC13 high */
    GPIOB->BRR  = (1UL << LED_PIN);            /* PB1 low  */
}

static void timing_timer_init(void)
{
    /* TODO 5: Enable TIM16 clock (APB2 bus) */
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;

    /*
     * TODO 6: Prescaler
     * Clock path (from SystemClock_Config):
     *   HSI 8 MHz → AHB /1 → APB2 /1 → TIM16 clock = 8 MHz
     * We want 1 µs per count → PSC + 1 = 8 → PSC = 7
     */
    TIM16->PSC = 7;

    /* TODO 7: Free-running 16-bit counter */
    TIM16->ARR  = 0xFFFF;          /* max period */
    TIM16->EGR  = TIM_EGR_UG;      /* force load of PSC */
    TIM16->CR1 |= TIM_CR1_CEN;     /* enable counter */
}

/* ---------------------------------------------------------------------------
 * Task 2 core algorithm
 * ------------------------------------------------------------------------ */
static inline uint32_t square_le(uint32_t mid, uint32_t x)
{
    /* TODO 8: promote to 64-bit to avoid overflow */
    return ((uint64_t)mid * mid) <= x;
}

static uint32_t isqrt(uint32_t x)
{
    /* TODO 9: binary search */
    uint32_t lo = 0u;
    uint32_t hi = (x < 65536u) ? x : 65535u;   /* max possible isqrt for 32-bit */

    if (x == 0u) return 0u;
    if (x == 1u) return 1u;

    while (lo < hi) {
        uint32_t mid = lo + ((hi - lo + 1u) >> 1);  /* ceiling mid */
        if (square_le(mid, x))
            lo = mid;
        else
            hi = mid - 1u;
    }
    return lo;
}

/* ---------------------------------------------------------------------------
 * Timing harness
 * ------------------------------------------------------------------------ */
static uint32_t time_one_call(uint32_t x)
{
    uint16_t a, b;

    GPIOC->BRR = (1UL << PULSE_PIN);           /* PC13 low: pulse starts */

    a = TIM16->CNT;                            /* TODO 10 */
    sink = isqrt(x);
    b = TIM16->CNT;                            /* TODO 11 */

    GPIOC->BSRR = (1UL << PULSE_PIN);          /* PC13 high: pulse ends */

    /* TODO 12: wrap-safe span (16-bit) */
    return (uint32_t)((uint16_t)(b - a));
}

static uint32_t time_n_calls(uint32_t x, uint32_t n)
{
    uint16_t a, b;

    GPIOC->BRR = (1UL << PULSE_PIN);

    a = TIM16->CNT;                            /* TODO 13 */
    for (uint32_t i = 0u; i < n; i++)
        sink = isqrt(x);
    b = TIM16->CNT;                            /* TODO 14 */

    GPIOC->BSRR = (1UL << PULSE_PIN);

    /* TODO 15: same wrap-safe expression */
    return (uint32_t)((uint16_t)(b - a));
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* USER CODE BEGIN 2 */
  gpio_init();
  timing_timer_init();

  /* Self-test against the ten golden values from Task 1 */
  pass_all = 1u;
  for (int i = 0; i < 10; i++)
  {
      if (isqrt(golden_inputs[i]) != golden_outputs[i])
      {
          pass_all = 0u;
          break;
      }
  }

  /* TODO 16: LED on for pass */
  if (pass_all)
      GPIOB->BSRR = (1UL << LED_PIN);          /* PB1 high = LED on */
  else
      GPIOB->BRR  = (1UL << LED_PIN);
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    single_call_span = time_one_call(TEST_INPUT);

    /*
     * TODO 17 – long run for wrap check.
     * Comment this out while placing scope cursors on the single pulse.
     */
    long_run_span    = time_n_calls(TEST_INPUT, LONG_RUN_N);
    /* timer clock is 1 MHz → 1 count = 1 µs */
    mean_us_per_call = (float)long_run_span / (float)LONG_RUN_N;

    /* Gap so the scope sees a clean single pulse */
    for (volatile int d = 0; d < 100000; d++)
    {
    }
    /* USER CODE END 3 */
  }
}

/* ... rest of the file (SystemClock_Config, Error_Handler, etc.) stays unchanged ... */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */

/*
 * ---------------------------------------------------------------------------
 * TASK 3 CHECKLIST. No code changes needed below this line.
 * ---------------------------------------------------------------------------
 *
 * Build this same file four times, once per optimisation level.
 *
 *   Project > Properties > C/C++ Build > Settings > MCU GCC Compiler
 *     > Optimization > Optimization Level
 *
 *   None            -O0
 *   Optimize        -O1
 *   Optimize more   -O2
 *   Optimize size   -Os
 *
 * At every level record:
 *   1. Text size in bytes. Build Analyzer tab, or run
 *        arm-none-eabi-size Debug/Practical1B.elf
 *   2. single_call_span from Live Expressions.
 *   3. The PC13 pulse width from the scope, with cursors.
 *
 * At -O2 also dump the disassembly of your isqrt function:
 *        arm-none-eabi-objdump -d Debug/Practical1B.elf > disasm_O2.txt
 *   or open Debug/Practical1B.list, which the build already produces.
 * Find one transformation the compiler applied, name it, and point at the
 * source lines above it acts on.
 * ---------------------------------------------------------------------------
 */
