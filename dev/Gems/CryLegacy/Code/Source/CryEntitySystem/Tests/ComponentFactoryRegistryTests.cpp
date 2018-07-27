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

#include "CryLegacy_precompiled.h"

////////////////////////////////////////////////////////////////////////////////
// Unit Testing
////////////////////////////////////////////////////////////////////////////////
#include <AzTest/AzTest.h>
#include "ComponentFactoryRegistry.h"

namespace ComponentFactoryRegistryTests
{
    class ComponentFactoryRegistryTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    class IComponentDog
        : public IComponent
    {
    public:
        DECLARE_COMPONENT_TYPE("ComponentDog", 0x495A0A6C14FB4261, 0xB2FCE87D9B14B64B)

        IComponentDog()
        {}

        virtual bool IsCComponentDog() const
        {
            return false;
        }
    };

    class CComponentDog
        : public IComponentDog
    {
    public:
        virtual bool IsCComponentDog() const
        {
            return true;
        }
    };

    class CComponentCanine
        : public IComponentDog
    {
    public:
    };

    class IComponentCat
        : public IComponent
    {
    public:
        DECLARE_COMPONENT_TYPE("ComponentCat", 0xEE23FDC22EAA4ACE, 0xB13797B09F374E76)

        IComponentCat()
        {}
    };

    class CComponentCat
        : public IComponentCat
    {
    };

    TEST_F(ComponentFactoryRegistryTests, CUT_ComponentFactoryRegistry)
    {
        CComponentFactoryRegistry registry;

        // test empty registry
        EXPECT_TRUE(!registry.GetFactory<IComponentDog>());
        EXPECT_TRUE(!registry.ReleaseFactory(IComponentDog::Type()));

        // register dog factory
        EXPECT_TRUE(registry.RegisterFactory(AZStd::make_unique<DefaultComponentFactory<CComponentDog, IComponentDog> >()));
        IComponentFactory<IComponentDog>* dogFactory = registry.GetFactory<IComponentDog>();
        EXPECT_TRUE(dogFactory != nullptr);

        // ensure it creates dogs
        AZStd::shared_ptr<IComponentDog> dog = dogFactory->CreateComponent();
        EXPECT_TRUE(dog.get() != nullptr);
        EXPECT_TRUE(dog->GetComponentType() == IComponentDog::Type());
        EXPECT_TRUE(dog->IsCComponentDog());

        // no cat factory exists
        EXPECT_TRUE(!registry.GetFactory<IComponentCat>());

        // double registration should fail
        EXPECT_TRUE(!registry.RegisterFactory(AZStd::make_unique<DefaultComponentFactory<CComponentDog, IComponentDog> >()));

        // registration of a factory with different concrete type
        // but same ComponentType should fail
        EXPECT_TRUE(!registry.RegisterFactory(AZStd::make_unique<DefaultComponentFactory<CComponentCanine, IComponentDog> >()));

        // double-check that the CComponentCanine factory failed to register
        // by ensuring the CComponentDogs are still being produced
        dog = registry.GetFactory<IComponentDog>()->CreateComponent();
        EXPECT_TRUE(dog.get() != nullptr);
        EXPECT_TRUE(dog->IsCComponentDog());

        // release dog factory
        EXPECT_TRUE(registry.ReleaseFactory(IComponentDog::Type()));

        // double unregister should fail
        EXPECT_TRUE(!registry.ReleaseFactory(IComponentDog::Type()));

        // register dog and cat factories
        EXPECT_TRUE(registry.RegisterFactory(AZStd::make_unique<DefaultComponentFactory<CComponentDog, IComponentDog> >()));
        EXPECT_TRUE(registry.RegisterFactory(AZStd::make_unique<DefaultComponentFactory<CComponentCat, IComponentCat> >()));
        dogFactory = registry.GetFactory<IComponentDog>();
        EXPECT_TRUE(dogFactory != nullptr);
        IComponentFactory<IComponentCat>* catFactory = registry.GetFactory<IComponentCat>();
        EXPECT_TRUE(catFactory != nullptr);

        // create dog and cat
        dog = dogFactory->CreateComponent();
        EXPECT_TRUE(dog.get() != nullptr);
        EXPECT_TRUE(dog->GetComponentType() == IComponentDog::Type());
        AZStd::shared_ptr<IComponentCat> cat = catFactory->CreateComponent();
        EXPECT_TRUE(cat.get() != nullptr);
        EXPECT_TRUE(cat->GetComponentType() == IComponentCat::Type());
        EXPECT_TRUE(cat->GetComponentType() != dog->GetComponentType());
    }
} // namespace ComponentFactoryRegistryTests
