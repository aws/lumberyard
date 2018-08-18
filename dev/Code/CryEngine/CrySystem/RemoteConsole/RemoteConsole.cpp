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

#include "StdAfx.h"
#include "RemoteConsole.h"

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/StringFunc/StringFunc.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define REMOTECONSOLE_CPP_SECTION_1 1
#define REMOTECONSOLE_CPP_SECTION_2 2
#endif

#ifdef USE_REMOTE_CONSOLE
#include "CryThread.h"
#include <IGame.h>
#include <IGameFramework.h>
#include <ILevelSystem.h>

#if 0 // currently no stroboscope support
#include "Stroboscope/Stroboscope.h"
#include "ThreadInfo.h"
#endif

#define DEFAULT_PORT 4600
#define DEFAULT_BUFFER 4096
#define MAX_BIND_ATTEMPTS 8 // it will try to connect using ports from DEFAULT_PORT to DEFAULT_PORT + MAX_BIND_ATTEMPTS - 1
#define SERVER_THREAD_NAME "RemoteConsoleServer"
#define CLIENT_THREAD_NAME "RemoteConsoleClient"
#endif

#include <AzCore/Socket/AzSocket.h>


bool RCON_IsRemoteAllowedToConnect(const AZ::AzSock::AzSocketAddress& connectee)
{
    if ((!gEnv) || (!gEnv->pConsole))
    {
        CryLog("Cannot allow incoming connection for remote console, because we do not yet have a console or an environment.");
        return false;
    }
    ICVar* remoteConsoleAllowedHostList = gEnv->pConsole->GetCVar("log_RemoteConsoleAllowedAddresses");
    if (!remoteConsoleAllowedHostList)
    {
        CryLog("Cannot allow incoming connection for remote console, because there is no registered log_RemoteConsoleAllowedAddresses console variable.");
        return false;
    }

    const char* value = remoteConsoleAllowedHostList->GetString();
    // the default or empty string indicates localhost.
    if (!value)
    {
        value = "";
    }

    AZStd::vector<AZStd::string> addresses;
    AzFramework::StringFunc::Tokenize(value, addresses, ',');

    if (addresses.empty())
    {
        addresses.push_back("127.0.0.1");
    }

    AZ::AzSock::AzSocketAddress testAddress;
    for (const AZStd::string& address : addresses)
    {
        // test the whitelisted addresses with connectee's port to see if we have a match
        testAddress.SetAddress(address.c_str(), connectee.GetAddrPort());

        if (testAddress == connectee)
        {
            // its an exact match.
            gEnv->pLog->LogToConsole("Remote console connected from ip %s (matches: %s)", connectee.GetAddress().c_str(), address.c_str());
            return true;
        }
    }

    if (gEnv->pLog)
    {
        gEnv->pLog->LogToConsole("An attempt to connect to remote console from ip %s failed because it is not on the whitelist.", connectee.GetAddress().c_str());
        gEnv->pLog->LogToConsole("Add to the whitelist using the CVAR log_RemoteConsoleAllowedAddresses (comma separated IPs or hostnames)");
        gEnv->pLog->LogToConsole("Example:  log_RemoteConsoleAllowedAddresses localhost,joescomputer");
    }
    return false; // return false by default, so you MUST pass an above check for you to be allowed in.
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
CRemoteConsole::CRemoteConsole()
    : m_listener(1)
    , m_running(false)
#ifdef USE_REMOTE_CONSOLE
    , m_pServer(new SRemoteServer())
    , m_pLogEnableRemoteConsole(nullptr)
    , m_remoteConsoleAllowedHostList(nullptr)
#endif
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
CRemoteConsole::~CRemoteConsole()
{
#ifdef USE_REMOTE_CONSOLE
    Stop();
    delete m_pServer;
    m_pServer = nullptr;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::RegisterConsoleVariables()
{
#ifdef USE_REMOTE_CONSOLE
    m_pLogEnableRemoteConsole = REGISTER_INT("log_EnableRemoteConsole", 1, VF_DUMPTODISK, "enables/disables the remote console");
    m_remoteConsoleAllowedHostList = REGISTER_STRING("log_RemoteConsoleAllowedAddresses", "", VF_DUMPTODISK, "COMMA separated list of allowed hosts or IP addresses which can connect");
    m_remoteConsolePort = REGISTER_INT("log_RemoteConsolePort", DEFAULT_PORT, VF_DUMPTODISK, "Base port (4600 for example) for remote console to listen on.  It will start there and continue upwards until an unused one is found.");
    m_lastPortValue = 0;
    
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::UnregisterConsoleVariables()
{
#ifdef USE_REMOTE_CONSOLE
    m_pLogEnableRemoteConsole = nullptr;
    m_remoteConsoleAllowedHostList = nullptr;
    m_remoteConsolePort = nullptr;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::Start()
{
#ifdef USE_REMOTE_CONSOLE
    if (!IsStarted())
    {
        m_pServer->StartServer();
        m_running = true;
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::Stop()
{
#ifdef USE_REMOTE_CONSOLE
    // make sure we don't stop if we never started the remote console in the first place
    if (IsStarted())
    {
        m_running = false;
        m_pServer->StopServer();
        m_pServer->WaitForThread();
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool CRemoteConsole::IsStarted() const
{
    return m_running;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogMessage(const char* log)
{
#ifdef USE_REMOTE_CONSOLE
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogMessage>(log);
    m_pServer->AddEvent(pEvent);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogWarning(const char* log)
{
#ifdef USE_REMOTE_CONSOLE
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogWarning>(log);
    m_pServer->AddEvent(pEvent);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::AddLogError(const char* log)
{
#ifdef USE_REMOTE_CONSOLE
    if (!IsStarted())
    {
        return;
    }

    IRemoteEvent* pEvent = new SStringEvent<eCET_LogError>(log);
    m_pServer->AddEvent(pEvent);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::Update()
{
#ifdef USE_REMOTE_CONSOLE
    if (m_pLogEnableRemoteConsole)
    {
        // we disable the remote console in the editor, since there is no reason to remote into it and we don't want it eating up that port
        // number anyway and preventing the game from using it.
        bool isEditor = gEnv && gEnv->IsEditor();
        bool isEnabled = (!isEditor) && (m_pLogEnableRemoteConsole->GetIVal());
        bool isStarted = IsStarted();

        // we actually skip the first update to give a chance for execute deferred command lines to occur:
        int newPortValue = m_remoteConsolePort->GetIVal();

        if (m_lastPortValue != 0)
        {
            // the editor never allows remote control.
            if ((isEnabled) && (!isStarted))
            {
                Start();
            }
            else if (isStarted)
            {
                if ((!isEnabled) || (newPortValue != m_lastPortValue))
                {
                    Stop();
                }
            }
        }

        m_lastPortValue = newPortValue;
        
    }

    TEventBuffer events;
    m_pServer->GetEvents(events);
    for (TEventBuffer::iterator it = events.begin(), end = events.end(); it != end; ++it)
    {
        IRemoteEvent* pEvent = *it;
        switch (pEvent->GetType())
        {
        case eCET_ConsoleCommand:
            for (TListener::Notifier notifier(m_listener); notifier.IsValid(); notifier.Next())
            {
                notifier->OnConsoleCommand(((SStringEvent<eCET_ConsoleCommand>*)pEvent)->GetData());
            }
            break;
        case eCET_GameplayEvent:
            for (TListener::Notifier notifier(m_listener); notifier.IsValid(); notifier.Next())
            {
                notifier->OnGameplayCommand(((SStringEvent<eCET_GameplayEvent>*)pEvent)->GetData());
            }
            break;

#if 0 // currently no stroboscope support
        case eCET_Strobo_GetThreads:
            SendThreadData();
            break;
        case eCET_Strobo_GetResult:
            SendStroboscopeResult();
            break;
        default:
            assert(false); // NOT SUPPORTED FOR THE SERVER!!!
#endif
        }
        delete *it;
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::RegisterListener(IRemoteConsoleListener* pListener, const char* name)
{
#ifdef USE_REMOTE_CONSOLE
    m_listener.Add(pListener, name);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CRemoteConsole::UnregisterListener(IRemoteConsoleListener* pListener)
{
#ifdef USE_REMOTE_CONSOLE
    m_listener.Remove(pListener);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_REMOTE_CONSOLE

void SRemoteServer::StartServer()
{
    StopServer();
    m_bAcceptClients = true;
    Start(0, SERVER_THREAD_NAME);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::StopServer()
{
    Stop();
    m_bAcceptClients = false;
    AZ::AzSock::CloseSocket(m_socket);
    m_socket = SOCKET_ERROR;
    m_lock.Lock();
    for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        it->pClient->StopClient();
    }
    m_lock.Unlock();
    m_stopEvent.Wait();
    m_stopEvent.Set();
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::ClientDone(SRemoteClient* pClient)
{
    m_lock.Lock();
    for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->pClient == pClient)
        {
            it->pClient->Stop();
            delete it->pClient;
            delete it->pEvents;
            m_clients.erase(it);
            break;
        }
    }

    if (m_clients.empty())
    {
        m_stopEvent.Set();
    }
    m_lock.Unlock();
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::Terminate()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::Run()
{
    SetName(SERVER_THREAD_NAME);
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION REMOTECONSOLE_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(RemoteConsole_cpp, AZ_RESTRICTED_PLATFORM)
#endif

    AZSOCKET sClient;
    AZ::AzSock::AzSocketAddress local;

    if (AZ::AzSock::SocketErrorOccured(AZ::AzSock::Startup()))
    {
        gEnv->pLog->LogError("[RemoteKeyboard] Failed to load Winsock!\n");
        return;
    }

    m_socket = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(m_socket))
    {
        CryLog("Remote console FAILED. socket() => SOCKET_ERROR");
        return;
    }

    // this CVAR is optional.
    unsigned short remotePort = DEFAULT_PORT;
    ICVar* remoteConsolePort = gEnv->pConsole->GetCVar("log_RemoteConsolePort");
    if (remoteConsolePort)
    {
        remotePort = static_cast<unsigned short>(remoteConsolePort->GetIVal());
    }


    bool bindOk = false;
    int result = 0;
    for (uint32 i = 0; i < MAX_BIND_ATTEMPTS; ++i)
    {
        local.SetAddrPort(remotePort + i);
        result = AZ::AzSock::Bind(m_socket, local);
        if (!AZ::AzSock::SocketErrorOccured(result))
        {
            bindOk = true;
            break;
        }
    }

    if (bindOk == false)
    {
        CryLog("Remote console FAILED. bind() => SOCKET_ERROR. Faild ports from %d to %d", remotePort, remotePort + MAX_BIND_ATTEMPTS - 1);
        return;
    }

    AZ::AzSock::Listen(m_socket, 8);
    AZ::AzSock::AzSocketAddress sockName;
    result = AZ::AzSock::GetSockName(m_socket, sockName);

    if (!AZ::AzSock::SocketErrorOccured(result))
    {
        CryLog("Remote console listening on: %d\n", sockName.GetAddrPort());
    }
    else
    {
        CryLog("Remote console FAILED to listen on: %d\n", sockName.GetAddrPort());
    }

    while (m_bAcceptClients)
    {
        AZ::AzSock::AzSocketAddress clientAddress;
        sClient = AZ::AzSock::Accept(m_socket, clientAddress);
        if (!m_bAcceptClients || !AZ::AzSock::IsAzSocketValid(sClient))
        {
            break;
        }

        if (!RCON_IsRemoteAllowedToConnect(clientAddress))
        {
            AZ::AzSock::CloseSocket(sClient);
            continue;
        }

        m_lock.Lock();
        m_stopEvent.Reset();
        SRemoteClient* pClient = new SRemoteClient(this);
        m_clients.push_back(SRemoteClientInfo(pClient));
        pClient->StartClient(sClient);
        m_lock.Unlock();
    }
    AZ::AzSock::CloseSocket(m_socket);
    CryLog("Remote console terminating.\n");
    //AZ::AzSock::Shutdown();
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::AddEvent(IRemoteEvent* pEvent)
{
    m_lock.Lock();
    for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        it->pEvents->push_back(pEvent->Clone());
    }
    m_lock.Unlock();
    delete pEvent;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteServer::GetEvents(TEventBuffer& buffer)
{
    m_lock.Lock();
    buffer = m_eventBuffer;
    m_eventBuffer.clear();
    m_lock.Unlock();
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteServer::WriteBuffer(SRemoteClient* pClient,  char* buffer, int& size)
{
    m_lock.Lock();
    IRemoteEvent* pEvent = nullptr;
    for (TClients::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->pClient == pClient)
        {
            TEventBuffer* pEvents = it->pEvents;
            if (!pEvents->empty())
            {
                pEvent = pEvents->front();
                pEvents->pop_front();
            }
            break;
        }
    }
    m_lock.Unlock();
    const bool res = (pEvent != nullptr);
    if (pEvent)
    {
        SRemoteEventFactory::GetInst()->WriteToBuffer(pEvent, buffer, size);
        delete pEvent;
    }
    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteServer::ReadBuffer(const char* buffer, int data)
{
    IRemoteEvent* pEvent = SRemoteEventFactory::GetInst()->CreateEventFromBuffer(buffer, data);
    const bool res = (pEvent != nullptr);
    if (pEvent)
    {
        if (pEvent->GetType() != eCET_Noop)
        {
            m_lock.Lock();
            m_eventBuffer.push_back(pEvent);
            m_lock.Unlock();
        }
        else
        {
            delete pEvent;
        }
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::StartClient(AZSOCKET socket)
{
    m_socket = socket;
    Start(0, CLIENT_THREAD_NAME);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::StopClient()
{
    AZ::AzSock::CloseSocket(m_socket);
    m_socket = AZ_SOCKET_INVALID;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::Terminate()
{
    m_pServer->ClientDone(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::Run()
{
    SetName(CLIENT_THREAD_NAME);
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION REMOTECONSOLE_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(RemoteConsole_cpp, AZ_RESTRICTED_PLATFORM)
#endif

    char szBuff[DEFAULT_BUFFER];
    int size;
    SNoDataEvent<eCET_Req> reqEvt;

    std::vector<string> autoCompleteList;
    FillAutoCompleteList(autoCompleteList);

    bool ok = true;
    bool autoCompleteDoneSent = false;
    while (ok)
    {
        // read data
        SRemoteEventFactory::GetInst()->WriteToBuffer(&reqEvt, szBuff, size);
        ok &= SendPackage(szBuff, size);
        ok &= RecvPackage(szBuff, size);
        ok &= m_pServer->ReadBuffer(szBuff, size);

        for (int i = 0; i < 20 && !autoCompleteList.empty(); ++i)
        {
            SStringEvent<eCET_AutoCompleteList> autoCompleteListEvt(autoCompleteList.back().c_str());
            SRemoteEventFactory::GetInst()->WriteToBuffer(&autoCompleteListEvt, szBuff, size);
            ok &= SendPackage(szBuff, size);
            ok &= RecvPackage(szBuff, size);
            ok &= m_pServer->ReadBuffer(szBuff, size);
            autoCompleteList.pop_back();
        }
        if (autoCompleteList.empty() && !autoCompleteDoneSent)
        {
            SNoDataEvent<eCET_AutoCompleteListDone> autoCompleteDone;
            SRemoteEventFactory::GetInst()->WriteToBuffer(&autoCompleteDone, szBuff, size);

            ok &= SendPackage(szBuff, size);
            ok &= RecvPackage(szBuff, size);
            ok &= m_pServer->ReadBuffer(szBuff, size);
            autoCompleteDoneSent = true;
        }

        // send data
        while (ok && m_pServer->WriteBuffer(this, szBuff, size))
        {
            ok &= SendPackage(szBuff, size);
            ok &= RecvPackage(szBuff, size);
            IRemoteEvent* pEvt = SRemoteEventFactory::GetInst()->CreateEventFromBuffer(szBuff, size);
            ok &= pEvt && pEvt->GetType() == eCET_Noop;
            delete pEvt;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteClient::RecvPackage(char* buffer, int& size)
{
    size = 0;
    int ret, idx = 0;
    do
    {
        ret = AZ::AzSock::Recv(m_socket, buffer + idx, DEFAULT_BUFFER - idx, 0);
        if (AZ::AzSock::SocketErrorOccured(ret))
        {
            return false;
        }
        idx += ret;
    } while (buffer[idx - 1] != '\0');
    size = idx;
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
bool SRemoteClient::SendPackage(const char* buffer, int size)
{
    int ret, idx = 0;
    int left = size + 1;
    assert(buffer[size] == '\0');
    while (left > 0)
    {
        ret = AZ::AzSock::Send(m_socket, &buffer[idx], left, 0);
        if (AZ::AzSock::SocketErrorOccured(ret))
        {
            return false;
        }
        left -= ret;
        idx += ret;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteClient::FillAutoCompleteList(std::vector<string>& list)
{
    std::vector<const char*> cmds;
    size_t count = gEnv->pConsole->GetSortedVars(nullptr, 0);
    cmds.resize(count);
    count = gEnv->pConsole->GetSortedVars(&cmds[0], count);
    for (size_t i = 0; i < count; ++i)
    {
        list.push_back(cmds[i]);
    }

    if (!gEnv->pSystem || !gEnv->pSystem->GetILevelSystem())
    {
        return;
    }

    for (int i = 0, end = gEnv->pSystem->GetILevelSystem()->GetLevelCount(); i < end; ++i)
    {
        ILevelInfo* pLevel = gEnv->pSystem->GetILevelSystem()->GetLevelInfo(i);
        string item = "map ";
        const char* levelName = pLevel->GetName();
        int start = 0;
        for (int k = 0, kend = strlen(levelName); k < kend; ++k)
        {
            if ((levelName[k] == '\\' || levelName[k] == '/') && k + 1 < kend)
            {
                start = k + 1;
            }
        }
        item += levelName + start;
        list.push_back(item);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Event factory ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#define REGISTER_EVENT_NODATA(evt) RegisterEvent(new SNoDataEvent<evt>());
#define REGISTER_EVENT_STRING(evt) RegisterEvent(new SStringEvent<evt>(""));

/////////////////////////////////////////////////////////////////////////////////////////////
SRemoteEventFactory::SRemoteEventFactory()
{
    REGISTER_EVENT_NODATA(eCET_Noop);
    REGISTER_EVENT_NODATA(eCET_Req);
    REGISTER_EVENT_STRING(eCET_LogMessage);
    REGISTER_EVENT_STRING(eCET_LogWarning);
    REGISTER_EVENT_STRING(eCET_LogError);
    REGISTER_EVENT_STRING(eCET_ConsoleCommand);
    REGISTER_EVENT_STRING(eCET_AutoCompleteList);
    REGISTER_EVENT_NODATA(eCET_AutoCompleteListDone);

    REGISTER_EVENT_NODATA(eCET_Strobo_GetThreads);
    REGISTER_EVENT_STRING(eCET_Strobo_ThreadAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_ThreadDone);
    REGISTER_EVENT_NODATA(eCET_Strobo_GetResult);
    REGISTER_EVENT_NODATA(eCET_Strobo_ResultStart);
    REGISTER_EVENT_NODATA(eCET_Strobo_ResultDone);

    REGISTER_EVENT_NODATA(eCET_Strobo_StatStart);
    REGISTER_EVENT_STRING(eCET_Strobo_StatAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_ThreadInfoStart);
    REGISTER_EVENT_STRING(eCET_Strobo_ThreadInfoAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_SymStart);
    REGISTER_EVENT_STRING(eCET_Strobo_SymAdd);
    REGISTER_EVENT_NODATA(eCET_Strobo_CallstackStart);
    REGISTER_EVENT_STRING(eCET_Strobo_CallstackAdd);

    REGISTER_EVENT_STRING(eCET_GameplayEvent);

    REGISTER_EVENT_NODATA(eCET_Strobo_FrameInfoStart);
    REGISTER_EVENT_STRING(eCET_Strobo_FrameInfoAdd);
}

/////////////////////////////////////////////////////////////////////////////////////////////
SRemoteEventFactory::~SRemoteEventFactory()
{
    for (TPrototypes::iterator it = m_prototypes.begin(), end = m_prototypes.end(); it != end; ++it)
    {
        delete it->second;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
IRemoteEvent* SRemoteEventFactory::CreateEventFromBuffer(const char* buffer, int size)
{
    if (size > 1 && buffer[size - 1] == '\0')
    {
        EConsoleEventType type = EConsoleEventType(buffer[0] - '0');
        TPrototypes::const_iterator it = m_prototypes.find(type);
        if (it != m_prototypes.end())
        {
            IRemoteEvent* pEvent = it->second->CreateFromBuffer(buffer + 1, size - 1);
            return pEvent;
        }
    }
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteEventFactory::WriteToBuffer(IRemoteEvent* pEvent, char* buffer, int& size)
{
    assert(m_prototypes.find(pEvent->GetType()) != m_prototypes.end());
    buffer[0] = '0' + (char)pEvent->GetType();
    pEvent->WriteToBuffer(buffer + 1, size, DEFAULT_BUFFER - 2);
    buffer[++size] = '\0';
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SRemoteEventFactory::RegisterEvent(IRemoteEvent* pEvent)
{
    assert(m_prototypes.find(pEvent->GetType()) == m_prototypes.end());
    m_prototypes[pEvent->GetType()] = pEvent;
}

/////////////////////////////////////////////////////////////////////////////////////////////

#endif //#ifdef USE_REMOTE_CONSOLE
