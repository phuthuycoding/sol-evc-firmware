/**
 * @file stm32_comm.h
 * @brief STM32 UART Communication (ESP8266 optimized)
 * @version 2.0.0
 *
 * Changes from v1:
 * - Uses RingBuffer instead of raw array
 * - Class-based design (stack allocated)
 * - Merged duplicate process/handle functions
 * - Better packet parsing with timeout
 * - Error codes instead of bool
 */

#ifndef STM32_COMM_H
#define STM32_COMM_H

#include <Arduino.h>
#include "utils/ring_buffer.h"
#include "../../shared/uart_protocol.h"

/**
 * @brief UART Communication error codes
 */
enum class UARTError : int8_t {
    SUCCESS = 0,
    INVALID_PARAM = -1,
    BUFFER_OVERFLOW = -2,
    CHECKSUM_ERROR = -3,
    TIMEOUT = -4,
    NOT_CONNECTED = -5,
    PARSE_ERROR = -6
};

/**
 * @brief STM32 communication status
 */
struct STM32Status {
    bool connected;
    uint32_t lastHeartbeat;
    uint32_t messageTxCount;
    uint32_t messageRxCount;
    uint32_t errorCount;
    uint32_t checksumErrors;
    uint32_t timeoutErrors;
};

/**
 * @brief Packet received callback
 */
typedef void (*PacketCallback)(const uart_packet_t* packet);

/**
 * @brief OOP STM32 Communicator
 *
 * Usage:
 *   STM32Communicator stm32;
 *   stm32.sendCommand(CMD_GET_TIME, nullptr, 0);
 *   stm32.handle(); // Call in loop()
 */
class STM32Communicator {
private:
    // RX buffer (512 bytes - optimized for ESP8266)
    RingBuffer<512> rxBuffer;

    // TX sequence counter
    uint8_t txSequence;

    // Status
    STM32Status status;

    // User callback
    PacketCallback userCallback;

    // Connection timeout (ms)
    static constexpr uint32_t CONNECTION_TIMEOUT = 10000;

    // Packet parsing timeout (ms)
    static constexpr uint32_t PARSE_TIMEOUT = 1000;

    // Last RX timestamp for timeout detection
    uint32_t lastRxTime;

    // Private methods
    bool parsePacket(uart_packet_t& packet);
    bool validatePacket(const uart_packet_t& packet) const;
    void handleParsedPacket(const uart_packet_t& packet);
    void updateStatus();

public:
    /**
     * @brief Constructor
     */
    STM32Communicator();

    /**
     * @brief Initialize UART communication
     * @param baudRate Baud rate (default 115200)
     * @return UARTError code
     */
    UARTError init(uint32_t baudRate = 115200);

    /**
     * @brief Send packet to STM32
     * @param packet Packet to send
     * @return UARTError code
     */
    UARTError sendPacket(const uart_packet_t& packet);

    /**
     * @brief Send command with payload
     * @param cmdType Command type
     * @param payload Payload data (can be nullptr)
     * @param length Payload length
     * @return UARTError code
     */
    UARTError sendCommand(uint8_t cmdType, const void* payload, uint16_t length);

    /**
     * @brief Send ACK response
     * @param sequence Sequence number to ACK
     * @param status Status code
     * @return UARTError code
     */
    UARTError sendAck(uint8_t sequence, uint8_t status);

    /**
     * @brief Handle UART communication
     * Must be called frequently in loop()
     */
    void handle();

    /**
     * @brief Set packet received callback
     */
    void setCallback(PacketCallback callback);

    /**
     * @brief Check if STM32 is connected
     * Based on recent heartbeat activity
     */
    bool isConnected() const;

    /**
     * @brief Get communication status
     */
    const STM32Status& getStatus() const { return status; }

    /**
     * @brief Get RX buffer usage
     */
    size_t getBufferUsage() const { return rxBuffer.available(); }

    /**
     * @brief Print buffer statistics (debug)
     */
    void printBufferStats() const {
        rxBuffer.printStats("STM32 RX Buffer");
    }

    /**
     * @brief Clear RX buffer
     */
    void clearBuffer() {
        rxBuffer.clear();
    }
};

/**
 * @brief Helper functions for common STM32 commands
 */
namespace STM32Commands {
    /**
     * @brief Request time data from ESP8266
     */
    inline uart_packet_t createGetTimeRequest(uint8_t sequence) {
        uart_packet_t packet;
        uart_init_packet(&packet, CMD_GET_TIME, sequence);
        packet.length = 0;
        return packet;
    }

    /**
     * @brief Request WiFi status
     */
    inline uart_packet_t createWiFiStatusRequest(uint8_t sequence) {
        uart_packet_t packet;
        uart_init_packet(&packet, CMD_WIFI_STATUS, sequence);
        packet.length = 0;
        return packet;
    }

    /**
     * @brief Request meter values
     */
    inline uart_packet_t createMeterValuesRequest(uint8_t sequence) {
        uart_packet_t packet;
        uart_init_packet(&packet, CMD_GET_METER_VALUES, sequence);
        packet.length = 0;
        return packet;
    }
}

#endif // STM32_COMM_H
