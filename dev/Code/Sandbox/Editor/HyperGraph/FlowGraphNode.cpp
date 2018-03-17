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
#include "FlowGraphNode.h"
#include "FlowGraphVariables.h"
#include "GameEngine.h"
#include "FlowGraph.h"
#include "Objects/EntityObject.h"
#include "Objects/Group.h"
#include "Prefabs/PrefabManager.h"
#include "Prefabs/PrefabEvents.h"
#include <AzCore/EBus/EBus.h>

#include <AzCore/EBus/EBus.h>
#include <MathConversion.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

#define FG_ALLOW_STRIPPED_PORTNAMES
//#undef  FG_ALLOW_STRIPPED_PORTNAMES

#define FG_WARN_ABOUT_STRIPPED_PORTNAMES
#undef  FG_WARN_ABOUT_STRIPPED_PORTNAMES

#define TITLE_COLOR_APPROVED       QColor(20, 200, 20)
#define TITLE_COLOR_ADVANCED       QColor(40, 40, 220)
#define TITLE_COLOR_DEBUG          QColor(220, 180, 20)
#define TITLE_COLOR_OBSOLETE       QColor(255, 0, 0)

//////////////////////////////////////////////////////////////////////////
CFlowNode::CFlowNode()
    : CHyperNode()
{
    m_flowNodeId = InvalidFlowNodeId;
    m_szDescription = "";
    m_entityGuid = GuidUtil::NullGuid;
    m_pEntity = NULL;
    m_needsEntityIdResolve = false;
}

//////////////////////////////////////////////////////////////////////////
CFlowNode::~CFlowNode()
{
    // Flow node deletion happens from multiple different paths (Sometimes notified from hypergraph listeners, from entity deletion, from prefab deletion
    // Notify prefab events manager about flownode removal here to cover all cases
    CPrefabManager* pPrefabManager = GetIEditor()->GetPrefabManager();
    if (pPrefabManager)
    {
        CPrefabEvents* pPrefabEvents = pPrefabManager->GetPrefabEvents();
        CRY_ASSERT(pPrefabEvents != NULL);
        pPrefabEvents->OnFlowNodeRemoval(this);
    }
}

//////////////////////////////////////////////////////////////////////////
IFlowGraph* CFlowNode::GetIFlowGraph() const
{
    CFlowGraph* pGraph = static_cast<CFlowGraph*> (GetGraph());
    return pGraph ? pGraph->GetIFlowGraph() : 0;
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::Init()
{
    QString sNodeName = QStringLiteral("<unknown>");

    if (m_name.isEmpty())
    {
        sNodeName = QString::number(GetId());
    }
    else
    {
        sNodeName = m_name;
    }
    m_flowNodeId = GetIFlowGraph()->CreateNode(m_classname.toUtf8().data(), sNodeName.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::Done()
{
    if (m_flowNodeId != InvalidFlowNodeId)
    {
        GetIFlowGraph()->RemoveNode(m_flowNodeId);
    }
    m_flowNodeId = InvalidFlowNodeId;
    m_pEntity = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::SetName(const QString& sName)
{
    if (GetIFlowGraph()->SetNodeName(m_flowNodeId, sName.toUtf8().data()))
    {
        CHyperNode::SetName(sName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::DebugPortActivation(TFlowPortId port, const char* value)
{
    m_portActivationMap[port] = string(value);
    m_debugPortActivations.push_back(port);

    CHyperGraph* pGraph = static_cast<CHyperGraph*> (GetGraph());
    if (pGraph)
    {
        IncrementDebugCount();
        pGraph->SendNotifyEvent(this, EHG_NODE_CHANGE_DEBUG_PORT);
    }
}

void CFlowNode::ResetDebugPortActivation(CHyperNodePort* port)
{
    std::vector<TFlowPortId>::iterator result = std::find(m_debugPortActivations.begin(), m_debugPortActivations.end(), port->nPortIndex);
    if (result != m_debugPortActivations.end())
    {
        m_debugPortActivations.erase(result);
    }
}

bool CFlowNode::IsDebugPortActivated(CHyperNodePort* port)
{
    std::vector<TFlowPortId>::iterator result = std::find(m_debugPortActivations.begin(), m_debugPortActivations.end(), port->nPortIndex);
    return (result != m_debugPortActivations.end());
}

//////////////////////////////////////////////////////////////////////////
bool CFlowNode::IsPortActivationModified(const CHyperNodePort* port)
{
    if (port)
    {
        const char* value = stl::find_in_map(m_portActivationMap, port->nPortIndex, NULL);
        if (value && strlen(value) > 0)
        {
            return true;
        }
        return false;
    }
    return m_portActivationMap.size() > 0;
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::ClearDebugPortActivation()
{
    ResetDebugCount();
    m_portActivationMap.clear();
    m_debugPortActivations.clear();
}

//////////////////////////////////////////////////////////////////////////
QString CFlowNode::GetDebugPortValue(const CHyperNodePort& pp)
{
    const char* value = stl::find_in_map(m_portActivationMap, pp.nPortIndex, NULL);
    if (value && strlen(value) > 0 && _stricmp(value, "out") && _stricmp(value, "unknown"))
    {
        QString portName = GetPortName(pp);
        portName += QStringLiteral("=");
        portName += value;
        return portName;
    }
    return GetPortName(pp);
}

//////////////////////////////////////////////////////////////////////////
CHyperNode* CFlowNode::Clone()
{
    CFlowNode* pNode = new CFlowNode;
    pNode->CopyFrom(*this);

    pNode->m_entityGuid = m_entityGuid;
    pNode->m_pEntity = m_pEntity;
    pNode->m_flowSystemNodeFlags = m_flowSystemNodeFlags;
    pNode->m_szDescription = m_szDescription;
    pNode->m_szUIClassName = m_szUIClassName;
    pNode->m_needsEntityIdResolve = m_needsEntityIdResolve;

#if 1 // AlexL: currently there is a per-variable-instance GetCustomItem interface
      // as there is a problem with undo
      // tell all input-port variables which node they belong to
      // output-port variables are not told (currently no need)
    Ports::iterator iter = pNode->m_inputs.begin();
    while (iter != pNode->m_inputs.end())
    {
        IVariable* pVar = (*iter).pVar;
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        if (pGetCustomItems != 0)
        {
            pGetCustomItems->SetFlowNode(pNode);
        }
        ++iter;
    }
#endif

    return pNode;
}

//////////////////////////////////////////////////////////////////////////
IUndoObject* CFlowNode::CreateUndo()
{
    // we create a full copy of the flowgraph. See comment in CFlowGraph::CUndoFlowGraph
    CHyperGraph* pGraph = static_cast<CHyperGraph*> (GetGraph());
    assert (pGraph != 0);
    return pGraph->CreateUndo();
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::SetSelectedEntity()
{
    CBaseObject* pSelObj = GetIEditor()->GetSelectedObject();
    if (qobject_cast<CEntityObject*>(pSelObj))
    {
        SetEntity((CEntityObject*)pSelObj);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::SetDefaultEntity()
{
    CEntityObject* pEntity = ((CFlowGraph*)GetGraph())->GetEntity();
    SetEntity(pEntity);
    SetFlag(EHYPER_NODE_GRAPH_ENTITY, true);
    SetFlag(EHYPER_NODE_GRAPH_ENTITY2, false);
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CFlowNode::GetDefaultEntity() const
{
    CEntityObject* pEntity = ((CFlowGraph*)GetGraph())->GetEntity();
    return pEntity;
}


//////////////////////////////////////////////////////////////////////////
void CFlowNode::SetEntity(CEntityObject* pEntity)
{
    SetFlag(EHYPER_NODE_ENTITY | EHYPER_NODE_ENTITY_VALID, true);
    SetFlag(EHYPER_NODE_GRAPH_ENTITY, false);
    SetFlag(EHYPER_NODE_GRAPH_ENTITY2, false);

    m_pEntity = pEntity;

    FlowEntityId id(FlowEntityId::s_invalidFlowEntityID);
    if (pEntity)
    {
        if (pEntity->GetType() == OBJTYPE_AZENTITY)
        {
            EBUS_EVENT_ID_RESULT(id, pEntity, AzToolsFramework::ComponentEntityObjectRequestBus, GetAssociatedEntityId);
            if (pEntity == GetDefaultEntity())
            {
                SetFlag(EHYPER_NODE_GRAPH_ENTITY, true);
            }
        }
        else
        {
            if (pEntity != GetDefaultEntity())
            {
                m_entityGuid = pEntity->GetId();
            }
            else
            {
                m_entityGuid = GuidUtil::NullGuid;
                SetFlag(EHYPER_NODE_GRAPH_ENTITY, true);
            }

            id = pEntity->GetEntityId();
        }
    }
    else
    {
        m_entityGuid = GuidUtil::NullGuid;
        SetFlag(EHYPER_NODE_ENTITY_VALID, false);
    }

    GetIFlowGraph()->SetEntityId(m_flowNodeId, id);
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CFlowNode::GetEntity() const
{
    if (m_pEntity && m_pEntity->CheckFlags(OBJFLAG_DELETED))
    {
        return 0;
    }
    return m_pEntity;
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::Unlinked(bool bInput)
{
    if (bInput)
    {
        SetInputs(false, true);
    }
    Invalidate(true);
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::SetInputs(bool bActivate, bool bForceResetEntities)
{
    IFlowGraph* pGraph = GetIFlowGraph();

    // small hack to make sure that entity id on target-entity nodes doesnt
    // get reset here
    int startPort = 0;
    if (CheckFlag(EHYPER_NODE_ENTITY))
    {
        startPort = 1;
    }

    for (int i = startPort; i < m_inputs.size(); i++)
    {
        IVariable* pVar = m_inputs[i].pVar;
        int type = pVar->GetType();
        if (type == IVariable::UNKNOWN)
        {
            continue;
        }

        // all variables which are NOT editable are not set!
        // e.g. EntityIDs
        if (bForceResetEntities == false && pVar->GetFlags() & IVariable::UI_DISABLED)
        {
            continue;
        }

        TFlowInputData value;
        bool bSet = false;

        switch (type)
        {
        case IVariable::INT:
        {
            int v;
            pVar->Get(v);
            bSet = value.Set(v);
        }
        break;
        case IVariable::BOOL:
        {
            bool v;
            pVar->Get(v);
            bSet = value.Set(v);
        }
        break;
        case IVariable::FLOAT:
        {
            float v;
            pVar->Get(v);
            bSet = value.Set(v);
        }
        break;
        case IVariable::VECTOR:
        {
            Vec3 v;
            pVar->Get(v);
            bSet = value.Set(v);
        }
        break;
        case IVariable::STRING:
        {
            QString v;
            pVar->Get(v);
            bSet = value.Set(string(v.toUtf8().data()));
            //string *str = value.GetPtr<string>();
        }
        break;
        case IVariable::FLOW_CUSTOM_DATA:
        {
            // Custom input values cannot currently be editited
            // in the ui, so no need to set them.
        }
        break;
        }
        if (bSet)
        {
            if (bActivate)
            {
                // we explicitly set the value here, because the flowgraph might not be enabled
                // in which case the ActivatePort would not set the value
                pGraph->SetInputValue(m_flowNodeId, i, value);
                pGraph->ActivatePort(SFlowAddress(m_flowNodeId, i, false), value);
            }
            else
            {
                pGraph->SetInputValue(m_flowNodeId, i, value);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::OnInputsChanged()
{
    SetInputs(true);
    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::OnEnteringGameMode()
{
    SetInputs(false);
}

//////////////////////////////////////////////////////////////////////////
QString CFlowNode::GetDescription() const
{
    if (m_szDescription && *m_szDescription)
    {
        return m_szDescription;
    }
    return QStringLiteral("");
}

//////////////////////////////////////////////////////////////////////////
QString CFlowNode::GetUIClassName() const
{
    if (m_szUIClassName.length())
    {
        return m_szUIClassName.c_str();
    }
    return GetClassName();
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    CHyperNode::Serialize(node, bLoading, ar);

    if (bLoading)
    {
#ifdef  FG_ALLOW_STRIPPED_PORTNAMES
        // backwards compatibility
        // variables (port values) are now saved with its real name

        XmlNodeRef portsNode = node->findChild("Inputs");
        if (portsNode)
        {
            for (int i = 0; i < m_inputs.size(); i++)
            {
                IVariable* pVar = m_inputs[i].pVar;
                if (pVar->GetType() != IVariable::UNKNOWN)
                {
                    if (portsNode->haveAttr(pVar->GetName().toUtf8().data()) == false)
                    {
                        // if we did not find a value for the variable we try to use the old name
                        // with the stripped version
                        QByteArray sVarName = pVar->GetName().toUtf8();
                        const char* sSpecial = strchr(sVarName.data(), '_');
                        if (sSpecial)
                        {
                            sVarName = sSpecial + 1;
                        }
                        const char* val = portsNode->getAttr(sVarName.data());
                        QByteArray name = GetGraph()->GetName().toUtf8();
                        if (val[0])
                        {
                            pVar->Set(val);
                            CryLogAlways("CFlowNode::Serialize: FG '%s': Value of Deprecated "
                                "Variable %s -> %s successfully resolved.",
                                name.data(),
                                sVarName,
                                pVar->GetName());
                            CryLogAlways(" --> To get rid of this warning re-save flowgraph!");
                        }
                        else
                        {
                            CryLogAlways("CFlowNode::Serialize: FG '%s': Can't resolve value for "
                                "'%s' <may not be severe>",
                                name.data(),
                                sVarName.data());
                        }
                    }
                }
            }
        }
#endif

        SetInputs(false);

        m_nFlags &= ~(EHYPER_NODE_GRAPH_ENTITY | EHYPER_NODE_GRAPH_ENTITY2);

        m_pEntity = 0;
        if (CheckFlag(EHYPER_NODE_ENTITY))
        {
            const bool bIsAIOrCustomAction = (m_pGraph != 0 && ((m_pGraph->GetAIAction() != 0) || (m_pGraph->GetCustomAction() != 0)));
            if (bIsAIOrCustomAction)
            {
                SetEntity(0);
            }
            else
            {
                uint64 entityIdU64;
                if (node->getAttr("ComponentEntityId", entityIdU64))
                {
                    if (!IsLegacyEntityId(AZ::EntityId(entityIdU64)))
                    {
                        GetIFlowGraph()->SetEntityId(m_flowNodeId, FlowEntityId(AZ::EntityId(entityIdU64)));
                        CEntityObject* entityObject = nullptr;
                        EBUS_EVENT_ID_RESULT(entityObject, AZ::EntityId(entityIdU64), AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);
                        if (entityObject)
                        {
                            SetEntity(entityObject);
                        }
                        else
                        {
                            m_needsEntityIdResolve = true;
                        }
                    }
                    else
                    {
                        EntityId legacyEntityId = static_cast<EntityId>(entityIdU64);
                        SetEntity(CEntityObject::FindFromEntityId(legacyEntityId));
                    }
                }
                else if (node->getAttr("EntityGUID", m_entityGuid))
                {
                    // If empty GUId then it is a default graph entity.
                    if (GuidUtil::IsEmpty(m_entityGuid))
                    {
                        SetFlag(EHYPER_NODE_GRAPH_ENTITY, true);
                        SetEntity(GetDefaultEntity());
                    }
                    else
                    {
                        GUID origGuid = m_entityGuid;
                        if (ar)
                        {
                            // use ar to remap ids
                            m_entityGuid = ar->ResolveID(m_entityGuid);
                        }
                        CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(m_entityGuid);
                        if (!pObj)
                        {
                            SetEntity(NULL);
                            // report error
                            CErrorRecord err;
                            err.module = VALIDATOR_MODULE_FLOWGRAPH;
                            err.severity = CErrorRecord::ESEVERITY_WARNING;
                            err.error = QObject::tr("FlowGraph %1 Node %2 (Class %3) targets unknown entity %4")
                                .arg(m_pGraph->GetName()).arg(GetId()).arg(GetClassName()).arg(GuidUtil::ToString(origGuid));
                            err.pItem = 0;
                            GetIEditor()->GetErrorReport()->ReportError(err);
                        }
                        else if (qobject_cast<CEntityObject*>(pObj))
                        {
                            SetEntity((CEntityObject*)pObj);
                        }
                    }
                }
                else
                {
                    SetEntity(NULL);
                }
            }
            if (node->haveAttr("GraphEntity"))
            {
                const char* sEntity = node->getAttr("GraphEntity");
                int index = atoi(node->getAttr("GraphEntity"));
                if (index == 0)
                {
                    SetFlag(EHYPER_NODE_GRAPH_ENTITY, true);
                    if (bIsAIOrCustomAction == false)
                    {
                        // Set the entity pointer if it's available. Otherwise it will be resolved when the graph entity is set
                        // in CFlowGraph::SetEntity.
                        CEntityObject* pEntity = GetDefaultEntity();
                        if (pEntity)
                        {
                            SetEntity(pEntity);
                        }
                    }
                }
                else
                {
                    SetFlag(EHYPER_NODE_GRAPH_ENTITY2, true);
                }
            }
        }
    }
    else // saving
    {
        if (CheckFlag(EHYPER_NODE_GRAPH_ENTITY) || CheckFlag(EHYPER_NODE_GRAPH_ENTITY2))
        {
            int index = 0;
            if (CheckFlag(EHYPER_NODE_GRAPH_ENTITY2))
            {
                index = 1;
            }
            node->setAttr("GraphEntity", index);
        }
        else
        {
            // If we are exporting the level we should pick the unique entity guid no matter wether we are part of a prefab or not
            // During level export they are getting expanded as normal objects. Runtime prefabs on the other hand are not.
            const bool isLevelExportingInProgress = GetIEditor()->GetObjectManager() ? GetIEditor()->GetObjectManager()->IsExportingLevelInprogress() : false;

            if (m_pEntity)
            {
                if (m_pEntity->GetType() == OBJTYPE_AZENTITY)
                {
                    AZ::EntityId id;
                    EBUS_EVENT_ID_RESULT(id, m_pEntity, AzToolsFramework::ComponentEntityObjectRequestBus, GetAssociatedEntityId);
                    node->setAttr("ComponentEntityId", static_cast<AZ::u64>(id));
                }
            }

            if (!GuidUtil::IsEmpty(m_entityGuid))
            {
                if (m_pEntity && m_pEntity->IsPartOfPrefab() && !isLevelExportingInProgress)
                {
                    GUID guidInPrefab = m_pEntity->GetIdInPrefab();

                    node->setAttr("EntityGUID", guidInPrefab);
                    EntityGUID entid64 = ToEntityGuid(guidInPrefab);
                    if (entid64)
                    {
                        node->setAttr("EntityGUID_64", entid64);
                    }
                }
                else
                {
                    node->setAttr("EntityGUID", m_entityGuid);
                    EntityGUID entid64 = ToEntityGuid(m_entityGuid);
                    if (entid64)
                    {
                        node->setAttr("EntityGUID_64", entid64);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowNode::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    // fix up all references
    CBaseObject* pOld = GetIEditor()->GetObjectManager()->FindObject(m_entityGuid);
    CBaseObject* pNew = ctx.FindClone(pOld);
    if (qobject_cast<CEntityObject*>(pNew))
    {
        SetEntity((CEntityObject*)pNew);
    }
}

//////////////////////////////////////////////////////////////////////////
QColor CFlowNode::GetCategoryColor()
{
    // This color coding is used to identify critical and obsolete nodes in the flowgraph editor. [Jan Mueller]
    int32 category = GetCategory();
    switch (category)
    {
    case EFLN_APPROVED:
        return TITLE_COLOR_APPROVED;
    //return CHyperNode::GetCategoryColor();
    case EFLN_ADVANCED:
        return TITLE_COLOR_ADVANCED;
    case EFLN_DEBUG:
        return TITLE_COLOR_DEBUG;
    case EFLN_OBSOLETE:
        return TITLE_COLOR_OBSOLETE;
    default:
        return CHyperNode::GetCategoryColor();
    }
}


//////////////////////////////////////////////////////////////////////////
bool CFlowNode::IsEntityValid() const
{
    return CheckFlag(EHYPER_NODE_ENTITY_VALID) && m_pEntity && !m_pEntity->CheckFlags(OBJFLAG_DELETED);
}


//////////////////////////////////////////////////////////////////////////
QString CFlowNode::GetEntityTitle() const
{
    if (CheckFlag(EHYPER_NODE_ENTITY))
    {
        // Check if entity port is connected.
        if (m_inputs.size() > 0 && m_inputs[0].nConnected != 0)
        {
            return QStringLiteral("<Input Entity>");
        }
    }
    if (CheckFlag(EHYPER_NODE_GRAPH_ENTITY))
    {
        if (m_pGraph->GetAIAction() != 0)
        {
            return QStringLiteral("<User Entity>");
        }
        else
        {
            return QStringLiteral("<Graph Entity>");
        }
    }
    else if (CheckFlag(EHYPER_NODE_GRAPH_ENTITY2))
    {
        if (m_pGraph->GetAIAction() != 0)
        {
            return QStringLiteral("<Object Entity>");
        }
        else
        {
            return QStringLiteral("<Graph Entity 2>");
        }
    }

    CEntityObject* pEntity = GetEntity();
    if (pEntity)
    {
        if (pEntity->GetGroup())
        {
            QString name = QStringLiteral("[");
            name += pEntity->GetGroup()->GetName();
            name += QStringLiteral("] ");
            name += pEntity->GetName();
            return name;
        }
        else
        {
            return pEntity->GetName();
        }
    }
    return QStringLiteral("Choose Entity");
}

void CFlowNode::ResolveEntityId()
{
    if (m_needsEntityIdResolve)
    {
        FlowEntityId entityId = GetIFlowGraph()->GetEntityId(m_flowNodeId);
        CEntityObject* entityObject = nullptr;
        EBUS_EVENT_ID_RESULT(entityObject, entityId, AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);
        AZ_Assert(entityObject, "Did not find entity with the ID %d ", entityId);
        if (entityObject)
        {
            SetEntity(entityObject);
        }

        m_needsEntityIdResolve = false;
    }
}

//////////////////////////////////////////////////////////////////////////
QString CFlowNode::GetPortName(const CHyperNodePort& port)
{
    //IFlowGraph *pGraph = GetIFlowGraph();

    //const TFlowInputData *pInputValue = pGraph->GetInputValue( m_flowNodeId,port.nPortIndex );

    if (port.bInput)
    {
        QString text = port.pVar->GetHumanName();
        if (port.pVar->GetType() != IVariable::UNKNOWN && !port.nConnected)
        {
            text = text + QStringLiteral("=") + VarToValue(port.pVar);

            /*
            if (pInputValue)
            {
                switch (pInputValue->GetType())
                {
                case eFDT_Int:
                    break;
                case eFDT_Float:
                    break;
                }
            }
            */
        }
        return text;
    }
    return port.pVar->GetHumanName();
}

QString CFlowNode::GetCategoryName() const
{
    const uint32 cat = GetCategory();
    switch (cat)
    {
    case EFLN_APPROVED:
        return QStringLiteral("Release");
    case EFLN_ADVANCED:
        return QStringLiteral("Advanced");
    case EFLN_DEBUG:
        return QStringLiteral("Debug");
    //case EFLN_WIP:
    //  return QStringLiteral("WIP");
    //case EFLN_LEGACY:
    //  return QStringLiteral("Legacy");
    //case EFLN_NOCATEGORY:
    //  return QStringLiteral("No Category");
    case EFLN_OBSOLETE:
        return QStringLiteral("Obsolete");
    default:
        return QStringLiteral("UNDEFINED!");
    }
}

//////////////////////////////////////////////////////////////////////////
CVarBlock* CFlowNode::GetInputsVarBlock()
{
    CVarBlock* pVarBlock = CHyperNode::GetInputsVarBlock();

#if 0 // commented out, because currently there is per-variable IGetCustomItems
      // which is set on node creation (in CFlowNode::Clone)
    for (int i = 0; i < pVarBlock->GetVarsCount(); ++i)
    {
        IVariable* pVar = pVarBlock->GetVariable(i);
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData());
        if (pGetCustomItems != 0)
        {
            pGetCustomItems->SetFlowNode(this);
        }
    }
#endif

    return pVarBlock;
}

//////////////////////////////////////////////////////////////////////////
QString CFlowNode::GetTitle() const
{
    QString title;
    if (m_name.isEmpty())
    {
        title = GetUIClassName();

#if 0 // show full group:class
        {
            int p = title.Find(':');
            if (p >= 0)
            {
                title = title.Mid(p + 1);
            }
        }
#endif

#if 0
        // Hack: drop AI: from AI: nodes (does not look nice)
        {
            int p = title.Find("AI:");
            if (p >= 0)
            {
                title = title.Mid(p + 3);
            }
        }
#endif
    }
    else
    {
        title = m_name + QStringLiteral(" (") + m_classname + QStringLiteral(")");
    }

    return title;
}


//////////////////////////////////////////////////////////////////////////
TFlowNodeTypeId CFlowNode::GetTypeId()
{
    if (GetFlowNodeId() == InvalidFlowNodeId || !GetIFlowGraph())
    {
        return InvalidFlowNodeTypeId;
    }

    IFlowNodeData* pData = GetIFlowGraph()->GetNodeData(GetFlowNodeId());
    if (pData)
    {
        return pData->GetNodeTypeId();
    }

    return InvalidFlowNodeTypeId;
}
