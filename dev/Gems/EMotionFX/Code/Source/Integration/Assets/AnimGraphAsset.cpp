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

#include "EMotionFX_precompiled.h"
#include <Integration/Assets/AnimGraphAsset.h>
#include <EMotionFX/Source/AnimGraphManager.h>


namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        AnimGraphAsset::AnimGraphAsset()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        AnimGraphAsset::AnimGraphInstancePtr AnimGraphAsset::CreateInstance(
            const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const EMotionFXPtr<EMotionFX::MotionSet>& motionSet)
        {
            AZ_Assert(m_emfxAnimGraph, "Anim graph asset is not loaded");
            auto animGraphInstance = EMotionFXPtr<EMotionFX::AnimGraphInstance>::MakeFromNew(
                EMotionFX::AnimGraphInstance::Create(m_emfxAnimGraph.get(), actorInstance.get(), motionSet.get()));

            if (animGraphInstance)
            {
                animGraphInstance->SetIsOwnedByRuntime(true);
            }

            return animGraphInstance;
        }

        //////////////////////////////////////////////////////////////////////////
        bool AnimGraphAssetHandler::OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            AnimGraphAsset* assetData = asset.GetAs<AnimGraphAsset>();
            assetData->m_emfxAnimGraph = EMotionFXPtr<EMotionFX::AnimGraph>::MakeFromNew(EMotionFX::GetImporter().LoadAnimGraph(
                assetData->m_emfxNativeData.data(),
                assetData->m_emfxNativeData.size()));

            if (assetData->m_emfxAnimGraph)
            {
                assetData->m_emfxAnimGraph->SetIsOwnedByRuntime(true);
            }

            AZ_Error("EMotionFX", assetData->m_emfxAnimGraph, "Failed to initialize anim graph asset %s", asset.GetId().ToString<AZStd::string>().c_str());
            return (assetData->m_emfxAnimGraph);
        }


        void AnimGraphAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            AnimGraphAsset* animGraphAsset = static_cast<AnimGraphAsset*>(ptr);
            EMotionFXPtr<EMotionFX::AnimGraph> animGraph = animGraphAsset->GetAnimGraph();

            if (animGraph)
            {
                // Get rid of all anim graph instances that refer to the anim graph we're about to destroy.
                EMotionFX::GetAnimGraphManager().RemoveAnimGraphInstances(animGraph.get());
            }

            delete ptr;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Data::AssetType AnimGraphAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<AnimGraphAsset>::Uuid();
        }

        //////////////////////////////////////////////////////////////////////////
        void AnimGraphAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.push_back("animgraph");
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Uuid AnimGraphAssetHandler::GetComponentTypeId() const
        {
            // EditorAnimGraphComponent
            return AZ::Uuid("{770F0A71-59EA-413B-8DAB-235FB0FF1384}");
        }

        //////////////////////////////////////////////////////////////////////////
        const char* AnimGraphAssetHandler::GetAssetTypeDisplayName() const
        {
            return "EMotion FX Anim Graph";
        }
    } // namespace Integration
} // namespace EMotionFX
