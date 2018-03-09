
#pragma once

#ifndef __PLAYGATE_SYSCFG_H__
#define __PLAYGATE_SYSCFG_H__ 1

//  ============================
//
//  Playgate.exe - syscfg.h
//
//  ============================

#define AUTH_CLIENT
#define platEndian platLittleEndian
#define platBuild TEUL_CLIENT
#define OPENGL3D

#include "stdafx.h"

#include "ww2src\PG_platforms.h"


//	------------------------------	Public Definitions

#include <stdexcept>
#include <cwchar>

#include "dbug/dbug.h"
#include "dbug/dbugExtErrors.h"

#include "wwii/wwiiOwnershipEnums.h"
#include "chat/chatHandle.h"
#include "wwii/wwiiCovert.h"
#include "weap/weapDefs.h"
#include "weap/weapSpec.h"
#include "teul/teulBasic.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulPackers.h"
#include "teul/teulTime.h"
#include "teul/teulKit.h"
#include "inst/instDescriptor.h"

#include "auth/authPlaygateTypes.h"
namespace Auth { extern GameList g_games ; }

#include "auth/authTypes.h"
#include "auth/authPlaygateTypes.h"

#include "main.h"
#include "registry.h"

#endif // __PLAYGATE_SYSCFG_H__