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

#include <LandscapeCanvasModule.h>
#if defined(LANDSCAPECANVAS_EDITOR)
#include <LandscapeCanvasSystemComponent.h>
#endif
#include <LandscapeCanvasComponent.h>

namespace LandscapeCanvas
{
    LandscapeCanvasModule::LandscapeCanvasModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
#if defined(LANDSCAPECANVAS_EDITOR)
            LandscapeCanvasSystemComponent::CreateDescriptor(),
#endif
            LandscapeCanvasComponent::CreateDescriptor()
        });
    }

    /**
     * Add required SystemComponents to the SystemEntity.
     */
    AZ::ComponentTypeList LandscapeCanvasModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
#if defined(LANDSCAPECANVAS_EDITOR)
            azrtti_typeid<LandscapeCanvasSystemComponent>(),
#endif
        };
    }
}

#if !defined(LANDSCAPECANVAS_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(LandscapeCanvas_19c2b2d5018940108baf252934b8e6bf, LandscapeCanvas::LandscapeCanvasModule)
#endif
