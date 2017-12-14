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
#include <Core/NodeFunctionGeneric.h>

#include "Entity.h"

namespace ScriptCanvas
{
    namespace EntityNodes
    {
        AZ_INLINE bool IsValid(AZ::EntityId id)
        {
            return id.IsValid();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsValid, "Entity/Game Entity", "{7CEC53AE-E12B-4738-B542-4587B8B95DC2}", "returns true if the entity id is valid", "");

        using EntityRegistrar = RegistrarGeneric<
            IsValidNode
        >;
    }

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
            AddNodeToRegistry<Entity, EntityID>(nodeRegistry);
            AddNodeToRegistry<Entity, EntityRef>(nodeRegistry);

            EntityNodes::EntityRegistrar::AddToRegistry<Entity>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Entity::GetComponentDescriptors()
        {
            auto descriptors = AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Entity::Rotate::CreateDescriptor(),
                ScriptCanvas::Nodes::Entity::EntityID::CreateDescriptor(),
                ScriptCanvas::Nodes::Entity::EntityRef::CreateDescriptor(),
            });

            EntityNodes::EntityRegistrar::AddDescriptors(descriptors);

            return descriptors;
        }
    }
}