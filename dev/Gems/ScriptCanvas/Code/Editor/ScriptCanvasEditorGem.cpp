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

#if defined (SCRIPTCANVAS_EDITOR)

#include <ScriptCanvasGem.h>

#include <platform_impl.h> // must be included once per DLL so things from CryCommon will function

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

#include <SystemComponent.h>

#include <Core/GraphAssetHandler.h>
#include <Core/GraphAsset.h>
#include <Core/Graph.h>

#include <Builder/BuilderSystemComponent.h>

#include <Debugger/Debugger.h>

#include <Editor/SystemComponent.h>
#include <Editor/Metrics.h>

#include <Editor/Components/IconComponent.h>
#include <Editor/Model/EntityMimeDataHandler.h>

#include <Libraries/Libraries.h>
#include <Editor/Nodes/EditorLibrary.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>

#include <Builder/CoreBuilderSystemComponent.h>
#include <Editor/GraphCanvas/Components/ClassMethodNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EBusHandlerNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EBusHandlerEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EBusSenderNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EntityRefNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/GetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/SetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/UserDefinedNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/VariableNodeDescriptorComponent.h>

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
            ScriptCanvas::Graph::CreateDescriptor(),
            ScriptCanvasBuilder::BuilderSystemComponent::CreateDescriptor(),
            ScriptCanvasBuilder::CoreBuilderSystemComponent::CreateDescriptor(),
            ScriptCanvasEditor::Graph::CreateDescriptor(),
            ScriptCanvasEditor::EditorScriptCanvasComponent::CreateDescriptor(),
            ScriptCanvasEditor::SystemComponent::CreateDescriptor(),
            ScriptCanvas::Debugger::Component::CreateDescriptor(),
            ScriptCanvasEditor::Metrics::SystemComponent::CreateDescriptor(),
            ScriptCanvasEditor::IconComponent::CreateDescriptor(),
            ScriptCanvasEditor::EntityMimeDataHandler::CreateDescriptor(),

            // GraphCanvas additions
            // Base Descriptor
            ScriptCanvasEditor::NodeDescriptorComponent::CreateDescriptor(),

            // Node Type Descriptor
            ScriptCanvasEditor::ClassMethodNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::EBusHandlerNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::EBusHandlerEventNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::EBusSenderNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::EntityRefNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::GetVariableNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::SetVariableNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::UserDefinedNodeDescriptorComponent::CreateDescriptor(),
            ScriptCanvasEditor::VariableNodeDescriptorComponent::CreateDescriptor()
        });

        ScriptCanvas::InitNodeRegistry();
        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors = ScriptCanvas::GetLibraryDescriptors();
        m_descriptors.insert(m_descriptors.end(), libraryDescriptors.begin(), libraryDescriptors.end());
        
        libraryDescriptors = ScriptCanvasEditor::GetLibraryDescriptors();
        m_descriptors.insert(m_descriptors.end(), libraryDescriptors.begin(), libraryDescriptors.end());
        ScriptCanvasEditor::Library::Editor::InitNodeRegistry(GetNodeRegistry().Get());
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
                azrtti_typeid<ScriptCanvasBuilder::BuilderSystemComponent>(),
                azrtti_typeid<ScriptCanvasBuilder::CoreBuilderSystemComponent>(),
                azrtti_typeid<ScriptCanvasEditor::SystemComponent>(),
                azrtti_typeid<ScriptCanvasEditor::Metrics::SystemComponent>(),
                azrtti_typeid<ScriptCanvas::Debugger::Component>(),
        });

        return components;
    }
}

AZ_DECLARE_MODULE_CLASS(ScriptCanvasGem_869a0d0ec11a45c299917d45c81555e6, ScriptCanvas::ScriptCanvasModule)

#endif // SCRIPTCANVAS_EDITOR
