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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzCore/Socket/AzSocket_fwd.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Synergy client that manages a connection with a synergy server. This should eventually
    //! be moved into a Gem, along with InputDeviceMouseSynergy and InputDeviceKeyboardSynergy.
    class SynergyClient
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(SynergyClient, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] clientScreenName Name of the synergy client screen this class implements
        //! \param[in] serverHostName Name of the synergy server host this client connects to
        SynergyClient(const char* clientScreenName, const char* serverHostName);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~SynergyClient();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the synergy client screen this class implements
        //! \return Name of the synergy client screen this class implements
        const AZStd::string& GetClientScreenName() const { return m_clientScreenName; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the synergy server host this client connects to
        //! \return Name of the synergy server host this client connects to
        const AZStd::string& GetServerHostName() const { return m_serverHostName; }

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The client connection run loop that runs in it's own thread
        void Run();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::string       m_clientScreenName;
        AZStd::string       m_serverHostName;
        AZStd::thread       m_threadHandle;
        AZStd::atomic_bool  m_threadQuit;
    public: // Temp until the implementation can be re-written
        AZSOCKET            m_socket;
        AZ::s32             m_packetOverrun;
    };
} // namespace SynergyInput
