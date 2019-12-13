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

#include <AzTest/AzTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>

namespace AzToolsFramework
{
    // Test components used to test component filters
    class EntitySearch_TestComponent1
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(EntitySearch_TestComponent1, "{D8ABC8F6-E43B-4ED9-AABE-BA8905D4099D}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EntitySearch_TestComponent1, AZ::Component>()
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EntitySearch_TestComponent1>("SearchTestComponent1", "Component 1 for Entity Search Unit Tests")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::Category, "Entity Search Test Components")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.amazongames.com/")
                    ;
                }
            }
        }

        virtual ~EntitySearch_TestComponent1() override
        {}

    private:
        void Init() override
        {}

        void Activate() override
        {}

        void Deactivate() override
        {}
    };

    class EntitySearch_TestComponent2
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(EntitySearch_TestComponent2, "{E50A848D-64C3-4445-A21B-D8F9C96972FE}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EntitySearch_TestComponent2, AZ::Component>()
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EntitySearch_TestComponent2>("SearchTestComponent2", "Component 2 for Entity Search Unit Tests")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::Category, "Entity Search Test Components")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.amazongames.com/")
                    ;
                }
            }
        }

        virtual ~EntitySearch_TestComponent2() override
        {}

    private:
        void Init() override
        {}

        void Activate() override
        {}

        void Deactivate() override
        {}
    };

    class EditorEntitySearchComponentTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_app.Start(m_descriptor);
            
            RegisterComponents();
            GenerateTestHierarchy();
        }

        void RegisterComponents()
        {
            // Register our test components (This process also reflects them to the appropriate contexts)
            auto* EntitySearch_TestComponent1Descriptor = EntitySearch_TestComponent1::CreateDescriptor();
            auto* EntitySearch_TestComponent2Descriptor = EntitySearch_TestComponent2::CreateDescriptor();

            m_app.RegisterComponentDescriptor(EntitySearch_TestComponent1Descriptor);
            m_app.RegisterComponentDescriptor(EntitySearch_TestComponent2Descriptor);

            m_testComponentType1 = azrtti_typeid<EntitySearch_TestComponent1>();
            m_testComponentType2 = azrtti_typeid<EntitySearch_TestComponent2>();
        }

        void GenerateTestHierarchy() 
        {
            /*
            *   City
            *   |_  Street              (Test Component 2)
            *       |_  Car
            *       |   |_ Passenger    (Test Component 1, Test Component 2)
            *       |   |_ Passenger
            *       |_  Car             (Test Component 1)
            *       |   |_ Passenger
            *       |_  SportsCar
            *           |_ Passenger    (Test Component 2)
            *           |_ Passenger
            */

            m_entityMap["cityId"] =         CreateEditorEntity("City",      AZ::EntityId());
            m_entityMap["streetId"] =       CreateEditorEntity("Street",    m_entityMap["cityId"],      false,  true);
            m_entityMap["carId1"] =         CreateEditorEntity("Car",       m_entityMap["streetId"]);
            m_entityMap["passengerId1"] =   CreateEditorEntity("Passenger", m_entityMap["carId1"],      true,   true);
            m_entityMap["passengerId2"] =   CreateEditorEntity("Passenger", m_entityMap["carId1"]);
            m_entityMap["carId2"] =         CreateEditorEntity("Car",       m_entityMap["streetId"],    true);
            m_entityMap["passengerId3"] =   CreateEditorEntity("Passenger", m_entityMap["carId2"]);
            m_entityMap["sportsCarId"] =    CreateEditorEntity("SportsCar", m_entityMap["streetId"]);
            m_entityMap["passengerId4"] =   CreateEditorEntity("Passenger", m_entityMap["sportsCarId"], false,  true);
            m_entityMap["passengerId5"] =   CreateEditorEntity("Passenger", m_entityMap["sportsCarId"]);

            // Add some Components
        }

        AZ::EntityId CreateEditorEntity(const char* name, AZ::EntityId parentId, bool addTestComponent1 = false, bool addTestComponent2 = false)
        {
            AZ::Entity* entity = nullptr;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(entity, &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateEditorEntity, name);

            entity->Deactivate();

            // add required components for the Editor entity
            entity->CreateComponent<Components::TransformComponent>();
            entity->CreateComponent<Components::EditorLockComponent>();
            entity->CreateComponent<Components::EditorVisibilityComponent>();

            if (addTestComponent1)
            {
                entity->CreateComponent<EntitySearch_TestComponent1>();
            }

            if (addTestComponent2)
            {
                entity->CreateComponent<EntitySearch_TestComponent2>();
            }

            entity->Activate();

            // Parent
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, parentId);

            return entity->GetId();
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;
        AZStd::unordered_map<AZStd::string, AZ::EntityId> m_entityMap;

        AZ::Uuid m_testComponentType1;
        AZ::Uuid m_testComponentType2;
    };

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_RootEntities)
    {
        {
            EntityIdList rootEntities;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(rootEntities, &AzToolsFramework::EditorEntitySearchRequests::GetRootEditorEntities);

            EXPECT_EQ(rootEntities.size(), 1);
            EXPECT_EQ(rootEntities[0], m_entityMap["cityId"]);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByName_Base)
    {
        {
            // No filters - return all entities

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, EntitySearchFilter());

            EXPECT_EQ(searchResults.size(), 10);
        }

        {
            // Filter by name - single entity

            EntitySearchFilter filter;
            filter.m_names.push_back("Street");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            // Filter by name - multiple entities

            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 5);
        }

        {
            // Filter by name - multiple names

            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");
            filter.m_names.push_back("Street");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 6);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByName_Wildcard)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Str*et");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }
        
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("St*t");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Str?et");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Str?t");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("C*");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("*");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 10);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByName_CaseSensitive)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByPath_Base)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|SportsCar");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["sportsCarId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|Car|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByPath_Wildcard)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|*|SportsCar");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["sportsCarId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|*|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 5);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|*Car|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 5);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|Sport*|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByPath_CaseSensitive)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("city|street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("city|street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByComponent_Base)
    {
        {
            EntitySearchFilter filter;
            filter.m_componentTypeIds.push_back(m_testComponentType1);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }

        {
            EntitySearchFilter filter;
            filter.m_componentTypeIds.push_back(m_testComponentType2);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByComponent_Multiple)
    {
        {
            EntitySearchFilter filter;
            filter.m_componentTypeIds.push_back(m_testComponentType1);
            filter.m_componentTypeIds.push_back(m_testComponentType2);
            filter.m_mustMatchAllComponents = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 4);
        }

        {
            EntitySearchFilter filter;
            filter.m_componentTypeIds.push_back(m_testComponentType1);
            filter.m_componentTypeIds.push_back(m_testComponentType2);
            filter.m_mustMatchAllComponents = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["passengerId1"]);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_Search_Roots_Base)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");
            filter.m_roots.push_back(m_entityMap["carId2"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["passengerId3"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("SportsCar");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|SportsCar|Passenger");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_Search_Roots_NamesAreRootBased)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            // No root - Default
            filter.m_namesAreRootBased = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            // No root - Default
            filter.m_namesAreRootBased = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["streetId"]);
            filter.m_namesAreRootBased = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["streetId"]);
            filter.m_namesAreRootBased = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["carId2"]);
            filter.m_namesAreRootBased = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_Search_MultipleFilters)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car");
            filter.m_componentTypeIds.push_back(m_testComponentType1);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["carId2"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Pass*");
            filter.m_roots.push_back(m_entityMap["sportsCarId"]);
            filter.m_componentTypeIds.push_back(m_testComponentType2);
            filter.m_namesAreRootBased = true;
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["passengerId4"]);
        }
    }
}