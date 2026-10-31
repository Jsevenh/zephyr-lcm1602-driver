#!/bin/bash
# Wrapper script to run west commands out of Zephyr workbench environment
# This script is auto-generated -- do not edit

# Set environment variables
export ZEPHYR_BASE="/home/mbayi/zephyrproject/zephyr"
export ZEPHYR_PROJECT_DIRECTORY="/home/mbayi/zephyrproject"
export ZEPHYR_TOOLCHAIN_VARIANT="zephyr"


# Source environment and execute West
. /home/mbayi/.zinstaller/env.sh && west "$@"
