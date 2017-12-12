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

#include <stdint.h>
#include <IRenderer.h>
#include <Cry_Vector2.h>

#include <AzFramework/Input/Buses/Notifications/InputChannelEventNotificationBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class IRecognizer : public AzFramework::InputChannelEventNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventNotifications::OnInputChannelEvent
        void OnInputChannelEvent(const AzFramework::InputChannel& inputChannel,
                                 bool& o_hasBeenConsumed) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a mouse button or finger on a touch screen is initially
        //! pressed, unless the input event was consumed by a higher priority gesture recognizer.
        //! \param[in] screenPositionPixels The screen position (in pixels) of the input event.
        //! \param[in] pointerIndex The pointer index of the input event.
        //! \return True to consume the underlying input event (preventing it from being sent on
        //! to other lower-priority gesture recognizers or input listeners), or false otherwise.
        virtual bool OnPressedEvent(const Vec2& screenPositionPixels, uint32_t pointerIndex) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified each frame a mouse button or finger on a touch screen remains
        //! pressed, unless the input event was consumed by a higher priority gesture recognizer.
        //! \param[in] screenPositionPixels The screen position (in pixels) of the input event.
        //! \param[in] pointerIndex The pointer index of the input event.
        //! \return True to consume the underlying input event (preventing it from being sent on
        //! to other lower-priority gesture recognizers or input listeners), or false otherwise.
        virtual bool OnDownEvent(const Vec2& screenPositionPixels, uint32_t pointerIndex) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Override to be notified when a pressed mouse button or finger on a touch screen becomes
        //! released, unless the input event was consumed by a higher priority gesture recognizer.
        //! \param[in] screenPositionPixels The screen position (in pixels) of the input event.
        //! \param[in] pointerIndex The pointer index of the input event.
        //! \return True to consume the underlying input event (preventing it from being sent on
        //! to other lower-priority gesture recognizers or input listeners), or false otherwise.
        virtual bool OnReleasedEvent(const Vec2& screenPositionPixels, uint32_t pointerIndex) = 0;

    protected:

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get the gesture pointer index associated with an input channel.
        //! \param[in] inputChannel The input channel to find the pointer index for.
        //! \return The gesture pointer index of the input channel, or INVALID_GESTURE_POINTER_INDEX.
        uint32_t GetGesturePointerIndex(const AzFramework::InputChannel& inputChannel);
        const uint32_t INVALID_GESTURE_POINTER_INDEX = static_cast<uint32_t>(-1);

        // Using Vec2 directly as a member of derived recognizer classes results in
        // linker warnings because Vec2 doesn't have dll import/export specifiers.
        struct ScreenPosition
        {
            ScreenPosition()
                : x(0.0f)
                , y(0.0f) {}
            ScreenPosition(const Vec2& vec2)
                : x(vec2.x)
                , y(vec2.y) {}
            operator Vec2() const
            {
                return Vec2(x, y);
            }

            float x;
            float y;
        };
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void IRecognizer::OnInputChannelEvent(const AzFramework::InputChannel& inputChannel,
                                                 bool& o_hasBeenConsumed)
    {
        if (o_hasBeenConsumed)
        {
            return;
        }

        const AzFramework::InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
        if (!positionData2D)
        {
            // This input event is not associated with a position, so it is irrelevant for gestures.
            return;
        }

        const uint32_t pointerIndex = GetGesturePointerIndex(inputChannel);
        if (pointerIndex == INVALID_GESTURE_POINTER_INDEX)
        {
            // This input event is not associated with a pointer index, so it is irrelevant for gestures.
            return;
        }

        const float posX = positionData2D->m_normalizedPosition.GetX() * static_cast<float>(gEnv->pRenderer->GetWidth());
        const float posY = positionData2D->m_normalizedPosition.GetY() * static_cast<float>(gEnv->pRenderer->GetHeight());
        const Vec2 eventScreenPositionPixels(posX, posY);
        if (inputChannel.IsStateBegan())
        {
            o_hasBeenConsumed = OnPressedEvent(eventScreenPositionPixels, pointerIndex);
        }
        else if (inputChannel.IsStateUpdated())
        {
            o_hasBeenConsumed = OnDownEvent(eventScreenPositionPixels, pointerIndex);
        }
        else if (inputChannel.IsStateEnded())
        {
            o_hasBeenConsumed = OnReleasedEvent(eventScreenPositionPixels, pointerIndex);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline uint32_t IRecognizer::GetGesturePointerIndex(const AzFramework::InputChannel& inputChannel)
    {
        const auto& mouseButtonIt = AZStd::find(AzFramework::InputDeviceMouse::Button::All.cbegin(),
                                                AzFramework::InputDeviceMouse::Button::All.cend(),
                                                inputChannel.GetInputChannelId());
        if (mouseButtonIt != AzFramework::InputDeviceMouse::Button::All.cend())
        {
            return static_cast<uint32_t>(mouseButtonIt - AzFramework::InputDeviceMouse::Button::All.cbegin());
        }

        const auto& touchIndexIt = AZStd::find(AzFramework::InputDeviceTouch::Touch::All.cbegin(),
                                               AzFramework::InputDeviceTouch::Touch::All.cend(),
                                                inputChannel.GetInputChannelId());
        if (touchIndexIt != AzFramework::InputDeviceTouch::Touch::All.cend())
        {
            return static_cast<uint32_t>(touchIndexIt - AzFramework::InputDeviceTouch::Touch::All.cbegin());
        }

        return INVALID_GESTURE_POINTER_INDEX;
    }
}
