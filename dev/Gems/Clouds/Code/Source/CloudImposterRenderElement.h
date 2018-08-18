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

#pragma once

#include <IRenderer.h>
#include "Cry_Camera.h"

//================================================================================

class IDynTexture;

namespace CloudsGem
{
    class CloudImposterRenderElement
        : public IRenderElementDelegate
    {
        bool IsImposterValid(const CameraViewParameters& viewParameters, float fRadiusX, float fRadiusY, float fCamRadiusX, float fCamRadiusY,
            const int iRequiredLogResX, const int iRequiredLogResY, const uint32 dwBestEdge);

        bool Display(bool bDisplayFrontOfSplit);

    public:
        static int s_MemUpdated;
        static int s_MemPostponed;
        static int s_PrevMemUpdated;
        static int s_PrevMemPostponed;
        static IDynTexture* s_pScreenTexture;

        CloudImposterRenderElement();
        virtual ~CloudImposterRenderElement();

        bool UpdateImposter();
        void ReleaseResources();
        bool PrepareForUpdate();

        // IRenderElementDelegate
        void mfPrepare(bool bCheckOverflow) override {};
        bool mfDraw(CShader* ef, SShaderPass* sl) override { return false; };

        const SMinMaxBox GetWorldSpaceBounds() { return m_WorldSpaceBV; }
        virtual bool IsSplit() { return m_bSplit; }
        virtual bool IsScreenImposter() { return m_bScreenImposter; }
        float GetTransparency() { return m_fCurTransparency; }
        void SetBBox(const Vec3& min, const Vec3& max) { m_WorldSpaceBV.SetMin(min); m_WorldSpaceBV.SetMax(max); }
        int GetFrameReset() { return m_nFrameReset; }
        int GetLogResolutionX() { return m_nLogResolutionX; }
        int GetLogResolutionY() { return m_nLogResolutionY; }
        CameraViewParameters& GetLastViewParameters() { return m_LastViewParameters; }
        IDynTexture** GetTexture() { return &m_pTexture; }
        IDynTexture** GetScreenTexture() { return &s_pScreenTexture; }
        IDynTexture** GetFrontTexture() { return &m_pFrontTexture; }
        IDynTexture** GetDepthTexture() { return &m_pTextureDepth; }
        float GetNear() { return m_fNear; }
        float GetFar() { return m_fFar; }
        Vec3 GetPosition() { return m_WorldSpaceBV.GetCenter(); }
        void SetScreenImposterState(bool state) { m_bScreenImposter = state; }
        void SetState(uint32 state) { m_State = state; }
        void SetAlphaRef(uint32 ref) { m_AlphaRef = ref; }
        void SetPosition(Vec3 pos) { m_vPos = pos; }
        void SetFrameResetValue(int frameResetValue) { m_nFrameReset = frameResetValue; }
        float GetRadiusX() { return m_fRadiusX; }
        float GetRadiusY() { return m_fRadiusY; }
        Vec3* GetQuadCorners() { return &m_vQuadCorners[0]; }
        IRenderElement* GetRE() { return m_gemRE; }
    private:
            
        int m_nFrameReset;
        int m_FrameUpdate;
        int m_nLogResolutionX{ 0 };                             //! Log of the x resolution
        int m_nLogResolutionY{ 0 };                             //! Log of the y resolution
        int m_AlphaRef{ -1 };                                   //! Reference for alpha blending

        float m_fErrorToleranceCosAngle{ 0.99999048072f };      // cos(0.004363323)  cosine of m_fErrorToleranceAngle used to check if IsImposterValid
        float m_fTimeUpdate;
        float m_fNear{0.f};                                     //! Near
        float m_fFar{0.f};                                      //! Far
        float m_fRadiusX{0.f};                                  //! y radius
        float m_fRadiusY{0.f};                                  //! x Radius
        float m_fCurTransparency{1.f};                          //! Current transparency value

        bool m_bScreenImposter{ false };                        //! Is this a 'screen' imposter
        bool m_bSplit{ false };                                 //! Do we need render a split imposter

        CameraViewParameters m_LastViewParameters;              //! Camera state as of last update
        Vec3 m_vQuadCorners[4];                                 //! in world space, relative to m_vPos, in clockwise order, can be rotated
        Vec3 m_vNearPoint{0, 0, 0};                             //! Is this used?
        Vec3 m_vFarPoint{0, 0, 0};                              //! Is this actually used??

        IDynTexture* m_pTexture{nullptr};
        IDynTexture* m_pFrontTexture{nullptr};
        IDynTexture* m_pTextureDepth{nullptr};
        SMinMaxBox m_WorldSpaceBV;
        uint32 m_State{GS_DEPTHWRITE};
        Vec3 m_vPos{0, 0, 0};
        Vec3 m_vLastSunDir{0, 0, 0};
        uint8 m_nLastBestEdge{0};                               // 0..11 this edge is favored to not jitter between different edges
        IRenderElement* m_gemRE;
    };
}