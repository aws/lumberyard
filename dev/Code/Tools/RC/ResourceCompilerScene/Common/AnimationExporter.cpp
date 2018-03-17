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

#include "stdafx.h"
#include <CryCrc32.h>
#include <MathConversion.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <RC/ResourceCompilerPC/CGA/Controller.h>
#include <RC/ResourceCompilerPC/CGA/ControllerPQ.h>
#include <RC/ResourceCompilerPC/CGA/ControllerPQLog.h>
#include <RC/ResourceCompilerPC/CGF/LoaderCAF.h>
#include <RC/ResourceCompilerPC/AdjustRotLog.h>

#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/AnimationExporter.h>
#include <RC/ResourceCompilerScene/Common/AssetExportUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        AnimationExporter::AnimationExporter()
        {
            BindToCall(&AnimationExporter::CreateControllerData);
        }

        void AnimationExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AnimationExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        //
        // Implemented by referencing to ColladaScene::CreateControllerSkinningInfo
        //
        SceneAPI::Events::ProcessingResult AnimationExporter::CreateControllerData(AnimationExportContext& context)
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            context.m_controllerSkinningInfo.m_rootBoneId = static_cast<AZ::u32>(-1);

            const char* rootBoneName = context.m_rootBoneName.c_str();
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(rootBoneName);
            if (!nodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone '%s' cannot be found.\n", rootBoneName);
                return SceneEvents::ProcessingResult::Failure;
            }

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();            
            auto nameContentView = SceneViews::MakePairView(nameStorage, contentStorage);
            auto graphDownwardsView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, nodeIndex, nameContentView.begin(), true);
            for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
            {
                if (!it->second)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                AZStd::shared_ptr<const SceneDataTypes::IBoneData> nodeBone = azrtti_cast<const SceneDataTypes::IBoneData*>(it->second);
                if (!nodeBone)
                {
                    it.IgnoreNodeDescendants();
                    continue;
                }

                // Currently only get the first (one) AnimationData
                auto childView = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(graph, graph.ConvertToNodeIndex(it.GetHierarchyIterator()), 
                    graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(childView.begin(), childView.end(), SceneContainers::DerivedTypeFilter<SceneDataTypes::IAnimationData>());
                if (result == childView.end())
                {
                    continue;
                }

                const SceneDataTypes::IAnimationData* animation = azrtti_cast<const SceneDataTypes::IAnimationData*>(result->get());

                CControllerPQLog* controller = new CControllerPQLog;
                controller->m_nControllerId = CCrc32::ComputeLowercase(it->first.GetName());

                // If we spot the root bone, store its controller Id to be passed to the compression step.
                if (0 == stricmp(it->first.GetPath(), context.m_rootBoneName.c_str()))
                {
                    context.m_controllerSkinningInfo.m_rootBoneId = controller->m_nControllerId;
                }

                uint32_t totalFrames = (context.m_endFrame - context.m_startFrame);
                controller->m_arrTimes.resize(totalFrames);
                controller->m_arrKeys.resize(totalFrames);

                CryQuat lastQuat;
                lastQuat.SetIdentity();
                CryKeyPQLog lastKey;
                lastKey.nTime = 0;
                lastKey.vPos = Vec3(0, 0, 0);
                lastKey.vRotLog = Vec3(0, 0, 0);
                
                for (size_t frame = 0; frame < totalFrames; ++frame)
                {
                    controller->m_arrTimes[frame] = frame * TICKS_CONVERT;
                    Transform boneTransform = animation->GetKeyFrame(frame + context.m_startFrame);

                    CryQuat rotationQuat = AZTransformToLYQuatT(boneTransform).q;
                    rotationQuat = !rotationQuat;

                    if (AssetExportUtilities::CryQuatDotProd(lastQuat, rotationQuat) >= 0)
                    {
                        lastQuat = rotationQuat;
                    }
                    else
                    {
                        lastQuat = -rotationQuat;
                    }

                    CryKeyPQLog currentKey;
                    currentKey.vPos = AZVec3ToLYVec3(boneTransform.GetTranslation());
                    currentKey.vRotLog = Quat::log(lastQuat);

                    AdjustRotLog(currentKey.vRotLog, lastKey.vRotLog);
                     
                    lastKey = currentKey;

                    PQLog controllerKey;
                    //this data is passed into CAFSaver::Save which saves controllers to the CONTROLLER_CHUNK_DESC_0830 format
                    //this format expects position in controller data to be in units 100 smaller than the engine (theoretically cm vs m)
                    //because our data is stored in the scene in meters we need to convert to cm before saving.
                    //this will be converted back to meters on loading in the following code
                    // pController->m_arrKeys[i].vPos = pCryKey[i].vPos / 100.0f;
                    // in GlobalAnimationHeaderCAF.cpp
                    //the same format is used for direct export from maya so it has existing
                    //data pipelines that encourage maintaining the scaling
                    controllerKey.vPos = currentKey.vPos * 100.0f; 
                    controllerKey.vRotLog = currentKey.vRotLog;
                    controller->m_arrKeys[frame] = controllerKey;
                }

                context.m_controllerSkinningInfo.m_pControllers.push_back(controller);
                context.m_controllerSkinningInfo.m_arrBoneNameTable.push_back(it->first.GetName());
            }
            
            return SceneEvents::ProcessingResult::Success;
        }
    } // namespace RC
} // namespace AZ