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

#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Utils/ProcessRawInputEventQueues.h>

#include <AzCore/Debug/Trace.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId InputDeviceTouch::Id("touch");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId InputDeviceTouch::Touch::Index0("touch_index_0");
    const InputChannelId InputDeviceTouch::Touch::Index1("touch_index_1");
    const InputChannelId InputDeviceTouch::Touch::Index2("touch_index_2");
    const InputChannelId InputDeviceTouch::Touch::Index3("touch_index_3");
    const InputChannelId InputDeviceTouch::Touch::Index4("touch_index_4");
    const InputChannelId InputDeviceTouch::Touch::Index5("touch_index_5");
    const InputChannelId InputDeviceTouch::Touch::Index6("touch_index_6");
    const InputChannelId InputDeviceTouch::Touch::Index7("touch_index_7");
    const InputChannelId InputDeviceTouch::Touch::Index8("touch_index_8");
    const InputChannelId InputDeviceTouch::Touch::Index9("touch_index_9");
    const AZStd::array<InputChannelId, 10> InputDeviceTouch::Touch::All =
    {{
        Index0,
        Index1,
        Index2,
        Index3,
        Index4,
        Index5,
        Index6,
        Index7,
        Index8,
        Index9
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::InputDeviceTouch()
        : InputDevice(Id)
        , m_allChannelsById()
        , m_touchChannelsById()
        , m_pimpl(nullptr)
        , m_implementationRequestHandler(*this)
    {
        // Create all touch input channels
        for (AZ::u32 i = 0; i < Touch::All.size(); ++i)
        {
            const InputChannelId& channelId = Touch::All[i];
            InputChannelAnalogWithPosition2D* channel = aznew InputChannelAnalogWithPosition2D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_touchChannelsById[channelId] = channel;
        }

        // Create the platform specific implementation
        m_pimpl.reset(Implementation::Create(*this));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::~InputDeviceTouch()
    {
        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy all touch input channels
        for (const auto& channelById : m_touchChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceTouch::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouch::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceTouch::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation::Implementation(InputDeviceTouch& inputDevice)
        : m_inputDevice(inputDevice)
        , m_rawTouchEventQueuesById()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceTouch::Implementation::RawTouchEvent::RawTouchEvent(float normalizedX,
                                                                   float normalizedY,
                                                                   float pressure,
                                                                   AZ::u32 index,
                                                                   State state)
        : InputChannelAnalogWithPosition2D::RawInputEvent(normalizedX, normalizedY, pressure)
        , m_index(index)
        , m_state(state)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::Implementation::QueueRawTouchEvent(const RawTouchEvent& rawTouchEvent)
    {
        if (rawTouchEvent.m_index >= Touch::All.size())
        {
            AZ_Warning("InputDeviceTouch", false,
                       "Raw touch has index: %d that is >= max: %zd",
                       rawTouchEvent.m_index, Touch::All.size());
            return;
        }

        auto& rawEventQueue = m_rawTouchEventQueuesById[Touch::All[rawTouchEvent.m_index]];
        if (rawEventQueue.empty() || rawEventQueue.back().m_state != rawTouchEvent.m_state)
        {
            // No raw touches for this index have been queued this frame,
            // or the last raw touch queued was in a different state.
            rawEventQueue.push_back(rawTouchEvent);
        }
        else
        {
            // Because touches are sampled at a rate (~30-60fps) independent of the simulation
            // (which is necessary for the controls to remain responsive), when the simulation
            // frame rate drops we end up receiving multiple touch move events each frame, for
            // the same finger, which (especially with multi-touch) can generate enough events
            // to cause the simulation frame rate to drop even further. To combat this we will
            // combine multiple touch events for the same finger if they are in the same state.
            //
            // For example, the following sequence of events with the same index in one frame:
            // - Began, Moved, Moved, Ended, Began, Moved, Moved, Moved
            //
            // Will be collapsed as follows:
            // - Began, Moved, Ended, Began, Moved
            //
            // This seems to maintain a good balance between responsiveness vs accuracy. While
            // it should not (in theory) be possible to receive multiple Began or Ended events
            // in succession, if it happens in practice for whatever reason this is still safe.
            rawEventQueue.back() = rawTouchEvent;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::Implementation::ProcessRawEventQueues()
    {
        // Process all raw input events that were queued since the last call to this function.
        ProcessRawInputEventQueues(m_rawTouchEventQueuesById, m_inputDevice.m_touchChannelsById);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceTouch::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }
} // namespace AzFramework
