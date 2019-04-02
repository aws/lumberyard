/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "Native.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

// CPP (or header)
void MyNativeStruct::Reflect(AZ::ReflectContext* reflection)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
    if (serializeContext)
    {
        serializeContext->Class<MyNativeStruct>()->
            Version(1)->
            Field("m_data1", &MyNativeStruct::m_data1)->
            Field("m_data2", &MyNativeStruct::m_data2);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<MyNativeStruct>("My Native Class", "Class used to store data1 and data2")->
                ClassElement(AZ::Edit::ClassElements::Group, "Special data group")->
                Attribute("ChangeNotify", &MyNativeStruct::IsShowData2)->
                DataElement("ComboSelector", &MyNativeStruct::m_data1)->
                Attribute("NumOptions", 2)->
                Attribute("GetOptions", &MyNativeStruct::GetData1Option)->
                DataElement("Slider", &MyNativeStruct::m_data2)->
                Attribute("Step", 5);
        }
    }
}

void MyNativeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("ServiceB", 0x1982c19b));
}

void MyNativeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
{
    dependent.push_back(AZ_CRC("ServiceE", 0x87e65438));
}

void MyNativeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC("ServiceF", 0x1eef0582));
}

void MyNativeComponent::Reflect(AZ::ReflectContext* reflection)
{
    // Reflect my related structure
    MyNativeStruct::Reflect(reflection);

    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
    if (serializeContext)
    {
        serializeContext->Class<MyNativeComponent>()->
            Version(1)->
            Field("m_mySerializedData", &MyNativeComponent::m_mySerializedData);
    }
}

void MyNativeComponent::Activate()
{
    MyNativeBus::Handler::BusConnect();
}

void MyNativeComponent::Deactivate()
{
    MyNativeBus::Handler::BusDisconnect();
}

void MyNativeComponent::OnPrintSerializeContextReflection(AZ::SerializeContext* context, void* classPtr, const AZ::Uuid& classUuid)
{
    if (context)
    {
        context->EnumerateInstance(classPtr, classUuid,
            [&](void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement) -> bool
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
            []() -> bool { return true; },
            AZ::SerializeContext::ENUM_ACCESS_FOR_READ,
            nullptr,
            nullptr,
            nullptr);
    }
}
