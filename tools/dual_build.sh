#!/bin/bash
# Dual Firmware Build Script
# Usage: ./dual_build.sh [build|flash|clean|all]

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_header() {
    echo -e "${BLUE}[DUAL-BUILD]${NC} $1"
}

build_both() {
    log_header "Building both STM32F103 and ESP8266 firmware..."
    
    # Create build directory
    mkdir -p "$PROJECT_DIR/build"
    
    # Build STM32F103 Master
    log_info "Building STM32F103 Master firmware..."
    if "$PROJECT_DIR/tools/flash_stm32.sh" build; then
        log_info "✓ STM32F103 build successful"
    else
        log_error "✗ STM32F103 build failed"
        exit 1
    fi
    
    echo ""
    
    # Build ESP8266 WiFi
    log_info "Building ESP8266 WiFi firmware..."
    if "$PROJECT_DIR/tools/flash_esp8266.sh" build; then
        log_info "✓ ESP8266 build successful"
    else
        log_error "✗ ESP8266 build failed"
        exit 1
    fi
    
    echo ""
    log_header "Both firmware builds completed successfully!"
    
    # Show build summary
    echo ""
    echo "=== BUILD SUMMARY ==="
    if [ -f "$PROJECT_DIR/build/stm32/firmware.bin" ]; then
        STM32_SIZE=$(stat -f%z "$PROJECT_DIR/build/stm32/firmware.bin" 2>/dev/null || stat -c%s "$PROJECT_DIR/build/stm32/firmware.bin" 2>/dev/null)
        echo "STM32F103 firmware: ${STM32_SIZE} bytes"
    fi
    
    if [ -f "$PROJECT_DIR/build/esp8266/firmware.bin" ]; then
        ESP8266_SIZE=$(stat -f%z "$PROJECT_DIR/build/esp8266/firmware.bin" 2>/dev/null || stat -c%s "$PROJECT_DIR/build/esp8266/firmware.bin" 2>/dev/null)
        echo "ESP8266 firmware:   ${ESP8266_SIZE} bytes"
    fi
    echo "===================="
}

flash_both() {
    log_header "Flashing both STM32F103 and ESP8266 firmware..."
    
    # Flash STM32F103 first
    log_info "Flashing STM32F103 Master firmware..."
    if "$PROJECT_DIR/tools/flash_stm32.sh" flash; then
        log_info "✓ STM32F103 flash successful"
    else
        log_error "✗ STM32F103 flash failed"
        exit 1
    fi
    
    echo ""
    
    # Flash ESP8266
    log_info "Flashing ESP8266 WiFi firmware..."
    if "$PROJECT_DIR/tools/flash_esp8266.sh" flash; then
        log_info "✓ ESP8266 flash successful"
    else
        log_error "✗ ESP8266 flash failed"
        exit 1
    fi
    
    echo ""
    log_header "Both firmware flashed successfully!"
    log_info "Device should be ready for operation"
}

clean_all() {
    log_header "Cleaning all build files..."
    
    "$PROJECT_DIR/tools/flash_stm32.sh" clean
    "$PROJECT_DIR/tools/flash_esp8266.sh" clean
    
    # Remove main build directory
    rm -rf "$PROJECT_DIR/build"
    
    log_header "All build files cleaned!"
}

show_status() {
    log_header "Firmware Status"
    
    echo ""
    echo "=== PROJECT STRUCTURE ==="
    echo "STM32F103 Master: $PROJECT_DIR/stm32-master/"
    echo "ESP8266 WiFi:     $PROJECT_DIR/esp8266-wifi/"
    echo "Shared Headers:   $PROJECT_DIR/shared/"
    echo "Build Tools:      $PROJECT_DIR/tools/"
    
    echo ""
    echo "=== BUILD STATUS ==="
    if [ -d "$PROJECT_DIR/build/stm32" ]; then
        echo "✓ STM32F103 build exists"
        ls -la "$PROJECT_DIR/build/stm32/"
    else
        echo "✗ STM32F103 not built"
    fi
    
    if [ -d "$PROJECT_DIR/build/esp8266" ]; then
        echo "✓ ESP8266 build exists"
        ls -la "$PROJECT_DIR/build/esp8266/"
    else
        echo "✗ ESP8266 not built"
    fi
    
    echo ""
    echo "=== DEPENDENCIES ==="
    if command -v pio &> /dev/null; then
        echo "✓ PlatformIO: $(pio --version)"
    else
        echo "✗ PlatformIO not found"
    fi
    
    if command -v st-flash &> /dev/null; then
        echo "✓ STLink tools: $(st-flash --version 2>&1 | head -1)"
    else
        echo "✗ STLink tools not found"
    fi
    
    if command -v node &> /dev/null; then
        echo "✓ Node.js: $(node --version)"
    else
        echo "✗ Node.js not found"
    fi
}

show_usage() {
    echo "EVSE Dual Firmware Build System"
    echo "Usage: $0 [build|flash|clean|status|all]"
    echo ""
    echo "Commands:"
    echo "  build   - Build both STM32F103 and ESP8266 firmware"
    echo "  flash   - Flash both firmware to devices"
    echo "  clean   - Clean all build files"
    echo "  status  - Show project and build status"
    echo "  all     - Build and flash both firmware"
    echo ""
    echo "Individual firmware commands:"
    echo "  STM32F103: ./tools/flash_stm32.sh [build|flash|monitor|clean]"
    echo "  ESP8266:   ./tools/flash_esp8266.sh [build|flash|ota|monitor|clean]"
    echo ""
    echo "Examples:"
    echo "  $0 build    # Build both firmware"
    echo "  $0 flash    # Flash both firmware"
    echo "  $0 all      # Build and flash everything"
}

# Main script
case "$1" in
    "build")
        build_both
        ;;
    "flash")
        flash_both
        ;;
    "clean")
        clean_all
        ;;
    "status")
        show_status
        ;;
    "all")
        build_both
        flash_both
        ;;
    *)
        show_usage
        exit 1
        ;;
esac
