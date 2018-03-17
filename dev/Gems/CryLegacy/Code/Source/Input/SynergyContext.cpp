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

#include "CryLegacy_precompiled.h"

#include "IRenderer.h"
#include "SynergyContext.h"

#include <AzCore/Socket/AzSocket.h>

#include "RawInputNotificationBus_synergy.h"
#define CSynergyContext SynergyClient // Temp until the implementation can be re-written
using namespace SynergyInput;

enum ArgType
{
    ARG_END = 0,
    ARG_UINT8,
    ARG_UINT16,
    ARG_UINT32
};

#define MAX_ARGS 16

struct Stream
{
    explicit Stream(int size)
    {
        buffer = (uint8*)malloc(size);
        end = data = buffer;
        bufferSize = size;
        packet = NULL;
    }

    ~Stream()
    {
        free(buffer);
    }

    uint8* data;
    uint8* end;
    uint8* buffer;
    uint8* packet;
    int bufferSize;

    void Rewind() { data = buffer; }
    int GetBufferSize() { return bufferSize; }

    char* GetBuffer() { return (char*)buffer; }
    char* GetData() { return (char*)data; }

    void SetLength(int len) { end = data + len; }
    int GetLength() { return (int)(end - data); }

    int ReadU32() { int ret = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]; data += 4; return ret; }
    int ReadU16() { int ret = (data[0] << 8) | data[1]; data += 2; return ret; }
    int ReadU8() { int ret = data[0]; data += 1; return ret; }
    void Eat(int len) { data += len; }

    void InsertString(const char* str) { int len = strlen(str); memcpy(end, str, len); end += len; }
    void InsertU32(int a) { end[0] = a >> 24; end[1] = a >> 16; end[2] = a >> 8; end[3] = a; end += 4; }
    void InsertU16(int a) { end[0] = a >> 8; end[1] = a; end += 2; }
    void InsertU8(int a) { end[0] = a; end += 1; }
    void OpenPacket() { packet = end; end += 4; }
    void ClosePacket() { int len = GetLength() - sizeof(uint32); packet[0] = len >> 24; packet[1] = len >> 16; packet[2] = len >> 8; packet[3] = len; packet = NULL; }
};

typedef bool (* packetCallback)(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft);

struct Packet
{
    const char* pattern;
    ArgType args[MAX_ARGS + 1];
    packetCallback callback;
};

static bool synergyConnectFunc(CSynergyContext* pContext)
{
    if (AZ::AzSock::IsAzSocketValid(pContext->m_socket))
    {
        AZ::AzSock::CloseSocket(pContext->m_socket);
    }
    pContext->m_socket = AZ::AzSock::Socket();
    if (AZ::AzSock::IsAzSocketValid(pContext->m_socket))
    {
        AZ::AzSock::AzSocketAddress sa;
        if (sa.SetAddress(pContext->GetServerHostName().c_str(), 24800))
        {
            int result = AZ::AzSock::Connect(pContext->m_socket, sa);
            if (!AZ::AzSock::SocketErrorOccured(result))
            {
                return true;
            }
        }
        AZ::AzSock::CloseSocket(pContext->m_socket);
        pContext->m_socket = AZ_SOCKET_INVALID;
    }
    return false;
}

static bool synergySendFunc(CSynergyContext* pContext, const char* buffer, int length)
{
    int ret = AZ::AzSock::Send(pContext->m_socket, buffer, length, 0);
    return (ret == length) ? true : false;
}

static bool synergyReceiveFunc(CSynergyContext* pContext, char* buffer, int maxLength, int* outLength)
{
    int ret = AZ::AzSock::Recv(pContext->m_socket, buffer, maxLength, 0);
    *outLength = ret;
    return (ret > 0) ? true : false;
}

static bool synergyPacket(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    Stream s(256);
    s.OpenPacket();
    s.InsertString("Synergy");
    s.InsertU16(1);
    s.InsertU16(4);
    s.InsertU32(pContext->GetClientScreenName().length());
    s.InsertString(pContext->GetClientScreenName().c_str());
    s.ClosePacket();
    return synergySendFunc(pContext, s.GetBuffer(), s.GetLength());
}

static bool synergyQueryInfo(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    Stream s(256);
    s.OpenPacket();
    s.InsertString("DINF");
    s.InsertU16(0);
    s.InsertU16(0);
    if (gEnv->pRenderer)
    {
        s.InsertU16(gEnv->pRenderer->GetWidth());
        s.InsertU16(gEnv->pRenderer->GetHeight());
    }
    else
    {
        s.InsertU16(1920);
        s.InsertU16(1080);
    }
    s.InsertU16(0);
    s.InsertU16(0);
    s.InsertU16(0);
    s.ClosePacket();
    return synergySendFunc(pContext, s.GetBuffer(), s.GetLength());
}

static bool synergyKeepAlive(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    Stream s(256);
    s.OpenPacket();
    s.InsertString("CALV");
    s.ClosePacket();
    return synergySendFunc(pContext, s.GetBuffer(), s.GetLength());
}

static bool synergyEnterScreen(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
#ifdef SUPPORT_HW_MOUSE_CURSOR
    if (gEnv->pRenderer && gEnv->pRenderer->GetIHWMouseCursor())
    {
        gEnv->pRenderer->GetIHWMouseCursor()->SetPosition(pArgs[0], pArgs[1]);
        gEnv->pRenderer->GetIHWMouseCursor()->Show();
    }
#endif
    if (gEnv->pRenderer)
    {
        const float normalizedMouseX = static_cast<float>(pArgs[0]) / static_cast<float>(gEnv->pRenderer->GetWidth());
        const float normalizedMouseY = static_cast<float>(pArgs[1]) / static_cast<float>(gEnv->pRenderer->GetHeight());
        RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawMousePositionEvent,
                                                  normalizedMouseX,
                                                  normalizedMouseY);
    }
    return true;
}

static bool synergyExitScreen(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
#ifdef SUPPORT_HW_MOUSE_CURSOR
    if (gEnv->pRenderer && gEnv->pRenderer->GetIHWMouseCursor())
    {
        gEnv->pRenderer->GetIHWMouseCursor()->Hide();
    }
#endif
    return true;
}

static bool synergyMouseMove(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
#ifdef SUPPORT_HW_MOUSE_CURSOR
    if (gEnv->pRenderer && gEnv->pRenderer->GetIHWMouseCursor())
    {
        gEnv->pRenderer->GetIHWMouseCursor()->SetPosition(pArgs[0], pArgs[1]);
    }
#endif
    if (gEnv->pRenderer)
    {
        const float normalizedMouseX = static_cast<float>(pArgs[0]) / static_cast<float>(gEnv->pRenderer->GetWidth());
        const float normalizedMouseY = static_cast<float>(pArgs[1]) / static_cast<float>(gEnv->pRenderer->GetHeight());
        RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawMousePositionEvent,
                                                  normalizedMouseX,
                                                  normalizedMouseY);
    }
    return true;
}

static bool synergyMouseButtonDown(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    const uint32_t buttonIndex = pArgs[0];
    RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawMouseButtonDownEvent, buttonIndex);
    return true;
}

static bool synergyMouseButtonUp(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    const uint32_t buttonIndex = pArgs[0];
    RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawMouseButtonUpEvent, buttonIndex);
    return true;
}

static bool synergyKeyboardDown(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    const uint32_t scanCode = pArgs[2];
    const ModifierMask activeModifiers = static_cast<ModifierMask>(pArgs[1]);
    RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawKeyboardKeyDownEvent, scanCode, activeModifiers);
    return true;
}

static bool synergyKeyboardUp(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    const uint32_t scanCode = pArgs[2];
    const ModifierMask activeModifiers = static_cast<ModifierMask>(pArgs[1]);
    RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawKeyboardKeyUpEvent, scanCode, activeModifiers);
    return true;
}

static bool synergyKeyboardRepeat(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    const uint32_t scanCode = pArgs[2];
    const ModifierMask activeModifiers = static_cast<ModifierMask>(pArgs[1]);
    RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawKeyboardKeyRepeatEvent, scanCode, activeModifiers);
    return true;
}

static bool synergyClipboard(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    for (int i = 0; i < pArgs[3]; i++)
    {
        int format = pStream->ReadU32();
        int size = pStream->ReadU32();
        if (format == 0) // Is text
        {
            char* clipboardContents = new char[size];
            memcpy(clipboardContents, pStream->GetData(), size);
            clipboardContents[size] = '\0';
            RawInputNotificationBusSynergy::Broadcast(&RawInputNotificationsSynergy::OnRawClipboardEvent, clipboardContents);
            delete[] clipboardContents;
        }
        pStream->Eat(size);
    }
    return true;
}

static bool synergyBye(CSynergyContext* pContext, int* pArgs, Stream* pStream, int streamLeft)
{
    CryLog("SYNERGY: Server said bye. Disconnecting\n");
    return false;
}

static Packet s_packets[] = {
    { "Synergy", { ARG_UINT16, ARG_UINT16 }, synergyPacket },
    { "QINF", {}, synergyQueryInfo },
    { "CALV", {}, synergyKeepAlive },
    { "CINN", { ARG_UINT16, ARG_UINT16, ARG_UINT32, ARG_UINT16 }, synergyEnterScreen },
    { "COUT", { }, synergyExitScreen },
    { "CBYE", { }, synergyBye },
    { "DMMV", { ARG_UINT16, ARG_UINT16 }, synergyMouseMove },
    { "DMDN", { ARG_UINT8 }, synergyMouseButtonDown },
    { "DMUP", { ARG_UINT8 }, synergyMouseButtonUp },
    { "DKDN", { ARG_UINT16, ARG_UINT16, ARG_UINT16 }, synergyKeyboardDown },
    { "DKUP", { ARG_UINT16, ARG_UINT16, ARG_UINT16 }, synergyKeyboardUp },
    { "DKRP", { ARG_UINT16, ARG_UINT16, ARG_UINT16, ARG_UINT16 }, synergyKeyboardRepeat },
    { "DCLP", { ARG_UINT8, ARG_UINT32, ARG_UINT32, ARG_UINT32 }, synergyClipboard }
};

static bool ProcessPackets(CSynergyContext* pContext, Stream* s)
{
    while (s->data < s->end)
    {
        int packetLen = s->ReadU32();
        const char* packetStart = s->GetData();
        int i;
        if (packetLen > s->GetLength())
        {
            CryLogAlways("SYNERGY: Packet overruns buffer. Probably lots of data on clipboard? (Size:%d LeftInBuffer:%d)\n", packetLen, s->GetLength());
            pContext->m_packetOverrun = packetLen - s->GetLength();
            return true;
        }
        for (i = 0; i < sizeof(s_packets) / sizeof(s_packets[0]); i++)
        {
            int len = strlen(s_packets[i].pattern);
            if (packetLen >= len && memcmp(s->GetData(), s_packets[i].pattern, len) == 0)
            {
                bool bDone = false;
                int numArgs = 0;
                int args[MAX_ARGS];
                s->Eat(len);
                while (!bDone)
                {
                    switch (s_packets[i].args[numArgs])
                    {
                    case ARG_UINT8:
                        args[numArgs++] = s->ReadU8();
                        break;
                    case ARG_UINT16:
                        args[numArgs++] = s->ReadU16();
                        break;
                    case ARG_UINT32:
                        args[numArgs++] = s->ReadU32();
                        break;
                    case ARG_END:
                        bDone = true;
                        break;
                    }
                }
                if (s_packets[i].callback)
                {
                    if (!s_packets[i].callback(pContext, args, s, packetLen - (int)(s->GetData() - packetStart)))
                    {
                        return false;
                    }
                }
                s->Eat(packetLen - (int)(s->GetData() - packetStart));
                break;
            }
        }
        if (i == sizeof(s_packets) / sizeof(s_packets[0]))
        {
            char* data = s->GetData() + packetLen;
            //CryLog("SYNERGY: Can't understand packet:%s Length:%d\n", s->GetData(), packetLen);
            s->Eat(packetLen);
        }
    }
    return true;
}

CSynergyContext::CSynergyContext(const char* clientScreenName, const char* serverHostName)
    : m_clientScreenName(clientScreenName)
    , m_serverHostName(serverHostName)
    , m_socket(AZ_SOCKET_INVALID)
    , m_packetOverrun(0)
    , m_threadHandle()
    , m_threadQuit(false)
{
    AZStd::thread_desc threadDesc;
    threadDesc.m_name = "Synergy Thread";
    m_threadHandle = AZStd::thread(AZStd::bind(&CSynergyContext::Run, this), &threadDesc);
}

CSynergyContext::~CSynergyContext()
{
    if (AZ::AzSock::IsAzSocketValid(m_socket))
    {
        AZ::AzSock::CloseSocket(m_socket);
    }
    m_threadQuit = true;
    m_threadHandle.join();
}

void CSynergyContext::Run()
{
    Stream s(4 * 1024);
    bool bReconnect = true;
    while (!m_threadQuit)
    {
        if (bReconnect)
        {
            if (synergyConnectFunc(this))
            {
                bReconnect = false;
            }
            else
            {
#ifdef SUPPORT_HW_MOUSE_CURSOR
                if (gEnv->pRenderer && gEnv->pRenderer->GetIHWMouseCursor())
                {
                    gEnv->pRenderer->GetIHWMouseCursor()->Hide();
                }
#endif
                Sleep(500);
            }
        }
        else
        {
            int outLen = 0;
            if (synergyReceiveFunc(this, s.GetBuffer(), s.GetBufferSize(), &outLen))
            {
                s.Rewind();
                s.SetLength(outLen);
                if (!ProcessPackets(this, &s))
                {
                    CryLog("SYNERGY: Packet processing failed. Reconnecting.\n");
                    bReconnect = true;
                }
                if (m_packetOverrun > 0)
                {
                    while (m_packetOverrun > 0)
                    {
                        if (!synergyReceiveFunc(this, s.GetBuffer(), MIN(m_packetOverrun, s.GetBufferSize()), &outLen))
                        {
                            bReconnect = true;
                            break;
                        }
                        m_packetOverrun -= outLen;
                    }
                }
            }
            else
            {
                CryLog("SYNERGY: Receive failed. Reconnecting.\n");
                bReconnect = true;
            }
        }
    }
}
