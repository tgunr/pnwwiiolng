#pragma once
//  Created by andrew wulf on 2/20/06.
//  Copyright 2006 PlayNet Inc -- All rights reserved.

#define SHOW_DEBUG(x)
#define DEBUGSTR1(x)   SHOW_DEBUG( fprintf( mDebugStream, x ); )
#define DEBUGSTR2(x,y)   SHOW_DEBUG( fprintf( mDebugStream, x, y ); )
#define DEBUGSTR3(x,y,z)   SHOW_DEBUG( fprintf( mDebugStream, x, y, z ); )
#define DEBUGSTR4(x,y,z,q)   SHOW_DEBUG( fprintf( mDebugStream, x, y, z, q ); )
#define DEBUGSTR5(x,y,z,q,t)   SHOW_DEBUG( fprintf( mDebugStream, x, y, z, q, t ); )
#include <Carbon/Carbon.h>
#include "sn/SoundBuffer.h"
#include "sn/sndsSoundControlAL.h"
#include "OpenAL/al.h"
#include "OpenAL/alc.h"
#include "OpenAL/MacOSX_OALExtensions.h"

#include <stdio.h>

#ifdef __cplusplus

#include <stdint.h>
#include <map>
#include <stack>
#include <algorithm>
#include <deque>

uint32_t SuperFastHash2 (const char * data, int len);

const static UInt32 EMPTY_BUS = 2147483647U;

struct lessthanstr
{
  bool operator()(const char* s, const char* t) const
  {
    return strcmp(s, t) < 0;
  }
};

typedef std::map<const char*, ALBuffer, lessthanstr> BufferMap;
typedef std::stack<ALSource> ALSourceStack;
typedef std::queue<ALSource> ALSourceQueue;
typedef std::deque<ALSource> ALSourceDeque;
typedef std::stack<ALBuffer> ALBufferStack;

#define MAX_AL_BUFFERS 1023
#define MAX_AL_SOURCES 128

enum
{
  ERROR_NO_DEVICE = 1000, // start of Mac OSX developer errors
  ERROR_NO_CONTEXT = 1001,
  ERROR_MISSING_FILE = 1002,
  ERROR_NO_MORE_BUFFERS = 1003,
  ERROR_BAD_SOUND = 1004,
  ERROR_NO_MORE_SOURCES = 1005
};

class OpenALAudio
{
public:
  OpenALAudio(FILE* debugStream, UINT32 maxSounds);
  OSStatus start();
  OSStatus kill();
  OSStatus init();

  // API -----------------

  OSStatus createSound(const snSound* oldSound, snSound* newSound);
  OSStatus duplicateSound(const snSound* oldSound, snSound* newSound);
  OSStatus freeSound(snSound* snd);
  LOGICAL isPlaying(snSound* snd);
  OSStatus loadSound(snSound* snd);
  OSStatus reloadSound(const snSound* oldSound, snSound* newSound);

  OSStatus setFrequency(snSound* snd);
  OSStatus setListener3DPos(piVECTOR* r);
  OSStatus setListener3DVel(piVECTOR* v);
  OSStatus setMaxDistance(snSound* snd);
  OSStatus setMinDistance(snSound* snd);
  OSStatus setOrientation(piVECTOR* at, piVECTOR* up);
  OSStatus setSound3DPos(snSound* snd);
  OSStatus setSound3DVel(snSound* snd);
  OSStatus setVolume(snSound* snd, FLOAT32 multiplier);
  OSStatus startSound(snSound* snd);
  OSStatus stopSound(snSound* snd);

private:

  void fixName(char*);
  void returnSource( snSound* snd );

  ALCcontext* mContext;
  ALCdevice* mDevice;

  FILE* mDebugStream;

  UINT32 mMaxSounds;
  ALBuffer mBufferArray[MAX_AL_BUFFERS];
  ALSource mSourceArray[MAX_AL_SOURCES];

//  ALSourceStack mAvailableSources; // unused sources
  ALSourceStack m2dAvailableSources;
  ALSourceStack m3dAvailableSources;
  ALBufferStack mAvailableBuffers;
  BufferMap mBufferIndex;

};
#endif
