#include <syscfg.h> // Path is project specific.
/*------------------------------------------------------------------------
Module: audio.cpp
Description:
-------------------------------------------------------------------------*/
#include "sn/audio.h"
#include "ui/uiUTF.h"
#include "ui/uiChat.h"
#include "mem/mem.h"

#include <algorithm>

#if XAUDIO

#define DEBUG_SOUND_ATTENUATION 0

#include "sn/SDKwavefile.h"
#include "math/Vec2f.h"

#if DEBUG_SOUND_ATTENUATION
static bool chatLogging = false;
#endif

static const X3DAUDIO_CONE ListenerCone = {PI*5.0f/6.0f, PI*11.0f/6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f};

static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = {0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f};
static const X3DAUDIO_DISTANCE_CURVE Emitter_LFE_Curve = {(X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3};

static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE Emitter_Reverb_Curve = {(X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3};

static X3DAUDIO_DISTANCE_CURVE_POINT LinearCurvePoints[2] =
{
    0.0f,
    1.0f,
    1.0f,
    0.0f
};

static X3DAUDIO_DISTANCE_CURVE LinearCurve =
{
    (X3DAUDIO_DISTANCE_CURVE_POINT*)&LinearCurvePoints[0],
    2
};

#define INPUTCHANNELS   1
#define OUTPUTCHANNELS  8

static unsigned long ChannelMask;
static unsigned int Channels;
static bool UseRedirectToLFE = false;

struct WaveData
{
    CWaveFile wave;
    unsigned char* data;
    float maxattenuation;

    ~WaveData()
    {
        if(data)
        {
            memFree(data);
            data = nullptr;
        }
    }
};

struct Emitters
{
    IXAudio2SourceVoice* voice;
    X3DAUDIO_EMITTER emitter;
    X3DAUDIO_CONE emitterCone;
    X3DAUDIO_DSP_SETTINGS settings;
    XAUDIO2_BUFFER buffer;

    float emitterAzimuths[INPUTCHANNELS];
    float matrixCoefficients[INPUTCHANNELS * OUTPUTCHANNELS];

    bool inhead;
    bool hasstarted;
    bool active;

    float frequency;

    VoiceCallback* callback;

    Emitters()
    {
        emitter.pCone = &emitterCone;
        emitter.pCone->InnerAngle = 0.0f;
        emitter.pCone->OuterAngle = 0.0f;
        emitter.pCone->InnerVolume = 0.0f;
        emitter.pCone->OuterVolume = 1.0f;
        emitter.pCone->InnerLPF = 0.0f;
        emitter.pCone->OuterLPF = 1.0f;
        emitter.pCone->InnerReverb = 0.0f;
        emitter.pCone->OuterReverb = 1.0f;

        emitter.OrientFront.x = 0;
        emitter.OrientFront.y = 1;
        emitter.OrientFront.z = 0;
        emitter.OrientTop.x = 0;
        emitter.OrientTop.y = 0;
        emitter.OrientTop.z = 1;
        emitter.ChannelCount = INPUTCHANNELS;
        emitter.ChannelRadius = 1.0f;
        emitter.pChannelAzimuths = emitterAzimuths;

        emitter.InnerRadius = 2.0f;
        emitter.InnerRadiusAngle = PI/4.0f;;

        emitter.pVolumeCurve = nullptr;//(X3DAUDIO_DISTANCE_CURVE*)&LinearCurve;
        emitter.pLFECurve = nullptr;//(X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
        emitter.pLPFDirectCurve = nullptr; // use default curve
        emitter.pLPFReverbCurve = nullptr; // use default curve
        emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
        emitter.CurveDistanceScaler = 1.0f;
        emitter.DopplerScaler = 1.0f;

        settings.SrcChannelCount = INPUTCHANNELS;
        settings.DstChannelCount = Channels;
        settings.pMatrixCoefficients = matrixCoefficients;

        inhead = false;
        hasstarted = false;
        active = false;

        frequency = 1.0f;

        callback = nullptr;
        voice = nullptr;
    }

    ~Emitters()
    {
        if(callback)
        {
            delete callback;
            callback = nullptr;
        }
    }
};

static unsigned int id = 1;

static std::map<std::string, WaveData*> cache;

typedef std::map<unsigned int, struct Emitters*> EmitterMap;

static EmitterMap emitters;

static IXAudio2* xaudio2 = nullptr;
static IXAudio2MasteringVoice* MasterVoice = nullptr;
static X3DAUDIO_LISTENER listener;
static X3DAUDIO_HANDLE x3DInstance;

/*------------------------------------------------------------------------
Function: audioToggleChatLogging
Description:
-------------------------------------------------------------------------*/
void audioToggleChatLogging()
{
#if DEBUG_SOUND_ATTENUATION
    chatLogging = !chatLogging;
    uiChatPrintLine("Audio Chat Logging: %s", chatLogging ? "ENABLED" : "DISABLED");
#else
    uiChatPrintLine("DEBUG_SOUND_ATTENUATION not enabled");
#endif
}

/*------------------------------------------------------------------------
Function: audioSetMinDistance
Description:
-------------------------------------------------------------------------*/
void audioSetMinDistance(const char* name, float value)
{
#if DEBUG_SOUND_ATTENUATION
    if(cache.find(name) == cache.end())
    {
        uiChatPrintLine("Cannot find %s", name);
        return;
    }

    WaveData* data = cache.find(name)->second;

    uiChatPrintLine("Sound %s min distance: then: %f now: %f", name, data->maxattenuation, value);
    data->maxattenuation = value;
#else
    uiChatPrintLine("DEBUG_SOUND_ATTENUATION not enabled");
#endif

}

/*------------------------------------------------------------------------
Function: audioLogMinDistances
Description:
-------------------------------------------------------------------------*/
void audioLogMinDistances()
{
#if DEBUG_SOUND_ATTENUATION
    std::map<std::string, WaveData*>::iterator itr;
    itr = cache.begin();

    Enable_Log(LOG_OLD, 1);

    for(; itr != cache.end(); ++itr)
    {
        Write_Debug("%s,%f", itr->first.c_str(), itr->second->maxattenuation);
    }

    Enable_Log(LOG_OLD, WantedUserLogEnabled);

    uiChatPrintLine("Sound attenuations written to log");
#else
    uiChatPrintLine("DEBUG_SOUND_ATTENUATION not enabled");
#endif
}

/*------------------------------------------------------------------------
Function: AudioGetXAudio
Description:
-------------------------------------------------------------------------*/
IXAudio2* AudioGetXAudio()
{
    if(!xaudio2)
    {
        AudioInit();
    }

    return xaudio2;
}

/*------------------------------------------------------------------------
Function: AudioGetDevice
Description:
-------------------------------------------------------------------------*/
static int AudioGetDevice()
{
#define LOG_AUDIO_STUFF 0

#if LOG_AUDIO_STUFF
    Enable_Log(LOG_OLD, 1);
#endif

    unsigned int devicecount = 0;

    xaudio2->GetDeviceCount(&devicecount);

    Write_Debug("AudioGetDevice: Number of devices - %d", devicecount);

    XAUDIO2_DEVICE_DETAILS deviceDetails;

    int preferredDevice = -1;
    int defaultDevice = -1;
    std::string str;

    for(unsigned int i = 0; i < devicecount; ++i)
    {
        xaudio2->GetDeviceDetails(i, &deviceDetails);

        str = uiUTF16toUTF8(deviceDetails.DisplayName);
        Write_Debug("AudioGetDevice: Device[%d] %s", i, str.c_str());

        if(deviceDetails.Role == GlobalDefaultDevice)
        {
            Write_Debug("AudioGetDevice: XAudio says this is your Global Default Device: %s", str.c_str());
            defaultDevice = i;
        }

        if(deviceDetails.Role == DefaultGameDevice)
        {
            Write_Debug("AudioGetDevice: XAudio says device is used for game audio: %s", str.c_str());
        }

        if(deviceDetails.Role == NotDefaultDevice)
        {
            Write_Debug("AudioGetDevice: XAudio says device is not used as the default for any application: %s", str.c_str());
        }

        if(deviceDetails.Role == DefaultConsoleDevice)
        {
            Write_Debug("AudioGetDevice: XAudio says device is used for console audio applications: %s", str.c_str());
        }

        if(deviceDetails.Role == DefaultMultimediaDevice)
        {
            Write_Debug("AudioGetDevice: XAudio says device is used for multimedia playback: %s", str.c_str());
        }

        if(deviceDetails.Role == DefaultCommunicationsDevice)
        {
            Write_Debug("AudioGetDevice: XAudio says device is used for voice communication: %s", str.c_str());
        }

        if(deviceDetails.Role == InvalidDeviceRole)
        {
            Write_Debug("AudioGetDevice: XAudio says device role is invalid: %s", str.c_str());
        }

        std::string str2 = uiUTF16toUTF8(deviceDetails.DeviceID);
        std::transform(str2.begin(), str2.end(), str2.begin(), ::toupper);

        if(str2.find(WantedSoundCard) != std::string::npos)
        {
            Write_Debug("AudioGetDevice: Found match for wanted device - %s", str.c_str());
            preferredDevice = i;
            break;
        }
    }

    if(preferredDevice < 0)
    {
        Write_Debug("AudioGetDevice: Cannot find device. Default to Global Default Device");
        preferredDevice = defaultDevice;
    }

    if(preferredDevice < 0)
    {
        Write_Debug("AudioGetDevice: No Global Default Device Found. Defaulting to first device found");

        if(devicecount > 0)
            preferredDevice = 0;
        else
            Write_Debug("AudioGetDevice: No sound devices found.");
    }

#if LOG_AUDIO_STUFF
    Enable_Log(LOG_OLD, WantedUserLogEnabled);
#endif

    return preferredDevice;
}

/*------------------------------------------------------------------------
Function: AudioKill
Description:
-------------------------------------------------------------------------*/
void AudioKill()
{
#if DEBUG_SOUND_ATTENUATION
    audioLogMinDistances();
#endif

#if(1) // Let OS kill audio to assist in tracking a CTD
    Write_Debug("AudioKill");

    std::map<std::string, WaveData*>::iterator itr = cache.begin();

    for(; itr != cache.end(); ++itr)
    {
        delete itr->second;
    }

    cache.clear();
#else
    if(MasterVoice)
    {
        MasterVoice->DestroyVoice();
        MasterVoice = nullptr;
    }

    if(xaudio2)
    {
        xaudio2->StopEngine();
        xaudio2->Release();
        xaudio2 = nullptr;
    }
#endif
}

/*------------------------------------------------------------------------
Function: AudioInit
Description:
-------------------------------------------------------------------------*/
bool AudioInit()
{
    if(xaudio2)
        return true;

    unsigned int flags = 0;

#if(0)// DEBUG_WW2
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    HRESULT hr = XAudio2Create(&xaudio2, flags, XAUDIO2_DEFAULT_PROCESSOR);

    if(hr != S_OK)
    {
        Write_Debug("AudioInit: XAudio2Create returned %x", hr);
        return false;
    }

    int device = AudioGetDevice();

    if(device < 0)
    {
        Write_Debug("AudioInit: AudioGetDevice() returned -1");
        return false;
    }

    hr = xaudio2->CreateMasteringVoice(&MasterVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, device, nullptr);

    if(hr != S_OK)
    {
        Write_Debug("AudioInit: CreateMasteringVoice returned %x", hr);
        return false;
    }

    const float speedofsound = X3DAUDIO_SPEED_OF_SOUND;

    XAUDIO2_DEVICE_DETAILS details;
    xaudio2->GetDeviceDetails(device, &details);

    ChannelMask = details.OutputFormat.dwChannelMask;
    Channels = details.OutputFormat.Format.nChannels;
    UseRedirectToLFE = (details.OutputFormat.dwChannelMask & SPEAKER_LOW_FREQUENCY) != 0;

    X3DAudioInitialize(details.OutputFormat.dwChannelMask, speedofsound, x3DInstance);

    Write_Debug("XAudio initialization succeeded");

    return true;
}

/*------------------------------------------------------------------------
Function: AudioSetListenerPosition
Description:
-------------------------------------------------------------------------*/
void AudioSetListenerPosition(const piVECTOR& pos)
{
    listener.Position.x = pos.x;
    listener.Position.y = pos.y;
    listener.Position.z = pos.z;
}

/*------------------------------------------------------------------------
Function: AudioSetListnerVelocity
Description:
-------------------------------------------------------------------------*/
void AudioSetListenerVelocity(const piVECTOR& vel)
{
    listener.Velocity.x = vel.x;
    listener.Velocity.y = vel.y;
    listener.Velocity.z = vel.z;
}

/*------------------------------------------------------------------------
Function: AudioSetListnerOrientation
Description:
-------------------------------------------------------------------------*/
void AudioSetListenerOrientation(const piVECTOR& front, const piVECTOR& top)
{
    listener.OrientFront.x = front.x;
    listener.OrientFront.y = front.y;
    listener.OrientFront.z = front.z;

    listener.OrientTop.x = top.x;
    listener.OrientTop.y = top.y;
    listener.OrientTop.z = top.z;
}

/*------------------------------------------------------------------------
Function: AudioSetListenerCone
Description:
-------------------------------------------------------------------------*/
void AudioSetListenerCone()
{
   listener.pCone = (X3DAUDIO_CONE*)&ListenerCone;
}

/*------------------------------------------------------------------------
Function: AudioLoadSound
Description:
-------------------------------------------------------------------------*/
bool AudioLoadSound(const std::string& file, float maxattenuation)
{
    WaveData* data;

    if(cache.find(file) == cache.end())
    {
        Write_Debug("AudioLoadSound %s", file.c_str());

        data = new WaveData;

        HRESULT hr = data->wave.Open(file.c_str(), nullptr, WAVEFILE_READ);

        if(hr != S_OK)
        {
            delete data;
            return false;
        }

        DWORD WaveSize = data->wave.GetSize();

        data->data = (unsigned char*)memAlloc(sizeof(unsigned char) * WaveSize);

        data->wave.Read(data->data, WaveSize, &WaveSize);
        data->maxattenuation = maxattenuation;

        cache[file] = data;
    }

    return true;
}

/*------------------------------------------------------------------------
Function: AudioCreateEmitter
Description:
-------------------------------------------------------------------------*/
bool AudioCreateEmitter(snSound& sound)
{
	if ( xaudio2 == nullptr )
		return false;

    AudioLoadSound(sound.name);

    WaveData* data;

    if(cache.find(sound.name) == cache.end())
        return false;

    Write_Log(LOG_SOUND, LOG_INFO, "AudioCreateEmitter BEGIN %s", sound.name);

    data = cache.find(sound.name)->second;

    struct Emitters* emitter = new struct Emitters;

    emitter->active = true;
    emitter->inhead = !sound.bIs3D;
    emitter->emitter.CurveDistanceScaler = 0.0f;

    if(data->maxattenuation > 0.0f)
        emitter->emitter.CurveDistanceScaler = data->maxattenuation;

#if(0)
    emitter->callback = new VoiceCallback;
    emitter->callback->sound = &sound;
#endif

    HRESULT hr = xaudio2->CreateSourceVoice(&emitter->voice,
                                            data->wave.GetFormat(),
                                            0,
                                            XAUDIO2_DEFAULT_FREQ_RATIO,
                                            nullptr,//emitter->callback,
                                            nullptr,
                                            nullptr);

    if(hr != S_OK)
    {
        delete emitter;
        return false;
    }

    ZeroMemory(&emitter->buffer, sizeof(XAUDIO2_BUFFER));
    emitter->buffer.pAudioData = data->data;
    emitter->buffer.Flags = XAUDIO2_END_OF_STREAM;
    emitter->buffer.AudioBytes = data->wave.GetSize();

    sound.handle = id++;

    emitters[sound.handle] = emitter;

    std::string str = sound.name;
    size_t pos = str.find(".wav");

    if(pos != std::string::npos)
    {
        str.replace(pos, 9, "_trail.wav");

        if(cache.find(str) != cache.end())
        {
            sound.hasTrail = true;
        }
    }

    return true;
}

/*------------------------------------------------------------------------
Function: AudioGetEmitter
Description:
-------------------------------------------------------------------------*/
static struct Emitters* AudioGetEmitter(unsigned int handle)
{
    if(emitters.empty())
        return nullptr;

    if(emitters.empty())
        return nullptr;

    EmitterMap::iterator itr = emitters.find(handle);

    if(itr != emitters.end())
    {
        return itr->second;
    }

    return nullptr;
}

/*------------------------------------------------------------------------
Function: SetVolumeCurve
Description:
-------------------------------------------------------------------------*/
static bool SetVolumeCurve(const snSound& sound)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    if(e)
    {
        if(e->emitter.CurveDistanceScaler == 0.0f)
        {
            e->emitter.CurveDistanceScaler = sound.fMinDistance;

#if DEBUG_SOUND_ATTENUATION
            std::map<std::string, WaveData*>::iterator itr;

            itr = cache.find(sound.name);

            if(itr != cache.end())
            {
                itr->second->maxattenuation = sound.fMinDistance;
            }
#endif
        }

        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioSetPosition
Description:
-------------------------------------------------------------------------*/
bool AudioSetPosition(const snSound& sound)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    if(e)
    {
        e->emitter.Position.x = (float)sound.position.x;
        e->emitter.Position.y = (float)sound.position.y;
        e->emitter.Position.z = (float)sound.position.z;

        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioSetVelocity
Description:
-------------------------------------------------------------------------*/
bool AudioSetVelocity(const snSound& sound)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    if(e)
    {
        e->emitter.Velocity.x = (float)sound.velocity.x;
        e->emitter.Velocity.y = (float)sound.velocity.y;
        e->emitter.Velocity.z = (float)sound.velocity.z;

        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioPlay
Description:
-------------------------------------------------------------------------*/
bool AudioPlay(const snSound& sound)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    if(e)
    {
        SetVolumeCurve(sound);
        e->buffer.LoopCount = sound.looping ? XAUDIO2_LOOP_INFINITE : 0;

        HRESULT hr = e->voice->SubmitSourceBuffer(&e->buffer);

        if(hr != S_OK)
        {
            return false;
        }

        if(!e->inhead)
        {
            AudioSetVolume(sound);
            AudioSetVelocity(sound);
            AudioSetPitch(sound);

            static unsigned int flags = X3DAUDIO_CALCULATE_MATRIX |
                                        X3DAUDIO_CALCULATE_DOPPLER |
                                        X3DAUDIO_CALCULATE_LPF_DIRECT |
                                        X3DAUDIO_CALCULATE_LPF_REVERB |
                                        X3DAUDIO_CALCULATE_REVERB;

            if(UseRedirectToLFE)
                flags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

            X3DAudioCalculate(x3DInstance, &listener, &e->emitter, flags, &e->settings);

            IXAudio2SourceVoice* voice = e->voice;

            if(voice)
            {
                voice->SetFrequencyRatio(e->settings.DopplerFactor * e->frequency);
                voice->SetOutputMatrix(MasterVoice, INPUTCHANNELS, Channels, e->matrixCoefficients);
            }

#if DEBUG_SOUND_ATTENUATION
        if(chatLogging)
            uiChatPrintLine("SOUND: %s has started: min attenuation %f", sound.name, e->emitter.CurveDistanceScaler);
#endif
        }

        e->voice->Start(0);
        e->hasstarted = true;

        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioStop
Description:
-------------------------------------------------------------------------*/
bool AudioStop(unsigned int handle)
{
    struct Emitters* e = AudioGetEmitter(handle);

    if(e && e->active)
    {
        e->voice->Stop(XAUDIO2_PLAY_TAILS);
        e->active = false;
        e->voice->FlushSourceBuffers();

        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioUpdate
Description:
-------------------------------------------------------------------------*/
void AudioKillSound(const snSound& sound)
{
    AudioStop(sound.handle);

    EmitterMap::iterator itr = emitters.find(sound.handle);

    if(itr != emitters.end())
    {
        itr->second->voice->DestroyVoice();

        delete itr->second;
        emitters.erase(itr);
    }
}

/*------------------------------------------------------------------------
Function: AudioUpdate
Description:
-------------------------------------------------------------------------*/
void AudioUpdate()
{
    EmitterMap::iterator itr = emitters.begin();

    static unsigned int flags = X3DAUDIO_CALCULATE_MATRIX |
                                X3DAUDIO_CALCULATE_DOPPLER |
                                X3DAUDIO_CALCULATE_LPF_DIRECT |
                                X3DAUDIO_CALCULATE_LPF_REVERB |
                                X3DAUDIO_CALCULATE_REVERB;

    if(UseRedirectToLFE)
        flags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

    for(; itr != emitters.end(); ++itr)
    {
        struct Emitters* e = itr->second;

        if(!e->inhead && e->hasstarted)
        {
            X3DAudioCalculate(x3DInstance, &listener, &e->emitter, flags, &e->settings);

            IXAudio2SourceVoice* voice = e->voice;

            if(voice)
            {
                voice->SetFrequencyRatio(e->settings.DopplerFactor * e->frequency);
                voice->SetOutputMatrix(MasterVoice, INPUTCHANNELS, Channels, e->matrixCoefficients);

#if(0)
                XAUDIO2_FILTER_PARAMETERS FilterParametersDirect =
                {
                    LowPassFilter,
                    2.0f * sinf(X3DAUDIO_PI/6.0f * e->settings.LPFDirectCoefficient),
                    1.0f
                };

                voice->SetOutputFilterParameters(MasterVoice, &FilterParametersDirect);
#endif
            }
        }
    }
}

/*------------------------------------------------------------------------
Function: AudioGetVolume
Description:
-------------------------------------------------------------------------*/
bool AudioGetVolume(const snSound& sound, float& volume)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    if(e)
    {
        e->voice->GetVolume(&volume);
        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioSetVolume
Description:
-------------------------------------------------------------------------*/
bool AudioSetVolume(const snSound& sound)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    if(e)
    {
        e->voice->SetVolume((float)sound.volume * VolumeMultiplier);
        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioSetPitch
Description:
-------------------------------------------------------------------------*/
bool AudioSetPitch(const snSound& sound)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    if(e)
    {
        e->frequency = (float)sound.freqShift;
        return true;
    }

    return false;
}

/*------------------------------------------------------------------------
Function: AudioIsPlaying
Description:
-------------------------------------------------------------------------*/
bool AudioIsPlaying(const snSound& sound)
{
    struct Emitters* e = AudioGetEmitter(sound.handle);

    // FIXME
    if(e)
    {
        XAUDIO2_VOICE_STATE state;
        e->voice->GetState(&state);

        return state.BuffersQueued > 0;
    }

    return false;
}
#endif // XAUDIO
