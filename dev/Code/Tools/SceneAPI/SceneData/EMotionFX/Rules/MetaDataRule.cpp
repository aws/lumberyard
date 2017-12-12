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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneData/EMotionFX/Rules/MetaDataRule.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {

            MetaDataRule::MetaDataRule(const char* metaData) : IRule()
            {
                SetMetaData(metaData);
            }


            const AZStd::string& MetaDataRule::GetMetaData() const
            {
                return m_metaData;
            }


            void MetaDataRule::SetMetaData(const char* metaData)
            {
                m_metaData = metaData;
            }


            void MetaDataRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext && !serializeContext->FindClassData(MetaDataRule::TYPEINFO_Uuid()))
                {
                    serializeContext->Class<MetaDataRule, AZ::SceneAPI::DataTypes::IManifestObject>()->
                                      Version(1)->
                                      Field("metaData", &MetaDataRule::m_metaData);
                }
            }


            bool MetaDataRule::LoadMetaData(const DataTypes::IGroup& group, AZStd::string& outMetaDataString)
            {
                AZStd::shared_ptr<AZ::SceneAPI::SceneData::MetaDataRule> metaDataRule = group.GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::MetaDataRule>();
                if (!metaDataRule)
                {
                    outMetaDataString.clear();
                    return false;
                }

                outMetaDataString = metaDataRule->GetMetaData();
                return true;
            }


            void MetaDataRule::SaveMetaData(DataTypes::IGroup& group, const AZStd::string& metaDataString)
            {
                Containers::RuleContainer& rules = group.GetRuleContainer();

                AZStd::shared_ptr<AZ::SceneAPI::SceneData::MetaDataRule> metaDataRule = rules.FindFirstByType<AZ::SceneAPI::SceneData::MetaDataRule>();
                if (!metaDataString.empty())
                {
                    // Update the meta data in case there is a meta data rule already.
                    if (metaDataRule)
                    {
                        metaDataRule->SetMetaData(metaDataString.c_str());
                    }
                    else
                    {
                        // No meta data rule exists yet, create one and add it.
                        metaDataRule = AZStd::make_shared<AZ::SceneAPI::SceneData::MetaDataRule>(metaDataString.c_str());
                        rules.AddRule(metaDataRule);
                    }
                }
                else
                {
                    if (metaDataRule)
                    {
                        // Rather than storing an empty string, remove the whole rule.
                        rules.RemoveRule(metaDataRule);
                    }
                }
            }

        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED