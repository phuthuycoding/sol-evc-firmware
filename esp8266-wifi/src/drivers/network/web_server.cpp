/**
 * @file web_server.cpp
 * @brief WebServer Driver Implementation
 * @version 1.0.0 - LittleFS filesystem serving
 */

#include "drivers/network/web_server.h"
#include <LittleFS.h>

WebServerDriver::WebServerDriver(uint16_t serverPort)
    : server(serverPort), initialized(false), port(serverPort) {
}

WebServerDriver::~WebServerDriver() {
    stop();
}

bool WebServerDriver::init() {
    if (initialized) {
        LOG_WARN("WebServer", "Already initialized");
        return true;
    }

    if (!LittleFS.begin()) {
        LOG_ERROR("WebServer", "Failed to mount LittleFS");
        return false;
    }

    LOG_INFO("WebServer", "LittleFS mounted successfully");
    initialized = true;
    return true;
}

bool WebServerDriver::start() {
    if (!initialized) {
        LOG_ERROR("WebServer", "Not initialized");
        return false;
    }

    // Serve static files from LittleFS /data directory
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // 404 handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        LOG_WARN("WebServer", "404: %s", request->url().c_str());
        request->send(404, "text/plain", "Not Found");
    });

    server.begin();
    LOG_INFO("WebServer", "Server started on port %d", port);
    return true;
}

void WebServerDriver::stop() {
    if (initialized) {
        server.end();
        LittleFS.end();
        LOG_INFO("WebServer", "Server stopped");
    }
}

void WebServerDriver::setCORSHeaders(AsyncWebServerResponse* response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void WebServerDriver::onGet(const char* uri, ArRequestHandlerFunction handler) {
    server.on(uri, HTTP_GET, handler);
    LOG_DEBUG("WebServer", "Registered GET %s", uri);
}

void WebServerDriver::onPost(const char* uri, ArRequestHandlerFunction handler) {
    server.on(uri, HTTP_POST, handler);
    LOG_DEBUG("WebServer", "Registered POST %s", uri);
}

void WebServerDriver::onPostWithBody(const char* uri,
                                      ArRequestHandlerFunction handler,
                                      ArBodyHandlerFunction bodyHandler) {
    server.on(uri, HTTP_POST, handler, NULL, bodyHandler);
    LOG_DEBUG("WebServer", "Registered POST with body %s", uri);
}

void WebServerDriver::serveStatic(const char* uri, const char* fsPath) {
    server.serveStatic(uri, LittleFS, fsPath);
    LOG_DEBUG("WebServer", "Registered static %s -> %s", uri, fsPath);
}
