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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPIExt/Rules/MeshRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MeshRule, AZ::SystemAllocator, 0)

            MeshRule::MeshRule()
                : m_optimizeTriangleList(true)
            {
            }

            bool MeshRule::GetOptimizeTriangleList() const
            {
                return m_optimizeTriangleList;
            }

            void MeshRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<IMeshRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);

                serializeContext->Class<MeshRule, IMeshRule>()->Version(2)
                    ->Field("optimizeTriangleList", &MeshRule::m_optimizeTriangleList);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MeshRule>("Mesh", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshRule::m_optimizeTriangleList, "Optimize triangle list", "");
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX