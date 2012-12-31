#pragma once

// Copyright 2008 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

struct CHAR_INPUT
{
	UINT32 uChar;
	UINT32 uIac;
	UINT32 uVK;
	UINT32 uScanCode;

	LOGICAL bState;

};

#include "imap/imapTypes.h"
#include "imap/iac.h"

#define NUM_IAC_QUEUE_ENTRIES		256
#define _DIMAIN_INPUT_BUFFERSIZE	256
#define NUM_SLIDERS_PER_JOYSTICK	2

	extern LOGICAL dimainInit(void);
	extern void dimainKill(void);
	extern LOGICAL dimainGetInputs(imap_queue_entry** plpiac, UINT32* pnumentries);

	extern UINT32 dimainVKToIAC(UINT32 uVK);

	extern void dimainGetTextInputs(struct CHAR_INPUT** lpBuf, SINT32* pNumElements);
	extern bool dimainAddChar(UINT32 ucChar);
	extern bool dimainVKUp(UINT32 uVK, UINT32 uScanCode);
	extern bool dimainVKDown(UINT32 uVK, UINT32 uScanCode);
