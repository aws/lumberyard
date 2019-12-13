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

#include "Native.h"
#include "Components.h"

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/OSAllocator.h>

class CodegenTest
{
public:
    // Running both versions of the implemented code
    void run()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();


        // Native implementation test
        {
            // reflection (called automatically by the ComponentApplication)
            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            AZ::Entity::Reflect(&serializeContext);
            MyNativeComponent::Reflect(&serializeContext);

            //
            AZ::Entity myEntity;
            MyNativeComponent* component = aznew MyNativeComponent();
            myEntity.AddComponent(component);
            myEntity.Init();
            myEntity.Activate(); // now component is started and ready to listen for a message

            MyNativeBus::Broadcast(&MyNativeBus::Events::OnPrintSerializeContextReflection, &serializeContext, &myEntity, AZ::AzTypeInfo<AZ::Entity>::Uuid());
            myEntity.Deactivate();

            myEntity.RemoveComponent(component);

            delete component;
        }


        // Code-gen implementation test
        {
            // reflection (called automatically by the ComponentApplication)
            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            AZ::Entity::Reflect(&serializeContext);
            MyCodegenComponent::Reflect(&serializeContext);

            //
            AZ::Entity myEntity;
            MyCodegenComponent* component = aznew MyCodegenComponent();
            myEntity.AddComponent(component);
            myEntity.Init();
            myEntity.Activate(); // now component is started and ready to listen for a message


            //MyCodegenBus::Broadcast(&MyCodegenBus::Events::OnPrintSerializeContextReflection, &serializeContext, &myEntity, AZ::AzTypeInfo<AZ::Entity>::Uuid());

            //MyCodegenEBusInterface::Bus::Broadcast::OnPrintSerializeContextReflection(&serializeContext, &myEntity, AZ::AzTypeInfo<AZ::Entity>::Uuid());

            myEntity.Deactivate();

            myEntity.RemoveComponent(component);

            delete component;
        }

        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
};

AZ_TEST_SUITE(CodeGenSuite)
AZ_TEST(CodegenTest)
AZ_TEST_SUITE_END

int main(void)
{
    // Does even more nothing than the other one!

    return 0;
}
