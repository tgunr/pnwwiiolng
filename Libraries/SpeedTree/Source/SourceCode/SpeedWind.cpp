///////////////////////////////////////////////////////////////////////  
//  SpeedWind.cpp
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2008 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com


/////////////////////////////////////////////////////////////////////////////
// Preprocessor

#include "SpeedTreeRT.h"
#include "LibGlobals_Source/IdvGlobals.h"
#include "Debug.h"
#include "LibVector_Source/IdvVector.h"
#include "UpVector.h"
using namespace std;


/////////////////////////////////////////////////////////////////////////////
// SpeedTree memory management
//
//  Used to capture heap allocations in STL containers to report back to
//  SpeedTreeRT's allocation mechanism

// define vector<stTransform>
DefineAllocator(st_allocator_stMatrix);
typedef std::vector<stTransform, st_allocator_stMatrix<stTransform> > st_vector_stMatrix;


/////////////////////////////////////////////////////////////////////////////
// class CSpeedWindDef

class CSpeedWindDef
{
public:
                                        CSpeedWindDef(CSpeedWind* pFather);
        virtual                         ~CSpeedWindDef( );

        // parameter setting
        void                            Reset(void);
        void                            SetQuantities(int nNumWindMatrices, int nNumLeafAngles);
        void                            SetWindResponse(float fResponse, float fReponseLimit);
        void                            SetWindStrengthAndDirection(float fWindStrength, float fWindDirectionX, float fWindDirectionY, float fWindDirectionZ);
        void                            SetMaxBendAngle(float fMaxBendAngle);
        void                            SetExponents(float fBranchExponent, float fLeafExponent);
        void                            SetGusting(float fGustStrengthMin, float fGustStrengthMax, float fGustFrequency, float fGustDurationMin, float fGustDurationMax);
        void                            SetBranchHorizontal(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);
        void                            SetBranchVertical(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);
        void                            SetLeafRocking(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);
        void                            SetLeafRustling(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);

        // parameter getting
        void                            GetQuantities(int& iNumWindMatrices, int& iNumLeafAngles) const;
        void                            GetWindResponse(float& fResponse, float& fReponseLimit) const;
        void                            GetWindStrengthAndDirection(float& fWindStrength, float& fWindDirectionX, float& fWindDirectionY, float& fWindDirectionZ) const;
        float                           GetMaxBendAngle(void) const;
        void                            GetExponents(float& fBranchExponent, float& fLeafExponent) const;
        void                            GetGusting(float& fGustStrengthMin, float& fGustStrengthMax, float& fGustFrequency, float& fGustDurationMin, float& fGustDurationMax) const;
        void                            GetBranchHorizontal(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;
        void                            GetBranchVertical(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;
        void                            GetLeafRocking(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;
        void                            GetLeafRustling(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;
        float                           GetTime(void) const;

        // render interface
        void                            Advance(float fCurrentTime, bool bUpdateMatrices = true, bool bUpdateLeafAngleMatrices = false, float fCameraX = 0.0f, float fCameraY = 0.0f, float fCameraZ = 1.0f);
        void                            UpdateSpeedTreeRT(void) const;
        void                            UpdateTree(CSpeedTreeRT* pTree) const;
        float                           GetFinalStrength(void);
        unsigned int                    GetNumWindMatrices(void) const;
        const float*                    GetWindMatrix(unsigned int uiIndex) const;
        unsigned int                    GetNumLeafAngleMatrices(void) const;
        const float*                    GetLeafAngleMatrix(unsigned int uiIndex) const;
        bool                            GetRustleAngles(float* pAngles) const;
        bool                            GetRockAngles(float* pAngles) const;

        // file I/O
        bool                            Load(const st_string& strFilename);
        bool                            Load(const char* pData, unsigned int uiNumBytes);
        bool                            Save(const st_string& strFilename) const;
        const char*                     Save(unsigned int& uiNumBytes) const;
        const char*                     GetFilename(void) const;

        // blending SpeedWinds into this one
        void                            InterpolateTarget(const CSpeedWind* pWind1, const CSpeedWind* pWind2, float fInterpolation);
        void                            InterpolateCurrent(const CSpeedWind* pWind1, const CSpeedWind* pWind2, float fInterpolation, bool bUpdateMatrices = true, bool bUpdateLeafAngleMatrices = false, float fCameraX = 0.0f, float fCameraY = 0.0f, float fCameraZ = 1.0f);
        void                            BlendParameters(const CSpeedWind** pWinds, const float* pWeights, unsigned int uiNumWinds);

        // internal use only
        void                            SetUseRandomBuffer(bool bUse, unsigned int uiSeed);

private:
        int                             Random(void);

        CSpeedWind*                     m_pFather;

        struct SOscillationParams
        {
                                        SOscillationParams(float fLowWindAngle = 10.0f, float fHighWindAngle = 4.0f, float fLowWindSpeed = 0.01f, float fHighWindSpeed = 0.3f);

                void                    Advance(float fDeltaTime, float fWindStrendth);
                void                    Reset(void) { m_fAdjustedTime = 0.0f; }

                float                   m_fLowWindAngle;
                float                   m_fHighWindAngle;
                float                   m_fLowWindSpeed;
                float                   m_fHighWindSpeed;
                float                   m_fAdjustedTime;
                st_vector_float         m_vOutputs;
        };

        class CSpeedWindController
        {
        public:
                                        CSpeedWindController( );

                void                    Advance(float fDeltaTime, float fP, float fMaxA);
                void                    Reset(float fValue);

                float                   m_fWantedValue;
                float                   m_fCurrentValue;
                float                   m_fLastDelta;
                float                   m_fLastDeltaTime;
        };

        // SpeedWind parameters
        SOscillationParams              m_sBranchHorizontal;
        SOscillationParams              m_sBranchVertical;
        SOscillationParams              m_sLeafRocking;
        SOscillationParams              m_sLeafRustling;
        float                           m_fMaxBendAngle;
        float                           m_fWindResponse;
        float                           m_fWindResponseLimit;
        float                           m_fGustingStrengthMin;
        float                           m_fGustingStrengthMax;
        float                           m_fGustingFrequency;
        float                           m_fGustingDurationMin;
        float                           m_fGustingDurationMax;
        float                           m_fBranchExponent;
        float                           m_fLeafExponent;

        // SpeedWind internals
        float                           m_fLastTime;
        float                           m_fWindStrength;
        float                           m_fCurrentGustingStrength;
        float                           m_fGustStopTime;

        // SpeedWind outputs
        CSpeedWindController            m_cFinalWindStrength;
        CSpeedWindController            m_cFinalWindDirectionX;
        CSpeedWindController            m_cFinalWindDirectionY;
        CSpeedWindController            m_cFinalWindDirectionZ;
        float                           m_fFinalBendAngle;

        st_vector_stMatrix              m_vWindMatrices;
        st_vector_stMatrix              m_vLeafAngleMatrices;

        // offline rendering support
        st_vector_int                   m_vRandomBuffer;
        int                             m_nRandomIndex;
        bool                            m_bUseRandomBuffer;

        // other
        st_string                       m_strFilename;
static  const int                       c_nRandomBufferSize;
};


/////////////////////////////////////////////////////////////////////////////
// constants

const int CSpeedWindDef::c_nRandomBufferSize = 1024;


/////////////////////////////////////////////////////////////////////////////
//  CSpeedWind redirects

CSpeedWind::CSpeedWind( ) { m_pDefinition = new ("CSpeedWind::CSpeedWindDef") CSpeedWindDef(this); }
CSpeedWind::~CSpeedWind( ) { st_delete<CSpeedWindDef>(m_pDefinition, "CSpeedWind::CSpeedWindDef"); }
CSpeedWind::CSpeedWind(const CSpeedWind& cCopy)
{
    m_pDefinition = new ("CSpeedWind::CSpeedWindDef") CSpeedWindDef(this);
    *m_pDefinition = *cCopy.m_pDefinition;
}

// operators
CSpeedWind& CSpeedWind::operator=(const CSpeedWind& cRight)
{
    *m_pDefinition = *cRight.m_pDefinition;

    return *this;
}

// mutators
void CSpeedWind::Reset(void) { m_pDefinition->Reset( ); }
void CSpeedWind::SetQuantities(int nNumWindMatrices, int nNumLeafAngles) { m_pDefinition->SetQuantities(nNumWindMatrices, nNumLeafAngles); }
void CSpeedWind::SetWindResponse(float fResponse, float fReponseLimit) { m_pDefinition->SetWindResponse(fResponse, fReponseLimit); }
void CSpeedWind::SetWindStrengthAndDirection(float fWindStrength, float fWindDirectionX, float fWindDirectionY, float fWindDirectionZ) 
    { m_pDefinition->SetWindStrengthAndDirection(fWindStrength, fWindDirectionX, fWindDirectionY, fWindDirectionZ); }
void CSpeedWind::SetMaxBendAngle(float fMaxBendAngle) { m_pDefinition->SetMaxBendAngle(fMaxBendAngle); }
void CSpeedWind::SetExponents(float fBranchExponent, float fLeafExponent) { m_pDefinition->SetExponents(fBranchExponent, fLeafExponent); }
void CSpeedWind::SetGusting(float fGustStrengthMin, float fGustStrengthMax, float fGustFrequency, float fGustDurationMin, float fGustDurationMax) 
    { m_pDefinition->SetGusting(fGustStrengthMin, fGustStrengthMax, fGustFrequency, fGustDurationMin, fGustDurationMax); }
void CSpeedWind::SetBranchHorizontal(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetBranchHorizontal(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::SetBranchVertical(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetBranchVertical(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::SetLeafRocking(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetLeafRocking(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::SetLeafRustling(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetLeafRustling(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }

// inspectors
void CSpeedWind::GetQuantities(int& nNumWindMatrices, int& nNumLeafAngles) const { m_pDefinition->GetQuantities(nNumWindMatrices, nNumLeafAngles); }
void CSpeedWind::GetWindResponse(float& fResponse, float& fReponseLimit) const { m_pDefinition->GetWindResponse(fResponse, fReponseLimit); }
void CSpeedWind::GetWindStrengthAndDirection(float& fWindStrength, float& fWindDirectionX, float& fWindDirectionY, float& fWindDirectionZ) const
    { m_pDefinition->GetWindStrengthAndDirection(fWindStrength, fWindDirectionX, fWindDirectionY, fWindDirectionZ); }
float CSpeedWind::GetMaxBendAngle(void) const { return m_pDefinition->GetMaxBendAngle( ); }
void CSpeedWind::GetExponents(float& fBranchExponent, float& fLeafExponent) const { m_pDefinition->GetExponents(fBranchExponent, fLeafExponent); }
void CSpeedWind::GetGusting(float& fGustStrengthMin, float& fGustStrengthMax, float& fGustFrequency, float& fGustDurationMin, float& fGustDurationMax) const 
    { m_pDefinition->GetGusting(fGustStrengthMin, fGustStrengthMax, fGustFrequency, fGustDurationMin, fGustDurationMax); }
void CSpeedWind::GetBranchHorizontal(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const 
    { m_pDefinition->GetBranchHorizontal(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::GetBranchVertical(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const 
    { m_pDefinition->GetBranchVertical(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::GetLeafRocking(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const 
    { m_pDefinition->GetLeafRocking(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::GetLeafRustling(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
    { m_pDefinition->GetLeafRustling(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
float CSpeedWind::GetTime(void) const
    { return m_pDefinition->GetTime( ); }

// render interface
void CSpeedWind::Advance(float fCurrentTime, bool bUpdateMatrices, bool bUpdateLeafAngleMatrices, float fCameraX, float fCameraY, float fCameraZ) 
    { m_pDefinition->Advance(fCurrentTime, bUpdateMatrices, bUpdateLeafAngleMatrices, fCameraX, fCameraY, fCameraZ); }
void CSpeedWind::UpdateSpeedTreeRT(void) const { m_pDefinition->UpdateSpeedTreeRT( ); }
void CSpeedWind::UpdateTree(CSpeedTreeRT* pTree) const { m_pDefinition->UpdateTree(pTree); }
float CSpeedWind::GetFinalStrength(void) { return m_pDefinition->GetFinalStrength( ); }
unsigned int CSpeedWind::GetNumWindMatrices(void) const { return m_pDefinition->GetNumWindMatrices( ); }
const float* CSpeedWind::GetWindMatrix(unsigned int uiIndex) const { return m_pDefinition->GetWindMatrix(uiIndex); }
unsigned int CSpeedWind::GetNumLeafAngleMatrices(void) const { return m_pDefinition->GetNumLeafAngleMatrices( ); }
const float* CSpeedWind::GetLeafAngleMatrix(unsigned int uiIndex) const { return m_pDefinition->GetLeafAngleMatrix(uiIndex); }
bool CSpeedWind::GetRustleAngles(float* pAngles) const { return m_pDefinition->GetRustleAngles(pAngles); }
bool CSpeedWind::GetRockAngles(float* pAngles) const { return m_pDefinition->GetRockAngles(pAngles); }

// file I/O
bool CSpeedWind::Load(const char* pFilename) { return m_pDefinition->Load(pFilename); }
bool CSpeedWind::Load(const char* pData, unsigned int uiNumBytes) { return m_pDefinition->Load(pData, uiNumBytes); }
bool CSpeedWind::Save(const char* pFilename) const { return m_pDefinition->Save(pFilename); }
const char* CSpeedWind::Save(unsigned int& uiNumBytes) const { return m_pDefinition->Save(uiNumBytes); }
const char* CSpeedWind::GetFilename(void) const { return m_pDefinition->GetFilename( ); }

// blending SpeedWinds into this one
void CSpeedWind::InterpolateTarget(const CSpeedWind* pWind1, const CSpeedWind* pWind2, float fInterpolation) { m_pDefinition->InterpolateTarget(pWind1, pWind2, fInterpolation); }
void CSpeedWind::InterpolateCurrent(const CSpeedWind* pWind1, const CSpeedWind* pWind2, float fInterpolation, bool bUpdateMatrices, bool bUpdateLeafAngleMatrices, float fCameraX, float fCameraY, float fCameraZ) 
    { m_pDefinition->InterpolateCurrent(pWind1, pWind2, fInterpolation, bUpdateMatrices, bUpdateLeafAngleMatrices, fCameraX, fCameraY, fCameraZ); }
void CSpeedWind::BlendParameters(const CSpeedWind** pWinds, const float* pWeights, unsigned int uiNumWinds) { m_pDefinition->BlendParameters(pWinds, pWeights, uiNumWinds); }

// offline rendering support
void CSpeedWind::SetUseRandomBuffer(bool bUse, unsigned int uiSeed) { m_pDefinition->SetUseRandomBuffer(bUse, uiSeed); }


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindController::CSpeedWindController

CSpeedWindDef::CSpeedWindController::CSpeedWindController(void) :
    m_fWantedValue(0.0f),
    m_fCurrentValue(0.0f),
    m_fLastDelta(0.0f),
    m_fLastDeltaTime(0.0f)
{
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindController::Advance

void CSpeedWindDef::CSpeedWindController::Advance(float fDeltaTime, float fP, float fMaxA)
{
    const float c_fMaxControllableDeltaTime = 0.03f;

    if (fDeltaTime > c_fMaxControllableDeltaTime)
        fDeltaTime = c_fMaxControllableDeltaTime;

    float fDelta = fP * fDeltaTime * (m_fWantedValue - m_fCurrentValue);

    if (fMaxA != 0.0f)
    {
        if (fabs(fDelta) > fabs(m_fLastDelta) && fDeltaTime > 0.0f && m_fLastDeltaTime > 0.0f)
        {
            float fAccel = fDelta / fDeltaTime - m_fLastDelta / m_fLastDeltaTime;
            if (fAccel < -fMaxA)
                fAccel = -fMaxA;
            if (fAccel > fMaxA)
                fAccel = fMaxA;

            fDelta = m_fLastDelta + fAccel * fDeltaTime;
        }

        m_fLastDelta = fDelta;
        m_fLastDeltaTime = fDeltaTime;
    }

    m_fCurrentValue += fDelta;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindController::Reset

void CSpeedWindDef::CSpeedWindController::Reset(float fValue)
{
    m_fCurrentValue = fValue;
    m_fWantedValue = fValue;
    m_fLastDelta = 0.0f;
    m_fLastDeltaTime = 0.0f;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SOscillationParams::SOscillationParams

CSpeedWindDef::SOscillationParams::SOscillationParams(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) :
    m_fLowWindAngle(fLowWindAngle),
    m_fHighWindAngle(fHighWindAngle),
    m_fLowWindSpeed(fLowWindSpeed),
    m_fHighWindSpeed(fHighWindSpeed),
    m_fAdjustedTime(0.0f)
{
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SOscillationParams::Advance

void CSpeedWindDef::SOscillationParams::Advance(float fDeltaTime, float fWindStrendth)
{
    float fAngle = VecInterpolate(m_fLowWindAngle, m_fHighWindAngle, fWindStrendth);
    float fSpeed = VecInterpolate(m_fLowWindSpeed, m_fHighWindSpeed, fWindStrendth);

    m_fAdjustedTime += fDeltaTime * fSpeed;

    unsigned int uiSize = (unsigned int) m_vOutputs.size( );
    for (unsigned int i = 0; i < uiSize; ++i)
        m_vOutputs[i] = fAngle * sinf(m_fAdjustedTime + float(i));
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindDef

CSpeedWindDef::CSpeedWindDef(CSpeedWind* pFather) :
    m_pFather(pFather),
    m_fMaxBendAngle(0.0f),
    m_fWindResponse(1.0f),
    m_fWindResponseLimit(0.0f),
    m_fGustingStrengthMin(0.0f),
    m_fGustingStrengthMax(0.0f),
    m_fGustingFrequency(0.0f),
    m_fGustingDurationMin(0.0f),
    m_fGustingDurationMax(0.0f),
    m_fBranchExponent(1.0f),
    m_fLeafExponent(1.0f),
    m_fLastTime(0.0f),
    m_fWindStrength(0.0f),
    m_fCurrentGustingStrength(0.0f),
    m_fGustStopTime(0.0f),
    m_fFinalBendAngle(0.0f),
    m_nRandomIndex(0),
    m_bUseRandomBuffer(false)
{
    SetQuantities(6, 8);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::~CSpeedWindDef

CSpeedWindDef::~CSpeedWindDef( )
{
    m_pFather = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetQuantities

void CSpeedWindDef::SetQuantities(int iNumWindMatrices, int iNumLeafAngles)
{
    if (iNumWindMatrices < 1)
        iNumWindMatrices = 1;

    if (iNumLeafAngles < 1)
        iNumLeafAngles = 1;

    m_vWindMatrices.clear( );
    m_vWindMatrices.resize(iNumWindMatrices);

    m_sBranchHorizontal.m_vOutputs.clear( );
    m_sBranchVertical.m_vOutputs.clear( );
    m_sBranchHorizontal.m_vOutputs.resize(iNumWindMatrices);
    m_sBranchVertical.m_vOutputs.resize(iNumWindMatrices);

    m_sLeafRocking.m_vOutputs.clear( );
    m_sLeafRustling.m_vOutputs.clear( );
    m_vLeafAngleMatrices.clear( );
    m_sLeafRocking.m_vOutputs.resize(iNumLeafAngles);
    m_sLeafRustling.m_vOutputs.resize(iNumLeafAngles);
    m_vLeafAngleMatrices.resize(iNumLeafAngles);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetWindResponse

void CSpeedWindDef::SetWindResponse(float fResponse, float fReponseLimit)
{
    m_fWindResponse = fResponse;
    m_fWindResponseLimit = fReponseLimit;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetWindStrengthAndDirection

void CSpeedWindDef::SetWindStrengthAndDirection(float fWindStrength, float fWindDirectionX, float fWindDirectionY, float fWindDirectionZ)
{
    m_fWindStrength = st_clamp<float>(fWindStrength, 0.0f, 1.0f);

    float fSum = fWindDirectionX * fWindDirectionX + fWindDirectionY * fWindDirectionY + fWindDirectionZ * fWindDirectionZ;
    if (fSum != 0.0f)
    {
        fSum = sqrt(fSum);
        fWindDirectionX /= fSum;
        fWindDirectionY /= fSum;
        fWindDirectionZ /= fSum;
    }

    m_cFinalWindDirectionX.m_fWantedValue = fWindDirectionX;
    m_cFinalWindDirectionY.m_fWantedValue = fWindDirectionY;
    m_cFinalWindDirectionZ.m_fWantedValue = fWindDirectionZ;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetMaxBendAngle

void CSpeedWindDef::SetMaxBendAngle(float fMaxBendAngle)
{
    m_fMaxBendAngle = fMaxBendAngle;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetExponents

void CSpeedWindDef::SetExponents(float fBranchExponent, float fLeafExponent)
{
    m_fBranchExponent = fBranchExponent;
    m_fLeafExponent = fLeafExponent;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetGusting

void CSpeedWindDef::SetGusting(float fGustStrengthMin, float fGustStrengthMax, float fGustFrequency, float fGustDurationMin, float fGustDurationMax)
{
    m_fGustingStrengthMin = fGustStrengthMin;
    m_fGustingStrengthMax = fGustStrengthMax;
    m_fGustingFrequency = fGustFrequency;
    m_fGustingDurationMin = fGustDurationMin;
    m_fGustingDurationMax = fGustDurationMax;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetBranchHorizontal

void CSpeedWindDef::SetBranchHorizontal(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sBranchHorizontal.m_fLowWindAngle = fLowWindAngle;
    m_sBranchHorizontal.m_fHighWindAngle = fHighWindAngle;
    m_sBranchHorizontal.m_fLowWindSpeed = fLowWindSpeed;
    m_sBranchHorizontal.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetBranchVertical

void CSpeedWindDef::SetBranchVertical(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sBranchVertical.m_fLowWindAngle = fLowWindAngle;
    m_sBranchVertical.m_fHighWindAngle = fHighWindAngle;
    m_sBranchVertical.m_fLowWindSpeed = fLowWindSpeed;
    m_sBranchVertical.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetLeafRocking

void CSpeedWindDef::SetLeafRocking(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sLeafRocking.m_fLowWindAngle = fLowWindAngle;
    m_sLeafRocking.m_fHighWindAngle = fHighWindAngle;
    m_sLeafRocking.m_fLowWindSpeed = fLowWindSpeed;
    m_sLeafRocking.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetLeafRustling

void CSpeedWindDef::SetLeafRustling(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sLeafRustling.m_fLowWindAngle = fLowWindAngle;
    m_sLeafRustling.m_fHighWindAngle = fHighWindAngle;
    m_sLeafRustling.m_fLowWindSpeed = fLowWindSpeed;
    m_sLeafRustling.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetTime

float CSpeedWindDef::GetTime(void) const
{
    return m_fLastTime;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Advance

void CSpeedWindDef::Advance(float fCurrentTime, bool bUpdateMatrices, bool bUpdateLeafAngleMatrices, float fCameraX, float fCameraY, float fCameraZ)
{
    float fDeltaTime = fCurrentTime - m_fLastTime;
    m_fLastTime = fCurrentTime;

    // stop current gust if needed
    if (fCurrentTime > m_fGustStopTime)
    {
        m_fCurrentGustingStrength = 0.0f;
    
        // make new gusts
        if (m_fGustingFrequency * fDeltaTime > float(Random( )) / float(RAND_MAX))
        {
            m_fCurrentGustingStrength = float(Random( )) * (m_fGustingStrengthMax - m_fGustingStrengthMin) / float(RAND_MAX) + m_fGustingStrengthMin;
            m_fGustStopTime = fCurrentTime + float(Random( )) * (m_fGustingDurationMax - m_fGustingDurationMin) / float(RAND_MAX) + m_fGustingDurationMin;
        }
    }

    // compute the actual wind strength due to gusting
    m_cFinalWindStrength.m_fWantedValue = m_fWindStrength + m_fCurrentGustingStrength;
    m_cFinalWindStrength.m_fWantedValue = st_clamp<float>(m_cFinalWindStrength.m_fWantedValue, 0.0f, 1.0f);
    m_cFinalWindStrength.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);
    m_cFinalWindStrength.m_fCurrentValue = st_clamp<float>(m_cFinalWindStrength.m_fCurrentValue, 0.0f, 1.0f);

    // smooth out the direction
    m_cFinalWindDirectionX.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);
    m_cFinalWindDirectionY.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);
    m_cFinalWindDirectionZ.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);

    // compute the branch and leaf wind strengths due to exponents
    float fBranchStrength = powf(m_cFinalWindStrength.m_fCurrentValue, m_fBranchExponent);
    float fLeafStrength = powf(m_cFinalWindStrength.m_fCurrentValue, m_fLeafExponent);

    // update the main tree bend angle (and compensate for unnormalized, over the pole movement)
    m_fFinalBendAngle = fBranchStrength * m_fMaxBendAngle;

    // update the oscillating parts
    m_sBranchHorizontal.Advance(fDeltaTime, fBranchStrength);
    m_sBranchVertical.Advance(fDeltaTime, fBranchStrength);
    m_sLeafRocking.Advance(fDeltaTime, fLeafStrength);
    m_sLeafRustling.Advance(fDeltaTime, fLeafStrength);

    stVec3 vecTemp(m_cFinalWindDirectionX.m_fCurrentValue, m_cFinalWindDirectionY.m_fCurrentValue, m_cFinalWindDirectionZ.m_fCurrentValue);
    vecTemp.Normalize( );
#ifdef SPEEDTREE_UPVECTOR_Z
    float fOverThePoleScalar = 1.0f - fabs(vecTemp[2]);
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
    float fOverThePoleScalar = 1.0f - fabs(vecTemp[1]);
#endif
    
    if (bUpdateMatrices)
    {
        // build wind matrices
        
        // vectors
#ifdef SPEEDTREE_UPVECTOR_Z
        float fWindAngle = -static_cast<float>(atan2(m_cFinalWindDirectionX.m_fCurrentValue, m_cFinalWindDirectionY.m_fCurrentValue));
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
        float fWindAngle = static_cast<float>(atan2(m_cFinalWindDirectionX.m_fCurrentValue, m_cFinalWindDirectionZ.m_fCurrentValue)) - c_fHalfPi;
#endif

#ifdef SPEEDTREE_UPVECTOR_NEG_Z
            float fWindAngle = 0;
#endif

        float fXRot = -cosf(fWindAngle);
        float fYRot = -sinf(fWindAngle);

        static stTransform cTemp;
        unsigned int uiSize = (unsigned int)m_vWindMatrices.size( );
        for (unsigned int i = 0; i < uiSize; ++i)
        {
#ifdef SPEEDTREE_UPVECTOR_Z
            m_vWindMatrices[i].RotateAxisXYPlane_Opt(fOverThePoleScalar * (m_fFinalBendAngle + m_sBranchVertical.m_vOutputs[i]), stVec(fXRot, fYRot, 0.0f));
            cTemp.RotateAxisXYPlane_Opt(fOverThePoleScalar * m_sBranchHorizontal.m_vOutputs[i], stVec(-fYRot, fXRot, 0.0f));
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
            m_vWindMatrices[i].RotateAxisXZPlane_Opt(fOverThePoleScalar * (m_fFinalBendAngle + m_sBranchVertical.m_vOutputs[i]), stVec(fYRot, 0.0f, fXRot));
            cTemp.RotateAxisXZPlane_Opt(fOverThePoleScalar * m_sBranchHorizontal.m_vOutputs[i], stVec(-fXRot, 0.0f, fYRot));
#endif
            // final matrix
            m_vWindMatrices[i] = m_vWindMatrices[i] * cTemp;
        }
    }

    if (bUpdateLeafAngleMatrices)
    {
        // build leaf angle matrices
        float afAdjustedCameraDir[3] = 
        {
#ifdef SPEEDTREE_UPVECTOR_Z
            fCameraX, fCameraY, fCameraZ
#endif
#ifdef SPEEDTREE_UPVECTOR_NEG_Z
            -fCameraX, fCameraY, -fCameraZ
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
            -fCameraX, fCameraZ, fCameraY
#endif
#ifdef SPEEDTREE_UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
            fCameraY, fCameraX, fCameraZ
#endif
        };

        float fAzimuth = VecRad2Deg(atan2f(afAdjustedCameraDir[1], afAdjustedCameraDir[0]));
        float fPitch = VecRad2Deg(-asinf(afAdjustedCameraDir[2]));

        unsigned int uiSize = (unsigned int) m_vLeafAngleMatrices.size( );
        for (unsigned int i = 0; i < uiSize; ++i)
        {
            stTransform& cMatrix = m_vLeafAngleMatrices[i];

            cMatrix.LoadIdentity( );
#ifdef SPEEDTREE_UPVECTOR_Z
            cMatrix.Rotate(fAzimuth, 'z');
            cMatrix.Rotate(fPitch, 'y');
            cMatrix.Rotate(m_sLeafRustling.m_vOutputs[i], 'z');
            cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'x');
#endif
#ifdef SPEEDTREE_UPVECTOR_NEG_Z
            cMatrix.Rotate(-fAzimuth, 'z');
            cMatrix.Rotate(fPitch, 'y');
            cMatrix.Rotate(m_sLeafRustling.m_vOutputs[i], 'z');
            cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'x');
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
            cMatrix.Rotate(fAzimuth, 'y');
            cMatrix.Rotate(fPitch, 'z');
            cMatrix.Rotate(-m_sLeafRustling.m_vOutputs[i], 'y');
            cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'x');
#endif
#ifdef SPEEDTREE_UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
            cMatrix.Rotate(-fAzimuth, 'z');
            cMatrix.Rotate(-fPitch, 'x');
            cMatrix.Rotate(m_sLeafRustling.m_vOutputs[i], 'z');
            cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'y');
#endif
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::UpdateTree

void CSpeedWindDef::UpdateTree(CSpeedTreeRT* pTree) const
{
    pTree->SetWindStrengthAndLeafAngles(m_cFinalWindStrength.m_fCurrentValue, 
                                        &(m_sLeafRocking.m_vOutputs[0]), 
                                        &(m_sLeafRustling.m_vOutputs[0]), 
                                        int(m_sLeafRocking.m_vOutputs.size( )));
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::UpdateSpeedTreeRT

void CSpeedWindDef::UpdateSpeedTreeRT(void) const
{
    unsigned int uiSize = (unsigned int) m_vWindMatrices.size( );
    for (unsigned int i = 0; i < uiSize; ++i)
        CSpeedTreeRT::SetWindMatrix(i, (const float*)m_vWindMatrices[i].m_afData);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetFinalStrength

float CSpeedWindDef::GetFinalStrength(void)
{
    return m_cFinalWindStrength.m_fCurrentValue;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetNumWindMatrices

unsigned int CSpeedWindDef::GetNumWindMatrices(void) const
{
    return (unsigned int) m_vWindMatrices.size( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetWindMatrix

const float* CSpeedWindDef::GetWindMatrix(unsigned int uiIndex) const
{
    float* pReturn = NULL;

    if (uiIndex < m_vWindMatrices.size( ))
        pReturn = ((float*)m_vWindMatrices[uiIndex].m_afData);

    return pReturn;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetNumWindMatrices

unsigned int CSpeedWindDef::GetNumLeafAngleMatrices(void) const
{
    return (unsigned int) m_vLeafAngleMatrices.size( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetLeafAngleMatrix

const float* CSpeedWindDef::GetLeafAngleMatrix(unsigned int uiIndex) const
{
    float* pReturn = NULL;

    if (uiIndex < m_vWindMatrices.size( ))
        pReturn = ((float*)m_vLeafAngleMatrices[uiIndex].m_afData);

    return pReturn;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Load

bool CSpeedWindDef::Load(const st_string& strFilename)
{
    bool bSuccess = false;

    FILE* pFile = fopen(strFilename.c_str( ), "r");
    if (pFile != NULL)
    {
        fseek(pFile, 0L, SEEK_END);
        int nNumBytes = ftell(pFile);
        int nErrorCode = fseek(pFile, 0, SEEK_SET);

        if (nNumBytes > 0 && nErrorCode >= 0)
        {
            char* pData = st_new_array<char>(nNumBytes, "CSpeedWindDef::char");
            fread(pData, nNumBytes, 1, pFile);

            bSuccess = Load(pData, nNumBytes);

            st_delete_array<char>(pData, "CSpeedWindDef::char");
        }

        fclose(pFile);
    }

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Load

bool CSpeedWindDef::Load(const char* pData, unsigned int uiNumBytes)
{
    st_assert(pData);

    st_string strData;
    strData.resize(uiNumBytes + 1);
    memcpy((void*)strData.c_str( ), pData, uiNumBytes);
    strData[uiNumBytes] = 0;
    size_t stStart = 0;

    int iTemp0 = 0;
    int iTemp1 = 0;
    float fTemp0 = 0.0f;
    float fTemp1 = 0.0f;
    float fTemp2 = 0.0f;
    float fTemp3 = 0.0f;

    unsigned int uiLinesRead = 0;
    while (uiLinesRead != 4095 && stStart != st_string::npos)
    {
        if (strData.compare(stStart, 28, "NumWindMatricesAndLeafAngles") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 28, "%d %d", &iTemp0, &iTemp1) != 2)
                break;
            SetQuantities(iTemp0, iTemp1);
            uiLinesRead |= (1 << 0);
        }
        else if (strData.compare(stStart, 20, "WindResponseAndLimit") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 20, "%g %g", &fTemp0, &fTemp1) != 2)
                break;
            SetWindResponse(fTemp0, fTemp1);
            uiLinesRead |= (1 << 1);
        }
        else if (strData.compare(stStart, 12, "MaxBendAngle") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 12, "%g", &m_fMaxBendAngle) != 1)
                break;
            uiLinesRead |= (1 << 2);
        }
        else if (strData.compare(stStart, 14, "BranchExponent") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 14, "%g", &m_fBranchExponent) != 1)
                break;
            uiLinesRead |= (1 << 3);
        }
        else if (strData.compare(stStart, 12, "LeafExponent") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 12, "%g", &m_fLeafExponent) != 1)
                break;
            uiLinesRead |= (1 << 4);
        }
        else if (strData.compare(stStart, 24, "GustStrengthFreqDuration") == 0)
        {
            // older style SpeedWind
            if (sscanf(strData.c_str( ) + stStart + 24, "%g %g %g", &fTemp0, &fTemp1, &fTemp2) != 3)
                break;
            SetGusting(fTemp0 * 0.25f, fTemp0 * 1.25f, fTemp1, fTemp2 * 0.5f, fTemp2 * 1.5f);
            uiLinesRead |= (1 << 5) | (1 << 6) | (1 << 7);
        }
        else if (strData.compare(stStart, 18, "GustStrengthMinMax") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 18, "%g %g", &m_fGustingStrengthMin, &m_fGustingStrengthMax) != 2)
                break;
            uiLinesRead |= (1 << 5);
        }
        else if (strData.compare(stStart, 18, "GustDurationMinMax") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 18, "%g %g", &m_fGustingDurationMin, &m_fGustingDurationMax) != 2)
                break;
            uiLinesRead |= (1 << 6);
        }
        else if (strData.compare(stStart, 13, "GustFrequency") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 13, "%g", &m_fGustingFrequency) != 1)
                break;
            uiLinesRead |= (1 << 7);
        }
        else if (strData.compare(stStart, 27, "BranchOscillationX_LaLsHaHs") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 27, "%g %g %g %g", &fTemp0, &fTemp1, &fTemp2, &fTemp3) != 4)
                break;
            SetBranchHorizontal(fTemp0, fTemp2, fTemp1, fTemp3);
            uiLinesRead |= (1 << 8);
        }
        else if (strData.compare(stStart, 27, "BranchOscillationY_LaLsHaHs") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 27, "%g %g %g %g", &fTemp0, &fTemp1, &fTemp2, &fTemp3) != 4)
                break;
            SetBranchVertical(fTemp0, fTemp2, fTemp1, fTemp3);
            uiLinesRead |= (1 << 9);
        }
        else if (strData.compare(stStart, 20, "LeafRocking_LaLsHaHs") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 20, "%g %g %g %g", &fTemp0, &fTemp1, &fTemp2, &fTemp3) != 4)
                break;
            SetLeafRocking(fTemp0, fTemp2, fTemp1, fTemp3);
            uiLinesRead |= (1 << 10);
        }
        else if (strData.compare(stStart, 21, "LeafRustling_LaLsHaHs") == 0)
        {
            if (sscanf(strData.c_str( ) + stStart + 21, "%g %g %g %g", &fTemp0, &fTemp1, &fTemp2, &fTemp3) != 4)
                break;
            SetLeafRustling(fTemp0, fTemp2, fTemp1, fTemp3);
            uiLinesRead |= (1 << 11);
        }
        
        stStart = strData.find('\n', stStart);
        if (stStart != st_string::npos)
            ++stStart;
    }

    return (uiLinesRead == 4095);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Save

bool CSpeedWindDef::Save(const st_string& strFilename) const
{
    bool bSuccess = false;

    unsigned int uiSize = 0;
    char* pData = (char*)Save(uiSize);

    if (pData != NULL && uiSize > 0)
    {
        FILE* pFile = fopen(strFilename.c_str( ), "w");
        if (pFile != NULL)
        {
            fwrite(pData, uiSize, 1, pFile);
            fclose(pFile);
            bSuccess = true;
        }

        st_delete_array<char>(pData, "CSpeedWindDef::char[]");
    }   

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Save

const char* CSpeedWindDef::Save(unsigned int& uiNumBytes) const
{
    char achTemp[64];

    st_string strSave = "NumWindMatricesAndLeafAngles ";
    sprintf(achTemp, "%d %d\n", m_vWindMatrices.size( ), m_sLeafRocking.m_vOutputs.size( ));
    strSave += achTemp;

    strSave += "WindResponseAndLimit ";
    sprintf(achTemp, "%g %g\n", m_fWindResponse, m_fWindResponseLimit);
    strSave += achTemp;

    strSave += "MaxBendAngle ";
    sprintf(achTemp, "%g\n", m_fMaxBendAngle);
    strSave += achTemp;

    strSave += "BranchExponent ";
    sprintf(achTemp, "%g\n", m_fBranchExponent);
    strSave += achTemp;

    strSave += "LeafExponent ";
    sprintf(achTemp, "%g\n", m_fLeafExponent);
    strSave += achTemp;

    strSave += "GustStrengthMinMax ";
    sprintf(achTemp, "%g %g\n", m_fGustingStrengthMin, m_fGustingStrengthMax);
    strSave += achTemp;

    strSave += "GustDurationMinMax ";
    sprintf(achTemp, "%g %g\n", m_fGustingDurationMin, m_fGustingDurationMax);
    strSave += achTemp;

    strSave += "GustFrequency ";
    sprintf(achTemp, "%g\n", m_fGustingFrequency);
    strSave += achTemp;

    strSave += "BranchOscillationX_LaLsHaHs ";
    sprintf(achTemp, "%g %g %g %g\n", m_sBranchHorizontal.m_fLowWindAngle, m_sBranchHorizontal.m_fLowWindSpeed,
                                    m_sBranchHorizontal.m_fHighWindAngle, m_sBranchHorizontal.m_fHighWindSpeed);
    strSave += achTemp;

    strSave += "BranchOscillationY_LaLsHaHs ";
    sprintf(achTemp, "%g %g %g %g\n", m_sBranchVertical.m_fLowWindAngle, m_sBranchVertical.m_fLowWindSpeed,
                                    m_sBranchVertical.m_fHighWindAngle, m_sBranchVertical.m_fHighWindSpeed);
    strSave += achTemp;

    strSave += "LeafRocking_LaLsHaHs ";
    sprintf(achTemp, "%g %g %g %g\n", m_sLeafRocking.m_fLowWindAngle, m_sLeafRocking.m_fLowWindSpeed,
                                    m_sLeafRocking.m_fHighWindAngle, m_sLeafRocking.m_fHighWindSpeed);
    strSave += achTemp;

    strSave += "LeafRustling_LaLsHaHs ";
    sprintf(achTemp, "%g %g %g %g\n", m_sLeafRustling.m_fLowWindAngle, m_sLeafRustling.m_fLowWindSpeed,
                                    m_sLeafRustling.m_fHighWindAngle, m_sLeafRustling.m_fHighWindSpeed);
    strSave += achTemp;

    uiNumBytes = (unsigned int)strSave.size( );
    char* pSaveData = st_new_array<char>(uiNumBytes, "CSpeedWindDef::char[]");
    memcpy(pSaveData, strSave.c_str( ), uiNumBytes);

    return pSaveData;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetFilename

const char* CSpeedWindDef::GetFilename(void) const
{
    return m_strFilename.c_str( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Reset

void CSpeedWindDef::Reset(void)
{
    unsigned int i = 0;
    unsigned int uiSize = (unsigned int) m_vWindMatrices.size( );
    for (i = 0; i < uiSize; ++i)
        m_vWindMatrices[i].LoadIdentity( );

    uiSize = (unsigned int) m_vLeafAngleMatrices.size( );
    for (i = 0; i < uiSize; ++i)
        m_vLeafAngleMatrices[i].LoadIdentity( );

    m_cFinalWindStrength.Reset(0.0f);
    m_cFinalWindDirectionX.Reset(0.0f);
    m_cFinalWindDirectionY.Reset(0.0f);
    m_cFinalWindDirectionZ.Reset(0.0f);

    m_nRandomIndex = 0;
    m_fCurrentGustingStrength = 0.0f;
    m_fGustStopTime = 0.0f;
    m_fLastTime = 0.0f;

    m_sBranchHorizontal.Reset( );
    m_sBranchVertical.Reset( );
    m_sLeafRocking.Reset( );
    m_sLeafRustling.Reset( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetQuantities

void CSpeedWindDef::GetQuantities(int& iNumWindMatrices, int& iNumLeafAngles) const
{
    iNumWindMatrices = (int)m_vWindMatrices.size( );
    iNumLeafAngles = (int)m_vLeafAngleMatrices.size( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetWindResponse

void CSpeedWindDef::GetWindResponse(float& fResponse, float& fReponseLimit) const
{
    fResponse = m_fWindResponse;
    fReponseLimit = m_fWindResponseLimit;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetWindStrengthAndDirection

void CSpeedWindDef::GetWindStrengthAndDirection(float& fWindStrength, float& fWindDirectionX, float& fWindDirectionY, float& fWindDirectionZ) const
{
    fWindStrength = m_fWindStrength;
    fWindDirectionX = m_cFinalWindDirectionX.m_fWantedValue;
    fWindDirectionY = m_cFinalWindDirectionY.m_fWantedValue;
    fWindDirectionZ = m_cFinalWindDirectionZ.m_fWantedValue;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetMaxBendAngle

float CSpeedWindDef::GetMaxBendAngle(void) const
{
    return m_fMaxBendAngle;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetExponents

void CSpeedWindDef::GetExponents(float& fBranchExponent, float& fLeafExponent) const
{
    fBranchExponent = m_fBranchExponent;
    fLeafExponent = m_fLeafExponent;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetGusting

void CSpeedWindDef::GetGusting(float& fGustStrengthMin, float& fGustStrengthMax, float& fGustFrequency, float& fGustDurationMin, float& fGustDurationMax) const
{
    fGustStrengthMin = m_fGustingStrengthMin;
    fGustStrengthMax = m_fGustingStrengthMax;
    fGustFrequency = m_fGustingFrequency;
    fGustDurationMin = m_fGustingDurationMin;
    fGustDurationMax = m_fGustingDurationMax;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetBranchHorizontal

void CSpeedWindDef::GetBranchHorizontal(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sBranchHorizontal.m_fLowWindAngle;
    fHighWindAngle = m_sBranchHorizontal.m_fHighWindAngle;
    fLowWindSpeed = m_sBranchHorizontal.m_fLowWindSpeed;
    fHighWindSpeed = m_sBranchHorizontal.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetBranchVertical

void CSpeedWindDef::GetBranchVertical(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sBranchVertical.m_fLowWindAngle;
    fHighWindAngle = m_sBranchVertical.m_fHighWindAngle;
    fLowWindSpeed = m_sBranchVertical.m_fLowWindSpeed;
    fHighWindSpeed = m_sBranchVertical.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetLeafRocking

void CSpeedWindDef::GetLeafRocking(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sLeafRocking.m_fLowWindAngle;
    fHighWindAngle = m_sLeafRocking.m_fHighWindAngle;
    fLowWindSpeed = m_sLeafRocking.m_fLowWindSpeed;
    fHighWindSpeed = m_sLeafRocking.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetLeafRustling

void CSpeedWindDef::GetLeafRustling(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sLeafRustling.m_fLowWindAngle;
    fHighWindAngle = m_sLeafRustling.m_fHighWindAngle;
    fLowWindSpeed = m_sLeafRustling.m_fLowWindSpeed;
    fHighWindSpeed = m_sLeafRustling.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetRustleAngles

bool CSpeedWindDef::GetRustleAngles(float* pAngles) const
{
    bool bSuccess = false;

    int nNumLeaves = int(m_sLeafRustling.m_vOutputs.size( ));

    st_assert(pAngles);
    memcpy(pAngles, &m_sLeafRustling.m_vOutputs[0], nNumLeaves * sizeof(float));
    bSuccess = true;

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetRockAngles

bool CSpeedWindDef::GetRockAngles(float* pAngles) const
{
    bool bSuccess = false;

    int nNumLeaves = int(m_sLeafRocking.m_vOutputs.size( ));

    st_assert(pAngles);
    memcpy(pAngles, &m_sLeafRocking.m_vOutputs[0], nNumLeaves * sizeof(float));
    bSuccess = true;

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::InterpolateTarget

void CSpeedWindDef::InterpolateTarget(const CSpeedWind* pWind1, const CSpeedWind* pWind2, float fInterpolation)
{
#define INTERPOLATE_WIND_PARAM(a) a = VecInterpolate(pWind1->m_pDefinition->a, pWind2->m_pDefinition->a, fInterpolation)

    if (pWind1 != NULL && pWind2 != NULL)
    {
        INTERPOLATE_WIND_PARAM(m_fWindStrength);
        INTERPOLATE_WIND_PARAM(m_cFinalWindDirectionX.m_fWantedValue);
        INTERPOLATE_WIND_PARAM(m_cFinalWindDirectionY.m_fWantedValue);
        INTERPOLATE_WIND_PARAM(m_cFinalWindDirectionZ.m_fWantedValue);

        // normalize direction
        stVec3 vecNormalized(m_cFinalWindDirectionX.m_fWantedValue, m_cFinalWindDirectionY.m_fWantedValue, m_cFinalWindDirectionZ.m_fWantedValue);
        vecNormalized.Normalize( );
        m_cFinalWindDirectionX.m_fWantedValue = vecNormalized.m_afData[0];
        m_cFinalWindDirectionY.m_fWantedValue = vecNormalized.m_afData[1];
        m_cFinalWindDirectionZ.m_fWantedValue = vecNormalized.m_afData[2];

        INTERPOLATE_WIND_PARAM(m_sBranchHorizontal.m_fLowWindAngle);
        INTERPOLATE_WIND_PARAM(m_sBranchHorizontal.m_fHighWindAngle);
        INTERPOLATE_WIND_PARAM(m_sBranchHorizontal.m_fLowWindSpeed);
        INTERPOLATE_WIND_PARAM(m_sBranchHorizontal.m_fHighWindSpeed);

        INTERPOLATE_WIND_PARAM(m_sBranchVertical.m_fLowWindAngle);
        INTERPOLATE_WIND_PARAM(m_sBranchVertical.m_fHighWindAngle);
        INTERPOLATE_WIND_PARAM(m_sBranchVertical.m_fLowWindSpeed);
        INTERPOLATE_WIND_PARAM(m_sBranchVertical.m_fHighWindSpeed);

        INTERPOLATE_WIND_PARAM(m_sLeafRocking.m_fLowWindAngle);
        INTERPOLATE_WIND_PARAM(m_sLeafRocking.m_fHighWindAngle);
        INTERPOLATE_WIND_PARAM(m_sLeafRocking.m_fLowWindSpeed);
        INTERPOLATE_WIND_PARAM(m_sLeafRocking.m_fHighWindSpeed);

        INTERPOLATE_WIND_PARAM(m_sLeafRustling.m_fLowWindAngle);
        INTERPOLATE_WIND_PARAM(m_sLeafRustling.m_fHighWindAngle);
        INTERPOLATE_WIND_PARAM(m_sLeafRustling.m_fLowWindSpeed);
        INTERPOLATE_WIND_PARAM(m_sLeafRustling.m_fHighWindSpeed);

        INTERPOLATE_WIND_PARAM(m_fMaxBendAngle);
        INTERPOLATE_WIND_PARAM(m_fWindResponse);
        INTERPOLATE_WIND_PARAM(m_fWindResponseLimit);
        INTERPOLATE_WIND_PARAM(m_fGustingStrengthMin);
        INTERPOLATE_WIND_PARAM(m_fGustingStrengthMax);
        INTERPOLATE_WIND_PARAM(m_fGustingFrequency);
        INTERPOLATE_WIND_PARAM(m_fGustingDurationMin);
        INTERPOLATE_WIND_PARAM(m_fGustingDurationMax);
        INTERPOLATE_WIND_PARAM(m_fBranchExponent);
        INTERPOLATE_WIND_PARAM(m_fLeafExponent);
        
    }

#undef INTERPOLATE_WIND_PARAM
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::BlendParameters

void CSpeedWindDef::BlendParameters(const CSpeedWind** pWinds, const float* pWeights, unsigned int uiNumWinds)
{
    // zero out our data
    m_fWindStrength = 0.0f;
    m_cFinalWindDirectionX.m_fWantedValue = 0.0f;
    m_cFinalWindDirectionY.m_fWantedValue = 0.0f;
    m_cFinalWindDirectionZ.m_fWantedValue = 0.0f;

    m_sBranchHorizontal.m_fLowWindAngle = 0.0f;
    m_sBranchHorizontal.m_fHighWindAngle = 0.0f;
    m_sBranchHorizontal.m_fLowWindSpeed = 0.0f;
    m_sBranchHorizontal.m_fHighWindSpeed = 0.0f;

    m_sBranchVertical.m_fLowWindAngle = 0.0f;
    m_sBranchVertical.m_fHighWindAngle = 0.0f;
    m_sBranchVertical.m_fLowWindSpeed = 0.0f;
    m_sBranchVertical.m_fHighWindSpeed = 0.0f;

    m_sLeafRocking.m_fLowWindAngle = 0.0f;
    m_sLeafRocking.m_fHighWindAngle = 0.0f;
    m_sLeafRocking.m_fLowWindSpeed = 0.0f;
    m_sLeafRocking.m_fHighWindSpeed = 0.0f;

    m_sLeafRustling.m_fLowWindAngle = 0.0f;
    m_sLeafRustling.m_fHighWindAngle = 0.0f;
    m_sLeafRustling.m_fLowWindSpeed = 0.0f;
    m_sLeafRustling.m_fHighWindSpeed = 0.0f;

    m_fMaxBendAngle = 0.0f;
    m_fWindResponse = 0.0f;
    m_fWindResponseLimit = 0.0f;
    m_fGustingStrengthMin = 0.0f;
    m_fGustingStrengthMax = 0.0f;
    m_fGustingFrequency = 0.0f;
    m_fGustingDurationMin = 0.0f;
    m_fGustingDurationMax = 0.0f;
    m_fBranchExponent = 0.0f;
    m_fLeafExponent = 0.0f;


#define SUM_WIND_PARAM(a) a += pWinds[i]->m_pDefinition->a * pWeights[i]

    // fill data based on winds and weights
    for (unsigned int i = 0; i < uiNumWinds; ++i)
    {
        SUM_WIND_PARAM(m_fWindStrength);
        SUM_WIND_PARAM(m_cFinalWindDirectionX.m_fWantedValue);
        SUM_WIND_PARAM(m_cFinalWindDirectionY.m_fWantedValue);
        SUM_WIND_PARAM(m_cFinalWindDirectionZ.m_fWantedValue);

        SUM_WIND_PARAM(m_sBranchHorizontal.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sBranchHorizontal.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sBranchHorizontal.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sBranchHorizontal.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_sBranchVertical.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sBranchVertical.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sBranchVertical.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sBranchVertical.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_sLeafRocking.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sLeafRocking.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sLeafRocking.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sLeafRocking.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_sLeafRustling.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sLeafRustling.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sLeafRustling.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sLeafRustling.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_fMaxBendAngle);
        SUM_WIND_PARAM(m_fWindResponse);
        SUM_WIND_PARAM(m_fWindResponseLimit);
        SUM_WIND_PARAM(m_fGustingStrengthMin);
        SUM_WIND_PARAM(m_fGustingStrengthMax);
        SUM_WIND_PARAM(m_fGustingFrequency);
        SUM_WIND_PARAM(m_fGustingDurationMin);
        SUM_WIND_PARAM(m_fGustingDurationMax);
        SUM_WIND_PARAM(m_fBranchExponent);
        SUM_WIND_PARAM(m_fLeafExponent);
    }

#undef SUM_WIND_PARAM

    // normalize direction
    stVec3 vecNormalize(m_cFinalWindDirectionX.m_fWantedValue, m_cFinalWindDirectionY.m_fWantedValue, m_cFinalWindDirectionZ.m_fWantedValue);
    vecNormalize.Normalize( );
    m_cFinalWindDirectionX.m_fWantedValue = vecNormalize.m_afData[0];
    m_cFinalWindDirectionY.m_fWantedValue = vecNormalize.m_afData[1];
    m_cFinalWindDirectionZ.m_fWantedValue = vecNormalize.m_afData[2];
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::InterpolateCurrent

void CSpeedWindDef::InterpolateCurrent(const CSpeedWind* pWind1, const CSpeedWind* pWind2, float fInterpolation, bool bUpdateMatrices, bool bUpdateLeafAngleMatrices, float fCameraX, float fCameraY, float fCameraZ)
{
    #define INTERPOLATE_WIND_PARAM(a) a = VecInterpolate(pWind1->m_pDefinition->a, pWind2->m_pDefinition->a, fInterpolation)

    if (pWind1 != NULL && pWind2 != NULL)
    {
        // rock angles
        int nNumLeaves = st_min(int(pWind1->m_pDefinition->m_sLeafRocking.m_vOutputs.size( )), int(pWind2->m_pDefinition->m_sLeafRocking.m_vOutputs.size( )));
        m_sLeafRocking.m_vOutputs.resize(nNumLeaves);
        int i = 0;
        for (i = 0; i < nNumLeaves; ++i)
            INTERPOLATE_WIND_PARAM(m_sLeafRocking.m_vOutputs[i]);

        // rustle angles
        nNumLeaves = st_min(int(pWind1->m_pDefinition->m_sLeafRustling.m_vOutputs.size( )), int(pWind2->m_pDefinition->m_sLeafRustling.m_vOutputs.size( )));
        m_sLeafRustling.m_vOutputs.resize(nNumLeaves);
        for (i = 0; i < nNumLeaves; ++i)
            INTERPOLATE_WIND_PARAM(m_sLeafRustling.m_vOutputs[i]);

        // wind matrices
        if (bUpdateMatrices)
        {
            INTERPOLATE_WIND_PARAM(m_cFinalWindDirectionX.m_fCurrentValue);
            INTERPOLATE_WIND_PARAM(m_cFinalWindDirectionY.m_fCurrentValue);
            INTERPOLATE_WIND_PARAM(m_cFinalWindDirectionZ.m_fCurrentValue);
            
            INTERPOLATE_WIND_PARAM(m_fFinalBendAngle);

#ifdef SPEEDTREE_UPVECTOR_Z
            float fWindAngle = -static_cast<float>(atan2(m_cFinalWindDirectionX.m_fCurrentValue, m_cFinalWindDirectionY.m_fCurrentValue));// + c_fHalfPi);
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
            float fWindAngle = static_cast<float>(atan2(m_cFinalWindDirectionX.m_fCurrentValue, m_cFinalWindDirectionZ.m_fCurrentValue)) - c_fHalfPi;
#endif

#ifdef SPEEDTREE_UPVECTOR_NEG_Z
            float fWindAngle = 0;
#endif

            float fXRot = -cosf(fWindAngle);
            float fYRot = -sinf(fWindAngle);

            // make sure vectors are big enough
            int nNumMatrices = st_min(int(pWind1->m_pDefinition->m_vWindMatrices.size( )), int(pWind2->m_pDefinition->m_vWindMatrices.size( )));
            m_vWindMatrices.resize(nNumMatrices);
            m_sBranchVertical.m_vOutputs.resize(nNumMatrices);
            m_sBranchHorizontal.m_vOutputs.resize(nNumMatrices);

            stVec3 vecTemp(m_cFinalWindDirectionX.m_fCurrentValue, m_cFinalWindDirectionY.m_fCurrentValue, m_cFinalWindDirectionZ.m_fCurrentValue);
            vecTemp.Normalize( );
#ifdef SPEEDTREE_UPVECTOR_Z
            float fOverThePoleScalar = 1.0f - fabs(vecTemp[2]);
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
            float fOverThePoleScalar = 1.0f - fabs(vecTemp[1]);
#endif

            stTransform cTemp;
            for (int i = 0; i < nNumMatrices; ++i)
            {
                // interpolate the parameters we need
                INTERPOLATE_WIND_PARAM(m_sBranchVertical.m_vOutputs[i]);
                INTERPOLATE_WIND_PARAM(m_sBranchHorizontal.m_vOutputs[i]);

#ifdef SPEEDTREE_UPVECTOR_Z
                // vertical oscillation and bend angle are in the same direction, so do them at the same time
                m_vWindMatrices[i].RotateAxisXYPlane_Opt(fOverThePoleScalar * (m_fFinalBendAngle + m_sBranchVertical.m_vOutputs[i]), stVec(fXRot, fYRot, 0.0f));
                // handle horizontal oscillation
                cTemp.RotateAxisXYPlane_Opt(fOverThePoleScalar * m_sBranchHorizontal.m_vOutputs[i], stVec(-fYRot, fXRot, 0.0f));
#endif
#ifdef SPEEDTREE_UPVECTOR_Y
                // vertical oscillation and bend angle are in the same direction, so do them at the same time
                m_vWindMatrices[i].RotateAxisXZPlane_Opt(fOverThePoleScalar * (m_fFinalBendAngle + m_sBranchVertical.m_vOutputs[i]), stVec(fYRot, 0.0f, fXRot));
                // handle horizontal oscillation
                cTemp.RotateAxisXZPlane_Opt(fOverThePoleScalar * m_sBranchHorizontal.m_vOutputs[i], stVec(-fXRot, 0.0f, fYRot));
#endif
                // final matrix
                m_vWindMatrices[i] = m_vWindMatrices[i] * cTemp;
            }
        }

        // leaf angle matrices
        if (bUpdateLeafAngleMatrices)
        {
            // build leaf angle matrices
            float afAdjustedCameraDir[3] = 
            {
                #ifdef SPEEDTREE_UPVECTOR_Z
                    fCameraX, fCameraY, fCameraZ
                #endif
                #ifdef SPEEDTREE_UPVECTOR_NEG_Z
                    -fCameraX, fCameraY, -fCameraZ
                #endif
                #ifdef SPEEDTREE_UPVECTOR_Y
                    -fCameraX, fCameraZ, fCameraY
                #endif
                #ifdef SPEEDTREE_UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
                    fCameraY, fCameraX, fCameraZ
                #endif
            };

            float fAzimuth = VecRad2Deg(atan2f(afAdjustedCameraDir[1], afAdjustedCameraDir[0]));
            float fPitch = VecRad2Deg(-asinf(afAdjustedCameraDir[2]));

            int nNumMatrices = st_min((int)pWind1->m_pDefinition->m_vLeafAngleMatrices.size( ),
                                      (int)pWind2->m_pDefinition->m_vLeafAngleMatrices.size( ));
            m_vLeafAngleMatrices.resize(nNumMatrices);

            for (unsigned int i = 0; i < (unsigned int) nNumMatrices; ++i)
            {
                stTransform& cMatrix = m_vLeafAngleMatrices[i];

                cMatrix.LoadIdentity( );
                #ifdef SPEEDTREE_UPVECTOR_Z
                    cMatrix.Rotate(fAzimuth, 'z');
                    cMatrix.Rotate(fPitch, 'y');
                    cMatrix.Rotate(m_sLeafRustling.m_vOutputs[i], 'z');
                    cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'x');
                #endif
                #ifdef SPEEDTREE_UPVECTOR_NEG_Z
                    cMatrix.Rotate(-fAzimuth, 'z');
                    cMatrix.Rotate(fPitch, 'y');
                    cMatrix.Rotate(m_sLeafRustling.m_vOutputs[i], 'z');
                    cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'x');
                #endif
                #ifdef SPEEDTREE_UPVECTOR_Y
                    cMatrix.Rotate(fAzimuth, 'y');
                    cMatrix.Rotate(fPitch, 'z');
                    cMatrix.Rotate(-m_sLeafRustling.m_vOutputs[i], 'y');
                    cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'x');
                #endif
                #ifdef SPEEDTREE_UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
                    cMatrix.Rotate(-fAzimuth, 'z');
                    cMatrix.Rotate(-fPitch, 'x');
                    cMatrix.Rotate(m_sLeafRustling.m_vOutputs[i], 'z');
                    cMatrix.Rotate(m_sLeafRocking.m_vOutputs[i], 'y');
                #endif
            }
        }
    }

    #undef INTERPOLATE_WIND_PARAM
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Random

inline int CSpeedWindDef::Random(void)
{
    if (m_bUseRandomBuffer)
    {
        st_assert(!m_vRandomBuffer.empty( ));
        return m_vRandomBuffer[m_nRandomIndex++ % c_nRandomBufferSize];
    }
    else
    {
        return rand( );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetUseRandomBuffer

void CSpeedWindDef::SetUseRandomBuffer(bool bUse, unsigned int uiSeed)
{
    m_bUseRandomBuffer = bUse;

    if (m_bUseRandomBuffer && uiSeed != 0)
    {
        m_vRandomBuffer.resize(c_nRandomBufferSize);

        // non-zero seed = reset
        srand(uiSeed);
        for (int i = 0; i < c_nRandomBufferSize; ++i)
            m_vRandomBuffer[i] = rand( );
    }
}
