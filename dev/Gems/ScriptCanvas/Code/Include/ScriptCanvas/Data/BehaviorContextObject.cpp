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
#include "BehaviorContextObject.h"
#include <ScriptCanvas/SystemComponent.h>

#include <AzCore/Component/EntityBus.h>

namespace ScriptCanvas
{
    void BehaviorContextObject::OnReadBegin()
    {
        if (!IsOwned())
        {
            Clear();
        }
    }

    void BehaviorContextObject::OnWriteEnd()
    {
        if (m_object.empty())
        {
            AZ_Assert(!IsOwned(), "Errors in BehaviorContextObject serialization. Empty objects should be not be considered owned.");
        }
        else 
        {
            AZ_Assert(IsOwned(), "Errors in BehaviorContextObject serialization. Not empty objects should be owned.");
        }
    }

    void BehaviorContextObject::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<BehaviorContextObject>()
                ->Version(0)
                ->EventHandler<SerializeContextEventHandler>()
                ->Field("m_flags", &BehaviorContextObject::m_flags)
                ->Field("m_object", &BehaviorContextObject::m_object)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BehaviorContextObject>("BehaviorContextObject", "BehaviorContextObject")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BehaviorContextObject::m_object, "Datum", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void BehaviorContextObject::SerializeContextEventHandler::OnReadBegin(void* classPtr)
    {
        BehaviorContextObject* object = reinterpret_cast<BehaviorContextObject*>(classPtr);
        object->OnReadBegin();
    }

    void BehaviorContextObject::SerializeContextEventHandler::OnWriteEnd(void* classPtr)
    {
        BehaviorContextObject* object = reinterpret_cast<BehaviorContextObject*>(classPtr);
        object->OnWriteEnd();
    }

    SystemComponent* BehaviorContextObject::GetSystemComponent()
    {
        SystemComponent* systemComponent{};
        SystemRequestBus::BroadcastResult(systemComponent, &SystemRequests::ModSystemComponent);
        return systemComponent;
    }

    BehaviorContextObjectPtr BehaviorContextObject::Create(const AZ::BehaviorClass& behaviorClass, const void* value)
    {
        CheckClass(behaviorClass);

        if (SystemComponent* scEditorSystemComponent = GetSystemComponent())
        {
            BehaviorContextObject* ownedObject = value ? CreateCopy(behaviorClass, value) : CreateDefault(behaviorClass);
            scEditorSystemComponent->AddOwnedObjectReference(ownedObject->Get(), ownedObject);
            return BehaviorContextObjectPtr(ownedObject);
        }

        AZ_Assert(false, "The Script Canvas SystemComponent needs to be part of the SystemEntity!");
        return nullptr;
    }

    BehaviorContextObjectPtr BehaviorContextObject::CreateReference(const AZ::Uuid& typeID, void* reference)
    {
        const AZ::u32 referenceFlags(0);
        SystemComponent* scEditorSystemComponent = GetSystemComponent();
        BehaviorContextObject* ownedObject = scEditorSystemComponent ? scEditorSystemComponent->FindOwnedObjectReference(reference) : nullptr;
        return ownedObject
            ? BehaviorContextObjectPtr(ownedObject)
            : BehaviorContextObjectPtr(aznew BehaviorContextObject(reference, GetAnyTypeInfoReference(typeID), referenceFlags));
    }

    void BehaviorContextObject::release()
    {
        if (--m_referenceCount == 0)
        {
            if (SystemComponent* scEditorSystemComponent = GetSystemComponent())
            {
                scEditorSystemComponent->RemoveOwnedObjectReference(Get());
            }
            delete this;
        }
    }

} // namespace ScriptCanvas