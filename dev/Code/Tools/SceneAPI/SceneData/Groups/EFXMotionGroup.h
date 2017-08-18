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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IEFXMotionGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class EFXMotionGroup
                : public DataTypes::IEFXMotionGroup
            {
            public:
                AZ_RTTI(EFXMotionGroup, "{1B0ABB1E-F6DF-4534-9A35-2DD8244BF58C}", DataTypes::IEFXMotionGroup);
                AZ_CLASS_ALLOCATOR_DECL
                
                EFXMotionGroup();
                ~EFXMotionGroup() override = default;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name);

                Containers::RuleContainer& GetRuleContainer();
                const Containers::RuleContainer& GetRuleContainerConst() const;

                // IEFXMotionGroup overrides
                const AZStd::string& GetSelectedRootBone() const override;
                uint32_t GetStartFrame() const override;
                uint32_t GetEndFrame() const override;
                void SetSelectedRootBone(const AZStd::string& selectedRootBone)  override;
                void SetStartFrame(uint32_t frame) override;
                void SetEndFrame(uint32_t frame) override;

                // IGroup overrides
                const AZStd::string& GetName() const override;

                static void Reflect(ReflectContext* context);

            protected:
                Containers::RuleContainer   m_rules;
                AZStd::string               m_name;
                AZStd::string               m_selectedRootBone;
                uint32_t                    m_startFrame;
                uint32_t                    m_endFrame;
            };
        }
    }
}
#endif //MOTIONCANVAS_GEM_ENABLED