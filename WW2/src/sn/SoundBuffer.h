#pragma once
/*
 *  SoundBuffer.h
 *  WW2OL
 *
 *  Created by andrew wulf on 11/28/05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 */

#include "OpenAL/al.h"

typedef ALuint ALSource;
typedef ALuint ALBuffer;

const ALSource ALSource_None = 9999999;
const ALBuffer ALBuffer_None = 9999999;

typedef struct SoundBuffer
{
  ALBuffer buffer;
  ALSource source;
} SoundBuffer;

