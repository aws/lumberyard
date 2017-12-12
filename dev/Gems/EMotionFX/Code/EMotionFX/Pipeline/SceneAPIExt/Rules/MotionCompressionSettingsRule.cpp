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

#include <SceneAPI/SceneData/Rules/EFXMotionCompressionSettingsRule.h>
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

            AZ_CLASS_ALLOCATOR_IMPL(EFXMotionCompressionSettingsRule, SystemAllocator, 0)

            EFXMotionCompressionSettingsRule::EFXMotionCompressionSettingsRule()
                : m_maxTranslationError(0.000025f)
                , m_maxRotationError(0.000025f)
                , m_maxScaleError(0.0001f)
            {
            }

            void EFXMotionCompressionSettingsRule::SetMaxTranslationError(float value)
            {
                m_maxTranslationError = value;
            }

            void EFXMotionCompressionSettingsRule::SetMaxRotationError(float value)
            {
                m_maxRotationError = value;
            }

            void EFXMotionCompressionSettingsRule::SetMaxScaleError(float value)
            {
                m_maxScaleError = value;
            }

            float EFXMotionCompressionSettingsRule::GetMaxTranslationError() const
            {
                return m_maxTranslationError;
            }
            
            float EFXMotionCompressionSettingsRule::GetMaxRotationError() const
            {
                return m_maxRotationError;
            }
            
            float EFXMotionCompressionSettingsRule::GetMaxScaleError() const
            {
                return m_maxScaleError;
            }

            void EFXMotionCompressionSettingsRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext || serializeContext->FindClassData(EFXMotionCompressionSettingsRule::TYPEINFO_Uuid()))
                {
                    return;
                }
                
                serializeContext->Class<EFXMotionCompressionSettingsRule, DataTypes::IEFXMotionCompressionSettingsRule>()->Version(1)
                    ->Field("maxTranslationError", &EFXMotionCompressionSettingsRule::m_maxTranslationError)
                    ->Field("maxRotationError", &EFXMotionCompressionSettingsRule::m_maxRotationError);
                // hide the max scale tolerance in UI as the engine does not support it yet.
                //    ->Field("maxScaleError", &EFXMotionCompressionSettingsRule::m_maxScaleError);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EFXMotionCompressionSettingsRule>("Compression settings", "Error tolerance settings while compressing")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->DataElement(Edit::UIHandlers::Slider, &EFXMotionCompressionSettingsRule::m_maxTranslationError, "Max translation error tolerance", "Maximum error allowed in translation")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 0.1f)
                        ->Attribute(Edit::Attributes::Step, 0.000001f)
                        ->Attribute(Edit::Attributes::Decimals, 6)
                        ->DataElement(Edit::UIHandlers::Slider, &EFXMotionCompressionSettingsRule::m_maxRotationError, "Max rotation error tolerance", "Maximum error allowed in rotation")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 0.025f)
                        ->Attribute(Edit::Attributes::Step, 0.000001f)
                        ->Attribute(Edit::Attributes::Decimals, 6);
                        // hide the max scale tolerance in UI as the engine does not support it yet.
                        // ->DataElement(Edit::UIHandlers::Slider, &EFXMotionCompressionSettingsRule::m_maxScaleError, "Max Scale Error", "Maximum error allowed in scale")
                        // ->Attribute(Edit::Attributes::Min, 0.0f)
                        // ->Attribute(Edit::Attributes::Max, 0.01f)
                        // ->Attribute(Edit::Attributes::Step, 0.00001f)
                        // ->Attribute(Edit::Attributes::Decimals, 5);
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED