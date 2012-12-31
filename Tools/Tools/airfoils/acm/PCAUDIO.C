/*  This routine will initialize a new direct sound buffer,
    set the data in the buffer, 
    set the rate, format, etc...

    Input:
    pFileInfo   -   Pointer to file info with all
    nessecary info filled, 
    like pbData, cbData, etc...

    Output:
    0 if successful, else the error code.

*/

int NewDirectSoundBuffer(
            FILEINFO *pFileInfo
            )
{

    DSBUFFERDESC        dsbd;
    DSBCAPS         dsbc;
    HRESULT         hr;
    BYTE            *pbData         = NULL;
    BYTE            *pbData2        = NULL;
    DWORD           dwLength;
    DWORD           dwLength2;

    // Set up the direct sound buffer. 
    memset(&dsbd, 0, sizeof(DSBUFFERDESC));
    dsbd.dwSize                 = sizeof(DSBUFFERDESC);
    dsbd.dwFlags                = 0;
    dsbd.dwFlags                |= DSBCAPS_STATIC;
    // Use new GetCurrentPosition() accuracy (DirectX 2 feature)
    dsbd.dwFlags                |= DSBCAPS_CTRLDEFAULT | DSBCAPS_GETCURRENTPOSITION2;
    if (pFileInfo->fSticky)
        dsbd.dwFlags |= DSBCAPS_STICKYFOCUS;
    dsbd.dwBufferBytes               = pFileInfo->cbSize;
    dsbd.lpwfxFormat            = pFileInfo->pwfx;
    if ((hr = gpds->lpVtbl->CreateSoundBuffer(gpds,
              &dsbd,
              &(pFileInfo->pDSB),
              NULL )) != 0)
    {
    goto ERROR_IN_ROUTINE;
    }

    // Ok, lock the sucker down, and copy the memory to it.
    if ((hr = pFileInfo->pDSB->lpVtbl->Lock(pFileInfo->pDSB,
            0,
            pFileInfo->cbSize,
            &pbData,
            &dwLength,
            &pbData2,
            &dwLength2,
                        0L)) != 0)
    {
    goto ERROR_IN_ROUTINE;
    }

    Assert(pbData != NULL);
    memcpy(pbData, pFileInfo->pbData, pFileInfo->cbSize);

    // Ok, now unlock the buffer, we don't need it anymore.
    if ((hr = pFileInfo->pDSB->lpVtbl->Unlock(pFileInfo->pDSB,
                          pbData, pFileInfo->cbSize,
                          NULL, 0)) != 0)
    {
    goto ERROR_IN_ROUTINE;
    }

    pbData = NULL;

    if ((hr = pFileInfo->pDSB->lpVtbl->SetVolume(pFileInfo->pDSB,
        MAXVOL_VAL)) != 0)
    {
    goto ERROR_IN_ROUTINE;
    }

    if ((hr = pFileInfo->pDSB->lpVtbl->SetPan(pFileInfo->pDSB,
        MIDPAN_VAL)) != 0)
    {
    goto ERROR_IN_ROUTINE;
    }

    dsbc.dwSize = sizeof(dsbc);
    if (hr = IDirectSoundBuffer_GetCaps(pFileInfo->pDSB, &dsbc))
    {
    goto ERROR_IN_ROUTINE;
    }

    if (dsbc.dwFlags & DSBCAPS_LOCHARDWARE) {
    pFileInfo->fHardware = TRUE;
    } else {
    pFileInfo->fHardware = FALSE;
    }

    goto DONE_ROUTINE;

ERROR_IN_ROUTINE:
    if (pbData != NULL)
    {
    hr = pFileInfo->pDSB->lpVtbl->Unlock(pFileInfo->pDSB, pbData,
                        pFileInfo->cbSize, NULL, 0);
    pbData = NULL;
    }

    if (pFileInfo->pDSB != NULL)
    {
    pFileInfo->pDSB->lpVtbl->Release(pFileInfo->pDSB);
    pFileInfo->pDSB = NULL;
    }
    
DONE_ROUTINE:

    return(hr); 

}

/*  This routine will release a direct sound buffer,
    freeing up memory, resources, 
    whatever.

    Input:
    pFileInfo   -   Pointer to the file info,
        with the proper stuff set.

    Output: 
    0 if successful, else the error code.

*/
int ReleaseDirectSoundBuffer( FILEINFO *pFileInfo )
{

    if (pFileInfo->pDSB != NULL)
    {
    pFileInfo->pDSB->lpVtbl->Release(pFileInfo->pDSB);
    pFileInfo->pDSB = NULL; 
    }

    return(0);


