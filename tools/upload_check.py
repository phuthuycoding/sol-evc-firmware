#!/usr/bin/env python3
"""
PlatformIO post-build script for ESP8266
Validates build output and provides upload information
"""

Import("env")

import os
from os.path import join, getsize

def print_firmware_info(source, target, env):
    """Print firmware size and upload information after build"""
    print("\n" + "=" * 50)
    print("Build completed successfully!")
    print("=" * 50)

    # Get firmware path
    firmware_path = str(target[0])

    if os.path.exists(firmware_path):
        firmware_size = getsize(firmware_path)
        print(f"\nFirmware file: {firmware_path}")
        print(f"Firmware size: {firmware_size:,} bytes ({firmware_size / 1024:.2f} KB)")

        # Calculate flash usage (assuming 4MB flash)
        flash_size = 4 * 1024 * 1024  # 4MB
        usage_percent = (firmware_size / flash_size) * 100
        print(f"Flash usage: {usage_percent:.2f}%")

        # Check if firmware is too large
        max_firmware_size = 1024 * 1024  # 1MB max for OTA
        if firmware_size > max_firmware_size:
            print(f"\n⚠️  WARNING: Firmware size exceeds recommended OTA limit ({max_firmware_size / 1024:.0f} KB)")

        print("\n" + "=" * 50)
        print("Upload Options:")
        print("=" * 50)
        print("  • Serial: pio run -t upload")
        print("  • OTA:    pio run -t upload --upload-port <IP_ADDRESS>")
        print("=" * 50 + "\n")
    else:
        print(f"\n⚠️  WARNING: Firmware file not found at {firmware_path}\n")

# Register the callback for post-build
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", print_firmware_info)
