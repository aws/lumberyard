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

#pragma once

#include <AzCore/Component/Component.h>

/**
 * An interface used for a service we will provide.
 */
class MyNativeEBusInterface
    : public AZ::EBusTraits
{
public:
    /// Message that will print the reflection of a generic (reflected) class into std out
    virtual void OnPrintSerializeContextReflection(AZ::SerializeContext* context, void* classPtr, const AZ::Uuid& classUuid) = 0;
};

typedef AZ::EBus<MyNativeEBusInterface> MyNativeBus;

struct MyNativeStruct
{
    AZ_TYPE_INFO(MyNativeStruct, "{820dd672-2ae8-4d97-9b94-70ee74d7df14}");


    MyNativeStruct()
        : m_data1(2)
        , m_data2(0)
    {}

    static void Reflect(AZ::ReflectContext* reflection);

    bool IsShowData2()
    {
        return m_data1 == 4 ? true : false;
    }

    int GetData1Option(int option)
    {
        if (option == 0)
        {
            return 2;
        }
        else
        {
            return 4;
        }
    }

    int m_data1;
    float m_data2;
};


class MyNativeComponent
    : public AZ::Component
    , public MyNativeBus::Handler
{
public:
    AZ_COMPONENT(MyNativeComponent, "{c0ec4125-3ffc-4a36-abbb-6f481fef2acc}");

    // AZ_COMPONENT implementation
    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
    static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
    static void Reflect(AZ::ReflectContext* reflection);

    // MyNativeBus
    void OnPrintSerializeContextReflection(AZ::SerializeContext* context, void* classPtr, const AZ::Uuid& classUuid) override;

    // Component
    void Activate() override;
    void Deactivate() override;

    MyNativeStruct m_mySerializedData;
    int m_myRuntimeData;
};

