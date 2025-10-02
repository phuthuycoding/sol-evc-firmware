/**
 * @file main.c
 * @brief STM32F103 Master Controller Main Entry Point
 * @version 1.0.0
 */

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Private includes */
#include "device_manager.h"
#include "ocpp_client.h"
#include "meter_service.h"
#include "relay_control.h"
#include "esp8266_comm.h"
#include "safety_monitor.h"
#include "uart_protocol.h"
#include "device_config.h"

/* Private variables */
static device_config_t g_device_config;
static TaskHandle_t heartbeat_task_handle;
static TaskHandle_t meter_task_handle;
static TaskHandle_t status_task_handle;
static TaskHandle_t safety_task_handle;
static TaskHandle_t comm_task_handle;

/* Hardware handles */
UART_HandleTypeDef huart1;  // ESP8266
UART_HandleTypeDef huart2;  // Debug
UART_HandleTypeDef huart3;  // RS485
SPI_HandleTypeDef hspi1;    // CS5460A
IWDG_HandleTypeDef hiwdg;

/* Private function prototypes */
static void system_clock_config(void);
static void gpio_init(void);
static void uart_init(void);
static void spi_init(void);
static void iwdg_init(void);
static void create_tasks(void);

/* Task functions */
static void heartbeat_task(void *argument);
static void meter_reading_task(void *argument);
static void status_monitor_task(void *argument);
static void safety_monitor_task(void *argument);
static void communication_task(void *argument);

/**
 * @brief Application entry point
 */
int main(void)
{
    /* MCU Configuration */
    HAL_Init();
    system_clock_config();
    gpio_init();
    uart_init();
    spi_init();
    iwdg_init();
    
    /* Initialize device configuration */
    device_manager_init(&g_device_config);
    
    /* Initialize hardware services */
    esp8266_comm_init();
    meter_service_init();
    relay_control_init();
    safety_monitor_init();
    
    /* Initialize OCPP client */
    ocpp_client_init(&g_device_config);
    
    /* Create FreeRTOS tasks */
    create_tasks();
    
    /* Start scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    while (1) {
        HAL_Delay(1000);
    }
}

/**
 * @brief System Clock Configuration - 72MHz
 */
static void system_clock_config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure HSE */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Configure system clock */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization
 */
static void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure LED pin (PC13) */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure relay control pins (PA0-PA9) */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                         GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|
                         GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure CS5460A chip select pins (PB0-PB9) */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                         GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|
                         GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Set all CS pins high (inactive) */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                             GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|
                             GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_SET);

    /* Configure ESP8266 reset pin (PA10) */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure RS485 DE pin (PA11) */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief UART Initialization
 */
static void uart_init(void)
{
    /* UART1 - ESP8266 Communication */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUD_RATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    /* UART2 - Debug */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }

    /* UART3 - RS485 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = RS485_BAUD_RATE;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief SPI Initialization for CS5460A
 */
static void spi_init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief Watchdog Initialization
 */
static void iwdg_init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = 4095;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief Create FreeRTOS tasks
 */
static void create_tasks(void)
{
    /* Create communication task (highest priority for responsiveness) */
    xTaskCreate(communication_task, "Comm", 512, NULL, 3, &comm_task_handle);
    
    /* Create safety monitor task (critical priority) */
    xTaskCreate(safety_monitor_task, "Safety", 256, NULL, 4, &safety_task_handle);
    
    /* Create meter reading task */
    xTaskCreate(meter_reading_task, "Meter", 512, NULL, 2, &meter_task_handle);
    
    /* Create status monitor task */
    xTaskCreate(status_monitor_task, "Status", 256, NULL, 2, &status_task_handle);
    
    /* Create heartbeat task (lowest priority) */
    xTaskCreate(heartbeat_task, "Heartbeat", 256, NULL, 1, &heartbeat_task_handle);
}

/**
 * @brief Communication task - Handle ESP8266 UART protocol
 */
static void communication_task(void *argument)
{
    UNUSED(argument);
    uart_packet_t rx_packet;
    
    while (1) {
        /* Check for incoming packets from ESP8266 */
        if (uart_receive_packet(&rx_packet, 100)) {
            esp8266_comm_handle_packet(&rx_packet);
        }
        
        /* Process outgoing message queue */
        esp8266_comm_process_queue();
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Safety monitor task - Highest priority
 */
static void safety_monitor_task(void *argument)
{
    UNUSED(argument);
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        /* Monitor safety conditions */
        safety_monitor_check_all();
        
        /* Refresh watchdog */
        HAL_IWDG_Refresh(&hiwdg);
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SAFETY_CHECK_INTERVAL));
    }
}

/**
 * @brief Meter reading task - UC006
 */
static void meter_reading_task(void *argument)
{
    UNUSED(argument);
    TickType_t last_wake_time = xTaskGetTickCount();
    meter_sample_t readings[MAX_CONNECTORS];
    
    while (1) {
        /* Read all meter channels */
        for (uint8_t i = 0; i < g_device_config.connector_count; i++) {
            if (g_device_config.connectors[i].enabled) {
                meter_service_read_channel(i, &readings[i]);
            }
        }
        
        /* Send meter values for active connectors */
        ocpp_client_send_meter_values(readings);
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(METER_READING_INTERVAL));
    }
}

/**
 * @brief Status monitor task - UC003
 */
static void status_monitor_task(void *argument)
{
    UNUSED(argument);
    TickType_t last_wake_time = xTaskGetTickCount();
    static connector_status_t prev_status[MAX_CONNECTORS] = {0};
    
    while (1) {
        /* Check status changes for all connectors */
        for (uint8_t i = 0; i < g_device_config.connector_count; i++) {
            connector_status_t current_status = ocpp_client_get_connector_status(i);
            
            if (current_status != prev_status[i]) {
                /* Status changed - send notification */
                ocpp_client_send_status_notification(i, current_status, ERROR_NO_ERROR);
                prev_status[i] = current_status;
            }
        }
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(STATUS_CHECK_INTERVAL));
    }
}

/**
 * @brief Heartbeat task - UC002
 */
static void heartbeat_task(void *argument)
{
    UNUSED(argument);
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        /* Send heartbeat */
        occp_client_send_heartbeat();
        
        /* Toggle status LED */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(HEARTBEAT_INTERVAL));
    }
}

/**
 * @brief Error Handler
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* Error indication - fast blink */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_Delay(100);
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief Assert failed handler
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add implementation to report file name and line number */
    Error_Handler();
}
#endif
