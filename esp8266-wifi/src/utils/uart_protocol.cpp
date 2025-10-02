/**
 * @file uart_protocol.cpp
 * @brief UART protocol implementation
 * @version 1.0.0
 */

#include "uart_protocol.h"

/**
 * @brief Calculate checksum for UART packet
 */
uint8_t uart_calculate_checksum(const uart_packet_t* packet) {
    if (packet == nullptr) return 0;

    uint8_t checksum = 0;
    checksum ^= packet->cmd_type;
    checksum ^= (packet->length & 0xFF);
    checksum ^= ((packet->length >> 8) & 0xFF);
    checksum ^= packet->sequence;

    for (uint16_t i = 0; i < packet->length; i++) {
        checksum ^= packet->payload[i];
    }

    return checksum;
}

/**
 * @brief Initialize UART packet
 */
void uart_init_packet(uart_packet_t* packet, uint8_t cmd_type, uint8_t sequence) {
    if (packet == nullptr) return;

    packet->start_byte = UART_START_BYTE;
    packet->cmd_type = cmd_type;
    packet->length = 0;
    packet->sequence = sequence;
    packet->checksum = 0;
    packet->end_byte = UART_END_BYTE;
}
