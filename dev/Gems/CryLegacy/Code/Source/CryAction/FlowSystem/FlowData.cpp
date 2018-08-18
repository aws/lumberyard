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
#include "FlowData.h"
#include "FlowSerialize.h"
#include "IEntityPoolManager.h"

// do some magic stuff to be backwards compatible with flowgraphs
// which don't use the REAL port name but a stripped version

#define FG_ALLOW_STRIPPED_PORTNAMES
// #undef  FG_ALLOW_STRIPPED_PORTNAMES

#define FG_WARN_ABOUT_STRIPPED_PORTNAMES
#undef  FG_WARN_ABOUT_STRIPPED_PORTNAMES

// Maximum number of output ports allowed for dynamic output
#define DYNAMIC_OUTPUT_MAX (64)

namespace
{
    // Placeholder for proper validation function in CryEngine xml handling
    // Note: This function is not comprehensive, replace with validation suite asap!
    bool IsValidXmlAttributeName(const char* str)
    {
        return strpbrk(str, " <>&\"'[]{}()!@#$%^*") == nullptr;
    }
}

CFlowData::CFlowData()
    : m_pInputData(nullptr)
    , m_pOutputFirstEdge(nullptr)
    , m_pImpl(nullptr)
    , m_getFlowgraphForwardingEntity(nullptr)
    , m_nInputs(0)
    , m_nOutputs(0)
    , m_forwardingEntityID(0)
    , m_typeId(0)
    , m_hasEntity(false)
    , m_failedGettingFlowgraphForwardingEntity(false)
{
}

CFlowData::CFlowData(IFlowNodePtr pImpl, const string& name, TFlowNodeTypeId typeId)
    : m_pInputData(nullptr)
    , m_pOutputFirstEdge(nullptr)
    , m_pImpl(pImpl)
    , m_name(name)
    , m_getFlowgraphForwardingEntity(0)
    , m_nInputs(0)
    , m_nOutputs(0)
    , m_forwardingEntityID(0)
    , m_typeId(typeId)
    , m_failedGettingFlowgraphForwardingEntity(false)
{
    SFlowNodeConfig config;
    GetConfiguration(config);
    m_hasEntity = (0 != (config.nFlags & EFLN_TARGET_ENTITY));
}

CFlowData::~CFlowData()
{
    if (m_getFlowgraphForwardingEntity)
    {
        gEnv->pScriptSystem->ReleaseFunc(m_getFlowgraphForwardingEntity);
    }
    SAFE_DELETE_ARRAY(m_pInputData);
    SAFE_DELETE_ARRAY(m_pOutputFirstEdge);
}

CFlowData::CFlowData(const CFlowData& rhs)
{
    bool bCopy = true;
    if (NULL != rhs.m_pImpl)
    {
        SFlowNodeConfig config;
        rhs.DoGetConfiguration(config);
        if (EFLN_DYNAMIC_OUTPUT == (config.nFlags & EFLN_DYNAMIC_OUTPUT))
        {
            m_pImpl = rhs.m_pImpl->Clone(NULL);
            bCopy = false;
        }
    }
    if (true == bCopy)
    {
        m_pImpl = rhs.m_pImpl;
    }

    m_nInputs = rhs.m_nInputs;
    m_nOutputs = rhs.m_nOutputs;

    m_pInputData = new TFlowInputData[m_nInputs];
    m_pOutputFirstEdge = new int[m_nOutputs];

    for (int i = 0; i < m_nInputs; ++i)
    {
        m_pInputData[i] = rhs.m_pInputData[i];
    }
    for (int i = 0; i < m_nOutputs; ++i)
    {
        m_pOutputFirstEdge[i] = rhs.m_pOutputFirstEdge[i];
    }

    m_name = rhs.m_name;
    m_typeId = rhs.m_typeId;
    m_hasEntity = rhs.m_hasEntity;
    m_failedGettingFlowgraphForwardingEntity = true;
    m_getFlowgraphForwardingEntity = 0;
    m_forwardingEntityID = 0;
}

void CFlowData::DoGetConfiguration(SFlowNodeConfig& config) const
{
    const int MAX_INPUT_PORTS = 64;

    // get the node's internal configuration...
    m_pImpl->GetConfiguration(config);

    {
        ScopedSwitchToGlobalHeap globalHeap;
        static SInputPortConfig* inputs = new SInputPortConfig[MAX_INPUT_PORTS];
        SInputPortConfig* pInput = inputs;

        // check if it needs an Entity port...
        if (config.nFlags & EFLN_TARGET_ENTITY)
        {
            *pInput++ = InputPortConfig<FlowEntityId>("entityId", _HELP("Changes the attached entity dynamically"));
        }

        // check if it needs an Activate port...
        if (config.nFlags & EFLN_ACTIVATION_INPUT)
        {
            *pInput++ = InputPortConfig<SFlowSystemVoid>("Activate", _HELP("Trigger this node"));
        }

        // if we added an input...
        if (pInput != inputs)
        {
            if (config.pInputPorts)
            {
                while (config.pInputPorts->name)
                {
                    CRY_ASSERT(pInput != inputs + MAX_INPUT_PORTS);
                    *pInput++ = *config.pInputPorts++;
                }
            }

            SInputPortConfig nullInput = {0};
            *pInput++ = nullInput;

            config.pInputPorts = inputs;
        }
    }
}

void CFlowData::CountConfigurationPorts(const SFlowNodeConfig& config, int& numInputs, int& numOutputs) const
{
    numInputs = 0;
    numOutputs = 0;

    if (config.pInputPorts)
    {
        while (config.pInputPorts[numInputs].name)
        {
            if (!IsValidXmlAttributeName(config.pInputPorts[numInputs].name))
            {
                CRY_ASSERT_TRACE(IsValidXmlAttributeName(config.pInputPorts[m_nInputs].name), ("Programmer error! Node \"%s\" has an input port with an illegal name \"%s\".  Truncating input port list!", m_name.c_str(), config.pInputPorts[m_nInputs].name));
                break;
            }
            ++numInputs;
        }
    }

    if (config.nFlags & EFLN_DYNAMIC_OUTPUT)
    {
        numOutputs = DYNAMIC_OUTPUT_MAX;
    }
    else if (config.pOutputPorts)
    {
        while (config.pOutputPorts[numOutputs].name)
        {
            // ly-note: the reason we don't need to sanitize output port names is that they aren't serialized to xml
            // as attributes, but input port names are.  refer to CL#23721
            ++numOutputs;
        }
    }
}

void CFlowData::Swap(CFlowData& rhs)
{
    std::swap(m_nInputs, rhs.m_nInputs);
    std::swap(m_nOutputs, rhs.m_nOutputs);
    std::swap(m_pInputData, rhs.m_pInputData);
    std::swap(m_pOutputFirstEdge, rhs.m_pOutputFirstEdge);
    std::swap(m_pImpl, rhs.m_pImpl);
    m_name.swap(rhs.m_name);
    TFlowNodeTypeId typeId = rhs.m_typeId;
    rhs.m_typeId = m_typeId;
    m_typeId = typeId;
    bool temp;
    temp = rhs.m_hasEntity;
    rhs.m_hasEntity = m_hasEntity;
    m_hasEntity = temp;
    temp = rhs.m_failedGettingFlowgraphForwardingEntity;
    rhs.m_failedGettingFlowgraphForwardingEntity = m_failedGettingFlowgraphForwardingEntity;
    m_failedGettingFlowgraphForwardingEntity = temp;

    std::swap(m_forwardingEntityID, rhs.m_forwardingEntityID);
    std::swap(m_getFlowgraphForwardingEntity, rhs.m_getFlowgraphForwardingEntity);
}

CFlowData& CFlowData::operator =(const CFlowData& rhs)
{
    CFlowData temp(rhs);
    Swap(temp);
    return *this;
}

bool CFlowData::ResolvePort(const char* name, TFlowPortId& port, bool isOutput)
{
    SFlowNodeConfig config;
    DoGetConfiguration(config);
    if (isOutput)
    {
        for (int i = 0; i < m_nOutputs; i++)
        {
            const char* sPortName = config.pOutputPorts[i].name;

            if (NULL != sPortName && !azstricmp(name, sPortName))
            {
                port = i;
                return true;
            }
            if (NULL == sPortName)
            {
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < m_nInputs; i++)
        {
            const char* sPortName = config.pInputPorts[i].name;

            if (NULL != sPortName && !azstricmp(name, sPortName))
            {
                port = i;
                return true;
            }

#ifdef FG_ALLOW_STRIPPED_PORTNAMES
            // fix for t_ stuff in current graphs these MUST NOT be stripped!
            if (sPortName != 0 && sPortName[0] == 't' && sPortName[1] == '_')
            {
                if (!azstricmp(name, sPortName))
                {
                    port = i;
                    return true;
                }
            }
            else if (sPortName)
            {
                // strip special char '_' and text before it!
                // it defines special data type only...
                const char* sSpecial = strchr(sPortName, '_');
                if (sSpecial)
                {
                    sPortName = sSpecial + 1;
                }

                if (!azstricmp(name, sPortName))
                {
#ifdef FG_WARN_ABOUT_STRIPPED_PORTNAMES
                    CryLogAlways("[flow] CFlowData::ResolvePort: Deprecation warning for port name: '%s' should be '%s'",
                        sPortName, config.pInputPorts[i].name);
                    CryLogAlways("[flow] Solution is to resave flowgraph in editor!");
#endif
                    port = i;
                    return true;
                }
            }
#endif
        }
    }

    return false;
}

bool CFlowData::SerializeXML(IFlowNode::SActivationInfo* pActInfo, const XmlNodeRef& node, bool reading)
{
    SFlowNodeConfig config;
    DoGetConfiguration(config);
    if (reading)
    {
        if (XmlNodeRef inputsNode = node->findChild("Inputs"))
        {
            for (int i = m_hasEntity; i < m_nInputs; i++)
            {
                const char* value = inputsNode->getAttr(config.pInputPorts[i].name);
                if (value[0])
                {
                    SetFromString(m_pInputData[i], value);
                }
#ifdef FG_ALLOW_STRIPPED_PORTNAMES
                else
                {
                    // strip special char '_' and text before it!
                    // it defines special data type only...
                    const char* sPortName = config.pInputPorts[i].name;
                    const char* sSpecial = strchr(sPortName, '_');
                    if (sSpecial)
                    {
                        sPortName = sSpecial + 1;
                    }

                    value = inputsNode->getAttr(sPortName);
                    if (value[0])
                    {
#ifdef FG_WARN_ABOUT_STRIPPED_PORTNAMES
                        CryLogAlways("[flow] CFlowData::SerializeXML: Deprecation warning for port name: '%s' should be '%s'",
                            sPortName, config.pInputPorts[i].name);
                        CryLogAlways("[flow] Solution is to resave flowgraph in editor!");
#endif
                        SetFromString(m_pInputData[i], value);
                    }
                }
#endif
            }
        }

        if (config.nFlags & EFLN_DYNAMIC_OUTPUT)
        {
            XmlNodeRef outputsNode = node->findChild("Outputs");
            if (outputsNode)
            {
                // Ugly way to hide this
                m_pImpl->SerializeXML(pActInfo, outputsNode, true);
                DoGetConfiguration(config);

                // Recalculate output size
                if (NULL == config.pOutputPorts)
                {
                    m_nOutputs = 0;
                }
                else
                {
                    for (m_nOutputs = 0; config.pOutputPorts[m_nOutputs].name; m_nOutputs++)
                    {
                        ;
                    }
                }
                SAFE_DELETE_ARRAY(m_pOutputFirstEdge);
                m_pOutputFirstEdge = new int[m_nOutputs];
            }
        }

        if (config.nFlags & EFLN_TARGET_ENTITY)
        {
            FlowEntityId entityId;
            const char* sGraphEntity = node->getAttr("GraphEntity");
            if (*sGraphEntity != 0)
            {
                int nIndex = atoi(sGraphEntity);
                if (nIndex == 0)
                {
                    if (SetEntityId((FlowEntityId)EFLOWNODE_ENTITY_ID_GRAPH1))
                    {
                        pActInfo->pGraph->ActivateNode(pActInfo->myID);
                    }
                }
                else
                {
                    if (SetEntityId((FlowEntityId)EFLOWNODE_ENTITY_ID_GRAPH2))
                    {
                        pActInfo->pGraph->ActivateNode(pActInfo->myID);
                    }
                }
            }
            else
            {
                if (node->haveAttr("EntityGUID_64"))
                {
                    EntityGUID entGuid = 0;
                    node->getAttr("EntityGUID_64", entGuid);
                    entityId = gEnv->pEntitySystem->FindEntityByGuid(entGuid);
                    if (entityId)
                    {
                        if (SetEntityId(FlowEntityId(entityId)))
                        {
                            pActInfo->pGraph->ActivateNode(pActInfo->myID);
                        }
                    }
                    else
                    {
                        const char* pClassName = node->getAttr("Class");
                        if (!pClassName)
                        {
                            pClassName = "[UNKNOWN]";
                        }
                        EntityId graphEntityId = pActInfo->pGraph->GetGraphEntity(0);
                        IEntity* pGraphEntity = gEnv->pEntitySystem->GetEntity(graphEntityId);
#if defined(_MSC_VER)

                        GameWarning("Flow Graph Node targets unknown entity guid: %I64x  nodeclass: '%s'    graph entity id: %d name: '%s'", entGuid, pClassName, graphEntityId, pGraphEntity ? pGraphEntity->GetName() : "[NULL]");
#else
                        GameWarning("Flow Graph Node targets unknown entity guid: %llx  nodeclass: '%s'     graph entity id: %d name '%s'", (long long)entGuid, pClassName, graphEntityId, pGraphEntity ? pGraphEntity->GetName() : "[NULL]");
#endif
                    }
                }
            }

            if (node->haveAttr("ComponentEntityId"))
            {
                uint64 entityIdU64;
                node->getAttr("ComponentEntityId", entityIdU64);

                FlowEntityId id = FlowEntityId(static_cast<AZ::EntityId>(entityIdU64));

                if (SetEntityId(id))
                {
                    pActInfo->pGraph->ActivateNode(pActInfo->myID);
                }
            }
        }
    }

    return true;
}

void CFlowData::Serialize(IFlowNode::SActivationInfo* pActInfo, TSerialize ser)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Flowgraph serialization");
    SFlowNodeConfig config;
    DoGetConfiguration(config);

    int startIndex = 0;

    ser.Value("m_forwardingEntityID", m_forwardingEntityID);

    // need to make sure we activate the node if we change the entity id due to serialization
    // so that the node ends up knowing
    if (ser.IsReading())
    {
        if (m_hasEntity)
        {
            startIndex = 1;
            TFlowInputData data;
            ser.ValueWithDefault("in_0", data, config.pInputPorts[0].defaultData);

            // StuartM: 07/01/10: we should only activate and re-set the EntityId if it actually changes
            // otherwise nodes like Entity:EntityId will re-trigger on quickload for no reason and confusing other nodes
            EntityId oldId = *(m_pInputData[0].GetPtr<EntityId>());
            EntityId newId = *data.GetPtr<EntityId>();

            if (oldId != newId)
            {
                SetEntityId(FlowEntityId(newId));
                pActInfo->pGraph->ActivateNode(pActInfo->myID);
            }
        }

        if (m_forwardingEntityID != 0)
        {
            m_failedGettingFlowgraphForwardingEntity = true;
        }
    }

    char idName[16] = "in_x";
    for (int i = startIndex; i < m_nInputs; i++)
    {
        azitoa(i, &idName[3], AZ_ARRAY_SIZE(idName) - 3, 10);
        ser.ValueWithDefault(idName, m_pInputData[i], config.pInputPorts[i].defaultData);
    }

    CompleteActivationInfo(pActInfo);
    m_pImpl->Serialize(pActInfo, ser);
}

void CFlowData::PostSerialize(IFlowNode::SActivationInfo* pActInfo)
{
    CompleteActivationInfo(pActInfo);
    m_pImpl->PostSerialize(pActInfo);
    if (m_forwardingEntityID != 0)
    {
        pActInfo->pEntity = gEnv->pEntitySystem->GetEntity(m_forwardingEntityID);
        if (pActInfo->pEntity)
        {
            m_pImpl->ProcessEvent(IFlowNode::eFE_SetEntityId, pActInfo);
        }
    }
}


void CFlowData::FlagInputPorts()
{
    for (int i = 0; i < m_nInputs; i++)
    {
        m_pInputData[i].SetUserFlag(true);
    }
}

void CFlowData::GetConfiguration(SFlowNodeConfig& config)
{
    DoGetConfiguration(config);

    int numInputs = 0;
    int numOutputs = 0;
    CountConfigurationPorts(config, numInputs, numOutputs);

    if (config.nFlags & EFLN_DYNAMIC_OUTPUT)
    {
        numOutputs = DYNAMIC_OUTPUT_MAX;
    }

    if (m_nInputs != numInputs)
    {
        SAFE_DELETE_ARRAY(m_pInputData);
        m_nInputs = numInputs;
        m_pInputData = new TFlowInputData[m_nInputs];
        for (int i = 0; i < m_nInputs; i++)
        {
            CRY_ASSERT(config.pInputPorts);
            m_pInputData[i] = config.pInputPorts[i].defaultData;
        }
    }

    if (m_nOutputs != numOutputs)
    {
        SAFE_DELETE_ARRAY(m_pOutputFirstEdge);
        m_nOutputs = numOutputs;
        m_pOutputFirstEdge = new int[m_nOutputs];
    }
}

bool CFlowData::SetEntityId(const FlowEntityId id)
{
    if (m_hasEntity)
    {
        m_pInputData[0].SetUnlocked();
        m_pInputData[0].Set(id);
        m_pInputData[0].SetUserFlag(true);
        m_pInputData[0].SetLocked();

        if (m_getFlowgraphForwardingEntity)
        {
            gEnv->pScriptSystem->ReleaseFunc(m_getFlowgraphForwardingEntity);
        }
        m_getFlowgraphForwardingEntity = 0;
        m_failedGettingFlowgraphForwardingEntity = true;
        if (id == 0)
        {
            m_forwardingEntityID = 0;
        }
    }
    return m_hasEntity;
}

bool CFlowData::NoForwarding(IFlowNode::SActivationInfo* pActInfo)
{
    if (m_forwardingEntityID)
    {
        m_pImpl->ProcessEvent(IFlowNode::eFE_SetEntityId, pActInfo);

        m_forwardingEntityID = 0;
    }
    return false;
}

bool CFlowData::ForwardingActivated(IFlowNode::SActivationInfo* pActInfo, IFlowNode::EFlowEvent event)
{
    bool forwardingDone = DoForwardingIfNeed(pActInfo);

    if (forwardingDone)
    {
        m_pImpl->ProcessEvent(event, pActInfo);
        forwardingDone = HasForwarding(pActInfo);
    }

    return false;
}



bool CFlowData::DoForwardingIfNeed(IFlowNode::SActivationInfo* pActInfo)
{
    CompleteActivationInfo(pActInfo);

    bool isForwarding = m_getFlowgraphForwardingEntity != NULL;

    if (m_failedGettingFlowgraphForwardingEntity)
    {
        IEntity* pEntity = pActInfo->pEntity;
        if (pEntity)
        {
            SmartScriptTable scriptTable = pEntity->GetScriptTable();
            if (!!scriptTable)
            {
                scriptTable->GetValue("GetFlowgraphForwardingEntity", m_getFlowgraphForwardingEntity);
                m_failedGettingFlowgraphForwardingEntity = false;
                isForwarding = m_getFlowgraphForwardingEntity != NULL;
            }
            else
            {
                return NoForwarding(pActInfo);
            }
        }
        else
        {
            if (!m_forwardingEntityID) // the original entity could not be accesible because is bookmarked. in this case we want to keep trying (this is rare. can happen only for waveAIs, only after a savegame load, only if that savegame happened after at least one AI was re-spawned, and only until next AI is spawned from the same original one). So performance is not a problem
            {
                return NoForwarding(pActInfo);
            }
            isForwarding = true;
        }
    }

    if (!isForwarding)
    {
        return NoForwarding(pActInfo);
    }

    IEntity* pEntity = pActInfo->pEntity;
    EntityId forwardingEntityID = 0;
    if (pEntity)
    {
        SmartScriptTable pTable = pEntity->GetScriptTable();
        if (!pTable)
        {
            return NoForwarding(pActInfo);
        }

        ScriptHandle ent(0);
        if (!Script::CallReturn(pTable->GetScriptSystem(), m_getFlowgraphForwardingEntity, pTable, ent))
        {
            return NoForwarding(pActInfo);
        }

        forwardingEntityID = (EntityId)ent.n;
    }
    else
    {  // if the pEntity does not exist because was bookmarked, we keep forwarding to the last forwarded id
        EntityId initialId = GetEntityId();
        if (initialId && gEnv->pEntitySystem->GetIEntityPoolManager()->IsEntityBookmarked(initialId))
        {
            forwardingEntityID = m_forwardingEntityID;
        }
        else
        {
            return NoForwarding(pActInfo);
        }
    }

    if (forwardingEntityID)
    {
        pActInfo->pEntity = gEnv->pEntitySystem->GetEntity(forwardingEntityID);

        if (m_forwardingEntityID != forwardingEntityID)
        {
            m_pImpl->ProcessEvent(IFlowNode::eFE_SetEntityId, pActInfo);

            m_forwardingEntityID = forwardingEntityID;
        }
    }

    return true;
}



