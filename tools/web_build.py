#!/usr/bin/env python3
"""
PlatformIO pre-build script for ESP8266 web assets
Processes web interface files before building firmware
"""

Import("env")

import os
import gzip
import shutil
from os.path import join, isfile, dirname

def compress_web_files(source, target, env):
    """Compress web assets for embedding in firmware"""
    print("=" * 50)
    print("Processing web assets...")
    print("=" * 50)

    web_src_dir = join(env.subst("$PROJECT_DIR"), "data", "www")
    web_build_dir = join(env.subst("$PROJECT_DIR"), "data", "www_compressed")

    # Create build directory if it doesn't exist
    if not os.path.exists(web_build_dir):
        os.makedirs(web_build_dir)
        print(f"Created directory: {web_build_dir}")

    # If no web source directory, skip
    if not os.path.exists(web_src_dir):
        print(f"No web source directory found at {web_src_dir}, skipping...")
        return

    # Compress web files
    file_count = 0
    for root, dirs, files in os.walk(web_src_dir):
        for filename in files:
            if filename.endswith(('.html', '.css', '.js', '.json')):
                src_path = join(root, filename)
                rel_path = os.path.relpath(src_path, web_src_dir)
                dst_path = join(web_build_dir, rel_path + '.gz')

                # Create subdirectories if needed
                os.makedirs(dirname(dst_path), exist_ok=True)

                # Compress file
                with open(src_path, 'rb') as f_in:
                    with gzip.open(dst_path, 'wb') as f_out:
                        shutil.copyfileobj(f_in, f_out)

                src_size = os.path.getsize(src_path)
                dst_size = os.path.getsize(dst_path)
                compression_ratio = (1 - dst_size / src_size) * 100

                print(f"  {rel_path}: {src_size} -> {dst_size} bytes ({compression_ratio:.1f}% reduction)")
                file_count += 1

    if file_count > 0:
        print(f"\nCompressed {file_count} web file(s)")
    else:
        print("No web files to compress")

    print("=" * 50)

# Register the callback
env.AddPreAction("$BUILD_DIR/src/main.cpp.o", compress_web_files)
