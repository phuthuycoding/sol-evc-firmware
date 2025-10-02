/**
 * @file ring_buffer.h
 * @brief Lightweight ring buffer for ESP8266 (stack allocated)
 * @version 1.0.0
 *
 * Features:
 * - Fixed size, no dynamic allocation
 * - Optimized for UART data buffering
 * - Overflow detection
 * - Statistics tracking
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <Arduino.h>

/**
 * @brief Fixed-size ring buffer for bytes
 *
 * Usage:
 *   RingBuffer<512> buffer;
 *   buffer.push(0x42);
 *   uint8_t data;
 *   if (buffer.pop(data)) { ... }
 */
template<size_t CAPACITY>
class RingBuffer {
private:
    uint8_t buffer[CAPACITY];
    size_t head;
    size_t tail;
    size_t count;

    // Statistics
    uint32_t totalPushed;
    uint32_t totalPopped;
    uint32_t overflowCount;
    size_t peakUsage;

public:
    /**
     * @brief Constructor
     */
    RingBuffer() : head(0), tail(0), count(0),
                   totalPushed(0), totalPopped(0),
                   overflowCount(0), peakUsage(0) {}

    /**
     * @brief Push single byte
     * @param byte Byte to push
     * @return true if pushed, false if full
     */
    bool push(uint8_t byte) {
        if (count >= CAPACITY) {
            overflowCount++;
            return false;
        }

        buffer[head] = byte;
        head = (head + 1) % CAPACITY;
        count++;
        totalPushed++;

        // Update peak usage
        if (count > peakUsage) {
            peakUsage = count;
        }

        return true;
    }

    /**
     * @brief Pop single byte
     * @param byte Output byte
     * @return true if popped, false if empty
     */
    bool pop(uint8_t& byte) {
        if (count == 0) {
            return false;
        }

        byte = buffer[tail];
        tail = (tail + 1) % CAPACITY;
        count--;
        totalPopped++;

        return true;
    }

    /**
     * @brief Peek at next byte without removing
     * @param byte Output byte
     * @return true if available, false if empty
     */
    bool peek(uint8_t& byte) const {
        if (count == 0) {
            return false;
        }

        byte = buffer[tail];
        return true;
    }

    /**
     * @brief Peek at byte at offset from tail
     * @param offset Offset from tail (0 = next byte)
     * @param byte Output byte
     * @return true if available
     */
    bool peekAt(size_t offset, uint8_t& byte) const {
        if (offset >= count) {
            return false;
        }

        size_t index = (tail + offset) % CAPACITY;
        byte = buffer[index];
        return true;
    }

    /**
     * @brief Push multiple bytes
     * @param data Data pointer
     * @param length Number of bytes
     * @return Number of bytes actually pushed
     */
    size_t pushMultiple(const uint8_t* data, size_t length) {
        size_t pushed = 0;
        for (size_t i = 0; i < length; i++) {
            if (!push(data[i])) {
                break;
            }
            pushed++;
        }
        return pushed;
    }

    /**
     * @brief Pop multiple bytes
     * @param data Output buffer
     * @param maxLength Maximum bytes to pop
     * @return Number of bytes actually popped
     */
    size_t popMultiple(uint8_t* data, size_t maxLength) {
        size_t popped = 0;
        for (size_t i = 0; i < maxLength; i++) {
            if (!pop(data[i])) {
                break;
            }
            popped++;
        }
        return popped;
    }

    /**
     * @brief Get number of available bytes
     */
    size_t available() const {
        return count;
    }

    /**
     * @brief Get free space
     */
    size_t free() const {
        return CAPACITY - count;
    }

    /**
     * @brief Check if empty
     */
    bool isEmpty() const {
        return count == 0;
    }

    /**
     * @brief Check if full
     */
    bool isFull() const {
        return count >= CAPACITY;
    }

    /**
     * @brief Get capacity
     */
    size_t capacity() const {
        return CAPACITY;
    }

    /**
     * @brief Clear buffer
     */
    void clear() {
        head = tail = count = 0;
    }

    /**
     * @brief Reset statistics
     */
    void resetStats() {
        totalPushed = 0;
        totalPopped = 0;
        overflowCount = 0;
        peakUsage = 0;
    }

    /**
     * @brief Get total bytes pushed
     */
    uint32_t getTotalPushed() const { return totalPushed; }

    /**
     * @brief Get total bytes popped
     */
    uint32_t getTotalPopped() const { return totalPopped; }

    /**
     * @brief Get overflow count
     */
    uint32_t getOverflowCount() const { return overflowCount; }

    /**
     * @brief Get peak usage
     */
    size_t getPeakUsage() const { return peakUsage; }

    /**
     * @brief Get usage percentage (0-100)
     */
    uint8_t getUsagePercent() const {
        return (count * 100) / CAPACITY;
    }

    /**
     * @brief Print statistics (debug)
     */
    void printStats(const char* name = "RingBuffer") const {
        Serial.printf("[%s] Stats:\n", name);
        Serial.printf("  Capacity: %u bytes\n", CAPACITY);
        Serial.printf("  Available: %u bytes (%u%%)\n", count, getUsagePercent());
        Serial.printf("  Peak usage: %u bytes (%u%%)\n", peakUsage, (peakUsage * 100) / CAPACITY);
        Serial.printf("  Total pushed: %u\n", totalPushed);
        Serial.printf("  Total popped: %u\n", totalPopped);
        Serial.printf("  Overflows: %u\n", overflowCount);
    }

    /**
     * @brief Search for pattern in buffer
     * @param pattern Pattern to search
     * @param patternLen Pattern length
     * @return Offset if found, -1 if not found
     */
    int findPattern(const uint8_t* pattern, size_t patternLen) const {
        if (patternLen == 0 || patternLen > count) {
            return -1;
        }

        for (size_t i = 0; i <= count - patternLen; i++) {
            bool match = true;
            for (size_t j = 0; j < patternLen; j++) {
                uint8_t byte;
                if (!peekAt(i + j, byte) || byte != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return (int)i;
            }
        }

        return -1;
    }

    /**
     * @brief Discard bytes from head
     * @param numBytes Number of bytes to discard
     * @return Number of bytes actually discarded
     */
    size_t discard(size_t numBytes) {
        if (numBytes > count) {
            numBytes = count;
        }

        tail = (tail + numBytes) % CAPACITY;
        count -= numBytes;
        totalPopped += numBytes;

        return numBytes;
    }
};

#endif // RING_BUFFER_H
