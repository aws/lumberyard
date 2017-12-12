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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/Behaviors/SkeletonGroup.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            const int SkeletonGroup::s_rigsPreferredTabOrder = 1;

            void SkeletonGroup::Activate()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void SkeletonGroup::Deactivate()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void SkeletonGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SkeletonGroup, BehaviorComponent>()->Version(1);
                }
            }

            void SkeletonGroup::GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene)
            {
                if (SceneHasSkeletonGroup(scene) || Utilities::DoesSceneGraphContainDataLike<DataTypes::IBoneData>(scene, false))
                {
                    categories.emplace_back("Rigs", SceneData::SkeletonGroup::TYPEINFO_Uuid(), s_rigsPreferredTabOrder);
                }
            }

            void SkeletonGroup::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(SceneData::SkeletonGroup::TYPEINFO_Uuid()))
                {
                    SceneData::SkeletonGroup* group = azrtti_cast<SceneData::SkeletonGroup*>(&target);

                    group->SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::ISkeletonGroup>(scene.GetName(), scene.GetManifest()));

                    const Containers::SceneGraph &graph = scene.GetGraph();
                    auto contentStorage = graph.GetContentStorage();
                    auto nameStorage = graph.GetNameStorage();
                    auto nameContentView = Containers::Views::MakePairView(nameStorage, contentStorage);
                    AZStd::string shallowestRootBoneName;
                    auto graphDownwardsView = Containers::Views::MakeSceneGraphDownwardsView<Containers::Views::BreadthFirst>(graph, graph.GetRoot(), nameContentView.begin(), true);
                    for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
                    {
                        if (!it->second)
                        {
                            continue;
                        }
                        if (it->second->RTTI_IsTypeOf(AZ::SceneData::GraphData::RootBoneData::TYPEINFO_Uuid()))
                        {
                            shallowestRootBoneName = it->first.GetPath();
                            break;
                        }
                    }
                    group->SetSelectedRootBone(shallowestRootBoneName);
                }
            }

            Events::ProcessingResult SkeletonGroup::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateSkeletonGroups(scene);
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            Events::ProcessingResult SkeletonGroup::BuildDefault(Containers::Scene& scene) const
            {
                if (SceneHasSkeletonGroup(scene) || !Utilities::DoesSceneGraphContainDataLike<DataTypes::IBoneData>(scene, true))
                {
                    return Events::ProcessingResult::Ignored;
                }

                // There are skeletons but no skeleton group, so add a default skeleton group to the manifest.
                AZStd::shared_ptr<SceneData::SkeletonGroup> group = AZStd::make_shared<SceneData::SkeletonGroup>();
                EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return Events::ProcessingResult::Success;
            }

            Events::ProcessingResult SkeletonGroup::UpdateSkeletonGroups(Containers::Scene& scene) const
            {
                bool updated = false;
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<SceneData::SkeletonGroup>(valueStorage);
                for (SceneData::SkeletonGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::ISkeletonGroup>(scene.GetName(), scene.GetManifest()));
                        updated = true;
                    }
                }

                return updated ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
            }

            bool SkeletonGroup::SceneHasSkeletonGroup(const Containers::Scene& scene) const
            {
                const Containers::SceneManifest& manifest = scene.GetManifest();
                Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto skeletonGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), Containers::DerivedTypeFilter<DataTypes::ISkeletonGroup>());
                return skeletonGroup != manifestData.end();
            }
        } // namespace Behaviors
    } // namespace SceneAPI
} // namespace AZ
