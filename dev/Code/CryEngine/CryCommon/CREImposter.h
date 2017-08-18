/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_CREIMPOSTER_H
#define CRYINCLUDE_CRYCOMMON_CREIMPOSTER_H
#pragma once

#include <IRenderer.h>
#include "Cry_Camera.h"

//================================================================================

struct SDynTexture2;
struct SDynTexture;

class CTerrainNode;

class CREImposter
    : public CRendElementBase
{
    friend class CRECloud;

    CameraViewParameters m_LastViewParameters;
    bool m_bScreenImposter;
    bool m_bSplit;
    float m_fRadiusX;
    float m_fRadiusY;
    Vec3 m_vQuadCorners[4];                             // in world space, relative to m_vPos, in clockwise order, can be rotated
    Vec3 m_vNearPoint;
    Vec3 m_vFarPoint;
    int m_nLogResolutionX;
    int m_nLogResolutionY;
    IDynTexture* m_pTexture;
    static IDynTexture* m_pScreenTexture;
    IDynTexture* m_pFrontTexture;
    IDynTexture* m_pTextureDepth;
    float m_fErrorToleranceCosAngle;            // cosine of m_fErrorToleranceAngle used to check if IsImposterValid
    SMinMaxBox m_WorldSpaceBV;
    CTerrainNode* m_pTerrainNode;
    uint32 m_State;
    int m_AlphaRef;
    float m_fCurTransparency;
    ColorF m_ColorHelper;
    Vec3 m_vPos;
    Vec3 m_vLastSunDir;
    uint8 m_nLastBestEdge;                              // 0..11 this edge is favored to not jitter between different edges
    float m_fNear;
    float m_fFar;

    bool IsImposterValid(const CameraViewParameters& viewParameters, float fRadiusX, float fRadiusY, float fCamRadiusX, float fCamRadiusY,
        const int iRequiredLogResX, const int iRequiredLogResY, const uint32 dwBestEdge);

    void ReleaseResources();
    const SMinMaxBox& mfGetWorldSpaceBounds()  { return m_WorldSpaceBV; }
    bool Display(bool bDisplayFrontOfSplit);
    bool IsSplit() { return m_bSplit; }
    bool UpdateImposter();
    bool PrepareForUpdate();
    Vec3 GetPosition();

public:
    int m_nFrameReset;
    int m_FrameUpdate;
    float m_fTimeUpdate;
    static int m_MemUpdated;
    static int m_MemPostponed;
    static int m_PrevMemUpdated;
    static int m_PrevMemPostponed;

    CREImposter()
        : CRendElementBase()
        , m_pTexture(NULL)
        , m_pFrontTexture(NULL)
        , m_pTextureDepth(NULL)
        , m_bSplit(false)
        , m_fRadiusX(0)
        , m_fRadiusY(0)
        , m_fErrorToleranceCosAngle(cos(DEG2RAD(0.25f)))
        , m_bScreenImposter(false)
        , m_State(GS_DEPTHWRITE)
        , m_AlphaRef(-1)
        , m_fCurTransparency(1.0f)
        , m_FrameUpdate(0)
        , m_nFrameReset(0)
        , m_fTimeUpdate(0)
        , m_pTerrainNode(0)
        , m_vLastSunDir(0, 0, 0)
        , m_nLogResolutionX(0)
        , m_nLogResolutionY(0)
        , m_nLastBestEdge(0)
    {
        mfSetType(eDATA_Imposter);
        mfUpdateFlags(FCEF_TRANSFORM);
        m_ColorHelper = Col_White;
    }
    virtual ~CREImposter()
    {
        ReleaseResources();
    }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sl);

    virtual CTerrainNode* GetTerrainNode() { return m_pTerrainNode; }
    virtual void SetTerrainNode(CTerrainNode* pTerrainNode) { m_pTerrainNode = pTerrainNode; }
    virtual void SetBBox(const Vec3& min, const Vec3& max) { m_WorldSpaceBV.SetMin(min); m_WorldSpaceBV.SetMax(max); }
};

#endif // CRYINCLUDE_CRYCOMMON_CREIMPOSTER_H
