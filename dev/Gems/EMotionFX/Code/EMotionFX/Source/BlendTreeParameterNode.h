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

#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     *
     */
    class EMFX_API BlendTreeParameterNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeParameterNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeParameterNode, "{4510529A-323F-40F6-B773-9FA8FC4DE53D}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000007
        };

        enum
        {
            ATTRIB_MASK     = 0
        };

        static BlendTreeParameterNode* Create(AnimGraph* animGraph);

        void InitForAnimGraph(AnimGraph* animGraph) override;
        void OnParametersChanged(AnimGraph* animGraph);
        void OnUpdateAttributes() override;
        void Reinit() override;

        void RegisterPorts() override;
        void RegisterAttributes() override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

        uint32 GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const MCore::Array<uint32>& GetParameterIndices() const;
        uint32 GetParameterIndex(uint32 portNr) const;
        uint32 GetPortForParameterIndex(uint32 parameterIndex) const;
        uint32 CalcNewPortForParameterIndex(uint32 parameterIndex, const AZStd::vector<AZStd::string>& parametersToBeRemoved) const;

        void CalcConnectedParameterNames(AZStd::vector<AZStd::string>& outParameterNames);

    private:
        MCore::Array<uint32>    mParameterIndices;              /**< The indices of the visible and available parameters. */
        bool                    mUpdateParameterMaskLocked;

        BlendTreeParameterNode(AnimGraph* animGraph);
        ~BlendTreeParameterNode();

        bool CheckIfParameterIndexUpdateNeeded() const;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
