/**
 * @file test_ntp_time.cpp
 * @brief Unit tests for NTPTimeDriver
 */

#include <unity.h>
#include "drivers/time/ntp_time.h"

NTPTimeDriver* ntpTime = nullptr;

void setUp(void) {
    ntpTime = new NTPTimeDriver();
}

void tearDown(void) {
    delete ntpTime;
    ntpTime = nullptr;
}

void test_ntp_initial_state(void) {
    // Assert
    TEST_ASSERT_FALSE(ntpTime->isSynced());
    TEST_ASSERT_EQUAL(0, ntpTime->getTimezoneOffset());
}

void test_ntp_init_sets_timezone(void) {
    // Act
    ntpTime->init("pool.ntp.org", 420);  // UTC+7 = 420 minutes

    // Assert
    TEST_ASSERT_EQUAL(420, ntpTime->getTimezoneOffset());
}

void test_ntp_unix_time_fallback_when_not_synced(void) {
    // Arrange
    ntpTime->init();

    // Act
    uint32_t time1 = ntpTime->getUnixTime();
    delay(100);
    uint32_t time2 = ntpTime->getUnixTime();

    // Assert
    // Should return millis-based time when not synced
    TEST_ASSERT_GREATER_OR_EQUAL(time1, time2);
}

void test_ntp_formatted_time_default(void) {
    // Arrange
    ntpTime->init();

    // Act
    String formatted = ntpTime->getFormattedTime();

    // Assert
    // Should return valid time format HH:MM:SS
    TEST_ASSERT_EQUAL(8, formatted.length());
    TEST_ASSERT_EQUAL(':', formatted.charAt(2));
    TEST_ASSERT_EQUAL(':', formatted.charAt(5));
}

void process(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ntp_initial_state);
    RUN_TEST(test_ntp_init_sets_timezone);
    RUN_TEST(test_ntp_unix_time_fallback_when_not_synced);
    RUN_TEST(test_ntp_formatted_time_default);

    UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
    delay(2000);
    process();
}
void loop() {}
#else
int main(int argc, char **argv) {
    process();
    return 0;
}
#endif
