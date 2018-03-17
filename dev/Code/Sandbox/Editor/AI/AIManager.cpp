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
#include "AIManager.h"

#include "MainWindow.h"

#include "AiGoalLibrary.h"
#include "AiGoal.h"
#include "AiBehaviorLibrary.h"

#include "IAgent.h"
#include "IAISystem.h"
#include <IAIAction.h>

#include <IScriptSystem.h>
#include <IEntitySystem.h>
#include <INavigationSystem.h>
#include "ScriptBind_AI.h"

#include "CoverSurfaceManager.h"

#include "../HyperGraph/FlowGraph.h"
#include "../HyperGraph/FlowGraphManager.h"
#include "../HyperGraph/HyperGraphDialog.h"
#include "StartupLogoDialog.h"
#include "Objects/EntityObject.h"

#include "QtUtil.h"

#include <QMessageBox>

extern void ReloadSmartObjects(IConsoleCmdArgs* /* pArgs */);

#define GRAPH_FILE_FILTER "Graph XML Files (*.xml)"

#define SOED_SOTEMPLATES_FILENAME "Libs/SmartObjects/Templates/SOTemplates.xml"

//////////////////////////////////////////////////////////////////////////
// Function created on designers request to automatically randomize
// model index variations per instance of the AI character.
//////////////////////////////////////////////////////////////////////////
void ed_randomize_variations(IConsoleCmdArgs* pArgs)
{
    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    for (int i = 0; i < (int)objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntityObject = (CEntityObject*)pObject;
            if (pEntityObject->GetProperties2())
            {
                CVarBlock* pEntityProperties = pEntityObject->GetProperties();
                if (pEntityObject->GetPrototype())
                {
                    pEntityProperties = pEntityObject->GetPrototype()->GetProperties();
                }
                if (!pEntityProperties)
                {
                    continue;
                }
                // This is some kind of AI entity.
                // Check if it have AI variations
                IVariable* p_nModelVariations = pEntityProperties->FindVariable("nModelVariations", true);
                IVariable* p_nVariation = pEntityObject->GetProperties2()->FindVariable("nVariation", true);
                if (p_nModelVariations && p_nVariation)
                {
                    int nModelVariations = 0;
                    p_nModelVariations->Get(nModelVariations);
                    if (nModelVariations > 0)
                    {
                        int nVariation = 1 + (int)(cry_random(0.0f, 1.0f) * nModelVariations);
                        IVariable* p_nVariation = pEntityObject->GetProperties2()->FindVariable("nVariation", true);
                        if (p_nVariation)
                        {
                            p_nVariation->Set(nVariation);
                            pEntityObject->Reload();
                            continue;
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CAI Manager.
//////////////////////////////////////////////////////////////////////////
CAIManager::CAIManager()
    : m_coverSurfaceManager(new CCoverSurfaceManager)
    , m_showNavigationAreas(true)
    , m_enableNavigationContinuousUpdate(true)
    , m_enableDebugDisplay(true)
    , m_pendingNavigationCalculateAccessibility(true)
{
    m_aiSystem = NULL;
    m_pScriptAI = NULL;
    m_goalLibrary = new CAIGoalLibrary;
    m_behaviorLibrary = new CAIBehaviorLibrary;

    GetIEditor()->RegisterNotifyListener(this);
}

void CAIManager::LoadNavigationEditorSettings()
{
    m_showNavigationAreas = gSettings.bNavigationShowAreas;
    m_enableNavigationContinuousUpdate = gSettings.bNavigationContinuousUpdate;
    m_enableDebugDisplay = gSettings.bNavigationDebugDisplay;

    if (NavigationAgentTypeID debugAgentTypeID = m_aiSystem->GetNavigationSystem()->GetAgentTypeID(gSettings.navigationDebugAgentType))
    {
        m_aiSystem->GetNavigationSystem()->SetDebugDisplayAgentType(debugAgentTypeID);
    }

    if (gSettings.bVisualizeNavigationAccessibility)
    {
        if (ICVar* pVar = gEnv->pConsole->GetCVar("ai_MNMDebugAccessibility"))
        {
            pVar->Set(1);
        }
    }
}

void CAIManager::SaveNavigationEditorSettings()
{
    QSettings settings;
    settings.beginGroup("Editor");
    settings.setValue(QStringLiteral("NavigationShowAreas"), m_showNavigationAreas ? 1 : 0);
    settings.setValue(QStringLiteral("NavigationEnableContinuousUpdate"), m_enableNavigationContinuousUpdate ? 1 : 0);

    // winApp->GetProfileInt(appName, "NavigationEnableDebugDisplay", m_enableDebugDisplay ? 1 : 0); // Removed by KDAB: no-op?

    QString agentName;

    if (NavigationAgentTypeID debugAgentTypeID = m_aiSystem->GetNavigationSystem()->GetDebugDisplayAgentType())
    {
        agentName = m_aiSystem->GetNavigationSystem()->GetAgentTypeName(debugAgentTypeID);
    }

    settings.setValue(QStringLiteral("NavigationDebugDisplayAgentName"), agentName);
}

void CAIManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
        PauseContinousUpdate();
        break;
    case eNotify_OnEndGameMode:
        RestartContinuousUpdate();
        break;
    }
}

CAIManager::~CAIManager()
{
    GetIEditor()->UnregisterNotifyListener(this);

    FreeActionGraphs();

    delete m_behaviorLibrary;
    m_behaviorLibrary = 0;
    delete m_goalLibrary;
    m_goalLibrary = 0;

    SAFE_DELETE(m_pScriptAI);
}

void CAIManager::Init(ISystem* system)
{
    m_aiSystem = system->GetAISystem();
    if (!m_aiSystem)
    {
        return;
    }

    if (!m_pScriptAI)
    {
        m_pScriptAI = new CScriptBind_AI(system);
    }

    // (MATT) Reset the AI to allow it to update its configuration, with respect to ai_CompatibilityMode cvar {2009/06/25}
    m_aiSystem->Reset(IAISystem::RESET_INTERNAL);

    CStartupLogoDialog::SetText("Loading AI Behaviors...");
    m_behaviorLibrary->LoadBehaviors("Scripts\\AI\\Behaviors\\");

    CStartupLogoDialog::SetText("Loading Smart Objects...");
    //enumerate Anchor actions.
    EnumAnchorActions();

    //load smart object templates
    ReloadTemplates();

    CStartupLogoDialog::SetText("Loading Action Flowgraphs...");
    LoadActionGraphs();

    REGISTER_COMMAND("so_reload", ReloadSmartObjects, VF_NULL, "");
    REGISTER_COMMAND("ed_randomize_variations", ed_randomize_variations, VF_NULL, "");

    LoadNavigationEditorSettings();
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::ReloadActionGraphs()
{
    if (!m_aiSystem)
    {
        return;
    }
    //  FreeActionGraphs();
    GetAISystem()->GetAIActionManager()->ReloadActions();
    LoadActionGraphs();
}

void CAIManager::LoadActionGraphs()
{
    if (!m_aiSystem)
    {
        return;
    }

    int i = 0;
    IAIAction* pAction;
    while (pAction = m_aiSystem->GetAIActionManager()->GetAIAction(i++))
    {
        CFlowGraph* m_pFlowGraph = GetIEditor()->GetFlowGraphManager()->FindGraphForAction(pAction);
        if (m_pFlowGraph)
        {
            // [3/24/2011 evgeny] Reconnect Sandbox CFlowGraph to CryAction CFlowGraph
            m_pFlowGraph->SetIFlowGraph(pAction->GetFlowGraph());
        }
        else
        {
            m_pFlowGraph = GetIEditor()->GetFlowGraphManager()->CreateGraphForAction(pAction);
            m_pFlowGraph->AddRef();
            QString filename(AI_ACTIONS_PATH);
            filename += '/';
            filename += pAction->GetName();
            filename += ".xml";
            m_pFlowGraph->SetName("");
            m_pFlowGraph->Load(filename.toUtf8().data());
        }
    }
}

void CAIManager::SaveAndReloadActionGraphs()
{
    if (!m_aiSystem)
    {
        return;
    }

    QString actionName;
    CHyperGraphDialog* pHGDlg = CHyperGraphDialog::instance();
    if (pHGDlg)
    {
        CHyperGraph* pGraph = pHGDlg->GetGraph();
        if (pGraph)
        {
            IAIAction* pAction = pGraph->GetAIAction();
            if (pAction)
            {
                actionName = pAction->GetName();
                pHGDlg->SetGraph(NULL, true);       // KDAB_PORT view only
            }
        }
    }

    SaveActionGraphs();
    ReloadActionGraphs();

    if (!actionName.isEmpty())
    {
        IAIAction* pAction = GetAISystem()->GetAIActionManager()->GetAIAction(actionName.toUtf8().data());
        if (pAction)
        {
            CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
            CFlowGraph* pFlowGraph = pManager->FindGraphForAction(pAction);
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pManager->OpenView(pFlowGraph);
            }
        }
    }
}

void CAIManager::SaveActionGraphs()
{
    if (!m_aiSystem)
    {
        return;
    }

    QWaitCursor waitCursor;

    int i = 0;
    IAIAction* pAction;
    while (pAction = m_aiSystem->GetAIActionManager()->GetAIAction(i++))
    {
        CFlowGraph* m_pFlowGraph = GetIEditor()->GetFlowGraphManager()->FindGraphForAction(pAction);
        if (m_pFlowGraph->IsModified())
        {
            m_pFlowGraph->Save((m_pFlowGraph->GetName() + QStringLiteral(".xml")).toUtf8().data());
            pAction->Invalidate();
        }
    }
}

void CAIManager::FreeActionGraphs()
{
    CFlowGraphManager* pFGMgr = GetIEditor()->GetFlowGraphManager();
    if (pFGMgr)
    {
        pFGMgr->FreeGraphsForActions();
    }
}

IAISystem*  CAIManager::GetAISystem()
{
    return GetIEditor()->GetSystem()->GetAISystem();
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::ReloadScripts()
{
    GetBehaviorLibrary()->ReloadScripts();
    EnumAnchorActions();
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::GetAnchorActions(QStringList& actions) const
{
    actions.clear();
    for (AnchorActions::const_iterator it = m_anchorActions.begin(); it != m_anchorActions.end(); it++)
    {
        actions.push_back(it->first);
    }
}

//////////////////////////////////////////////////////////////////////////
int CAIManager::AnchorActionToId(const char* sAction) const
{
    AnchorActions::const_iterator it = m_anchorActions.find(sAction);
    if (it != m_anchorActions.end())
    {
        return it->second;
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct CAIAnchorDump
    : public IScriptTableDumpSink
{
    CAIManager::AnchorActions actions;

    CAIAnchorDump(IScriptTable* obj)
    {
        m_pScriptObject = obj;
    }

    virtual void OnElementFound(int nIdx, ScriptVarType type) {}
    virtual void OnElementFound(const char* sName, ScriptVarType type)
    {
        if (type == svtNumber)
        {
            // New behavior.
            int val;
            if (m_pScriptObject->GetValue(sName, val))
            {
                actions[sName] = val;
            }
        }
    }
private:
    IScriptTable* m_pScriptObject;
};

//////////////////////////////////////////////////////////////////////////
void CAIManager::EnumAnchorActions()
{
    IScriptSystem* pScriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();

    SmartScriptTable pAIAnchorTable(pScriptSystem, true);
    if (pScriptSystem->GetGlobalValue("AIAnchorTable", pAIAnchorTable))
    {
        CAIAnchorDump anchorDump(pAIAnchorTable);
        pAIAnchorTable->Dump(&anchorDump);
        m_anchorActions = anchorDump.actions;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::GetSmartObjectStates(QStringList& values) const
{
    if (!m_aiSystem)
    {
        return;
    }

    const char* sStateName;
    for (int i = 0; sStateName = m_aiSystem->GetSmartObjectManager()->GetSmartObjectStateName(i); ++i)
    {
        values.push_back(sStateName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::GetSmartObjectActions(QStringList& values) const
{
    if (!m_aiSystem)
    {
        return;
    }

    IAIActionManager* pAIActionManager = m_aiSystem->GetAIActionManager();
    assert(pAIActionManager);
    if (!pAIActionManager)
    {
        return;
    }

    values.clear();

    for (int i = 0; IAIAction* pAIAction = pAIActionManager->GetAIAction(i); ++i)
    {
        const char* szActionName = pAIAction->GetName();
        if (szActionName)
        {
            values.push_back(szActionName);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::AddSmartObjectState(const char* sState)
{
    if (!m_aiSystem)
    {
        return;
    }
    m_aiSystem->GetSmartObjectManager()->RegisterSmartObjectState(sState);
}

//////////////////////////////////////////////////////////////////////////
bool CAIManager::NewAction(QString& filename, QWidget* container)
{
    AZStd::string xmlDataPath = Path::GetEditingGameDataFolder() + "/" + AI_ACTIONS_PATH;
    CFileUtil::CreateDirectory(xmlDataPath.c_str());

    QString newFileName;
    if (!CFileUtil::SelectSaveFile(GRAPH_FILE_FILTER, "xml", xmlDataPath.c_str(), newFileName))
    {
        return false;
    }
    filename = newFileName.toLower();

    // check if file exists.
    FILE* file = fopen(filename.toUtf8().data(), "rb");
    if (file)
    {
        fclose(file);
        QMessageBox::critical(container, QString(), QObject::tr("Can't create AI Action because another AI Action with this name already exists!\n\nCreation canceled..."));
        return false;
    }

    // Make a new graph.
    CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
    CHyperGraph* pGraph = pManager->CreateGraph();

    CHyperNode* pStartNode = (CHyperNode*) pGraph->CreateNode("AI:ActionStart");
    pStartNode->SetPos(QPointF(80, 10));
    CHyperNode* pEndNode = (CHyperNode*) pGraph->CreateNode("AI:ActionEnd");
    pEndNode->SetPos(QPointF(400, 10));
    CHyperNode* pPosNode = (CHyperNode*) pGraph->CreateNode("Entity:EntityPos");
    pPosNode->SetPos(QPointF(20, 70));

    pGraph->UnselectAll();
    pGraph->ConnectPorts(pStartNode, &pStartNode->GetOutputs()->at(1), pPosNode, &pPosNode->GetInputs()->at(0), false);

    bool r = pGraph->Save(filename.toUtf8().data());

    delete pGraph;

    ReloadActionGraphs();

    return r;
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::AssignPFPropertiesToPathType(const string& sPathType, const AgentPathfindingProperties& properties)
{
    mapPFProperties.insert(PFPropertiesMap::value_type(sPathType, properties));
}

//////////////////////////////////////////////////////////////////////////
const AgentPathfindingProperties* CAIManager::GetPFPropertiesOfPathType(const string& sPathType)
{
    PFPropertiesMap::iterator iterPFProperties = mapPFProperties.find(sPathType);
    return (iterPFProperties != mapPFProperties.end()) ? &iterPFProperties->second : 0;
}

//////////////////////////////////////////////////////////////////////////
string CAIManager::GetPathTypeNames()
{
    string pathNames;
    for (PFPropertiesMap::iterator iter = mapPFProperties.begin(); iter != mapPFProperties.end(); ++iter)
    {
        pathNames += iter->first + ' ';
    }
    return pathNames;
}

//////////////////////////////////////////////////////////////////////////
string CAIManager::GetPathTypeName(int index)
{
    CRY_ASSERT(index >= 0 && index < mapPFProperties.size());
    PFPropertiesMap::iterator iter = mapPFProperties.begin();
    if (index >= 0 && index < mapPFProperties.size())
    {
        std::advance(iter, index);
        return iter->first;
    }

    //  int i=0;
    //  for (PFPropertiesMap::iterator iter = mapPFProperties.begin(); iter != mapPFProperties.end(); ++iter, ++i)
    //  {
    //          if(i == index)
    //          return iter->first;
    //  }
    return "";
}

//////////////////////////////////////////////////////////////////////////
void CAIManager::FreeTemplates()
{
    MapTemplates::iterator it, itEnd = m_mapTemplates.end();
    for (it = m_mapTemplates.begin(); it != itEnd; ++it)
    {
        delete it->second;
    }
    m_mapTemplates.clear();
}

bool CAIManager::ReloadTemplates()
{
    XmlNodeRef root = GetISystem()->LoadXmlFromFile(SOED_SOTEMPLATES_FILENAME);
    if (!root || !root->isTag("SOTemplates"))
    {
        return false;
    }

    FreeTemplates();

    QtUtil::QtMFCScopedHWNDCapture parentWidgetCapture;

    int count = root->getChildCount();
    for (int i = 0; i < count; ++i)
    {
        XmlNodeRef node = root->getChild(i);
        if (node->isTag("Template"))
        {
            CSOTemplate* pTemplate = new CSOTemplate;

            node->getAttr("id", pTemplate->id);
            pTemplate->name = node->getAttr("name");
            pTemplate->description = node->getAttr("description");

            // check is the id unique
            MapTemplates::iterator find = m_mapTemplates.find(pTemplate->id);
            if (find != m_mapTemplates.end())
            {
                QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("WARNING:\nSmart object template named %1 will be ignored! The id is not unique.").arg(pTemplate->name));
                delete pTemplate;
                continue;
            }

            // load params
            pTemplate->params = LoadTemplateParams(node);
            if (!pTemplate->params)
            {
                QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("WARNING:\nSmart object template named %1 will be ignored! Can't load params.").arg(pTemplate->name));
                delete pTemplate;
                continue;
            }

            // insert in map
            m_mapTemplates[ pTemplate->id ] = pTemplate;
        }
    }
    return true;
}

CSOParamBase* CAIManager::LoadTemplateParams(XmlNodeRef root) const
{
    CSOParamBase* pFirst = NULL;

    int count = root->getChildCount();
    for (int i = 0; i < count; ++i)
    {
        XmlNodeRef node = root->getChild(i);
        if (node->isTag("Param"))
        {
            CSOParam* pParam = new CSOParam;

            pParam->sName = node->getAttr("name");
            pParam->sCaption = node->getAttr("caption");
            node->getAttr("visible", pParam->bVisible);
            node->getAttr("editable", pParam->bEditable);
            pParam->sValue = node->getAttr("value");
            pParam->sHelp = node->getAttr("help");

            IVariable* pVar = NULL;
            /*
                        if ( pParam->sName == "bNavigationRule" )
                            pVar = &gSmartObjectsUI.bNavigationRule;
                        else if ( pParam->sName == "sEvent" )
                            pVar = &gSmartObjectsUI.sEvent;
                        else if ( pParam->sName == "userClass" )
                            pVar = &gSmartObjectsUI.userClass;
                        else if ( pParam->sName == "userState" )
                            pVar = &gSmartObjectsUI.userState;
                        else if ( pParam->sName == "userHelper" )
                            pVar = &gSmartObjectsUI.userHelper;
                        else if ( pParam->sName == "iMaxAlertness" )
                            pVar = &gSmartObjectsUI.iMaxAlertness;
                        else if ( pParam->sName == "objectClass" )
                            pVar = &gSmartObjectsUI.objectClass;
                        else if ( pParam->sName == "objectState" )
                            pVar = &gSmartObjectsUI.objectState;
                        else if ( pParam->sName == "objectHelper" )
                            pVar = &gSmartObjectsUI.objectHelper;
                        else if ( pParam->sName == "entranceHelper" )
                            pVar = &gSmartObjectsUI.entranceHelper;
                        else if ( pParam->sName == "exitHelper" )
                            pVar = &gSmartObjectsUI.exitHelper;
                        else if ( pParam->sName == "limitsDistanceFrom" )
                            pVar = &gSmartObjectsUI.limitsDistanceFrom;
                        else if ( pParam->sName == "limitsDistanceTo" )
                            pVar = &gSmartObjectsUI.limitsDistanceTo;
                        else if ( pParam->sName == "limitsOrientation" )
                            pVar = &gSmartObjectsUI.limitsOrientation;
                        else if ( pParam->sName == "delayMinimum" )
                            pVar = &gSmartObjectsUI.delayMinimum;
                        else if ( pParam->sName == "delayMaximum" )
                            pVar = &gSmartObjectsUI.delayMaximum;
                        else if ( pParam->sName == "delayMemory" )
                            pVar = &gSmartObjectsUI.delayMemory;
                        else if ( pParam->sName == "multipliersProximity" )
                            pVar = &gSmartObjectsUI.multipliersProximity;
                        else if ( pParam->sName == "multipliersOrientation" )
                            pVar = &gSmartObjectsUI.multipliersOrientation;
                        else if ( pParam->sName == "multipliersVisibility" )
                            pVar = &gSmartObjectsUI.multipliersVisibility;
                        else if ( pParam->sName == "multipliersRandomness" )
                            pVar = &gSmartObjectsUI.multipliersRandomness;
                        else if ( pParam->sName == "fLookAtOnPerc" )
                            pVar = &gSmartObjectsUI.fLookAtOnPerc;
                        else if ( pParam->sName == "userPreActionState" )
                            pVar = &gSmartObjectsUI.userPreActionState;
                        else if ( pParam->sName == "objectPreActionState" )
                            pVar = &gSmartObjectsUI.objectPreActionState;
                        else if ( pParam->sName == "highPriority" )
                            pVar = &gSmartObjectsUI.highPriority;
                        else if ( pParam->sName == "actionName" )
                            pVar = &gSmartObjectsUI.actionName;
                        else if ( pParam->sName == "userPostActionState" )
                            pVar = &gSmartObjectsUI.userPostActionState;
                        else if ( pParam->sName == "objectPostActionState" )
                            pVar = &gSmartObjectsUI.objectPostActionState;

                        if ( !pVar )
                        {
                            QMessageBox( QApplication::activeWindow(), CString("WARNING:\nSmart object template has a Param tag named ") + pParam->sName + " which is not recognized as valid name!\nThe Param will be ignored..." );
                            delete pParam;
                            continue;
                        }
            */
            pParam->pVariable = pVar;

            if (pFirst)
            {
                pFirst->PushBack(pParam);
            }
            else
            {
                pFirst = pParam;
            }
        }
        else if (node->isTag("ParamGroup"))
        {
            CSOParamGroup* pGroup = new CSOParamGroup;

            pGroup->sName = node->getAttr("name");
            node->getAttr("expand", pGroup->bExpand);
            pGroup->sHelp = node->getAttr("help");

            // load children
            pGroup->pChildren = LoadTemplateParams(node);
            if (!pGroup->pChildren)
            {
                QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("WARNING:\nSmart object template has a ParamGroup tag named %1 without any children nodes!\nThe ParamGroup will be ignored...").arg(pGroup->sName));
                delete pGroup;
                continue;
            }

            CVariableArray* pVar = new CVariableArray();
            pVar->AddRef();
            pGroup->pVariable = pVar;

            if (pFirst)
            {
                pFirst->PushBack(pGroup);
            }
            else
            {
                pFirst = pGroup;
            }
        }
    }

    return pFirst;
}

void CAIManager::OnEnterGameMode(bool inGame)
{
    if (inGame)
    {
        SaveNavigationEditorSettings();
        m_aiSystem->GetNavigationSystem()->StopWorldMonitoring();
    }
    else
    {
        m_aiSystem->GetNavigationSystem()->StartWorldMonitoring();
    }
}

void CAIManager::EnableNavigationContinuousUpdate(bool enabled)
{
    m_enableNavigationContinuousUpdate = enabled;
}

bool CAIManager::GetNavigationContinuousUpdateState() const
{
    return m_enableNavigationContinuousUpdate;
}

void CAIManager::NavigationContinuousUpdate()
{
    if (m_aiSystem) //0-pointer crash when starting without Game-Dll
    {
        const INavigationSystem::WorkingState state = m_aiSystem->GetNavigationSystem()->Update();
        switch (state)
        {
        case INavigationSystem::Idle:
            if (m_pendingNavigationCalculateAccessibility)
            {
                CalculateNavigationAccessibility();
                m_pendingNavigationCalculateAccessibility = false;
            }
            break;

        case INavigationSystem::Working:
            m_pendingNavigationCalculateAccessibility = true;
            break;
        }
    }
}

void CAIManager::RebuildAINavigation()
{
    if (m_aiSystem) //0-pointer crash when starting without Game-Dll
    {
        m_aiSystem->GetNavigationSystem()->ClearAndNotify();
    }
}

void CAIManager::PauseContinousUpdate()
{
    if (m_aiSystem) //0-pointer crash when starting without Game-Dll
    {
        m_aiSystem->GetNavigationSystem()->PauseNavigationUpdate();
    }
}

void CAIManager::RestartContinuousUpdate()
{
    if (m_aiSystem) //0-pointer crash when starting without Game-Dll
    {
        m_aiSystem->GetNavigationSystem()->RestartNavigationUpdate();
    }
}

bool CAIManager::IsNavigationFullyGenerated() const
{
    if (m_aiSystem)
    {
        return (m_aiSystem->GetNavigationSystem()->GetState() == INavigationSystem::Idle);
    }

    return true;
}

void CAIManager::ShowNavigationAreas(bool show)
{
    m_showNavigationAreas = show;
}

bool CAIManager::GetShowNavigationAreasState() const
{
    return m_showNavigationAreas;
}

size_t CAIManager::GetNavigationAgentTypeCount() const
{
    size_t agentCount = 0;
    if (m_aiSystem) //0-pointer crash when starting without Game-Dll
    {
        agentCount = m_aiSystem->GetNavigationSystem()->GetAgentTypeCount();
    }

    return agentCount;
}

const char* CAIManager::GetNavigationAgentTypeName(size_t i) const
{
    const NavigationAgentTypeID id = m_aiSystem->GetNavigationSystem()->GetAgentTypeID(i);
    if (id)
    {
        return m_aiSystem->GetNavigationSystem()->GetAgentTypeName(id);
    }

    return 0;
}

void CAIManager::SetNavigationDebugDisplayAgentType(size_t i) const
{
    const NavigationAgentTypeID id = m_aiSystem->GetNavigationSystem()->GetAgentTypeID(i);
    m_aiSystem->GetNavigationSystem()->SetDebugDisplayAgentType(id);
}

void CAIManager::EnableNavigationDebugDisplay(bool enable)
{
    m_enableDebugDisplay = enable;
}


bool CAIManager::GetNavigationDebugDisplayState() const
{
    return m_enableDebugDisplay;
}

bool CAIManager::GetNavigationDebugDisplayAgent(size_t* index) const
{
    INavigationSystem* pNavigationSystem(NULL);
    if (!m_aiSystem)
    {
        return false;
    }

    pNavigationSystem = m_aiSystem->GetNavigationSystem();
    if (!pNavigationSystem)
    {
        return false;
    }

    const NavigationAgentTypeID id = pNavigationSystem->GetDebugDisplayAgentType();
    if (id)
    {
        const size_t agentTypeCount = m_aiSystem->GetNavigationSystem()->GetAgentTypeCount();
        for (size_t i = 0; i < agentTypeCount; ++i)
        {
            if (id == m_aiSystem->GetNavigationSystem()->GetAgentTypeID(i))
            {
                if (index)
                {
                    *index = i;
                }
                return true;
            }
        }
    }

    return false;
}

void CAIManager::CalculateNavigationAccessibility()
{
    if (m_aiSystem)
    {
        INavigationSystem* pINavigationSystem = m_aiSystem->GetNavigationSystem();
        if (pINavigationSystem)
        {
            pINavigationSystem->CalculateAccessibility();
        }
    }
}

bool CAIManager::CalculateAndToggleVisualizationNavigationAccessibility()
{
    bool shouldVisualizeNavigationAccessibility = !gSettings.bVisualizeNavigationAccessibility;

    ICVar* pVar = gEnv->pConsole->GetCVar("ai_MNMDebugAccessibility");
    CRY_ASSERT_MESSAGE(pVar, "The cvar ai_MNMDebugAccessibility is not defined.");
    if (pVar)
    {
        pVar->Set(shouldVisualizeNavigationAccessibility == false ? 0 : 1);
    }

    return shouldVisualizeNavigationAccessibility;
}

void CAIManager::NavigationDebugDisplay()
{
    if (m_enableDebugDisplay)
    {
        if (m_aiSystem)
        {
            INavigationSystem* pINavigationSystem = m_aiSystem->GetNavigationSystem();
            if (pINavigationSystem)
            {
                const NavigationAgentTypeID id = pINavigationSystem->GetDebugDisplayAgentType();
                if (id)
                {
                    m_aiSystem->GetNavigationSystem()->DebugDraw();
                }
            }
        }
    }
}

