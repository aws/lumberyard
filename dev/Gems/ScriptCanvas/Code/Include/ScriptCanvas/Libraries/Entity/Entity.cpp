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

#include <Libraries/Libraries.h>

#include "Entity.h"

namespace ScriptCanvas
{
    namespace Library
    {
        void Entity::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Entity, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Entity>("Entity", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/Entity.png")
                        ;
                }

            }
        }

        void Entity::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Entity;
            AddNodeToRegistry<Entity, Rotate>(nodeRegistry);
            AddNodeToRegistry<Entity, EntityRef>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Entity::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Entity::Rotate::CreateDescriptor(),
                ScriptCanvas::Nodes::Entity::EntityRef::CreateDescriptor(),
            });
        }
    }
}