#pragma once

extern SINT32 obmeshCreate(SINT32 iNumVertsAlongX,
                    SINT32 iNumVertsAlongY,
                    FLOAT32 fLengthAlongX,
                    FLOAT32 fLengthAlongY,
                    FLOAT32 u1,
                    FLOAT32 v1,
                    FLOAT32 u2,
                    FLOAT32 v2,
                    struct piLVERTEX** lpVertices);
