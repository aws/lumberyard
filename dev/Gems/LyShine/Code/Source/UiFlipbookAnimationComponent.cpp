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
#include "LyShine_precompiled.h"

#include "UiFlipbookAnimationComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiGameEntityContextBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/ISprite.h>
#include <ITimer.h>

namespace
{
    const char* spritesheetNotConfiguredMessage = "<No spritesheet configured>";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Forwards events to Lua for UiFlipbookAnimationNotificationsBus
class UiFlipbookAnimationNotificationsBusBehaviorHandler
    : public UiFlipbookAnimationNotificationsBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiFlipbookAnimationNotificationsBusBehaviorHandler, "{0A92A44E-0C32-4AD6-9C49-222A484B54FF}", AZ::SystemAllocator,
        OnAnimationStarted, OnAnimationStopped, OnLoopSequenceCompleted);

    void OnAnimationStarted() override
    {
        Call(FN_OnAnimationStarted);
    }

    void OnAnimationStopped() override
    {
        Call(FN_OnAnimationStopped);
    }

    void OnLoopSequenceCompleted() override
    {
        Call(FN_OnLoopSequenceCompleted);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

void UiFlipbookAnimationComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<UiFlipbookAnimationComponent, AZ::Component>()
            ->Version(2)
            ->Field("Start Frame", &UiFlipbookAnimationComponent::m_startFrame)
            ->Field("End Frame", &UiFlipbookAnimationComponent::m_endFrame)
            ->Field("Loop Start Frame", &UiFlipbookAnimationComponent::m_loopStartFrame)
            ->Field("Loop Type", &UiFlipbookAnimationComponent::m_loopType)
            ->Field("Frame Delay", &UiFlipbookAnimationComponent::m_frameDelay)
            ->Field("Auto Play", &UiFlipbookAnimationComponent::m_isAutoPlay)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiFlipbookAnimationComponent>("FlipbookAnimation",
                    "Allows you to animate a sprite sheet entity.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Flipbook.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Flipbook.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_startFrame, "Start frame", "Frame to start at")
                ->Attribute("EnumValues", &UiFlipbookAnimationComponent::PopulateIndexStringList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiFlipbookAnimationComponent::OnStartFrameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_endFrame, "End frame", "Frame to end at")
                ->Attribute("EnumValues", &UiFlipbookAnimationComponent::PopulateIndexStringList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiFlipbookAnimationComponent::OnEndFrameChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_loopStartFrame, "Loop start frame", "Frame to start looping from")
                ->Attribute("EnumValues", &UiFlipbookAnimationComponent::PopulateConstrainedIndexStringList)
            ;

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiFlipbookAnimationComponent::m_loopType, "Loop type", "Go from start to end continuously or start to end and back to start")
                ->EnumAttribute(UiFlipbookAnimationInterface::LoopType::None, "None")
                ->EnumAttribute(UiFlipbookAnimationInterface::LoopType::Linear, "Linear")
                ->EnumAttribute(UiFlipbookAnimationInterface::LoopType::PingPong, "PingPong")
            ;

            editInfo->DataElement(0, &UiFlipbookAnimationComponent::m_frameDelay, "Frame delay", "Number of seconds to delay until the next frame")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, AZ_FLT_MAX)
            ;

            editInfo->DataElement(0, &UiFlipbookAnimationComponent::m_isAutoPlay, "Auto Play", "Automatically starts playing the animation")
            ;
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiFlipbookAnimationBus>("UiFlipbookAnimationBus")
            ->Event("Start", &UiFlipbookAnimationBus::Events::Start)
            ->Event("Stop", &UiFlipbookAnimationBus::Events::Stop)
            ->Event("IsPlaying", &UiFlipbookAnimationBus::Events::IsPlaying)
            ->Event("GetStartFrame", &UiFlipbookAnimationBus::Events::GetStartFrame)
            ->Event("SetStartFrame", &UiFlipbookAnimationBus::Events::SetStartFrame)
            ->Event("GetEndFrame", &UiFlipbookAnimationBus::Events::GetEndFrame)
            ->Event("SetEndFrame", &UiFlipbookAnimationBus::Events::SetEndFrame)
            ->Event("GetCurrentFrame", &UiFlipbookAnimationBus::Events::GetCurrentFrame)
            ->Event("SetCurrentFrame", &UiFlipbookAnimationBus::Events::SetCurrentFrame)
            ->Event("GetLoopStartFrame", &UiFlipbookAnimationBus::Events::GetLoopStartFrame)
            ->Event("SetLoopStartFrame", &UiFlipbookAnimationBus::Events::SetLoopStartFrame)
            ->Event("GetLoopType", &UiFlipbookAnimationBus::Events::GetLoopType)
            ->Event("SetLoopType", &UiFlipbookAnimationBus::Events::SetLoopType)
            ->Event("GetFrameDelay", &UiFlipbookAnimationBus::Events::GetFrameDelay)
            ->Event("SetFrameDelay", &UiFlipbookAnimationBus::Events::SetFrameDelay)
            ->Event("GetIsAutoPlay", &UiFlipbookAnimationBus::Events::GetIsAutoPlay)
            ->Event("SetIsAutoPlay", &UiFlipbookAnimationBus::Events::SetIsAutoPlay)
        ;

        behaviorContext->EBus<UiFlipbookAnimationNotificationsBus>("UiFlipbookAnimationNotificationsBus")
            ->Handler<UiFlipbookAnimationNotificationsBusBehaviorHandler>();

        behaviorContext->Enum<(int)UiFlipbookAnimationInterface::LoopType::None>("eUiFlipbookAnimationLoopType_None")
            ->Enum<(int)UiFlipbookAnimationInterface::LoopType::Linear>("eUiFlipbookAnimationLoopType_Linear")
            ->Enum<(int)UiFlipbookAnimationInterface::LoopType::PingPong>("eUiFlipbookAnimationLoopType_PingPong");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiFlipbookAnimationComponent::GetMaxFrame() const
{
    AZ::u32 numFramesInSpriteSheet = 0;

    EBUS_EVENT_ID_RESULT(numFramesInSpriteSheet, GetEntityId(), UiImageBus, GetSpriteSheetCellCount);

    return numFramesInSpriteSheet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiFlipbookAnimationComponent::FrameWithinRange(AZ::u32 frameValue)
{
    AZ::u32 maxFrame = GetMaxFrame();
    return maxFrame > 0 && frameValue < maxFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::AZu32ComboBoxVec UiFlipbookAnimationComponent::PopulateIndexStringList() const
{
    AZ::u32 numCells = GetMaxFrame();
    if (numCells != 0)
    {
        ISprite* sprite = nullptr;
        EBUS_EVENT_ID_RESULT(sprite, GetEntityId(), UiImageBus, GetSprite);
        return UiImageComponent::GetEnumSpriteIndexList(0, numCells - 1, sprite);
    }
    
    // Add an empty element to prevent an AzToolsFramework warning that fires
    // when an empty container is encountered.
    UiImageComponent::AZu32ComboBoxVec comboBoxVec;
    comboBoxVec.push_back(AZStd::make_pair(0, spritesheetNotConfiguredMessage));

    return comboBoxVec;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::AZu32ComboBoxVec UiFlipbookAnimationComponent::PopulateConstrainedIndexStringList() const
{
    ISprite* sprite = nullptr;
    EBUS_EVENT_ID_RESULT(sprite, GetEntityId(), UiImageBus, GetSprite);

    const char* errorMessage = spritesheetNotConfiguredMessage;
    if (sprite && sprite->IsSpriteSheet())
    {
        errorMessage = "<Invalid loop range>";
    }

    return UiImageComponent::GetEnumSpriteIndexList(m_startFrame, m_endFrame, sprite, errorMessage);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::OnStartFrameChange()
{
    m_endFrame = AZ::GetMax<AZ::u32>(m_startFrame, m_endFrame);
    m_currentFrame = AZ::GetClamp<AZ::u32>(m_currentFrame, m_startFrame, m_endFrame);
    m_loopStartFrame = AZ::GetClamp<AZ::u32>(m_loopStartFrame, m_startFrame, m_endFrame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::OnEndFrameChange()
{
    m_startFrame = AZ::GetMin<AZ::u32>(m_startFrame, m_endFrame);
    m_currentFrame = AZ::GetClamp<AZ::u32>(m_currentFrame, m_startFrame, m_endFrame);
    m_loopStartFrame = AZ::GetClamp<AZ::u32>(m_loopStartFrame, m_startFrame, m_endFrame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Activate()
{
    UiFlipbookAnimationBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Deactivate()
{
    UiFlipbookAnimationBus::Handler::BusDisconnect();
    UiInitializationBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Update(float deltaTime)
{
    if (m_isPlaying)
    {
        m_elapsedTime += deltaTime;

        if (m_elapsedTime >= m_frameDelay)
        {
            const bool lastFrame = m_currentFrame == m_endFrame;
            // Show current frame
            EBUS_EVENT_ID(GetEntityId(), UiImageBus, SetSpriteSheetCellIndex, m_currentFrame);

            // Determine the number of frames that has elapsed and adjust 
            // "elapsed time" to account for any additional time that has
            // passed given the current delta.
            const AZ::s32 numFramesElapsed = static_cast<AZ::s32>(m_elapsedTime / m_frameDelay);
            m_elapsedTime = m_elapsedTime - numFramesElapsed * m_frameDelay;

            // In case the loop direction is negative, we don't want to
            // subtract from the current frame if its zero.
            const AZ::s32 nextFrameNum = AZ::GetMax<AZ::s32>(0, static_cast<AZ::s32>(m_currentFrame) + numFramesElapsed * m_currentLoopDirection);
            m_currentFrame = static_cast<AZ::u32>(nextFrameNum);

            switch (m_loopType)
            {
            case LoopType::None:
                if (m_currentFrame > m_endFrame)
                {
                    // Need to set the end frame once the animation ends if it wasn't set before. 
                    // This handles the case where the frame delay is so low that causes it to end
                    // up missing the last frame due to how fast it's going.
                    if (!lastFrame) 
                    {
                        EBUS_EVENT_ID(GetEntityId(), UiImageBus, SetSpriteSheetCellIndex, m_endFrame);
                    }
                    Stop();
                }
                break;
            case LoopType::Linear:
                if (m_currentFrame > m_endFrame)
                {
                    EBUS_EVENT_ID(GetEntityId(), UiFlipbookAnimationNotificationsBus, OnLoopSequenceCompleted);
                    m_currentFrame = m_loopStartFrame;
                }
                break;
            case LoopType::PingPong:
                if (m_currentLoopDirection > 0 && m_currentFrame >= m_endFrame)
                {
                    EBUS_EVENT_ID(GetEntityId(), UiFlipbookAnimationNotificationsBus, OnLoopSequenceCompleted);
                    m_currentLoopDirection = -1;
                    m_currentFrame = m_endFrame;
                }
                else if (m_currentLoopDirection < 0 && m_currentFrame <= m_loopStartFrame)
                {
                    EBUS_EVENT_ID(GetEntityId(), UiFlipbookAnimationNotificationsBus, OnLoopSequenceCompleted);
                    m_currentLoopDirection = 1;
                    m_currentFrame = m_loopStartFrame;
                }
                break;
            default:
                break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::InGamePostActivate()
{
    if (m_isAutoPlay)
    {
        Start();
    }
    else
    {
        m_isPlaying = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Start()
{
    m_currentFrame = m_startFrame;
    m_currentLoopDirection = 1;
    m_isPlaying = true;

    // This will prime our animation for the first tick. The update loop sets
    // the current frame only once the elapsed time has passed, so we force
    // it to show the start frame on first tick by setting the elapsed time
    // to the given frame delay.
    m_elapsedTime = m_frameDelay;

    // Start the update loop
    UiUpdateBus::Handler::BusConnect(m_entity->GetId());

    // Let listeners know that we started playing
    EBUS_EVENT_ID(GetEntityId(), UiFlipbookAnimationNotificationsBus, OnAnimationStarted);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::Stop()
{
    m_isPlaying = false;
    UiUpdateBus::Handler::BusDisconnect();

    EBUS_EVENT_ID(GetEntityId(), UiFlipbookAnimationNotificationsBus, OnAnimationStopped);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetStartFrame(AZ::u32 startFrame)
{
    if (!FrameWithinRange(startFrame))
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", startFrame);
        return;
    }

    m_startFrame = startFrame;
    OnStartFrameChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetEndFrame(AZ::u32 endFrame)
{
    if (!FrameWithinRange(endFrame))
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", endFrame);
        return;
    }

    m_endFrame = endFrame;
    OnEndFrameChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetCurrentFrame(AZ::u32 currentFrame)
{
    // The current frame needs to stay between the start and end frames
    const bool validFrameValue = currentFrame >= m_startFrame && currentFrame <= m_endFrame;
    if (!validFrameValue)
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", currentFrame);
        return;
    }

    m_currentFrame = currentFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetLoopStartFrame(AZ::u32 loopStartFrame)
{
    // Ensure that loop start frame exists within start and end frame range
    const bool validFrameValue = loopStartFrame >= m_startFrame && loopStartFrame <= m_endFrame;
    if (!validFrameValue)
    {
        AZ_Warning("UI", false, "Invalid frame value given: %u", loopStartFrame);
        return;
    }

    m_loopStartFrame = loopStartFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFlipbookAnimationComponent::SetLoopType(UiFlipbookAnimationInterface::LoopType loopType)
{
    m_loopType = loopType;

    // PingPong is currently the only loop type that supports a negative loop
    // direction.
    if (m_loopType != LoopType::PingPong)
    {
        m_currentLoopDirection = 1;
    }
}
