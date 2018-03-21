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
#include "FlowGraphManager.h"
#include "FlowGraph.h"
#include "FlowGraphNode.h"
#include "FlowGraphModuleManager.h"
#include "TrackEventNode.h"
#include "FlowGraphVariables.h"
#include "IconManager.h"
#include "GameEngine.h"

#include "HyperGraphDialog.h"

#include <IEntitySystem.h>
#include <IFlowSystem.h>
#include <IFlowGraphModuleManager.h>
#include <IAIAction.h>

#include "Objects/EntityObject.h"

#include "UIEnumsDatabase.h"
#include "Util/Variable.h"

#include "Prefabs/PrefabManager.h"
#include "Prefabs/PrefabEvents.h"
#include "Objects/PrefabObject.h"
#include "Prefabs/PrefabItem.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

#define FLOWGRAPH_BITMAP_FOLDER "Editor\\Icons\\FlowGraph\\"

#define FG_INTENSIVE_DEBUG
#undef  FG_INTENSIVE_DEBUG

#undef GetClassName

static XmlNodeRef g_flowgraphDocumentationData = NULL;

//////////////////////////////////////////////////////////////////////////
void WriteFlowgraphDocumentation(IConsoleCmdArgs* /* pArgs */)
{
    g_flowgraphDocumentationData = gEnv->pSystem->CreateXmlNode("FlowGraphDocumentationRoot");

    GetIEditor()->GetFlowGraphManager()->ReloadClasses();
    g_flowgraphDocumentationData->saveToFile("@LOG@/flowgraphdocu.xml");

    g_flowgraphDocumentationData->removeAllChilds();
    g_flowgraphDocumentationData = NULL;
}

void ReloadFlowgraphClasses(IConsoleCmdArgs* /* pArgs */)
{
    GetIEditor()->GetFlowGraphManager()->ReloadClasses();
}

void WriteFlownodeToDocumentation(IFlowNodeData* pFlowNodeData, const IFlowNodeTypeIterator::SNodeType& nodeType)
{
    SFlowNodeConfig config;
    pFlowNodeData->GetConfiguration(config);

    string name = nodeType.typeName;
    int seperator = name.find(':');
    if (seperator == 0)
    {
        seperator = name.length();
    }

    //create node entry, split class and name

    //find or add class
    XmlNodeRef flowNodeParent = g_flowgraphDocumentationData->findChild(name.substr(0, seperator));
    if (!flowNodeParent)
    {
        flowNodeParent = gEnv->pSystem->CreateXmlNode(name.substr(0, seperator));
        g_flowgraphDocumentationData->addChild(flowNodeParent);
    }

    //add new node with given name
    string nodeName = name.substr(seperator + 1, name.length() - seperator - 1);
    if (nodeName.empty())
    {
        return;
    }

    if (*nodeName.begin() >= '0' && *nodeName.begin() <= '9')
    {
        nodeName = "_" + nodeName;
    }

    XmlNodeRef flowNodeData = gEnv->pSystem->CreateXmlNode(nodeName.c_str());
    flowNodeData->setAttr("Description", config.sDescription ? config.sDescription : "");

    //Input subgroup
    XmlNodeRef inputs = gEnv->pSystem->CreateXmlNode("Inputs");
    int iInputs = pFlowNodeData->GetNumInputPorts();
    for (int input = 0; input < iInputs; ++input)
    {
        const SInputPortConfig& port = config.pInputPorts[input];
        if (!port.name)
        {
            break;
        }

        name = port.name;
        if (*name.begin() >= '0' && *name.begin() <= '9')
        {
            name = "_" + name;
        }
        name.replace(' ',  '_');

        inputs->setAttr(name.c_str(), port.description ? port.description : "");
    }
    flowNodeData->addChild(inputs);

    //Output subgroup
    XmlNodeRef outputs = gEnv->pSystem->CreateXmlNode("Outputs");
    int iOutputs = pFlowNodeData->GetNumOutputPorts();
    for (int output = 0; output < iOutputs; ++output)
    {
        const SOutputPortConfig& port = config.pOutputPorts[output];
        if (!port.name)
        {
            break;
        }

        name = port.name;
        if (*name.begin() > '0' && *name.begin() <= '9')
        {
            name = "_" + name;
        }
        name.replace(' ', '_');

        outputs->setAttr(name.c_str(), port.description ? port.description : "");
    }
    flowNodeData->addChild(outputs);

    flowNodeParent->addChild(flowNodeData);
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphManager::~CFlowGraphManager()
{
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    FlowGraphNotificationBus::Handler::BusDisconnect();

    while (!m_graphs.empty())
    {
        CFlowGraph* pFG = m_graphs.back();
        if (pFG)
        {
            pFG->OnHyperGraphManagerDestroyed();
            UnregisterGraph(pFG);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::Init()
{
    CHyperGraphManager::Init();

    REGISTER_COMMAND("fg_writeDocumentation", WriteFlowgraphDocumentation, VF_NULL, "Write flownode documentation file.");
    REGISTER_COMMAND("fg_reloadClasses", ReloadFlowgraphClasses, VF_NULL, "Reloads all flowgraph nodes.");

    // Enumerate all flow graph classes.
    ReloadClasses();

    FlowGraphNotificationBus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::OnEnteringGameMode(bool inGame)
{
    if (!inGame)
    {
        return;
    }

    for (int i = 0; i < m_graphs.size(); ++i)
    {
        m_graphs[i]->OnEnteringGameMode();
    }
}

//////////////////////////////////////////////////////////////////////////
CHyperGraph* CFlowGraphManager::CreateGraph()
{
    CFlowGraph* pGraph = new CFlowGraph(this);
    return pGraph;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::RegisterGraph(CFlowGraph* pGraph)
{
    if (stl::find(m_graphs, pGraph) == true)
    {
        CryLogAlways("CFlowGraphManager::RegisterGraph: OLD Graph 0x%p", pGraph);
        assert (false);
    }
    else
    {
#ifdef FG_INTENSIVE_DEBUG
        CryLogAlways("CFlowGraphManager::RegisterGraph: NEW Graph 0x%p", pGraph);
#endif
        m_graphs.push_back(pGraph);
    }
    // assert (stl::find(m_graphs, pGraph) == false);
    SendNotifyEvent(EHG_GRAPH_ADDED, pGraph);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::UnregisterGraph(CFlowGraph* pGraph)
{
#ifdef FG_INTENSIVE_DEBUG
    CryLogAlways("CFlowGraphManager::UnregisterGraph: Graph 0x%p", pGraph);
#endif
    stl::find_and_erase(m_graphs, pGraph);
    SendNotifyEvent(EHG_GRAPH_REMOVED, pGraph);
}

//////////////////////////////////////////////////////////////////////////
CHyperNode* CFlowGraphManager::CreateNode(CHyperGraph* pGraph, const char* sNodeClass, HyperNodeID nodeId, const QPointF&  pos, CBaseObject* pObj, bool bAllowMissing)
{
    // AlexL: ignore pos, as a SetPos would screw up Undo. And it will be set afterwards anyway

    CHyperNode* pNode = 0;

    CFlowGraph* pFlowGraph = (CFlowGraph*)pGraph;
    // Check for special node classes.
    if (pObj)
    {
        pNode = CreateSelectedEntityNode(pFlowGraph, pObj);
    }
    else if (strcmp(sNodeClass, "default_entity") == 0)
    {
        pNode = CreateEntityNode(pFlowGraph, pFlowGraph->GetEntity());
    }
    else
    {
        pNode = CHyperGraphManager::CreateNode(pGraph, sNodeClass, nodeId, pos, pObj, bAllowMissing);
    }

    return pNode;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::ReloadClasses()
{
    // since some nodes could be changed / removed
    // serialize all FlowGraphs, and then restore state after reloading
    std::vector<IUndoObject*> m_CurrentFgUndos;
    m_CurrentFgUndos.resize(m_graphs.size());
    for (int i = 0; i < m_graphs.size(); ++i)
    {
        m_CurrentFgUndos[i] = m_graphs[i]->CreateUndo();
    }

    m_prototypes.clear();

    IFlowSystem* pFlowSystem = GetIEditor()->GetGameEngine()->GetIFlowSystem();

    IFlowGraphPtr pFlowGraph = pFlowSystem->CreateFlowGraph();

    IFlowNodeTypeIteratorPtr pTypeIterator = pFlowSystem->CreateNodeTypeIterator();
    IFlowNodeTypeIterator::SNodeType nodeType;
    while (pTypeIterator->Next(nodeType))
    {
        CFlowNode* pProtoFlowNode = NULL;
        if (strcmp(nodeType.typeName, TRACKEVENT_CLASS) == 0)
        {
            pProtoFlowNode = new CTrackEventNode;
        }
        else
        {
            pProtoFlowNode = new CFlowNode;
        }
        pProtoFlowNode->SetClass(nodeType.typeName);
        m_prototypes[nodeType.typeName] = pProtoFlowNode;

        TFlowNodeId nodeId = pFlowGraph->CreateNode(nodeType.typeName, nodeType.typeName);
        IFlowNodeData* pFlowNodeData = pFlowGraph->GetNodeData(nodeId);
        assert(pFlowNodeData);

        GetNodeConfig(pFlowNodeData, pProtoFlowNode);

        if (g_flowgraphDocumentationData)
        {
            WriteFlownodeToDocumentation(pFlowNodeData, nodeType);
        }
    }

    // restore all loaded FlowGraphs
    for (int i = 0; i < m_CurrentFgUndos.size(); ++i)
    {
        m_CurrentFgUndos[i]->Undo(false);
        delete m_CurrentFgUndos[i];
    }
}

enum UIEnumType
{
    eUI_None,
    eUI_Int,
    eUI_Float,
    eUI_String,
    eUI_GlobalEnum,
    eUI_GlobalEnumRef,
    eUI_GlobalEnumDef,
};

namespace
{
    float GetValue(const QString& s, const char* token)
    {
        float fValue = 0.0f;
        int pos = s.indexOf(token);
        if (pos >= 0)
        {
            // fill in Enum Pairs
            pos = s.indexOf('=', pos + 1);
            if (pos >= 0)
            {
                sscanf(s.toUtf8().data() + pos + 1, "%f", &fValue);
            }
        }
        return fValue;
    }
}

UIEnumType ParseUIConfig(const char* sUIConfig, std::map<QString, QString>& outEnumPairs, Vec3& uiValueRanges)
{
    UIEnumType enumType = eUI_None;
    QString uiConfig (sUIConfig);

    // ranges
    uiValueRanges.x = GetValue(uiConfig, "v_min");
    uiValueRanges.y = GetValue(uiConfig, "v_max");

    int pos = uiConfig.indexOf(':');
    if (pos <= 0)
    {
        return enumType;
    }

    QString enumTypeS = uiConfig.mid(0, pos);
    if (enumTypeS == "enum_string")
    {
        enumType = eUI_String;
    }
    else if (enumTypeS == "enum_int")
    {
        enumType = eUI_Int;
    }
    else if (enumTypeS == "enum_float")
    {
        enumType = eUI_Float;
    }
    else if (enumTypeS == "enum_global")
    {
        // don't do any tokenzing. "enum_global:ENUM_NAME"
        enumType = eUI_GlobalEnum;
        QString enumName = uiConfig.mid(pos + 1);
        if (enumName.isEmpty() == false)
        {
            outEnumPairs[enumName] = enumName;
        }
        return enumType;
    }
    else if (enumTypeS == "enum_global_def")
    {
        // don't do any tokenzing. "enum_global_def:ENUM_NAME"
        enumType = eUI_GlobalEnumDef;
        QString enumName = uiConfig.mid(pos + 1);
        if (enumName.isEmpty() == false)
        {
            outEnumPairs[enumName] = enumName;
        }
        return enumType;
    }
    else if (enumTypeS == "enum_global_ref")
    {
        // don't do any tokenzing. "enum_global_ref:ENUM_NAME_FORMAT_STRING:REF_PORT"
        enumType = eUI_GlobalEnumRef;
        int pos1 = uiConfig.indexOf(':', pos + 1);
        if (pos1 < 0)
        {
            Warning(QObject::tr("FlowGraphManager: Wrong enum_global_ref format while parsing UIConfig '%1'").arg(sUIConfig).toUtf8().data());
            return eUI_None;
        }

        QString enumFormat = uiConfig.mid(pos + 1, pos1 - pos - 1);
        QString enumPort = uiConfig.mid(pos1 + 1);
        if (enumFormat.isEmpty() || enumPort.isEmpty())
        {
            Warning(QObject::tr("FlowGraphManager: Wrong enum_global_ref format while parsing UIConfig '%1'").arg(sUIConfig).toUtf8().data());
            return eUI_None;
        }
        outEnumPairs[enumFormat] = enumPort;
        return enumType;
    }

    if (enumType != eUI_None)
    {
        // fill in Enum Pairs
        QString values = uiConfig.mid(pos + 1);
        pos = 0;
        for (auto resToken : values.split(",", QString::SkipEmptyParts))
        {
            QString str = resToken.trimmed();
            QString value = str;
            int pos_e = str.indexOf('=');
            if (pos_e >= 0)
            {
                value = str.mid(pos_e + 1);
                str = str.mid(0, pos_e);
            }
            outEnumPairs[str] = value;
        }
    }

    return enumType;
}

template<typename T>
void FillEnumVar (CVariableEnum<T>* pVar, const std::map<QString, QString>& nameValueMap)
{
    var_type::type_convertor convertor;
    T val;
    for (std::map<QString, QString>::const_iterator iter = nameValueMap.begin(); iter != nameValueMap.end(); ++iter)
    {
        convertor (iter->second, val);
        //int val = atoi (iter->second.GetString());
        pVar->AddEnumItem(iter->first, val);
    }
}


//////////////////////////////////////////////////////////////////////////
IVariable* CFlowGraphManager::MakeInVar(const SInputPortConfig* pPortConfig, uint32 portId, CFlowNode* pFlowNode)
{
    int type = pPortConfig->defaultData.GetType();
    if (!pPortConfig->defaultData.IsLocked())
    {
        type = eFDT_Any;
    }

    IVariable* pVar = 0;
    // create variable

    CFlowNodeGetCustomItemsBase* pGetCustomItems = 0;

    const char* name = pPortConfig->name;
    bool isEnumDataType = false;

    Vec3 uiValueRanges (ZERO);

    // UI Parsing
    if (pPortConfig->sUIConfig != 0)
    {
        isEnumDataType = true;
        std::map<QString, QString> enumPairs;
        UIEnumType type = ParseUIConfig(pPortConfig->sUIConfig, enumPairs, uiValueRanges);
        switch (type)
        {
        case eUI_Int:
        {
            CVariableFlowNodeEnum<int>* pEnumVar = new CVariableFlowNodeEnum<int>();
            FillEnumVar<int> (pEnumVar, enumPairs);
            pVar = pEnumVar;
        }
        break;
        case eUI_Float:
        {
            CVariableFlowNodeEnum<float>* pEnumVar = new CVariableFlowNodeEnum<float>();
            FillEnumVar<float> (pEnumVar, enumPairs);
            pVar = pEnumVar;
        }
        break;
        case eUI_String:
        {
            CVariableFlowNodeEnum<QString>* pEnumVar = new CVariableFlowNodeEnum<QString>();
            FillEnumVar<QString> (pEnumVar, enumPairs);
            pVar = pEnumVar;
        }
        break;
        case eUI_GlobalEnum:
        {
            CVariableFlowNodeEnum<QString>* pEnumVar = new CVariableFlowNodeEnum<QString>();
            pEnumVar->SetFlags(IVariable::UI_USE_GLOBAL_ENUMS);     // FIXME: is not really needed. may be dropped
            QString globalEnumName;
            if (enumPairs.empty())
            {
                globalEnumName = name;
            }
            else
            {
                globalEnumName = enumPairs.begin()->first;
            }

            CVarGlobalEnumList* pEnumList = new CVarGlobalEnumList(globalEnumName);
            pEnumVar->SetEnumList(pEnumList);
            pVar = pEnumVar;
        }
        break;
        case eUI_GlobalEnumRef:
        {
            assert (enumPairs.size() == 1);
            CVariableFlowNodeDynamicEnum* pEnumVar = new CVariableFlowNodeDynamicEnum (enumPairs.begin()->first, enumPairs.begin()->second);
            pEnumVar->SetFlags(IVariable::UI_USE_GLOBAL_ENUMS);     // FIXME: is not really needed. may be dropped
            pVar = pEnumVar;
            // take care of custom item base
            pGetCustomItems = new CFlowNodeGetCustomItemsBase;
            pGetCustomItems->SetUIConfig(pPortConfig->sUIConfig);
        }
        break;
        case eUI_GlobalEnumDef:
        {
            assert (enumPairs.size() == 1);
            CVariableFlowNodeDefinedEnum* pEnumVar = new CVariableFlowNodeDefinedEnum(enumPairs.begin()->first, portId);
            pEnumVar->SetFlags(IVariable::UI_USE_GLOBAL_ENUMS);     // FIXME: is not really needed. may be dropped
            pVar = pEnumVar;
            // take care of custom item base
            pGetCustomItems = new CFlowNodeGetCustomItemsBase;
            pGetCustomItems->SetUIConfig(pPortConfig->sUIConfig);
        }
        break;
        default:
            isEnumDataType = false;
            break;
        }
    }

    // Make a simple var, it was no enum type
    if (pVar == 0)
    {
        pVar = MakeSimpleVarFromFlowType(type);
    }

    // Set ranges if applicable
    if (uiValueRanges.x != 0.0f || uiValueRanges.y != 0.0f)
    {
        pVar->SetLimits(uiValueRanges.x, uiValueRanges.y, 0.f, true, true);
    }

    // Take care of predefined datatypes
    if (!isEnumDataType)
    {
        const FlowGraphVariables::MapEntry* mapEntry = 0;
        if (pPortConfig->sUIConfig != 0)
        {
            // parse UI config, if a certain datatype is specified
            // ui_dt=datatype
            QString uiConfig (pPortConfig->sUIConfig);
            const char* prefix = "dt=";
            const int pos = uiConfig.indexOf(prefix);
            if (pos >= 0)
            {
                QString dt = uiConfig.mid(pos + 3); // 3==strlen(prefix)
                dt = dt.left(dt.indexOf(QRegularExpression("[ ,]")));
                dt += "_";
                mapEntry = FlowGraphVariables::FindEntry(dt.toUtf8().data());
            }
        }
        if (mapEntry == 0)
        {
            mapEntry = FlowGraphVariables::FindEntry(pPortConfig->name);
            if (mapEntry != 0)
            {
                name += strlen(mapEntry->sPrefix);
            }
        }

        if (mapEntry != 0)
        {
            pVar->SetDataType(mapEntry->eDataType);
            if (mapEntry->eDataType == IVariable::DT_USERITEMCB)
            {
                assert (pGetCustomItems == 0);
                assert (mapEntry->pGetCustomItemsCreator != 0);
                pGetCustomItems = mapEntry->pGetCustomItemsCreator();
                pGetCustomItems->SetUIConfig(pPortConfig->sUIConfig);
            }
        }
    }

    // Set Name of Variable
    pVar->SetName(pPortConfig->name);   // ALEXL 08/11/05: from now on we always use the REAL port name

    // Set HumanName of Variable
    if (pPortConfig->humanName)
    {
        pVar->SetHumanName(pPortConfig->humanName);
    }
    else
    {
        pVar->SetHumanName(name);  // if there is no human name we set the 'name' (stripped prefix if it was a predefined data type!)
    }
    // Set variable description
    if (pPortConfig->description)
    {
        pVar->SetDescription(pPortConfig->description);
    }

    if (pGetCustomItems)
    {
        pGetCustomItems->AddRef();
    }

    pVar->SetUserData(QVariant::fromValue<void *>(pGetCustomItems));

    return pVar;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::GetNodeConfig(IFlowNodeData* pSrcNode, CFlowNode* pFlowNode)
{
    if (!pSrcNode)
    {
        return;
    }
    pFlowNode->ClearPorts();
    SFlowNodeConfig config;
    pSrcNode->GetConfiguration(config);

    if (config.nFlags & EFLN_TARGET_ENTITY)
    {
        pFlowNode->SetFlag(EHYPER_NODE_ENTITY, true);
    }
    if (config.nFlags & EFLN_HIDE_UI)
    {
        pFlowNode->SetFlag(EHYPER_NODE_HIDE_UI, true);
    }
    if (config.nFlags & EFLN_UNREMOVEABLE)
    {
        pFlowNode->SetFlag(EHYPER_NODE_UNREMOVEABLE, true);
    }

    pFlowNode->m_flowSystemNodeFlags = config.nFlags;
    if (pFlowNode->GetCategory() != EFLN_APPROVED)
    {
        pFlowNode->SetFlag(EHYPER_NODE_CUSTOM_COLOR1, true);
    }

    pFlowNode->m_szDescription = config.sDescription; // can be 0
    if (config.sUIClassName)
    {
        pFlowNode->m_szUIClassName = config.sUIClassName;
    }

    if (config.pInputPorts)
    {
        const SInputPortConfig* pPortConfig = config.pInputPorts;
        uint32 portId = 0;
        while (pPortConfig->name)
        {
            CHyperNodePort port;
            port.bInput = true;

            int type = pPortConfig->defaultData.GetType();
            if (!pPortConfig->defaultData.IsLocked())
            {
                type = eFDT_Any;
            }

            port.pVar = MakeInVar(pPortConfig, portId, pFlowNode);

            if (port.pVar)
            {
                switch (type)
                {
                case eFDT_Bool:
                    port.pVar->Set(*pPortConfig->defaultData.GetPtr<bool>());
                    break;
                case eFDT_Int:
                    port.pVar->Set(*pPortConfig->defaultData.GetPtr<int>());
                    break;
                case eFDT_Float:
                    port.pVar->Set(*pPortConfig->defaultData.GetPtr<float>());
                    break;
                case eFDT_Double:
                    port.pVar->Set(*pPortConfig->defaultData.GetPtr<double>());
                    break;
                case eFDT_EntityId:
                    port.pVar->Set((int)*pPortConfig->defaultData.GetPtr<EntityId>());
                    port.pVar->SetFlags(port.pVar->GetFlags() | IVariable::UI_DISABLED);
                    break;
                case eFDT_Vec3:
                    port.pVar->Set(*pPortConfig->defaultData.GetPtr<Vec3>());
                    break;
                case eFDT_String:
                    port.pVar->Set((pPortConfig->defaultData.GetPtr<string>())->c_str());
                    break;
                case eFDT_CustomData:
                    string tmp = "(hidden)";
                    pPortConfig->defaultData.GetPtr<FlowCustomData>()->GetAs(tmp);
                    port.pVar->SetDisplayValue(tmp.c_str());
                    break;
                }

                pFlowNode->AddPort(port);
            }
            ++pPortConfig;
            ++portId;
        }
    }
    if (config.pOutputPorts)
    {
        const SOutputPortConfig* pPortConfig = config.pOutputPorts;
        while (pPortConfig->name)
        {
            CHyperNodePort port;
            port.bInput = false;
            port.bAllowMulti = true;
            port.pVar = MakeSimpleVarFromFlowType(pPortConfig->type);
            if (port.pVar)
            {
                port.pVar->SetName(pPortConfig->name);
                if (pPortConfig->description)
                {
                    port.pVar->SetDescription(pPortConfig->description);
                }
                if (pPortConfig->humanName)
                {
                    port.pVar->SetHumanName(pPortConfig->humanName);
                }
                pFlowNode->AddPort(port);
            }
            ++pPortConfig;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IVariable* CFlowGraphManager::MakeSimpleVarFromFlowType(int type)
{
    switch (type)
    {
    case eFDT_Void:
        return new CVariableFlowNodeVoid();
        break;
    case eFDT_Bool:
        return new CVariableFlowNode<bool>();
        break;
    case eFDT_Int:
        return new CVariableFlowNode<int>();
        break;
    case eFDT_Float:
        return new CVariableFlowNode<float>();
        break;
    case eFDT_Double:
        return new CVariableFlowNode<double>();
        break;
    case eFDT_EntityId:
        return new CVariableFlowNode<int>();
        break;
    case eFDT_Vec3:
        return new CVariableFlowNode<Vec3>();
        break;
    case eFDT_String:
        return new CVariableFlowNode<QString>();
        break;
    case eFDT_CustomData:
        return new CVariableFlowNodeCustomData();
        break;
    default:
        // Any type.
        return new CVariableFlowNodeVoid();
        break;
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::CreateGraphForEntity(CEntityObject* pEntity, const char* sGroupName)
{
    CFlowGraph* pFlowGraph = new CFlowGraph(this, sGroupName);
    pFlowGraph->SetEntity(pEntity);
#ifdef FG_INTENSIVE_DEBUG
    CryLogAlways("CFlowGraphManager::CreateGraphForEntity: graph=0x%p entity=0x%p guid=%s", pFlowGraph, pEntity, GuidUtil::ToString(pEntity->GetId()));
#endif
    return pFlowGraph;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::FindGraphForAction(IAIAction* pAction)
{
    for (int i = 0; i < m_graphs.size(); ++i)
    {
        if (m_graphs[i]->GetAIAction() == pAction)
        {
            return m_graphs[i];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::CreateGraphForAction(IAIAction* pAction)
{
    CFlowGraph* pFlowGraph = new CFlowGraph(this);
    pFlowGraph->GetIFlowGraph()->SetSuspended(true);
    pFlowGraph->SetAIAction(pAction);
    return pFlowGraph;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::FreeGraphsForActions()
{
    int i = m_graphs.size();
    while (i--)
    {
        if (m_graphs[i]->GetAIAction())
        {
            // delete will call UnregisterGraph and remove itself from the vector
            //  assert( m_graphs[i]->NumRefs() == 1 );
            m_graphs[i]->SetAIAction(NULL);
            m_graphs[i]->SetName("");
            m_graphs[i]->SetName("Default");
            m_graphs[i]->Release();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::CreateGraphForModule(IFlowGraphModule* pModule)
{
    // make sure this graph is connected to the underlying module graph
    assert(pModule);
    if (pModule)
    {
        QString filename = pModule->GetPath();
        IFlowGraph* pIGraph = pModule->GetRootGraph();

        CFlowGraph* pFlowGraph = new CFlowGraph(this, pIGraph);
        pFlowGraph->GetIFlowGraph()->SetType(IFlowGraph::eFGT_Module);
        pFlowGraph->SetName(pModule->GetName());
        pFlowGraph->Load(filename.toUtf8().data());

        QString groupName = filename.toLower();

        // if module is in a subfolder, extract the folder name
        //  to use as a group in the FG editor window
        QString moduleFolder = "flowgraphmodules\\";
        const int moduleFolderLength = moduleFolder.length();
        int offset = groupName.indexOf(moduleFolder);
        int offset2 = groupName.indexOf("\\", offset + moduleFolderLength);

        if (offset != -1 && offset2 != -1)
        {
            groupName.truncate(offset2);
            groupName = groupName.right(offset2 - offset - moduleFolderLength);
            pFlowGraph->SetGroupName(groupName);
        }

        return pFlowGraph;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::FindGraph(IFlowGraphPtr pFG)
{
    for (int i = 0; i < m_graphs.size(); ++i)
    {
        if (m_graphs[i]->GetIFlowGraph() == pFG.get())
        {
            return m_graphs[i];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::FindGraphForCustomAction(ICustomAction* pCustomAction)
{
    for (int i = 0; i < m_graphs.size(); ++i)
    {
        if (m_graphs[i]->GetCustomAction() == pCustomAction)
        {
            return m_graphs[i];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::CreateGraphForCustomAction(ICustomAction* pCustomAction)
{
    CFlowGraph* pFlowGraph = new CFlowGraph(this);
    pFlowGraph->GetIFlowGraph()->SetSuspended(true);
    pFlowGraph->SetCustomAction(pCustomAction);
    return pFlowGraph;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::FreeGraphsForCustomActions()
{
    int i = m_graphs.size();
    while (i--)
    {
        if (m_graphs[i]->GetCustomAction())
        {
            // delete will call UnregisterGraph and remove itself from the vector
            //  assert( m_graphs[i]->NumRefs() == 1 );
            m_graphs[i]->SetCustomAction(NULL);
            m_graphs[i]->SetName("");
            m_graphs[i]->SetName("Default");
            m_graphs[i]->Release();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CFlowNode* CFlowGraphManager::CreateSelectedEntityNode(CFlowGraph* pFlowGraph, CBaseObject* pSelObj)
{
    if (pSelObj)
    {
        if (qobject_cast<CEntityObject*>(pSelObj))
        {
            return CreateEntityNode(pFlowGraph, (CEntityObject*)pSelObj);
        }
        else if (qobject_cast<CPrefabObject*>(pSelObj))
        {
            return CreatePrefabInstanceNode(pFlowGraph, (CPrefabObject*)pSelObj);
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph* CFlowGraphManager::CreateGraphForMatFX(IFlowGraphPtr pFG, const QString& filename)
{
    CFlowGraph* pFlowGraph = new CFlowGraph(this, pFG.get(), PathUtil::GetFileName(filename.toUtf8().data()));
    pFlowGraph->GetIFlowGraph()->SetType(IFlowGraph::eFGT_MaterialFx);
    pFlowGraph->Load(filename.toUtf8().data());

    return pFlowGraph;
}

//////////////////////////////////////////////////////////////////////////
CFlowNode* CFlowGraphManager::CreateEntityNode(CFlowGraph* pFlowGraph, CEntityObject* pEntity)
{
    if (!pEntity)
    {
        return 0;
    }

    string classname;
    if (pEntity->GetIEntity())
    {
        classname = string("entity:") + pEntity->GetIEntity()->GetClass()->GetName();
    }
    else
    {
        classname = "Entity:EntityId";
    }


    CFlowNode* pNode = (CFlowNode*)CHyperGraphManager::CreateNode(pFlowGraph, classname.c_str(), pFlowGraph->AllocateId());
    if (pNode)
    {
        // check if we add it to an AIAction
        if (pFlowGraph->GetAIAction() == 0)
        {
            pNode->SetEntity(pEntity);
        }
        else
        {
            pNode->SetEntity(0); // AIAction -> reset entity
        }
        IFlowNodeData* pFlowNodeData = pFlowGraph->GetIFlowGraph()->GetNodeData(pNode->GetFlowNodeId());
        GetNodeConfig(pFlowNodeData, pNode);
    }

    return pNode;
}

//////////////////////////////////////////////////////////////////////////
CFlowNode* CFlowGraphManager::CreatePrefabInstanceNode(CFlowGraph* pFlowGraph, CPrefabObject* pPrefabObj)
{
    if (!pPrefabObj)
    {
        return 0;
    }

    CFlowNode* pNode = (CFlowNode*)CHyperGraphManager::CreateNode(pFlowGraph, "Prefab:Instance", pFlowGraph->AllocateId());
    if (pNode)
    {
        // Associate flownode to the prefab
        CPrefabManager* pPrefabManager = GetIEditor()->GetPrefabManager();
        CRY_ASSERT(pPrefabManager != NULL);
        if (pPrefabManager)
        {
            CPrefabEvents* pPrefabEvents = pPrefabManager->GetPrefabEvents();
            CRY_ASSERT(pPrefabEvents != NULL);
            bool bResult = pPrefabEvents->AddPrefabInstanceNodeFromSelection(pNode, pPrefabObj);
            if (!bResult)
            {
                Warning("FlowGraphManager: Failed to add prefab instance node for prefab instance: %s", pPrefabObj->GetName());
            }
        }
    }

    return pNode;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::OpenFlowGraphView(IFlowGraph* pIFlowGraph)
{
    assert(pIFlowGraph);
    CFlowGraph* pFlowGraph = new CFlowGraph(this, pIFlowGraph);
    OpenView(pFlowGraph);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::UnregisterAndResetView(CFlowGraph* pFlowGraph, bool bInitFlowGraph)
{
    if (pFlowGraph)
    {
        // CryLogAlways("CFlowGraphManager::UnregisterAndResetView: Graph to 0x%p", pFlowGraph);

        CHyperGraphDialog* pHgDlg = CHyperGraphDialog::instance();
        if (pHgDlg)
        {
            pHgDlg->DeleteItem(pFlowGraph);
        }

        UnregisterGraph(pFlowGraph);

        if (pHgDlg && bInitFlowGraph && pHgDlg->GetGraph() == pFlowGraph)
        {
            pHgDlg->SetGraph(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::GetAvailableGroups(std::set<QString>& outGroups, bool bActionGraphs)
{
    std::vector<CFlowGraph*>::iterator iter = m_graphs.begin();
    while (iter != m_graphs.end())
    {
        CFlowGraph* pGraph = *iter;
        if (pGraph != 0)
        {
            if (bActionGraphs == false)
            {
                CEntityObject* pEntity = pGraph->GetEntity();
                if (pEntity)
                {
                    if (pEntity->GetType() != OBJTYPE_AZENTITY)
                    {
                        if (pEntity->CheckFlags(OBJFLAG_PREFAB) == false)
                        {
                            QString groupName = pGraph->GetGroupName();
                            if (groupName.isEmpty() == false)
                            {
                                outGroups.insert(groupName);
                            }
                        }
                    }
                }
            }
            else
            {
                IAIAction* pAction = pGraph->GetAIAction();
                if (pAction)
                {
                    QString groupName = pGraph->GetGroupName();
                    if (groupName.isEmpty() == false)
                    {
                        outGroups.insert(groupName);
                    }
                }
            }
        }
        ++iter;
    }
}


//////////////////////////////////////////////////////////////////////////
void CFlowGraphManager::UpdateLayerName(const QString& oldName, const QString& name)
{
    if (!gEnv->pFlowSystem)
    {
        return;
    }

    bool isChanged = false;
    const TFlowNodeId layerTypeId = gEnv->pFlowSystem->GetTypeId("Engine:LayerSwitch");
    for (int i = 0; i < GetFlowGraphCount(); ++i)
    {
        CFlowGraph* pEditorFG = GetFlowGraph(i);
        IHyperGraphEnumerator* pEnum = pEditorFG->GetNodesEnumerator();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
        {
            if (!((CHyperNode*)pINode)->IsFlowNode() || ((CHyperNode*)pINode)->GetFlowNodeId() == InvalidFlowNodeId)
            {
                continue;
            }

            CFlowNode* pEditorFN = (CFlowNode*)pINode;
            if (pEditorFN->GetTypeId() == layerTypeId)
            {
                _smart_ptr<CVarBlock> pVarBlock = pEditorFN->GetInputsVarBlock();
                if (pVarBlock)
                {
                    IVariable* pVar = pVarBlock->FindVariable("Layer");
                    QString layerName;
                    pVar->Get(layerName);
                    if (layerName == oldName)
                    {
                        pVar->Set(name);
                        isChanged = true;
                    }
                }
            }
        }
        pEnum->Release();
    }

    if (isChanged)
    {
        SendNotifyEvent(EHG_GRAPH_CLEAR_SELECTION);
    }
}

void CFlowGraphManager::ReloadNodeConfig(IFlowNodeData* pFlowNodeData, CFlowNode* pNode)
{
    if (pFlowNodeData && pNode)
    {
        GetNodeConfig(pFlowNodeData, pNode);

        // Update the internal prototype
        CHyperNode* pPrototype = stl::find_in_map(m_prototypes, pNode->GetClassName(), NULL);
        if (pPrototype)
        {
            pPrototype = pNode->Clone();
        }
    }
}

void CFlowGraphManager::ComponentFlowGraphAdded(IFlowGraph* graph, const AZ::EntityId& id, const char* flowGraphName)
{
    if (graph)
    {
        AZ::Entity* entity;
        const AZ::EntityId fgEntityId = graph->GetGraphEntity(0);
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, fgEntityId);

        AZ_Assert(entity, "Every Flowgraph component-owned graph must be attached to an Entity");

        CFlowGraph* editorFlowGraph = new CFlowGraph(this, graph, flowGraphName, entity->GetName().c_str());
        editorFlowGraph->AddRef();

        CEntityObject* entityObject = nullptr;
        EBUS_EVENT_ID_RESULT(entityObject, id, AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);
        if (entityObject)
        {
            editorFlowGraph->SetEntity(entityObject, true);
        }
    }
}

void CFlowGraphManager::ComponentFlowGraphLoaded(IFlowGraph* graph, const AZ::EntityId& id, const AZStd::string& flowGraphName, const AZStd::string& flowGraphXML, bool isTemporaryGraph)
{
    if (!graph)
    {
        return;
    }

    IXmlParser* parser = GetISystem()->GetXmlUtils()->CreateXmlParser();
    if (!parser)
    {
        AZ_Assert(parser, "XmlUtils failed to create an XmlParser");
        return;
    }
    XmlNodeRef xmlRootNode = parser->ParseBuffer(flowGraphXML.c_str(), static_cast<int>(flowGraphXML.length()), false);
    if (!xmlRootNode)
    {
        AZ_Assert(xmlRootNode, "ParseBuffer on FlowGraph xml failed to return a valid xml root node");
        return;
    }

    // Convert "FlowGraph" to "Graph" for hypergraph parsing
    if (xmlRootNode->isTag("FlowGraph"))
    {
        xmlRootNode->setTag("Graph");
    }

    if (isTemporaryGraph)
    {
        SetGUIControlsProcessEvents(false, false);
    }

    CFlowGraph* editorFlowGraph = new CFlowGraph(this, graph, flowGraphName.c_str(), nullptr);
    editorFlowGraph->AddRef();
    editorFlowGraph->GetIFlowGraph()->SetType(IFlowGraph::eFGT_FlowGraphComponent);

    CEntityObject* entityObject = nullptr;
    EBUS_EVENT_ID_RESULT(entityObject, id, AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);
    if (entityObject)
    {
        editorFlowGraph->SetEntity(entityObject, true);
    }

    editorFlowGraph->LoadInternal(xmlRootNode, "");

    // Change to the custom name and notify the editor.
    editorFlowGraph->SetName(flowGraphName.c_str());
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_OWNER_CHANGE);

    if (isTemporaryGraph)
    {
        SetGUIControlsProcessEvents(true, false);
    }
}

void CFlowGraphManager::ComponentFlowGraphRemoved(IFlowGraph* graph)
{
    if (graph)
    {
        CFlowGraph* editorFlowGraph = FindGraph(graph);
        if (editorFlowGraph)
        {
            UnregisterAndResetView(editorFlowGraph);
        }
    }
}

void CFlowGraphManager::OnEntityStreamLoadSuccess()
{
    for (int i = 0; i < GetFlowGraphCount(); ++i)
    {
        CFlowGraph* editorFG = GetFlowGraph(i);
        IHyperGraphEnumerator* nodeEnum = editorFG->GetNodesEnumerator();
        for (IHyperNode* nodeInterface = nodeEnum->GetFirst(); nodeInterface; nodeInterface = nodeEnum->GetNext())
        {
            CHyperNode* node = static_cast<CHyperNode*>(nodeInterface);

            if (node->IsFlowNode() && node->CheckFlag(EHYPER_NODE_ENTITY))
            {
                node->ResolveEntityId();
            }
        }
    }
}
