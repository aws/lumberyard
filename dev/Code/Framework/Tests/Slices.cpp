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

#include <Tests/TestTypes.h>

#include <AzCore/base.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include "EntityTestbed.h"

#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/FileIO.h>

namespace UnitTest
{
    using namespace AZ;

    class SliceInteractiveWorkflowTest
        : public EntityTestbed
        , AZ::Data::AssetBus::MultiHandler
    {
    public:

        class TestComponent1
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(TestComponent1, "{54BA51C3-41BD-4BB6-B1ED-7F6CEFAC2F9F}");

            void Init() override
            {
            }
            void Activate() override {}
            void Deactivate() override {}

            static void Reflect(ReflectContext* context)
            {
                auto* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<TestComponent1, AZ::Component>()
                        ->Version(1)
                        ->Field("SomeFlag", &TestComponent1::m_someFlag)
                    ;

                    AZ::EditContext* ec = serialize->GetEditContext();
                    if (ec)
                    {
                        ec->Class<TestComponent1>("Another component", "A component.")
                            ->DataElement("CheckBox", &TestComponent1::m_someFlag, "SomeFlag", "");
                    }
                }
            }

            bool m_someFlag = false;
        };

        class TestComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(TestComponent, "{F146074C-152E-483C-AD33-6D1945B4261A}");

            void Init() override
            {
                m_rootElement = aznew Entity("Blah");
                m_rootElement->CreateComponent<TestComponent1>();
            }
            void Activate() override {}
            void Deactivate() override {}

            static void Reflect(ReflectContext* context)
            {
                auto* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<TestComponent, AZ::Component>()
                        ->Version(1)
                        ->Field("RootElement", &TestComponent::m_rootElement)
                        ->Field("LastElement", &TestComponent::m_lastElementId)
                        ->Field("DrawOrder", &TestComponent::m_drawOrder)
                        ->Field("IsPixelAligned", &TestComponent::m_isPixelAligned)
                    ;

                    AZ::EditContext* ec = serialize->GetEditContext();
                    if (ec)
                    {
                        ec->Class<TestComponent>("Ui Canvas", "A component.")
                            ->DataElement("CheckBox", &TestComponent::m_isPixelAligned, "IsPixelAligned", "Is pixel aligned.");
                    }
                }
            }

            AZ::Entity* m_rootElement = nullptr;
            unsigned int m_lastElementId = 0;
            int m_drawOrder = 0;
            bool m_isPixelAligned = false;
        };

        AZ::Data::AssetId m_instantiatingSliceAsset;
        AZStd::atomic_int m_stressLoadPending;
        AZStd::vector<AZ::Data::Asset<AZ::SliceAsset> > m_stressTestSliceAssets;

        enum
        {
            Stress_Descendents = 3,
            Stress_Generations = 5,
        };

        SliceInteractiveWorkflowTest()
        {
        }

        ~SliceInteractiveWorkflowTest() override
        {
            EntityTestbed::Destroy();
        }

        void OnSetup() override
        {
            auto* catalogBus = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
            if (catalogBus)
            {
                // Register asset types the asset DB should query our catalog for.
                catalogBus->AddAssetType(AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
                catalogBus->AddAssetType(AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid());

                // Build the catalog (scan).
                catalogBus->AddExtension(".xml");
                catalogBus->AddExtension(".lua");
            }
        }

        void OnReflect(AZ::SerializeContext& context, AZ::Entity& systemEntity) override
        {
            (void)context;
            (void)systemEntity;

            TestComponent::Reflect(&context);
            TestComponent1::Reflect(&context);
        }

        void OnAddButtons(QHBoxLayout& layout) override
        {
            QPushButton* sliceSelected = new QPushButton(QString("New Slice"));
            QPushButton* sliceInherit = new QPushButton(QString("Inherit Slice"));
            QPushButton* sliceInstance = new QPushButton(QString("Instantiate Slice"));
            QPushButton* saveRoot = new QPushButton(QString("Save Root"));
            QPushButton* stressGen = new QPushButton(QString("Stress Gen"));
            QPushButton* stressLoad = new QPushButton(QString("Stress Load"));
            QPushButton* stressInst = new QPushButton(QString("Stress Inst"));
            QPushButton* stressAll = new QPushButton(QString("Stress All"));
            stressInst->setEnabled(false);
            layout.addWidget(sliceSelected);
            layout.addWidget(sliceInherit);
            layout.addWidget(sliceInstance);
            layout.addWidget(saveRoot);
            layout.addWidget(stressGen);
            layout.addWidget(stressLoad);
            layout.addWidget(stressInst);
            layout.addWidget(stressAll);

            m_qtApplication->connect(sliceSelected, &QPushButton::pressed, [ this ]() { this->CreateSlice(false); });
            m_qtApplication->connect(sliceInherit, &QPushButton::pressed, [this](){this->CreateSlice(true); });
            m_qtApplication->connect(sliceInstance, &QPushButton::pressed, [ this ]() { this->InstantiateSlice(); });
            m_qtApplication->connect(saveRoot, &QPushButton::pressed, [ this ]() { this->SaveRoot(); });
            m_qtApplication->connect(stressGen, &QPushButton::pressed, [ this ]() { this->StressGen(); });
            m_qtApplication->connect(stressLoad, &QPushButton::pressed, [ this, stressInst ]()
                {
                    if (this->StressLoad())
                    {
                        stressInst->setEnabled(true);
                    }
                });
            m_qtApplication->connect(stressInst, &QPushButton::pressed, [ this ]() { this->StressInst(); });
            m_qtApplication->connect(stressAll, &QPushButton::pressed,
                [ this ]()
                {
                    this->StressGen();
                    this->StressLoad();
                    this->StressInst();
                }
                );
        }

        void OnEntityAdded(AZ::Entity& entity) override
        {
            (void)entity;

            entity.CreateComponent<TestComponent>();
        }

        void StressGenDrill(const AZ::Data::Asset<AZ::SliceAsset>& parent, size_t& nextSliceIndex, size_t generation, size_t& slicesCreated)
        {
            AZ::Data::Asset<AZ::SliceAsset> descendents[Stress_Descendents];

            for (size_t i = 0; i < Stress_Descendents; ++i)
            {
                AZ::Entity* entity = new AZ::Entity;
                auto* slice = entity->CreateComponent<AZ::SliceComponent>();
                {
                    slice->AddSlice(parent);
                    AZ::SliceComponent::EntityList entities;
                    slice->GetEntities(entities);

                    entities[0]->SetName(AZStd::string::format("Gen%u_Descendent%u_%u", generation, i, nextSliceIndex));
                    entities[1]->SetName(AZStd::string::format("Gen%u_Descendent%u_%u", generation, i, nextSliceIndex + 1));
                    //entities[0]->FindComponent<TestComponent>()->m_floatValue = float(nextSliceIndex) + 0.1f;
                    //entities[0]->FindComponent<TestComponent>()->m_intValue = int(generation);
                    //entities[1]->FindComponent<TestComponent>()->m_floatValue = float(nextSliceIndex) + 0.2f;
                }

                char assetFile[512];
                azsnprintf(assetFile, AZ_ARRAY_SIZE(assetFile), "GeneratedSlices/Gen%zu_Descendent%zu_%zu.xml", generation, i, nextSliceIndex++);

                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, assetFile, azrtti_typeid<AZ::SliceAsset>(), true);

                AZ::Utils::SaveObjectToFile(assetFile, AZ::DataStream::ST_XML, entity);

                slicesCreated += 1;

                descendents[i].Create(assetId, false);
                descendents[i].Get()->SetData(entity, slice, false);
            }

            // Drill down on next generation of inheritence.
            if (generation + 1 < Stress_Generations)
            {
                for (size_t i = 0; i < Stress_Descendents; ++i)
                {
                    StressGenDrill(descendents[i], nextSliceIndex, generation + 1, slicesCreated);
                }
            }
        }

        void StressGen()
        {
            ResetRoot();

            // Build a base slice containing two entities.
            AZ::Entity* e1 = new AZ::Entity();
            e1->SetName("Gen0_Left");
            //auto* c1 = e1->CreateComponent<TestComponent>();
            //c1->m_floatValue = 0.1f;

            AZ::Entity* e2 = new AZ::Entity();
            e2->SetName("Gen0_Right");
            //auto* c2 = e2->CreateComponent<TestComponent>();
            //c2->m_floatValue = 0.2f;

            AZ::Entity* root = new AZ::Entity();
            auto* slice = root->CreateComponent<AZ::SliceComponent>();
            slice->AddEntity(e1);
            slice->AddEntity(e2);

            AZ::Utils::SaveObjectToFile("GeneratedSlices/Gen0.xml", AZ::DataStream::ST_XML, root);

            // Build a deep binary tree, where we create two branches of each slice, each with a different
            // override from the parent.

            AZ::Data::AssetId assetId;
            EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, "GeneratedSlices/Gen0.xml", azrtti_typeid<AZ::SliceAsset>(), true);

            AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
            baseSliceAsset.Create(assetId, false);
            baseSliceAsset.Get()->SetData(root, slice);

            // Generate tree to Stress_Generations # of generations.
            size_t nextSliceIndex = 1;
            size_t slicesCreated = 1;
            (void)nextSliceIndex;
            (void)slicesCreated;
            StressGenDrill(baseSliceAsset, nextSliceIndex, 1, slicesCreated);

            AZ_TracePrintf("Debug", "Done generating %u assets\n", slicesCreated);
        }

        void StressLoadDrill(size_t& nextSliceIndex, size_t generation, AZStd::atomic_int& pending, size_t& assetsLoaded)
        {
            for (size_t i = 0; i < Stress_Descendents; ++i)
            {
                char assetFile[512];
                azsnprintf(assetFile, AZ_ARRAY_SIZE(assetFile), "GeneratedSlices/Gen%zu_Descendent%zu_%zu.xml", generation, i, nextSliceIndex++);

                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, assetFile, azrtti_typeid<AZ::SliceAsset>(), true);

                if (assetId.IsValid())
                {
                    ++pending;
                    AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);

                    AZ::Data::Asset<AZ::SliceAsset> asset;
                    if (!asset.Create(assetId, true))
                    {
                        AZ_Error("Debug", false, "Asset %s could not be created.", assetFile);
                        --pending;
                    }

                    ++assetsLoaded;
                }
                else
                {
                    AZ_Error("Debug", false, "Asset %s could not be found.", assetFile);
                }
            }

            if (generation + 1 < Stress_Generations)
            {
                for (size_t i = 0; i < Stress_Descendents; ++i)
                {
                    StressLoadDrill(nextSliceIndex, generation + 1, pending, assetsLoaded);
                }
            }
        }

        void StressInstDrill(const AZ::Data::Asset<AZ::SliceAsset>& asset, size_t& nextSliceIndex, size_t generation, size_t& slicesInstantiated)
        {
            // Recurse...
            if (generation < Stress_Generations)
            {
                for (size_t i = 0; i < Stress_Descendents; ++i)
                {
                    char assetFile[512];
                    azsnprintf(assetFile, AZ_ARRAY_SIZE(assetFile), "GeneratedSlices/Gen%zu_Descendent%zu_%zu.xml", generation, i, nextSliceIndex++);

                    AZ_Error("Debug", asset.IsReady(), "Asset %s not ready?", assetFile);

                    StressInstDrill(asset, nextSliceIndex, generation + 1, slicesInstantiated);
                }
            }

            if (asset.IsReady())
            {
                EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, InstantiateEditorSlice, asset, AZ::Transform::CreateIdentity());
                ++slicesInstantiated;
            }
        }

        bool StressLoad()
        {
            m_instantiatingSliceAsset.SetInvalid();
            m_stressTestSliceAssets.clear();
            m_stressLoadPending = 0;

            ResetRoot();

            // Preload all slice assets.
            AZ::Data::AssetId rootAssetId;
            EBUS_EVENT_RESULT(rootAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, "GeneratedSlices/Gen0.xml", azrtti_typeid<AZ::SliceAsset>(), true);
            if (rootAssetId.IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(rootAssetId);

                ++m_stressLoadPending;

                AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
                if (!baseSliceAsset.Create(rootAssetId, true))
                {
                    return false;
                }

                const AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();

                size_t nextIndex = 1;
                size_t assetsLoaded = 1;
                StressLoadDrill(nextIndex, 1, m_stressLoadPending, assetsLoaded);

                while (m_stressLoadPending > 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                    EBUS_EVENT(AZ::TickBus, OnTick, 0.3f, AZ::ScriptTimePoint());
                }

                const AZStd::chrono::system_clock::time_point assetLoadFinishTime = AZStd::chrono::system_clock::now();

                AZ_Printf("StressTest", "All Assets Loaded: %u assets, took %.2f ms\n", assetsLoaded,
                    float(AZStd::chrono::microseconds(assetLoadFinishTime - startTime).count()) * 0.001f);

                return true;
            }

            return false;
        }

        bool StressInst()
        {
            ResetRoot();

            // Instantiate from the bottom generation up.
            {
                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, "GeneratedSlices/Gen0.xml", azrtti_typeid<AZ::SliceAsset>(), true);

                AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
                baseSliceAsset.Create(assetId, false);

                if (baseSliceAsset.IsReady())
                {
                    size_t nextIndex = 1;
                    size_t slices = 0;
                    size_t liveAllocs = 0;
                    size_t totalAllocs = 0;

                    auto cb = [&liveAllocs](void*, const AZ::Debug::AllocationInfo&, unsigned char)
                        {
                            ++liveAllocs;
                            return true;
                        };

                    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords()->EnumerateAllocations(cb);
                    totalAllocs = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords()->RequestedAllocs();
                    AZ_TracePrintf("StressTest", "Allocs Before Inst: %u live, %u total\n", liveAllocs, totalAllocs);

                    const AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
                    StressInstDrill(baseSliceAsset, nextIndex, 1, slices);
                    const AZStd::chrono::system_clock::time_point instantiateFinishTime = AZStd::chrono::system_clock::now();

                    liveAllocs = 0;
                    totalAllocs = 0;
                    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords()->EnumerateAllocations(cb);
                    totalAllocs = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords()->RequestedAllocs();
                    AZ_TracePrintf("StressTest", "Allocs AfterInst: %u live, %u total\n", liveAllocs, totalAllocs);
                    // 1023 slices, 2046 entities
                    // Before         -> After          = Delta
                    // (Live)|(Total) -> (Live)|(Total) = (Live)|(Total)
                    // 28626 | 171792 -> 53157 | 533638 = 24531 | 361846
                    // 38842 | 533654 -> 53157 | 716707 = 14315 | 183053
                    // 38842 | 716723 -> 53157 | 899776 = 14315 | 183053
                    AZ::SliceComponent* rootSlice;
                    EBUS_EVENT_RESULT(rootSlice, AzToolsFramework::EditorEntityContextRequestBus, GetEditorRootSlice);
                    AZ::SliceComponent::EntityList entities;
                    entities.reserve(128);
                    rootSlice->GetEntities(entities);

                    AZ_Printf("StressTest", "All Assets Instantiated: %u slices, %u entities, took %.2f ms\n", slices, entities.size(),
                        float(AZStd::chrono::microseconds(instantiateFinishTime - startTime).count()) * 0.001f);

                    return true;
                }
            }

            return false;
        }

        void CreateSlice(bool inherit)
        {
            (void)inherit;

            static AZ::u32 sliceCounter = 1;

            AzToolsFramework::EntityIdList selected;
            EBUS_EVENT_RESULT(selected, AzToolsFramework::ToolsApplicationRequests::Bus, GetSelectedEntities);

            AZ::SliceComponent* rootSlice = nullptr;
            EBUS_EVENT_RESULT(rootSlice, AzToolsFramework::EditorEntityContextRequestBus, GetEditorRootSlice);
            AZ_Assert(rootSlice, "Failed to get root slice.");

            if (!selected.empty())
            {
                AZ::Entity newEntity(AZStd::string::format("Slice%u", sliceCounter).c_str());
                AZ::SliceComponent* newSlice = newEntity.CreateComponent<AZ::SliceComponent>();

                AZStd::vector<AZ::Entity*> reclaimFromSlice;
                AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;

                // Add all selected entities.
                for (AZ::EntityId id : selected)
                {
                    AZ::Entity* entity = nullptr;
                    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, id);
                    if (entity)
                    {
                        AZ::SliceComponent::SliceInstanceAddress sliceAddress = rootSlice->FindSlice(entity);
                        if (sliceAddress.first)
                        {
                            // This entity already belongs to a slice instance, so inherit that instance (the whole thing for now).
                            if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
                            {
                                sliceInstances.push_back(sliceAddress);
                            }
                        }
                        else
                        {
                            // Otherwise add loose.
                            newSlice->AddEntity(entity);
                            reclaimFromSlice.push_back(entity);
                        }
                    }
                }

                for (AZ::SliceComponent::SliceInstanceAddress& info : sliceInstances)
                {
                    info = newSlice->AddSliceInstance(info.first, info.second);
                }

                const QString saveAs = QFileDialog::getSaveFileName(nullptr,
                        QString("Save As..."), QString("."), QString("Xml Files (*.xml)"));
                if (!saveAs.isEmpty())
                {
                    AZ::Utils::SaveObjectToFile(saveAs.toUtf8().constData(), AZ::DataStream::ST_XML, &newEntity);
                }

                // Reclaim entities.
                for (AZ::Entity* entity : reclaimFromSlice)
                {
                    newSlice->RemoveEntity(entity, false);
                }

                // Reclaim slices.
                for (const AZ::SliceComponent::SliceInstanceAddress& info : sliceInstances)
                {
                    rootSlice->AddSliceInstance(info.first, info.second);
                }

                ++sliceCounter;
            }
        }

        void InstantiateSlice()
        {
            const QString loadFrom = QFileDialog::getOpenFileName(nullptr,
                    QString("Select Slice..."), QString("."), QString("Xml Files (*.xml)"));

            if (!loadFrom.isEmpty())
            {
                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, loadFrom.toUtf8().constData(), azrtti_typeid<AZ::SliceAsset>(), true);

                AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
                baseSliceAsset.Create(assetId, true);
                m_instantiatingSliceAsset = baseSliceAsset.GetId();

                AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
            }
        }
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            if (asset.GetId() == m_instantiatingSliceAsset)
            {
            }
            else
            {
                --m_stressLoadPending;
            }
        }

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            if (asset.GetId() == m_instantiatingSliceAsset)
            {
                if (asset.Get() == nullptr)
                {
                    return;
                }

                m_instantiatingSliceAsset.SetInvalid();

                // Just add the slice to the level slice.
                AZ::Data::Asset<AZ::SliceAsset> sliceAsset = asset;
                EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, InstantiateEditorSlice, sliceAsset, AZ::Transform::CreateIdentity());

                // Init everything in the slice.
                AZ::SliceComponent* rootSlice = nullptr;
                EBUS_EVENT_RESULT(rootSlice, AzToolsFramework::EditorEntityContextRequestBus, GetEditorRootSlice);
                AZ_Assert(rootSlice, "Failed to get root slice.");
                AZ::SliceComponent::EntityList entities;
                rootSlice->GetEntities(entities);
                for (AZ::Entity* entity : entities)
                {
                    if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                    {
                        entity->Init();
                    }
                }

                m_entityCounter += AZ::u32(entities.size());
            }
            else
            {
                m_stressTestSliceAssets.push_back(asset);
                --m_stressLoadPending;
            }
        }

        void run()
        {
            int argc = 0;
            char* argv = nullptr;
            Run(argc, &argv);
        }
    }; // class SliceInteractiveWorkflowTest

    TEST_F(SliceInteractiveWorkflowTest, DISABLED_Test)
    {
        run();
    }

    class MinimalEntityWorkflowTester
        : public EntityTestbed
    {
    public:
        void run()
        {
            int argc = 0;
            char* argv = nullptr;
            Run(argc, &argv);
        }

        void OnEntityAdded(AZ::Entity& entity) override
        {
            // Add your components.
            entity.CreateComponent<AzToolsFramework::Components::TransformComponent>();
        }
    };

    TEST_F(MinimalEntityWorkflowTester, DISABLED_Test)
    {
        run();
    }
} // namespace UnitTest

