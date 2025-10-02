/**
 * @file mock_stm32_comm.h
 * @brief Mock STM32 Communicator for testing
 */

#ifndef MOCK_STM32_COMM_H
#define MOCK_STM32_COMM_H

#include "../../shared/uart_protocol.h"
#include <Arduino.h>
#include <string.h>

/**
 * @brief Mock STM32 Communicator
 */
class MockSTM32Communicator {
private:
    bool ackSent;
    uint8_t lastSequence;
    uint8_t lastStatus;
    uart_packet_t lastPacket;
    bool packetSent;

public:
    MockSTM32Communicator() : ackSent(false), lastSequence(0), lastStatus(0), packetSent(false) {
        memset(&lastPacket, 0, sizeof(lastPacket));
    }

    void sendAck(uint8_t sequence, uint8_t status) {
        ackSent = true;
        lastSequence = sequence;
        lastStatus = status;
    }

    void sendPacket(const uart_packet_t& packet) {
        packetSent = true;
        memcpy(&lastPacket, &packet, sizeof(uart_packet_t));
    }

    bool wasAckSent() const { return ackSent; }
    uint8_t getLastSequence() const { return lastSequence; }
    uint8_t getLastStatus() const { return lastStatus; }

    bool wasPacketSent() const { return packetSent; }
    const uart_packet_t& getLastPacket() const { return lastPacket; }

    void reset() {
        ackSent = false;
        packetSent = false;
        lastSequence = 0;
        lastStatus = 0;
        memset(&lastPacket, 0, sizeof(lastPacket));
    }
};

#endif // MOCK_STM32_COMM_H
