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

#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/Input/Buses/Requests/RawInputRequestBus_darwin.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_darwin.h>

#include <AppKit/AppKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for the input system component on osx
    class InputSystemComponentOsx : public InputSystemComponent::Implementation
                                  , public RawInputRequestBusOsx::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputSystemComponentOsx, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputSystem Reference to the input system component
        InputSystemComponentOsx(InputSystemComponent& inputSystem);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputSystemComponentOsx() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputRequestsOsx::PumpRawEventLoop
        void PumpRawEventLoop() override;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponent::Implementation* InputSystemComponent::Implementation::Create(
        InputSystemComponent& inputSystem)
    {
        return aznew InputSystemComponentOsx(inputSystem);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponentOsx::InputSystemComponentOsx(InputSystemComponent& inputSystem)
        : InputSystemComponent::Implementation(inputSystem)
    {
        RawInputRequestBusOsx::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponentOsx::~InputSystemComponentOsx()
    {
        RawInputRequestBusOsx::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponentOsx::PumpRawEventLoop()
    {
        NSAutoreleasePool* autoreleasePool = [[NSAutoreleasePool alloc] init];
        do
        {
            NSEvent* event = [NSApp nextEventMatchingMask: NSAnyEventMask
                                    untilDate: [NSDate distantPast]
                                    inMode: NSDefaultRunLoopMode
                                    dequeue: YES];
            if (event != nil)
            {
                RawInputNotificationBusOsx::Broadcast(&RawInputNotificationsOsx::OnRawInputEvent, event);
                [NSApp sendEvent: event];
            }
            else
            {
                break;
            }
        } while (true);

        [autoreleasePool release];
    }
} // namespace AzFramework
