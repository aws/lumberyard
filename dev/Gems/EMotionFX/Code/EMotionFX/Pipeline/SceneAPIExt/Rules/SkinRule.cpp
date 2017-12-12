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
#include <SceneAPI/SceneData/Rules/EFXSkinRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(EFXSkinRule, SystemAllocator, 0)

            EFXSkinRule::EFXSkinRule()
                : m_maxWeightsPerVertex(4)
                , m_weightThreshold(0.001f)
            {
            }
            uint32_t EFXSkinRule::GetMaxWeightsPerVertex() const
            {
                return m_maxWeightsPerVertex;
            }

            float EFXSkinRule::GetWeightThreshold() const
            {
                return m_weightThreshold;
            }

            void EFXSkinRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext || serializeContext->FindClassData(EFXSkinRule::TYPEINFO_Uuid()))
                {
                    return;
                }

                serializeContext->Class<EFXSkinRule, DataTypes::IEFXSkinRule>()->Version(2)
                    ->Field("maxWeightsPerVertex", &EFXSkinRule::m_maxWeightsPerVertex)
                    ->Field("weightThreshold", &EFXSkinRule::m_weightThreshold);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EFXSkinRule>("Skin", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &EFXSkinRule::m_maxWeightsPerVertex, "Max weights per vertex", "")
                        ->Attribute(Edit::Attributes::Min, 1)
                        ->Attribute(Edit::Attributes::Max, 4)
                        ->DataElement(Edit::UIHandlers::Default, &EFXSkinRule::m_weightThreshold, "Weight threshold", "Weight value less than this will be ignored during import.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f);
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED