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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/Random.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionSet.h>


namespace EMotionFX
{
    const char* AnimGraphMotionCondition::s_functionMotionEvent = "Motion Event";
    const char* AnimGraphMotionCondition::s_functionHasEnded = "Has Ended";
    const char* AnimGraphMotionCondition::s_functionHasReachedMaxNumLoops = "Has Reached Max Num Loops";
    const char* AnimGraphMotionCondition::s_functionHasReachedPlayTime = "Has Reached Specified Play Time";
    const char* AnimGraphMotionCondition::s_functionHasLessThan = "Has Less Play Time Left";
    const char* AnimGraphMotionCondition::s_functionIsMotionAssigned = "Is Motion Assigned?";
    const char* AnimGraphMotionCondition::s_functionIsMotionNotAssigned = "Is Motion Not Assigned?";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionCondition, AnimGraphConditionAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionCondition::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionCondition::EventHandler, AnimGraphObjectUniqueDataAllocator, 0)

    AnimGraphMotionCondition::AnimGraphMotionCondition()
        : AnimGraphTransitionCondition()
        , m_motionNodeId(AnimGraphNodeId::InvalidId)
        , m_motionNode(nullptr)
        , m_numLoops(1)
        , m_playTime(0.0f)
        , m_testFunction(FUNCTION_HASENDED)
    {
    }


    AnimGraphMotionCondition::AnimGraphMotionCondition(AnimGraph* animGraph)
        : AnimGraphMotionCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphMotionCondition::~AnimGraphMotionCondition()
    {
    }


    void AnimGraphMotionCondition::Reinit()
    {
        if (!AnimGraphNodeId(m_motionNodeId).IsValid())
        {
            m_motionNode = nullptr;
            return;
        }

        AnimGraphNode* node = mAnimGraph->RecursiveFindNodeById(m_motionNodeId);
        m_motionNode = azdynamic_cast<AnimGraphMotionNode*>(node);
    }


    bool AnimGraphMotionCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* AnimGraphMotionCondition::GetPaletteName() const
    {
        return "Motion Condition";
    }


    bool AnimGraphMotionCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // Make sure the motion node to which the motion condition is linked to is valid.
        if (!m_motionNode)
        {
            return false;
        }

        // Early condition function check pass for the 'Is motion assigned?'. We can do this before retrieving the unique data.
        if (m_testFunction == FUNCTION_ISMOTIONASSIGNED || m_testFunction == FUNCTION_ISMOTIONNOTASSIGNED)
        {
            const EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();
            if (!motionSet)
            {
                return false;
            }

            // Iterate over motion entries from the motion node and check if there are motions assigned to them.
            const size_t numMotions = m_motionNode->GetNumMotions();
            for (size_t i = 0; i < numMotions; ++i)
            {
                const char* motionId = m_motionNode->GetMotionId(i);
                const EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(motionId);

                if (m_testFunction == FUNCTION_ISMOTIONASSIGNED)
                {
                    // Any unassigned motion entry will make the condition to fail.
                    if (!motionEntry || (motionEntry && motionEntry->GetFilenameString().empty()))
                    {
                        return false;
                    }
                }
                else
                {
                    if (motionEntry && !motionEntry->GetFilenameString().empty())
                    {
                        // Any assigned motion entry will make the condition to fail.
                        return false;
                    }
                }
            }

            return true;
        }

        // Add the unique data for the condition to the anim graph.
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = aznew UniqueData(const_cast<AnimGraphMotionCondition*>(this), animGraphInstance, nullptr);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        MotionInstance* motionInstance = m_motionNode->FindMotionInstance(animGraphInstance);
        if (!motionInstance)
        {
            return false;
        }

        // Update the unique data.
        if (uniqueData->mMotionInstance != motionInstance ||
            uniqueData->mMotionInstance->FindEventHandlerIndex(uniqueData->mEventHandler) == MCORE_INVALIDINDEX32)
        {
            // Create a new event handler for this motion condition and add it to the motion instance.
            uniqueData->mMotionInstance = motionInstance;
            uniqueData->mEventHandler = AnimGraphMotionCondition::EventHandler::Create(const_cast<AnimGraphMotionCondition*>(this), uniqueData);
            uniqueData->mMotionInstance->AddEventHandler(uniqueData->mEventHandler);
        }

        // In case a motion event got triggered or a motion has looped constantly fire true until the condition gets reset.
        if (uniqueData->mTriggered)
        {
            uniqueData->mTriggered = false;
            return true;
        }

        // Process the condition depending on the function used.
        switch (m_testFunction)
        {
        // Has motion finished playing?
        case FUNCTION_HASENDED:
        {
            // Special case for non looping motions only.
            if (!motionInstance->GetIsPlayingForever())
            {
                // Get the play time and the animation length.
                const float currentTime = m_motionNode->GetCurrentPlayTime(animGraphInstance);
                const float maxTime     = motionInstance->GetMaxTime();

                // Differentiate between the play modes.
                const EPlayMode playMode = motionInstance->GetPlayMode();
                if (playMode == PLAYMODE_FORWARD)
                {
                    // Return true in case the current playtime has reached the animation end.
                    return currentTime >= maxTime - MCore::Math::epsilon;
                }
                else if (playMode == PLAYMODE_BACKWARD)
                {
                    // Return true in case the current playtime has reached the animation start.
                    return currentTime <= MCore::Math::epsilon;
                }
            }
        }
        break;

        // Less than a given amount of play time left.
        case FUNCTION_PLAYTIMELEFT:
        {
            const float timeLeft = motionInstance->GetMaxTime() - m_motionNode->GetCurrentPlayTime(animGraphInstance);
            return timeLeft <= m_playTime || AZ::IsClose(timeLeft, m_playTime, 0.0001f);
        }
        break;

        // Maximum number of loops.
        case FUNCTION_HASREACHEDMAXNUMLOOPS:
        {
            return motionInstance->GetNumCurrentLoops() >= m_numLoops;
        }
        break;

        // Reached the specified play time.
        // The has reached play time condition is not part of the event handler, so we have to manually handle it here.
        case FUNCTION_PLAYTIME:
        {
            return m_motionNode->GetCurrentPlayTime(animGraphInstance) >= (m_playTime - AZ::g_fltEps);
        }
        break;
        }

        // No event got triggered, continue playing the state and don't autostart the transition.
        return false;
    }


    void AnimGraphMotionCondition::Reset(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData)
        {
            uniqueData->mTriggered = false;
        }
    }


    void AnimGraphMotionCondition::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // Add the unique data for the condition to the anim graph.
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (!uniqueData)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance, nullptr);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
    }


    void AnimGraphMotionCondition::SetTestFunction(TestFunction testFunction)
    {
        m_testFunction = testFunction;
    }


    AnimGraphMotionCondition::TestFunction AnimGraphMotionCondition::GetTestFunction() const
    {
        return m_testFunction;
    }


    const char* AnimGraphMotionCondition::GetTestFunctionString() const
    {
        switch (m_testFunction)
        {
            case FUNCTION_EVENT:
            {
                return s_functionMotionEvent;
            }
            case FUNCTION_HASENDED:
            {
                return s_functionHasEnded;
            }
            case FUNCTION_HASREACHEDMAXNUMLOOPS:
            {
                return s_functionHasReachedMaxNumLoops;
            }
            case FUNCTION_PLAYTIME:
            {
                return s_functionHasReachedPlayTime;
            }
            case FUNCTION_PLAYTIMELEFT:
            {
                return s_functionHasLessThan;
            }
            case FUNCTION_ISMOTIONASSIGNED:
            {
                return s_functionIsMotionAssigned;
            }
            case FUNCTION_ISMOTIONNOTASSIGNED:
            {
                return s_functionIsMotionNotAssigned;
            }
            default:
            {
                return "None";
            }
        }
    }


    void AnimGraphMotionCondition::SetEventType(const AZStd::string& eventType)
    {
        m_eventType = eventType;
    }


    const AZStd::string& AnimGraphMotionCondition::GetEventType() const
    {
        return m_eventType;
    }



    void AnimGraphMotionCondition::SetEventParameter(const AZStd::string& eventParameter)
    {
        m_eventParameter = eventParameter;
    }


    const AZStd::string& AnimGraphMotionCondition::GetEventParameter() const
    {
        return m_eventParameter;
    }


    void AnimGraphMotionCondition::SetMotionNodeId(AnimGraphNodeId motionNodeId)
    {
        m_motionNodeId = motionNodeId;
        if (mAnimGraph)
        {
            Reinit();
        }
    }


    AnimGraphNodeId AnimGraphMotionCondition::GetMotionNodeId() const
    {
        return m_motionNodeId;
    }


    AnimGraphNode* AnimGraphMotionCondition::GetMotionNode() const
    {
        return m_motionNode;
    }



    void AnimGraphMotionCondition::SetNumLoops(AZ::u32 numLoops)
    {
        m_numLoops = numLoops;
    }


    AZ::u32 AnimGraphMotionCondition::GetNumLoops() const
    {
        return m_numLoops;
    }



    void AnimGraphMotionCondition::SetPlayTime(float playTime)
    {
        m_playTime = playTime;
    }


    float AnimGraphMotionCondition::GetPlayTime() const
    {
        return m_playTime;
    }


    void AnimGraphMotionCondition::GetSummary(AZStd::string* outResult) const
    {
        AZStd::string nodeName;
        if (m_motionNode)
        {
            nodeName = m_motionNode->GetNameString();
        }

        *outResult = AZStd::string::format("%s: Motion Node Name='%s', Test Function='%s'", RTTI_GetTypeName(), nodeName.c_str(), GetTestFunctionString());
    }


    void AnimGraphMotionCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName;
        AZStd::string columnValue;

        // Add the condition type.
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"130\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // Add the motion node name.
        AZStd::string nodeName;
        if (m_motionNode)
        {
            nodeName = m_motionNode->GetNameString();
        }

        columnName = "Motion Node Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), nodeName.c_str());

        // Add the test function.
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td><nobr>%s</nobr></td></tr></table>", columnName.c_str(), columnValue.c_str());
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphMotionCondition::UniqueData
    //--------------------------------------------------------------------------------
    AnimGraphMotionCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance, MotionInstance* motionInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
        mMotionInstance = motionInstance;
        mEventHandler   = nullptr;
        mTriggered      = false;
    }


    AnimGraphMotionCondition::UniqueData::~UniqueData()
    {
        if (mMotionInstance && mEventHandler)
        {
            mMotionInstance->RemoveEventHandler(mEventHandler);
        }
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphMotionCondition::EventHandler
    //--------------------------------------------------------------------------------
    AnimGraphMotionCondition::EventHandler::EventHandler(AnimGraphMotionCondition* condition, UniqueData* uniqueData)
        : MotionInstanceEventHandler()
    {
        mCondition      = condition;
        mUniqueData     = uniqueData;
    }


    AnimGraphMotionCondition::EventHandler::~EventHandler()
    {
    }


    void AnimGraphMotionCondition::EventHandler::OnDeleteMotionInstance()
    {
        // Reset the motion instance and the flag in the unique data.
        mUniqueData->mMotionInstance    = nullptr;
        mUniqueData->mTriggered         = false;
        mUniqueData->mEventHandler      = nullptr;
    }


    void AnimGraphMotionCondition::EventHandler::OnEvent(const EventInfo& eventInfo)
    {
        // Make sure the condition function is the right one.
        if (mCondition->GetTestFunction() != FUNCTION_EVENT)
        {
            return;
        }

        // Check if the triggered motion event is of the given type and parameter from the motion condition.
        const AZStd::string& eventParameter = mCondition->GetEventParameter();
        const AZStd::string& eventType = mCondition->GetEventType();
        if ((eventParameter.empty() || eventParameter == *eventInfo.mParameters) &&
            (eventType.empty() || AzFramework::StringFunc::Equal(eventType.c_str(), GetEventManager().GetEventTypeString(eventInfo.mTypeID), true /* case sensitive */)))
        {
            if (eventInfo.mIsEventStart)
            {
                mUniqueData->mTriggered = true;
            }
            else
            {
                mUniqueData->mTriggered = false;
            }
        }
    }


    void AnimGraphMotionCondition::EventHandler::OnHasLooped()
    {
        if (mCondition->GetTestFunction() != FUNCTION_HASENDED)
        {
            return;
        }

        mUniqueData->mTriggered = true;
    }


    void AnimGraphMotionCondition::EventHandler::OnIsFrozenAtLastFrame()
    {
        if (mCondition->GetTestFunction() != FUNCTION_HASENDED)
        {
            return;
        }

        mUniqueData->mTriggered = true;
    }


    // Callback that gets called before a node gets removed.
    void AnimGraphMotionCondition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        MCORE_UNUSED(animGraph);
        if (m_motionNodeId == nodeToRemove->GetId())
        {
            SetMotionNodeId(AnimGraphNodeId::InvalidId);
        }
    }


    AnimGraphMotionCondition::EventHandler* AnimGraphMotionCondition::EventHandler::Create(AnimGraphMotionCondition* condition, UniqueData* uniqueData)
    {
        return aznew AnimGraphMotionCondition::EventHandler(condition, uniqueData);
    }


    AZ::Crc32 AnimGraphMotionCondition::GetNumLoopsVisibility() const
    {
        return m_testFunction == FUNCTION_HASREACHEDMAXNUMLOOPS ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphMotionCondition::GetPlayTimeVisibility() const
    {
        if (m_testFunction == FUNCTION_PLAYTIME || m_testFunction == FUNCTION_PLAYTIMELEFT)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }
        
        return AZ::Edit::PropertyVisibility::Hide;
    }


    AZ::Crc32 AnimGraphMotionCondition::GetEventPropertiesVisibility() const
    {
        return m_testFunction == FUNCTION_EVENT ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    void AnimGraphMotionCondition::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        auto itConvertedIds = convertedIds.find(GetMotionNodeId());
        if (itConvertedIds != convertedIds.end())
        {
            // need to convert
            attributesString = AZStd::string::format("-motionNodeId %llu", itConvertedIds->second);
        }
    }

    void AnimGraphMotionCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphMotionCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("motionNodeId", &AnimGraphMotionCondition::m_motionNodeId)
            ->Field("testFunction", &AnimGraphMotionCondition::m_testFunction)
            ->Field("numLoops", &AnimGraphMotionCondition::m_numLoops)
            ->Field("playTime", &AnimGraphMotionCondition::m_playTime)
            ->Field("eventType", &AnimGraphMotionCondition::m_eventType)
            ->Field("eventParameter", &AnimGraphMotionCondition::m_eventParameter)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphMotionCondition>("Motion Condition", "Motion condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC("AnimGraphMotionNodeId", 0xe19a0672), &AnimGraphMotionCondition::m_motionNodeId, "Motion", "The motion node to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphMotionCondition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC("AnimGraph", 0x0d53d4b3), &AnimGraphMotionCondition::GetAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphMotionCondition::m_testFunction, "Test Function", "The type of test function or condition.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->EnumAttribute(FUNCTION_EVENT,                 s_functionMotionEvent)
                ->EnumAttribute(FUNCTION_HASENDED,              s_functionHasEnded)
                ->EnumAttribute(FUNCTION_HASREACHEDMAXNUMLOOPS, s_functionHasReachedMaxNumLoops)
                ->EnumAttribute(FUNCTION_PLAYTIME,              s_functionHasReachedPlayTime)
                ->EnumAttribute(FUNCTION_PLAYTIMELEFT,          s_functionHasLessThan)
                ->EnumAttribute(FUNCTION_ISMOTIONASSIGNED,      s_functionIsMotionAssigned)
                ->EnumAttribute(FUNCTION_ISMOTIONNOTASSIGNED,   s_functionIsMotionNotAssigned)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionCondition::m_numLoops, "Num Loops", "The int value to test against the number of loops the motion already played.")
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<AZ::s32>::max())
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionCondition::GetNumLoopsVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionCondition::m_playTime, "Time Value", "The float value in seconds to test against.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionCondition::GetPlayTimeVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionCondition::m_eventType, "Event Type", "The event type to catch.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionCondition::GetEventPropertiesVisibility)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphMotionCondition::m_eventParameter, "Event Parameter", "The event parameter to catch.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphMotionCondition::GetEventPropertiesVisibility)
            ;
    }
} // namespace EMotionFX