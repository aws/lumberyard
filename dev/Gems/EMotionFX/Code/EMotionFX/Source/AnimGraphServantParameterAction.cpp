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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphServantParameterAction.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphServantParameterAction, AnimGraphAllocator, 0)

    AnimGraphServantParameterAction::AnimGraphServantParameterAction()
        : AnimGraphTriggerAction()
        , m_triggerValue(0.0f)
    {
    }


    AnimGraphServantParameterAction::AnimGraphServantParameterAction(AnimGraph* animGraph)
        : AnimGraphServantParameterAction()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphServantParameterAction::~AnimGraphServantParameterAction()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }


    bool AnimGraphServantParameterAction::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTriggerAction::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        return true;
    }


    const char* AnimGraphServantParameterAction::GetPaletteName() const
    {
        return "Servant Parameter Action";
    }


    void AnimGraphServantParameterAction::TriggerAction(AnimGraphInstance* animGraphInstance) const
    {
        const AZStd::vector<AnimGraphInstance*>& servantGraphs = animGraphInstance->GetServantGraphs();

        for (AnimGraphInstance* servantGraph : servantGraphs)
        {
            MCore::Attribute* attribute = servantGraph->FindParameter(m_parameterName);
            if (attribute)
            {
                switch (attribute->GetType())
                {
                case MCore::AttributeBool::TYPE_ID:
                {
                    MCore::AttributeBool* atrBool = static_cast<MCore::AttributeBool*>(attribute);
                    atrBool->SetValue(m_triggerValue != 0.0f);
                    break;
                }
                case MCore::AttributeFloat::TYPE_ID:
                {
                    MCore::AttributeFloat* atrFloat = static_cast<MCore::AttributeFloat*>(attribute);
                    atrFloat->SetValue(m_triggerValue);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Type %d of attribute %s are not supported", attribute->GetType(), m_parameterName);
                    break;
                }
                }

                AZ::Outcome<size_t> index = servantGraph->FindParameterIndex(m_parameterName);
                const ValueParameter* valueParameter = servantGraph->GetAnimGraph()->FindValueParameter(index.GetValue());

                AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnParameterActionTriggered, valueParameter);
            }
        }
    }


    void AnimGraphServantParameterAction::SetParameterName(const AZStd::string& parameterName)
    {
        m_parameterName = parameterName;
    }


    const AZStd::string& AnimGraphServantParameterAction::GetParameterName() const
    {
        return m_parameterName;
    }


    // Construct and output the information summary string for this object
    void AnimGraphServantParameterAction::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Parameter Name='%s", RTTI_GetTypeName(), m_parameterName.c_str());
    }


    // Construct and output the tooltip for this object
    void AnimGraphServantParameterAction::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // Add the action type
        columnName = "Action Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // Add the parameter
        columnName = "Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_parameterName.c_str());
    }


    AnimGraph* AnimGraphServantParameterAction::GetRefAnimGraph() const
    {
        if (m_refAnimGraphAsset.GetId().IsValid() && m_refAnimGraphAsset.IsReady())
        {
            return m_refAnimGraphAsset.Get()->GetAnimGraph();
        }
        return nullptr;
    }


    void AnimGraphServantParameterAction::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphServantParameterAction, AnimGraphTriggerAction>()
            ->Version(1)
            ->Field("animGraphAsset", &AnimGraphServantParameterAction::m_refAnimGraphAsset)
            ->Field("parameterName", &AnimGraphServantParameterAction::m_parameterName)
            ->Field("triggerValue", &AnimGraphServantParameterAction::m_triggerValue)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphServantParameterAction>("Servant Parameter Action", "Servant parameter action attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphServantParameterAction::m_refAnimGraphAsset, "Servant anim graph", "Servant anim graph that we want to pick a parameter from")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphServantParameterAction::OnAnimGraphAssetChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("AnimGraphParameter", 0x778af55a), &AnimGraphServantParameterAction::m_parameterName, "Servant parameter", "The servant parameter that we want to sync to.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphServantParameterAction::GetRefAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphServantParameterAction::m_triggerValue, "Trigger value", "The value that the parameter will be override to.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ;
    }


    void AnimGraphServantParameterAction::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            m_refAnimGraphAsset = asset;

            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);

            OnAnimGraphAssetReady();
        }
    }


    void AnimGraphServantParameterAction::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);
            m_refAnimGraphAsset = asset;

            OnAnimGraphAssetReady();
        }
    }

    void AnimGraphServantParameterAction::OnAnimGraphAssetChanged()
    {
        LoadAnimGraphAsset();
    }

    void AnimGraphServantParameterAction::LoadAnimGraphAsset()
    {
        if (m_refAnimGraphAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_refAnimGraphAsset.GetId());

            if (!m_refAnimGraphAsset.IsLoading()) // someone else is loading it, we will get notified
            {
                if (!m_refAnimGraphAsset.IsReady())
                {
                    if (!m_refAnimGraphAsset.QueueLoad())
                    {
                        // When switching between assets I have seen that the asset is not ready, we call QueueLoad, it fails
                        // because the asset is already loaded, and at that point it assigns the right m_assetData
                        AZ_Assert(m_refAnimGraphAsset.IsReady(), "Expecting a ready anim graph asset");
                        OnAnimGraphAssetReady();
                    }
                }
                else
                {
                    // Asset is ready
                    OnAnimGraphAssetReady();
                }
            }
        }
    }

    void AnimGraphServantParameterAction::OnAnimGraphAssetReady()
    {
        // Verify if the m_parameterName is valid in the ref anim graph
        AnimGraph* refAnimGraph = GetRefAnimGraph();
        if (refAnimGraph)
        {
            if (!refAnimGraph->FindParameterByName(m_parameterName))
            {
                m_parameterName.clear();
            }
        }
    }
} // namespace EMotionFX