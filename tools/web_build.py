#!/usr/bin/env python3
"""
PlatformIO pre-build script for ESP8266 web assets
1. Build Preact web UI
2. Compress files with gzip
3. Copy to data root for LittleFS upload
"""

Import("env")

import os
import gzip
import shutil
import subprocess
from os.path import join, isfile, dirname

def build_and_prepare_web_ui(source, target, env):
    """Build Web UI, compress, and prepare for LittleFS"""
    print("=" * 60)
    print("Building and preparing Web UI for LittleFS...")
    print("=" * 60)

    project_dir = env.subst("$PROJECT_DIR")
    web_ui_dir = join(project_dir, "web-ui")
    data_www_dir = join(project_dir, "data", "www")
    data_root_dir = join(project_dir, "data")

    # Step 1: Build Preact web UI
    if os.path.exists(web_ui_dir):
        print("\n[1/3] Building Preact web UI...")
        try:
            result = subprocess.run(
                ["npm", "run", "build"],
                cwd=web_ui_dir,
                capture_output=True,
                text=True,
                check=True
            )
            print("✓ Web UI built successfully")
        except subprocess.CalledProcessError as e:
            print(f"✗ Error building web UI: {e.stderr}")
            return

        # Copy dist to data/www
        dist_dir = join(web_ui_dir, "dist")
        if os.path.exists(dist_dir):
            if os.path.exists(data_www_dir):
                shutil.rmtree(data_www_dir)
            shutil.copytree(dist_dir, data_www_dir)
            print(f"✓ Copied build output to {data_www_dir}")
    else:
        print(f"\n[1/3] No web-ui directory found, skipping build...")

    # Step 2: Compress web files
    if not os.path.exists(data_www_dir):
        print(f"\n[2/3] No web source directory found at {data_www_dir}, skipping...")
        return

    print("\n[2/3] Compressing web files...")
    file_count = 0
    compressed_files = []

    for root, dirs, files in os.walk(data_www_dir):
        for filename in files:
            if filename.endswith(('.html', '.css', '.js', '.json')):
                src_path = join(root, filename)
                rel_path = os.path.relpath(src_path, data_www_dir)

                # Compress and save to data root with .gz extension
                dst_filename = filename + '.gz'
                dst_path = join(data_root_dir, dst_filename)

                # Compress file
                with open(src_path, 'rb') as f_in:
                    with gzip.open(dst_path, 'wb', compresslevel=9) as f_out:
                        shutil.copyfileobj(f_in, f_out)

                src_size = os.path.getsize(src_path)
                dst_size = os.path.getsize(dst_path)
                compression_ratio = (1 - dst_size / src_size) * 100

                print(f"  {filename} → {dst_filename}: {src_size} → {dst_size} bytes ({compression_ratio:.1f}% saved)")
                file_count += 1
                compressed_files.append(dst_filename)


    # Step 3: Summary
    print(f"\n[3/3] Summary:")
    print(f"  Compressed files: {file_count}")
    print(f"  Output directory: {data_root_dir}")
    print(f"  Files ready for LittleFS upload:")
    for f in compressed_files:
        file_path = join(data_root_dir, f)
        size = os.path.getsize(file_path)
        print(f"    - {f} ({size} bytes)")

    print("\n✓ Web UI ready for upload!")
    print("  Run: pio run --target uploadfs")
    print("=" * 60)
# Register the callback
env.AddPreAction("$BUILD_DIR/src/main.cpp.o", build_and_prepare_web_ui)
