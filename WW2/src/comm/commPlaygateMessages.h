#pragma once

#include "teul/teulEnums.h"
#include "teul/teulBasic.h"

// Initialize Playgate's comm functions (network transport).
extern bool commInit(void);
// Install a particular callback.
extern TEUL_CALLBACK* commInstallMessage(UCHAR MessageGroup,
                                            UCHAR MessageNumber,
                                            ACTION* SendFunc,
                                            ACTION* ReceiveFunc);
// Process network updates
extern bool commUpdate(void);
// Shutdown teulKit.
extern void commKill(void);

// Connect to an auth host.
extern bool commOpenConnection(UINT32 HostNumber, UINT16 Port);
// Close the auth host connection.
extern void commCloseConnection();
// Send data via a callback to the authHost.
extern SINT32 commSend(struct TEUL_CALLBACK* object, void*  data);
