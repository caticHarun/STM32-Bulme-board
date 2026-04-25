# STM32
STM32 Dev - Harun

## Printf

1. Open [https://www.serialterminal.com/](Serial Terminal in chrome)
2. Check Baud in CubeMX (Pinout & Configuration -> Connectivity -> USART2 -> Baud Rate)
3. Enter that baud in Serial Terminal 
4. Use 
```c
uint8_t message[] = "Button down\n";
HAL_UART_Transmit(&huart2, message, sizeof(message) - 1, HAL_MAX_TIMEOUT);
```