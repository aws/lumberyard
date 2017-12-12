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

#if !defined(SCRIPTCANVAS_EDITOR)

#include <ScriptCanvasGem.h>

#if !defined(AZ_MONOLITHIC_BUILD)
#include <platform_impl.h> // must be included once per DLL so things from CryCommon will function
#endif

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

#include <SystemComponent.h>

#include <Builder/CoreBuilderSystemComponent.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/Libraries/Libraries.h>

#include <ScriptCanvas/Debugger/Debugger.h>

namespace ScriptCanvas
{
    ////////////////////////////////////////////////////////////////////////////
    // ScriptCanvasModule
    ////////////////////////////////////////////////////////////////////////////

    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    ScriptCanvasModule::ScriptCanvasModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            ScriptCanvas::SystemComponent::CreateDescriptor(),
            ScriptCanvasBuilder::CoreBuilderSystemComponent::CreateDescriptor(),
            ScriptCanvas::Graph::CreateDescriptor(),
            ScriptCanvas::Debugger::Component::CreateDescriptor(),
       });

        ScriptCanvas::InitNodeRegistry();
        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors = ScriptCanvas::GetLibraryDescriptors();
        m_descriptors.insert(m_descriptors.end(), libraryDescriptors.begin(), libraryDescriptors.end());
    }

    ScriptCanvasModule::~ScriptCanvasModule()
    {
        ScriptCanvas::ResetNodeRegistry();
    }

    AZ::ComponentTypeList ScriptCanvasModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components;

        components.insert(components.end(), std::initializer_list<AZ::Uuid> {
            azrtti_typeid<ScriptCanvas::SystemComponent>(),
            azrtti_typeid<ScriptCanvasBuilder::CoreBuilderSystemComponent>(),
            azrtti_typeid<ScriptCanvas::Debugger::Component>(),
        });

        return components;
    }
}

AZ_DECLARE_MODULE_CLASS(ScriptCanvasGem_869a0d0ec11a45c299917d45c81555e6, ScriptCanvas::ScriptCanvasModule)

#endif // !SCRIPTCANVAS_EDITOR