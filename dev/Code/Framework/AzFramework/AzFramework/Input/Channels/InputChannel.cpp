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

#include <AzFramework/Input/Channels/InputChannel.h>

#include <AzFramework/Input/Devices/InputDevice.h>

#include <AzFramework/Input/Buses/Notifications/InputChannelEventNotificationBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::Snapshot::Snapshot(const InputChannel& inputChannel)
        : m_channelId(inputChannel.GetInputChannelId())
        , m_deviceId(inputChannel.GetInputDevice().GetInputDeviceId())
        , m_state(inputChannel.GetState())
        , m_value(inputChannel.GetValue())
        , m_delta(inputChannel.GetDelta())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::Snapshot::Snapshot(const InputChannelId& channelId,
                                     const InputDeviceId& deviceId,
                                     State state)
        : m_channelId(channelId)
        , m_deviceId(deviceId)
        , m_state(state)
        , m_value((state == State::Began || state == State::Updated) ? 1.0f : 0.0f)
        , m_delta((state == State::Began) ? 1.0f : ((state == State::Ended) ? -1.0f : 0.0f))
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::Snapshot::Snapshot(const InputChannelId& channelId,
                                     const InputDeviceId& deviceId,
                                     State state,
                                     float value,
                                     float delta)
        : m_channelId(channelId)
        , m_deviceId(deviceId)
        , m_state(state)
        , m_value(value)
        , m_delta(delta)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::InputChannel(const InputChannelId& inputChannelId, const InputDevice& inputDevice)
        : m_inputChannelId(inputChannelId)
        , m_inputDevice(inputDevice)
        , m_state(State::Idle)
    {
        const InputChannelRequests::BusIdType busId(m_inputChannelId,
                                                    m_inputDevice.GetInputDeviceId().GetIndex());
        InputChannelRequestBus::Handler::BusConnect(busId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::~InputChannel()
    {
        ResetState();

        const InputChannelRequests::BusIdType busId(m_inputChannelId,
                                                    m_inputDevice.GetInputDeviceId().GetIndex());
        InputChannelRequestBus::Handler::BusDisconnect(busId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel* InputChannel::GetInputChannel() const
    {
        return this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId& InputChannel::GetInputChannelId() const
    {
        return m_inputChannelId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice& InputChannel::GetInputDevice() const
    {
        return m_inputDevice;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannel::State InputChannel::GetState() const
    {
        return m_state;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateIdle() const
    {
        return m_state == State::Idle;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateBegan() const
    {
        return m_state == State::Began;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateUpdated() const
    {
        return m_state == State::Updated;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsStateEnded() const
    {
        return m_state == State::Ended;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::IsActive() const
    {
        return IsStateBegan() || IsStateUpdated();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannel::GetValue() const
    {
        return 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannel::GetDelta() const
    {
        return 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannel::GetCustomData() const
    {
        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputChannel::UpdateState(bool isChannelActive)
    {
        const State previousState = m_state;
        switch (m_state)
        {
            case State::Idle:
            {
                if (isChannelActive)
                {
                    m_state = State::Began;
                }
            }
            break;
            case State::Began:
            {
                if (isChannelActive)
                {
                    m_state = State::Updated;
                }
                else
                {
                    m_state = State::Ended;
                }
            }
            break;
            case State::Updated:
            {
                if (!isChannelActive)
                {
                    m_state = State::Ended;
                }
            }
            break;
            case State::Ended:
            {
                if (!isChannelActive)
                {
                    m_state = State::Idle;
                }
                else
                {
                    m_state = State::Began;
                }
            }
            break;
        }

        if (m_state != State::Idle)
        {
            bool hasBeenConsumed = false;
            InputChannelEventNotificationBus::Broadcast(&InputChannelEventNotifications::OnInputChannelEvent,
                                                        *this,
                                                        hasBeenConsumed);
        }

        return m_state != previousState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannel::ResetState()
    {
        // Ensure the channel transitions to the 'Ended' state if it happens to currently be active
        if (IsActive())
        {
            UpdateState(false);
        }

        // Directly return the channel to the 'Idle' state
        m_state = State::Idle;
    }
} // namespace AzFramework
