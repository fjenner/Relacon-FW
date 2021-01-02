// ST libraries
#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_gpio.h"

// TinyUSB
#include "tusb.h"

#ifdef ENABLE_UART_DEBUG
#include "printf.h"
#endif

#include <stdint.h>
#include <stdarg.h>

/*
 * Important: The port designations used by the ADU protocol and by the
 * higher-level (i.e. above the board-level) software is independent of the
 * actual microcontroller port designations. For example, the ADU protocol
 * uses the "PORT K" designation for the relay port even though it is actually
 * implemented on "PORT A" for this board. Here is the mapping of the ADU port
 * designations for this board:
 *
 * ADU Port | MCU Port
 * ---------+---------
 * PORTK0   | PORTA0
 * PORTK1   | PORTA1
 * PORTK2   | PORTA2
 * PORTK3   | PORTA3
 * PORTK4   | PORTA4
 * PORTK5   | PORTA5
 * PORTK6   | PORTA6
 * PORTK7   | PORTA7
 * PORTA0   | PORTB0
 * PORTA1   | PORTB1
 * PORTA2   | PORTA8 <-- Note: A PORTB2 pin does not exist on this MCU part
 * PORTA3   | PORTB3
 * PORTB0   | PORTB4
 * PORTB1   | PORTB5
 * PORTB2   | PORTB6
 * PORTB3   | PORTB7
 */

// GPIO ports used by this application
#define PORT_RELAYS         GPIOA
#define PORT_USART          GPIOA
#define PORT_INPUTS_BANK1   GPIOB
#define PORT_INPUTS_BANK2   GPIOA

// Relay output pins
#define PIN_RELAY_0         GPIO_PIN_0
#define PIN_RELAY_1         GPIO_PIN_1
#define PIN_RELAY_2         GPIO_PIN_2
#define PIN_RELAY_3         GPIO_PIN_3
#define PIN_RELAY_4         GPIO_PIN_4
#define PIN_RELAY_5         GPIO_PIN_5
#define PIN_RELAY_6         GPIO_PIN_6
#define PIN_RELAY_7         GPIO_PIN_7

#define PIN_RELAY_ALL       (PIN_RELAY_0 | PIN_RELAY_1 | \
                             PIN_RELAY_2 | PIN_RELAY_3 | \
                             PIN_RELAY_4 | PIN_RELAY_5 | \
                             PIN_RELAY_6 | PIN_RELAY_7)

// Input pins
#define PIN_INPUT_BANK1_0   GPIO_PIN_0
#define PIN_INPUT_BANK1_1   GPIO_PIN_1
#define PIN_INPUT_BANK2_2   GPIO_PIN_8
#define PIN_INPUT_BANK1_3   GPIO_PIN_3
#define PIN_INPUT_BANK1_4   GPIO_PIN_4
#define PIN_INPUT_BANK1_5   GPIO_PIN_5
#define PIN_INPUT_BANK1_6   GPIO_PIN_6
#define PIN_INPUT_BANK1_7   GPIO_PIN_7

#define PIN_INPUT_BANK1_ALL (PIN_INPUT_BANK1_0 | PIN_INPUT_BANK1_1 | \
                             0 /* No PORTB2 */ | PIN_INPUT_BANK1_3 | \
                             PIN_INPUT_BANK1_4 | PIN_INPUT_BANK1_5 | \
                             PIN_INPUT_BANK1_6 | PIN_INPUT_BANK1_7)

#define PIN_INPUT_BANK2_ALL PIN_INPUT_BANK2_2

// UART pins
#define PIN_USART_TX        GPIO_PIN_9
#define PIN_USART_RX        GPIO_PIN_10

#define PIN_USART_ALL       (PIN_USART_TX | PIN_USART_RX)

#define DEBUG_CONSOLE_BAUD_RATE 115200

#ifdef ENABLE_UART_DEBUG
static UART_HandleTypeDef UartHandle =
{
    .Instance = USART1,
    .Init =
    {
        .BaudRate = DEBUG_CONSOLE_BAUD_RATE,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .HwFlowCtl = UART_HWCONTROL_NONE,
        .Mode = UART_MODE_TX_RX,
        .OverSampling = UART_OVERSAMPLING_16
    }
};
#endif

static TIM_HandleTypeDef TimerHandle =
{
    .Instance = TIM2,
    .Init =
    {
        .Prescaler = 48 - 1, // Scale the 48MHz clock source to a 1us period
        .CounterMode = TIM_COUNTERMODE_UP,
        .Period = 0xffffffff,
        .ClockDivision = TIM_CLOCKDIVISION_DIV1,
        .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE
    }
};

/**
 * The SysTick interrupt handler. This overrides the default handler in the
 * startup assembly file. This one simply calls the HAL_IncTick() function in
 * ST's HAL framework, which they require in order for functionality such as
 * HAL_Delay() to work properly.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
 * The USB interrupt handler. This overrides the default handler in the
 * startup assembly file. We simply delegate to TinyUSB to actually handle
 * the interrupt.
 */
void USB_IRQHandler(void)
{
    tud_int_handler(0);
}

static void InitClocks()
{
    // There's no external oscillator on this board. Enable the 48MHz high
    // speed internal oscillator that we need for USB anyway and also use it
    // to drive the PLL
    RCC_OscInitTypeDef oscConfig =
    {
        .OscillatorType = RCC_OSCILLATORTYPE_HSI48,
        .HSI48State = RCC_HSI48_ON,
        .PLL.PLLState = RCC_PLL_ON,
        .PLL.PLLSource = RCC_PLLSOURCE_HSI48,
        .PLL.PREDIV = RCC_PREDIV_DIV2,
        .PLL.PLLMUL = RCC_PLL_MUL2,
    };

    if (HAL_RCC_OscConfig(&oscConfig) != HAL_OK)
    {
        // Error
        for (;;);
    }

    // Drive SYSCLK, HCLK, and PCLK1 from PLL with the specified dividers
    RCC_ClkInitTypeDef clkConfig =
    {
        .ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1,
        .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
        .AHBCLKDivider = RCC_SYSCLK_DIV1,
        .APB1CLKDivider = RCC_HCLK_DIV1,
    };

    if (HAL_RCC_ClockConfig(&clkConfig, FLASH_LATENCY_1) != HAL_OK)
    {
        // Error
        for (;;);
    }

    // Enable peripheral clocks
    __HAL_RCC_GPIOA_CLK_ENABLE(); // PORT_RELAYS, PORT_USART, and PORT_INPUT_BANK2
    __HAL_RCC_GPIOB_CLK_ENABLE(); // PORT_INPUT_BANK1
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_USB_CLK_ENABLE();
#ifdef ENABLE_UART_DEBUG
    __HAL_RCC_USART1_CLK_ENABLE();
#endif
}

static void InitPins()
{
    // Initialize GPIO output pins for relays
    GPIO_InitTypeDef gpioConfigRelays =
    {
        .Pin = PIN_RELAY_ALL,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_PULLDOWN,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(PORT_RELAYS, &gpioConfigRelays);

    // Initialize GPIO input pins
    GPIO_InitTypeDef gpioConfigInputsBank1 =
    {
        .Pin = PIN_INPUT_BANK1_ALL,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_PULLDOWN,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(PORT_INPUTS_BANK1, &gpioConfigInputsBank1);

    GPIO_InitTypeDef gpioConfigInputsBank2 =
    {
        .Pin = PIN_INPUT_BANK2_ALL,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_PULLDOWN,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(PORT_INPUTS_BANK2, &gpioConfigInputsBank2);

#ifdef ENABLE_UART_DEBUG
    // Initialize USART TX/RX pins
    GPIO_InitTypeDef gpioConfigUsart =
    {
        .Pin = PIN_USART_ALL,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF1_USART1
    };
    HAL_GPIO_Init(PORT_USART, &gpioConfigUsart);
#endif
}

static void InitPeripherals()
{
    // Initialize timer used for BoardGetElapsedTimeUs() time base
    if (HAL_TIM_Base_Init(&TimerHandle) != HAL_OK)
    {
        // Error
        for (;;);
    }
    if (HAL_TIM_Base_Start(&TimerHandle) != HAL_OK)
    {
        // Error
        for (;;);
    }

#ifdef ENABLE_UART_DEBUG
    // Initialize UART
    if (HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        // Error
        for (;;);
    }
#endif
}

void BoardInit()
{
    HAL_Init();

    InitClocks();
    InitPins();
    InitPeripherals();
}

uint32_t BoardGetElapsedTimeUs()
{
    return __HAL_TIM_GET_COUNTER(&TimerHandle);
}

void BoardWriteRelays(uint8_t relayState)
{
    LL_GPIO_WriteOutputPort(PORT_RELAYS, relayState);
}

uint8_t BoardReadRelays()
{
    return LL_GPIO_ReadOutputPort(PORT_RELAYS) & PIN_RELAY_ALL;
}

uint8_t BoardReadDigitalInputs()
{
    uint32_t portInputBank1 = LL_GPIO_ReadInputPort(PORT_INPUTS_BANK1);
    uint32_t portInputBank2 = LL_GPIO_ReadInputPort(PORT_INPUTS_BANK2);

    uint32_t pinsInputBank1 = portInputBank1 & PIN_INPUT_BANK1_ALL;
    uint32_t pinsInputBank2 = portInputBank2 & PIN_INPUT_BANK2_ALL;

    // Shift the PORTA8 bit into the gap from the missing PORTB2 bit
    return pinsInputBank1 | (pinsInputBank2 >> 6);
}

#ifdef ENABLE_UART_DEBUG
int BoardDebugPrint(const char *format, ...)
{
    char buf[128];
    va_list va;
    va_start(va, format);
    int len = vsnprintf_(buf, sizeof(buf), format, va);
    va_end(va);

    if (len > 0)
    {
        HAL_UART_Transmit(&UartHandle, (uint8_t*)buf, len, HAL_MAX_DELAY);
    }

    return len;
}
#endif