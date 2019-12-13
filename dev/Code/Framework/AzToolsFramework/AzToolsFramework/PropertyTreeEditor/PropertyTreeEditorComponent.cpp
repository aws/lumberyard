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

#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditorComponent.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void PropertyTreeEditorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PropertyTreeEditorComponent>();
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<PropertyTreeEditor>("PropertyTreeEditor")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Property")
                    ->Attribute(AZ::Script::Attributes::Module, "property")

                    ->Method("BuildPathsList", &PropertyTreeEditor::BuildPathsList, nullptr, "Get a complete list of all property paths in the tree.")
                        ->Attribute(AZ::Script::Attributes::Alias, "build_paths_list")

                    ->Method("GetProperty", &PropertyTreeEditor::GetProperty, nullptr, "Gets a property value.")
                        ->Attribute(AZ::Script::Attributes::Alias, "get_value")
                    ->Method("SetProperty", &PropertyTreeEditor::SetProperty, nullptr, "Sets a property value.")
                        ->Attribute(AZ::Script::Attributes::Alias, "set_value")
                    ;
            }
        }
    }
}
