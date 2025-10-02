# Firmware Rules - Device EV

1. Memory usage phải được optimize (stack < 80%, heap < 70%)
2. Sử dụng appropriate data types (uint8_t, uint16_t, float)
3. Sensor reading phải có timeout và error handling
4. Communication protocol phải có checksum validation
5. Power management - sleep mode khi idle > 30s
6. Watchdog timer phải được enable
7. Critical sections phải disable interrupts
8. Buffer overflow protection cho UART/SPI communication
9. Hardware abstraction layer cho portability
10. Firmware version tracking trong code
