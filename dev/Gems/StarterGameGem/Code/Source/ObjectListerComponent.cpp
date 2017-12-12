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


#include "StdAfx.h"
#include "ObjectListerComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <GameplayEventBus.h>

namespace StarterGameGem
{
    //-------------------------------------------
    // ObjectBase
    //-------------------------------------------
    void ObjectBase::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ObjectBase>()
                ->Version(1)
                ->SerializerForEmptyClass()
                ;
        }
    }

    //-------------------------------------------
    // ObjectEntity
    //-------------------------------------------
    AZ::u32 ObjectEntity::GetListSize()
    {
        return m_entities.size();
    }

    void* ObjectEntity::GetItemAt(AZ::u32 index)
    {
        if (index < m_entities.size())
        {
            return &m_entities[index];
        }
        else
        {
            AZ_Warning("StarterGame", false, "ObjectListerComponent::ObjectEntity: Attempted to index outside the valid range.");
            return nullptr;
        }
    }

    void ObjectEntity::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ObjectEntity, ObjectBase>()
                ->Version(1)
                ->Field("Entities", &ObjectEntity::m_entities)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ObjectEntity>("Entity List", "Entity object for lists.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &ObjectEntity::m_entities, "Entities", "An entity")
                    ;
            }
        }
    }

    //-------------------------------------------
    // ObjectString
    //-------------------------------------------
    AZ::u32 ObjectString::GetListSize()
    {
        return m_strings.size();
    }

    void* ObjectString::GetItemAt(AZ::u32 index)
    {
        if (index < m_strings.size())
        {
            return &m_strings[index];
        }
        else
        {
            AZ_Warning("StarterGame", false, "ObjectListerComponent::ObjectString: Attempted to index outside the valid range.");
            return nullptr;
        }
    }

    void ObjectString::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ObjectString, ObjectBase>()
                ->Version(1)
                ->Field("Strings", &ObjectString::m_strings)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ObjectString>("String List", "String object for lists.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &ObjectString::m_strings, "Strings", "A string")
                    ;
            }
        }
    }

    //-------------------------------------------
    // ObjectListerComponent
    //-------------------------------------------
    void ObjectListerComponent::Init()
    {
    }

    void ObjectListerComponent::Activate()
    {
        ObjectListerComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void ObjectListerComponent::Deactivate()
    {
        ObjectListerComponentRequestsBus::Handler::BusDisconnect();
    }

    void ObjectListerComponent::Reflect(AZ::ReflectContext* context)
    {
        ObjectBase::Reflect(context);
        ObjectEntity::Reflect(context);
        ObjectString::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ObjectListerComponent>()
                ->Version(1)
                ->Field("Objects", &ObjectListerComponent::m_objects)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ObjectListerComponent>("Object Lister", "Create lists that can be accessed in Lua")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ->DataElement(0, &ObjectListerComponent::m_objects, "Objects", "Lists of objects.")
                    ;
            }
        }

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // ObjectFromList return type
            behavior->Class<ObjectFromList>("ObjectFromList")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("entityId", BehaviorValueProperty(&ObjectFromList::m_entityId))
                ->Property("string", BehaviorValueProperty(&ObjectFromList::m_string))
                ;

            behavior->EBus<ObjectListerComponentRequestsBus>("ObjectListerComponentRequestsBus")
                ->Event("GetListSize", &ObjectListerComponentRequestsBus::Events::GetListSize)
                ->Event("GetListItem", &ObjectListerComponentRequestsBus::Events::GetListItem)
                ;
        }
    }

    AZ::u32 ObjectListerComponent::GetListSize(AZ::u32 listID)
    {
        AZ::u32 size = 0;
        if (listID < m_objects.size())
        {
            size = m_objects[listID]->GetListSize();
        }
        else
        {
            AZ_Warning("StarterGame", false, "ObjectListerComponent: Attempted to index a list outside the valid range.");
        }
        return size;
    }

    ObjectFromList ObjectListerComponent::GetListItem(AZ::u32 listID, AZ::u32 index)
    {
        ObjectFromList res;
        if (listID < m_objects.size())
        {
            void* item = m_objects[listID]->GetItemAt(index);
            res = GenerateReturnFromVoidPtr(item);
        }
        else
        {
            AZ_Warning("StarterGame", false, "ObjectListerComponent: Attempted to index a list outside the valid range.");
        }
        return res;
    }

    ObjectFromList ObjectListerComponent::GenerateReturnFromVoidPtr(void* item) const
    {
        ObjectFromList res;

        if (AZ::EntityId* entityId = static_cast<AZ::EntityId*>(item))
        {
            res.m_entityId = *entityId;
        }
        else if (AZStd::string* str = static_cast<AZStd::string*>(item))
        {
            res.m_string = *str;
        }

        return res;
    }

}
