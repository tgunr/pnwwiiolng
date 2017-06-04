//
//  authDefaults.h
//  WW2OL
//
//  Created by Gwynne Raskind on 6/3/17.
//  Copyright © 2017 Cornered Rat Software - All rights reserved.
//
//
#pragma once

#include "wwii/wwiiNetPorts.h"

#ifndef NDEBUG
#define DEFAULT_AUTH_SERVER "authdev.playnet.com"
#else
#define DEFAULT_AUTH_SERVER "auth.playnet.com"
#endif

#define DEFAULT_AUTH_PORT WWII::Net::Ports::AuthHost
