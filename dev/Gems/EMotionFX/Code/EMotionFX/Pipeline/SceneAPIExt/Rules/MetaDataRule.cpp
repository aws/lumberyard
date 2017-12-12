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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
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
                if (serializeContext)
                {
                    serializeContext->Class<MetaDataRule, AZ::SceneAPI::DataTypes::IManifestObject>()->
                                      Version(1)->
                                      Field("metaData", &MetaDataRule::m_metaData);

#if defined(_DEBUG)
                    // Only enabled in debug builds, not in profile builds as this is currently only used for debugging
                    //      purposes and not to be presented to the user.
                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<MetaDataRule>("Meta data", "Additional information attached by EMotion FX.")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute("AutoExpand", true)
                                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &MetaDataRule::m_metaData, "", "EMotion FX data build as string.")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, true);
                    }
#endif // _DEBUG
                }
            }

            bool MetaDataRule::LoadMetaData(const AZ::SceneAPI::DataTypes::IGroup& group, AZStd::string& outMetaDataString)
            {
                AZStd::shared_ptr<MetaDataRule> metaDataRule = group.GetRuleContainerConst().FindFirstByType<MetaDataRule>();
                if (!metaDataRule)
                {
                    outMetaDataString.clear();
                    return false;
                }

                outMetaDataString = metaDataRule->GetMetaData();
                return true;
            }

            void MetaDataRule::SaveMetaData(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group, const AZStd::string& metaDataString)
            {
                namespace SceneEvents = AZ::SceneAPI::Events;

                AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();

                AZStd::shared_ptr<MetaDataRule> metaDataRule = rules.FindFirstByType<MetaDataRule>();
                if (!metaDataString.empty())
                {
                    // Update the meta data in case there is a meta data rule already.
                    if (metaDataRule)
                    {
                        metaDataRule->SetMetaData(metaDataString.c_str());
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, metaDataRule.get(), nullptr);
                    }
                    else
                    {
                        // No meta data rule exists yet, create one and add it.
                        metaDataRule = AZStd::make_shared<MetaDataRule>(metaDataString.c_str());
                        rules.AddRule(metaDataRule);
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::InitializeObject, scene, *metaDataRule);
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, &group, nullptr);
                    }
                }
                else
                {
                    if (metaDataRule)
                    {
                        // Rather than storing an empty string, remove the whole rule.
                        rules.RemoveRule(metaDataRule);
                        SceneEvents::ManifestMetaInfoBus::Broadcast(&SceneEvents::ManifestMetaInfoBus::Events::ObjectUpdated, scene, &group, nullptr);
                    }
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX