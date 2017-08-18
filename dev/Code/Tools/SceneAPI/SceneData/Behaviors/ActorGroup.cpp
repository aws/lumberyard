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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/Groups/ActorGroup.h>
#include <SceneAPI/SceneData/Behaviors/ActorGroup.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            const int ActorGroup::s_animationsPreferredTabOrder = 3;

            AZ_CLASS_ALLOCATOR_IMPL(ActorGroup, SystemAllocator, 0)

                ActorGroup::ActorGroup()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            ActorGroup::~ActorGroup()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void ActorGroup::GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene)
            {
                if (SceneHasActorGroup(scene))
                {
                    categories.emplace_back("Actors", SceneData::ActorGroup::TYPEINFO_Uuid(), s_animationsPreferredTabOrder);
                }
            }

            void ActorGroup::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(SceneData::ActorGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                SceneData::ActorGroup* group = azrtti_cast<SceneData::ActorGroup*>(&target);
                group->SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IActorGroup>(scene.GetName(), scene.GetManifest()));

                const Containers::SceneGraph& graph = scene.GetGraph();

                // Gather the nodes that should be selected by default
                DataTypes::ISceneNodeSelectionList& nodeSelectionList = group->GetSceneNodeSelectionList();
                Utilities::SceneGraphSelector::UnselectAll(graph, nodeSelectionList);
                Containers::SceneGraph::ContentStorageConstData graphContent = graph.GetContentStorage();
                auto view = Containers::Views::MakeFilterView(graphContent, Containers::DerivedTypeFilter<DataTypes::IMeshData>());
                for (auto iter = view.begin(), iterEnd = view.end(); iter != iterEnd; ++iter)
                {
                    Containers::SceneGraph::NodeIndex nodeIndex = graph.ConvertToNodeIndex(iter.GetBaseIterator());
                    nodeSelectionList.AddSelectedNode(graph.GetNodeName(nodeIndex).GetPath());
                }
                Utilities::SceneGraphSelector::UpdateNodeSelection(graph, nodeSelectionList);
            }

            Events::ProcessingResult ActorGroup::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateActorGroups(scene);
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            Events::ProcessingResult ActorGroup::BuildDefault(Containers::Scene& scene) const
            {
                if (SceneHasActorGroup(scene))
                {
                    return Events::ProcessingResult::Ignored;
                }

                // Add a default actor group to the manifest.
                AZStd::shared_ptr<SceneData::ActorGroup> group = AZStd::make_shared<SceneData::ActorGroup>();
                EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return Events::ProcessingResult::Success;
            }

            Events::ProcessingResult ActorGroup::UpdateActorGroups(Containers::Scene& scene) const
            {
                bool updated = false;
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<SceneData::ActorGroup>(valueStorage);
                for (SceneData::ActorGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IActorGroup>(scene.GetName(), scene.GetManifest()));
                        updated = true;
                    }
                }

                return updated ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
            }

            bool ActorGroup::SceneHasActorGroup(const Containers::Scene& scene) const
            {
                const Containers::SceneManifest& manifest = scene.GetManifest();
                Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto actorGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), Containers::DerivedTypeFilter<DataTypes::IActorGroup>());
                return actorGroup != manifestData.end();
            }
        } // Behaviors
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED