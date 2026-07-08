/* main.c – Полная прошивка для STM32F103C8T6 */
#include "stm32f1xx_hal.h"
#include <stdbool.h>

/* ------------------- Определения пинов ------------------- */
#define PIN_PB2   GPIO_PIN_2
#define PIN_PB12  GPIO_PIN_12
#define PIN_PB11  GPIO_PIN_11
#define PIN_PA3   GPIO_PIN_3
#define PIN_PA4   GPIO_PIN_4
#define PIN_PA7   GPIO_PIN_7
#define PIN_PB10  GPIO_PIN_10
#define PIN_PB1   GPIO_PIN_1
#define PIN_PB6   GPIO_PIN_6
#define PIN_PB9   GPIO_PIN_9
#define PIN_PB4   GPIO_PIN_4
#define PIN_PB3   GPIO_PIN_3

#define PORT_A    GPIOA
#define PORT_B    GPIOB

/* ------------------- Глобальные переменные ------------------- */
volatile uint32_t half_sec_counter = 0;
volatile bool    pb3_triggered = false;
volatile uint32_t trigger_half_count = 0;
volatile bool    pb10_set = false;

volatile bool tim1_running = false; // PB1 (1 кГц)
volatile bool tim2_running = false; // PA3 (1.5 кГц)
volatile bool tim3_running = false; // PA7 (3 кГц)
volatile bool tim4_running = false; // PB6 (14 кГц)

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim6;

/* ------------------- Прототипы функций ------------------- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM6_Init(void);

void set_pb10_high(void);
void start_pb3_actions(void);
void start_tim1_if_needed(void);
void stop_tim1_if_needed(void);

/* ==================== ОСНОВНАЯ ПРОГРАММА ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_TIM6_Init();

    // Запускаем таймер для 1 Гц (всегда)
    HAL_TIM_Base_Start_IT(&htim6);

    while (1)
    {
        // --- Проверка PB9 (вход с PullDown) ---
        if (HAL_GPIO_ReadPin(PORT_B, PIN_PB9) == GPIO_PIN_SET) {
            if (!tim1_running) {
                // Включаем дополнительный канал TIM1_CH3N (PB1) вручную
                TIM1->CCER |= TIM_CCER_CC3NE;
                tim1_running = true;
            }
        } else {
            if (tim1_running) {
                TIM1->CCER &= ~TIM_CCER_CC3NE;
                tim1_running = false;
                HAL_GPIO_WritePin(PORT_B, PIN_PB1, GPIO_PIN_RESET);
            }
        }

        // --- Проверка PB3 (вход с PullUp) – запуск действий при LOW (однократно) ---
        if (!pb3_triggered && HAL_GPIO_ReadPin(PORT_B, PIN_PB3) == GPIO_PIN_RESET) {
            pb3_triggered = true;
            trigger_half_count = half_sec_counter;
            start_pb3_actions();
        }

        HAL_Delay(10);
    }
}

/* ==================== ИНИЦИАЛИЗАЦИЯ ==================== */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;   // 8 МГц * 9 = 72 МГц
    HAL_RCC_OscConfig(&osc);

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                    | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;      // HCLK = 72 МГц
    clk.APB1CLKDivider = RCC_HCLK_DIV2;       // PCLK1 = 36 МГц
    clk.APB2CLKDivider = RCC_HCLK_DIV1;       // PCLK2 = 72 МГц
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // ---------- Выходы (default LOW) ----------
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pull = GPIO_NOPULL;

    // PB2
    gpio.Pin = PIN_PB2;
    HAL_GPIO_Init(PORT_B, &gpio);
    HAL_GPIO_WritePin(PORT_B, PIN_PB2, GPIO_PIN_RESET);

    // PB12
    gpio.Pin = PIN_PB12;
    HAL_GPIO_Init(PORT_B, &gpio);
    HAL_GPIO_WritePin(PORT_B, PIN_PB12, GPIO_PIN_RESET);

    // PB11
    gpio.Pin = PIN_PB11;
    HAL_GPIO_Init(PORT_B, &gpio);
    HAL_GPIO_WritePin(PORT_B, PIN_PB11, GPIO_PIN_RESET);

    // PB10
    gpio.Pin = PIN_PB10;
    HAL_GPIO_Init(PORT_B, &gpio);
    HAL_GPIO_WritePin(PORT_B, PIN_PB10, GPIO_PIN_RESET);

    // PB1 – будет переопределён как AF для TIM1
    HAL_GPIO_WritePin(PORT_B, PIN_PB1, GPIO_PIN_RESET);

    // PA4
    gpio.Pin = PIN_PA4;
    HAL_GPIO_Init(PORT_A, &gpio);
    HAL_GPIO_WritePin(PORT_A, PIN_PA4, GPIO_PIN_RESET);

    // ---------- Входы ----------
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    // PB9, PB4 – PullDown
    gpio.Pin = PIN_PB9;
    HAL_GPIO_Init(PORT_B, &gpio);
    gpio.Pin = PIN_PB4;
    HAL_GPIO_Init(PORT_B, &gpio);

    // PB3 – PullUp
    gpio.Pull = GPIO_PULLUP;
    gpio.Pin = PIN_PB3;
    HAL_GPIO_Init(PORT_B, &gpio);
}

/* ---------- TIM1 (1 кГц на PB1, канал 3N) ---------- */
static void MX_TIM1_Init(void)
{
    __HAL_RCC_TIM1_CLK_ENABLE();

    // PB1 как AF1 (TIM1_CH3N)
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = PIN_PB1;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(PORT_B, &gpio);

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 1;                  // 72 МГц / 2 = 36 МГц
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 35999;                 // 36 МГц / 36000 = 1 кГц
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim1);

    // Настройка канала 3 (основной) – он не будет использоваться, но нужен для генерации
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 18000;                         // 50% скважность
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_LOW;     // дополнительный канал инвертирован, чтобы на выходе был тот же сигнал
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_3);

    // Включаем дополнительный канал (CC3NE), отключаем основной (CC3E)
    TIM1->CCER = (TIM1->CCER & ~TIM_CCER_CC3E) | TIM_CCER_CC3NE;
    // Разрешаем выходы TIM1
    TIM1->BDTR |= TIM_BDTR_MOE;
    // Таймер ещё не запущен – запустим его сразу, чтобы был готов
    __HAL_TIM_ENABLE(&htim1);
    // Но выход дополнительного канала выключен (CC3NE=0) – включим по условию
    TIM1->CCER &= ~TIM_CCER_CC3NE; // пока выключен
}

/* ---------- TIM2 (1.5 кГц на PA3, канал 4) ---------- */
static void MX_TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = PIN_PA3;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(PORT_A, &gpio);

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;                 // 72 МГц
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 47999;                // 72e6/1500 - 1
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 24000;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_4);

    // Пока выключен
}

/* ---------- TIM3 (3 кГц на PA7, канал 2) ---------- */
static void MX_TIM3_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = PIN_PA7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(PORT_A, &gpio);

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 23999;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 12000;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_2);

    // Пока выключен
}

/* ---------- TIM4 (14 кГц на PB6, канал 1) ---------- */
static void MX_TIM4_Init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = PIN_PB6;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(PORT_B, &gpio);

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 5142;                 // 72e6/14000 - 1 ≈ 5141.85
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim4);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 2571;                          // 5143/2 ≈ 2571.5
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim4, &oc, TIM_CHANNEL_1);

    // Пока выключен
}

/* ---------- TIM6 (1 Гц и отсчёт 10 секунд) ---------- */
static void MX_TIM6_Init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 7199;              // 72 МГц / 7200 = 10 кГц
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 4999;                 // 10 кГц / 5000 = 2 Гц (период 0.5 с)
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim6);

    HAL_NVIC_SetPriority(TIM6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);
}

/* ==================== ПРЕРЫВАНИЕ TIM6 ==================== */
void TIM6_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        half_sec_counter++;

        // --- Переключение 1 Гц на PB2, PB12, PB11, PA4 ---
        HAL_GPIO_TogglePin(PORT_B, PIN_PB2);
        HAL_GPIO_TogglePin(PORT_B, PIN_PB12);
        HAL_GPIO_TogglePin(PORT_B, PIN_PB11);
        HAL_GPIO_TogglePin(PORT_A, PIN_PA4);

        // --- Проверка 10 секунд для PB10 (после срабатывания PB3) ---
        if (pb3_triggered && !pb10_set) {
            uint32_t elapsed = half_sec_counter - trigger_half_count;
            if (elapsed >= 20) {   // 20 * 0.5 с = 10 с
                set_pb10_high();
                pb10_set = true;
            }
        }
    }
}

/* ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==================== */
void set_pb10_high(void)
{
    HAL_GPIO_WritePin(PORT_B, PIN_PB10, GPIO_PIN_SET);
}

void start_pb3_actions(void)
{
    // Включаем таймеры (PWM) для PB6, PA3, PA7
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    tim4_running = true;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    tim2_running = true;
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    tim3_running = true;
    // PB10 установится через 10 секунд в прерывании
}

/* ==================== ОБРАБОТЧИК ОШИБОК ==================== */
void Error_Handler(void)
{
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    while (1) {}
}
#endif
