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
#include "ApplicationAPI_linux.h"

namespace AzFramework
{
    class ApplicationLifecycleEventsHandler::Pimpl
        : public LinuxLifecycleEvents::Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Pimpl, AZ::SystemAllocator, 0);

        Pimpl()
        {
            LinuxLifecycleEvents::Bus::Handler::BusConnect();
        }

        ~Pimpl() override
        {
            LinuxLifecycleEvents::Bus::Handler::BusDisconnect();
        }
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
