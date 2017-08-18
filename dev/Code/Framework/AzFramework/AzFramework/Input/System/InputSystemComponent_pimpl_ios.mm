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
#include <AzFramework/Input/Buses/Requests/RawInputRequestBus_ios.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for the input system component on ios
    class InputSystemComponentIos : public InputSystemComponent::Implementation
                                  , public RawInputRequestBusIos::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputSystemComponentIos, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputSystem Reference to the input system component
        InputSystemComponentIos(InputSystemComponent& inputSystem);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputSystemComponentIos() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputRequestBusIos::PumpRawEventLoop
        void PumpRawEventLoop() override;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponent::Implementation* InputSystemComponent::Implementation::Create(
        InputSystemComponent& inputSystem)
    {
        return aznew InputSystemComponentIos(inputSystem);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponentIos::InputSystemComponentIos(InputSystemComponent& inputSystem)
        : InputSystemComponent::Implementation(inputSystem)
    {
        RawInputRequestBusIos::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponentIos::~InputSystemComponentIos()
    {
        RawInputRequestBusIos::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponentIos::PumpRawEventLoop()
    {
        SInt32 result;
        do
        {
            result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
        }
        while (result == kCFRunLoopRunHandledSource);
    }
} // namespace AzFramework
