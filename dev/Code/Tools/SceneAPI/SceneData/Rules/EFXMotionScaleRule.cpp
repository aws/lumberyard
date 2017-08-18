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

#include <SceneAPI/SceneData/Rules/EFXMotionScaleRule.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {

            AZ_CLASS_ALLOCATOR_IMPL(EFXMotionScaleRule, SystemAllocator, 0)

            EFXMotionScaleRule::EFXMotionScaleRule()
                : m_scaleFactor(1.0f)
            {
            }

            void EFXMotionScaleRule::SetScaleFactor(float value)
            {
                m_scaleFactor = value;
            }

            float EFXMotionScaleRule::GetScaleFactor()  const
            {
                return m_scaleFactor;
            }

            void EFXMotionScaleRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext || serializeContext->FindClassData(EFXMotionScaleRule::TYPEINFO_Uuid()))
                {
                    return;
                }
                
                serializeContext->Class<EFXMotionScaleRule, DataTypes::IEFXMotionScaleRule>()->Version(1)
                    ->Field("scaleFactor", &EFXMotionScaleRule::m_scaleFactor);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EFXMotionScaleRule>("Scale motion", "Scale the spatial extent of motion")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Default, &EFXMotionScaleRule::m_scaleFactor, "Scale factor", "Scale factor")
                            ->Attribute(Edit::Attributes::Min, 0.0001f)
                            ->Attribute(Edit::Attributes::Max, 1000000.0f);
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED