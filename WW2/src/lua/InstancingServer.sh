#! /bin/bash
######################################################################
# Execute the Lua instancing server.

SCENARIO_PATH="${HOME}/scenarios"

cd "${SCENARIO_PATH}" || exit "Cannot access scenario path."

# For a quick svn update.
svn update

# Try running the script.
LUA_PATH="${SCENARIO_PATH}/?;${SCENARIO_PATH}/?.lua;${LUA_PATH}" lua InstancingServer.lua

