#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include <io.h>
#include <mmsystem.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "fio/fio.h"
#include "mem/mem.h"
#include "ld/ldwav.h"


BOOL ldwavParse(BYTE* infile, BYTE* outbuffer, WAVEFORMATEX* WaveFormat, DWORD* length)
{
HMMIO         hmmioIn=NULL;
MMCKINFO      ckInRIFF;
MMCKINFO      ckIn;
MMIOINFO      mmioinfo;
BOOL          retval=TRUE;
DWORD         howmuch;

  do
	{
		/*
		.	make MMIO read from memory buffer instead of file
		*/
		memset((void far*)&mmioinfo, 0, sizeof(MMIOINFO));
		mmioinfo.pchBuffer = (HPSTR)infile;
		mmioinfo.fccIOProc = FOURCC_MEM;
		mmioinfo.cchBuffer = *length;
		mmioinfo.adwInfo[0] = 1024;

		/*
		. Open the input file for reading using buffered I/O.
		*/
		hmmioIn = mmioOpen(NULL, &mmioinfo, MMIO_READ);
		if (hmmioIn == NULL)
		{
			retval = FALSE;
			break;
		}

		/*
		 . Descend the input file into the 'RIFF' chunk.
		*/
		if (mmioDescend(hmmioIn, &ckInRIFF, NULL, 0) != 0)
		{
			retval = FALSE;
			break;
		}

		/*
		. Make sure the input file is a WAVE file.
		*/
		if ((ckInRIFF.ckid != FOURCC_RIFF) || (ckInRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
		{
			retval = FALSE;
			break;
		}

		/*
		. Search the input file for for the 'fmt ' chunk.
		*/
		ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
		if (mmioDescend(hmmioIn, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0)
		{
			retval = FALSE;
			break;
		}

		if (WaveFormat != NULL)
		{
      memset(WaveFormat, 0, sizeof(WAVEFORMATEX));

			/*
			 . Expect the 'fmt' chunk to be at least as large as WaveFormat;
			 . if there are extra parameters at the end, we'll ignore them
			*/
			if (ckIn.cksize < (long) sizeof(PCMWAVEFORMAT))
			{
				retval = FALSE;
				break;
			}

			/*
			 . Read the 'fmt ' chunk into pcmWaveFormat.
			*/
			if (mmioRead(hmmioIn, (HPSTR) WaveFormat, (long) sizeof(PCMWAVEFORMAT)) != (long) sizeof(PCMWAVEFORMAT))
			{
				retval = FALSE;
				break;
			}
		}

		/*
		 . Ascend the input file out of the 'fmt ' chunk.
		*/
		if (mmioAscend(hmmioIn, &ckIn, 0) != 0)
		{
			retval = FALSE;
			break;
		}

		/*
		 . Search the input file for the 'data' chunk.
		*/
		ckIn.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if (mmioDescend(hmmioIn, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0)
		{
			retval = FALSE;
			break;
		}

		/*
		 . User wants to know how much memory to allocate
		 */
		if (outbuffer == NULL)
		{
			break;
		}

		/* read the data */
		howmuch = mmioRead(hmmioIn, (char*)outbuffer, ckIn.cksize);
		if (ckIn.cksize != (DWORD)howmuch)
		{
			retval = FALSE;
			break;
		}
	}
	while (FALSE);

	/*
	 . Close the file
	*/
	if (hmmioIn != NULL)
	{
		mmioClose(hmmioIn, 0);
	}

	if (!retval)
	{
    Write_Debug("Could not read the .WAVE file %s", infile);
	}
	else
	{
		/*
		 . Return the length of the wave data
		 */
		*length = ckIn.cksize;
	}

	return(retval);
}

BOOL ldwavLoadFile(char* filename, BYTE** outbuffer, WAVEFORMATEX* WaveFormat, DWORD* length)
{
	char fullpath[MAX_PATH];
	BOOL    retval;
	BYTE*    inriff, *outwave;
	int     stream;
	size_t	filesize;

	lstrcpy(fullpath, filename);

    // For external developement of sound testing
#ifdef BETA_RELEASE
  {
    // Check if there is a sound directory first
    SINT32 fioSwitch = fioQuery();

    if(fioSwitch == ISARCHIVE)
    {
        fioSet(ISDIRECTORY);
        stream = fioOpen(filename);

        if(stream == -1)
        {
            Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] loading %s from archive", filename);
            fioSet(ISARCHIVE);
            stream = fioOpen(filename);
        }
        else
        {
            Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] loading %s from directory", filename);
        }
    }
    else
    {
        Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] loading %s from directory", filename);
        stream = fioOpen(filename);
    }

    fioSet(fioSwitch);
  }
#else
  stream = fioOpen(filename);
#endif

  if (stream == -1)
  {
    Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] FAILED to load %s", filename);
    return(FALSE);
  }

  filesize = fioSeek(stream, 0, SEEK_END);
  fioSeek(stream, 0, SEEK_SET);
  inriff = (BYTE*)memAlloc(filesize);
  if (inriff == NULL)
  {
    fioClose(stream);
    Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] FAILED to load %s", filename);
    return(FALSE);
  }

  if (filesize != fioRead(stream, inriff, filesize))
  {
    fioClose(stream);
    memFree(inriff);
    Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] FAILED to load %s", filename);
    return(FALSE);
  }

  fioClose(stream);

  *length = (DWORD)filesize;

  if (!ldwavParse(inriff, NULL, NULL, (DWORD*)&filesize))
  {
    memFree(inriff);
    Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] FAILED to load %s", filename);
    return(FALSE);
  }

  outwave = (BYTE*)memAlloc(filesize);
  if (outwave == NULL)
  {
    memFree(inriff);
    Write_Log(LOG_SOUND, LOG_INFO, "[ldwavLoadFile] FAILED to load %s", filename);
    return(FALSE);
  }

  retval = ldwavParse(inriff, outwave, WaveFormat, length);

  memFree(inriff);
  if (retval == FALSE)
  {
    memFree(outwave);
  }
  else
  {
    *outbuffer = outwave;
  }

  return(retval);
}
