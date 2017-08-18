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

#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/typetraits/is_base_of.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputDevice;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all input channels that represent the current state of a single input source.
    //! Derived classes should provide additional functions that allow their parent input devices to
    //! update the state and value(s) of the input channel as raw input is received from the system,
    //! and they can (optionally) override the virtual GetCustomData function to return custom data.
    class InputChannel : public InputChannelRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! State of the input channel (not all channels will go through all states)
        enum class State
        {
            Idle,    //!< Examples: inactive or idle, not currently emitting input events
            Began,   //!< Examples: button pressed, trigger engaged, thumb-stick exits deadzone
            Updated, //!< Examples: button held, trigger changed, thumb-stick is outside deadzone
            Ended    //!< Examples: button released, trigger released, thumb-stick enters deadzone
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Base struct from which to derive all custom input data
        struct CustomData
        {
            AZ_RTTI(CustomData, "{887E38BB-64AF-4F4E-A1AE-C1B02371F9EC}");
            virtual ~CustomData() = default;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Custom data struct for input channels associated with a 2D position
        struct PositionData2D : public CustomData
        {
            AZ_RTTI(PositionData2D, "{354437EC-6BFD-41D4-A0F2-7740018D3589}", CustomData);
            virtual ~PositionData2D() = default;

            AZ::Vector2 m_normalizedPosition = AZ::Vector2(0.5f, 0.5f);
            AZ::Vector2 m_normalizedPositionDelta = AZ::Vector2::CreateZero();
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannel, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input channel
        //! \param[in] inputDevice Input device that owns the input channel
        InputChannel(const InputChannelId& inputChannelId,
                     const InputDevice& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputChannel);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputChannel() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::GetInputChannel
        const InputChannel* GetInputChannel() const final;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input channel's id
        //! \return Id of the input channel
        const InputChannelId& GetInputChannelId() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input channel's device
        //! \return Input device that owns the input channel
        const InputDevice& GetInputDevice() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Direct access to the input channel's current state
        //! \return The current state of the input channel
        State GetState() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Indirect access to the input channel's current state
        //! \return True if the input channel is currently in the specified state, false otherwise
        bool IsStateIdle() const;
        bool IsStateBegan() const;
        bool IsStateUpdated() const;
        bool IsStateEnded() const;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Indirect access to the input channel's current state
        //! \return True if the channel is in the 'Began' or 'Updated' states, false otherwise
        bool IsActive() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional float value of the input channel
        //! \return The current one dimensional float value of the input channel
        virtual float GetValue() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the one dimensional float delta of the input channel
        //! \return The current one dimensional float delta of the input channel
        virtual float GetDelta() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to any custom data provided by the input channel
        //! \return Pointer to the custom data if it exists, nullptr otherwise
        virtual const CustomData* GetCustomData() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to any custom data associated with the input channel
        //! \return Pointer to the custom data if it exists and is of type T, nullptr othewise
        template<class T> const T* GetCustomData() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Update the channel's state based on whether it is active/engaged or inactive/idle, which
        //! will broadcast an input event if the channel is left in a non-idle state. Should only be
        //! called a maximum of once per channel per frame from InputDeviceRequests::TickInputDevice
        //! to ensure input channels broadcast no more than one event each frame (at the same time).
        //! \param[in] isChannelActive Whether the input channel is currently active/engaged
        //! \return Whether the update resulted in a state transition (was m_state changed)
        virtual bool UpdateState(bool isChannelActive);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::ResetState
        void ResetState() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        const InputChannelId    m_inputChannelId;   //!< Id of the input channel
        const InputDevice&      m_inputDevice;      //!< Input device that owns the input channel
        State                   m_state;            //!< Current state of the input channel
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Inline Implementation
    template<class T>
    inline const T* InputChannel::GetCustomData() const
    {
        AZ_STATIC_ASSERT((AZStd::is_base_of<CustomData, T>::value),
            "Custom input data must inherit from InputChannel::CustomData");

        const CustomData* customData = GetCustomData();
        return customData ? azdynamic_cast<const T*>(customData) : nullptr;
    }
} // namespace AzFramework
