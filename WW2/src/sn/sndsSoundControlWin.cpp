#include <syscfg.h> // Path is project specific.
#include "sn/sndsSoundControlWin.h"
#include "sn/sndsSoundTypes.h"
#include "ld/ldwav.h"
#include "mem/mem.h"
//#include "resource.h"
#include <math.h>
#include <windowsx.h>
#include <process.h>
#include "sys/sysreg.h"
#include "include/vars.h"
#include "pi/piinfo.h" // for device ids
#include "cfg/cfgPrefs.h"
#include <MMSystem.h>
#include <DSound.h>
#include "ui/uiUTF.h"

#if XAUDIO
#include "sn/audio.h"
#endif

#ifdef DIRECT3D
#include "dxvars.h"
#endif	// DIRECT3D

#if defined(OPENGL3D) && platOS == OS_WIN32
extern HWND myhwnd;
#endif	// defined(OPENGL3D) && platOS == OS_WIN32

/*
	DirectX constants and typedefs
*/
struct piGUID
{
	UINT32	Data1;
	UINT16	Data2;
	UINT16	Data3;
	UCHAR	Data4[8];
};

struct SNDDETDRIVERS
{
	piGUID					Guid;
	SINT8					DriverDescription[256];
	SINT8					DriverName[256];
};

static DSBUFFERDESC				dsbdesc;
static LPGUID					dsguid;
static LPDIRECTSOUND8           lpds = nullptr;
static LPDIRECTSOUND3DLISTENER8 g_lpListener8 = nullptr;

// used to enumerate direct sound devices
static int                      dsnumdrivers = 0;
static int                      curdriver = 0;
static SNDDETDRIVERS            dsdrivers[MAXENUMDRIVERS];

#define REG_KEY_CFG			    _T("Software\\") SYS_REGISTRY_NAME _T("\\Config")
#define REG_AUDIO  	            _T("Audio")

#if(0)
static BOOL dsEnumDrivers();
static LOGICAL sndsPlatformInitListener();
#endif

// Function that throws up an error when the sound card fails to initialize.
static inline void _soundError(const char* description)
{
	static const TCHAR* _SND_TITLE = _T("Error initializing sound");
	static const TCHAR* _SND_ERR_1 = _T("Failed to initialize the sound device: ");
	static const TCHAR* _SND_ERR_2 = _T("\nPlease check your sound card configuration and try again.");

	Write_Log(LOG_SOUND, LOG_WARN, description);
	std::wstring message = _SND_ERR_1;
	message += UTF16(description);
	message += _SND_ERR_2;

	MessageBox(nullptr, message.c_str(), _SND_TITLE, MB_ICONEXCLAMATION|MB_OK);
}

////////////////////////////////////////////////
// Function: sndsPlatformInit
// Description:
////////////////////////////////////////////////
LOGICAL sndsPlatformInit()
{
#if XAUDIO
    return AudioInit();
#else
    WAVEFORMATEX wf;
    HRESULT hr;
    LPDIRECTSOUNDBUFFER lpPrimary = nullptr;

	try
	{
	    /////////////////////////////
		// Create DirectSound object
	    /////////////////////////////
		hr = DirectSoundCreate8(nullptr, &lpds, nullptr);
	    if(hr != DS_OK)
	    {
			DDError(hr);
			throw "Could not create the DirectSound object";
		}

	    ///////////////////////
	    // Set the co-op level
	    ///////////////////////
	    hr = lpds->SetCooperativeLevel(myhwnd, DSSCL_PRIORITY);

	    if(hr != DS_OK)
	    {
			DDError(hr);
			throw "Could not set the cooperative level";
		}

		///////////////////////////////
		// Create primary buffer
		///////////////////////////////
		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));

	    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	    dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

	    LPDIRECTSOUNDBUFFER primary = nullptr;

	    hr = lpds->CreateSoundBuffer(&dsbdesc, &primary, nullptr);

	    if(hr != DS_OK)
	    {
			DDError(hr);
			throw "Could not create the primary sound buffer";
		}

	    /////////////////////////////
		// Set primary buffer format
	    /////////////////////////////
		memset(&wf, 0, sizeof(WAVEFORMATEX));

		wf.wFormatTag = WAVE_FORMAT_PCM;
	    wf.nChannels = 2;
		wf.nSamplesPerSec = 22050;
		wf.wBitsPerSample = 16;
		wf.nBlockAlign = (wf.wBitsPerSample / 8) * wf.nChannels;
		wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	    hr = primary->SetFormat(&wf);

	    if(hr != DS_OK)
	    {
			sndsPlatformKill();
			throw "Could not set the format for the primary DirectSound buffer";
		}

		// Don't need this anymore
		primary->Release();
		primary = nullptr;

	    if(b3DSoundEnable && !sndsPlatformInitListener())
	    {
			sndsPlatformKill();
			throw "Unable to initialize hardware sound driver";
		}

	    if(VolumeMultiplier < 0.01f)
	    {
			float newVolume = 0.01f;
			cfgPrefsSetGlobal(cfgVOLUME_MULTIPLIER, &newVolume, true);
		}
		else if(VolumeMultiplier > 1.0f)
		{
			float newVolume = 1.0f;
			cfgPrefsSetGlobal(cfgVOLUME_MULTIPLIER, &newVolume, true);
		}

	    if(MusicMultiplier < 0.01f)
	    {
			float newVolume = 0.01f;
			cfgPrefsSetGlobal(cfgMUSIC_MULTIPLIER, &newVolume, true);
		}
		else if(MusicMultiplier > 1.0f)
		{
			float newVolume = 1.0f;
			cfgPrefsSetGlobal(cfgMUSIC_MULTIPLIER, &newVolume, true);
		}

		/////////////////////////////////////////////////
		// MaxNumberOfSounds is set through settings.exe
		// Set to match default value in cfgLoad.c
		/////////////////////////////////////////////////
		MaxNumberOfSounds = MaxNumberOfSounds < 16 ? 16 :
							MaxNumberOfSounds > 128 ? 128 :
							MaxNumberOfSounds;

#if(0)
		//////////////////////////////////////////////////////////////////////
		// Set the buffer playing and looping.  This keeps the mixer running.
		//////////////////////////////////////////////////////////////////////
		hr = g_lpDSPrimary->Play(0, 0, DSBPLAY_LOOPING);

		if(hr != DS_OK)
		{
	        DDError(hr);
			sndsPlatformKill();
			throw "Could not play the primary sound buffer";
		}
#endif

	    return TRUE;
	}
	catch ( const char* error )
	{
		_soundError(error);
	}
	catch ( std::exception& e )
	{
		_soundError(e.what());
	}
	catch ( ... )
	{
		_soundError("[unknown error]");
	}

	return FALSE;
#endif // XAUDIO
}

//////////////////////////////////////////////////////////////////////
// Function: sndsPlatformInitListener
// Description: This function queries the primary sound buffer object
// for the IDirectSound3Dlistener interface.  It then sets up some
// settings for the listner.
//////////////////////////////////////////////////////////////////////
LOGICAL sndsPlatformInitListener()
{
#if XAUDIO
#else
    DSBUFFERDESC desc;
    memset(&desc, 0, sizeof(DSBUFFERDESC));

    desc.dwSize = sizeof(DSBUFFERDESC);
    desc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;

    LPDIRECTSOUNDBUFFER primary = nullptr;

    HRESULT hr = lpds->CreateSoundBuffer(&desc, &primary, nullptr);

    if(hr != DS_OK)
    {
        DDError(hr);
        return FALSE;
    }

    // Create the 3D Listener object.
    hr = primary->QueryInterface(IID_IDirectSound3DListener8, (void**)&g_lpListener8);

    // Don't need this anymore
    primary->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        return FALSE;
    }

    g_lpListener8->SetOrientation(0.0f, // X Front
                                  1.0f, // Y Front
                                  0.0f, // Z Front
                                  0.0f, // X Top
                                  0.0f, // Y Top
                                  1.0f, // Z Top
                                  DS3D_DEFERRED);

   g_lpListener8->CommitDeferredSettings();
#endif

   return TRUE;
}

//////////////////////////////////////
// Function: sndsPlatformKill
// Description:
/////////////////////////////////////
LOGICAL sndsPlatformKill()
{
#if XAUDIO
    AudioKill();
#else
    if(g_lpListener8 != nullptr)
    {
        g_lpListener8->Release();
        g_lpListener8 = nullptr;
    }

    if(lpds != nullptr)
    {
        lpds->Release();
        lpds = nullptr;
    }
#endif

	return TRUE;
}

LOGICAL sndsPlatformOverLimit(UINT32 numPlaying)
{
	// this is arbitrary right now - some sort of hardware analysis should be done so that
	// superior sound cards can play more sounds - AJL
	return (numPlaying > MaxNumberOfSounds);
}

//FIXME for IDirectSoundBuffer8
LOGICAL sndsPlatformReloadSound(snSound* oldSound, snSound* newSound)
{
#if XAUDIO
#else
    HRESULT hr;

    *newSound = *oldSound;

    hr = lpds->DuplicateSoundBuffer(oldSound->lpdsb, &newSound->lpdsb);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformReloadSound] **FAILED**");
        return FALSE;
    }
#endif
	return(TRUE);
}

LOGICAL sndsPlatformLoadSound(snSound* whichSound)
{
#if XAUDIO
    if(whichSound == nullptr)
        return FALSE;

    return AudioLoadSound(whichSound->name);
#else
BYTE*			wave;
SINT32			dbVol;	// will be in the format DirectSound expects, between DSBVOLUME_MAX, DSBVOLUME_MIN
DWORD			length;
WAVEFORMATEX	wf;
DWORD			writepos, wavelen;
DWORD			abytes1, abytes2;
void*			aptr1, *aptr2;
HRESULT         hr;

    if(lpds == nullptr)
        return FALSE;

    if (whichSound == nullptr)
	{
		return(FALSE);
	}

	if (!ldwavLoadFile(whichSound->name, &wave, &wf, &length))
	{
		return(FALSE);
	}

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);

    if(whichSound->bIs3D)
    {
        // Setting DSBCAPS_LOCHARDWARE will force DirectX to always use hardware. However
        // if hardware resources run out, the sound will fail. It's best to let DirectX
        // decide where to put the sound...hardware if we have resources, otherwise software.
        // Since there are many occasions where more sounds play than hardware resources, it's
        // best to leave this flag out. See DirectSound documentation.

	    dsbdesc.dwFlags =  DSBCAPS_CTRLFREQUENCY |
						   DSBCAPS_CTRLVOLUME |
						   DSBCAPS_CTRL3D |
                           DSBCAPS_MUTE3DATMAXDISTANCE |
                           DSBCAPS_GLOBALFOCUS |
                           DSBCAPS_GETCURRENTPOSITION2;
    }
    else
    {
	    dsbdesc.dwFlags =	DSBCAPS_GETCURRENTPOSITION2 |	// Always a good idea
						    DSBCAPS_GLOBALFOCUS |			// Allows background playing
						    DSBCAPS_CTRLFREQUENCY |			// needed for frequency changes
						    DSBCAPS_CTRLVOLUME |			// needed for volume changes
						    DSBCAPS_CTRLPAN |				// needed for panning
						    DSBCAPS_LOCSOFTWARE;			// FIXME
    }

	dsbdesc.dwBufferBytes = length;
	dsbdesc.lpwfxFormat = &wf;

    hr = lpds->CreateSoundBuffer(&dsbdesc, &whichSound->lpdsb, nullptr);

    if(hr != DS_OK)
    {
        SCHAR szBuff[256];

        DDError(hr);

        sprintf(szBuff, "%s %s",
        "[sndsPlatformLoadSound] **FAILED** to create sound buffer for",
        whichSound->name);

        Write_Log(LOG_SOUND, LOG_WARN, szBuff);

        if(wave)
		{
			memFree(wave);
		}

        return FALSE;
    }

	writepos = 0;
	wavelen = length;
	if ((whichSound->lpdsb != nullptr) && (DS_OK != DDError(whichSound->lpdsb->Lock(writepos, wavelen,
														                        &aptr1, &abytes1,
														                        &aptr2, &abytes2,
														                        0))))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "Could not lock sound buffer");

		if (wave)
		{
			memFree(wave);
		}

		return(FALSE);
	}

	// copy the wave data into the buffer.  Note that Lock may have returned 2 chunks in the buffer to write into
	memcpy(aptr1, wave, std::min(abytes1, wavelen));
	wavelen -= abytes1;
	if (aptr2 != nullptr && wavelen != 0)
	{
		memcpy(aptr2, &wave[abytes1], std::min(abytes2, wavelen));
	}

	if (wave)
	{
		memFree(wave);
	}

	if ((whichSound->lpdsb != nullptr) && (DS_OK != DDError(whichSound->lpdsb->Unlock(aptr1, abytes1,
														                           aptr2, abytes2))))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "Could not unlock sound buffer");
		return(FALSE);
	}

	// get the base frequency for the sound and stick that number into the snSound
	if (DS_OK != DDError(whichSound->lpdsb->GetFrequency((LPDWORD)&(whichSound->frequency))))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "Could not get the frequency on sound %s", whichSound->name);
		return(FALSE);
	}

	dbVol = (SINT32)(1000.0 * (log10((FLOAT64)VolumeMultiplier)));
	if (DS_OK != DDError(whichSound->lpdsb->SetVolume(dbVol)))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "Could not set volume %f on sound %s", dbVol, whichSound->name);
		return(FALSE);
	}
#endif

	return(TRUE);
}

LOGICAL sndsPlatformFreeSound(snSound* whichSound)
{
#if XAUDIO
    if(whichSound == nullptr)
        return FALSE;

    AudioKillSound(*whichSound);
#else
	if (whichSound == nullptr)
	{
		return(FALSE);
	}

	sndsPlatformStopSound(whichSound);

	if (whichSound->lpdsb != nullptr)
	{
		whichSound->lpdsb->Stop();
		whichSound->lpdsb->Release();
		whichSound->lpdsb = nullptr;
	}
#endif

	return(TRUE);
}

LOGICAL sndsPlatformReset()
{
	// not implemented yet
	return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: sndsPlatformCreateSound
// Description: This function creates a new sound buffer from
// the original sound. It replaces sndsPlaformReloadSound
// DuplicateSoundBuffer, since we may need to change the properties
// of the sound based on 3D or 2D.
//////////////////////////////////////////////////////////////////////
LOGICAL sndsPlatformCreateSound(snSound* lpOld, snSound* lpNew)
{
#if XAUDIO
    if(lpNew == nullptr)
        return FALSE;

    return AudioCreateEmitter(*lpNew);
#else
    WAVEFORMATEX wf;
    DSBUFFERDESC dsDesc;
    HRESULT hr;
    DSBCAPS dsCaps;
    LPDIRECTSOUNDBUFFER lpDSB = nullptr;
    DWORD dwVol;
    DWORD dwWritePos;
    DWORD dwWaveLen;
    DWORD toBytes1, toBytes2, fromBytes1, fromBytes2;
    void* toPtr1, *toPtr2, *fromPtr1, *fromPtr2;

    if(lpOld == nullptr) return FALSE;
    if(lpNew == nullptr) return FALSE;
    if(lpOld == lpNew) return FALSE;

    memset(&dsDesc, 0, sizeof(DSBUFFERDESC));
    dsDesc.dwSize = sizeof(DSBUFFERDESC);

    if(lpNew->bIs3D)
    {
        dsDesc.dwFlags = DSBCAPS_CTRLFREQUENCY |
                         DSBCAPS_CTRLVOLUME |
                         DSBCAPS_CTRL3D |
                         DSBCAPS_MUTE3DATMAXDISTANCE |
                         DSBCAPS_GLOBALFOCUS |
                         DSBCAPS_GETCURRENTPOSITION2;
    }
    else
    {
        dsDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 |	// Always a good idea
                         DSBCAPS_GLOBALFOCUS |			// Allows background playing
                         DSBCAPS_CTRLFREQUENCY |		// needed for frequency changes
                         DSBCAPS_CTRLVOLUME |			// needed for volume changes
                         DSBCAPS_CTRLPAN |				// needed for panning
                         DSBCAPS_LOCSOFTWARE;			// FIXME
    }

    dsCaps.dwSize = sizeof(DSBCAPS);

    hr = lpOld->lpdsb->GetCaps(&dsCaps);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] GetCaps** FAILED**");
        return FALSE;
    }

    dsDesc.dwBufferBytes = dsCaps.dwBufferBytes;

    hr = lpOld->lpdsb->GetFormat(&wf, sizeof(WAVEFORMATEX), nullptr);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] GetFormat** FAILED**");
        return FALSE;
    }

	dsDesc.lpwfxFormat = &wf;

    hr = lpds->CreateSoundBuffer(&dsDesc, &lpDSB, nullptr);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] CreateSoundBuffer** FAILED**");
        return FALSE;
    }

    dwWritePos = 0;
    dwWaveLen = dsCaps.dwBufferBytes;

    hr = lpDSB->Lock(dwWritePos,
                     dwWaveLen,
                     &toPtr1,
                     &toBytes1,
                     &toPtr2,
                     &toBytes2,
                     DSBLOCK_ENTIREBUFFER);

    if(hr != DS_OK)
    {
        lpDSB->Release();
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] lpDSB Lock** FAILED**");
        return FALSE;
    }

    hr = lpOld->lpdsb->Lock(dwWritePos,
                            dwWaveLen,
                            &fromPtr1,
                            &fromBytes1,
                            &fromPtr2,
                            &fromBytes2,
                            DSBLOCK_ENTIREBUFFER);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] lpOld Lock** FAILED**");
        lpDSB->Release();
        return FALSE;
    }

    if(toBytes1 + toBytes2 == fromBytes1 + fromBytes2)
    {
        if(toBytes1 == fromBytes1)
        {
            if(toBytes1 > 0 && toPtr1 != nullptr)
                memcpy(toPtr1, fromPtr1, toBytes1);

            if(toBytes2 > 0 && toPtr2 != nullptr)
                memcpy(toPtr2, fromPtr2, toBytes2);
        }
        else
        {
            LOGICAL	smallerFirstRead;

            // read either all of first read block into part of first write block,
            // or part of first read block into all of first write block
            void*	readPtr, *writePtr;
            DWORD	bytesCopied = 0, bytesToCopy;

            if(toBytes1 < fromBytes1)
            {
                bytesToCopy = toBytes1;
                smallerFirstRead = FALSE;
            }
            else
            {
                bytesToCopy = fromBytes1;
                smallerFirstRead = TRUE;
            }

            readPtr = fromPtr1;
            writePtr = toPtr1;
            memcpy(writePtr, readPtr, bytesToCopy);
            bytesCopied += bytesToCopy;

            if(smallerFirstRead == TRUE)
            {
                // read portion of second read buffer into remainder of first write buffer
                readPtr = fromPtr2;
                writePtr = (void*) ((char*)toPtr1 + bytesToCopy);
                bytesToCopy = toBytes1 - fromBytes1;
                memcpy(writePtr, readPtr, bytesToCopy);
                bytesCopied += bytesToCopy;

                // read rest of second read buffer into second write buffer
                readPtr = (void*) ((char*) fromPtr2 + bytesToCopy);
                writePtr = toPtr2;
                bytesToCopy = toBytes2;
                memcpy(writePtr, readPtr, bytesToCopy);
                bytesCopied += bytesToCopy;
            }
            else
            {
                // read remainder of first read buffer into beginning of second write buffer
                readPtr = (void*) ((char*) fromPtr1 + bytesToCopy);
                writePtr = toPtr2;
                bytesToCopy = fromBytes1 - bytesToCopy;
                memcpy(writePtr, readPtr, bytesToCopy);
                bytesCopied += bytesToCopy;

                // read second read buffer into remainder of second write buffer
                readPtr = fromPtr2;
                writePtr = (void*) ((char*) toPtr2 + bytesToCopy);
                bytesToCopy = fromBytes2;
                memcpy(writePtr,readPtr, bytesToCopy);
                bytesCopied += bytesToCopy;
            }

            memcpy(writePtr, readPtr, bytesToCopy);
        }
    }
    else
    {
        Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: writable bytes in new buffer does not equal readable bytes in old buffer!");
    }

    hr = lpDSB->Unlock(toPtr1,
                       toBytes1,
                       toPtr2,
                       toBytes2);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] lpDSB UnLock** FAILED**");
        lpDSB->Release();
        return FALSE;
    }

    hr = lpOld->lpdsb->Unlock(fromPtr1,
                              fromBytes1,
				              fromPtr2,
                              fromBytes2);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] lpOld UnLock** FAILED**");
        lpDSB->Release();
        return FALSE;
    }

    hr = lpDSB->GetFrequency((LPDWORD)&lpNew->frequency);

    if(hr != DS_OK)
	{
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] GetFrequency** FAILED**");
        lpDSB->Release();
        return FALSE;
	}

	dwVol = (SINT32)(1000.0 * (log10((FLOAT64)VolumeMultiplier)));

    hr = lpDSB->SetVolume(dwVol);

    if(hr != DS_OK)
	{
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCreateSound] SetVolume** FAILED**");
        lpDSB->Release();
        return FALSE;
	}

    lpNew->lpdsb = lpDSB;
#endif // XAUDIO
    return TRUE;
}

LOGICAL	sndsPlatformDuplicateSound(snSound* to, const snSound* from)
{
#if XAUDIO
    return TRUE;
#else
WAVEFORMATEX			wf;
DWORD					writepos, wavelen;
DWORD					toBytes1, toBytes2, fromBytes1, fromBytes2;
void*					toPtr1, *toPtr2, *fromPtr1, *fromPtr2;
LPDIRECTSOUNDBUFFER		newSoundBuffer;
LOGICAL					canCopyData = TRUE, retval = TRUE;
DSBCAPS					caps;

	if(to == nullptr || from == nullptr || to == from)
		return retval;

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);

    if(from->bIs3D)
    {
	    dsbdesc.dwFlags =  DSBCAPS_CTRLFREQUENCY |
						   DSBCAPS_CTRLVOLUME |
						   DSBCAPS_CTRL3D |
                           DSBCAPS_MUTE3DATMAXDISTANCE |
                           DSBCAPS_GLOBALFOCUS |
                           DSBCAPS_GETCURRENTPOSITION2;
    }
    else
    {
	    dsbdesc.dwFlags =	DSBCAPS_GETCURRENTPOSITION2 |	// Always a good idea
						    DSBCAPS_GLOBALFOCUS |			// Allows background playing
						    DSBCAPS_CTRLFREQUENCY |			// needed for frequency changes
						    DSBCAPS_CTRLVOLUME |			// needed for volume changes
						    DSBCAPS_CTRLPAN |				// needed for panning
						    DSBCAPS_LOCSOFTWARE;			// FIXME
    }

	caps.dwSize = sizeof(caps);
	if(DS_OK != DDError(from->lpdsb->GetCaps(&caps)))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: Could not GetCaps from original sound");
		retval = FALSE;
	}

	dsbdesc.dwBufferBytes = caps.dwBufferBytes;

	if(DS_OK != DDError(from->lpdsb->GetFormat(&wf, sizeof(wf), nullptr)))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: Could not obtain format of original buffer");
		retval = FALSE;
	}
	dsbdesc.lpwfxFormat = &wf;

	if(DS_OK != DDError(lpds->CreateSoundBuffer(&dsbdesc, &newSoundBuffer, nullptr)))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: Could not create the duplicate sound buffer");
		retval = FALSE;
	}

	writepos = 0;
	wavelen = caps.dwBufferBytes;
	if((newSoundBuffer != nullptr) && (DS_OK != DDError(newSoundBuffer->Lock(writepos, wavelen,
															              &toPtr1, &toBytes1,
															              &toPtr2, &toBytes2,
															              DSBLOCK_ENTIREBUFFER ))))
	{
		canCopyData = FALSE;
		Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: Could not lock duplicate sound buffer");
		retval = FALSE;
	}

	if(DS_OK != DDError(from->lpdsb->Lock(writepos, wavelen,
										  &fromPtr1, &fromBytes1,
										  &fromPtr2, &fromBytes2, DSBLOCK_ENTIREBUFFER)))
	{
		canCopyData = FALSE;
		Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: Could not lock original sound buffer");
		retval = FALSE;
	}

	if(canCopyData == TRUE)
	{
		if(toBytes1 + toBytes2 == fromBytes1 + fromBytes2)
		{
			if(toBytes1 == fromBytes1)
			{
				if(toBytes1 > 0 && toPtr1 != nullptr)
					memcpy(toPtr1, fromPtr1, toBytes1);

				if(toBytes2 > 0 && toPtr2 != nullptr)
					memcpy(toPtr2, fromPtr2, toBytes2);
			}
			else
			{
				LOGICAL	smallerFirstRead;

				// read either all of first read block into part of first write block,
				// or part of first read block into all of first write block
				void*	readPtr, *writePtr;
				DWORD	bytesCopied = 0, bytesToCopy;
				if(toBytes1 < fromBytes1)
				{
					bytesToCopy = toBytes1;
					smallerFirstRead = FALSE;
				}
				else
				{
					bytesToCopy = fromBytes1;
					smallerFirstRead = TRUE;
				}

				readPtr = fromPtr1;
				writePtr = toPtr1;
				memcpy(writePtr, readPtr, bytesToCopy);
				bytesCopied += bytesToCopy;

				if(smallerFirstRead == TRUE)
				{
					// read portion of second read buffer into remainder of first write buffer
					readPtr = fromPtr2;
					writePtr = (void*) ((char*) toPtr1 + bytesToCopy);
					bytesToCopy = toBytes1 - fromBytes1;
					memcpy(writePtr, readPtr, bytesToCopy);
					bytesCopied += bytesToCopy;

					// read rest of second read buffer into second write buffer
					readPtr = (void*) ((char*) fromPtr2 + bytesToCopy);
					writePtr = toPtr2;
					bytesToCopy = toBytes2;
					memcpy(writePtr, readPtr, bytesToCopy);
					bytesCopied += bytesToCopy;
				}
				else
				{
					// read remainder of first read buffer into beginning of second write buffer
					readPtr = (void*) ((char*) fromPtr1 + bytesToCopy);
					writePtr = toPtr2;
					bytesToCopy = fromBytes1 - bytesToCopy;
					memcpy(writePtr, readPtr, bytesToCopy);
					bytesCopied += bytesToCopy;

					// read second read buffer into remainder of second write buffer
					readPtr = fromPtr2;
					writePtr = (void*) ((char*) toPtr2 + bytesToCopy);
					bytesToCopy = fromBytes2;
					memcpy(writePtr,readPtr, bytesToCopy);
					bytesCopied += bytesToCopy;
				}

				memcpy(writePtr, readPtr, bytesToCopy);
			}
		}
		else
		{
			Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: writable bytes in new buffer does not equal readable bytes in old buffer!");
		}
	}

	if ((newSoundBuffer != nullptr) && DS_OK != DDError(newSoundBuffer->Unlock(toPtr1, toBytes1,
                                                                            toPtr2, toBytes2)))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "DuplicateSound: Could not unlock duplicate sound buffer");
		retval = FALSE;
	}

	if (DS_OK != DDError(from->lpdsb->Unlock(fromPtr1, fromBytes1,
											 fromPtr2, fromBytes2)))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "Could not unlock original sound buffer");
		retval = FALSE;
	}

	if (retval == TRUE)
	{
	SINT32	dbVol;	// will be in the format DirectSound expects, between DSBVOLUME_MAX, DSBVOLUME_MIN

		to->lpdsb = newSoundBuffer;

		dbVol = (SINT32)(1000.0 * (log10((FLOAT64)VolumeMultiplier)));
		if (DS_OK != DDError(to->lpdsb->SetVolume(dbVol)))
		{
			Write_Log(LOG_SOUND, LOG_WARN, "Could not set volume %f on sound %s", dbVol, to->name);
			retval = FALSE;
		}
	}
	return retval;
#endif // XAUDIO
}

UINT32 sndsGetMaxNumberOfSounds()
{
    return MaxNumberOfSounds;
}

#if(0)
static BOOL CALLBACK DlgProc(HWND hwndDlg,  // handle to dialog box
                             UINT uMsg,     // message
                             WPARAM wParam, // first message parameter
                             LPARAM lParam) // second message parameter
{
int   x, y, w, h;
RECT  srect, wrect;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		GetWindowRect(hwndDlg, &wrect);
		SystemParametersInfo(SPI_GETWORKAREA, 0, &srect, 0);
		w = wrect.right - wrect.left + 1;
		h = wrect.bottom - wrect.top + 1;
		x = (srect.right - srect.left + 1) / 2 - w / 2;
		y = (srect.bottom - srect.top + 1) / 2 - h / 2;
		MoveWindow(hwndDlg, x, y, w, h, FALSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwndDlg, 0);
			break;
		}
		break;
	}

	return(0);
}
#endif

LOGICAL sndsPlatformDriverConfig(void* obj)
{
#if XAUDIO
    return 1;
#else
HINSTANCE	hinst = (HINSTANCE) obj;
int   i;

	curdriver = -1;

	if (!dsEnumDrivers())
	{
		return(FALSE);
	}

	if (dsnumdrivers != 0)
	{
		const char* guid = nullptr;

		if(dsnumdrivers > 0)
			curdriver = 0;

		for (i=0 ; i < dsnumdrivers ; i++)
		{
			guid = piinfoGetDeviceIDAsString((LPGUID)&dsdrivers[i].Guid);

			if (strcasecmp(guid, WantedSoundCard) == 0) // pretty sure primary sound card is always index 0
			{
				curdriver = i;
				break;
			}
		}

		if (curdriver == -1)
		{
		char* args[1] = {""};

			DialogBox(hinst, MAKEINTRESOURCE(IDD_AUDIO_DEVICE), nullptr, DlgProc);
			_spawnl(_P_NOWAIT, "Settings.exe", "Settings.exe", nullptr);
			return 0;
		}

		if (curdriver >= 0)
		{
			dsguid = (LPGUID)&dsdrivers[curdriver].Guid;
			//sysregSetKeyText(REG_KEY_CFG, REG_AUDIO, dsdrivers[curdriver].DriverDescription);

			Write_Log(LOG_SOUND, LOG_WARN, "Using %s", dsdrivers[curdriver].DriverDescription);
		}
	}
	else
	{
		curdriver = 0;
	}

	return(curdriver != -1);
#endif
}

#if(0)
static BOOL CALLBACK EnumDriverCallback(LPGUID lpGuid, const TCHAR* lpDriverDescription, const TCHAR* lpDriverName, LPVOID lpContext)
{
#if XAUDIO
#else
	if (dsnumdrivers >= MAXENUMDRIVERS)
	{
		return(FALSE);
	}

	if (lpGuid == nullptr)
	{
		memset(&dsdrivers[dsnumdrivers].Guid, 0, sizeof(GUID));
	}
	else
	{
		dsdrivers[dsnumdrivers].Guid = *(piGUID*)lpGuid;
	}

	strcpy(dsdrivers[dsnumdrivers].DriverDescription, UTF8(lpDriverDescription));
	_strlwr(dsdrivers[dsnumdrivers].DriverDescription);
	strcpy(dsdrivers[dsnumdrivers].DriverName, UTF8(lpDriverName));
	_strlwr(dsdrivers[dsnumdrivers].DriverName);

	dsnumdrivers += 1;
#endif

	return(TRUE);
}
#endif

#if(0)
static BOOL dsEnumDrivers()
{
#if XAUDIO
#else
	dsnumdrivers = 0;

	if (DS_OK != DDError(DirectSoundEnumerate(EnumDriverCallback, nullptr)))
	{
		Write_Log(LOG_SOUND, LOG_WARN, "The audio drivers could not be enumerated");
		return(FALSE);
	}
#endif
	return(TRUE);
}
#endif

////////////////////////////////////////////////////////
// Function: sndsPlatformSetListener3DPos
// Description: Set the listener position.
////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetListener3DPos(piVECTOR* pPos, UINT32 uApply)
{
#if XAUDIO
    if(pPos == nullptr)
        return FALSE;

    AudioSetListenerPosition(*pPos);
#else
    HRESULT hr = 0;

    if(g_lpListener8 == nullptr || pPos == nullptr)
        return FALSE;

    hr = g_lpListener8->SetPosition(pPos->x, pPos->y, pPos->z, uApply);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsSetListener3DPos] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

////////////////////////////////////////////////////////
// Function: sndsPlatformSetListener3DVel
// Description: Set the listener velocity.
////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetListener3DVel(piVECTOR* pVel, UINT32 uApply)
{
#if XAUDIO
    if(pVel == nullptr)
        return FALSE;

    AudioSetListenerVelocity(*pVel);
#else
    HRESULT hr = 0;

    if(g_lpListener8 == nullptr || pVel == nullptr)
        return FALSE;

    hr = g_lpListener8->SetVelocity(pVel->x, pVel->y, pVel->z, uApply);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsSetListener3DVel] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

////////////////////////////////////////////////////////
// Function: sndsPlatformSetOrientation
// Description: Set the listener orientation.
////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetOrientation(piVECTOR* pFront, piVECTOR* pTop, UINT32 uApply)
{
#if XAUDIO
    if(!pFront || !pTop)
        return FALSE;

    AudioSetListenerOrientation(*pFront, *pTop);
#else
    HRESULT hr;

    if(pFront == nullptr || pTop == nullptr)
        return FALSE;

    if(g_lpListener8 == nullptr)
        return FALSE;

    hr = g_lpListener8->SetOrientation(pFront->x,
                                       pFront->y,
                                       pFront->z,
                                       pTop->x,
                                       pTop->y,
                                       pTop->z,
                                       uApply);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetOrientation] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

////////////////////////////////////////////////////////
// Function: sndsPlatformSetSound3DPos
// Description: Set the sound position.
////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetSound3DPos(snSound* sound, UINT32 uApply)
{
#if XAUDIO
    if(sound == nullptr)
        return FALSE;

    AudioSetPosition(*sound);
#else
    HRESULT hr = 0;
    LPDIRECTSOUND3DBUFFER8 lpDS3D = nullptr;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    hr = sound->lpdsb->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&lpDS3D);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetSound3DPos] **FAILED**");
        return FALSE;
    }

    hr = lpDS3D->SetPosition((D3DVALUE)sound->position.x,
                             (D3DVALUE)sound->position.y,
                             (D3DVALUE)sound->position.z,
                             uApply);

    lpDS3D->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetSound3DPos] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

////////////////////////////////////////////////////////
// Function: sndsPlatformSetSound3DVel
// Description: Set the sound velocity.
////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetSound3DVel(snSound* sound, UINT32 uApply)
{
#if XAUDIO
    if(sound == nullptr)
        return FALSE;

    AudioSetVelocity(*sound);
#else
    HRESULT hr = 0;
    LPDIRECTSOUND3DBUFFER8 lpDS3D = nullptr;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    hr = sound->lpdsb->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&lpDS3D);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetSound3DVel] **FAILED**");
        return FALSE;
    }

    hr = lpDS3D->SetVelocity((D3DVALUE)sound->velocity.x,
                             (D3DVALUE)sound->velocity.y,
                             (D3DVALUE)sound->velocity.z,
                             uApply);

    lpDS3D->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetSound3DVel] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

////////////////////////////////////////////////////////
// Function: sndsPlatformSetSound3DMode
// Description: Set the sound mode.
////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetSound3DMode(snSound* s, UINT32 uApply)
{
#if XAUDIO
#else
    HRESULT hr = 0;
    LPDIRECTSOUND3DBUFFER8 lpDS3D = nullptr;

    if(s == nullptr) return FALSE;

    if(!s->bIs3D) return TRUE;

    if(s->lpdsb == nullptr) return FALSE;

    hr = s->lpdsb->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&lpDS3D);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetSound3DMode] **FAILED**");
        return FALSE;
    }

    hr = lpDS3D->SetMode(s->uMode, uApply);

    lpDS3D->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetSound3DMode] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformCommitDeferredSettings
// Description: Commit deferred settings for the listener.
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformCommitDeferredSettings()
{
#if XAUDIO
#else
    HRESULT hr;

    if(g_lpListener8 == nullptr)
        return FALSE;

    hr = g_lpListener8->CommitDeferredSettings();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformCommitDeferredSettings] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetMaxDistance
// Description: Set maximum distance for sound buffer.
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetMaxDistance(snSound* s, UINT32 uApply)
{
#if XAUDIO
    return TRUE;
#else
    HRESULT hr;
    LPDIRECTSOUND3DBUFFER8 lpDS3D = nullptr;

    if(s == nullptr) return FALSE;

    if(!s->bIs3D) return TRUE;

    if(s->lpdsb == nullptr) return FALSE;

    hr = s->lpdsb->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&lpDS3D);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetMaxDistance] **FAILED**");
        return FALSE;
    }

    hr = lpDS3D->SetMaxDistance(s->fMaxDistance, uApply);

    lpDS3D->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetMaxDistance] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetMinDistance
// Description: Set minimum distance for sound buffer.
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetMinDistance(snSound* s, UINT32 uApply)
{
#if XAUDIO
    return TRUE;
#else
    HRESULT hr;
    LPDIRECTSOUND3DBUFFER8 lpDS3D = nullptr;

    if(s == nullptr) return FALSE;

    if(!s->bIs3D) return TRUE;

    if(s->lpdsb == nullptr) return FALSE;

    hr = s->lpdsb->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&lpDS3D);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetMinDistance] **FAILED**");
        return FALSE;
    }

    hr = lpDS3D->SetMinDistance(s->fMinDistance, uApply);

    lpDS3D->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetMinDistance] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetConeAngles
// Description: Set inside and outside volume cone angles.
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetConeAngles(snSound* sound, UINT32 uApply)
{
#if XAUDIO
#else
    HRESULT hr;
    LPDIRECTSOUND3DBUFFER8 lpDS3D = nullptr;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    hr = sound->lpdsb->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&lpDS3D);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetConeAngles] **FAILED**");
        return FALSE;
    }

    hr = lpDS3D->SetConeAngles(sound->uInsideConeAngle,
                               sound->uOutsideConeAngle,
                               uApply);

    lpDS3D->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetConeAngles] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetConeOrientation
// Description: Set inside and outside volume cone angles.
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetConeOrientation(snSound* sound, UINT32 uApply)
{
#if XAUDIO
#else
    HRESULT hr;
    LPDIRECTSOUND3DBUFFER8 lpDS3D = nullptr;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    hr = sound->lpdsb->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&lpDS3D);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetConeOrientation] **FAILED**");
        return FALSE;
    }

    hr = lpDS3D->SetConeOrientation(sound->vecConeOrientation.x,
                                    sound->vecConeOrientation.y,
                                    sound->vecConeOrientation.z,
                                    uApply);

    lpDS3D->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetConeOrientation] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformGetVolume
// Description: Return volume for specific sound buffer.
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformGetVolume(snSound* sound, SINT32* pVolume)
{
#if XAUDIO
    //AudioGetVolume(sound->handle, volume);
    return TRUE;
#else
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;
    HRESULT hr;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;
    if(pVolume == nullptr) return FALSE;

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformGetVolume] **FAILED**");
        return FALSE;
    }

    hr = lpDSB->GetVolume((LPLONG)pVolume);

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformGetVolume] **FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetVolume
// Description:
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetVolume(snSound* sound)
{
#if XAUDIO
    if(sound == nullptr)
        return FALSE;

    AudioSetVolume(*sound);
    return TRUE;
#else
    SINT32 iDecibel = 0;
    FLOAT64 fuseVol = 0.0f;
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;
    HRESULT hr;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetVolume] **FAILED**");
        return FALSE;
    }

    fuseVol = sound->volume;

    if(sound->use3DVolume)
    {
        fuseVol *= sound->volume3D;
    }

    fuseVol *= (FLOAT64)VolumeMultiplier;

    fuseVol = fuseVol < 0.00001f ? 0.00001f : fuseVol;

    iDecibel = (SINT32)(1000.0 * (log10(fuseVol)));

    if(iDecibel < DSBVOLUME_MIN || iDecibel > DSBVOLUME_MAX)
    {
        lpDSB->Release();
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetVolume] **volume out of range**");
        return FALSE;
    }

    hr = lpDSB->SetVolume(iDecibel);

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetVolume] **FAILED**");
        return FALSE;
    }
    return TRUE;
#endif
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformIsPlaying
// Description:
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformIsPlaying(snSound* sound)
{
#if XAUDIO
    if(sound)
        return AudioIsPlaying(*sound);

    return FALSE;
#else
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;
    HRESULT hr;
    DWORD dwStatus = 0;

	// Check lpds nullptr which is set on program kill. This may be called after application
	// exit by UI and results in a crash.
	if(!lpds || !sound || !sound->lpdsb)
	{
		return FALSE;
	}

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformIsPlaying] QueryInterface** FAILED**");
        return FALSE;
    }

    hr = lpDSB->GetStatus(&dwStatus);

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformIsPlaying] GetStatus** FAILED**");
        return FALSE;
    }

    if(dwStatus & DSBSTATUS_PLAYING)
    {
        return TRUE;
    }
    return FALSE;
#endif
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetCurrentPosition
// Description:
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetCurrentPosition(snSound* sound)
{
#if XAUDIO
#else
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;
    HRESULT hr = 0;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetCurrentPosition] QueryInterface** FAILED**");
        return FALSE;
    }

    hr  = lpDSB->SetCurrentPosition(sound->uCurrentPosition);

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetCurrentPosition] SetCurrentPosition** FAILED**");
        return FALSE;
    }
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetFrequency
// Description:
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetFrequency(snSound* sound)
{
#if XAUDIO
    if(sound)
    {
        if(sound->freqShift < XAUDIO2_MIN_FREQ_RATIO || sound->freqShift > XAUDIO2_DEFAULT_FREQ_RATIO)
        {
		    Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetFrequency] Frequency %d outside limits", sound->freqShift);
        return FALSE;
    }

        AudioSetPitch(*sound);

        return TRUE;
    }

    return FALSE;//AudioSetPitch(sound->handle, sound->freqShift);
#else
    DWORD dwFreq = 0;
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;
    HRESULT hr = 0;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetFrequency] QueryInterface** FAILED**");
        return FALSE;
    }

    dwFreq = (DWORD)(sound->freqShift * sound->frequency);

    if(sound->use3DFreqShift)
	{
        dwFreq = (DWORD)(dwFreq * sound->freqShift3D);
	}

    if(dwFreq < DSBFREQUENCY_MIN || dwFreq > DSBFREQUENCY_MAX)
    {
        lpDSB->Release();
		Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetFrequency] Frequency %d outside limits", dwFreq);
        return FALSE;
    }

    hr = lpDSB->SetFrequency(dwFreq);

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetFrequency] SetFrequency** FAILED**");
        return FALSE;
    }
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetFrequency
// Description:
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformStopSound(snSound* sound)
{
#if XAUDIO
    if(sound == nullptr)
        return FALSE;

    return AudioStop(sound->handle);
#else
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;
    HRESULT hr = 0;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformStopSound] QueryInterface** FAILED**");
        return FALSE;
    }

    hr = lpDSB->Stop();

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformStopSound] Stop** FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformStartSound
// Description:
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformStartSound(snSound* sound)
{
#if XAUDIO
    if(sound == nullptr)
        return FALSE;

    return AudioPlay(*sound);
#else
    DWORD dwPlayFlags = 0;
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;
    HRESULT hr = 0;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformStartSound] QueryInterface** FAILED**");
        return FALSE;
    }

    if(sound->looping)
    {
        dwPlayFlags |= DSBPLAY_LOOPING;
    }

    hr = lpDSB->Play(0, 0, dwPlayFlags);

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformStartSound] Play** FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////
// Function: sndsPlatformSetPanPosition
// Description:
//////////////////////////////////////////////////////////
LOGICAL sndsPlatformSetPanPosition(snSound* sound)
{
#if XAUDIO
#else
    FLOAT64	fUsePan;
    SINT32  iDSPan;
    LOGICAL bLeftPan = FALSE;
    HRESULT hr = 0;
    LPDIRECTSOUNDBUFFER8 lpDSB = nullptr;

    if(sound == nullptr) return FALSE;
    if(sound->lpdsb == nullptr) return FALSE;

    if(sound->use3DPan)
    {
        fUsePan = (FLOAT64)sound->pan3D;
    }
    else
    {
        fUsePan = (FLOAT64)sound->pan;
    }

    // Ranges from -1.0f (left) to 1.0f (right)
    fUsePan = (fUsePan - 50.0f) / 50.0f;

    if(fUsePan < 0.0f)
    {
        bLeftPan = TRUE;
        fUsePan = -fUsePan;
    }

    fUsePan = 1.0f - fUsePan;

    fUsePan *= (FLOAT64)VolumeMultiplier;

    if(fUsePan < 0.00001f)
        fUsePan = 0.00001f;

    fUsePan = 1000.0f * log10(fUsePan);

    if(bLeftPan == FALSE)
        fUsePan = -fUsePan;

    iDSPan = (SINT32)fUsePan;

    if(iDSPan <= DSBPAN_LEFT || iDSPan >= DSBPAN_RIGHT)
    {
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetPanPosition] **FAILED** pan outside limits");
        return FALSE;
    }

    /////////////////////////////////////////////////////////////////
    // If this fails, make sure you are not using a duplicate sound.
    // DuplicateSoundBuffer will not allow access to the interface.
    /////////////////////////////////////////////////////////////////
    hr = sound->lpdsb->QueryInterface(IID_IDirectSoundBuffer8, (void**)&lpDSB);

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetPanPosition] QueryInterface** FAILED**");
        return FALSE;
    }

    hr = lpDSB->SetPan(iDSPan);

    lpDSB->Release();

    if(hr != DS_OK)
    {
        DDError(hr);
        Write_Log(LOG_SOUND, LOG_WARN, "[sndsPlatformSetPanPosition] SetPan** FAILED**");
        return FALSE;
    }
#endif
    return TRUE;
}
