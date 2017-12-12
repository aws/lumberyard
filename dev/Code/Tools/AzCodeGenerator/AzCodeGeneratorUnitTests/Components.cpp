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

#include "Components.h"

#include "Components.generated.cpp"
#include "Enums.generated.cpp"

void DummyComponent::Activate()
{
}

void DummyComponent::Deactivate()
{    
}

void MyCodegenComponent::Activate()
{
//    MyCodegenBus::Handler::BusConnect();
}

void MyCodegenComponent::Deactivate()
{
//    MyCodegenBus::Handler::BusDisconnect();
}

/*void MyCodegenComponent::OnPrintSerializeContextReflection(AZ::SerializeContext* context, void* classPtr, const AZ::Uuid& classUuid)
{
    if (context)
    {
        context->EnumerateInstance(classPtr, classUuid,
            [](void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement) -> bool
        {
            (void)instance;
            (void)classData;
            (void)classElement;
            if (classElement)
            {
                AZ_TracePrintf("Test", "Address: %p ElementName: %s HasEditData: %s\n", instance, classElement->m_name, classElement->m_editData ? "true" : "false");
            }
            else
            {
                AZ_TracePrintf("Test", "Address: %p TypeId: %s ClassName: %s HasEditData: %s\n", instance, classData->m_typeId.ToString<AZStd::string>().c_str(), classData->m_name, classData->m_editData ? "true" : "false");
            }

            return true;
        },
            []() -> bool { return true;  },
            AZ::SerializeContext::ENUM_ACCESS_FOR_READ,
            nullptr,
            nullptr,
            nullptr);
    }
}*/
