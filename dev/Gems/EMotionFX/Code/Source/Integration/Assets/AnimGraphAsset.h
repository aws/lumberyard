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


#pragma once

#include <Integration/Assets/AssetCommon.h>

namespace EMotionFX
{
    class AnimGraph;
    class AnimGraphInstance;
    class ActorInstance;

    namespace Integration
    {
        class ActorAsset;

        class AnimGraphAsset 
            : public EMotionFXAsset
        {
        public:

            friend class AnimGraphAssetHandler;

            AZ_CLASS_ALLOCATOR_DECL
            AZ_RTTI(AnimGraphAsset, "{28003359-4A29-41AE-8198-0AEFE9FF5263}", EMotionFXAsset);

            AnimGraphAsset();

            typedef EMotionFXPtr<EMotionFX::AnimGraphInstance> AnimGraphInstancePtr;
            AnimGraphInstancePtr CreateInstance(
                EMotionFX::ActorInstance* actorInstance,
                EMotionFX::MotionSet* motionSet);

            EMotionFX::AnimGraph* GetAnimGraph() { return m_emfxAnimGraph ? m_emfxAnimGraph.get() : nullptr; }

            void SetData(EMotionFX::AnimGraph* animGraph);

        private:

            AZStd::unique_ptr<EMotionFX::AnimGraph> m_emfxAnimGraph;
        };

        class AnimGraphAssetHandler : public EMotionFXAssetHandler<AnimGraphAsset>
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override final;
            void DestroyAsset(AZ::Data::AssetPtr ptr) override final;
            AZ::Data::AssetType GetAssetType() const override final;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override final;
            AZ::Uuid GetComponentTypeId() const override final;
            const char* GetAssetTypeDisplayName() const override final;
        };
    } // namespace Integration
} // namespace EMotionFX

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::Integration::AnimGraphAsset>, "{BF1ACFB9-8295-4B55-8B55-DC64BFF36BD3}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::EMotionFXPtr<EMotionFX::AnimGraphInstance>, "{769ED685-EC18-449D-9453-7D47D9BC1B8A}");
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::AnimGraphInstance, "{1E447EA4-687C-4DE8-9C48-CFF207FA5B78}");
}