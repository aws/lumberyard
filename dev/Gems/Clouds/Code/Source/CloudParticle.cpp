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
#include "StdAfx.h"
#include "CloudParticle.h"

namespace CloudsGem
{
    void CloudParticle::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CloudParticle>()
                ->Version(1)
                ->Field("Position", &CloudParticle::m_position)
                ->Field("Radius", &CloudParticle::m_radius)
                ->Field("UV0", &CloudParticle::m_topLeftUV)
                ->Field("UV1", &CloudParticle::m_bottomRightUV)
                ->Field("Size", &CloudParticle::m_size)
                ->Field("SizeV", &CloudParticle::m_sizeVariance)
                ->Field("Sort", &CloudParticle::m_squareSortDistance)
                ->Field("TextureId", &CloudParticle::m_textureId);
        }
    }

    CloudParticleData::CloudParticleData()
    {
        m_bounds.CreateFromPoint(AZ::Vector3::CreateZero());
    }

    void CloudParticleData::Reflect(AZ::ReflectContext* reflection)
    {
        CloudParticle::Reflect(reflection);
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<CloudParticleData>()
                ->Version(1)
                ->Field("Bounds", &CloudParticleData::m_bounds)
                ->Field("Offset", &CloudParticleData::m_offset)
                ->Field("Particles", &CloudParticleData::m_particles);
        }
    }
}