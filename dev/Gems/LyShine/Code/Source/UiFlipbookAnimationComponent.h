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
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <LyShine/Bus/UiUpdateBus.h>

#include <LyShine/Bus/UiFlipbookAnimationBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/UiComponentTypes.h>

#include "UiImageComponent.h"

//////////////////////////////////////////////////////////////////////////
//! FlipbookAnimationComponent
//! FlipbookAnimationComponent provides a way to create an animated sprite for a UI canvas
//! using sprite-sheets (via the image component)
class UiFlipbookAnimationComponent
    : public AZ::Component
    , public UiUpdateBus::Handler
    , public UiFlipbookAnimationBus::Handler
    , public UiInitializationBus::Handler
{
public: // static functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiFlipbookService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiFlipbookService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiImageService"));
    }

public: // functions

    AZ_COMPONENT(UiFlipbookAnimationComponent, LyShine::UiFlipbookAnimationComponentUuid);

    UiFlipbookAnimationComponent() = default;
    ~UiFlipbookAnimationComponent() override = default;

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void Activate() override;
    void Deactivate() override;

    //////////////////////////////////////////////////////////////////////////
    // UiUpdateBus::Handler
    void Update(float deltaTime) override;

    //////////////////////////////////////////////////////////////////////////
    // UiInitializationBus::Handler
    void InGamePostActivate() override;

    //////////////////////////////////////////////////////////////////////////
    // UiFlipbookAnimationBus::Handler
    void Start() override;
    void Stop() override;
    bool IsPlaying() override { return m_isPlaying; }
    AZ::u32 GetStartFrame() override { return m_startFrame; }
    void SetStartFrame(AZ::u32 startFrame) override;
    AZ::u32 GetEndFrame() override { return m_endFrame; }
    void SetEndFrame(AZ::u32 endFrame) override;
    AZ::u32 GetCurrentFrame() override { return m_currentFrame; }
    void SetCurrentFrame(AZ::u32 currentFrame) override;
    AZ::u32 GetLoopStartFrame() override { return m_loopStartFrame; }
    void SetLoopStartFrame(AZ::u32 loopStartFrame) override;
    UiFlipbookAnimationInterface::LoopType GetLoopType() override { return m_loopType; }
    void SetLoopType(UiFlipbookAnimationInterface::LoopType loopType) override;
    float GetFrameDelay() override { return m_frameDelay; }
    void SetFrameDelay(float frameDelay) override { m_frameDelay = AZ::GetMax<float>(0.0f, frameDelay); }
    bool GetIsAutoPlay() override { return m_isAutoPlay; }
    void SetIsAutoPlay(bool isAutoPlay) override { m_isAutoPlay = isAutoPlay; }

protected: // static functions

    //////////////////////////////////////////////////////////////////////////
    // Component descriptor
    static void Reflect(AZ::ReflectContext* context);
    //////////////////////////////////////////////////////////////////////////

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

protected: // functions

    //! Returns a string representation of the indices used to index sprite-sheet types.
    UiImageComponent::AZu32ComboBoxVec PopulateIndexStringList() const;

    //! Populates a list of enumerated frame values between the start and end frame range
    UiImageComponent::AZu32ComboBoxVec PopulateConstrainedIndexStringList() const;

    //! Total number of cells within sprite-sheet
    AZ::u32  GetMaxFrame() const;

    //! Ensures that the given frame value is valid for the associated sprite-sheet
    bool FrameWithinRange(AZ::u32 frameValue);

    //! Updates correlated frame values when the start frame value changes.
    void OnStartFrameChange();

    //! Updates correlated frame values when the end frame value changes.
    void OnEndFrameChange();

protected: // variables

    // Serialized members
    AZ::u32 m_startFrame = 0;       //!< Start frame of animation. Can be different from "loop start" frame
                                    //!< to allow animations to have an "intro" sequence.

    AZ::u32 m_endFrame = 0;         //!< Last frame of animation.

    AZ::u32 m_loopStartFrame = 0;   //!< Start frame for looped animations

    UiFlipbookAnimationInterface::LoopType m_loopType = UiFlipbookAnimationInterface::LoopType::None;

    float   m_frameDelay = 0.0f;    //!< Number of seconds to wait before showing the next frame

    bool    m_isAutoPlay = true;    //!< Whether the animation should automatically start playing.

    // Non-serialized members
    AZ::u32 m_currentFrame = 0;     //!< Current sprite-sheet frame/index displayed

    float   m_elapsedTime;          //!< Used to determine passage of time for handling frame delay

    bool    m_isPlaying;            //!< True if the animation is playing, false otherwise

    AZ::s32 m_currentLoopDirection; //!< Used for PingPong loop direction (positive/negative)
};
