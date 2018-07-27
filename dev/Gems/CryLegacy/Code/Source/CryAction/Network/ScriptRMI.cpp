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

// Description : Provides remote method invocation to script


#include "CryLegacy_precompiled.h"
#include "ScriptRMI.h"
#include "ScriptSerialize.h"
#include "Serialization/NetScriptSerialize.h"
#include "GameContext.h"
#include "CryAction.h"
#include "IConsole.h"

#define FLAGS_FIELD "__sendto"
#define ID_FIELD "__id"
#define CLIENT_DISPATCH_FIELD "__client_dispatch"
#define SERVER_DISPATCH_FIELD "__server_dispatch"
#define VALIDATED_FIELD "__validated"
#define SERIALIZE_FUNCTION "__serialize"
#define NET_MARSHAL_FUNCTION "__netmarshal"
#define NET_UNMARSHAL_FUNCTION "__netunmarshal"
#define SERVER_SYNCHED_FIELD "__synched"
#define HIDDEN_FIELD "__hidden"

//#pragma optimize("",off)
//#pragma inline_depth(0)

static ICVar* pLogRMIEvents = NULL;
static ICVar* pDisconnectOnError = NULL;

CScriptRMI* CScriptRMI::m_pThis = NULL;

// MUST be plain-old-data (no destructor called)
struct CScriptRMI::SSynchedPropertyInfo
{
    char name[MaxSynchedPropertyNameLength + 1];
    EScriptSerializeType type;
};

// MUST be plain-old-data (see CScriptTable::AddFunction)
struct CScriptRMI::SFunctionInfo
{
    char format[MaxRMIParameters + 1];
    ENetReliabilityType reliability;
    ERMIAttachmentType attachment;
    uint8 funcID;
};

struct SSerializeFunctionParams
{
    SSerializeFunctionParams(TSerialize s, IEntity* p)
        : ser(s)
        , pEntity(p) {}
    TSerialize ser;
    IEntity* pEntity;
};

struct SNetMarshalFunctionParams
{
    SNetMarshalFunctionParams(Serialization::INetScriptMarshaler* scriptNetMarshaler, IEntity* p)
        : m_scriptNetMarshaler(scriptNetMarshaler)
        , pEntity(p)        
    {}
    
    Serialization::INetScriptMarshaler* m_scriptNetMarshaler;
    IEntity* pEntity;
};

struct SNetUnmarshalFunctionParams
{
    SNetUnmarshalFunctionParams(TSerialize ser, IEntity* p)
        : serializer(ser)
        , pEntity(p)
    {
    }

    TSerialize serializer;
    IEntity* pEntity;    
};

// this class implements the body of a remote method invocation message
class CScriptRMI::CScriptMessage
    : public _i_reference_target_t
    , public CScriptRMISerialize
{
public:
    CScriptMessage(
        ENetReliabilityType reliability_ = eNRT_ReliableUnordered,
        ERMIAttachmentType attachment_ = eRAT_NoAttach,
        EntityId objId_ = kInvalidEntityId,
        uint8 funcId_ = 0,
        const char* fmt_ = nullptr,
        bool client_ = false)
        : objId(objId_)
        , funcId(funcId_)
        , CScriptRMISerialize(fmt_)
        , client(client_)
        , m_refs(0)
    {
    }

    void SerializeHeader(TSerialize pSerialize)
    {
        // Serialize only header information. When reading, this allows
        // us to first read object and call-related information, so we
        // know how to process the rest of the message (i.e. parameters).
        pSerialize.Value("object", objId, 'eid');
        pSerialize.Value("funcId", funcId);
        pSerialize.Value("client", client);
    }

    void SerializeParameters(TSerialize pSerialize)
    {
        // Serialize the parameter blob only. CScriptRMISerialize format
        // must already been applied, otherwise parameter data is unknown.
        CScriptRMISerialize::SerializeWith(pSerialize);
    }

    void SerializeWith(TSerialize pSerialize)
    {
        // Invoked by the RMI system when preparing the full package,
        // so serialize the entire payload (header + parameter blob).
        SerializeHeader(pSerialize);
        SerializeParameters(pSerialize);
    }

    void SetParamsFormat(const char* fmt)
    {
        // Apply the parameter format so we know what parameter data
        // to pull from the serialization buffer.
        CScriptRMISerialize::SetFormat(fmt);
    }

    int8 GetRefs()
    {
        return m_refs;
    }

    size_t GetSize()
    {
        return sizeof(*this);
    }

#if ENABLE_RMI_BENCHMARK
    virtual const SRMIBenchmarkParams* GetRMIBenchmarkParams()
    {
        return NULL;
    }
#endif

public:

    EntityId objId;
    uint8 funcId;
    bool client;

private:

    int8 m_refs;
};

CScriptRMI::CScriptRMI()
    : m_pParent(0)
{
    m_pThis = this;
}

bool CScriptRMI::Init()
{
    /*
    CryAutoCriticalSection lkDispatch(m_dispatchMutex);

    IScriptSystem * pSS = gEnv->pScriptSystem;
    IEntityClassRegistry * pReg = gEnv->pEntitySystem->GetClassRegistry();
    IEntityClass * pClass = NULL;
    for (pReg->IteratorMoveFirst(); pClass = pReg->IteratorNext();)
    {
        pClass->LoadScript(false);
        if (IEntityScript * pEntityScript = pClass->GetIEntityScript())
        {
            if (m_entityClassToEntityTypeID.find(pClass->GetName()) != m_entityClassToEntityTypeID.end())
            {
                SmartScriptTable entityTable;
                pSS->GetGlobalValue( pClass->GetName(), entityTable );
                if (!!entityTable)
                {
                    //ValidateDispatchTable( pClass->GetName(), , , false );
                    //ValidateDispatchTable( pClass->GetName(), , , true );
                }
            }
        }
    }
    */
    return true;
}

void CScriptRMI::UnloadLevel()
{
    CryAutoLock<CryCriticalSection> lock(m_dispatchMutex);
    m_entities.clear();
    m_entityClassToEntityTypeID.clear();

    const size_t numDispatches = m_dispatch.size();
    for (size_t i = 0; i < numDispatches; ++i)
    {
        SDispatch& dispatch = m_dispatch[i];
        if (dispatch.m_serverDispatchScriptTable)
        {
            dispatch.m_serverDispatchScriptTable->SetValue(VALIDATED_FIELD, false);
        }
        if (dispatch.m_clientDispatchScriptTable)
        {
            dispatch.m_clientDispatchScriptTable->SetValue(VALIDATED_FIELD, false);
        }
    }

    stl::free_container(m_dispatch);
}

void CScriptRMI::SetContext(CGameContext* pContext)
{
    m_pParent = pContext;
}

void CScriptRMI::RegisterCVars()
{
    pLogRMIEvents = REGISTER_INT(
            "net_log_remote_methods", 0, VF_DUMPTODISK,
            "Log remote method invocations");
    pDisconnectOnError = REGISTER_INT(
            "net_disconnect_on_rmi_error", 0, VF_DUMPTODISK,
            "Disconnect remote connections on script exceptions during RMI calls");
}

void CScriptRMI::UnregisterCVars()
{
    SAFE_RELEASE(pLogRMIEvents);
    SAFE_RELEASE(pDisconnectOnError);
}

// implementation of Net.Expose() - exposes a class
int CScriptRMI::ExposeClass(IFunctionHandler* pFH)
{
    SmartScriptTable params, cls, clientMethods, serverMethods;
    SmartScriptTable clientTable, serverTable;
    SmartScriptTable serverProperties;

    IScriptSystem* pSS = pFH->GetIScriptSystem();

    if (pFH->GetParamCount() != 1)
    {
        pSS->RaiseError("Net.Expose takes only one parameter - a table");
        return pFH->EndFunction(false);
    }

    if (!pFH->GetParam(1, params))
    {
        pSS->RaiseError("Net.Expose takes only one parameter - a table");
        return pFH->EndFunction(false);
    }
    if (!params->GetValue("Class", cls))
    {
        pSS->RaiseError("No 'Class' parameter to Net.Expose");
        return pFH->EndFunction(false);
    }

    if (!params->GetValue("ClientMethods", clientMethods))
    {
        GameWarning("No 'ClientMethods' parameter to Net.Expose");
    }
    else if (!cls->GetValue("Client", clientTable))
    {
        pSS->RaiseError("'ClientMethods' exposed, but no 'Client' table in class");
        return pFH->EndFunction(false);
    }
    if (!params->GetValue("ServerMethods", serverMethods))
    {
        GameWarning("No 'ServerMethods' parameter to Net.Expose");
    }
    else if (!cls->GetValue("Server", serverTable))
    {
        pSS->RaiseError("'ServerMethods' exposed, but no 'Server' table in class");
        return pFH->EndFunction(false);
    }
    params->GetValue("ServerProperties", serverProperties);

    if (clientMethods.GetPtr())
    {
        CRY_ASSERT(clientTable.GetPtr());
        if (!BuildDispatchTable(clientMethods, clientTable, cls, CLIENT_DISPATCH_FIELD))
        {
            return pFH->EndFunction(false);
        }
    }

    if (serverMethods.GetPtr())
    {
        CRY_ASSERT(serverTable.GetPtr());
        if (!BuildDispatchTable(serverMethods, serverTable, cls, SERVER_DISPATCH_FIELD))
        {
            return pFH->EndFunction(false);
        }
    }

    if (serverProperties.GetPtr() && serverProperties->HasElements())
    {
        if (!BuildSynchTable(serverProperties, cls, SERVER_SYNCHED_FIELD))
        {
            return pFH->EndFunction(false);
        }
    }

    return pFH->EndFunction(true);
}

// build a script dispatch table - this table is the metatable for all
// dispatch proxy tables used (onClient, allClients, otherClients)
bool CScriptRMI::BuildDispatchTable(
    SmartScriptTable methods,
    SmartScriptTable classMethodTable,
    SmartScriptTable cls,
    const char* name)
{
    IScriptSystem* pSS = methods->GetScriptSystem();
    SmartScriptTable dispatch(pSS);

    uint8 funcID = 0;

    IScriptTable::SUserFunctionDesc fd;
    SFunctionInfo info;

    IScriptTable::Iterator iter = methods->BeginIteration();
    while (methods->MoveNext(iter))
    {
        if (iter.sKey)
        {
            const char* function = iter.sKey;

            if (strlen(function) >= 2 && function[0] == '_' && function[1] == '_')
            {
                methods->EndIteration(iter);
                pSS->RaiseError("In Net.Expose: can't expose functions beginning with '__' (function was %s)",
                    function);
                return false;
            }

            SmartScriptTable specTable;
            if (!methods->GetValue(function, specTable))
            {
                methods->EndIteration(iter);
                pSS->RaiseError("In Net.Expose: function %s entry is not a table (in %s)",
                    function, name);
                return false;
            }

            // fetch format
            int count = specTable->Count();
            if (count < 1)
            {
                methods->EndIteration(iter);
                pSS->RaiseError("In Net.Expose: function %s entry is an empty table (in %s)",
                    function, name);
                return false;
            }
            else if (count - 1 > MaxRMIParameters)
            {
                methods->EndIteration(iter);
                pSS->RaiseError("In Net.Expose: function %s has too many parameters (%d) (in %s)",
                    function, count - 1, name);
                return false;
            }
            int tempReliability;
            if (!specTable->GetAt(1, tempReliability) || tempReliability < 0 || tempReliability >= eNRT_NumReliabilityTypes)
            {
                methods->EndIteration(iter);
                pSS->RaiseError("In Net.Expose: function %s has invalid reliability type %d (in %s)",
                    function, tempReliability, name);
                return false;
            }
            ENetReliabilityType reliability = (ENetReliabilityType)tempReliability;
            if (!specTable->GetAt(2, tempReliability) || tempReliability < 0 || tempReliability >= eRAT_NumAttachmentTypes)
            {
                methods->EndIteration(iter);
                pSS->RaiseError("In Net.Expose: function %s has invalid attachment type %d (in %s)",
                    function, tempReliability, name);
            }
            ERMIAttachmentType attachment = (ERMIAttachmentType)tempReliability;
            string format;
            format.reserve(count - 1);
            for (int i = 3; i <= count; i++)
            {
                int type = 666;
                if (!specTable->GetAt(i, type) || type < -128 || type > 127)
                {
                    methods->EndIteration(iter);
                    pSS->RaiseError("In Net.Expose: function %s has invalid serialization policy %d at %d (in %s)",
                        function, type, i, name);
                    return false;
                }
                format.push_back((char)type);
            }

            CRY_ASSERT(format.length() <= MaxRMIParameters);
            azstrcpy(info.format, AZ_ARRAY_SIZE(info.format), format.c_str());
            info.funcID = funcID;
            info.reliability = reliability;
            info.attachment = attachment;

            fd.pUserDataFunc = ProxyFunction;
            fd.sFunctionName = function;
            fd.nDataSize = sizeof(SFunctionInfo);
            fd.pDataBuffer = &info;
            fd.sGlobalName = "<net-dispatch>";
            fd.sFunctionParams = "(...)";

            dispatch->AddFunction(fd);

            string lookupData = function;
            lookupData += ":";
            lookupData += format;
            dispatch->SetAt(funcID + 1, lookupData.c_str());


            funcID++;
            if (funcID == 0)
            {
                funcID--;
                methods->EndIteration(iter);
                pSS->RaiseError("Too many functions... max is %d", funcID);
                return false;
            }
        }
        else
        {
            GameWarning("In Net.Expose: non-string key ignored");
        }
    }
    methods->EndIteration(iter);

    dispatch->SetValue(VALIDATED_FIELD, false);
    cls->SetValue(name, dispatch);

    return true;
}

// setup the meta-table for synched variables
bool CScriptRMI::BuildSynchTable(SmartScriptTable vars, SmartScriptTable cls, const char* name)
{
    IScriptSystem* pSS = vars->GetScriptSystem();
    SmartScriptTable synched(pSS);
    SmartScriptTable defaultValues(pSS);

    // TODO: Improve
    IScriptTable::SUserFunctionDesc fd;
    fd.pFunctor = functor_ret(SynchedNewIndexFunction);
    fd.sFunctionName = "__newindex";
    fd.sGlobalName = "<net-dispatch>";
    fd.sFunctionParams = "(...)";
    synched->AddFunction(fd);

    std::vector<SSynchedPropertyInfo> properties;

    IScriptTable::Iterator iter = vars->BeginIteration();
    while (vars->MoveNext(iter))
    {
        if (iter.sKey)
        {
            int type;
            if (!vars->GetValue(iter.sKey, type))
            {
                vars->EndIteration(iter);
                pSS->RaiseError("No type for %s", iter.sKey);
                return false;
            }
            size_t len = strlen(iter.sKey);
            if (len > MaxSynchedPropertyNameLength)
            {
                vars->EndIteration(iter);
                pSS->RaiseError("Synched var name '%s' too long (max is %d)",
                    iter.sKey, (int)MaxSynchedPropertyNameLength);
                return false;
            }
            SSynchedPropertyInfo info;
            azstrcpy(info.name, AZ_ARRAY_SIZE(info.name), iter.sKey);
            info.type = (EScriptSerializeType)type;
            properties.push_back(info);

            switch (info.type)
            {
            case eSST_String:
                defaultValues->SetValue(iter.sKey, "");
                break;
            case eSST_EntityId:
                defaultValues->SetValue(iter.sKey, ScriptHandle(0));
                break;
            case eSST_Void:
                defaultValues->SetValue(iter.sKey, ScriptAnyValue());
                break;
            case eSST_Vec3:
                defaultValues->SetValue(iter.sKey, Vec3(0, 0, 0));
                break;
            default:
                defaultValues->SetValue(iter.sKey, 0);
            }
        }
    }
    vars->EndIteration(iter);

    if (properties.empty())
    {
        return true;
    }

    // Serialize Function
    fd.pFunctor = NULL;
    fd.pUserDataFunc = SerializeFunction;
    fd.nDataSize = sizeof(SSynchedPropertyInfo) * properties.size();
    fd.pDataBuffer = &properties[0];
    fd.sFunctionName = SERIALIZE_FUNCTION;
    fd.sFunctionParams = "(...)";
    fd.sGlobalName = "<net-dispatch>";
    synched->AddFunction(fd);

    // Marshal Function
    fd.pFunctor = NULL;
    fd.pUserDataFunc = NetMarshalFunction;
    fd.nDataSize = sizeof(SSynchedPropertyInfo) * properties.size();
    fd.pDataBuffer = &properties[0];
    fd.sFunctionName = NET_MARSHAL_FUNCTION;
    fd.sFunctionParams = "(...)";
    fd.sGlobalName = "<net-dispatch>";
    synched->AddFunction(fd);

    // Unmarshal Function
    fd.pFunctor = NULL;
    fd.pUserDataFunc = NetUnmarshalFunction;
    fd.nDataSize = sizeof(SSynchedPropertyInfo) * properties.size();
    fd.pDataBuffer = &properties[0];
    fd.sFunctionName = NET_UNMARSHAL_FUNCTION;
    fd.sFunctionParams = "(...)";
    fd.sGlobalName = "<net-dispatch>";
    synched->AddFunction(fd);

    cls->SetValue(SERVER_SYNCHED_FIELD, synched);
    cls->SetValue("synched", defaultValues);

    return true;
}

// initialize an entity for networked methods
void CScriptRMI::SetupEntity(EntityId eid, IEntity* pEntity, bool client, bool server)
{
    if (!m_pParent)
    {
        GameWarning("Trying to setup an entity for network with no game started... failing");
        return;
    }

    IEntityClass* pClass = pEntity->GetClass();
    stack_string className = pClass->GetName();

    IScriptTable* pEntityTable = pEntity->GetScriptTable();
    IScriptSystem* pSS = pEntityTable->GetScriptSystem();

    SmartScriptTable clientDispatchTable, serverDispatchTable, serverSynchedTable;
    pEntityTable->GetValue(CLIENT_DISPATCH_FIELD, clientDispatchTable);
    pEntityTable->GetValue(SERVER_DISPATCH_FIELD, serverDispatchTable);
    pEntityTable->GetValue(SERVER_SYNCHED_FIELD, serverSynchedTable);

    bool validated;
    if (clientDispatchTable.GetPtr())
    {
        if (!clientDispatchTable->GetValue(VALIDATED_FIELD, validated))
        {
            return;
        }
        if (!validated)
        {
            SmartScriptTable methods;
            if (!pEntityTable->GetValue("Client", methods))
            {
                GameWarning("No Client table, but has a client dispatch on class %s",
                    pEntity->GetClass()->GetName());
                return;
            }
            if (!ValidateDispatchTable(pEntity->GetClass()->GetName(), clientDispatchTable, methods, false))
            {
                GameWarning("Client dispatch table failed validation in class %s",
                    pEntity->GetClass()->GetName());
                return;
            }
        }
    }
    if (serverDispatchTable.GetPtr())
    {
        if (!serverDispatchTable->GetValue(VALIDATED_FIELD, validated))
        {
            return;
        }
        if (!validated)
        {
            SmartScriptTable methods;
            if (!pEntityTable->GetValue("Server", methods))
            {
                GameWarning("No Server table, but has a server dispatch on class %s",
                    pEntity->GetClass()->GetName());
                return;
            }
            if (!ValidateDispatchTable(pEntity->GetClass()->GetName(), serverDispatchTable, methods, true))
            {
                GameWarning("Server dispatch table failed validation in class %s",
                    pEntity->GetClass()->GetName());
                return;
            }
        }
    }

    ScriptHandle id;
    id.n = eid;

    ScriptHandle flags;

    if (client && serverDispatchTable.GetPtr())
    {
        flags.n = eDF_ToServer;
        AddProxyTable(pEntityTable, id, flags, "server", serverDispatchTable);
    }

    if (server && clientDispatchTable.GetPtr())
    {
        // only expose ownClient, otherClients for actors with a channelId
        flags.n = eDF_ToClientOnChannel;
        AddProxyTable(pEntityTable, id, flags, "onClient", clientDispatchTable);
        flags.n = eDF_ToClientOnOtherChannels;
        AddProxyTable(pEntityTable, id, flags, "otherClients", clientDispatchTable);
        flags.n = eDF_ToClientOnChannel | eDF_ToClientOnOtherChannels;
        AddProxyTable(pEntityTable, id, flags, "allClients", clientDispatchTable);
    }

    if (serverSynchedTable.GetPtr())
    {
        AddSynchedTable(pEntityTable, id, "synched", serverSynchedTable);
    }

    CryAutoCriticalSection lkDispatch(m_dispatchMutex);
    std::map<string, size_t>::iterator iter = m_entityClassToEntityTypeID.find(CONST_TEMP_STRING(pEntity->GetClass()->GetName()));
    if (iter == m_entityClassToEntityTypeID.end())
    {
        //[Timur] commented out as spam.
        //GameWarning("[scriptrmi] unable to find class %s", pEntity->GetClass()->GetName());
    }
    else
    {
        m_entities[eid] = iter->second;
    }
}

void CScriptRMI::RemoveEntity(EntityId id)
{
    CryAutoCriticalSection lkDispatch(m_dispatchMutex);
    m_entities.erase(id);
}

bool CScriptRMI::SerializeScript(TSerialize ser, IEntity* pEntity)
{
    SSerializeFunctionParams p(ser, pEntity);
    ScriptHandle hdl(&p);
    IScriptTable* pTable = pEntity->GetScriptTable();
    if (pTable)
    {
        SmartScriptTable serTable;
        SmartScriptTable synchedTable;
        pTable->GetValue("synched", synchedTable);
        if (synchedTable.GetPtr())
        {
            synchedTable->GetValue(HIDDEN_FIELD, serTable);
            if (serTable.GetPtr())
            {
                IScriptSystem* pScriptSystem = pTable->GetScriptSystem();
                pScriptSystem->BeginCall(serTable.GetPtr(), SERIALIZE_FUNCTION);
                pScriptSystem->PushFuncParam(serTable);
                pScriptSystem->PushFuncParam(hdl);
                return pScriptSystem->EndCall();
            }
        }
    }
    return true;
}

bool CScriptRMI::NetMarshalScript(Serialization::INetScriptMarshaler* scriptNetMarshaler, IEntity* pEntity)
{
    SNetMarshalFunctionParams params(scriptNetMarshaler,pEntity);
    ScriptHandle hdl(&params);

    IScriptTable* pTable = pEntity->GetScriptTable();
    if (pTable)
    {
        SmartScriptTable serTable;
        SmartScriptTable synchedTable;
        pTable->GetValue("synched", synchedTable);
        if (synchedTable.GetPtr())
        {
            synchedTable->GetValue(HIDDEN_FIELD, serTable);
            if (serTable.GetPtr())
            {
                IScriptSystem* pScriptSystem = pTable->GetScriptSystem();
                pScriptSystem->BeginCall(serTable.GetPtr(), NET_MARSHAL_FUNCTION);
                pScriptSystem->PushFuncParam(serTable);
                pScriptSystem->PushFuncParam(hdl);
                return pScriptSystem->EndCall();
            }
        }
    }
    return true;
}

void CScriptRMI::NetUnmarshalScript(TSerialize ser, IEntity* pEntity)
{
    SSerializeFunctionParams p(ser, pEntity);
    ScriptHandle hdl(&p);
    IScriptTable* pTable = pEntity->GetScriptTable();
    if (pTable)
    {
        SmartScriptTable serTable;
        SmartScriptTable synchedTable;
        pTable->GetValue("synched", synchedTable);
        if (synchedTable.GetPtr())
        {
            synchedTable->GetValue(HIDDEN_FIELD, serTable);
            if (serTable.GetPtr())
            {
                IScriptSystem* pScriptSystem = pTable->GetScriptSystem();
                pScriptSystem->BeginCall(serTable.GetPtr(), NET_UNMARSHAL_FUNCTION);
                pScriptSystem->PushFuncParam(serTable);
                pScriptSystem->PushFuncParam(hdl);
                pScriptSystem->EndCall();
            }
        }
    }    
}

int CScriptRMI::SerializeFunction(IFunctionHandler* pH, void* pBuffer, int nSize)
{
    SmartScriptTable serTable;
    ScriptHandle hdl;
    pH->GetParam(1, serTable);
    pH->GetParam(2, hdl);
    SSerializeFunctionParams* p = (SSerializeFunctionParams*)hdl.ptr;

    SSynchedPropertyInfo* pInfo = (SSynchedPropertyInfo*)pBuffer;
    int nProperties = nSize / sizeof(SSynchedPropertyInfo);

    if (p->ser.IsReading())
    {
        for (int i = 0; i < nProperties; i++)
        {
            if (!m_pThis->ReadValue(pInfo[i].name, pInfo[i].type, p->ser, serTable.GetPtr()))
            {
                return pH->EndFunction(false);
            }
        }
    }
    else
    {
        for (int i = 0; i < nProperties; i++)
        {
            if (!m_pThis->WriteValue(pInfo[i].name, pInfo[i].type, p->ser, serTable.GetPtr()))
            {
                return pH->EndFunction(false);
            }
        }
    }
    return pH->EndFunction(true);
}

int CScriptRMI::NetMarshalFunction(IFunctionHandler* ph, void* pBuffer, int nSize)
{
    FRAME_PROFILER("NetMarshalFunction", GetISystem(), PROFILE_NETWORK);

    SmartScriptTable serTable;
    ScriptHandle hdl;
    ph->GetParam(1,serTable);
    ph->GetParam(2, hdl);

    SNetMarshalFunctionParams* params = static_cast<SNetMarshalFunctionParams*>(hdl.ptr);
    Serialization::INetScriptMarshaler* scriptMarshaler = params->m_scriptNetMarshaler;

    SSynchedPropertyInfo* pInfo = (SSynchedPropertyInfo*)pBuffer;
    int nProperties = nSize /sizeof(SSynchedPropertyInfo);

    if (scriptMarshaler != nullptr)
    {
        AZ_Error("ScriptRMI",nProperties <= scriptMarshaler->GetMaxServerProperties(),"Trying to create too many Server properties");        

        IScriptTable* table = serTable.GetPtr();

        // Next step, maybe do a dirty table?
        // Should be able to hook into a custom throttler in order to manipulate them correctly.
        for (int i=0; i < nProperties; ++i)
        {
            TSerialize serializer = scriptMarshaler->FindSerializer(pInfo[i].name);

            bool wroteValue = false;

            if (scriptMarshaler && serializer.Ok())
            {
                // Need to write out the name so we can synchronize it on the other side
                serializer.Value("index",i);
                wroteValue = m_pThis->WriteValue(pInfo[i].name, pInfo[i].type, serializer, serTable.GetPtr());
            }

            scriptMarshaler->CommitSerializer(pInfo[i].name,serializer);
            
            if (!wroteValue)
            {
                return ph->EndFunction(false);
            }
        }
    }

    return ph->EndFunction(true);
}

int CScriptRMI::NetUnmarshalFunction(IFunctionHandler* ph, void* pBuffer, int nSize)
{
    SmartScriptTable serTable;
    ScriptHandle handle;
    ph->GetParam(1,serTable);
    ph->GetParam(2,handle);

    SNetUnmarshalFunctionParams* functionParams = static_cast<SNetUnmarshalFunctionParams*>(handle.ptr);
    TSerialize serializer = functionParams->serializer;    

    int index = -1;
    serializer.Value("index",index);

    SSynchedPropertyInfo* pInfo = (SSynchedPropertyInfo*)pBuffer;
    int nProperties = nSize /sizeof(SSynchedPropertyInfo);

    bool wroteValue = false;

    if (index >= 0 && index < nProperties)
    {        
        wroteValue = m_pThis->ReadValue(pInfo[index].name, pInfo[index].type, serializer, serTable.GetPtr());

        ScriptAnyValue value;
        serTable->GetValueAny(pInfo[index].name,value);

        AZ_Error("ScriptRMI",wroteValue,"Error with NetSynched Value - %s",pInfo[index].name);
    }

    return ph->EndFunction(wroteValue);
}

// one-time validation of entity tables
bool CScriptRMI::ValidateDispatchTable(const char* clazz, SmartScriptTable dispatch, SmartScriptTable methods, bool bServerTable)
{
    CryAutoCriticalSection lkDispatch(m_dispatchMutex);

    std::map<string, size_t>::iterator iterN = m_entityClassToEntityTypeID.lower_bound(clazz);
    if (iterN == m_entityClassToEntityTypeID.end() || iterN->first != clazz)
    {
        iterN = m_entityClassToEntityTypeID.insert(iterN, std::make_pair(clazz, m_entityClassToEntityTypeID.size()));
        CRY_ASSERT(iterN->second == m_dispatch.size());
        m_dispatch.push_back(SDispatch());
    }
    SDispatch& dispatchTblCont = m_dispatch[iterN->second];
    SFunctionDispatchTable& dispatchTbl = bServerTable ? dispatchTblCont.server : dispatchTblCont.client;

    IScriptTable::Iterator iter = dispatch->BeginIteration();
    while (dispatch->MoveNext(iter))
    {
        if (iter.sKey)
        {
            if (iter.sKey[0] == '_' && iter.sKey[1] == '_')
            {
                continue;
            }

            ScriptVarType type = methods->GetValueType(iter.sKey);
            if (type != svtFunction)
            {
                GameWarning("In class %s: function %s is exposed but not defined",
                    clazz, iter.sKey);
                dispatch->EndIteration(iter);
                return false;
            }
        }
        else
        {
            int id = iter.nKey;
            CRY_ASSERT(id > 0);
            id--;

            if (id >= dispatchTbl.size())
            {
                dispatchTbl.resize(id + 1);
            }
            SFunctionDispatch& dt = dispatchTbl[id];
            if (iter.value.GetVarType() != svtString)
            {
                GameWarning("Expected a string in dispatch table, got type %d", iter.value.GetVarType());
                dispatch->EndIteration(iter);
                return false;
            }

            const char* funcData = iter.value.str;
            const char* colon = strchr(funcData, ':');
            if (colon == NULL)
            {
                dispatch->EndIteration(iter);
                return false;
            }
            if (colon - funcData > MaxSynchedPropertyNameLength)
            {
                dispatch->EndIteration(iter);
                return false;
            }
            memcpy(dt.name, funcData, colon - funcData);
            dt.name[colon - funcData] = 0;
            azstrcpy(dt.format, AZ_ARRAY_SIZE(dt.format), colon + 1);
        }
    }
    dispatch->EndIteration(iter);
    dispatch->SetValue(VALIDATED_FIELD, true);

    if (bServerTable)
    {
        dispatchTblCont.m_serverDispatchScriptTable = dispatch;
    }
    else
    {
        dispatchTblCont.m_clientDispatchScriptTable = dispatch;
    }

    return true;
}

class CScriptRMI::CCallHelper
{
public:
    CCallHelper()
    {
        m_pScriptTable = 0;
        m_pScriptSystem = 0;
        m_pEntitySystem = 0;
        m_pEntity = 0;
    }

    bool Prologue(EntityId objID, bool bClient, uint8 funcID)
    {
        if (!InitGameMembers(objID))
        {
            return false;
        }

        SmartScriptTable dispatchTable;
        if (!m_pScriptTable->GetValue(bClient ? CLIENT_DISPATCH_FIELD : SERVER_DISPATCH_FIELD, dispatchTable))
        {
            return false;
        }

        const char* funcData;
        if (!dispatchTable->GetAt(funcID + 1, funcData))
        {
            GameWarning("No such function dispatch index %d on entity %s (class %s)", funcID,
                m_pEntity->GetName(), m_pEntity->GetClass()->GetName());
            return false;
        }

        const char* colon = strchr(funcData, ':');
        if (colon == NULL)
        {
            return false;
        }
        if (colon - funcData > BUFFER)
        {
            return false;
        }
        memcpy(m_function, funcData, colon - funcData);
        m_function[colon - funcData] = 0;
        m_format = colon + 1;
        return true;
    }

    void Prologue(const char* function, const char* format)
    {
        azstrcpy(m_function, MaxSynchedPropertyNameLength, function);
        m_format = format;
    }

    const char* GetFormat() const { return m_format; }

    bool PerformCall(EntityId objID, CScriptRMISerialize* pSer, bool bClient, ChannelId channelId)
    {
        if (!InitGameMembers(objID))
        {
            return false;
        }

        SmartScriptTable callTable;
        if (!m_pScriptTable->GetValue(bClient ? "Client" : "Server", callTable))
        {
            return false;
        }

        m_pScriptSystem->BeginCall(callTable, m_function);
        m_pScriptSystem->PushFuncParam(m_pScriptTable);
        pSer->PushFunctionParams(m_pScriptSystem);
        m_pScriptSystem->PushFuncParam(channelId);
        return m_pScriptSystem->EndCall();
    }

private:
    bool InitGameMembers(EntityId id)
    {
        if (!m_pScriptSystem)
        {
            ISystem* pSystem = GetISystem();

            m_pEntitySystem = gEnv->pEntitySystem;
            IEntity* pEntity = m_pEntitySystem->GetEntity(id);
            if (!pEntity)
            {
                return false;
            }

            m_pScriptTable = pEntity->GetScriptTable();
            if (!m_pScriptTable.GetPtr())
            {
                return false;
            }

            m_pScriptSystem = pSystem->GetIScriptSystem();
        }
        return true;
    }

    IScriptSystem* m_pScriptSystem;
    IEntitySystem* m_pEntitySystem;
    IEntity* m_pEntity;
    SmartScriptTable m_pScriptTable;

    static const size_t BUFFER = 256;
    char m_function[BUFFER];
    const char* m_format;
};

void CScriptRMI::HandleGridMateScriptRMI(TSerialize serialize, ChannelId toChannelId, ChannelId avoidChannelId)
{
    const auto localChannelId = gEnv->pNetwork->GetLocalChannelId();

    if (avoidChannelId != kInvalidChannelId && avoidChannelId == localChannelId)
    {
        return;
    }

    if (toChannelId == kInvalidChannelId || toChannelId == localChannelId)
    {
        CScriptMessage msg;
        msg.SerializeHeader(serialize);

        if (msg.objId != kInvalidEntityId)
        {
            auto entityInfo = m_entities.find(msg.objId);
            if (entityInfo != m_entities.end())
            {
                auto& dispatch = m_dispatch[ entityInfo->second ];

                const char* fmt = msg.client ?
                    dispatch.client[ msg.funcId ].format :
                    dispatch.server[ msg.funcId ].format;

                msg.SetParamsFormat(fmt);
                msg.SerializeParameters(serialize);
            }
        }

        CCallHelper helper;
        if (helper.Prologue(msg.objId, msg.client, msg.funcId))
        {
            helper.PerformCall(msg.objId, &msg, msg.client, toChannelId);
        }
        else
        {
            GameWarning("Helper prologue failed, Discarding Script RMI.");
        }
    }
}

void CScriptRMI::OnInitClient(uint16 channelId, IEntity* pEntity)
{
    SmartScriptTable server;
    SmartScriptTable entity = pEntity->GetScriptTable();

    if (!entity.GetPtr())
    {
        return;
    }

    if (!entity->GetValue("Server", server) || !server.GetPtr())
    {
        return;
    }

    IScriptSystem* pSystem = entity->GetScriptSystem();
    if ((server->GetValueType("OnInitClient") == svtFunction) &&
        pSystem->BeginCall(server, "OnInitClient"))
    {
        pSystem->PushFuncParam(entity);
        pSystem->PushFuncParam(channelId);
        pSystem->EndCall();
    }
}

void CScriptRMI::OnPostInitClient(uint16 channelId, IEntity* pEntity)
{
    SmartScriptTable server;
    SmartScriptTable entity = pEntity->GetScriptTable();

    if (!entity.GetPtr())
    {
        return;
    }

    if (!entity->GetValue("Server", server) || !server.GetPtr())
    {
        return;
    }

    IScriptSystem* pSystem = entity->GetScriptSystem();
    if ((server->GetValueType("OnPostInitClient") == svtFunction) &&
        pSystem->BeginCall(server, "OnPostInitClient"))
    {
        pSystem->PushFuncParam(entity);
        pSystem->PushFuncParam(channelId);
        pSystem->EndCall();
    }
}


// add a dispatch proxy table to an entity
void CScriptRMI::AddProxyTable(IScriptTable* pEntityTable,
    ScriptHandle id, ScriptHandle flags,
    const char* name, SmartScriptTable dispatchTable)
{
    SmartScriptTable proxyTable(pEntityTable->GetScriptSystem());
    proxyTable->Delegate(dispatchTable.GetPtr());
    proxyTable->SetValue(FLAGS_FIELD, flags);
    proxyTable->SetValue(ID_FIELD, id);
    proxyTable->SetValue("__nopersist", true);
    pEntityTable->SetValue(name, proxyTable);
}

// add a synch proxy table to an entity
void CScriptRMI::AddSynchedTable(IScriptTable* pEntityTable,
    ScriptHandle id,
    const char* name, SmartScriptTable dispatchTable)
{
    SmartScriptTable synchedTable(pEntityTable->GetScriptSystem());
    SmartScriptTable hiddenTable(pEntityTable->GetScriptSystem());
    SmartScriptTable origTable;
    pEntityTable->GetValue(name, origTable);

    hiddenTable->Clone(dispatchTable);
    hiddenTable->SetValue("__index", hiddenTable);

    IScriptTable::Iterator iter = origTable->BeginIteration();
    while (origTable->MoveNext(iter))
    {
        if (iter.sKey)
        {
            if (hiddenTable->GetValueType(iter.sKey) != svtNull)
            {
                GameWarning("Replacing non-null value %s", iter.sKey);
            }

            ScriptAnyValue value;
            origTable->GetValueAny(iter.sKey, value);
            hiddenTable->SetValueAny(iter.sKey, value);
        }
    }
    origTable->EndIteration(iter);

    synchedTable->SetValue(ID_FIELD, id);
    synchedTable->SetValue(HIDDEN_FIELD, hiddenTable);
    synchedTable->Delegate(hiddenTable);
    pEntityTable->SetValue(name, synchedTable);
}

// send a single call to a channel - used for implementation of ProxyFunction
void CScriptRMI::DispatchRMI(ChannelId toChannelId, ChannelId avoidChannelId, _smart_ptr<CScriptMessage> pMsg, bool bClient)
{
    if (!m_pThis->m_pParent)
    {
        GameWarning("Trying to dispatch a remote method invocation with no game started... failing");
        return;
    }

    CCryAction* pFramework = CCryAction::GetCryAction();

    pMsg->client = bClient;

    if (toChannelId != gEnv->pNetwork->GetLocalChannelId())
    {
        gEnv->pNetwork->InvokeScriptRMI(&*pMsg, !bClient, toChannelId, avoidChannelId);
    }
    else
    {
        CCallHelper helper;
        if (!helper.Prologue(pMsg->objId, bClient, pMsg->funcId))
        {
            CRY_ASSERT_MESSAGE(0, "Helper prologue failed.");
            return;
        }

        helper.PerformCall(pMsg->objId,
            pMsg,
            bClient,
            toChannelId);
    }
}

// send a call
int CScriptRMI::ProxyFunction(IFunctionHandler* pH, void* pBuffer, int nSize)
{
    if (!m_pThis->m_pParent)
    {
        pH->GetIScriptSystem()->RaiseError("Trying to proxy a remote method invocation with no game started... failing");
        return pH->EndFunction();
    }

    string funcInfo;
    string dispatchInfo;
    bool gatherDebugInfo = pLogRMIEvents && pLogRMIEvents->GetIVal() != 0;

    IScriptSystem* pSS = pH->GetIScriptSystem();
    const SFunctionInfo* pFunctionInfo = static_cast<const SFunctionInfo*>(pBuffer);

    SmartScriptTable proxyTable;
    if (!pH->GetParam(1, proxyTable))
    {
        return pH->EndFunction(false);
    }

    ScriptHandle flags;
    ScriptHandle id;
    if (!proxyTable->GetValue(FLAGS_FIELD, flags))
    {
        return pH->EndFunction(false);
    }
    if (!proxyTable->GetValue(ID_FIELD, id))
    {
        return pH->EndFunction(false);
    }

    int formatStart = 2;
    if (uint32 flagsMask = flags.n & (eDF_ToClientOnChannel | eDF_ToClientOnOtherChannels))
    {
        if (flagsMask != (eDF_ToClientOnChannel | eDF_ToClientOnOtherChannels))
        {
            formatStart++;
        }
    }

    _smart_ptr<CScriptMessage> pMsg = new CScriptMessage(
            pFunctionInfo->reliability,
            pFunctionInfo->attachment,
            (EntityId)id.n,
            pFunctionInfo->funcID,
            pFunctionInfo->format);
    if (!pMsg->SetFromFunction(pH, formatStart))
    {
        return pH->EndFunction(false);
    }

    if (gatherDebugInfo)
    {
        funcInfo = pMsg->DebugInfo();
    }

    if (flags.n & eDF_ToServer)
    {
        DispatchRMI(gEnv->pNetwork->GetServerChannelId(), kInvalidChannelId, pMsg, false);
    }
    else if (flags.n & (eDF_ToClientOnChannel | eDF_ToClientOnOtherChannels))
    {
        ChannelId toChannelId = kInvalidChannelId;
        bool ok = true;
        if (flags.n != (eDF_ToClientOnChannel | eDF_ToClientOnOtherChannels))
        {
            if (!pH->GetParam(2, toChannelId))
            {
                pSS->RaiseError("RMI onClient or otherClients must have a valid channel id for its first argument");
                ok = false;
            }
        }
        if (ok)
        {
            ChannelId avoidChannelId = kInvalidChannelId;

            if (!!(flags.n & eDF_ToClientOnChannel) && !!(flags.n & eDF_ToClientOnOtherChannels))
            {
                toChannelId = kInvalidChannelId;
            }
            else if (!!(flags.n & eDF_ToClientOnOtherChannels))
            {
                avoidChannelId = toChannelId;
                toChannelId = kInvalidChannelId;
            }

            const char* funcName = pH->GetFuncName();

            //if (!gEnv->bServer)
            //{
            //    pSS->RaiseError("Client RMI %s should only be called from the server!", pH->GetFuncName());
            //}
            DispatchRMI(toChannelId, avoidChannelId, pMsg, true);
        }
    }

    if (gatherDebugInfo)
    {
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity((EntityId)id.n);
        const char* name = "<invalid>";
        if (pEntity)
        {
            name = pEntity->GetName();
        }
        CryLogAlways("[rmi] %s(%s) [%s] on entity[%d] %s",
            pH->GetFuncName(), funcInfo.c_str(), dispatchInfo.c_str(), (int)id.n, name);
    }

    return pH->EndFunction(true);
}

int CScriptRMI::SynchedNewIndexFunction(IFunctionHandler* pH)
{
    if (!m_pThis->m_pParent)
    {
        pH->GetIScriptSystem()->RaiseError("Trying to set a synchronized variable with no game started... failing");
        return pH->EndFunction();
    }

    SmartScriptTable table;
    SmartScriptTable hidden;
    const char* key;
    ScriptAnyValue value;

    if (!pH->GetParam(1, table))
    {
        return pH->EndFunction(false);
    }

    ScriptHandle id;
    if (!table->GetValue(ID_FIELD, id))
    {
        return pH->EndFunction(false);
    }
    if (!table->GetValue(HIDDEN_FIELD, hidden))
    {
        return pH->EndFunction(false);
    }

    if (!pH->GetParam(2, key))
    {
        return pH->EndFunction(false);
    }
    if (!pH->GetParamAny(3, value))
    {
        return pH->EndFunction(false);
    }

    hidden->SetValueAny(key, value);

    if (gEnv && gEnv->pNetwork)
    {
        gEnv->pNetwork->ChangedAspects((EntityId)id.n, eEA_Script);
    }

    return pH->EndFunction(true);
}

void CScriptRMI::GetMemoryStatistics(ICrySizer* s)
{
    CryAutoCriticalSection lk(m_dispatchMutex);

    s->AddObject(this, sizeof(*this));
    s->AddObject(m_entityClassToEntityTypeID);
    s->AddObject(m_entities);
    s->AddObject(m_dispatch);
}
