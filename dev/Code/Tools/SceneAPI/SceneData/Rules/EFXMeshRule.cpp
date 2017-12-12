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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Rules/EFXMeshRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(EFXMeshRule, SystemAllocator, 0)

            EFXMeshRule::EFXMeshRule()
                : m_optimizeTriangleList(true)
            {
            }

            bool EFXMeshRule::GetOptimizeTriangleList() const
            {
                return m_optimizeTriangleList;
            }

            void EFXMeshRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext || serializeContext->FindClassData(EFXMeshRule::TYPEINFO_Uuid()))
                {
                    return;
                }

                serializeContext->Class<EFXMeshRule, DataTypes::IEFXMeshRule>()->Version(2)
                    ->Field("optimizeTriangleList", &EFXMeshRule::m_optimizeTriangleList);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EFXMeshRule>("Mesh", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &EFXMeshRule::m_optimizeTriangleList, "Optimize triangle list", "");
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED