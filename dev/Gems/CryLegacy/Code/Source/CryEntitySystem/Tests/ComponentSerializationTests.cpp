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
#include <AzTest/AzTest.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/unordered_map.h>

#include "Components/ComponentSerialization.h"
#include "ISerialize.h"

#pragma warning(disable: 4800)      // 'int' : forcing value to bool 'true' or 'false' (performance warning)


namespace ComponetSerializationTests
{
    class ITestEntity
    {
    public:
        virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading) = 0;
        virtual void Serialize(TSerialize serializer, int nFlags = 0) = 0;
    };
    class TestEntity
        : public ITestEntity
    {
    public:
        virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading)
        {
            auto serializationComponent = GetComponent<ComponentSerialization>();
            if (serializationComponent)
            {
                serializationComponent->SerializeXML(entityNode, bLoading);
            }
        }

        virtual void Serialize(TSerialize serializer, int nFlags = 0)
        {
            auto serializationComponent = GetComponent<ComponentSerialization>();
            if (serializationComponent)
            {
                serializationComponent->Serialize(serializer);
            }
        }

        virtual bool NeedSerialize()
        {
            auto serializationComponent = GetComponent<ComponentSerialization>();
            if (serializationComponent)
            {
                if (serializationComponent->NeedSerialize())
                {
                    return true;
                }
            }

            return false;
        }

        void RegisterComponent(const ComponentType& type, AZStd::shared_ptr<IComponent> component)
        {
            if (m_components.find(type) == m_components.end())
            {
                m_components[type] = component;
            }
        }

        template <typename T>
        T* GetComponent()
        {
            return static_cast<T*>(m_components[T::Type()].get());
        }

    protected:

        AZStd::unordered_map<ComponentType, AZStd::shared_ptr<IComponent> > m_components;
    };

    class ITestComponent
        : public IComponent
    {
    public:

        ITestComponent(TestEntity& entity)
            : m_entity(entity)
        {}

        virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading) = 0;
        virtual void Serialize(TSerialize serializer) = 0;
        virtual bool NeedSerialize() = 0;
        virtual bool GetSignature(TSerialize signature) = 0;

        TestEntity& GetEntity()
        {
            return m_entity;
        }

    protected:

        TestEntity& m_entity;
    };

    namespace SerializationOrder
    {
        static const int TestComponentA = 0;
        static const int TestComponentC = 1;
        static const int TestComponentB = 2;

        // Must correspond to order specified above.
        static const char* ComponentNames[] = {
            "TestComponentA",
            "TestComponentC",
            "TestComponentB"
        };
    }

    class TestComponentA
        : public ITestComponent
    {
    public:
        DECLARE_COMPONENT_TYPE("TestComponentA", 0xB25D04D4BC6B4EB7, 0x9843E405153DFA6C);

        TestComponentA(TestEntity& entity)
            : ITestComponent(entity)
        {
            auto serializationComponent = m_entity.GetComponent<ComponentSerialization>();
            if (serializationComponent)
            {
                serializationComponent->Register<TestComponentA>(SerializationOrder::TestComponentA, *this, nullptr, &TestComponentA::SerializeXML, &TestComponentA::NeedSerialize, &TestComponentA::GetSignature);
            }
        }
        void SerializeXML(XmlNodeRef& entityNode, bool bLoading) override
        {
            if (!bLoading)
            {
                auto node = entityNode->createNode(Type().Name());
                entityNode->addChild(node);
            }
            else
            {
                auto node = entityNode->findChild(Type().Name());
                EXPECT_TRUE(node != nullptr);
            }
        }

        bool NeedSerialize() override
        {
            return true;
        }

        bool GetSignature(TSerialize signature) override
        {
            return false;
        }

        void Serialize(TSerialize serializer) override
        {
            // This function was deliberately not registered and will not be called.
        }
    };

    class TestComponentB
        : public ITestComponent
    {
    public:
        DECLARE_COMPONENT_TYPE("TestComponentB", 0xB25D04D4BC6B4EB7, 0x9843E405153DFA6D);

        TestComponentB(TestEntity& entity)
            : ITestComponent(entity)
        {
            auto serializationComponent = m_entity.GetComponent<ComponentSerialization>();
            if (serializationComponent)
            {
                serializationComponent->Register<TestComponentB>(SerializationOrder::TestComponentB, *this, nullptr, &TestComponentB::SerializeXML, &TestComponentB::NeedSerialize, &TestComponentB::GetSignature);
            }
        }
        void SerializeXML(XmlNodeRef& entityNode, bool bLoading) override
        {
            if (!bLoading)
            {
                auto node = entityNode->createNode(Type().Name());
                entityNode->addChild(node);
            }
            else
            {
                auto node = entityNode->findChild(Type().Name());
                EXPECT_TRUE(node != nullptr);
            }
        }

        bool GetSignature(TSerialize signature) override
        {
            return false;
        }

        void Serialize(TSerialize serializer) override
        {
            // This function was deliberately not registered and will not be called.
        }

        bool NeedSerialize() override
        {
            return true;
        }
    };

    class TestComponentC
        : public ITestComponent
    {
    public:
        DECLARE_COMPONENT_TYPE("TestComponentC", 0xB25D04D4BC6B4EB7, 0x9843E405153DFA6E);


        TestComponentC(TestEntity& entity)
            : ITestComponent(entity)
        {
            auto serializationComponent = m_entity.GetComponent<ComponentSerialization>();
            if (serializationComponent)
            {
                serializationComponent->Register<TestComponentC>(SerializationOrder::TestComponentC, *this, nullptr, &TestComponentC::SerializeXML, &TestComponentC::NeedSerialize, &TestComponentC::GetSignature);
            }
        }
        void SerializeXML(XmlNodeRef& entityNode, bool bLoading) override
        {
            if (!bLoading)
            {
                auto node = entityNode->createNode(Type().Name());
                entityNode->addChild(node);
            }
            else
            {
                auto node = entityNode->findChild(Type().Name());
                EXPECT_TRUE(node != nullptr);
            }
        }

        bool GetSignature(TSerialize signature) override
        {
            return false;
        }

        void Serialize(TSerialize serializer) override
        {
            // This function was deliberately not registered and will not be called.
        }

        bool NeedSerialize() override
        {
            return true;
        }
    };

    INTEG_TEST(ComponentSerializationTests, Main)
    {
        TestEntity entity;

        AZStd::shared_ptr<ComponentSerialization> serializationComponent = AZStd::make_shared<ComponentSerialization>();
        entity.RegisterComponent(ComponentSerialization::Type(), serializationComponent);

        AZStd::shared_ptr<TestComponentA> testComponentA = AZStd::make_shared<TestComponentA>(entity);
        entity.RegisterComponent(TestComponentA::Type(), testComponentA);

        AZStd::shared_ptr<TestComponentB> testComponentB = AZStd::make_shared<TestComponentB>(entity);
        entity.RegisterComponent(TestComponentB::Type(), testComponentB);

        AZStd::shared_ptr<TestComponentC> testComponentC = AZStd::make_shared<TestComponentC>(entity);
        entity.RegisterComponent(TestComponentC::Type(), testComponentC);

        XmlNodeRef node = gEnv->pSystem->CreateXmlNode("Root");
        EXPECT_TRUE(node);

        entity.SerializeXML(node, false);
        entity.SerializeXML(node, true);

        // We have serialized 3 components
        EXPECT_TRUE(node->getChildCount() == 3);

        // Do any of the entity components need to be serialized?
        EXPECT_TRUE(entity.NeedSerialize());

        // Nodes must be in the same order as specified
        for (int i = 0; i < node->getChildCount(); ++i)
        {
            auto child = node->getChild(i);
            EXPECT_TRUE(strcmp(child->getTag(), SerializationOrder::ComponentNames[i]) == 0);
        }

        // Not strictly required, but may be useful to visualize the output
        bool saveToFile = false;
        if (saveToFile)
        {
            node->saveToFile(string().Format("%s/TestResults/ComponentSerialization.xml", gEnv->pCryPak->GetAlias("@cache@")));
        }

        serializationComponent->Unregister(TestComponentC::Type());

        node = gEnv->pSystem->CreateXmlNode("Root");
        EXPECT_TRUE(node);

        entity.SerializeXML(node, false);
        entity.SerializeXML(node, true);

        // One less node
        EXPECT_TRUE(node->getChildCount() == 2);

        // Nodes must have retained order as specified

        // TestComponentA
        auto child = node->getChild(0);
        EXPECT_TRUE(strcmp(child->getTag(), SerializationOrder::ComponentNames[0]) == 0);

        // TestComponentB
        child = node->getChild(1);
        EXPECT_TRUE(strcmp(child->getTag(), SerializationOrder::ComponentNames[2]) == 0);

        if (saveToFile)
        {
            node->saveToFile(string().Format("%s/TestResults/ComponentSerialization2.xml", gEnv->pCryPak->GetAlias("@cache@")));
        }
    }
}
