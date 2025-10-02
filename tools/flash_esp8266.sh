#!/bin/bash

# ESP8266 WiFi Module Build and Flash Script
# Usage: ./flash_esp8266.sh [build|flash|monitor|clean|ota|web]

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ESP8266_DIR="$PROJECT_DIR/esp8266-wifi"
LOG_FILE="$PROJECT_DIR/logs/esp8266_flash.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create logs directory
mkdir -p "$PROJECT_DIR/logs"

# Function to log messages
log() {
    echo -e "${GREEN}[$(date '+%Y-%m-%d %H:%M:%S')] $1${NC}" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}[ERROR] $1${NC}" | tee -a "$LOG_FILE"
}

warn() {
    echo -e "${YELLOW}[WARNING] $1${NC}" | tee -a "$LOG_FILE"
}

info() {
    echo -e "${BLUE}[INFO] $1${NC}" | tee -a "$LOG_FILE"
}

# Check dependencies
check_dependencies() {
    log "Checking dependencies..."
    
    if ! command -v pio &> /dev/null; then
        error "PlatformIO CLI not found. Please install PlatformIO."
        exit 1
    fi
    
    if ! command -v python3 &> /dev/null; then
        error "Python 3 not found. Please install Python 3."
        exit 1
    fi
    
    log "Dependencies check passed"
}

# Build ESP8266 firmware
build_firmware() {
    log "Building ESP8266 firmware..."
    cd "$ESP8266_DIR"
    
    if pio run; then
        log "ESP8266 firmware build successful"
    else
        error "ESP8266 firmware build failed"
        exit 1
    fi
}

# Build web interface files
build_web_interface() {
    log "Building web interface..."
    cd "$ESP8266_DIR"
    
    # Ensure data directory exists
    mkdir -p data
    
    # Build filesystem image
    if pio run --target buildfs; then
        log "Web interface build successful"
    else
        warn "Web interface build failed, continuing without web files"
    fi
}

# Flash ESP8266 firmware
flash_firmware() {
    log "Flashing ESP8266 firmware..."
    cd "$ESP8266_DIR"
    
    if pio run --target upload; then
        log "ESP8266 firmware flash successful"
    else
        error "ESP8266 firmware flash failed"
        exit 1
    fi
}

# Flash filesystem (web interface)
flash_filesystem() {
    log "Flashing filesystem..."
    cd "$ESP8266_DIR"
    
    if pio run --target uploadfs; then
        log "Filesystem flash successful"
    else
        warn "Filesystem flash failed, web interface may not work"
    fi
}

# Monitor serial output
monitor_serial() {
    log "Starting serial monitor..."
    cd "$ESP8266_DIR"
    pio device monitor
}

# Clean build files
clean_build() {
    log "Cleaning ESP8266 build files..."
    log_info "Cleaning ESP8266 build files..."
    cd "$ESP8266_DIR"
    pio run --target clean
    rm -rf .pio/
    rm -rf node_modules/
    rm -rf dist/
    rm -rf "$PROJECT_DIR/build/esp8266"
    log_info "Clean complete!"
}

show_device_info() {
    log_info "ESP8266 Device Information:"
    cd "$ESP8266_DIR"
    pio device list
    echo ""
    log_info "Available OTA devices:"
    if command -v avahi-browse &> /dev/null; then
        avahi-browse -t _arduino._tcp
    elif command -v dns-sd &> /dev/null; then
        dns-sd -B _arduino._tcp
    else
        log_warn "No mDNS browser found. Install avahi-utils or use macOS dns-sd"
    fi
}

show_usage() {
    echo "Usage: $0 [build|flash|monitor|clean|ota|info|all]"
    echo ""
    echo "Commands:"
    echo "  build   - Build the firmware and web interface"
    echo "  flash   - Flash the firmware to device via USB"
    echo "  ota     - Update firmware via OTA (WiFi)"
    echo "  monitor - Start serial monitor"
    echo "  clean   - Clean build files"
    echo "  info    - Show device information"
    echo "  all     - Build and flash via USB"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 flash"
    echo "  $0 ota"
    echo "  $0 all"
}

# Main script
case "$1" in
    "build")
        check_dependencies
        build_firmware
        ;;
    "flash")
        check_dependencies
        flash_firmware
        ;;
    "ota")
        check_dependencies
        ota_update
        ;;
    "monitor")
        check_dependencies
        monitor_serial
        ;;
    "clean")
        clean_build
        ;;
    "info")
        show_device_info
        ;;
    "all")
        check_dependencies
        build_firmware
        flash_firmware
        ;;
    *)
        show_usage
        exit 1
        ;;
esac
