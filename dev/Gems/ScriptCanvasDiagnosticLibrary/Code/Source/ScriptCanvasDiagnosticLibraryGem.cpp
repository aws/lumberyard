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

#include "precompiled.h"
#include <platform_impl.h> // must be included once per DLL so things from CryCommon will function

#include <ScriptCanvasDiagnosticLibrary/ScriptCanvasDiagnosticLibraryGem.h>
#include <ScriptCanvasDiagnosticSystemComponent.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Debug.h>

namespace ScriptCanvasDiagnostics
{
    Module::Module()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            ScriptCanvasDiagnostics::SystemComponent::CreateDescriptor(),
        });

        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(ScriptCanvas::Libraries::Debug::GetComponentDescriptors());
        m_descriptors.insert(m_descriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
    }

    AZ::ComponentTypeList Module::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList({ ScriptCanvasDiagnostics::SystemComponent::RTTI_Type() });
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(ScriptCanvasDiagnosticLibrary_d969b997bb7f4342ba0b3eedc378025d, ScriptCanvasDiagnostics::Module)

