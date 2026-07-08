/* stm32f1xx_hal_conf.h – конфигурация HAL, обязательное значение HSE */
#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------- Module Selection -----------------------*/
#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
/* Добавьте другие модули по необходимости */

/*----------------------- HSE/HSI Values ------------------------*/
#define HSE_VALUE    ((uint32_t)8000000)   /* Внешний кварц 8 МГц */
#define HSI_VALUE    ((uint32_t)8000000)   /* Внутренний 8 МГц (не используется) */

/*----------------------- Memory Management ----------------------*/
#define USE_HAL_DRIVER

/*----------------------- Asserts -------------------------------*/
// #define USE_FULL_ASSERT    1
// #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CONF_H */