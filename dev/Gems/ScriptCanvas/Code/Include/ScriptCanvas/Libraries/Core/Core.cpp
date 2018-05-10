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

#include <ScriptCanvas/Libraries/Libraries.h>

#include "Core.h"

namespace ScriptCanvas
{
    namespace Library
    {
        void Core::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Core, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Core>("Core", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/Core.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".time")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TimeNodeTitlePalette")
                        ;
                }
            }

            Nodes::Core::EBusEventEntry::Reflect(reflection);
            Nodes::Core::ExtractProperty::SlotMetadata::Reflect(reflection);
        }

        void Core::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Core;
            AddNodeToRegistry<Core, Assign>(nodeRegistry);
            AddNodeToRegistry<Core, Error>(nodeRegistry);
            AddNodeToRegistry<Core, ErrorHandler>(nodeRegistry);
            AddNodeToRegistry<Core, Method>(nodeRegistry);
            AddNodeToRegistry<Core, BehaviorContextObjectNode>(nodeRegistry);
            AddNodeToRegistry<Core, Start>(nodeRegistry);
            AddNodeToRegistry<Core, ScriptCanvas::Nodes::Core::String>(nodeRegistry);
            AddNodeToRegistry<Core, EBusEventHandler>(nodeRegistry);
            AddNodeToRegistry<Core, ExtractProperty>(nodeRegistry);
            AddNodeToRegistry<Core, GetVariableNode>(nodeRegistry);
            AddNodeToRegistry<Core, SetVariableNode>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Core::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Core::Assign::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Error::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ErrorHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Method::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::BehaviorContextObjectNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Start::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::String::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::EBusEventHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ExtractProperty::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::GetVariableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::SetVariableNode::CreateDescriptor(),
            });
        }
    }
}