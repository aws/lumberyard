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

#ifndef CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
#define CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
#pragma once

#include <IConsole.h>
#include <CryListenerSet.h>

#if !defined(RELEASE) || defined(RELEASE_LOGGING) || defined(ENABLE_PROFILING_CODE)
#include <AzCore/Socket/AzSocket_fwd.h>
#define USE_REMOTE_CONSOLE
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
// CRemoteConsole
//
// IRemoteConsole implementation
//
/////////////////////////////////////////////////////////////////////////////////////////////

struct SRemoteServer;
class CRemoteConsole
    : public IRemoteConsole
{
public:
    static CRemoteConsole* GetInst() { static CRemoteConsole inst; return &inst; }

    virtual void RegisterConsoleVariables();
    virtual void UnregisterConsoleVariables();

    virtual void Start();
    virtual void Stop();
    virtual bool IsStarted() const;

    virtual void AddLogMessage(const char* log);
    virtual void AddLogWarning(const char* log);
    virtual void AddLogError(const char* log);

    virtual void Update();

    virtual void RegisterListener(IRemoteConsoleListener* pListener, const char* name);
    virtual void UnregisterListener(IRemoteConsoleListener* pListener);

private:
    CRemoteConsole();
    virtual ~CRemoteConsole();

private:
    SRemoteServer* m_pServer;
    volatile bool m_running;
    typedef CListenerSet< IRemoteConsoleListener* > TListener;
    TListener m_listener;
    int m_lastPortValue = 0;
    ICVar* m_pLogEnableRemoteConsole = nullptr;
    ICVar* m_remoteConsoleAllowedHostList = nullptr;
    ICVar* m_remoteConsolePort = nullptr;
};

#ifdef USE_REMOTE_CONSOLE
/////////////////////////////////////////////////////////////////////////////////////////////
// IRemoteEvent
//
// remote events
// EConsoleEventType must be unique, also make sure order is same on clients!
// each package starts with ASCII ['0' + EConsoleEventType]
// not more supported than 256 - '0'!
//
/////////////////////////////////////////////////////////////////////////////////////////////

enum EConsoleEventType
{
    eCET_Noop = 0,
    eCET_Req,
    eCET_LogMessage,
    eCET_LogWarning,
    eCET_LogError,
    eCET_ConsoleCommand,
    eCET_AutoCompleteList,
    eCET_AutoCompleteListDone,

    eCET_Strobo_GetThreads,
    eCET_Strobo_ThreadAdd,
    eCET_Strobo_ThreadDone,

    eCET_Strobo_GetResult,
    eCET_Strobo_ResultStart,
    eCET_Strobo_ResultDone,

    eCET_Strobo_StatStart,
    eCET_Strobo_StatAdd,
    eCET_Strobo_ThreadInfoStart,
    eCET_Strobo_ThreadInfoAdd,
    eCET_Strobo_SymStart,
    eCET_Strobo_SymAdd,
    eCET_Strobo_CallstackStart,
    eCET_Strobo_CallstackAdd,

    eCET_GameplayEvent,

    eCET_Strobo_FrameInfoStart,
    eCET_Strobo_FrameInfoAdd,
};

struct SRemoteEventFactory;
struct IRemoteEvent
{
    virtual ~IRemoteEvent() {}
    IRemoteEvent(EConsoleEventType type)
        : m_type(type) {}
    EConsoleEventType GetType() const { return m_type; }

    virtual IRemoteEvent* Clone() = 0;

protected:
    friend struct SRemoteEventFactory;
    virtual void WriteToBuffer(char* buffer, int& size, int maxsize) = 0;
    virtual IRemoteEvent* CreateFromBuffer(const char* buffer, int size) = 0;

private:
    EConsoleEventType m_type;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Event implementations
//
// implementations for simple data packages
//
/////////////////////////////////////////////////////////////////////////////////////////////
template <EConsoleEventType T>
struct SNoDataEvent
    : public IRemoteEvent
{
    SNoDataEvent()
        : IRemoteEvent(T) {};
    virtual IRemoteEvent* Clone() { return new SNoDataEvent<T>(); }

protected:
    virtual void WriteToBuffer(char* buffer, int& size, int maxsize) {  size = 0; }
    virtual IRemoteEvent* CreateFromBuffer(const char* buffer, int size) { return Clone(); }
};

/////////////////////////////////////////////////////////////////////////////////////////////
template <EConsoleEventType T>
struct SStringEvent
    : public IRemoteEvent
{
    SStringEvent(const char* string)
        : IRemoteEvent(T)
        , m_data(string) {};
    virtual IRemoteEvent* Clone() { return new SStringEvent<T>(GetData()); }
    const char* GetData() const { return m_data.c_str(); }

protected:
    virtual void WriteToBuffer(char* buffer, int& size, int maxsize)
    {
        const char* data = GetData();
        size = min((int)strlen(data), maxsize);
        memcpy(buffer, data, size);
    }
    virtual IRemoteEvent* CreateFromBuffer(const char* buffer, int size) { return new SStringEvent<T>(buffer); }

private:
    string m_data;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// SRemoteEventFactory
//
// remote event factory
//
/////////////////////////////////////////////////////////////////////////////////////////////
struct SRemoteEventFactory
{
    static SRemoteEventFactory* GetInst() { static SRemoteEventFactory inst; return &inst; }
    IRemoteEvent* CreateEventFromBuffer(const char* buffer, int size);
    void WriteToBuffer(IRemoteEvent* pEvent, char* buffer, int& size);

private:
    SRemoteEventFactory();
    ~SRemoteEventFactory();

    void RegisterEvent(IRemoteEvent* pEvent);
private:
    typedef std::map<EConsoleEventType, IRemoteEvent*> TPrototypes;
    TPrototypes m_prototypes;
};

typedef std::list<IRemoteEvent*> TEventBuffer;

/////////////////////////////////////////////////////////////////////////////////////////////
// SRemoteServer
//
// Server thread
//
/////////////////////////////////////////////////////////////////////////////////////////////
struct SRemoteClient;
struct SRemoteServer
    : public CrySimpleThread<>
{
    SRemoteServer()
        : m_socket(AZ_SOCKET_INVALID) { m_stopEvent.Set(); }

    void StartServer();
    void StopServer();

    void AddEvent(IRemoteEvent* pEvent);
    void GetEvents(TEventBuffer& buffer);

    // CrySimpleThread
    void Terminate() override;
    void Run() override;
    // ~CrySimpleThread

private:
    bool WriteBuffer(SRemoteClient* pClient,  char* buffer, int& size);
    bool ReadBuffer(const char* buffer, int data);

    void ClientDone(SRemoteClient* pClient);

private:
    struct SRemoteClientInfo
    {
        SRemoteClientInfo(SRemoteClient* client)
            : pClient(client)
            , pEvents(new TEventBuffer) {}
        SRemoteClient* pClient;
        TEventBuffer* pEvents;
    };

    typedef std::vector<SRemoteClientInfo> TClients;
    TClients m_clients;
    AZSOCKET m_socket;
    CryMutex m_lock;
    TEventBuffer m_eventBuffer;
    CryEvent m_stopEvent;
    volatile bool m_bAcceptClients;
    friend struct SRemoteClient;
};


/////////////////////////////////////////////////////////////////////////////////////////////
// SRemoteClient
//
// Client thread
//
/////////////////////////////////////////////////////////////////////////////////////////////
struct SRemoteClient
    : public CrySimpleThread<>
{
    SRemoteClient(SRemoteServer* pServer)
        : m_pServer(pServer)
        , m_socket(AZ_SOCKET_INVALID) {}

    void StartClient(AZSOCKET socket);
    void StopClient();

    // CrySimpleThread
    void Terminate() override;
    void Run() override;
    // ~CrySimpleThread

private:
    bool RecvPackage(char* buffer, int& size);
    bool SendPackage(const char* buffer, int size);
    void FillAutoCompleteList(std::vector<string>& list);

private:
    SRemoteServer* m_pServer;
    AZSOCKET m_socket;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

#endif //#ifdef USE_REMOTE_CONSOLE

#endif // CRYINCLUDE_CRYSYSTEM_REMOTECONSOLE_REMOTECONSOLE_H
