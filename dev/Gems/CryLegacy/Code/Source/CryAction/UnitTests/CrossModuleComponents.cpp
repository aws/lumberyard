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

#include <Components/IComponentModuleTest.h>

#include <AzTest/AzTest.h>

#pragma warning( push )
#pragma warning( disable: 4800 )    // 'int' : forcing value to bool 'true' or 'false' (performance warning)

namespace CrossModuleComponentsTests
{
    // A component type declared in CryAction
    class CryActionComponent
        : public IComponent
        , public NoCopy // nocopy to ensure that it's not copied across modules
    {
    public:
        DECLARE_COMPONENT_TYPE("CryActionComponent", 0x79FF4364B36443AB, 0x96A9AEF3C151E0F5);
        CryActionComponent() {}
        virtual void ProcessEvent(SEntityEvent& event) override {};
    };

    // Factory for CryActionComponents
    class CryActionFactory
        : public DefaultComponentFactory<CryActionComponent, CryActionComponent>
        , public NoCopy // nocopy to ensure that it's not copied across modules
    {
    public:
        static int sConstructorRunCount;
        static int sDestructorRunCount;

        CryActionFactory()
            : DefaultComponentFactory()
        {
            sConstructorRunCount++;
        }

        ~CryActionFactory()
        {
            sDestructorRunCount++;
        }
    };
    int CryActionFactory::sConstructorRunCount = 0;
    int CryActionFactory::sDestructorRunCount = 0;

    // Node for CryActionFactory
    class CryActionFactoryCreationNode
        : public ComponentFactoryCreationNode
    {
    public:
        CryActionFactoryCreationNode()
            : ComponentFactoryCreationNode([](){ return AZStd::make_unique<CryActionFactory>(); })
        {}
    };

    // Create and destroy and component of given type,
    // using its factory, fetched via the registry
    template<class T>
    void TestComponentCreation(IComponentFactoryRegistry& registry)
    {
        IComponentFactory<T>* factory = registry.GetFactory<T>();
        EXPECT_TRUE(factory);
        AZStd::weak_ptr<T> weakPtr;
        {
            AZStd::shared_ptr<T> component = factory->CreateComponent();
            EXPECT_TRUE(component.get() != nullptr);
            EXPECT_TRUE(component->GetComponentType() == T::Type());
            weakPtr = component;
            // check that there are no dangling references in other modules
            EXPECT_TRUE(weakPtr.use_count() == 1);
        }
        // check that component was deleted when its shared_ptr went out of range
        EXPECT_TRUE(weakPtr.use_count() == 0);
    }

    // Most tests on components exist in CryEntitySystem.
    // Test that a component declared in CryAction works properly
    INTEG_TEST(CrossModuleComponentsTests, CUT_CrossModuleComponentFactory)
    {
        EXPECT_TRUE(gEnv->pEntitySystem && gEnv->pEntitySystem->GetComponentFactoryRegistry());
        IComponentFactoryRegistry& registry = *gEnv->pEntitySystem->GetComponentFactoryRegistry();

        // cache number of existing ComponentFactoryCreationNodes
        size_t prevFactoryCount = ComponentFactoryCreationNode::GetGlobalList().size();
        {
            // create node and check that it exists in global list
            CryActionFactoryCreationNode factoryNode;
            EXPECT_TRUE(ComponentFactoryCreationNode::GetGlobalList().size() == (prevFactoryCount + 1));

            // register factory
            EXPECT_TRUE(registry.RegisterFactory(factoryNode.GetCreateFactoryFunction()()));

            // try creating component
            TestComponentCreation<CryActionComponent>(registry);

            // unregister
            EXPECT_TRUE(registry.ReleaseFactory(CryActionComponent::Type()));
        }
        // check node was removed from global list
        EXPECT_TRUE(ComponentFactoryCreationNode::GetGlobalList().size() == prevFactoryCount);

        // check that only one factory was created, and that it's dead now
        EXPECT_TRUE(CryActionFactory::sConstructorRunCount == 1);
        EXPECT_TRUE(CryActionFactory::sDestructorRunCount == 1);
    }

    INTEG_TEST(CrossModuleComponentsTests, CUT_ComponentModuleTest)
    {
        EXPECT_TRUE(gEnv->pEntitySystem && gEnv->pEntitySystem->GetComponentFactoryRegistry());
        IComponentFactoryRegistry& registry = *gEnv->pEntitySystem->GetComponentFactoryRegistry();

        auto factory = registry.GetFactory<IComponentModuleTest>();
        EXPECT_TRUE(factory);

        // IComponentModuleTest has a lot of functions that access static state,
        // but the function itself isn't static. I'm going to use 'component'
        // throughout the rest of this function to access that static state.
        AZStd::shared_ptr<IComponentModuleTest> component(factory->CreateComponent());
        EXPECT_TRUE(component.get());
        {
            // Test creating component declared in another module.
            // Check that constructor and destructor run just once during test.
            const int prevConstructorRunCount = component->GetConstructorRunCount();
            const int prevDestructorRunCount = component->GetDestructorRunCount();
            TestComponentCreation<IComponentModuleTest>(registry);
            EXPECT_TRUE(component->GetConstructorRunCount() == (prevConstructorRunCount + 1));
            EXPECT_TRUE(component->GetDestructorRunCount() == (prevDestructorRunCount + 1));
        }

        //
        // Now run a bunch of tests concerning shared_ptrs, in different modules,
        // referencing the same object.
        //

        const int prevConstructorRunCount = component->GetConstructorRunCount();
        const int prevDestructorRunCount = component->GetDestructorRunCount();

        // Create a shared_ptr in the other module
        EXPECT_TRUE(component->GetStaticSharedPtrRef().use_count() == 0);
        EXPECT_TRUE(component->CreateStaticSharedPtr());
        EXPECT_TRUE(component->GetStaticSharedPtrRef().use_count() == 1);

        // Create a shared_ptr in this module
        AZStd::shared_ptr<IComponentModuleTest> crossModulePtrA = component->GetStaticSharedPtrRef();
        EXPECT_TRUE(component->GetStaticSharedPtrRef().use_count() == 2);
        EXPECT_TRUE(crossModulePtrA.use_count() == 2);

        // Create a shared_ptr using a function that returns shared_ptr by value
        AZStd::shared_ptr<IComponentModuleTest> crossModulePtrB = component->GetStaticSharedPtr();
        EXPECT_TRUE(component->GetStaticSharedPtrRef().use_count() == 3);
        EXPECT_TRUE(crossModulePtrB.use_count() == 3);
        EXPECT_TRUE(crossModulePtrA.use_count() == 3);

        // Create a weak_ptr in various ways. The use_count should stay the same.
        AZStd::weak_ptr<IComponentModuleTest> weakPtr = crossModulePtrA;
        EXPECT_TRUE(component->GetStaticSharedPtrRef().use_count() == 3);
        EXPECT_TRUE(weakPtr.use_count() == 3);
        weakPtr = component->GetStaticSharedPtrRef();
        EXPECT_TRUE(weakPtr.use_count() == 3);
        weakPtr = component->GetStaticSharedPtr();
        EXPECT_TRUE(weakPtr.use_count() == 3);

        // Clear the local shared_ptrs, the object should still exist
        // and the weak_ptr should still work
        crossModulePtrA.reset();
        crossModulePtrB.reset();
        EXPECT_TRUE(component->GetStaticSharedPtrRef().use_count() == 1);
        EXPECT_TRUE(weakPtr.use_count() == 1);
        EXPECT_TRUE(weakPtr.lock()->GetComponentType() == IComponentModuleTest::Type());

        // Clear the shared_ptr in the other module, the object should be gone
        component->GetStaticSharedPtrRef().reset();
        EXPECT_TRUE(weakPtr.use_count() == 0);

        // Ensure we really only ever made 1 more component
        EXPECT_TRUE(component->GetConstructorRunCount() == (prevConstructorRunCount + 1));
        EXPECT_TRUE(component->GetDestructorRunCount() == (prevDestructorRunCount + 1));

        // Create another component with shared_ptrs across modules.
        EXPECT_TRUE(component->CreateStaticSharedPtr());
        crossModulePtrA = component->GetStaticSharedPtrRef();
        weakPtr = component->GetStaticSharedPtrRef();
        EXPECT_TRUE(weakPtr.use_count() == 2);

        // Clear the remote shared_ptr, the object should still be around
        component->GetStaticSharedPtrRef().reset();
        EXPECT_TRUE(weakPtr.use_count() == 1);
        EXPECT_TRUE(crossModulePtrA->GetComponentType() == IComponentModuleTest::Type());

        // Clear the local shared_ptr, the object should be gone
        crossModulePtrA.reset();
        EXPECT_TRUE(weakPtr.use_count() == 0);

        // Check that only one more component was created/destroyed
        EXPECT_TRUE(component->GetConstructorRunCount() == (prevConstructorRunCount + 2));
        EXPECT_TRUE(component->GetDestructorRunCount() == (prevDestructorRunCount + 2));
    }
} // namespace CrossModuleComponentsTests

#pragma warning( pop )
