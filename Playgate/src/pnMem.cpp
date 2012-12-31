// $Id: pnMem.cpp 102 2009-01-09 20:42:45Z osmith@CRS $
// Stub 'mem' functions that we need to provide
// since we don't really want all of the usual
// memory management palava.

extern "C" void *memAllocDebug(SINT32 Size, SINT8 *file, SINT32 line)
{
	return calloc(1, Size) ;
}

extern "C" void memFreeDebug(void *Pointer, SINT8 *file, SINT32 line)
{
	free(Pointer) ;
}

extern "C" void *memAllocRelease(SINT32 Size)
{
	return calloc(1, Size) ;
}

extern "C" void memFreeRelease(void *Pointer)
{
	free(Pointer) ;
}
