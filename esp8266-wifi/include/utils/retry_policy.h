/**
 * @file retry_policy.h
 * @brief Retry policies for ESP8266
 * @version 1.0.0
 */

#ifndef RETRY_POLICY_H
#define RETRY_POLICY_H

#include <Arduino.h>

/**
 * @brief Retry policy interface
 */
class IRetryPolicy {
public:
    virtual ~IRetryPolicy() = default;
    virtual uint32_t getNextDelay(uint8_t attemptCount) = 0;
    virtual bool shouldRetry(uint8_t attemptCount) = 0;
    virtual void reset() = 0;
};

/**
 * @brief Exponential backoff retry policy
 */
class ExponentialBackoff : public IRetryPolicy {
private:
    uint32_t initialDelay;
    uint32_t maxDelay;
    uint8_t maxAttempts;

public:
    ExponentialBackoff(uint32_t initial = 1000, uint32_t max = 60000, uint8_t attempts = 5)
        : initialDelay(initial), maxDelay(max), maxAttempts(attempts) {}

    uint32_t getNextDelay(uint8_t attemptCount) override {
        uint32_t delay = initialDelay * (1 << attemptCount);
        return min(delay, maxDelay);
    }

    bool shouldRetry(uint8_t attemptCount) override {
        return attemptCount < maxAttempts;
    }

    void reset() override {}
};

/**
 * @brief Fixed delay retry policy
 */
class FixedDelay : public IRetryPolicy {
private:
    uint32_t delay;
    uint8_t maxAttempts;

public:
    FixedDelay(uint32_t d = 5000, uint8_t attempts = 3)
        : delay(d), maxAttempts(attempts) {}

    uint32_t getNextDelay(uint8_t attemptCount) override {
        return delay;
    }

    bool shouldRetry(uint8_t attemptCount) override {
        return attemptCount < maxAttempts;
    }

    void reset() override {}
};

#endif // RETRY_POLICY_H
