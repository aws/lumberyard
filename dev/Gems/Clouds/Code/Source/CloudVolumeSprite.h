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

#include "CloudParticle.h"
#include "CloudComponentRenderNode.h"

namespace CloudsGem
{
    /**
     * CloudVolumeSprite
     */
    class CloudVolumeSprite : public ICloudVolume
    {
    public:
        
        CloudVolumeSprite();
        virtual ~CloudVolumeSprite();

        const AABB& GetBoundingBox() const override { return m_worldBoundingBox; }
        MaterialPtr GetMaterial() override { return m_material; }
        void SetBoundingBox(const AABB& boundingBox) override { m_worldBoundingBox = boundingBox; }
        void SetMaterial(MaterialPtr material) override { m_material = material; }
        void SetDensity(float density) { /* not used for sprite cloud*/ }
        void Update(const Matrix34& worldMatrix, const Vec3& offset) override;
        void Refresh(CloudParticleData& cloudData, const Matrix34& worldMatrix) override;
        void Render(const struct SRendParams& rParams, const SRenderingPassInfo& passInfo, float alpha, int isAfterWater) override;

        // Members
        AABB m_localBoundingBox;
        AABB m_worldBoundingBox;
        Vec3 m_origin{ 0.0f, 0.0f, 0.0f };
        Matrix34 m_worldMatrix;
        _smart_ptr<IMaterial> m_material{ nullptr };
        CloudRenderElement* m_renderElement{ nullptr };
        CloudImposterRenderElement* m_pREImposter{ nullptr };
    };
}