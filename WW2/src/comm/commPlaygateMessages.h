#pragma once

#include "teul/teulEnums.h"
#include "teul/teulBasic.h"

// Initialize Playgate's comm functions (network transport).
bool commInit(void);
// Install a particular callback.
TEUL_CALLBACK* commInstallMessage(UCHAR MessageGroup,
                                            UCHAR MessageNumber,
                                            ACTION* SendFunc,
                                            ACTION* ReceiveFunc);
// Process network updates
bool commUpdate(void);
// Shutdown teulKit.
void commKill(void);

// Connect to an auth host.
bool commOpenConnection(UINT32 HostNumber, UINT16 Port);
// Close the auth host connection.
void commCloseConnection();
// Send data via a callback to the authHost.
SINT32 commSend(struct TEUL_CALLBACK* object, void*  data);
