#pragma once

//! @file MoreGetRoutines.h
//! @brief Routines for fetching Hardware Survey data.
//! Functions for getting computer hardware specs: cpu, ram, sound,
//! network, and graphics/opengl/direct caps.

//! Fetch the overall computer info structure.
extern void GetComputerInfo(HINSTANCE hInstance, struct PLAY_COMPUTER_INFO* info, BOOL Messages);

//extern void GetMemory(struct PLAY_MEM* meminfo);
//extern void GetSoundDevice(struct PLAY_SOUND* sound);
//extern void GetNetInfo(struct PLAY_NET_ADAPTERS* adapters);

extern BOOL VerifyDirectXFileVersion(struct PLAY_DX_VERS* vers, DWORD MinProductVersionMS, DWORD MinProductVersionLS, BOOL Messages);
extern BOOL VerifyDirectXVersion(BOOL Messages);
