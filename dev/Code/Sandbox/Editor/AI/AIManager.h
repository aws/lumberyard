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

#ifndef CRYINCLUDE_EDITOR_AI_AIMANAGER_H
#define CRYINCLUDE_EDITOR_AI_AIMANAGER_H
#pragma once


// forward declarations.
class CSOParamBase;

class CSOParamBase
{
public:
    CSOParamBase* pNext;
    IVariable* pVariable;
    bool bIsGroup;

    CSOParamBase(bool isGroup)
        : pNext(0)
        , pVariable(0)
        , bIsGroup(isGroup) {}
    virtual ~CSOParamBase()
    {
        if (pNext)
        {
            delete pNext;
        }
    }

    void PushBack(CSOParamBase* pParam)
    {
        if (pNext)
        {
            pNext->PushBack(pParam);
        }
        else
        {
            pNext = pParam;
        }
    }
};

class CSOParam
    : public CSOParamBase
{
public:
    CSOParam()
        : CSOParamBase(false)
        , bVisible(true)
        , bEditable(true) {}

    QString sName;      // the internal name
    QString sCaption;   // property name as it is shown in the editor
    bool bVisible;      // is the property visible
    bool bEditable;     // should the property be enabled
    QString sValue;     // default value
    QString sHelp;      // description of the property
};

class CSOParamGroup
    : public CSOParamBase
{
public:
    CSOParamGroup()
        : CSOParamBase(true)
        , pChildren(0)
        , bExpand(true) {}
    virtual ~CSOParamGroup()
    {
        if (pChildren)
        {
            delete pChildren;
        }
        if (pVariable)
        {
            pVariable->Release();
        }
    }

    QString sName;      // property group name
    bool bExpand;       // is the group expanded by default
    QString sHelp;      // description
    CSOParamBase* pChildren;
};

class CSOTemplate
{
public:
    CSOTemplate()
        : params(0) {}
    ~CSOTemplate()
    {
        if (params)
        {
            delete params;
        }
    }

    int id;                 // id used in the rules to reference this template
    QString name;           // name of the template
    QString description;    // description
    CSOParamBase* params;   // pointer to the first param
};


// map of all known smart object templates mapped by template id
typedef std::map< int, CSOTemplate* > MapTemplates;



// forward declarations.
class CScriptBind_AI;
class CAIGoalLibrary;
class CAIBehaviorLibrary;
class CCoverSurfaceManager;
class QWidget;

//////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class SANDBOX_API CAIManager
    : public IEditorNotifyListener
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CAIManager();
    ~CAIManager();
    void Init(ISystem* system);

    IAISystem*  GetAISystem();

    CAIGoalLibrary* GetGoalLibrary() { return m_goalLibrary; };
    CAIBehaviorLibrary* GetBehaviorLibrary() { return m_behaviorLibrary; };

    CCoverSurfaceManager* GetCoverSurfaceManager() { return m_coverSurfaceManager.get(); };

    //////////////////////////////////////////////////////////////////////////
    //! AI Anchor Actions enumeration.
    void GetAnchorActions(QStringList& actions) const;
    int AnchorActionToId(const char* sAction) const;

    //////////////////////////////////////////////////////////////////////////
    //! Smart Objects States and Actions enumeration
    void GetSmartObjectStates(QStringList& values) const;
    void AddSmartObjectState(const char* sState);

    // Enumerate all AI characters.

    //////////////////////////////////////////////////////////////////////////
    void ReloadScripts();

    const MapTemplates& GetMapTemplates() const { return m_mapTemplates; }
    const char* GetSmartObjectTemplateName(int index) const { return nullptr; }

    // Pathfinding properties
    void AssignPFPropertiesToPathType(const string& sPathType, const AgentPathfindingProperties& properties);
    const AgentPathfindingProperties* GetPFPropertiesOfPathType(const string& sPathType);
    string GetPathTypeNames();
    string GetPathTypeName(int index);
    size_t GetNumPathTypes() const { return mapPFProperties.size(); }

    void OnEnterGameMode(bool inGame);

    void EnableNavigationContinuousUpdate(bool enabled);
    bool GetNavigationContinuousUpdateState() const;
    void NavigationContinuousUpdate();
    void RebuildAINavigation();
    void PauseContinousUpdate();
    void RestartContinuousUpdate();

    bool IsNavigationFullyGenerated() const;

    void ShowNavigationAreas(bool show);
    bool GetShowNavigationAreasState() const;

    size_t GetNavigationAgentTypeCount() const;
    const char* GetNavigationAgentTypeName(size_t i) const;
    void SetNavigationDebugDisplayAgentType(size_t i) const;
    void EnableNavigationDebugDisplay(bool enable);
    bool GetNavigationDebugDisplayState() const;
    bool GetNavigationDebugDisplayAgent(size_t* index) const;

    void CalculateNavigationAccessibility();
    // It returns the current value of the accessibility visualization
    bool CalculateAndToggleVisualizationNavigationAccessibility();

    void NavigationDebugDisplay();

    void LoadNavigationEditorSettings();
    void SaveNavigationEditorSettings();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

private:
    void EnumAnchorActions();
    void RandomizeAIVariations();

    void FreeTemplates();
    bool ReloadTemplates();
    CSOParamBase* LoadTemplateParams(XmlNodeRef root) const;

    MapTemplates m_mapTemplates;

    CAIGoalLibrary* m_goalLibrary;
    CAIBehaviorLibrary* m_behaviorLibrary;
    IAISystem*  m_aiSystem;

    std::unique_ptr<CCoverSurfaceManager> m_coverSurfaceManager;

    //! AI Anchor Actions.
    friend struct CAIAnchorDump;
    typedef std::map<QString, int> AnchorActions;
    AnchorActions m_anchorActions;

    CScriptBind_AI* m_pScriptAI;

    typedef std::map<string, AgentPathfindingProperties> PFPropertiesMap;
    PFPropertiesMap mapPFProperties;

    bool m_showNavigationAreas;
    bool m_enableNavigationContinuousUpdate;
    bool m_enableDebugDisplay;
    bool m_pendingNavigationCalculateAccessibility;
};

#endif // CRYINCLUDE_EDITOR_AI_AIMANAGER_H
