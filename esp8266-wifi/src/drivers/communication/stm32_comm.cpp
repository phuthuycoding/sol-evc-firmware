/**
 * @file stm32_comm.cpp
 * @brief STM32 UART Communication implementation
 * @version 2.0.0
 */

#include "drivers/communication/stm32_comm.h"

/**
 * @brief Constructor
 */
STM32Communicator::STM32Communicator()
    : txSequence(0),
      userCallback(nullptr),
      lastRxTime(0) {

    memset(&status, 0, sizeof(STM32Status));
}

/**
 * @brief Initialize UART
 */
UARTError STM32Communicator::init(uint32_t baudRate) {
    Serial.begin(baudRate);
    Serial.setTimeout(100);

    rxBuffer.clear();
    txSequence = 0;
    lastRxTime = millis();

    Serial.println(F("[STM32] UART communication initialized"));
    return UARTError::SUCCESS;
}

/**
 * @brief Send packet to STM32
 */
UARTError STM32Communicator::sendPacket(const uart_packet_t& packet) {
    // Create mutable copy to set checksum
    uart_packet_t txPacket = packet;
    txPacket.checksum = uart_calculate_checksum(&txPacket);

    // Send packet header
    Serial.write(txPacket.start_byte);
    Serial.write(txPacket.cmd_type);
    Serial.write((uint8_t)(txPacket.length & 0xFF));
    Serial.write((uint8_t)(txPacket.length >> 8));
    Serial.write(txPacket.sequence);

    // Send payload if present
    if (txPacket.length > 0) {
        Serial.write(txPacket.payload, txPacket.length);
    }

    // Send footer
    Serial.write(txPacket.checksum);
    Serial.write(txPacket.end_byte);

    status.messageTxCount++;

    Serial.printf("[STM32] TX: CMD=0x%02X LEN=%u SEQ=%u\n",
                  txPacket.cmd_type, txPacket.length, txPacket.sequence);

    return UARTError::SUCCESS;
}

/**
 * @brief Send command with payload
 */
UARTError STM32Communicator::sendCommand(uint8_t cmdType, const void* payload, uint16_t length) {
    if (length > UART_MAX_PAYLOAD) {
        return UARTError::INVALID_PARAM;
    }

    uart_packet_t packet;
    uart_init_packet(&packet, cmdType, txSequence++);

    if (length > 0 && payload != nullptr) {
        memcpy(packet.payload, payload, length);
        packet.length = length;
    } else {
        packet.length = 0;
    }

    return sendPacket(packet);
}

/**
 * @brief Send ACK response
 */
UARTError STM32Communicator::sendAck(uint8_t sequence, uint8_t statusCode) {
    uart_packet_t packet;
    uart_init_packet(&packet, RSP_MQTT_ACK, sequence);
    packet.payload[0] = statusCode;
    packet.length = 1;

    return sendPacket(packet);
}

/**
 * @brief Handle UART communication
 */
void STM32Communicator::handle() {
    // Read available data into ring buffer
    while (Serial.available()) {
        uint8_t byte = Serial.read();

        if (!rxBuffer.push(byte)) {
            // Buffer overflow
            status.errorCount++;
            Serial.println(F("[STM32] RX buffer overflow, clearing old data"));
            rxBuffer.discard(64); // Discard 64 bytes to make room
            rxBuffer.push(byte);
        }

        lastRxTime = millis();
    }

    // Try to parse packets from buffer
    uart_packet_t packet;
    while (parsePacket(packet)) {
        // Valid packet received
        status.messageRxCount++;
        status.lastHeartbeat = millis();
        status.connected = true;

        Serial.printf("[STM32] RX: CMD=0x%02X LEN=%u SEQ=%u\n",
                      packet.cmd_type, packet.length, packet.sequence);

        // Handle packet
        handleParsedPacket(packet);

        // Call user callback
        if (userCallback) {
            userCallback(&packet);
        }
    }

    // Check for connection timeout
    updateStatus();

    // Check for parse timeout (stale data in buffer)
    if (rxBuffer.available() > 0) {
        if (millis() - lastRxTime > PARSE_TIMEOUT) {
            Serial.printf("[STM32] Parse timeout, discarding %u bytes\n", rxBuffer.available());
            rxBuffer.clear();
            status.timeoutErrors++;
        }
    }
}

/**
 * @brief Parse packet from ring buffer
 */
bool STM32Communicator::parsePacket(uart_packet_t& packet) {
    // Need at least 8 bytes for minimum packet (header + footer, no payload)
    if (rxBuffer.available() < 8) {
        return false;
    }

    // Search for start byte
    uint8_t byte;
    size_t searchCount = 0;
    bool foundStart = false;

    while (rxBuffer.available() >= 8 && searchCount < 256) {
        if (!rxBuffer.peek(byte)) {
            return false;
        }

        if (byte == UART_START_BYTE) {
            foundStart = true;
            break;
        }

        // Not start byte, discard it
        rxBuffer.pop(byte);
        searchCount++;
    }

    if (!foundStart) {
        return false;
    }

    // Try to parse header
    uint8_t header[5];
    for (int i = 0; i < 5; i++) {
        if (!rxBuffer.peekAt(i, header[i])) {
            return false; // Not enough data yet
        }
    }

    // Extract packet info
    packet.start_byte = header[0];
    packet.cmd_type = header[1];
    packet.length = header[2] | (header[3] << 8);
    packet.sequence = header[4];

    // Validate packet length
    if (packet.length > UART_MAX_PAYLOAD) {
        Serial.printf("[STM32] Invalid packet length: %u\n", packet.length);
        rxBuffer.pop(byte); // Discard start byte
        status.errorCount++;
        return false;
    }

    // Calculate total packet size
    size_t packetSize = 5 + packet.length + 2; // Header + payload + checksum + end byte

    // Check if we have complete packet
    if (rxBuffer.available() < packetSize) {
        return false; // Wait for more data
    }

    // Read payload
    for (uint16_t i = 0; i < packet.length; i++) {
        if (!rxBuffer.peekAt(5 + i, packet.payload[i])) {
            return false;
        }
    }

    // Read footer
    if (!rxBuffer.peekAt(5 + packet.length, packet.checksum)) {
        return false;
    }
    if (!rxBuffer.peekAt(5 + packet.length + 1, packet.end_byte)) {
        return false;
    }

    // Validate packet
    if (!validatePacket(packet)) {
        // Invalid packet, discard start byte and try again
        rxBuffer.pop(byte);
        return false;
    }

    // Valid packet! Remove it from buffer
    rxBuffer.discard(packetSize);

    return true;
}

/**
 * @brief Validate packet
 */
bool STM32Communicator::validatePacket(const uart_packet_t& packet) const {
    // Check end byte
    if (packet.end_byte != UART_END_BYTE) {
        Serial.printf("[STM32] Invalid end byte: 0x%02X\n", packet.end_byte);
        return false;
    }

    // Validate checksum
    uint8_t calculatedChecksum = uart_calculate_checksum(&packet);
    if (calculatedChecksum != packet.checksum) {
        Serial.printf("[STM32] Checksum error: calc=0x%02X recv=0x%02X\n",
                      calculatedChecksum, packet.checksum);
        const_cast<STM32Status&>(status).checksumErrors++;
        const_cast<STM32Status&>(status).errorCount++;
        return false;
    }

    return true;
}

/**
 * @brief Handle parsed packet (internal commands)
 */
void STM32Communicator::handleParsedPacket(const uart_packet_t& packet) {
    // This is where you'd handle specific commands
    // For now, just log
    switch (packet.cmd_type) {
        case CMD_MQTT_PUBLISH:
            Serial.println(F("[STM32] Received MQTT publish request"));
            // Will be handled by user callback
            break;

        case CMD_GET_TIME:
            Serial.println(F("[STM32] Time request received"));
            // Will be handled by user callback
            break;

        case CMD_WIFI_STATUS:
            Serial.println(F("[STM32] WiFi status request received"));
            // Will be handled by user callback
            break;

        default:
            Serial.printf("[STM32] Unknown command: 0x%02X\n", packet.cmd_type);
            sendAck(packet.sequence, STATUS_INVALID);
            break;
    }
}

/**
 * @brief Update status
 */
void STM32Communicator::updateStatus() {
    // Check connection timeout
    if (millis() - status.lastHeartbeat > CONNECTION_TIMEOUT) {
        status.connected = false;
    }
}

/**
 * @brief Set packet callback
 */
void STM32Communicator::setCallback(PacketCallback callback) {
    userCallback = callback;
}

/**
 * @brief Check if connected
 */
bool STM32Communicator::isConnected() const {
    return (millis() - status.lastHeartbeat) < CONNECTION_TIMEOUT;
}
