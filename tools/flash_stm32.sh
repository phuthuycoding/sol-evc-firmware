#!/bin/bash
# STM32F103 Flash Script
# Usage: ./flash_stm32.sh [build|flash|monitor|clean]

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STM32_DIR="$PROJECT_DIR/stm32-master"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
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

check_dependencies() {
    log_info "Checking dependencies..."
    
    if ! command -v pio &> /dev/null; then
        log_error "PlatformIO not found. Please install: pip install platformio"
        exit 1
    fi
    
    if ! command -v st-flash &> /dev/null; then
        log_warn "st-flash not found. Install stlink tools for direct flashing"
    fi
}

build_firmware() {
    log_info "Building STM32F103 firmware..."
    cd "$STM32_DIR"
    
    # Clean previous build
    pio run --target clean
    
    # Build firmware
    pio run
    
    if [ $? -eq 0 ]; then
        log_info "Build successful!"
        
        # Show memory usage
        pio run --target size
        
        # Copy binaries to output directory
        mkdir -p "$PROJECT_DIR/build/stm32"
        cp .pio/build/stm32f103c8/firmware.bin "$PROJECT_DIR/build/stm32/"
        cp .pio/build/stm32f103c8/firmware.elf "$PROJECT_DIR/build/stm32/"
        
        log_info "Binaries copied to build/stm32/"
    else
        log_error "Build failed!"
        exit 1
    fi
}

flash_firmware() {
    log_info "Flashing STM32F103 firmware..."
    cd "$STM32_DIR"
    
    # Try PlatformIO upload first
    if pio run --target upload; then
        log_info "Flash successful via PlatformIO!"
    elif command -v st-flash &> /dev/null; then
        log_warn "PlatformIO upload failed, trying st-flash..."
        st-flash write .pio/build/stm32f103c8/firmware.bin 0x8000000
        if [ $? -eq 0 ]; then
            log_info "Flash successful via st-flash!"
        else
            log_error "Flash failed!"
            exit 1
        fi
    else
        log_error "Flash failed and no alternative method available!"
        exit 1
    fi
}

monitor_serial() {
    log_info "Starting serial monitor..."
    cd "$STM32_DIR"
    pio device monitor --baud 115200
}

clean_build() {
    log_info "Cleaning STM32F103 build files..."
    cd "$STM32_DIR"
    pio run --target clean
    rm -rf .pio/
    rm -rf "$PROJECT_DIR/build/stm32"
    log_info "Clean complete!"
}

show_usage() {
    echo "Usage: $0 [build|flash|monitor|clean|all]"
    echo ""
    echo "Commands:"
    echo "  build   - Build the firmware"
    echo "  flash   - Flash the firmware to device"
    echo "  monitor - Start serial monitor"
    echo "  clean   - Clean build files"
    echo "  all     - Build and flash"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 flash"
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
    "monitor")
        check_dependencies
        monitor_serial
        ;;
    "clean")
        clean_build
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
