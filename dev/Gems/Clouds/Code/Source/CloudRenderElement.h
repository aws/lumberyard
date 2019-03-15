
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

struct SRenderShaderResources;
class CShaderResources;

#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace CloudsGem
{
    class CloudParticle;
    class CloudRenderElement
        : public IRenderElementDelegate
    {
    public:
        CloudRenderElement();
        virtual ~CloudRenderElement() {}

        void mfPrepare(bool bCheckOverflow) override;
        bool mfDraw(CShader* shader, SShaderPass* pass) override;

        virtual void SetParticles(const AZStd::vector<CloudParticle>& particles, const SMinMaxBox& box);
        IRenderElement* GetRE() { return m_gemRE; }

    protected:

        enum class ESortDirection : uint32
        {
            eSort_TOWARD,
            eSort_AWAY
        };

        void SortParticles(const Vec3& vViewDir, const Vec3& vSortPoint, ESortDirection eDir);
        void GetIllumParams(ColorF& specColor, ColorF& diffColor);
        void ShadeCloud(Vec3 vPos);
        void DrawBillboards(const CameraViewParameters& camera);
        bool Display(bool bDisplayFrontOfSplit);
        void UpdateWorldSpaceBounds(CRenderObject* pObj);
        inline float GetScale() { return m_fScale; }
        bool UpdateImposter(CRenderObject* pObj);

        AZStd::vector<AZStd::shared_ptr<CloudParticle>> m_particles;
        SMinMaxBox m_boundingBox;           
        bool m_bUseAnisoLighting{true};     
        Vec3 m_vLastSortViewDir{0, 0, 0};
        Vec3 m_vLastSortCamPos{ 0, 0, 0};
        Vec3 m_vSortPos{0, 0, 0};
        bool m_bReshadeCloud{true};         
        bool m_bEnabled{true};              
        float m_fScale{1.f};                

        ITexture* m_pTexParticle{nullptr};
        uint32 m_nNumPlanes{0};
        uint32 m_nNumColorGradients{0};
        ColorF m_CurSpecColor{Col_White};
        ColorF m_CurDiffColor{Col_White};
        float m_fCloudColorScale{1};
        IRenderElement* m_gemRE{ nullptr };
    };
}
