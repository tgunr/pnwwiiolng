/*
 *  WW2OL
 *
 *  Created by andrew wulf on 2/20/06.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#include "sn/OpenALAudio.h"
#include "fio/fio.h"
#include "mem/mem.h"
#include "include/vars.h"
#include <math.h>
#include "sn/sndsSoundTypes.h"
#include "oalImp.h"
#include "busmessage.h"

#define xMIN_SOUND_PRINTS
#if RELEASE_CLIENT == 1
#undef MIN_SOUND_PRINTS
#endif

static void _LoadWAVMemory(ALbyte* memory,ALenum* format,ALvoid** data,ALsizei* size,ALsizei* freq);

#define RETURN_IF_AL_ERROR() { ALenum x; x = alGetError(); if ( x!=AL_NO_ERROR ) return x; }
#define RETURN_AL_ERROR() { ALenum x; x = alGetError(); if ( x!=AL_NO_ERROR ) return x; else return noErr; }

#define TRACKSOUND() SHOW_DEBUG( if ( snd->bIs3D ) { is3D++; if ( is3D > is3D_max ) { is3D_max = is3D; } } else { not3D++; if ( not3D > not3D_max ) { not3D_max = not3D; } }   DEBUGSTR5( "start is3D=%d not3D=%d is3D_max=%d not3D_max=%d \n", is3D, not3D, is3D_max, not3D_max ))
#define TRACKSOUNDOFF() SHOW_DEBUG (if ( snd->bIs3D ) { is3D--; } else { not3D--; } DEBUGSTR3( "free is3D=%d not3D=%d\n", is3D, not3D ))

void SetSourceAvailable( ALuint src ) // el hacko to know when openal is done with source, not used atm
{
}

//---------------------------------------

OpenALAudio::OpenALAudio(FILE* debugStream, UINT32 maxSounds)
{
  maxSounds = 128;                                // hard coded
  mDebugStream = debugStream;
  mMaxSounds = maxSounds>MAX_AL_SOURCES?MAX_AL_SOURCES:maxSounds;
}

//---------------------------------------

OSStatus OpenALAudio::start()
{
  return noErr;
}

//---------------------------------------

OSStatus OpenALAudio::kill()
{
  alcMakeContextCurrent(NULL);
  alcDestroyContext(mContext);
  alcCloseDevice(mDevice);

  SHOW_DEBUG(fflush( mDebugStream );)

    return noErr;
}

//---------------------------------------

//#define ALC_SPATIAL_RENDERING_QUALITY        0xF002
//extern ALvoid alSetInteger (ALenum pname, const ALint value);

UInt32 SoundQuality=0;

OSStatus OpenALAudio::init()
{
  alcMacOSXMixerMaxiumumBussesProcPtr setBusses
    = (alcMacOSXMixerMaxiumumBussesProcPtr)alGetProcAddress("alcMacOSXMixerMaxiumumBusses");

  int sq = 0;
  cfgPrefsGetInt("app.macsound",&sq);
  SoundQuality = sq;

  if ( setBusses == NULL ) return ERROR_NO_DEVICE;

  (*setBusses)(mMaxSounds);

  mDevice  = alcOpenDevice(NULL);
  if ( mDevice == NULL )
  {
    return ERROR_NO_DEVICE;
  }

  mContext = alcCreateContext(mDevice,NULL);
  if ( mContext == NULL )
  {
    return ERROR_NO_CONTEXT;
  }
  if ( alcMakeContextCurrent(mContext) == ALC_FALSE )
  {
    return ERROR_NO_CONTEXT;
  }

  //UInt32 x = 1;
  //alcASASetListener(ALC_ASA_REVERB_ON, &x, sizeof(x));
  //float32 y = 1.0;
 // alcASASetListener(ALC_ASA_REVERB_GLOBAL_LEVEL, &y, sizeof(float32));
  //ALint z = 5;
  //alcASASetListener(ALC_ASA_REVERB_ROOM_TYPE_Plate, &z, sizeof(ALint));


  //alDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED);

  //alDopplerFactor(1);
  //alSpeedOfSound(344.0);

  alGenBuffers( MAX_AL_BUFFERS, mBufferArray );
  RETURN_IF_AL_ERROR();

  for ( UINT32 i=0; i <MAX_AL_BUFFERS; i++ )
  {
    mAvailableBuffers.push(mBufferArray[i]);
  }

  alGenSources( mMaxSounds, mSourceArray );
  RETURN_IF_AL_ERROR();

  for ( UINT32 i=0; i <mMaxSounds/4; i++ )
  {
    m2dAvailableSources.push(mSourceArray[i]);
  }

  for ( UINT32 i=mMaxSounds/4; i <mMaxSounds; i++ )
  {
    m3dAvailableSources.push(mSourceArray[i]);
    alSource3f(mSourceArray[i], AL_POSITION,
               500000.,
               500000.,
               500000.);

  }

  return noErr;
}

//---------------------------------------

OSStatus OpenALAudio::createSound(const snSound* oldSound, snSound* newSound)
{
  return reloadSound(oldSound,newSound);
}

//---------------------------------------

OSStatus OpenALAudio::duplicateSound(const snSound* oldSound, snSound* newSound)
{
  return reloadSound(oldSound,newSound);
}

//---------------------------------------

OSStatus OpenALAudio::freeSound(snSound* snd)
{
  if ( snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return noErr;
  }

  stopSound(snd);

  snd->buffer.source = ALSource_None;
  snd->buffer.buffer = ALBuffer_None;

  return noErr;
}

void OpenALAudio::fixName(char*name)
{
  char*p = name;
  while (*p!=0)
  {
    if ( *p == '/' || *p == '\\' )
    {
      *p='_';
    }
    p++;
  }
}

//---------------------------------------

OSStatus OpenALAudio::loadSound(snSound* snd)
{
  char name[MAX_SND_NAME_LENGTH];
  strcpy(name,snd->name);
  fixName(name);

  /*
  DEBUGSTR1( "-----\n");
  for(BufferMap::iterator iter = mBufferIndex.begin(); iter != mBufferIndex.end(); iter++)
  {
    DEBUGSTR2( "%s\n", (*iter).first );
  }
  DEBUGSTR1( "-----\n");
  */

  BufferMap::iterator found = mBufferIndex.find(name);
  if ( found != mBufferIndex.end() )
  {
    snd->buffer.buffer = found->second;
    snd->buffer.source = ALSource_None;
    DEBUGSTR2("** found %s!\n", name );
  }
  else
  {
    DEBUGSTR2("**** loading %s!\n", name );

    int fd = fioOpen(snd->name);
    if (fd < 0)
    {
      snd->buffer.buffer = ALBuffer_None;
      snd->buffer.source = ALSource_None;
      return ERROR_MISSING_FILE;
    }

    if ( mAvailableBuffers.empty() )
    {
      snd->buffer.buffer = ALBuffer_None;
      snd->buffer.source = ALSource_None;
      return ERROR_NO_MORE_BUFFERS;
    }

    size_t wavLen = fioFileSize(snd->name);
    void* wavData = memAlloc(wavLen);

    fioRead(fd, wavData, wavLen);
    fioClose(fd);

    ALenum format;
    ALsizei size, freq;
    ALvoid* alData = NULL;

    _LoadWAVMemory( (ALbyte*)wavData, &format, &alData, &size, &freq );

    //printf("%-70s %5d %7d %7d\n", snd->name, format, size, freq );

    snd->buffer.buffer = mAvailableBuffers.top(); mAvailableBuffers.pop();
    snd->buffer.source = ALSource_None;

    alBufferData(snd->buffer.buffer, format, alData, size, freq);
    ALenum err = alGetError();
    if ( err != AL_NO_ERROR )
    {
      mAvailableBuffers.push( snd->buffer.buffer );
      snd->buffer.buffer = ALBuffer_None;
      //printf("BUFFER ERROR: %s %d\n",snd->name,err);
    }
    else
    {
      char* n = (char*)memAlloc(strlen(name)+1);    // argh, I hate C
      strcpy(n,name);
      mBufferIndex[n] = snd->buffer.buffer;
      //printf("LOADED: %s as %d\n",snd->name,snd->buffer.buffer);
    }

    if(alData)
      free(alData);

    memFree( wavData );
    return err;
  }

  //DEBUGSTR4( "--> format: %d  size: %x  freq: %x\n", format, size, freq );

  return noErr;
}

//---------------------------------------

OSStatus OpenALAudio::reloadSound(const snSound* oldSound, snSound* newSound)
{
  if(oldSound == NULL
    || newSound == NULL
    || oldSound->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

  return loadSound( newSound );
}

//---------------------------------------

LOGICAL OpenALAudio::isPlaying(snSound* snd)
{
  if ( snd == 0 || snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return FALSE;
  }

  ALint state;
  alGetSourcei(snd->buffer.source, AL_SOURCE_STATE, &state);
  return (state == AL_PLAYING);
}

//---------------------------------------

OSStatus OpenALAudio::setFrequency(snSound* snd)
{
  if ( snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

  alSourcef(snd->buffer.source, AL_PITCH, (FLOAT32)snd->freqShift);
  RETURN_AL_ERROR();
}

static piVECTOR lastuserposition = {0,0,0};
static piVECTOR lastuservelocity = {0,0,0};

//---------------------------------------

OSStatus OpenALAudio::setListener3DPos(piVECTOR* r)
{
  lastuserposition = *r;
  alListener3f(AL_POSITION, r->x, r->y, r->z);
  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::setListener3DVel(piVECTOR* v)
{
  lastuservelocity = *v;
  alListener3f(AL_VELOCITY, v->x, v->y, v->z);
  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::setMaxDistance(snSound* snd)
{
  if ( snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

  alSourcef(snd->buffer.source, AL_MAX_DISTANCE, snd->fMaxDistance);
  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::setMinDistance(snSound* snd)
{
  if ( snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

  alSourcef(snd->buffer.source, AL_REFERENCE_DISTANCE, snd->fMinDistance);
  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::setOrientation(piVECTOR* at, piVECTOR* up)
{
  FLOAT32 orient[6] = {at->x, at->y, at->z, up->x, up->y, up->z};
  alListenerfv(AL_ORIENTATION, orient);
  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::setSound3DPos(snSound* snd)
{
  if ( snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

    if ( std::isnan( snd->position.x ) || std::isnan( snd->position.y ) || std::isnan( snd->position.z ) )
  {
    alSource3f(snd->buffer.source, AL_POSITION,
      MAXFLOAT,
      MAXFLOAT,
      MAXFLOAT);
  }
  else
  {
    alSource3f(snd->buffer.source, AL_POSITION,
      (FLOAT32)snd->position.x,
      (FLOAT32)snd->position.y,
      (FLOAT32)snd->position.z);
  }
  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::setSound3DVel(snSound* snd)
{
  if ( snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

    if ( std::isnan( snd->velocity.x ) || std::isnan( snd->velocity.y ) || std::isnan( snd->velocity.z ) )
  {
    alSource3f(snd->buffer.source, AL_VELOCITY,
      0.0,
      0.0,
      0.0);
  }
  else
  {
    alSource3f(snd->buffer.source, AL_VELOCITY,
      (FLOAT32)snd->velocity.x,
      (FLOAT32)snd->velocity.y,
      (FLOAT32)snd->velocity.z);
  }
  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::setVolume(snSound* snd, FLOAT32 multiplier)
{
  //return 0;
  if ( snd->buffer.source == ALSource_None || snd->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

  alSourcef(snd->buffer.source, AL_GAIN, (FLOAT32)snd->volume*multiplier);
  RETURN_AL_ERROR();

}

static int sounds3d = 0, sounds2d = 0;
static int sounds3dmax = 0, sounds2dmax = 0;

//---------------------------------------

void OpenALAudio::returnSource( snSound* snd )
{
  if ( snd->bIs3D )
  {
    m3dAvailableSources.push(snd->buffer.source);
    alSource3f(snd->buffer.source, AL_POSITION,
               500000.,
               500000.,
               500000.);
  }
  else
  {
    m2dAvailableSources.push(snd->buffer.source);
  }

  snd->buffer.source = ALSource_None;
}

//---------------------------------------

OSStatus OpenALAudio::startSound(snSound* snd)
{
  if ( snd->buffer.buffer == ALBuffer_None )
  {
    return ERROR_BAD_SOUND;
  }

//  if ( mAvailableSources.empty() )
//  {
//    return(  ERROR_NO_MORE_SOURCES );
//  }

  if ( strcmp(snd->name, "sounds/bulletby.wav")==0 )
  {
     return(  ERROR_BAD_SOUND ); // until concussion bug is fixed
  }

  if ( snd->bIs3D )
  {
    if ( m3dAvailableSources.empty() )
    {
      return ERROR_NO_MORE_SOURCES;
    }

    snd->buffer.source = m3dAvailableSources.top(); m3dAvailableSources.pop();
  }
  else
  {
    if ( m2dAvailableSources.empty() )
    {
      return ERROR_NO_MORE_SOURCES;
    }

    snd->buffer.source = m2dAvailableSources.top(); m2dAvailableSources.pop();
  }

  TRACKSOUND();

#ifdef MIN_SOUND_PRINTS
  printf("%s started [%s] [%s] %u\n",snd->name,snd->looping ? "[loop]" : "[once]",snd->bIs3D?"[3d]":"[2d]", snd->buffer.source);
#endif
  //printf( "snd: %s %s %s src=%d buffer=%d\n",  snd->name, snd->bIs3D?"[3d]":"[2d]", snd->looping ? "[loop]" : "[once]", snd->buffer.source, snd->buffer.buffer);

//  alSourcei(snd->buffer.source, AL_BUFFER,  0);

  int n = 0;
  ALint state=AL_PLAYING;
  while (state == AL_PLAYING )
  {
    alGetSourcei(snd->buffer.source, AL_SOURCE_STATE, &state);
    n++;
  }
  if ( n > 10 )
  {
#ifdef MIN_SOUND_PRINTS
    printf("waited %d\n",n);
#endif
  }

  alSourcei(snd->buffer.source, AL_BUFFER,  snd->buffer.buffer);

  alSourcei(snd->buffer.source, AL_LOOPING, snd->looping);

  if ( snd->bIs3D )
  {
    sounds3d++;
    if ( sounds3d > sounds3dmax )
    {
      sounds3dmax++;
      //printf("max: [3d] = %d @ %d\n", sounds3dmax, sounds3d);
    }
    alSourcei(snd->buffer.source, AL_SOURCE_RELATIVE, AL_FALSE );
    alSourcef(snd->buffer.source, AL_ROLLOFF_FACTOR, 1.2);
    setMaxDistance(snd);
    setMinDistance(snd);
    setFrequency( snd );

    setSound3DPos(snd);
    setSound3DVel(snd);
  }
  else
  {
    sounds2d++;
    if ( sounds2d > sounds2dmax )
    {
      sounds2dmax++;
      //printf("max: [2d] = %d @ %d\n", sounds2dmax, sounds2d);
    }
//    alListener3f(AL_POSITION, lastuserposition.x, lastuserposition.y, lastuserposition.z);
//    alListener3f(AL_VELOCITY, lastuservelocity.x, lastuservelocity.y, lastuservelocity.z );
    alSourcei(snd->buffer.source, AL_SOURCE_RELATIVE, AL_TRUE );
    alSourcef(snd->buffer.source, AL_ROLLOFF_FACTOR, 0);
    alSourcef(snd->buffer.source, AL_MAX_DISTANCE, 10);
    alSourcef(snd->buffer.source, AL_REFERENCE_DISTANCE, 1);
    alSourcef(snd->buffer.source, AL_PITCH, 1.0);
//    setSound3DPos(snd);
//    setSound3DVel(snd);
  }

  setVolume( snd, VolumeMultiplier );

  //alSourcef(snd->buffer.source, AL_GAIN, (FLOAT32)VolumeMultiplier);

  alSourcePlay(snd->buffer.source);

  RETURN_AL_ERROR();
}

//---------------------------------------

OSStatus OpenALAudio::stopSound(snSound* snd)
{
  if ( snd->buffer.source == ALSource_None )
  {
    return(  ERROR_BAD_SOUND );
  }

#ifdef MIN_SOUND_PRINTS
  printf("%s stopped %u\n",snd->name,snd->buffer.source);
#endif
  alSourceStop(snd->buffer.source); // does not actually stop until x ms later

  if ( snd->bIs3D )
  {
    sounds3d--;
  }
  else
  {
    sounds2d--;
  }

  returnSource( snd );

  TRACKSOUNDOFF();
  return noErr;
}

//---------------------------------------

struct WAVFileHdr_Struct                          /* WAV File-header */
{
  ALubyte  Id[4];
  ALsizei  Size;
  ALubyte  Type[4];
};

struct WAVFmtHdr_Struct                           /* WAV Fmt-header */
{
  ALushort Format;
  ALushort Channels;
  ALuint   SamplesPerSec;
  ALuint   BytesPerSec;
  ALushort BlockAlign;
  ALushort BitsPerSample;
};

struct WAVFmtExHdr_Struct                         /* WAV FmtEx-header */
{
  ALushort Size;
  ALushort SamplesPerBlock;
};

struct WAVSmplHdr_Struct                          /* WAV Smpl-header */
{
  ALuint   Manufacturer;
  ALuint   Product;
  ALuint   SamplePeriod;
  ALuint   Note;
  ALuint   FineTune;
  ALuint   SMPTEFormat;
  ALuint   SMPTEOffest;
  ALuint   Loops;
  ALuint   SamplerData;
  struct
  {
    ALuint Identifier;
    ALuint Type;
    ALuint Start;
    ALuint End;
    ALuint Fraction;
    ALuint Count;
  }      Loop[1];
};

struct WAVChunkHdr_Struct                         /* WAV Chunk-header */
{
  ALubyte  Id[4];
  ALuint   Size;
};

void SwapWords(unsigned int* puint);
void SwapBytes(unsigned short* pshort);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SwapWords(unsigned int* puint)
{
  #if __BIG_ENDIAN__
  unsigned int tempint;
  char* pChar1, *pChar2;

  tempint = *puint;
  pChar2 = (char*)&tempint;
  pChar1 = (char*)puint;

  pChar1[0]=pChar2[3];
  pChar1[1]=pChar2[2];
  pChar1[2]=pChar2[1];
  pChar1[3]=pChar2[0];
  #endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SwapBytes(unsigned short* pshort)
{
  #if __BIG_ENDIAN__
  unsigned short tempshort;
  char* pChar1, *pChar2;

  tempshort = *pshort;
  pChar2 = (char*)&tempshort;
  pChar1 = (char*)pshort;

  pChar1[0]=pChar2[1];
  pChar1[1]=pChar2[0];
  #endif
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void _LoadWAVMemory(ALbyte* memory,ALenum* format,ALvoid** data,ALsizei* size,ALsizei* freq)
{
  WAVChunkHdr_Struct ChunkHdr;
  WAVFmtExHdr_Struct FmtExHdr;
  WAVFileHdr_Struct FileHdr;
  WAVSmplHdr_Struct SmplHdr;
  WAVFmtHdr_Struct FmtHdr;
  ALbyte* Stream;

  *format=AL_FORMAT_MONO16;
  *data=NULL;
  *size=0;
  *freq=22050;
  if (memory)
  {
    Stream=memory;
    if (Stream)
    {
      memcpy(&FileHdr,Stream,sizeof(WAVFileHdr_Struct));
      Stream+=sizeof(WAVFileHdr_Struct);
      SwapWords((unsigned int*) &FileHdr.Size);
      FileHdr.Size=((FileHdr.Size+1)&~1)-4;
      while ((FileHdr.Size!=0)&&(memcpy(&ChunkHdr,Stream,sizeof(WAVChunkHdr_Struct))))
      {
        Stream+=sizeof(WAVChunkHdr_Struct);
        SwapWords(&ChunkHdr.Size);

        if ((ChunkHdr.Id[0] == 'f') && (ChunkHdr.Id[1] == 'm') && (ChunkHdr.Id[2] == 't') && (ChunkHdr.Id[3] == ' '))
        {
          memcpy(&FmtHdr,Stream,sizeof(WAVFmtHdr_Struct));
          SwapBytes(&FmtHdr.Format);
          if (FmtHdr.Format==0x0001)
          {
            SwapBytes(&FmtHdr.Channels);
            SwapBytes(&FmtHdr.BitsPerSample);
            SwapWords(&FmtHdr.SamplesPerSec);
            SwapBytes(&FmtHdr.BlockAlign);

            *format=(FmtHdr.Channels==1?
              (FmtHdr.BitsPerSample==8?AL_FORMAT_MONO8:AL_FORMAT_MONO16):
            (FmtHdr.BitsPerSample==8?AL_FORMAT_STEREO8:AL_FORMAT_STEREO16));
            *freq=FmtHdr.SamplesPerSec;
            Stream+=ChunkHdr.Size;
          }
          else
          {
            memcpy(&FmtExHdr,Stream,sizeof(WAVFmtExHdr_Struct));
            Stream+=ChunkHdr.Size;
          }
        }
        else if ((ChunkHdr.Id[0] == 'd') && (ChunkHdr.Id[1] == 'a') && (ChunkHdr.Id[2] == 't') && (ChunkHdr.Id[3] == 'a'))
        {
          if (FmtHdr.Format==0x0001)
          {
            *size=ChunkHdr.Size;
            if(*data == NULL)
            {
              *data=malloc(ChunkHdr.Size + 31);
              memset(*data,0,ChunkHdr.Size+31);
            }
            else
            {
              realloc(*data,ChunkHdr.Size + 31);
              memset(*data,0,ChunkHdr.Size+31);
            }
            if (*data)
            {
              memcpy(*data,Stream,ChunkHdr.Size);
              memset(((char*)*data)+ChunkHdr.Size,0,31);
              Stream+=ChunkHdr.Size;
              if (FmtHdr.BitsPerSample == 16)
              {
                for (UInt32 i = 0; i < (ChunkHdr.Size / 2); i++)
                {
                  SwapBytes(&(*(unsigned short **)data)[i]);
                }
              }
            }
          }
          else if (FmtHdr.Format==0x0011)
          {
            //IMA ADPCM
          }
          else if (FmtHdr.Format==0x0055)
          {
            //MP3 WAVE
          }
        }
        else if ((ChunkHdr.Id[0] == 's') && (ChunkHdr.Id[1] == 'm') && (ChunkHdr.Id[2] == 'p') && (ChunkHdr.Id[3] == 'l'))
        {
          memcpy(&SmplHdr,Stream,sizeof(WAVSmplHdr_Struct));
          Stream+=ChunkHdr.Size;
        }
        else Stream+=ChunkHdr.Size;
        Stream+=ChunkHdr.Size&1;
        FileHdr.Size-=(((ChunkHdr.Size+1)&~1)+8);
      }
    }
  }
}
