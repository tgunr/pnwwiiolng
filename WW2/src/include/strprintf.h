#pragma once
// Copyright (C) PlayNet, Inc 2016
// Originating Programmer: Oliver "kfsone" Smith

// snprintf wrapper
template<size_t bufSize, typename... Args>
UINT32 strprintf(char(&buf)[bufSize], const char* fmt, Args&&... args)
{
    return static_cast<UINT32>(snprintf(buf, bufSize, fmt, std::forward<Args>(args)...));
}
template<size_t bufSize> UINT32 strprintf(char(&buf)[bufSize], const char* text) { return strprintf(buf, "%s", text); }
