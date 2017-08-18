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

#include "ApplicationAPI.h"
#include "ApplicationAPI_win.h"

namespace AzFramework
{
    class ApplicationLifecycleEventsHandler::Pimpl
        : public WindowsLifecycleEvents::Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Pimpl, AZ::SystemAllocator, 0);

        Pimpl()
            : m_lastEvent(ApplicationLifecycleEvents::Event::None)
        {
            WindowsLifecycleEvents::Bus::Handler::BusConnect();
        }

        ~Pimpl() override
        {
            WindowsLifecycleEvents::Bus::Handler::BusDisconnect();
        }

        void OnMinimized() override
        {
            // I have been unable to confirm whether we can assume windows
            // WM_SIZE events with wParam == SIZE_MINIMIZED will be paired
            // with a matching WM_SIZE event with wParam == SIZE_MAXIMIZED,
            // so guard against the possibility of this not being the case.
            if (m_lastEvent != ApplicationLifecycleEvents::Event::Constrain)
            {
                EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationConstrained, m_lastEvent);
                m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
            }
        }

        void OnMaximizedOrRestored() override
        {
            // I have been unable to confirm whether we can assume windows
            // WM_SIZE events with wParam == SIZE_MINIMIZED will be paired
            // with a matching WM_SIZE event with wParam == SIZE_MAXIMIZED,
            // so guard against the possibility of this not being the case.
            if (m_lastEvent != ApplicationLifecycleEvents::Event::Unconstrain)
            {
                EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationUnconstrained, m_lastEvent);
                m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
            }
        }

    private:
        ApplicationLifecycleEvents::Event m_lastEvent;
    };

    ApplicationLifecycleEventsHandler::ApplicationLifecycleEventsHandler()
        : m_pimpl(aznew Pimpl())
    {
    }

    ApplicationLifecycleEventsHandler::~ApplicationLifecycleEventsHandler()
    {
        delete m_pimpl;
    }
} // namespace AzFramework
