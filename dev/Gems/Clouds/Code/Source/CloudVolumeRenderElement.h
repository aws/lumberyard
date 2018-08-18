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

#include "VertexFormats.h"

namespace CloudsGem
{
    class CloudVolumeTexture;
    class CloudVolumeRenderElement 
        : public IRenderElementDelegate

    {
    public:
        CloudVolumeRenderElement();
        virtual ~CloudVolumeRenderElement() {};

        void mfPrepare(bool bCheckOverflow) override;
        bool mfDraw(CShader* ef, SShaderPass* sfm) override;
        bool mfSetSampler(int customId, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit) override;
        
        // Members
        Matrix34 m_inverseWorldMatrix; 
        Vec3 m_center{ 0.0f, 0.0f, 0.0f };
        Vec3 m_eyePosInWS{ 0.0f, 0.0f, 0.0f };
        Vec3 m_eyePosInOS{ 0.0f, 0.0f, 0.0f };
        Plane m_volumeTraceStartPlane;
        IVolumeTexture* m_pDensVol{ nullptr };
        IVolumeTexture* m_pShadVol{ nullptr };
        _smart_ptr<IRenderMesh> m_pHullMesh{ nullptr };
        bool m_viewerInsideVolume{ false };
        bool m_nearPlaneIntersectsVolume{ false };
        float m_alpha{ 1.0f };
        float m_scale{ 1.0f };
        AABB m_renderBoundsOS;
        IRenderElement* m_gemRE;
    };
}