#pragma once

#define FORCE_DDS_NO_FLIP
//#undef FORCE_DDS_NO_FLIP

// For legacy texture creation on mdl loads
extern SINT32 ldddsLoadDDSTexture2D(const SCHAR* name, UINT32 transColor, UINT32 flags);
// DDS texture loading
extern SINT32 ldddsLoadDDSTexture(const SCHAR* name, GLenum target, UINT32 flags);
extern SINT32 ldddsCopyDDSTexture(SINT32 from, SCHAR* to, UINT32 flags);

extern void ldddsTextureKill(TEXTURE* t);

extern LOGICAL ldddsGenerateTexture(SINT32 texid);

