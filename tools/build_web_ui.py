#!/usr/bin/env python3
"""
Build Web UI and prepare for ESP8266
"""

Import("env")

import os
import subprocess
import shutil
from os.path import join

def build_and_copy_web_ui(source, target, env):
    """Build Preact web UI and copy to data folder"""
    print("=" * 50)
    print("Building Web UI...")
    print("=" * 50)

    web_ui_dir = join(env.subst("$PROJECT_DIR"), "web-ui")
    data_dir = join(env.subst("$PROJECT_DIR"), "data", "www")

    # Check if web-ui exists
    if not os.path.exists(web_ui_dir):
        print(f"No web-ui directory found at {web_ui_dir}, skipping...")
        return

    # Build web UI
    print(f"Building Preact app in {web_ui_dir}...")
    try:
        result = subprocess.run(
            ["npm", "run", "build"],
            cwd=web_ui_dir,
            capture_output=True,
            text=True,
            check=True
        )
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"Error building web UI: {e.stderr}")
        return

    # Copy to data/www
    dist_dir = join(web_ui_dir, "dist")
    if os.path.exists(dist_dir):
        print(f"Copying web UI from {dist_dir} to {data_dir}...")

        # Create data/www if not exists
        os.makedirs(data_dir, exist_ok=True)

        # Remove old files
        if os.path.exists(data_dir):
            shutil.rmtree(data_dir)

        # Copy new files
        shutil.copytree(dist_dir, data_dir)

        # List files
        print("Files in data/www:")
        for file in os.listdir(data_dir):
            file_path = join(data_dir, file)
            size = os.path.getsize(file_path)
            print(f"  {file}: {size} bytes")

        print("✓ Web UI built and copied successfully")
    else:
        print(f"No dist directory found at {dist_dir}")

    print("=" * 50)

# Register the callback (runs before web_build.py compression)
env.AddPreAction("$BUILD_DIR/src/main.cpp.o", build_and_copy_web_ui)
