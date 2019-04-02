/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "StdAfx.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "RoadsAndRiversModule.h"
#include "RoadComponent.h"
#include "RiverComponent.h"
#include "SplineGeometry.h"

#if defined(ROADS_RIVERS_EDITOR)
#include "EditorRoadComponent.h"
#include "EditorRiverComponent.h"
#include "LegacyConversion/RoadsAndRiversLegacyConverter.h"
#endif

namespace RoadsAndRivers
{
    RoadsAndRiversModule::RoadsAndRiversModule()
        : CryHooksModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            RoadsAndRiversSystemComponent::CreateDescriptor(),
            RoadComponent::CreateDescriptor(),
            RiverComponent::CreateDescriptor(),
        });

#if defined(ROADS_RIVERS_EDITOR)
        m_descriptors.insert(m_descriptors.end(), {
            EditorRoadComponent::CreateDescriptor(),
            EditorRiverComponent::CreateDescriptor()
        });
        m_legacyConverter = AZStd::make_unique<RoadsAndRiversConverter>();
#endif
    }

    void RoadsAndRiversSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        SplineUtils::ReflectHelperClasses(context);
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(RoadsAndRivers_b7c7bf6c8acf4c33acb5f2a372760d3b, RoadsAndRivers::RoadsAndRiversModule)
