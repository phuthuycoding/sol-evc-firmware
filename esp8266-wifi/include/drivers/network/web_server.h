/**
 * @file web_server.h
 * @brief WebServer Driver (thin wrapper around ESPAsyncWebServer)
 * @version 1.0.0
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "utils/logger.h"

/**
 * @brief WebServer driver (thin wrapper)
 *
 * Serves static files from LittleFS and provides API route registration
 */
class WebServerDriver {
private:
    AsyncWebServer server;
    bool initialized;
    uint16_t port;

    // CORS headers helper
    void setCORSHeaders(AsyncWebServerResponse* response);

public:
    explicit WebServerDriver(uint16_t serverPort = 80);
    ~WebServerDriver();

    /**
     * @brief Initialize web server and mount filesystem
     */
    bool init();

    /**
     * @brief Start serving requests
     */
    bool start();

    /**
     * @brief Stop server
     */
    void stop();

    /**
     * @brief Register API route (GET)
     */
    void onGet(const char* uri, ArRequestHandlerFunction handler);

    /**
     * @brief Register API route (POST)
     */
    void onPost(const char* uri, ArRequestHandlerFunction handler);

    /**
     * @brief Register API route (POST with body)
     */
    void onPostWithBody(const char* uri,
                        ArRequestHandlerFunction handler,
                        ArBodyHandlerFunction bodyHandler);

    /**
     * @brief Serve static files from LittleFS
     */
    void serveStatic(const char* uri, const char* fsPath);

    /**
     * @brief Get server instance (for advanced usage)
     */
    AsyncWebServer& getServer() { return server; }

    // Prevent copying
    WebServerDriver(const WebServerDriver&) = delete;
    WebServerDriver& operator=(const WebServerDriver&) = delete;
};

#endif // WEB_SERVER_H
