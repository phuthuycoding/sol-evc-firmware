/**
 * @file ocpp_message_handler.cpp
 * @brief OCPP Message Handler Implementation
 */

#include "handlers/ocpp_message_handler.h"
#include "drivers/mqtt/mqtt_topic_builder.h"
#include "utils/logger.h"
#include <ArduinoJson.h>

// Status Notification
bool OCPPMessageHandler::publishStatusNotification(
    MQTTClient& mqtt,
    const DeviceConfig& config,
    const status_notification_t& status
) {
    // Build topic
    char topic[128];
    MQTTTopicBuilder::buildStatus(topic, sizeof(topic), config, status.connector_id);

    // Build JSON payload
    StaticJsonDocument<512> doc;
    doc["msgId"] = status.msg_id;
    doc["timestamp"] = status.timestamp;
    doc["connectorId"] = status.connector_id;
    doc["status"] = (int)status.status;
    doc["errorCode"] = (int)status.error_code;
    doc["info"] = status.info;
    doc["vendorId"] = status.vendor_id;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));

    // Publish
    MQTTError result = mqtt.publish(topic, payload, 1);

    if (result == MQTTError::SUCCESS) {
        LOG_DEBUG("OCPP", "Status published: connector=%d, status=%d", status.connector_id, status.status);
        return true;
    } else {
        LOG_ERROR("OCPP", "Status publish failed");
        return false;
    }
}

// Meter Values
bool OCPPMessageHandler::publishMeterValues(
    MQTTClient& mqtt,
    const DeviceConfig& config,
    const meter_values_t& meter
) {
    // Build topic
    char topic[128];
    MQTTTopicBuilder::buildMeter(topic, sizeof(topic), config, meter.connector_id);

    // Build JSON payload
    StaticJsonDocument<512> doc;
    doc["msgId"] = meter.msg_id;
    doc["timestamp"] = meter.timestamp;
    doc["connectorId"] = meter.connector_id;
    doc["transactionId"] = meter.transaction_id;

    JsonObject sample = doc.createNestedObject("sample");
    sample["energy_wh"] = meter.sample.energy_wh;
    sample["power_w"] = meter.sample.power_w;
    sample["voltage_v"] = meter.sample.voltage_v;
    sample["current_a"] = meter.sample.current_a;
    sample["frequency_hz"] = meter.sample.frequency_hz;
    sample["temperature_c"] = meter.sample.temperature_c;
    sample["power_factor_pct"] = meter.sample.power_factor_pct;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));

    // Publish
    MQTTError result = mqtt.publish(topic, payload, 1);

    if (result == MQTTError::SUCCESS) {
        LOG_DEBUG("OCPP", "Meter published: connector=%d, energy=%u Wh", meter.connector_id, meter.sample.energy_wh);
        return true;
    } else {
        LOG_ERROR("OCPP", "Meter publish failed");
        return false;
    }
}

// Start Transaction
bool OCPPMessageHandler::publishStartTransaction(
    MQTTClient& mqtt,
    const DeviceConfig& config,
    const start_transaction_t& txStart
) {
    // Build topic
    char topic[128];
    MQTTTopicBuilder::buildTransaction(topic, sizeof(topic), config, "start");

    // Build JSON payload
    StaticJsonDocument<512> doc;
    doc["msgId"] = txStart.msg_id;
    doc["timestamp"] = txStart.timestamp;
    doc["connectorId"] = txStart.connector_id;
    doc["idTag"] = txStart.id_tag;
    doc["meterStart"] = txStart.meter_start;
    doc["reservationId"] = txStart.reservation_id;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));

    // Publish
    MQTTError result = mqtt.publish(topic, payload, 1);

    if (result == MQTTError::SUCCESS) {
        LOG_INFO("OCPP", "Start TX published: connector=%d, tag=%s", txStart.connector_id, txStart.id_tag);
        return true;
    } else {
        LOG_ERROR("OCPP", "Start TX publish failed");
        return false;
    }
}

// Stop Transaction
bool OCPPMessageHandler::publishStopTransaction(
    MQTTClient& mqtt,
    const DeviceConfig& config,
    const stop_transaction_t& txStop
) {
    // Build topic
    char topic[128];
    MQTTTopicBuilder::buildTransaction(topic, sizeof(topic), config, "stop");

    // Build JSON payload
    StaticJsonDocument<512> doc;
    doc["msgId"] = txStop.msg_id;
    doc["timestamp"] = txStop.timestamp;
    doc["transactionId"] = txStop.transaction_id;
    doc["idTag"] = txStop.id_tag;
    doc["meterStop"] = txStop.meter_stop;
    doc["reason"] = txStop.reason;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));

    // Publish
    MQTTError result = mqtt.publish(topic, payload, 1);

    if (result == MQTTError::SUCCESS) {
        LOG_INFO("OCPP", "Stop TX published: txId=%u", txStop.transaction_id);
        return true;
    } else {
        LOG_ERROR("OCPP", "Stop TX publish failed");
        return false;
    }
}

// Boot Notification
bool OCPPMessageHandler::publishBootNotification(
    MQTTClient& mqtt,
    const DeviceConfig& config,
    const boot_notification_t& boot
) {
    // Build topic
    char topic[128];
    MQTTTopicBuilder::buildBoot(topic, sizeof(topic), config);

    // Build JSON payload
    StaticJsonDocument<512> doc;
    doc["msgId"] = boot.msg_id;
    doc["timestamp"] = boot.timestamp;
    doc["chargePointModel"] = boot.charge_point_model;
    doc["chargePointVendor"] = boot.charge_point_vendor;
    doc["firmwareVersion"] = boot.firmware_version;
    doc["chargePointSerialNumber"] = boot.charge_point_serial_number;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));

    // Publish
    MQTTError result = mqtt.publish(topic, payload, 1);

    if (result == MQTTError::SUCCESS) {
        LOG_INFO("OCPP", "Boot notification published");
        return true;
    } else {
        LOG_ERROR("OCPP", "Boot notification failed");
        return false;
    }
}

// Topic builders removed - now using MQTTTopicBuilder namespace
