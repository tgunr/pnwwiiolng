#pragma once

#include "sn/sndsSoundControl.h"
#include "sn/sndsSoundTypes.h"

#if XAUDIO

#define infinite_sound  0x00000001

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    HANDLE BufferEndEvent;

    VoiceCallback() :
        BufferEndEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr))
    {
        sound = nullptr;
    }

    ~VoiceCallback()
    {
        sound = nullptr;
        CloseHandle(BufferEndEvent);
    }

    //Called when the voice has just finished playing a contiguous audio stream.
    STDMETHOD_(void, OnStreamEnd) (THIS)
    {
        SetEvent(BufferEndEvent);
    }

    STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS)
    {
    }

    STDMETHOD_(void, OnVoiceProcessingPassStart) (THIS_ UINT32 BytesRequired)
    {
    }

    STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext)
    {
    }

    STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext)
    {
    }

    STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext)
    {
    }

    STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error)
    {
#if DEBUG_WW2
        Enable_Log(LOG_OLD, 1);
#endif

        Write_Debug("OnVoiceError: %x", Error);

#if DEBUG_WW2
        Enable_Log(LOG_OLD, WantedUserLogEnabled);
#endif
    }

    snSound* sound;
};

extern bool AudioInit();
extern bool AudioPlay(const snSound& sound);
extern bool AudioLoadSound(const std::string& file, float maxattenuation = 0.0f);
extern bool AudioSetPosition(const snSound& sound);
extern bool AudioSetVelocity(const snSound& sound);
extern bool AudioIsPlaying(const snSound& sound);
extern bool AudioStop(unsigned int handle);
extern bool AudioGetVolume(const snSound& sound);
extern bool AudioSetVolume(const snSound& sound);
extern bool AudioSetPitch(const snSound& sound);
extern bool AudioCreateEmitter(snSound& sound);

extern void AudioSetListenerVelocity(const piVECTOR& vel);
extern void AudioSetListenerPosition(const piVECTOR& pos);
extern void AudioKillSound(const snSound& sound);
extern void AudioKill();
extern void AudioUpdate();
extern void AudioSetListenerOrientation(const piVECTOR& front, const piVECTOR& top);

extern IXAudio2* AudioGetXAudio();

#endif // XAUDIO

