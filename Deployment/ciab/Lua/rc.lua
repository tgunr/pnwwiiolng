--[[
 This is the primary startup script for the Lua environment;
 any additional Lua modules that should be loaded on startup
 across all processes should be included here.

 DO NOT include any host or language specific code here.

 For China specific code, see rc-zh.lua
]]--

if not events then events = {} end

require 'glue'				-- Glue functions.
require 'localization'		-- For string localization routines
require 'event-queue'		-- Event handling system

