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

#include <SceneAPI/SceneData/Behaviors/EFXMotionGroupBehavior.h>
#include <SceneAPI/SceneData/Groups/EFXMotionGroup.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            const int EFXMotionGroupBehavior::s_preferredTabOrder = 2;

            AZ_CLASS_ALLOCATOR_IMPL(EFXMotionGroupBehavior, SystemAllocator, 0)

            EFXMotionGroupBehavior::EFXMotionGroupBehavior()
            {
                Events::ManifestMetaInfoBus::Handler::BusConnect();
                Events::AssetImportRequestBus::Handler::BusConnect();
            }

            EFXMotionGroupBehavior::~EFXMotionGroupBehavior()
            {
                Events::AssetImportRequestBus::Handler::BusDisconnect();
                Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void EFXMotionGroupBehavior::GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene)
            {
                if (SceneHasMotionGroup(scene) || Utilities::DoesSceneGraphContainDataLike<DataTypes::IAnimationData>(scene, false))
                {
                    categories.emplace_back("Motions", SceneData::EFXMotionGroup::TYPEINFO_Uuid(), s_preferredTabOrder);
                }
            }

            void EFXMotionGroupBehavior::InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(SceneData::EFXMotionGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                SceneData::EFXMotionGroup* group = azrtti_cast<SceneData::EFXMotionGroup*>(&target);
                group->SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IEFXMotionGroup>(scene.GetName(), scene.GetManifest()));

                const Containers::SceneGraph& graph = scene.GetGraph();
                auto nameStorage = graph.GetNameStorage();
                auto contentStorage = graph.GetContentStorage();

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

                auto animationData = AZStd::find_if(contentStorage.begin(), contentStorage.end(), Containers::DerivedTypeFilter<DataTypes::IAnimationData>());
                if (animationData == contentStorage.end())
                {
                    return;
                }
                const DataTypes::IAnimationData* animation = azrtti_cast<const DataTypes::IAnimationData*>(animationData->get());
                uint32_t frameCount = aznumeric_caster(animation->GetKeyFrameCount());
                group->SetStartFrame(0);
                group->SetEndFrame((frameCount > 0) ? frameCount - 1 : 0);
            }

            Events::ProcessingResult EFXMotionGroupBehavior::UpdateManifest(Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateEFXMotionGroupBehaviors(scene);
                }
                else
                {
                    return Events::ProcessingResult::Ignored;
                }
            }

            Events::ProcessingResult EFXMotionGroupBehavior::BuildDefault(Containers::Scene& scene) const
            {
                if (SceneHasMotionGroup(scene) || !Utilities::DoesSceneGraphContainDataLike<DataTypes::IAnimationData>(scene, true))
                {
                    return Events::ProcessingResult::Ignored;
                }

                // There are animations but no motion group, so add a default motion group to the manifest.
                AZStd::shared_ptr<SceneData::EFXMotionGroup> group = AZStd::make_shared<SceneData::EFXMotionGroup>();
                EBUS_EVENT(Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return Events::ProcessingResult::Success;
            }

            Events::ProcessingResult EFXMotionGroupBehavior::UpdateEFXMotionGroupBehaviors(Containers::Scene& scene) const
            {
                bool updated = false;
                Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = Containers::MakeDerivedFilterView<SceneData::EFXMotionGroup>(valueStorage);
                for (SceneData::EFXMotionGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(DataTypes::Utilities::CreateUniqueName<DataTypes::IEFXMotionGroup>(scene.GetName(), scene.GetManifest()));
                        updated = true;
                    }
                }

                return updated ? Events::ProcessingResult::Success : Events::ProcessingResult::Ignored;
            }

            bool EFXMotionGroupBehavior::SceneHasMotionGroup(const Containers::Scene& scene) const
            {
                const Containers::SceneManifest& manifest = scene.GetManifest();
                Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto motionGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), Containers::DerivedTypeFilter<DataTypes::IEFXMotionGroup>());
                return motionGroup != manifestData.end();
            }
        } // Behaviors
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED