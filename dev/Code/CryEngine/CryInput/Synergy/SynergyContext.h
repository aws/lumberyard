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

#include <AzFramework/Input/System/InputSystemComponent.h>
#if defined(AZ_FRAMEWORK_INPUT_ENABLED)

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

#elif defined(USE_SYNERGY_INPUT)

#include <AzCore/Socket/AzSocket_fwd.h>
#include "CryThread.h"

#define MAX_CLIPBOARD_SIZE  1024

#define SYNERGY_MODIFIER_SHIFT          0x1
#define SYNERGY_MODIFIER_CTRL               0x2
#define SYNERGY_MODIFIER_LALT               0x4
#define SYNERGY_MODIFIER_WINDOWS        0x10
#define SYNERGY_MODIFIER_RALT               0x20
#define SYNERGY_MODIFIER_CAPSLOCK   0x1000
#define SYNERGY_MODIFIER_NUMLOCK    0x2000
#define SYNERGY_MODIFIER_SCROLLOCK  0x4000

class CSynergyContext
    : public CryThread<CSynergyContext>
    , public CMultiThreadRefCount
{
public:
    CSynergyContext(const char* pIdentifier, const char* pHost);
    ~CSynergyContext();
    void Run();

    bool GetKey(uint32& key, bool& bPressed, bool& bRepeat, uint32& modifier);
    bool GetMouse(uint16& x, uint16& y, uint16& wheelX, uint16& wheelY, bool& buttonL, bool& buttonM, bool& buttonR);
    const char* GetClipboard();

    const string& GetClientScreenName() const { return m_name; }
    const string& GetServerHostName() const { return m_host; }

public:
    struct KeyPress
    {
        KeyPress(uint32 _key, bool _bPressed, bool _bRepeat, uint32 _modifier) { key = _key; bPressed = _bPressed; bRepeat = _bRepeat; modifier = _modifier; }
        uint32 key;
        bool bPressed;
        bool bRepeat;
        uint32 modifier;
    };
    struct MouseEvent
    {
        uint16 x, y, wheelX, wheelY;
        bool button[3];
    };

    CryCriticalSection m_keyboardLock;
    CryCriticalSection m_mouseLock;
    CryCriticalSection m_clipboardLock;
    std::deque<KeyPress> m_keyboardQueue;
    std::deque<MouseEvent> m_mouseQueue;
    char m_clipboard[MAX_CLIPBOARD_SIZE];
    char m_clipboardThread[MAX_CLIPBOARD_SIZE];
    string m_host;
    string m_name;
    MouseEvent m_mouseState;
    AZSOCKET m_socket;
    int m_packetOverrun;

private:

    bool m_threadQuit;
};

#endif // USE_SYNERGY_INPUT
