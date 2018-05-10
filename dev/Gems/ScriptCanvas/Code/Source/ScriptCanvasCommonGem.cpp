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
#include <ScriptCanvasGem.h>

#include <SystemComponent.h>

#include <Asset/RuntimeAssetSystemComponent.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Libraries.h>

#include <ScriptCanvas/Debugger/Debugger.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

namespace ScriptCanvas
{
    /////////////////////////////
    // ScriptCanvasModuleCommon
    /////////////////////////////

    ScriptCanvasModuleCommon::ScriptCanvasModuleCommon()
        : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            // System Component
            ScriptCanvas::SystemComponent::CreateDescriptor(),
            
            // Components
            ScriptCanvas::Debugger::Component::CreateDescriptor(),
            ScriptCanvas::Graph::CreateDescriptor(),
            ScriptCanvas::GraphVariableManagerComponent::CreateDescriptor(),
            ScriptCanvas::RuntimeComponent::CreateDescriptor(),
            
            // ScriptCanvasBuilder
            ScriptCanvas::RuntimeAssetSystemComponent::CreateDescriptor()
        });
        
        ScriptCanvas::InitNodeRegistry();
        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors = ScriptCanvas::GetLibraryDescriptors();
        m_descriptors.insert(m_descriptors.end(), libraryDescriptors.begin(), libraryDescriptors.end());

        InitDataRegistry();
    }

    ScriptCanvasModuleCommon::~ScriptCanvasModuleCommon()
    {
        ScriptCanvas::ResetNodeRegistry();
        ResetDataRegistry();
    }
    
    AZ::ComponentTypeList ScriptCanvasModuleCommon::GetCommonSystemComponents() const
    {   
        return std::initializer_list<AZ::Uuid> {
            azrtti_typeid<ScriptCanvas::SystemComponent>(),
            azrtti_typeid<ScriptCanvas::RuntimeAssetSystemComponent>(),
            azrtti_typeid<ScriptCanvas::Debugger::Component>(),
        };
    }
}