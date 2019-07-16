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
#include "Vegetation_precompiled.h"
#include "DescriptorRenderGroup.h"

#include <I3DEngine.h>
#include <Tarray.h>
#include <MathConversion.h>

namespace Vegetation
{
    DescriptorRenderGroup::DescriptorRenderGroup(const Descriptor& descriptor, I3DEngine* engine)
        : m_engine(engine)
    {
        if (!m_engine)
        {
            AZ_Error("vegetation", false, "Could not acquire I3DEngine!");
            return;
        }

        if (!descriptor.m_meshAsset.IsReady())
        {
            AZ_Error("vegetation", false, "Mesh asset must be valid and loaded!");
            return;
        }

        StatInstGroupEventBus::BroadcastResult(m_id, &StatInstGroupEventBus::Events::GenerateStatInstGroupId);
        if (m_id == StatInstGroupEvents::s_InvalidStatInstGroupId)
        {
            AZ_Error("vegetation", false, "GenerateStatInstGroupId failed!");
            return;
        }

        IStatInstGroup group;
        group.pStatObj = descriptor.m_meshAsset.Get()->m_statObj;
        group.bAutoMerged = descriptor.m_autoMerge;
        group.pMaterial = descriptor.m_materialOverride;
        group.fBending = descriptor.m_windBending;
        group.fAirResistance = descriptor.m_airResistance;
        group.fStiffness = descriptor.m_stiffness;
        group.fVariance = descriptor.m_variance;
        group.fDamping = descriptor.m_damping;
        group.fLodDistRatio = descriptor.m_lodDistanceRatio;
        group.fMaxViewDistRatio = descriptor.m_viewDistanceRatio;
        group.bUseTerrainColor = descriptor.m_useTerrainColor;

        if (!m_engine->SetStatInstGroup(m_id, group))
        {
            AZ_Error("vegetation", false, "SetStatInstGroup failed!");
            StatInstGroupEventBus::Broadcast(&StatInstGroupEventBus::Events::ReleaseStatInstGroupId, m_id);
            m_id = StatInstGroupEvents::s_InvalidStatInstGroupId;
        }
    }

    DescriptorRenderGroup::~DescriptorRenderGroup()
    {
        if (m_id != StatInstGroupEvents::s_InvalidStatInstGroupId)
        {
            if (m_engine)
            {
                IStatInstGroup group;
                m_engine->SetStatInstGroup(m_id, group);
            }
            StatInstGroupEventBus::Broadcast(&StatInstGroupEventBus::Events::ReleaseStatInstGroupId, m_id);
            m_id = StatInstGroupEvents::s_InvalidStatInstGroupId;
        }
    }

    bool DescriptorRenderGroup::IsReady() const
    {
        return m_id != StatInstGroupEvents::s_InvalidStatInstGroupId;
    }

    StatInstGroupId DescriptorRenderGroup::GetId() const
    {
        return m_id;
    }

    bool DescriptorRenderGroup::ShouldDescriptorsShareGroup(const Descriptor& lhs, const Descriptor& rhs)
    {
        return
            lhs.m_autoMerge == rhs.m_autoMerge &&
            lhs.m_meshAsset == rhs.m_meshAsset &&
            lhs.m_materialOverride == rhs.m_materialOverride &&
            lhs.m_windBending == rhs.m_windBending &&
            lhs.m_airResistance == rhs.m_airResistance &&
            lhs.m_stiffness == rhs.m_stiffness &&
            lhs.m_variance == rhs.m_variance &&
            lhs.m_damping == rhs.m_damping &&
            lhs.m_lodDistanceRatio == rhs.m_lodDistanceRatio &&
            lhs.m_viewDistanceRatio == rhs.m_viewDistanceRatio;
    }
}