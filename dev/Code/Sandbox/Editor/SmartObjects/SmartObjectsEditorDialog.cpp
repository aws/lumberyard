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
#include <IAISystem.h>
#include <IAgent.h>

#include "IViewPane.h"
#include "Objects/EntityObject.h"
#include "Objects/SmartObject.h"
#include "Objects/SelectionGroup.h"
#include "GameEngine.h"
#include "QtViewPaneManager.h"

#include "AI/AIManager.h"
#include "ItemDescriptionDlg.h"
#include "SmartObjectClassDialog.h"
#include "SmartObjectHelperDialog.h"
#include "SmartObjectTemplateDialog.h"
#include <AzQtComponents/Components/StyledDockWidget.h>

#include "SmartObjectHelperObject.h"
#include "SmartObjectsEditorDialog.h"

#include <QtViewPane.h>

#include <QtUI/ClickableLabel.h>
#include <QtUI/QCollapsibleGroupBox.h>

#include <Util/AbstractGroupProxyModel.h>
#include <Util/PathUtil.h> // for getting the game folder

#include <QBoxLayout>
#include <QDockWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QMenu>
#include <QMimeData>
#include <QPalette>
#include <QPainter>
#include <QSettings>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QTextEdit>
#include <QTreeView>

#define SOED_DIALOGFRAME_CLASSNAME "SmartObjectsEditorDialog"
#define CLASS_TEMPLATES_FOLDER "Libs/SmartObjects/ClassTemplates/"

#define IDC_REPORTCONTROL   1234

static const char* const s_smartObjectsDialogName = "Smart Objects Editor";

#define COLUMN_TYPE         -1
#define COLUMN_NAME         0
#define COLUMN_DESCRIPTION  1
#define COLUMN_USER_CLASS   2
#define COLUMN_USER_STATE   3
#define COLUMN_OBJECT_CLASS 4
#define COLUMN_OBJECT_STATE 5
#define COLUMN_ACTION       6
#define COLUMN_ORDER        7


// static members
SmartObjectConditions               CSOLibrary::m_Conditions;
CSOLibrary::VectorHelperData        CSOLibrary::m_vHelpers;
CSOLibrary::VectorEventData         CSOLibrary::m_vEvents;
CSOLibrary::VectorStateData         CSOLibrary::m_vStates;
CSOLibrary::VectorClassTemplateData CSOLibrary::m_vClassTemplates;
CSOLibrary::VectorClassData         CSOLibrary::m_vClasses;
bool CSOLibrary::m_bSaveNeeded = false;
bool CSOLibrary::m_bLoadNeeded = true;
int CSOLibrary::m_iNumEditors = 0;
CSOLibrary* CSOLibrary::m_pInstance = NULL;


// functor used for comparing and ordering data structures by name
template< class T >
struct less_name
{
    bool operator() (const T& _Left, const T& _Right) const
    {
        return _Left.name < _Right.name;
    }
};

// functor used for comparing and ordering data structures by name (case insensitive)
template< class T >
struct less_name_no_case
{
    bool operator() (const T& _Left, const T& _Right) const
    {
        return _stricmp(_Left.name, _Right.name) < 0;
    }
};


void ReloadSmartObjects(IConsoleCmdArgs* /* pArgs */)
{
    CSOLibrary::Reload();
}


//////////////////////////////////////////////////////////////////////////
// Called by the editor to notify the listener about the specified event.
void CSOLibrary::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:           // Sent when editor goes to game mode.
    case eNotify_OnBeginSimulationMode:     // Sent when simulation mode is started.
    case eNotify_OnBeginSceneSave:          // Sent when document is about to be saved.
    case eNotify_OnQuit:                    // Sent before editor quits.
    case eNotify_OnBeginNewScene:           // Sent when the document is begin to be cleared.
    case eNotify_OnBeginSceneOpen:          // Sent when document is about to be opened.
    case eNotify_OnCloseScene:              // Send when the document is about to close.
        if (m_bSaveNeeded && m_iNumEditors == 0 && Save())
        {
            m_bSaveNeeded = false;
            m_bLoadNeeded = false;
        }
        if (event == eNotify_OnQuit)
        {
            GetIEditor()->UnregisterNotifyListener(this);
            m_pInstance = NULL;
            delete this;
        }
        break;

    case eNotify_OnInit:                    // Sent after editor fully initialized.
        REGISTER_COMMAND("so_reload", ReloadSmartObjects, VF_NULL, "");
        break;

    case eNotify_OnEndSceneOpen:            // Sent after document have been opened.
    case eNotify_OnEndSceneSave:            // Sent after document have been saved.
    case eNotify_OnEndNewScene:             // Sent after the document have been cleared.
    case eNotify_OnMissionChange:           // Send when the current mission changes.
    case eNotify_OnEditModeChange:          // Sent when editing mode change (move,rotate,scale,....)
    case eNotify_OnEditToolChange:          // Sent when edit tool is changed (ObjectMode,TerrainModify,....)
    case eNotify_OnEndGameMode:             // Send when editor goes out of game mode.
    case eNotify_OnUpdateViewports:         // Sent when editor needs to update data in the viewports.
    case eNotify_OnReloadTrackView:         // Sent when editor needs to update the track view.
    case eNotify_OnInvalidateControls:      // Sent when editor needs to update some of the data that can be cached by controls like combo boxes.
    case eNotify_OnSelectionChange:         // Sent when object selection change.
    case eNotify_OnPlaySequence:            // Sent when editor start playing animation sequence.
    case eNotify_OnStopSequence:            // Sent when editor stop playing animation sequence.
        break;
    }
}

void CSOLibrary::Reload()
{
    m_bLoadNeeded = true;
    CSmartObjectsEditorDialog* pView = FindViewPane<CSmartObjectsEditorDialog>(s_smartObjectsDialogName);
    if (pView)
    {
        pView->ReloadEntries(true);
    }
    else
    {
        Load();
    }
    InvalidateSOEntities();
    if (gEnv->pAISystem && !GetIEditor()->IsInGameMode() && !GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        gEnv->pAISystem->GetSmartObjectManager()->ReloadSmartObjectRules();
    }
}

void CSOLibrary::InvalidateSOEntities()
{
    CObjectClassDesc* pClass = GetIEditor()->GetObjectManager()->FindClass("SmartObject");
    if (!pClass)
    {
        return;
    }

    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);
    CBaseObjectsArray::iterator it, itEnd = objects.end();
    for (it = objects.begin(); it != itEnd; ++it)
    {
        CBaseObject* pBaseObject = *it;
        if (pBaseObject->GetClassDesc() == pClass)
        {
            CSmartObject* pSOEntity = (CSmartObject*) pBaseObject;
            pSOEntity->OnPropertyChange(0);
        }
    }
}

bool CSOLibrary::Load()
{
    if (!m_bLoadNeeded)
    {
        return true;
    }
    m_bLoadNeeded = false;

    if (!m_pInstance)
    {
        m_pInstance = new CSOLibrary;
        GetIEditor()->RegisterNotifyListener(m_pInstance);
    }

    LoadClassTemplates();

    // load the rules from SmartObjects.xml, if it exists
    QString smartObjectsXmlPath = Path::Make(Path::GetEditingGameDataFolder().c_str(), SMART_OBJECTS_XML);
    if (QFileInfo(smartObjectsXmlPath).exists())
    {
        if (!LoadFromFile(smartObjectsXmlPath.toUtf8().data()))
        {
            m_bLoadNeeded = true;
            Warning("CSOLibrary::Load() failed to load from %s", smartObjectsXmlPath.toUtf8().constData());
            return false;
        }
        m_bSaveNeeded = false;
    }
    else
    {
        // SMART_OBJECTS_XML file didn't exist in the game folder, set m_bSaveNeeded flag so an empty file will be saved later
        // in the game folder
        m_bSaveNeeded = true;
    }
    return true;
}

void CSOLibrary::LoadClassTemplates()
{
    m_vClassTemplates.clear();

    string sLibPath = PathUtil::Make(CLASS_TEMPLATES_FOLDER, "*", "xml");
    ICryPak* pack = gEnv->pCryPak;
    _finddata_t fd;
    intptr_t handle = pack->FindFirst(sLibPath, &fd);
    int nCount = 0;
    if (handle < 0)
    {
        return;
    }

    do
    {
        XmlNodeRef root = GetISystem()->LoadXmlFromFile(string(CLASS_TEMPLATES_FOLDER) + fd.name);
        if (!root)
        {
            continue;
        }

        CClassTemplateData* data = AddClassTemplate(PathUtil::GetFileName(fd.name));
        if (!data)
        {
            continue;
        }

        int count = root->getChildCount();
        for (int i = 0; i < count; ++i)
        {
            XmlNodeRef node = root->getChild(i);
            if (node->isTag("object"))
            {
                data->model = node->getAttr("model");
            }
            else if (node->isTag("helper"))
            {
                CClassTemplateData::CTemplateHelper helper;
                helper.name = node->getAttr("name");
                node->getAttr("pos", helper.qt.t);
                node->getAttr("rot", helper.qt.q);
                node->getAttr("radius", helper.radius);
                node->getAttr("projectOnGround", helper.project);
                data->helpers.push_back(helper);
            }
        }
    } while (pack->FindNext(handle, &fd) >= 0);

    pack->FindClose(handle);
}

CSOLibrary::CClassTemplateData* CSOLibrary::AddClassTemplate(const char* name)
{
    CClassTemplateData classTemplateData;
    classTemplateData.name = name;
    VectorClassTemplateData::iterator it = std::lower_bound(m_vClassTemplates.begin(), m_vClassTemplates.end(), classTemplateData, less_name_no_case< CClassTemplateData >());

    if (it != m_vClassTemplates.end() && _stricmp(it->name, name) == 0)
    {
        return NULL;
    }

    return &*m_vClassTemplates.insert(it, classTemplateData);
}


//////////////////////////////////////////////////////////////////////////
// Smart Objects UI structures.
//////////////////////////////////////////////////////////////////////////

/** User Interface definition of Smart Objects
*/
class CSmartObjectsUIDefinition
{
public:
    CVariable< QString >    sName;
    CVariable< QString >    sFolder;
    CVariable< QString >    sDescription;
    CVariable< bool >       bEnabled;
    CVariable< QString >    sTemplate;

    CVariable< bool >       bNavigationRule;
    CVariable< QString >    sEvent;
    CVariable< QString >    sChainedUserEvent;
    CVariable< QString >    sChainedObjectEvent;

    CVariableArray          objectTable;
    CVariableArray          userTable;
    CVariableArray          navigationTable;
    CVariableArray          limitsTable;
    CVariableArray          delayTable;
    CVariableArray          multipliersTable;
    CVariableArray          actionTable;

    //////////////////////////////////////////////////////////////////////////
    // Object variables
    CVariable< QString >    objectClass;
    CVariable< QString >    objectState;
    CVariable< QString >    objectHelper;

    //////////////////////////////////////////////////////////////////////////
    // User variables
    CVariable< QString >    userClass;
    CVariable< QString >    userState;
    CVariable< QString >    userHelper;
    CVariable< int >        iMaxAlertness;

    //////////////////////////////////////////////////////////////////////////
    // Navigation variables
    CVariable< QString >    entranceHelper;
    CVariable< QString >    exitHelper;

    //////////////////////////////////////////////////////////////////////////
    // Limits variables
    CVariable< float >    limitsDistanceFrom;
    CVariable< float >    limitsDistanceTo;
    CVariable< float >    limitsOrientation;
    CVariable< bool >     horizLimitOnly;
    CVariable< float >    limitsOrientationToTarget;

    //////////////////////////////////////////////////////////////////////////
    // Delay variables
    CVariable< float >      delayMinimum;
    CVariable< float >      delayMaximum;
    CVariable< float >      delayMemory;

    //////////////////////////////////////////////////////////////////////////
    // Multipliers variables
    CVariable< float >      multipliersProximity;
    CVariable< float >      multipliersOrientation;
    CVariable< float >      multipliersVisibility;
    CVariable< float >      multipliersRandomness;

    //////////////////////////////////////////////////////////////////////////
    // Action variables
    CVariable< float >      fLookAtOnPerc;
    CVariable< QString >    userPreActionState;
    CVariable< QString >    objectPreActionState;
    CVariableEnum< int >    actionType;
    CVariable< QString >    actionName;
    CVariable< QString >    userPostActionState;
    CVariable< QString >    objectPostActionState;

    //////////////////////////////////////////////////////////////////////////
    // Exact positioning
    CVariableEnum< int >    approachSpeed;
    CVariableEnum< int >    approachStance;
    CVariable< QString >    animationHelper;
    CVariable< QString >    approachHelper;
    CVariable< float >      fStartWidth;
    CVariable< float >      fStartDirectionTolerance;
    CVariable< float >      fStartArcAngle;

    //  CVarEnumList<int> enumType;
    //  CVarEnumList<int> enumBlendType;

    CVarBlockPtr                m_vars;
    IVariable::OnSetCallback    m_onSetCallback;

    //////////////////////////////////////////////////////////////////////////
    CVarBlock* CreateVars()
    {
        m_vars = new CVarBlock;

        /*
                //////////////////////////////////////////////////////////////////////////
                // Init enums.
                //////////////////////////////////////////////////////////////////////////
                enumBlendType.AddRef(); // We not using pointer.
                enumBlendType.AddItem( "AlphaBlend",ParticleBlendType_AlphaBased );
                enumBlendType.AddItem( "ColorBased",ParticleBlendType_ColorBased );
                enumBlendType.AddItem( "Additive",ParticleBlendType_Additive );
                enumBlendType.AddItem( "None",ParticleBlendType_None );

                enumType.AddRef(); // We not using pointer.
                enumType.AddItem( "Billboard",PART_FLAG_BILLBOARD );
                enumType.AddItem( "Horizontal",PART_FLAG_HORIZONTAL );
                enumType.AddItem( "Underwater",PART_FLAG_UNDERWATER );
                enumType.AddItem( "Align To Direction",PART_FLAG_ALIGN_TO_DIR );
                enumType.AddItem( "Line",PART_FLAG_LINEPARTICLE );
        */

        AddVariable(m_vars, sName, "Name", IVariable::DT_SIMPLE, "The name for this smart object rule");
        AddVariable(m_vars, sDescription, "Description", IVariable::DT_SIMPLE, "A description for this smart object rule");
        AddVariable(m_vars, sFolder, "Location", IVariable::DT_SIMPLE, "The path to folder where this smart object rule is located");
        AddVariable(m_vars, bEnabled, "Enabled", IVariable::DT_SIMPLE, "Is this smart object rule active?");
        AddVariable(m_vars, sTemplate, "Template", IVariable::DT_SOTEMPLATE, "Template used on this smart object rule");

        AddVariable(m_vars, bNavigationRule, "Navigation Rule", IVariable::DT_SIMPLE, "Is this a navigation smart object rule?");
        AddVariable(m_vars, sEvent, "Event", IVariable::DT_SOEVENT, "Event on which this rule is triggered");
        AddVariable(m_vars, sChainedUserEvent, "Chained User Event", IVariable::DT_SOEVENT, "Event chained with the user");
        AddVariable(m_vars, sChainedObjectEvent, "Chained Object Event", IVariable::DT_SOEVENT, "Event chained with the object");
        //      AddVariable( m_vars, bRelativeToTarget, "Relative to Target" );

        // setup the limits
        iMaxAlertness.SetLimits(0.0f, 2.0f);

        //passRadius.SetLimits( 0.0f, 1000.0f );

        limitsDistanceFrom.SetLimits(0.0f, 1000.0f);
        limitsDistanceTo.SetLimits(0.0f, 1000.0f);
        limitsOrientation.SetLimits(-360.0f, 360.0f);
        limitsOrientationToTarget.SetLimits(-360.0f, 360.0f);

        delayMinimum.SetLimits(0.0f, 1000.0f);
        delayMaximum.SetLimits(0.0f, 1000.0f);
        delayMemory.SetLimits(0.0f, 1000.0f);

        multipliersRandomness.SetLimits(0.0f, 100.0f);
        fLookAtOnPerc.SetLimits(0.0f, 100.0f);

        fStartWidth.SetLimits(0.0f, 10.0f);
        fStartDirectionTolerance.SetLimits(0.0f, 180.0f);
        fStartArcAngle.SetLimits(-360.0f, 360.0f);

        // setup the enumeration lists
        actionType.AddEnumItem("None", 0);                      // eAT_None
        actionType.AddEnumItem("AI Action", 1);                 // eAT_Action
        actionType.AddEnumItem("High Priority Action", 2);      // eAT_PriorityAction
        //      actionType.AddEnumItem( "Go To Object", 3 );            // eAT_Approach
        //      actionType.AddEnumItem( "High Priority Go To", 4 );     // eAT_PriorityApproach
        //      actionType.AddEnumItem( "Go To + Action", 5 );          // eAT_ApproachAction
        //      actionType.AddEnumItem( "H.P. Go To + Action", 6 );     // eAT_PriorityApproachAction
        actionType.AddEnumItem("AI Signal", 7);                 // eAT_AISignal
        actionType.AddEnumItem("One Shot Animation", 8);        // eAT_AnimationSignal
        actionType.AddEnumItem("Looping Animation", 9);         // eAT_AnimationAction
        actionType.AddEnumItem("HP One Shot Animation", 10);        // eAT_PriorityAnimationSignal
        actionType.AddEnumItem("HP Looping Animation", 11);         // eAT_PriorityAnimationAction

        approachSpeed.AddEnumItem("<ignore>", -1);
        approachSpeed.AddEnumItem("Normal/Walk", 0);
        approachSpeed.AddEnumItem("Fast/Run", 1);
        approachSpeed.AddEnumItem("Very Fast/Sprint", 2);

        approachStance.AddEnumItem("<ignore>", -1);
        approachStance.AddEnumItem("Combat", STANCE_STAND);
        approachStance.AddEnumItem("Crouch", STANCE_CROUCH);
        approachStance.AddEnumItem("Prone", STANCE_PRONE);
        approachStance.AddEnumItem("Relaxed", STANCE_RELAXED);
        approachStance.AddEnumItem("Stealth", STANCE_STEALTH);
        approachStance.AddEnumItem("Alerted", STANCE_ALERTED);
        approachStance.AddEnumItem("LowCover", STANCE_LOW_COVER);
        approachStance.AddEnumItem("HighCover", STANCE_HIGH_COVER);

        //////////////////////////////////////////////////////////////////////////
        // Init tables.
        //////////////////////////////////////////////////////////////////////////

        objectTable.DeleteAllVariables();
        userTable.DeleteAllVariables();
        navigationTable.DeleteAllVariables();
        limitsTable.DeleteAllVariables();
        delayTable.DeleteAllVariables();
        multipliersTable.DeleteAllVariables();
        actionTable.DeleteAllVariables();

        AddVariable(m_vars, userTable, "User", IVariable::DT_SIMPLE, "Define the smart object user");
        AddVariable(m_vars, objectTable, "Object", IVariable::DT_SIMPLE, "Define the smart object to be used\nIf empty the user will use himself");
        AddVariable(m_vars, navigationTable, "Navigation", IVariable::DT_SIMPLE, "An AI navigation link will be created between these two helpers");
        AddVariable(m_vars, limitsTable, "Limits", IVariable::DT_SIMPLE, "Only objects within this range will be considered");
        AddVariable(m_vars, delayTable, "Delay", IVariable::DT_SIMPLE, "");
        AddVariable(m_vars, multipliersTable, "Multipliers", IVariable::DT_SIMPLE, "");
        AddVariable(m_vars, actionTable, "Action", IVariable::DT_SIMPLE, "");

        userTable.SetFlags(IVariable::UI_BOLD);
        objectTable.SetFlags(IVariable::UI_BOLD);
        navigationTable.SetFlags(IVariable::UI_BOLD);
        limitsTable.SetFlags(IVariable::UI_BOLD);
        delayTable.SetFlags(IVariable::UI_BOLD);
        multipliersTable.SetFlags(IVariable::UI_BOLD);
        actionTable.SetFlags(IVariable::UI_BOLD);

        AddVariable(userTable, userClass, "Class", IVariable::DT_SOCLASS, "Only users of this class will be considered");
        AddVariable(userTable, userState, "State Pattern", IVariable::DT_SOSTATEPATTERN, "Only users which state matches this pattern will be considered");
        AddVariable(userTable, userHelper, "Helper", IVariable::DT_SOHELPER, "User's helper point used in calculations");
        AddVariable(userTable, iMaxAlertness, "Max. Alertness", IVariable::DT_SIMPLE, "Consider only users which alertness state is not higher");

        AddVariable(objectTable, objectClass, "Class", IVariable::DT_SOCLASS, "Only objects of this class will be considered");
        AddVariable(objectTable, objectState, "State Pattern", IVariable::DT_SOSTATEPATTERN, "Only objects which state matches this pattern will be considered");
        AddVariable(objectTable, objectHelper, "Helper", IVariable::DT_SOHELPER, "Object's helper point used in calculations");

        //      AddVariable( navigationTable, passRadius, "Pass Radius", IVariable::DT_SIMPLE, "Defines a navigational smart object if higher than zero" );
        AddVariable(navigationTable, entranceHelper, "Entrance", IVariable::DT_SONAVHELPER, "Entrance point of navigational smart object");
        AddVariable(navigationTable, exitHelper, "Exit", IVariable::DT_SONAVHELPER, "Exit point of navigational smart object");

        AddVariable(limitsTable, limitsDistanceFrom, "Distance from", IVariable::DT_SIMPLE, "Range at which objects could be used");
        AddVariable(limitsTable, limitsDistanceTo, "Distance to", IVariable::DT_SIMPLE, "Range at which objects could be used");
        AddVariable(limitsTable, limitsOrientation, "Orientation", IVariable::DT_SIMPLE, "Orientation limit at which objects could be used");
        AddVariable(limitsTable, horizLimitOnly, "Horizontal Limit Only", IVariable::DT_SIMPLE, "When checked, the limit only applies to horizontal orientation (vertical orientation is excluded)");
        AddVariable(limitsTable, limitsOrientationToTarget, "Orient. to Target", IVariable::DT_SIMPLE, "Object's orientation limit towards the user's attention target");

        AddVariable(delayTable, delayMinimum, "Minimum", IVariable::DT_SIMPLE, "Shortest time needed for interaction to happen");
        AddVariable(delayTable, delayMaximum, "Maximum", IVariable::DT_SIMPLE, "Longest time needed for interaction to happen");
        AddVariable(delayTable, delayMemory, "Memory", IVariable::DT_SIMPLE, "Time needed to forget");

        AddVariable(multipliersTable, multipliersProximity, "Proximity", IVariable::DT_SIMPLE, "How important is the proximity");
        AddVariable(multipliersTable, multipliersOrientation, "Orientation", IVariable::DT_SIMPLE, "How important is the orientation of the user towards the object");
        AddVariable(multipliersTable, multipliersVisibility, "Visibility", IVariable::DT_SIMPLE, "How important is the visibility between user and object");
        AddVariable(multipliersTable, multipliersRandomness, "Randomness", IVariable::DT_PERCENT, "How much randomness will be added");

        AddVariable(actionTable, fLookAtOnPerc, "LookAt On %", IVariable::DT_PERCENT, "Look at the object before using it (only when the user is an AI agent)");
        AddVariable(actionTable, userPreActionState, "User: Pre-Action State", IVariable::DT_SOSTATES, "How user's states will change before executing the action");
        AddVariable(actionTable, objectPreActionState, "Object: Pre-Action State", IVariable::DT_SOSTATES, "How object's states will change before executing the action");
        AddVariable(actionTable, actionType, "Action Type", IVariable::DT_SIMPLE, "What type of action to execute");
        AddVariable(actionTable, actionName, "Action Name", IVariable::DT_SOACTION, "The action to be executed on interaction");
        AddVariable(actionTable, userPostActionState, "User: Post-Action State", IVariable::DT_SOSTATES, "How user's states will change after executing the action");
        AddVariable(actionTable, objectPostActionState, "Object: Post-Action State", IVariable::DT_SOSTATES, "How object's states will change after executing the action");

        AddVariable(actionTable, approachSpeed, "Speed", IVariable::DT_SIMPLE, "Movement speed while approaching the helper point");
        AddVariable(actionTable, approachStance, "Stance", IVariable::DT_SIMPLE, "Body stance while approaching the helper point");
        AddVariable(objectTable, animationHelper, "Animation Helper", IVariable::DT_SOANIMHELPER, "Object's helper point at which the animation will be played");
        AddVariable(objectTable, approachHelper, "Approach Helper", IVariable::DT_SOHELPER, "Object's helper point to approach before the animation play (if empty Animation Helper is used)");
        AddVariable(actionTable, fStartWidth, "Start Line Width", IVariable::DT_SIMPLE, "Width of the starting line for playing animation");
        AddVariable(actionTable, fStartArcAngle, "Arc Angle", IVariable::DT_SIMPLE, "Arc angle if the start line need to be bended");
        AddVariable(actionTable, fStartDirectionTolerance, "Direction Tolerance", IVariable::DT_SIMPLE, "Start direction tolerance for playing animation");

        return m_vars;
    }

    //////////////////////////////////////////////////////////////////////////
    // returns true if folder was edited
    bool GetConditionFromUI(SmartObjectCondition& condition) const
    {
        QString temp;
        bool result = false;

        condition.sName = static_cast<QString>(sName).toUtf8().data();
        condition.sDescription = static_cast<QString>(sDescription).toUtf8().data();

        QString newLocation;
        QString location = sFolder;
        int i = 0;
        for (auto folder : location.split(QRegularExpression(QStringLiteral(R"([/\\])")), QString::SkipEmptyParts))
        {
            newLocation += '/';
            newLocation += folder;
        }
        if (newLocation != condition.sFolder.c_str())
        {
            result = true;
            condition.sFolder = newLocation.toUtf8().data();
        }

        condition.bEnabled = bEnabled;
        condition.iRuleType = bNavigationRule ? 1 : 0;
        condition.sEvent = static_cast<QString>(sEvent).toUtf8().data();
        condition.sChainedUserEvent = static_cast<QString>(sChainedUserEvent).toUtf8().data();
        condition.sChainedObjectEvent = static_cast<QString>(sChainedObjectEvent).toUtf8().data();

        QRegularExpression reg("(^[ :]+)|([ :]+$)");

        userClass.Get(temp);
        condition.sUserClass = temp.remove(reg).toUtf8().data();
        userState.Get(temp);
        condition.sUserState = temp.toUtf8().data();
        userHelper.Get(temp);
        temp.remove(0, temp.lastIndexOf(':') + 1);
        condition.sUserHelper = temp.toUtf8().data();
        condition.iMaxAlertness = iMaxAlertness;

        objectClass.Get(temp);
        condition.sObjectClass = temp.remove(reg).toUtf8().data();
        objectState.Get(temp);
        condition.sObjectState = temp.toUtf8().data();
        objectHelper.Get(temp);
        temp.remove(0, temp.lastIndexOf(':') + 1);
        condition.sObjectHelper = temp.toUtf8().data();

        entranceHelper.Get(temp);
        temp.remove(0, temp.lastIndexOf(':') + 1);
        condition.sEntranceHelper = temp.toUtf8().data();
        exitHelper.Get(temp);
        temp.remove(0, temp.lastIndexOf(':') + 1);
        condition.sExitHelper = temp.toUtf8().data();

        condition.fDistanceFrom = limitsDistanceFrom;
        condition.fDistanceTo = limitsDistanceTo;
        condition.fOrientationLimit = limitsOrientation;
        condition.bHorizLimitOnly = horizLimitOnly;
        condition.fOrientationToTargetLimit = limitsOrientationToTarget;

        condition.fMinDelay = delayMinimum;
        condition.fMaxDelay = delayMaximum;
        condition.fMemory = delayMemory;

        condition.fProximityFactor = multipliersProximity;
        condition.fOrientationFactor = multipliersOrientation;
        condition.fVisibilityFactor = multipliersVisibility;
        condition.fRandomnessFactor = multipliersRandomness;

        condition.fLookAtOnPerc = fLookAtOnPerc;
        objectPreActionState.Get(temp);
        condition.sObjectPreActionState = temp.toUtf8().data();
        userPreActionState.Get(temp);
        condition.sUserPreActionState = temp.toUtf8().data();
        condition.eActionType = (EActionType) (int) actionType;
        actionName.Get(temp);
        condition.sAction = temp.toUtf8().data();
        objectPostActionState.Get(temp);
        condition.sObjectPostActionState = temp.toUtf8().data();
        userPostActionState.Get(temp);
        condition.sUserPostActionState = temp.toUtf8().data();

        condition.fApproachSpeed = approachSpeed;
        condition.iApproachStance = approachStance;
        animationHelper.Get(temp);
        temp.remove(0, temp.lastIndexOf(':') + 1);
        condition.sAnimationHelper = temp.toUtf8().data();
        approachHelper.Get(temp);
        temp.remove(0, temp.lastIndexOf(':') + 1);
        condition.sApproachHelper = temp.toUtf8().data();
        condition.fStartWidth = fStartWidth;
        condition.fDirectionTolerance = fStartDirectionTolerance;
        condition.fStartArcAngle = fStartArcAngle;

        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void SetConditionToUI(const SmartObjectCondition& condition, ReflectedPropertyControl* pPropertyCtrl, QWidget* messageBoxParent)
    {
        const MapTemplates& mapTemplates = GetIEditor()->GetAI()->GetMapTemplates();
        MapTemplates::const_iterator find = mapTemplates.find(condition.iTemplateId);
        if (find == mapTemplates.end())
        {
            find = mapTemplates.find(0);
        }

        // template id 0 must always exist!
        // if doesn't then probably templates aren't loaded...
        assert(find != mapTemplates.end());

        CSOTemplate* pTemplate = find->second;
        sTemplate = pTemplate->name;

        sName = condition.sName.c_str();
        sFolder = condition.sFolder.c_str();
        sDescription = condition.sDescription.c_str();

        bEnabled = condition.bEnabled;
        bNavigationRule = condition.iRuleType == 1;
        sEvent = condition.sEvent.c_str();
        sChainedUserEvent = condition.sChainedUserEvent.c_str();
        sChainedObjectEvent = condition.sChainedObjectEvent.c_str();

        QRegularExpression reg("(^[ :]+)|([ :]+$)");

        QString temp = QtUtil::ToQString(condition.sUserClass);
        userClass = temp.remove(reg).toUtf8().data();
        userState = condition.sUserState.c_str();
        userHelper = temp.isEmpty() ? "" : temp + ':' + condition.sUserHelper.c_str();
        iMaxAlertness = condition.iMaxAlertness;

        temp = condition.sObjectClass;
        objectClass = temp.remove(reg).toUtf8().data();
        objectState = condition.sObjectState.c_str();
        objectHelper = temp.isEmpty() ? "" : temp + ':' + condition.sObjectHelper.c_str();

        entranceHelper = temp + ':' + condition.sEntranceHelper.c_str();
        exitHelper = temp + ':' + condition.sExitHelper.c_str();

        limitsDistanceFrom = condition.fDistanceFrom;
        limitsDistanceTo = condition.fDistanceTo;
        limitsOrientation = condition.fOrientationLimit;
        horizLimitOnly = condition.bHorizLimitOnly;
        limitsOrientationToTarget = condition.fOrientationToTargetLimit;

        delayMinimum = condition.fMinDelay;
        delayMaximum = condition.fMaxDelay;
        delayMemory = condition.fMemory;

        multipliersProximity = condition.fProximityFactor;
        multipliersOrientation = condition.fOrientationFactor;
        multipliersVisibility = condition.fVisibilityFactor;
        multipliersRandomness = condition.fRandomnessFactor;

        fLookAtOnPerc = condition.fLookAtOnPerc;
        userPreActionState = condition.sUserPreActionState.c_str();
        objectPreActionState = condition.sObjectPreActionState.c_str();
        actionType = condition.eActionType;
        actionName = condition.sAction.c_str();
        userPostActionState = condition.sUserPostActionState.c_str();
        objectPostActionState = condition.sObjectPostActionState.c_str();

        approachSpeed = condition.fApproachSpeed;
        approachStance = condition.iApproachStance;
        animationHelper = temp.isEmpty() ? "" : temp + ':' + condition.sAnimationHelper.c_str();
        approachHelper = temp.isEmpty() ? "" : temp + ':' + condition.sApproachHelper.c_str();
        fStartWidth = condition.fStartWidth;
        fStartDirectionTolerance = condition.fDirectionTolerance;
        fStartArcAngle = condition.fStartArcAngle;

        // remove all variables from UI
        m_vars->DeleteAllVariables();

        // add standard variables
        m_vars->AddVariable(&sName);
        m_vars->AddVariable(&sDescription);
        m_vars->AddVariable(&sFolder);
        m_vars->AddVariable(&bEnabled);
        m_vars->AddVariable(&sTemplate);

        // add template variables
        AddVariablesFromTemplate(m_vars, pTemplate->params, messageBoxParent);

        pPropertyCtrl->RemoveAllItems();
        pPropertyCtrl->AddVarBlock(m_vars);

        pPropertyCtrl->ExpandAll();
        for (int i = 0; i < m_collapsedItems.size(); ++i)
        {
            pPropertyCtrl->Expand(pPropertyCtrl->FindItemByVar(m_collapsedItems[i]), false);
        }

        m_collapsedItems.clear();
    }

    IVariable* ResolveVariable(const CSOParam* pParam, QWidget* messageBoxParent)
    {
        IVariable* pVar = NULL;
        if (pParam->sName == "bNavigationRule")
        {
            return &bNavigationRule;
        }
        else if (pParam->sName == "sEvent")
        {
            return &sEvent;
        }
        else if (pParam->sName == "sChainedUserEvent")
        {
            return &sChainedUserEvent;
        }
        else if (pParam->sName == "sChainedObjectEvent")
        {
            return &sChainedObjectEvent;
        }
        else if (pParam->sName == "userClass")
        {
            return &userClass;
        }
        else if (pParam->sName == "userState")
        {
            return &userState;
        }
        else if (pParam->sName == "userHelper")
        {
            return &userHelper;
        }
        else if (pParam->sName == "iMaxAlertness")
        {
            return &iMaxAlertness;
        }
        else if (pParam->sName == "objectClass")
        {
            return &objectClass;
        }
        else if (pParam->sName == "objectState")
        {
            return &objectState;
        }
        else if (pParam->sName == "objectHelper")
        {
            return &objectHelper;
        }
        else if (pParam->sName == "entranceHelper")
        {
            return &entranceHelper;
        }
        else if (pParam->sName == "exitHelper")
        {
            return &exitHelper;
        }
        else if (pParam->sName == "limitsDistanceFrom")
        {
            return &limitsDistanceFrom;
        }
        else if (pParam->sName == "limitsDistanceTo")
        {
            return &limitsDistanceTo;
        }
        else if (pParam->sName == "limitsOrientation")
        {
            return &limitsOrientation;
        }
        else if (pParam->sName == "horizLimitOnly")
        {
            return &horizLimitOnly;
        }
        else if (pParam->sName == "limitsOrientationToTarget")
        {
            return &limitsOrientationToTarget;
        }
        else if (pParam->sName == "delayMinimum")
        {
            return &delayMinimum;
        }
        else if (pParam->sName == "delayMaximum")
        {
            return &delayMaximum;
        }
        else if (pParam->sName == "delayMemory")
        {
            return &delayMemory;
        }
        else if (pParam->sName == "multipliersProximity")
        {
            return &multipliersProximity;
        }
        else if (pParam->sName == "multipliersOrientation")
        {
            return &multipliersOrientation;
        }
        else if (pParam->sName == "multipliersVisibility")
        {
            return &multipliersVisibility;
        }
        else if (pParam->sName == "multipliersRandomness")
        {
            return &multipliersRandomness;
        }
        else if (pParam->sName == "fLookAtOnPerc")
        {
            return &fLookAtOnPerc;
        }
        else if (pParam->sName == "userPreActionState")
        {
            return &userPreActionState;
        }
        else if (pParam->sName == "objectPreActionState")
        {
            return &objectPreActionState;
        }
        else if (pParam->sName == "actionType")
        {
            return &actionType;
        }
        else if (pParam->sName == "actionName")
        {
            return &actionName;
        }
        else if (pParam->sName == "userPostActionState")
        {
            return &userPostActionState;
        }
        else if (pParam->sName == "objectPostActionState")
        {
            return &objectPostActionState;
        }
        else if (pParam->sName == "approachSpeed")
        {
            return &approachSpeed;
        }
        else if (pParam->sName == "approachStance")
        {
            return &approachStance;
        }
        else if (pParam->sName == "animationHelper")
        {
            return &animationHelper;
        }
        else if (pParam->sName == "approachHelper")
        {
            return &approachHelper;
        }
        else if (pParam->sName == "fStartRadiusTolerance")   // backward compatibility
        {
            return &fStartWidth;
        }
        else if (pParam->sName == "fStartWidth")
        {
            return &fStartWidth;
        }
        else if (pParam->sName == "fStartDirectionTolerance")
        {
            return &fStartDirectionTolerance;
        }
        else if (pParam->sName == "fTargetRadiusTolerance")   // backward compatibility
        {
            return &fStartArcAngle;
        }
        else if (pParam->sName == "fStartArcAngle")
        {
            return &fStartArcAngle;
        }

        QMessageBox::critical(messageBoxParent, QString(), QObject::tr("WARNING:\nSmart object template has a Param tag named %1 which is not recognized as valid name!\nThe Param will be ignored...").arg(pParam->sName));
        return NULL;
    }

private:
    std::vector< IVariable* > m_collapsedItems;

    void AddVariablesFromTemplate(CVarBlock* var, CSOParamBase* param, QWidget* messageBoxParent)
    {
        while (param)
        {
            if (!param->bIsGroup)
            {
                CSOParam* pParam = static_cast< CSOParam* >(param);
                if (pParam->bVisible)
                {
                    if (!pParam->pVariable)
                    {
                        pParam->pVariable = ResolveVariable(pParam, messageBoxParent);
                    }

                    if (pParam->pVariable)
                    {
                        pParam->pVariable->SetHumanName(pParam->sCaption);
                        pParam->pVariable->SetDescription(pParam->sHelp);
                        pParam->pVariable->SetFlags(pParam->bEditable ? 0 : IVariable::UI_DISABLED);
                        var->AddVariable(param->pVariable);
                    }
                }
            }
            else
            {
                CSOParamGroup* pParam = static_cast< CSOParamGroup* >(param);
                pParam->pVariable->DeleteAllVariables();

                pParam->pVariable->SetName(pParam->sName);
                pParam->pVariable->SetDescription(pParam->sHelp);
                var->AddVariable(pParam->pVariable);
                AddVariablesFromTemplate(pParam->pVariable, pParam->pChildren, messageBoxParent);

                if (!pParam->bExpand)
                {
                    m_collapsedItems.push_back(pParam->pVariable);
                }
            }
            param = param->pNext;
        }
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariablesFromTemplate(IVariable* var, CSOParamBase* param, QWidget* messageBoxParent)
    {
        while (param)
        {
            if (!param->bIsGroup)
            {
                CSOParam* pParam = static_cast< CSOParam* >(param);
                if (pParam->bVisible)
                {
                    if (!pParam->pVariable)
                    {
                        pParam->pVariable = ResolveVariable(pParam, messageBoxParent);
                    }

                    if (pParam->pVariable)
                    {
                        pParam->pVariable->SetHumanName(pParam->sCaption);
                        pParam->pVariable->SetDescription(pParam->sHelp);
                        pParam->pVariable->SetFlags(pParam->bEditable ? 0 : IVariable::UI_DISABLED);
                        var->AddVariable(param->pVariable);
                    }
                }
            }
            else
            {
                CSOParamGroup* pParam = static_cast< CSOParamGroup* >(param);
                pParam->pVariable->DeleteAllVariables();

                pParam->pVariable->SetName(pParam->sName);
                pParam->pVariable->SetDescription(pParam->sHelp);
                var->AddVariable(pParam->pVariable);
                AddVariablesFromTemplate(pParam->pVariable, pParam->pChildren, messageBoxParent);

                if (!pParam->bExpand)
                {
                    m_collapsedItems.push_back(pParam->pVariable);
                }
            }
            param = param->pNext;
        }
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVariableArray& varArray, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE, const char* description = NULL)
    {
        var.AddRef(); // Variables are local and must not be released by CVarBlock.
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        if (description)
        {
            var.SetDescription(description);
        }
        if (m_onSetCallback)
        {
            var.AddOnSetCallback(m_onSetCallback);
        }
        varArray.AddVariable(&var);
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVarBlock* vars, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE, const char* description = NULL)
    {
        var.AddRef(); // Variables are local and must not be released by CVarBlock.
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        if (description)
        {
            var.SetDescription(description);
        }
        if (m_onSetCallback)
        {
            var.AddOnSetCallback(m_onSetCallback);
        }
        vars->AddVariable(&var);
    }
};

static CSmartObjectsUIDefinition    gSmartObjectsUI;

void CSmartObjectsEditorDialog::OnContextMenu(const QPoint& point)
{
    int selCount = m_View->selectionModel()->selectedRows().count();

    QMenu menu;
    QAction* actionNew = menu.addAction(tr("New..."));
    QAction* actionDuplicate = menu.addAction(tr("Duplicate..."));
    QAction* actionDelete = menu.addAction(tr("Delete"));
    actionDuplicate->setEnabled(selCount == 1);
    actionDelete->setEnabled(selCount > 0);

    connect(actionNew, &QAction::triggered, this, &CSmartObjectsEditorDialog::OnAddEntry);
    connect(actionDuplicate, &QAction::triggered, this, &CSmartObjectsEditorDialog::OnDuplicateEntry);
    connect(actionDelete, &QAction::triggered, this, &CSmartObjectsEditorDialog::OnRemoveEntry);

    //menu.AppendMenu( MF_SEPARATOR );

    /*
    // create columns items
    CXTPReportColumns* pColumns = m_View.GetColumns();
    int nColumnCount = pColumns->GetCount();
    for ( int i = 0; i < nColumnCount; ++i )
    {
    CXTPReportColumn* pCol = pColumns->GetAt(i);
    CString sCaption = pCol->GetCaption();
    if ( !sCaption.IsEmpty() )
    {
    menu.AppendMenu( MF_STRING, ID_COLUMN_SHOW + i, sCaption );
    menu.CheckMenuItem( ID_COLUMN_SHOW + i, MF_BYCOMMAND | (pCol->IsVisible() ? MF_CHECKED : MF_UNCHECKED) );
    }
    }
    */

    // track menu
    menu.exec(m_View->viewport()->mapToGlobal(point));
}


//////////////////////////////////////////////////////////////////////////

class SmartObjectsEditorModel
    : public QAbstractTableModel
{
public:
    SmartObjectsEditorModel(QObject* parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    enum Column
    {
        ColumnName,
        ColumnDescription,
        ColumnUserClass,
        ColumnUserState,
        ColumnObjectClass,
        ColumnObjectState,
        ColumnAction,
        ColumnOrder
    };


    void MoveAllRules(const QString& from, const QString& to);

    QMimeData* mimeData(const QModelIndexList& indexes) const override
    {
        QMimeData* d = new QMimeData;
        QVector<int> rows;
        std::for_each(indexes.begin(), indexes.end(), [&](const QModelIndex& index)
            {
                if (!rows.contains(index.row()))
                {
                    rows.push_back(index.row());
                }
            });
        d->setData(QStringLiteral("x-application/x-smartobjects-rows"), QByteArray(reinterpret_cast<char*>(rows.data()), rows.count() * sizeof(int)));
        return d;
    }

    QStringList mimeTypes() const override
    {
        return QStringList() << QStringLiteral("x-application/x-smartobjects-rows");
    }

    void addCondition(const SmartObjectCondition& condition)
    {
        const int row = rowCount();
        beginInsertRows(QModelIndex(), row, row);
        CSOLibrary::GetConditions().push_back(condition);
        endInsertRows();
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        if (row < 0 || row + count > rowCount(parent))
        {
            return false;
        }

        auto first = CSOLibrary::GetConditions().begin();
        std::advance(first, row);
        auto last = CSOLibrary::GetConditions().begin();
        std::advance(last, row + count);
        beginRemoveRows(QModelIndex(), row, row + count - 1);
        CSOLibrary::GetConditions().erase(first, last);
        endRemoveRows();
        return true;
    }

    void reloadItems()
    {
        beginResetModel();
        endResetModel();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 8;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : CSOLibrary::GetConditions().size();
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
        if (index.column() < 2)
        {
            f |= Qt::ItemIsEditable;
        }
        if (index.column() == 0)
        {
            f |= Qt::ItemIsUserCheckable;
        }
        return f;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()) || index.column() >= columnCount(index.parent()))
        {
            return QVariant();
        }

        auto condition = CSOLibrary::GetConditions().begin();
        std::advance(condition, index.row());

        static QFont underline;
        underline.setUnderline(true);

        if (role == Qt::CheckStateRole && index.column() == 0)
        {
            return condition->bEnabled ? Qt::Checked : Qt::Unchecked;
        }

        if (role == Qt::UserRole)
        {
            return QVariant::fromValue<SmartObjectCondition*>(&(*condition));
        }

        if (role == Qt::ForegroundRole && index.column() == ColumnAction && condition->eActionType == eAT_Action)
        {
            return QPalette().color(QPalette::Link);
        }

        if (role == Qt::FontRole && index.column() == ColumnAction && condition->eActionType == eAT_Action)
        {
            return underline;
        }

        if (role == Qt::ForegroundRole)
        {
            return condition->bEnabled ? QVariant() : QPalette().color(QPalette::Disabled, QPalette::WindowText);
        }

        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            switch (index.column())
            {
            case ColumnName:
                return QString(condition->sName.c_str());
            case ColumnDescription:
                return QString(condition->sDescription.c_str());
            case ColumnUserClass:
                return QString(condition->sUserClass.c_str());
            case ColumnUserState:
                return QString(condition->sUserState.c_str());
            case ColumnObjectClass:
                return QString(condition->sObjectClass.c_str());
            case ColumnObjectState:
                return QString(condition->sObjectState.c_str());
            case ColumnAction:
                return QString(condition->sAction.c_str());
            case ColumnOrder:
                return condition->iOrder;
            }
        }

        return QVariant();
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()) || index.column() >= columnCount(index.parent()))
        {
            return false;
        }

        if (!CSOLibrary::StartEditing())
        {
            return false;
        }

        auto condition = CSOLibrary::GetConditions().begin();
        std::advance(condition, index.row());

        if (role == Qt::EditRole)
        {
            switch (index.column())
            {
            case ColumnName:
                if (condition->sName == value.toString().toUtf8().data())
                {
                    return true;
                }
                condition->sName = value.toString().toUtf8().data();
                emit dataChanged(index, index);
                return true;
            case ColumnDescription:
                if (condition->sDescription == value.toString().toUtf8().data())
                {
                    return true;
                }
                condition->sDescription = value.toString().toUtf8().data();
                emit dataChanged(index, index);
                return true;
            }
        }
        else if (role == Qt::UserRole)
        {
            auto cond = value.value<SmartObjectCondition*>();
            Q_ASSERT(cond == &(*condition));
            emit dataChanged(index, index.sibling(index.row(), columnCount() - 1));
            return true;
        }
        else if (role == Qt::CheckStateRole)
        {
            if (condition->bEnabled == (value.toInt() == Qt::Checked))
            {
                return true;
            }
            condition->bEnabled = (value.toInt() == Qt::Checked);
            emit dataChanged(index, index.sibling(index.row(), columnCount() - 1));
            return true;
        }

        return false;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::DisplayRole)
            {
                switch (section)
                {
                case ColumnName:
                    return tr("Name");
                case ColumnDescription:
                    return tr("Description");
                case ColumnUserClass:
                    return tr("User's Class");
                case ColumnUserState:
                    return tr("User's State Pattern");
                case ColumnObjectClass:
                    return tr("Object's Class");
                case ColumnObjectState:
                    return tr("Object's State Pattern");
                case ColumnAction:
                    return tr("Action");
                case ColumnOrder:
                    return QString();
                default:
                    return QVariant();
                }
            }
        }

        return QAbstractTableModel::headerData(section, orientation, role);
    }
};

class SmartObjectsEditorTreeModel
    : public AbstractGroupProxyModel
{
public:
    SmartObjectsEditorTreeModel(QObject* parent)
        : AbstractGroupProxyModel(parent)
    {
    }

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override
    {
        if (row != -1 || column != -1)
        {
            return false;
        }

        const QByteArray d = data->data(QStringLiteral("x-application/x-smartobjects-rows"));
        QVector<int> rows(d.count() / sizeof(int));
        AZStd::copy(d.begin(), d.end(), reinterpret_cast<char*>(rows.data()));
        const QString path = parent.data(Qt::UserRole).toString();
        for (auto i : rows)
        {
            const QModelIndex index = sourceModel()->index(i, 0);
            auto cond = index.data(Qt::UserRole).value<SmartObjectCondition*>();
            cond->sFolder = path.toUtf8().data();
            sourceModel()->setData(index, QVariant::fromValue(cond), Qt::UserRole);
        }
        return true;
    }

    QStringList mimeTypes() const override
    {
        return QStringList() << QStringLiteral("x-application/x-smartobjects-rows");
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        const QModelIndex sourceIndex = mapToSource(index);
        if (!sourceIndex.isValid() && index.isValid())
        {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
        }
        return sourceModel()->flags(sourceIndex);
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DecorationRole)
        {
            return QIcon(":/HyperGraph/graphs/graph_folder.png");
        }
        else if (role == Qt::UserRole && !index.parent().isValid())
        {
            return QVariant();
        }
        else if (role == Qt::UserRole)
        {
            return data(index.parent(), Qt::UserRole).toString() + QString("/") + data(index).toString();
        }

        else
        {
            return AbstractGroupProxyModel::data(index, role == Qt::EditRole ? Qt::DisplayRole : role);
        }
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (role == Qt::EditRole)
        {
            if (!index.parent().isValid())
            {
                return false;
            }
            const QString folder = value.toString();
            // first check is there already a folder with than name
            QModelIndexList matches = match(SmartObjectsEditorTreeModel::index(0, 0, index.parent()), Qt::DisplayRole, folder, 1, Qt::MatchFixedString);
            matches.removeAll(index);
            if (!matches.isEmpty())
            {
                QMessageBox::warning(qobject_cast<QWidget*>(QObject::parent()), QString(), tr("There is already a sibling folder with that name!"));
                return false;
            }

            // renaming is allowed => update paths
            const QString from = data(index, Qt::UserRole).toString();
            const QString to = data(index.parent(), Qt::UserRole).toString() + QStringLiteral("/") + folder;
            static_cast<SmartObjectsEditorModel*>(sourceModel())->MoveAllRules(from, to);
        }
        return false;
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return 1;
    }

    QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override
    {
        auto cond = sourceIndex.data(Qt::UserRole).value<SmartObjectCondition*>();
        QStringList group = QString(cond->sFolder.c_str()).split(QRegularExpression(QStringLiteral("[\\/]")));
        if (group.isEmpty())
        {
            return QStringList();
        }
        if (group.first().isEmpty())
        {
            group[0] = QStringLiteral("/");
        }
        return group;
    }
};

class LeafRemovingFilterModel
    : public QSortFilterProxyModel
{
public:
    LeafRemovingFilterModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        return sourceModel()->hasChildren(sourceModel()->index(source_row, 0, source_parent));
    }
};

class SmartObjectsPathFilterModel
    : public QSortFilterProxyModel
{
public:
    SmartObjectsPathFilterModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    void setFilterClasses(const SetStrings& classes)
    {
        m_filterClasses = classes;
        invalidateFilter();
    }

    void setPath(const QString& path)
    {
        QString p = path;
        p.replace('\\', '/');
        while (p.startsWith('/'))
        {
            p.remove(0, 1);
        }
        if (m_path == p)
        {
            return;
        }
        m_path = p;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        auto condition = sourceModel()->index(source_row, 0, source_parent).data(Qt::UserRole).value<SmartObjectCondition*>();
        QString folder(condition->sFolder);
        folder.replace('\\', '/');
        while (folder.startsWith('/'))
        {
            folder.remove(0, 1);
        }
        bool folderMatches = folder.compare(m_path, Qt::CaseInsensitive) == 0;

        if (!folderMatches)
        {
            return false;
        }

        if (m_filterClasses.empty())
        {
            return true;
        }

        return m_filterClasses.find(condition->sUserClass) != m_filterClasses.end() ||
               !condition->sObjectClass.empty() && m_filterClasses.find(condition->sObjectClass) != m_filterClasses.end();
    }

private:
    QString m_path;
    SetStrings m_filterClasses;
};

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.paneRect = QRect(100, 250, 500, 500);
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;

    AzToolsFramework::RegisterViewPane<CSmartObjectsEditorDialog>(s_smartObjectsDialogName, LyViewPane::CategoryOther, options);
    GetIEditor()->GetSettingsManager()->AddToolName("SmartObjectsLayout", "Smart Objects");
}

const GUID& CSmartObjectsEditorDialog::GetClassID()
{
    // {9bc75ea1-6556-42b7-a23a-55036906027c}
    static const GUID guid = {
        0x9bc75ea1, 0x6556, 0x42b7, { 0xa2, 0x3a, 0x55, 0x03, 0x69, 0x06, 0x02, 0x7c }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
CSmartObjectsEditorDialog::CSmartObjectsEditorDialog(QWidget* parent)
    : QMainWindow(parent)
    , m_bIgnoreNotifications(false)
    , m_View(new QTreeView(this))
    , m_topLabel(new SmartObjectsEditorDialogLabel)
    , m_Tree(new QTreeView(this))
{
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setLayout(new QVBoxLayout);
    centralWidget->layout()->setSpacing(0);
    centralWidget->layout()->addWidget(m_topLabel);
    centralWidget->layout()->addWidget(m_View);
    setCentralWidget(centralWidget);

    connect(m_topLabel, &SmartObjectsEditorDialogLabel::closeRequested, this, [this]() {
        m_bFilterCanceled = true;
        m_bSinkNeeded = true;
    });

    GetIEditor()->RegisterNotifyListener(this);
    ++CSOLibrary::m_iNumEditors;
    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CSmartObjectsEditorDialog::OnObjectEvent));
    m_bSinkNeeded = true;

    OnInitDialog();

    m_View->setContextMenuPolicy(Qt::CustomContextMenu);
    m_View->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_View, &QWidget::customContextMenuRequested, this, &CSmartObjectsEditorDialog::OnContextMenu);
    connect(m_View->header(), &QWidget::customContextMenuRequested, this, &CSmartObjectsEditorDialog::OnReportColumnRClick);
    connect(m_View->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSmartObjectsEditorDialog::OnReportSelChanged);

    connect(m_Tree->selectionModel(), &QItemSelectionModel::currentChanged, this, &CSmartObjectsEditorDialog::OnTreeSelChanged);

    connect(m_Description, &QTextEdit::textChanged, this, &CSmartObjectsEditorDialog::OnDescriptionEdit);

    connect(m_View->model(), &QAbstractItemModel::dataChanged, this, &CSmartObjectsEditorDialog::OnTreeViewDataChanged);
}

//////////////////////////////////////////////////////////////////////////
CSmartObjectsEditorDialog::~CSmartObjectsEditorDialog()
{
    OnDestroy();

    if (CSOLibrary::m_bSaveNeeded && SaveSOLibrary(false))
    {
        CSOLibrary::m_bSaveNeeded = false;
        CSOLibrary::m_bLoadNeeded = false;
    }

    --CSOLibrary::m_iNumEditors;
    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CSmartObjectsEditorDialog::OnObjectEvent));
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
SmartObjectConditions::iterator CSmartObjectsEditorDialog::FindRuleByPtr(const SmartObjectCondition* pRule)
{
    SmartObjectConditions::iterator it, itEnd = CSOLibrary::m_Conditions.end();
    for (it = CSOLibrary::m_Conditions.begin(); it != itEnd; ++it)
    {
        if (&*it == pRule)
        {
            return it;
        }
    }

    // if this ever happen it means that the rule is already deleted!
    assert(!"Rule not found in CSmartObjectsEditorDialog::FindRuleByPtr!");
    return itEnd;
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnInitDialog()
{
    m_bFilterCanceled = false;

    // root variable block
    //  block = new CVarBlock;
    //  block->AddRef();

    // init panes
    CreatePanes();

    // init client pane
    m_model = new SmartObjectsEditorModel(this);
    SmartObjectsEditorTreeModel* treeModel = new SmartObjectsEditorTreeModel(this);
    m_pathModel = new SmartObjectsPathFilterModel(this);
    m_pathModel->setSourceModel(m_model);
    m_pathModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_View->setRootIsDecorated(false);
    m_View->setModel(m_pathModel);
    m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnDescription, true);
    m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnOrder, true);
    m_View->header()->resizeSection(SmartObjectsEditorModel::ColumnName, 120);
    m_View->header()->resizeSection(SmartObjectsEditorModel::ColumnDescription, 320);
    m_View->header()->resizeSection(SmartObjectsEditorModel::ColumnUserClass, 120);
    m_View->header()->resizeSection(SmartObjectsEditorModel::ColumnUserState, 120);
    m_View->header()->resizeSection(SmartObjectsEditorModel::ColumnObjectClass, 120);
    m_View->header()->resizeSection(SmartObjectsEditorModel::ColumnObjectState, 120);
    m_View->header()->resizeSection(SmartObjectsEditorModel::ColumnAction, 120);

    auto treeFilterModel = new LeafRemovingFilterModel(this);
    treeModel->setSourceModel(m_model);
    treeFilterModel->setSourceModel(treeModel);
    m_Tree->setModel(treeFilterModel);
    m_Tree->setHeaderHidden(true);

    m_View->setSortingEnabled(true);
    m_pathModel->setDynamicSortFilter(true);
    m_pathModel->sort(SmartObjectsEditorModel::ColumnOrder);

    m_View->setDragEnabled(true);
    m_Tree->setDragDropMode(QAbstractItemView::DragOnly);
    m_Tree->viewport()->setAcceptDrops(true);
    m_Tree->setDragDropMode(QAbstractItemView::DropOnly);

    m_View->setSelectionMode(QAbstractItemView::ExtendedSelection);

    const QSettings settings("Amazon", "Lumberyard");
    restoreState(settings.value(QStringLiteral("SmartObjectsLayout")).toByteArray());

    //  // Load templates
    //  ReloadTemplates();
}


//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::CreatePanes()
{
    /////////////////////////////////////////////////////////////////////////
    // Docking Pane for Properties
    QDockWidget* pDockPane = new AzQtComponents::StyledDockWidget;
    addDockWidget(Qt::RightDockWidgetArea, pDockPane);
    pDockPane->setWindowTitle(tr("Properties"));
    pDockPane->setFeatures(QDockWidget::NoDockWidgetFeatures);

    //////////////////////////////////////////////////////////////////////////
    // Create properties control.
    //////////////////////////////////////////////////////////////////////////
    pDockPane->setWidget(m_Properties = new ReflectedPropertyControl(this));
    m_Properties->Setup();

    //m_Properties->m_props.SetFlags( CPropertyCtrl::F_VS_DOT_NET_STYLE );
    m_Properties->ExpandAll();

    m_vars = gSmartObjectsUI.CreateVars();
    m_Properties->AddVarBlock(m_vars);
    m_Properties->ExpandAll();
    m_Properties->setEnabled(false);

    m_Properties->SetUpdateCallback(functor(*this, &CSmartObjectsEditorDialog::OnUpdateProperties));
    m_Properties->EnableUpdateCallback(true);
    pDockPane->resize(120, m_Properties->GetContentHeight());


    /////////////////////////////////////////////////////////////////////////
    // Docking Pane for Tree Control
    pDockPane = new AzQtComponents::StyledDockWidget;
    addDockWidget(Qt::LeftDockWidgetArea, pDockPane);
    pDockPane->setWindowTitle(tr("Rules"));
    pDockPane->setFeatures(QDockWidget::NoDockWidgetFeatures);

    /////////////////////////////////////////////////////////////////////////
    // Create Tree Control
    pDockPane->setWidget(m_Tree);

    /////////////////////////////////////////////////////////////////////////
    // Docking Pane for TaskPanel
    QDockWidget* pTaskPane = new AzQtComponents::StyledDockWidget;
    addDockWidget(Qt::LeftDockWidgetArea, pTaskPane, Qt::Vertical);
    pTaskPane->setWindowTitle(tr("Tasks"));
    pTaskPane->setFeatures(QDockWidget::NoDockWidgetFeatures);


    /////////////////////////////////////////////////////////////////////////
    // Create empty Task Panel
    pTaskPane->setWidget(m_taskPanel = new QWidget);
    m_taskPanel->setLayout(new QVBoxLayout);

    ClickableLabel* pItem =  NULL;
    QCollapsibleGroupBox* pGroup = NULL;
    int groupId = 0;

    m_taskPanel->layout()->addWidget(pGroup = new QCollapsibleGroupBox(this));
    pGroup->setLayout(new QVBoxLayout);
    pGroup->setTitle(tr("Rules"));

    pGroup->layout()->addWidget(pItem = new ClickableLabel(tr("New..."), this));
    pItem->setToolTip(tr("Adds a new entry"));
    connect(pItem, &ClickableLabel::linkActivated, this, &CSmartObjectsEditorDialog::OnAddEntry);

    pGroup->layout()->addWidget(m_pDuplicate = new ClickableLabel(tr("Duplicate...")));
    m_pDuplicate->setToolTip(tr("Duplicates selected entry"));
    connect(m_pDuplicate, &ClickableLabel::linkActivated, this, &CSmartObjectsEditorDialog::OnDuplicateEntry);

    pGroup->layout()->addWidget(m_pDelete = new ClickableLabel(tr("Delete")));
    m_pDelete->setToolTip(tr("Removes selected entries"));
    connect(m_pDelete, &ClickableLabel::linkActivated, this, &CSmartObjectsEditorDialog::OnRemoveEntry);

    m_taskPanel->layout()->addWidget(pGroup = new QCollapsibleGroupBox(this));
    pGroup->setLayout(new QVBoxLayout);
    pGroup->setTitle(tr("Helpers"));

    pGroup->layout()->addWidget(m_pItemHelpersEdit = new ClickableLabel(tr("Edit...")));
    m_pItemHelpersEdit->setToolTip(tr("Displays Smart Object helpers for editing"));
    connect(m_pItemHelpersEdit, &ClickableLabel::linkActivated, this, &CSmartObjectsEditorDialog::OnHelpersEdit);

    pGroup->layout()->addWidget(m_pItemHelpersNew = new ClickableLabel(tr("New...")));
    m_pItemHelpersNew->setToolTip(tr("Adds a new Smart Object helper"));
    connect(m_pItemHelpersNew, &ClickableLabel::linkActivated, this, &CSmartObjectsEditorDialog::OnHelpersNew);

    pGroup->layout()->addWidget(m_pItemHelpersDelete = new ClickableLabel(tr("Delete...")));
    m_pItemHelpersDelete->setToolTip(tr("Removes selected Smart Object helper(s)"));
    connect(m_pItemHelpersDelete, &ClickableLabel::linkActivated, this, &CSmartObjectsEditorDialog::OnHelpersDelete);

    pGroup->layout()->addWidget(m_pItemHelpersDone = new ClickableLabel(tr("Done")));
    m_pItemHelpersDone->setToolTip(tr("Ends helper edit mode"));
    connect(m_pItemHelpersDone, &ClickableLabel::linkActivated, this, &CSmartObjectsEditorDialog::OnHelpersDone);

    m_taskPanel->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding));

    /////////////////////////////////////////////////////////////////////////
    // Docking Pane for description
    pDockPane = new AzQtComponents::StyledDockWidget;
    addDockWidget(Qt::BottomDockWidgetArea, pDockPane);
    pDockPane->setWindowTitle(tr("Description"));
    pDockPane->setFeatures(QDockWidget::NoDockWidgetFeatures);

    /////////////////////////////////////////////////////////////////////////
    // Create Edit Control
    m_Description = new QTextEdit;
    pDockPane->setWidget(m_Description);
}

//////////////////////////////////////////////////////////////////////////
QString CSmartObjectsEditorDialog::GetFolderPath(const QModelIndex& index) const
{
    return index.data(Qt::UserRole).toString();
}

//////////////////////////////////////////////////////////////////////////
QString CSmartObjectsEditorDialog::GetCurrentFolderPath() const
{
    return GetFolderPath(m_Tree->currentIndex());
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSmartObjectsEditorDialog::SetCurrentFolder(const QString& folder)
{
    const QModelIndexList matches = m_Tree->model()->match(m_Tree->model()->index(0, 0), Qt::UserRole, folder, 1, Qt::MatchFixedString | Qt::MatchRecursive);
    if (matches.isEmpty())
    {
        return QModelIndex();
    }
    m_Tree->setCurrentIndex(matches.first());
    return matches.first();
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::ReloadEntries(bool bFromFile)
{
    m_model->reloadItems();
}

//////////////////////////////////////////////////////////////////////////
bool CSmartObjectsEditorDialog::ChangeTemplate(SmartObjectCondition* pRule, const CSOParamBase* pParam) const
{
    QString msg;
    SetTemplateDefaults(*pRule, pParam, &msg);
    if (msg.isEmpty())
    {
        SetTemplateDefaults(*pRule, pParam);
        return true;
    }

    msg = tr("WARNING!\n\nThe change of the template will implicitly alter the following field(s):\n%1\nDo you really want to change the template?").arg(msg);

    if (QMessageBox::warning(const_cast<CSmartObjectsEditorDialog*>(this), QString(), msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    {
        return false;
    }

    SetTemplateDefaults(*pRule, pParam);
    return true;
}

void CSmartObjectsEditorDialog::OnTreeViewDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (m_View->currentIndex().row() >= topLeft.row() && m_View->currentIndex().row() <= bottomRight.row())
    {
        m_Properties->EnableUpdateCallback(false);
        auto condition = m_View->currentIndex().data(Qt::UserRole).value<SmartObjectCondition*>();
        gSmartObjectsUI.sName = condition->sName.c_str();
        gSmartObjectsUI.sDescription = condition->sDescription.c_str();
        gSmartObjectsUI.bEnabled = condition->bEnabled;
        if (m_Description->toPlainText() != condition->sDescription.c_str())
        {
            m_Description->setPlainText(condition->sDescription.c_str());
        }
        m_Properties->EnableUpdateCallback(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnUpdateProperties(IVariable* pVar)
{
    auto currentCondition = m_View->currentIndex().data(Qt::UserRole).value<SmartObjectCondition*>();

    if (currentCondition)
    {
        if (!CSOLibrary::StartEditing())
        {
            return;
        }

        if (pVar->GetDataType() == IVariable::DT_SOTEMPLATE)
        {
            // Template changed -> special handling here...
            QString name;
            pVar->Get(name);
            const MapTemplates& mapTemplates = GetIEditor()->GetAI()->GetMapTemplates();
            MapTemplates::const_iterator it, itEnd = mapTemplates.end();
            for (it = mapTemplates.begin(); it != itEnd; ++it)
            {
                if (it->second->name == name)
                {
                    break;
                }
            }

            // is it a valid template name?
            if (it == itEnd)
            {
                QMessageBox::critical(this, QString(), tr("Specified template is not defined!"));
                it = mapTemplates.find(currentCondition->iTemplateId);
                if (it != itEnd)
                {
                    m_Properties->EnableUpdateCallback(false);
                    pVar->Set(it->second->name);
                    m_Properties->EnableUpdateCallback(true);
                }
                return;
            }

            // does it differs from current?
            if (currentCondition->iTemplateId != it->first)
            {
                // try to set default template values
                if (ChangeTemplate(currentCondition, it->second->params))
                {
                    currentCondition->iTemplateId = it->first;

                    // Update variables.
                    //m_Properties.EnableUpdateCallback( false );
                    //gSmartObjectsUI.SetConditionToUI( *m_pEditedEntry->m_pCondition, &m_Properties );
                    //m_Properties.EnableUpdateCallback( true );
                }
            }
            return;
        }

        // Update variables.
        //m_Properties.EnableUpdateCallback( false );
        if (gSmartObjectsUI.GetConditionFromUI(*currentCondition))
        {
            // folder property was edited
            m_bSinkNeeded = true;

            // make sure the folder exists in the tree view
            findChild<SmartObjectsEditorTreeModel*>()->setSourceModel(m_model);
            SetCurrentFolder(currentCondition->sFolder.c_str());
            const auto matches = m_model->match(m_model->index(0, 0), Qt::UserRole, QVariant::fromValue(currentCondition), 1, Qt::MatchRecursive | Qt::MatchWrap);
            m_model->setData(matches.first(), QVariant::fromValue(currentCondition), Qt::UserRole);
            /*
            HTREEITEM root = m_Tree.GetRootItem();
            CString folder = m_pEditedEntry->m_pCondition->sFolder.c_str();
            CString realFolder;
            HTREEITEM item = root;
            int token = 0;
            CString name = folder.Tokenize( _T("/\\"), token );
            while ( !name.IsEmpty() )
            {
                m_Tree.Expand( item, TVE_EXPAND );

                HTREEITEM child = m_Tree.GetChildItem( item );
                while ( child && m_Tree.GetItemText( child ).CompareNoCase( name ) != 0 )
                    child = m_Tree.GetNextSiblingItem( child );
                if ( child )
                {
                    name = m_Tree.GetItemText( child );
                    item = child;
                }
                else
                    item = m_Tree.InsertItem( name, 2, 2, item, TVI_SORT );
                realFolder += _T('/');
                realFolder += name;
                name = folder.Tokenize( _T("/\\"), token );
            }
            m_pEditedEntry->m_pCondition->sFolder = realFolder;
            m_Tree.SelectItem( item );
            */
        }
        //m_Properties.EnableUpdateCallback( true );

        if (pVar->GetDataType() == IVariable::DT_SOCLASS ||
            //          pVar->GetDataType() == IVariable::DT_SOSTATE ||
            pVar->GetDataType() == IVariable::DT_SOSTATES ||
            pVar->GetDataType() == IVariable::DT_SOSTATEPATTERN ||
            pVar->GetDataType() == IVariable::DT_SOACTION ||
            pVar == &gSmartObjectsUI.sName ||
            pVar == &gSmartObjectsUI.sDescription ||
            pVar == &gSmartObjectsUI.bEnabled)
        //          pVar->GetDataType() == IVariable::DT_OBJECT )
        {
            m_View->model()->setData(m_View->currentIndex(), QVariant::fromValue(currentCondition), Qt::UserRole);
            m_Description->setPlainText(gSmartObjectsUI.sDescription);
        }

        struct Local
        {
            static void CopyClassToHelper(const QString& sClass, CVariable< QString >& var)
            {
                QString sHelper;
                if (!sClass.isEmpty())
                {
                    var.Get(sHelper);
                    int f = sHelper.lastIndexOf(':');
                    if (f >= 0)
                    {
                        sHelper.remove(0, f + 1);
                    }
                    sHelper = sClass + ':' + sHelper;
                }
                var.Set(sHelper);
            }
        };

        // if class was changed update helper value
        //m_Properties.EnableUpdateCallback( false );
        if (pVar->GetDataType() == IVariable::DT_SOCLASS)
        {
            if (pVar == &gSmartObjectsUI.userClass)
            {
                QString sHelper, sClass;
                pVar->Get(sClass);
                Local::CopyClassToHelper(sClass, gSmartObjectsUI.userHelper);
            }
            else if (pVar == &gSmartObjectsUI.objectClass)
            {
                QString sHelper, sClass;
                pVar->Get(sClass);
                Local::CopyClassToHelper(sClass, gSmartObjectsUI.objectHelper);
                Local::CopyClassToHelper(sClass, gSmartObjectsUI.entranceHelper);
                Local::CopyClassToHelper(sClass, gSmartObjectsUI.exitHelper);
                Local::CopyClassToHelper(sClass, gSmartObjectsUI.animationHelper);
                Local::CopyClassToHelper(sClass, gSmartObjectsUI.approachHelper);
            }
        }
        //m_Properties.EnableUpdateCallback( true );

        if (pVar == &gSmartObjectsUI.bNavigationRule)
        {
            UpdatePropertyTables();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// Called by the editor to notify the listener about the specified event.
void CSmartObjectsEditorDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)      // Sent every frame while editor is idle.
    {
        if (m_bSinkNeeded)
        {
            SinkSelection();
        }
        return;
    }

    switch (event)
    {
    case eNotify_OnBeginGameMode:           // Sent when editor goes to game mode.
    case eNotify_OnBeginSimulationMode:     // Sent when simulation mode is started.
    case eNotify_OnBeginSceneSave:          // Sent when document is about to be saved.
    case eNotify_OnQuit:                    // Sent before editor quits.
    case eNotify_OnBeginNewScene:           // Sent when the document is begin to be cleared.
    case eNotify_OnBeginSceneOpen:          // Sent when document is about to be opened.
    case eNotify_OnCloseScene:              // Send when the document is about to close.
        OnHelpersDone();
        if (CSOLibrary::m_bSaveNeeded && SaveSOLibrary())
        {
            CSOLibrary::m_bSaveNeeded = false;
            CSOLibrary::m_bLoadNeeded = false;
        }
        break;

    case eNotify_OnInit:                    // Sent after editor fully initialized.
    case eNotify_OnEndSceneOpen:            // Sent after document have been opened.
        //      ReloadEntries();
        break;

    case eNotify_OnEndSceneSave:            // Sent after document have been saved.
    case eNotify_OnEndNewScene:             // Sent after the document have been cleared.
    case eNotify_OnMissionChange:           // Send when the current mission changes.
        break;

    //////////////////////////////////////////////////////////////////////////
    // Editing events.
    //////////////////////////////////////////////////////////////////////////
    case eNotify_OnEditModeChange:          // Sent when editing mode change (move,rotate,scale,....)
    case eNotify_OnEditToolChange:          // Sent when edit tool is changed (ObjectMode,TerrainModify,....)
        break;

    // Game related events.
    case eNotify_OnEndGameMode:             // Send when editor goes out of game mode.
        break;

    // UI events.
    case eNotify_OnUpdateViewports:         // Sent when editor needs to update data in the viewports.
    case eNotify_OnReloadTrackView:         // Sent when editor needs to update the track view.
    case eNotify_OnInvalidateControls:      // Sent when editor needs to update some of the data that can be cached by controls like combo boxes.
        break;

    // Object events.
    case eNotify_OnSelectionChange:         // Sent when object selection change.
        // Unfortunately I have never received this notification!!!
        // SinkSelection();
        break;
    case eNotify_OnPlaySequence:            // Sent when editor start playing animation sequence.
    case eNotify_OnStopSequence:            // Sent when editor stop playing animation sequence.
        break;
    }
}

void CSOLibrary::String2Classes(const string& sClass, SetStrings& classes)
{
    Load();

    int start = 0, end, length = sClass.length();
    string single;

    while (start < length)
    {
        start = sClass.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz", start);
        if (start < 0)
        {
            break;
        }

        end = sClass.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz", start);
        if (end == -1)
        {
            end = length;
        }

        single = sClass.substr(start, end - start);
        classes.insert(single);

        start = end + 1;
    }
    ;
}

void CSmartObjectsEditorDialog::ParseClassesFromProperties(CBaseObject* pObject, SetStrings& classes)
{
    if ((pObject->GetType() & OBJTYPE_ENTITY) ||
        (pObject->GetType() & OBJTYPE_AIPOINT) && (!QString::compare(pObject->GetTypeName(), "AIAnchor") || !QString::compare(pObject->GetTypeName(), "SmartObject")))
    {
        CEntityObject* pEntity = static_cast< CEntityObject* >(pObject);
        CEntityPrototype* pPrototype = pEntity->GetPrototype();
        CVarBlock* pVars = pPrototype ? pPrototype->GetProperties() : pEntity->GetProperties();
        if (pVars)
        {
            IVariablePtr pVariable = pVars->FindVariable("soclasses_SmartObjectClass", false);
            if (pVariable)
            {
                QString value;
                pVariable->Get(value);
                if (!value.isEmpty())
                {
                    CSOLibrary::String2Classes(value.toUtf8().data(), classes);
                }
            }
        }

        pVars = pEntity->GetProperties2();
        if (pVars)
        {
            IVariablePtr pVariable = pVars->FindVariable("soclasses_SmartObjectClass", false);
            if (pVariable)
            {
                QString value;
                pVariable->Get(value);
                if (!value.isEmpty())
                {
                    CSOLibrary::String2Classes(value.toUtf8().data(), classes);
                }
            }
        }
    }
}

void CSmartObjectsEditorDialog::SinkSelection()
{
    m_bSinkNeeded = false;

    // get selected folder
    QString folder = GetCurrentFolderPath();

    m_sNewObjectClass.clear();

    m_sFilterClasses.clear();
    m_sFirstFilterClass.clear();
    SetStrings filterClasses;

    CSelectionGroup* pSelection = GetIEditor()->GetSelection();
    int selCount = pSelection->GetCount();

    m_pItemHelpersDelete->setEnabled(!m_sEditedClass.isEmpty() && selCount == 1 && qobject_cast<CSmartObjectHelperObject*>(pSelection->GetObject(0)));


    // Update helpers
    m_bIgnoreNotifications = true;
    if (m_sEditedClass.isEmpty())
    {
        MapHelperObjects::iterator it, itEnd = m_mapHelperObjects.end();
        for (it = m_mapHelperObjects.begin(); it != itEnd; ++it)
        {
            CSmartObjectHelperObject* pHelperObject = it->second;
            pHelperObject->RemoveEventListener(functor(*this, &CSmartObjectsEditorDialog::OnObjectEvent));
            pHelperObject->DetachThis();
            pHelperObject->Release();
            GetIEditor()->GetObjectManager()->DeleteObject(pHelperObject);
        }
        m_mapHelperObjects.clear();
    }
    else
    {
        typedef std::set< CEntityObject* > SetEntities;
        SetEntities setEntities;

        int i = selCount;

        // build a list of selected entities which smart object class matches 'm_sEditedClass'
        while (i--)
        {
            CBaseObject* pSelected = pSelection->GetObject(i);
            if (pSelected->GetType() & OBJTYPE_OTHER)
            {
                if (qobject_cast<CSmartObjectHelperObject*>(pSelected))
                {
                    pSelected = pSelected->GetParent();
                }
            }

            SetStrings classes;
            ParseClassesFromProperties(pSelected, classes);
            if (classes.find(m_sEditedClass.toUtf8().data()) != classes.end())
            {
                setEntities.insert(static_cast< CEntityObject* >(pSelected));
            }
        }

        // remove not needed helpers
        MapHelperObjects::iterator it, next, itEnd = m_mapHelperObjects.end();
        for (it = m_mapHelperObjects.begin(); it != itEnd; ++it)
        {
            CEntityObject* const pEntity = it->first;
            CSmartObjectHelperObject* pHelperObject = it->second;

            if (setEntities.find(pEntity) == setEntities.end())
            {
                // remove all helpers for this entity
                it->second = NULL;
                pHelperObject->RemoveEventListener(functor(*this, &CSmartObjectsEditorDialog::OnObjectEvent));
                pHelperObject->DetachThis();
                pHelperObject->Release();
                GetIEditor()->GetObjectManager()->DeleteObject(pHelperObject);
            }
            else
            {
                // remove this helper if it isn't in the list of helpers for this class
                if (CSOLibrary::FindHelper(m_sEditedClass, pHelperObject->GetName()) == CSOLibrary::m_vHelpers.end())
                {
                    it->second = NULL;
                    pHelperObject->RemoveEventListener(functor(*this, &CSmartObjectsEditorDialog::OnObjectEvent));
                    pHelperObject->DetachThis();
                    pHelperObject->Release();
                    GetIEditor()->GetObjectManager()->DeleteObject(pHelperObject);
                }
            }
        }

        // remove NULL helpers
        it = m_mapHelperObjects.begin();
        while (it != itEnd)
        {
            next = it;
            next++;
            if (!it->second)
            {
                m_mapHelperObjects.erase(it);
            }
            it = next;
        }

        // for each helper of edited class...
        CSOLibrary::VectorHelperData::iterator itHelpers, itHelpersEnd = CSOLibrary::HelpersUpperBound(m_sEditedClass);
        for (itHelpers = CSOLibrary::HelpersLowerBound(m_sEditedClass); itHelpers != itHelpersEnd; ++itHelpers)
        {
            const CSOLibrary::CHelperData& helper = *itHelpers;

            // ... and for each entity
            SetEntities::iterator it, itEnd = setEntities.end();
            for (it = setEntities.begin(); it != itEnd; ++it)
            {
                CEntityObject* pEntity = *it;

                // create/update child helper object
                CSmartObjectHelperObject* pHelperObject = NULL;
                int i = pEntity->GetChildCount();
                while (i--)
                {
                    CBaseObject* pChild = pEntity->GetChild(i);
                    if ((pChild->GetType() & OBJTYPE_OTHER) != 0 && qobject_cast<CSmartObjectHelperObject*>(pChild))
                    {
                        pHelperObject = static_cast< CSmartObjectHelperObject* >(pChild);
                        if (helper.name == pHelperObject->GetName())
                        {
                            break;
                        }
                        else
                        {
                            pHelperObject = NULL;
                        }
                    }
                }

                if (!pHelperObject)
                {
                    pHelperObject = static_cast< CSmartObjectHelperObject* >(GetIEditor()->GetObjectManager()->NewObject("SmartObjectHelper"));
                    if (pHelperObject)
                    {
                        pHelperObject->AddRef();
                        pEntity->AttachChild(pHelperObject);
                        m_mapHelperObjects.insert(std::make_pair(pEntity, pHelperObject));
                        pHelperObject->AddEventListener(functor(*this, &CSmartObjectsEditorDialog::OnObjectEvent));
                    }
                }

                if (pHelperObject)
                {
                    pHelperObject->SetSmartObjectEntity(pEntity);
                    GetIEditor()->GetObjectManager()->ChangeObjectName(pHelperObject, helper.name);
                    //Vec3 pos = pHelper->qt.t
                    //Vec3 pos = pEntity->GetWorldTM().GetInvertedFast().TransformPoint( GetWorldPos() );
                    //pVar->Set( pos );
                    pHelperObject->SetPos(helper.qt.t, eObjectUpdateFlags_PositionChanged);
                    pHelperObject->SetRotation(helper.qt.q, eObjectUpdateFlags_RotationChanged);
                }
            }
        }
    }
    m_bIgnoreNotifications = false;


    // Get the selection once again since it might be changed
    pSelection = GetIEditor()->GetSelection();
    selCount = pSelection->GetCount();

    m_pItemHelpersDelete->setEnabled(!m_sEditedClass.isEmpty() && selCount == 1 && qobject_cast<CSmartObjectHelperObject*>(pSelection->GetObject(0)));


    // make a list of all selected SmartObjectClass properties
    int j = selCount;
    while (j--)
    {
        CBaseObject* pObject = pSelection->GetObject(j);

        SetStrings classes;
        ParseClassesFromProperties(pObject, classes);
        if (classes.empty())
        {
            filterClasses.clear();
            break;
        }
        if (!m_bFilterCanceled)
        {
            filterClasses.insert(classes.begin(), classes.end());
        }
    }

    // Update report table
    m_pathModel->setFilterClasses(filterClasses);

    SetStrings::iterator it, itEnd = filterClasses.end();
    for (it = filterClasses.begin(); it != itEnd; ++it)
    {
        if (m_sFilterClasses.isEmpty())
        {
            m_sFilterClasses += *it;
            m_sFirstFilterClass = *it;
        }
        else
        {
            m_sFilterClasses += ',' + *it;
        }
    }

    m_topLabel->setText(m_sFilterClasses);
    m_topLabel->setVisible(!m_sFilterClasses.isEmpty());
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::DeleteHelper(const QString& className, const QString& helperName)
{
    m_bSinkNeeded = true;
    CSOLibrary::m_bSaveNeeded = true;

    CSOLibrary::VectorHelperData::iterator it = CSOLibrary::FindHelper(className, helperName);
    if (it != CSOLibrary::m_vHelpers.end())
    {
        CSOLibrary::m_vHelpers.erase(it);
    }
}

//////////////////////////////////////////////////////////////////////////
CSOLibrary::VectorHelperData::iterator CSOLibrary::FindHelper(const QString& className, const QString& helperName)
{
    Load();

    VectorHelperData::iterator it, itEnd = m_vHelpers.end();
    for (it = m_vHelpers.begin(); it != itEnd; ++it)
    {
        if (it->className == className && it->name == helperName)
        {
            return it;
        }
    }
    return itEnd;
}

//////////////////////////////////////////////////////////////////////////
CSOLibrary::VectorHelperData::iterator CSOLibrary::HelpersLowerBound(const QString& className)
{
    Load();

    VectorHelperData::iterator it, itEnd = m_vHelpers.end();
    for (it = m_vHelpers.begin(); it != itEnd; ++it)
    {
        if (it->className >= className)
        {
            return it;
        }
    }
    return itEnd;
}

//////////////////////////////////////////////////////////////////////////
CSOLibrary::VectorHelperData::iterator CSOLibrary::HelpersUpperBound(const QString& className)
{
    Load();

    VectorHelperData::iterator it, itEnd = m_vHelpers.end();
    for (it = m_vHelpers.begin(); it != itEnd; ++it)
    {
        if (it->className > className)
        {
            return it;
        }
    }
    return itEnd;
}

//////////////////////////////////////////////////////////////////////////
CSOLibrary::VectorEventData::iterator CSOLibrary::FindEvent(const char* name)
{
    Load();

    VectorEventData::iterator it, itEnd = m_vEvents.end();
    for (it = m_vEvents.begin(); it != itEnd; ++it)
    {
        if (it->name == name)
        {
            return it;
        }
    }
    return itEnd;
}

//////////////////////////////////////////////////////////////////////////
void CSOLibrary::AddEvent(const char* name, const char* description)
{
    Load();

    m_bSaveNeeded = true;
    VectorEventData::iterator it, itEnd = m_vEvents.end();
    for (it = m_vEvents.begin(); it != itEnd; ++it)
    {
        if (it->name > name)
        {
            break;
        }
    }

    CEventData event;
    event.name = name;
    event.description = description;
    m_vEvents.insert(it, event);
}

//////////////////////////////////////////////////////////////////////////
CSOLibrary::VectorStateData::iterator CSOLibrary::FindState(const char* name)
{
    Load();

    CStateData stateData;
    stateData.name = name;

    VectorStateData::iterator it = std::lower_bound(m_vStates.begin(), m_vStates.end(), stateData, less_name< CStateData >());
    if (it != m_vStates.end() && it->name == name)
    {
        return it;
    }
    else
    {
        return m_vStates.end();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSOLibrary::AddState(const char* name, const char* description, const char* location)
{
    Load();

    m_bSaveNeeded = true;
    CStateData stateData = { name, description, location };
    VectorStateData::iterator it = std::lower_bound(m_vStates.begin(), m_vStates.end(), stateData, less_name< CStateData >());
    if (it != m_vStates.end() && it->name == name)
    {
        it->description = description;
        it->location = location;
    }
    else
    {
        m_vStates.insert(it, stateData);
    }
}

//////////////////////////////////////////////////////////////////////////
CSOLibrary::VectorClassData::iterator CSOLibrary::FindClass(const char* name)
{
    Load();

    CClassData classData;
    classData.name = name;

    VectorClassData::iterator it = std::lower_bound(m_vClasses.begin(), m_vClasses.end(), classData, less_name< CClassData >());
    if (it != m_vClasses.end() && it->name == name)
    {
        return it;
    }
    else
    {
        return m_vClasses.end();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSOLibrary::AddClass(const char* name, const char* description, const char* location, const char* templateName)
{
    Load();

    m_bSaveNeeded = true;
    CClassTemplateData const* pTemplate = FindClassTemplate(templateName);
    CClassData classData = { name, description, location, templateName, pTemplate };
    VectorClassData::iterator it = std::lower_bound(m_vClasses.begin(), m_vClasses.end(), classData, less_name< CClassData >());
    if (it != m_vClasses.end() && it->name == name)
    {
        it->description = description;
        it->location = location;
        it->templateName = templateName;
        it->pClassTemplateData = pTemplate;
    }
    else
    {
        m_vClasses.insert(it, classData);
    }
}

//////////////////////////////////////////////////////////////////////////
CSOLibrary::CClassTemplateData const* CSOLibrary::FindClassTemplate(const char* name)
{
    if (!name || !*name)
    {
        return NULL;
    }

    Load();

    CClassTemplateData classTemplateData;
    classTemplateData.name = name;

    VectorClassTemplateData::iterator it = std::lower_bound(m_vClassTemplates.begin(), m_vClassTemplates.end(), classTemplateData, less_name_no_case< CClassTemplateData >());
    if (it != m_vClassTemplates.end() && _stricmp(it->name, name) == 0)
    {
        it->name = name;
        return &*it;
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnObjectEvent(CBaseObject* object, int event)
{
    if (!m_bIgnoreNotifications)
    {
        switch (event)
        {
        case CBaseObject::ON_DELETE:     // Sent after object was deleted from object manager.
            if (!m_sEditedClass.isEmpty() && qobject_cast<CSmartObjectHelperObject*>(object))
            {
                CSmartObjectHelperObject* pHelperObject = (CSmartObjectHelperObject*) object;
                DeleteHelper(m_sEditedClass, object->GetName());
            }
            m_bSinkNeeded = true;
            break;
        case CBaseObject::ON_SELECT:     // Sent when objects becomes selected.
        case CBaseObject::ON_UNSELECT:   // Sent when objects unselected.
            m_bSinkNeeded = true;
            m_bFilterCanceled = false;
            break;
        case CBaseObject::ON_TRANSFORM:  // Sent when object transformed.
            if (static_cast<CSmartObjectHelperObject*>(object))
            {
                CSmartObjectHelperObject* pHelperObject = (CSmartObjectHelperObject*) object;
                CSOLibrary::VectorHelperData::iterator it = CSOLibrary::FindHelper(m_sEditedClass, pHelperObject->GetName());
                if (it != CSOLibrary::m_vHelpers.end())
                {
                    it->qt.t = object->GetPos();
                    it->qt.q = object->GetRotation();
                    m_bSinkNeeded = true;
                    CSOLibrary::m_bSaveNeeded = true;
                }
            }
            break;
        case CBaseObject::ON_VISIBILITY: // Sent when object visibility changes.
        case CBaseObject::ON_RENAME:     // Sent when object changes name.
        case CBaseObject::ON_ADD:        // Sent after object was added to object manager.
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::ModifyRuleOrder(int from, int to)
{
    SmartObjectConditions::iterator it = CSOLibrary::m_Conditions.begin(), itEnd = CSOLibrary::m_Conditions.end();
    if (from < to)
    {
        for (; it != itEnd; ++it)
        {
            int order = it->iOrder;
            if (it->iOrder == from)
            {
                it->iOrder = to;
            }
            else if (from < it->iOrder && it->iOrder <= to)
            {
                it->iOrder--;
            }
        }
    }
    else
    {
        for (; it != itEnd; ++it)
        {
            if (it->iOrder == from)
            {
                it->iOrder = to;
            }
            else if (to <= it->iOrder && it->iOrder < from)
            {
                it->iOrder++;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnAddEntry()
{
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnEditEntry()
{
    //  ReloadEntries();
    CSOLibrary::m_bSaveNeeded = true;
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnRemoveEntry()
{
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnDuplicateEntry()
{
    auto cond = m_View->currentIndex().data(Qt::UserRole).value<SmartObjectCondition*>();
    if (!cond)
    {
        return;
    }

    SmartObjectCondition condition = *cond;

    int order = condition.iOrder + 1;
    condition.iOrder = INT_MAX;

    CItemDescriptionDlg dlg(this, true, true);
    dlg.setWindowTitle(tr("Duplicate Smart Object Rule"));
    dlg.setItemCaption(tr("Rule name:"));
    dlg.setItem(tr("Copy of %1").arg(condition.sName.c_str()));
    dlg.setDescription(condition.sDescription.c_str());
    if (!dlg.exec())
    {
        return;
    }

    condition.sName = dlg.item().toUtf8().data();
    condition.sDescription = dlg.description().toUtf8().data();

    m_model->addCondition(condition);
    ModifyRuleOrder(INT_MAX, order);
    condition.iOrder = order; // to select the right row

    // now select duplicated row
    const QModelIndexList matches = m_View->model()->match(m_View->model()->index(0, 0), Qt::DisplayRole, QString(condition.sName.c_str()), 1, Qt::MatchFixedString);
    if (!matches.isEmpty())
    {
        m_View->setCurrentIndex(matches.first());
    }

    CSOLibrary::m_bSaveNeeded = true;
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnHelpersEdit()
{
    if (!CheckOutLibrary())
    {
        return;
    }

    if (m_sEditedClass.isEmpty())
    {
        QString sClass = m_sFirstFilterClass;
        auto condition = m_View->currentIndex().data(Qt::UserRole).value<SmartObjectCondition*>();
        if (condition && sClass.isEmpty())
        {
            if (!condition->sObjectClass.empty())
            {
                sClass = condition->sObjectClass;
            }
            else
            {
                sClass = condition->sUserClass;
            }
        }

        CSmartObjectClassDialog dlg(this);
        dlg.SetSOClass(sClass);
        dlg.EnableOK();
        if (dlg.exec() == QDialog::Accepted)
        {
            sClass = dlg.GetSOClass();
            if (!sClass.isEmpty())
            {
                m_sEditedClass = sClass;

                m_pItemHelpersNew->setEnabled(true);
                m_pItemHelpersDone->setEnabled(true);

                m_pItemHelpersEdit->setText(tr("Editing %1").arg(sClass));
                QFont f;
                f.setBold(true);
                m_pItemHelpersEdit->setFont(f);

                m_bSinkNeeded = true;
            }
        }
    }
    else if (!m_mapHelperObjects.empty())
    {
        QString sSelectedHelper;
        CBaseObject* pSelectedObject = GetIEditor()->GetSelectedObject();
        if (!pSelectedObject)
        {
            return;
        }

        if (qobject_cast<CSmartObjectHelperObject*>(pSelectedObject))
        {
            sSelectedHelper = pSelectedObject->GetName();
        }

        CSmartObjectHelperDialog dlg(this, false, false, false);
        dlg.SetSOHelper(m_sEditedClass, sSelectedHelper);
        if (dlg.exec() == QDialog::Accepted)
        {
            sSelectedHelper = dlg.GetSOHelper();
            MapHelperObjects::iterator it, itEnd = m_mapHelperObjects.end();
            for (it = m_mapHelperObjects.begin(); it != itEnd; ++it)
            {
                if (it->second->GetName() == sSelectedHelper)
                {
                    GetIEditor()->SelectObject(it->second);
                    GetIEditor()->GetObjectManager()->UnselectObject(pSelectedObject);
                    break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnHelpersNew()
{
    if (!m_sEditedClass.isEmpty())
    {
        CItemDescriptionDlg dlg(this);
        dlg.setWindowTitle(QObject::tr("New Helper"));
        dlg.setItemCaption(QObject::tr("Helper name:"));
        if (dlg.exec())
        {
            CSOLibrary::VectorHelperData::iterator it = CSOLibrary::FindHelper(m_sEditedClass, dlg.item());
            if (it != CSOLibrary::m_vHelpers.end())
            {
                QMessageBox::information(this, QString(), tr("The specified smart object helper name is already used.\n\nCreation canceled..."));
                return;
            }
            CSOLibrary::CHelperData helper;
            helper.className = m_sEditedClass;
            helper.qt.t.zero();
            helper.qt.q.SetIdentity();
            helper.name = dlg.item();
            helper.description = dlg.description();
            it = CSOLibrary::HelpersUpperBound(m_sEditedClass);
            CSOLibrary::m_vHelpers.insert(it, helper);
            CSOLibrary::m_bSaveNeeded = true;

            // this will eventually show the new helper
            m_bSinkNeeded = true;
            SinkSelection();

            // try to select the newly created helper
            CBaseObject* pSelected = GetIEditor()->GetSelectedObject();
            if (pSelected)
            {
                if (qobject_cast<CSmartObjectHelperObject*>(pSelected))
                {
                    pSelected = pSelected->GetParent();
                }

                int i = pSelected->GetChildCount();
                while (i--)
                {
                    CBaseObject* pChild = pSelected->GetChild(i);
                    if (pChild->GetName() == helper.name)
                    {
                        GetIEditor()->ClearSelection();
                        GetIEditor()->SelectObject(pChild);
                        break;
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnHelpersDelete()
{
    CSelectionGroup* pSelection = GetIEditor()->GetSelection();
    int selCount = pSelection->GetCount();

    if (!m_sEditedClass.isEmpty() && selCount == 1 && qobject_cast<CSmartObjectHelperObject*>(pSelection->GetObject(0)))
    {
        const QString msg = tr("Delete helper '%1'?").arg(pSelection->GetObject(0)->GetName());
        if (QMessageBox::warning(this, QString(), msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
        {
            DeleteHelper(m_sEditedClass, pSelection->GetObject(0)->GetName());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnHelpersDone()
{
    if (!m_sEditedClass.isEmpty())
    {
        m_bIgnoreNotifications = true;
        MapHelperObjects::iterator it, itEnd = m_mapHelperObjects.end();
        for (it = m_mapHelperObjects.begin(); it != itEnd; ++it)
        {
            CSmartObjectHelperObject* pHelperObject = it->second;
            pHelperObject->RemoveEventListener(functor(*this, &CSmartObjectsEditorDialog::OnObjectEvent));
            pHelperObject->DetachThis();
            pHelperObject->Release();
            GetIEditor()->GetObjectManager()->DeleteObject(pHelperObject);
        }
        m_mapHelperObjects.clear();
        m_bIgnoreNotifications = false;

        m_sEditedClass.clear();

        m_pItemHelpersEdit->setText(tr("Edit..."));
        m_pItemHelpersEdit->setFont(QFont());

        m_pItemHelpersNew->setEnabled(false);
        m_pItemHelpersDelete->setEnabled(false);
        m_pItemHelpersDone->setEnabled(false);

        m_bSinkNeeded = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::EnableIfOneSelected(QWidget* target)
{
    target->setEnabled(m_View->selectionModel()->selectedRows().count() == 1);
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::EnableIfSelected(QWidget* target)
{
    target->setEnabled(m_View->selectionModel()->hasSelection());
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::closeEvent(QCloseEvent* event)
{
    // This will call CSmartObjectsEditorDialog::OnUpdateProperties if needed.
    // It must be done here to ensure applying changes on the edited rule!
    m_Properties->SelectItem(0);

    QMainWindow::closeEvent(event);
}

void SmartObjectsEditorDialogLabel::mouseMoveEvent(QMouseEvent* event)
{
    update();
}

void SmartObjectsEditorDialogLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->modifiers() == Qt::NoModifier && event->buttons() == Qt::LeftButton && m_rcCloseBtn.contains(event->pos()))
    {
        m_closing = true;
    }
    event->setAccepted(m_closing);
    update();
}

void SmartObjectsEditorDialogLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->modifiers() == Qt::NoModifier && event->buttons() == Qt::NoButton && m_closing && m_rcCloseBtn.contains(event->pos()))
    {
        emit closeRequested();
    }
    if (event->button() == Qt::LeftButton)
    {
        m_closing = false;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
void SmartObjectsEditorDialogLabel::paintEvent(QPaintEvent* event)
{
    //if (!m_sFilterClasses.IsEmpty())
    {
        QPainter painter(this);
        QRect rc = rect().adjusted(0, 0, 0, -2);

        painter.fillRect(rc, palette().toolTipBase());
        painter.setPen(palette().toolTipText().color());

        QRect rcText(rc);
        rcText.setRight(max(rcText.left(), rcText.right() - 19));
        painter.drawLine(rcText.right() + 3, rcText.top() + 6, rcText.right() + 11, rcText.top() + 14);
        painter.drawLine(rcText.right() + 4, rcText.top() + 6, rcText.right() + 12, rcText.top() + 14);
        painter.drawLine(rcText.right() + 3, rcText.top() + 14, rcText.right() + 11, rcText.top() + 6);
        painter.drawLine(rcText.right() + 4, rcText.top() + 14, rcText.right() + 12, rcText.top() + 6);

        m_rcCloseBtn = QRect(QPoint(rcText.right() + 1, rcText.top() + 4), QPoint(rcText.right() + 15, rcText.bottom() - 4));

        if (text().contains(','))
        {
            painter.drawText(rcText, Qt::AlignVCenter, painter.fontMetrics().elidedText(tr(" Filtered by classes: %1").arg(text()), Qt::ElideRight, rcText.width()));
        }
        else
        {
            painter.drawText(rcText, Qt::AlignVCenter, painter.fontMetrics().elidedText(tr(" Filtered by class: %1").arg(text()), Qt::ElideRight, rcText.width()));
        }
        painter.setPen(palette().shadow().color());
        if (m_rcCloseBtn.contains(mapFromGlobal(QCursor::pos())))
        {
            painter.drawRect(m_rcCloseBtn.adjusted(0, 0, -1, -1));
        }
        painter.drawRect(rc.adjusted(0, 0, -1, -1));

        rc.setTop(rc.bottom());
        rc.setBottom(rc.bottom() + 2);
        painter.fillRect(rc, palette().button());
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObjectsEditorDialog::OnDestroy()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.setValue(QStringLiteral("SmartObjectsLayout"), saveState());
}

//////////////////////////////////////////////////////////////////////////
#define ID_REMOVE_ITEM              1
#define ID_SORT_ASC                 2
#define ID_SORT_DESC                3
#define ID_GROUP_BYTHIS             4
#define ID_SHOW_GROUPBOX            5
#define ID_SHOW_FIELDCHOOSER        6
#define ID_COLUMN_BESTFIT           7
#define ID_COLUMN_ARRANGEBY         100
#define ID_COLUMN_ALIGMENT          200
#define ID_COLUMN_ALIGMENT_LEFT     ID_COLUMN_ALIGMENT + 1
#define ID_COLUMN_ALIGMENT_RIGHT    ID_COLUMN_ALIGMENT + 2
#define ID_COLUMN_ALIGMENT_CENTER   ID_COLUMN_ALIGMENT + 3

void CSmartObjectsEditorDialog::OnReportColumnRClick(const QPoint& point)
{
    QMenu menu;

    connect(menu.addAction(tr("Name/Description")), &QAction::triggered, m_View, [=]()
        {
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnName, false);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnDescription, false);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnUserClass, true);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnUserState, true);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnObjectClass, true);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnObjectState, true);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnAction, true);
        });
    connect(menu.addAction(tr("Name/User/Object")), &QAction::triggered, m_View, [=]()
        {
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnName, false);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnDescription, true);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnUserClass, false);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnUserState, false);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnObjectClass, false);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnObjectState, false);
            m_View->header()->setSectionHidden(SmartObjectsEditorModel::ColumnAction, false);
        });

    menu.addSeparator();

    // create columns items
    int nColumnCount = m_View->model()->columnCount();
    for (int i = 0; i < nColumnCount; ++i)
    {
        const QString sCaption = m_View->model()->headerData(i, Qt::Horizontal).toString();
        if (!sCaption.isEmpty())
        {
            QAction* action = menu.addAction(sCaption);
            action->setCheckable(true);
            action->setChecked(!m_View->header()->isSectionHidden(i));
            connect(action, &QAction::triggered, this, [=]() {m_View->header()->setSectionHidden(i, !action->isChecked()); });
        }
    }

    // track menu
    menu.exec(m_View->header()->mapToGlobal(point));
}

void CSmartObjectsEditorDialog::OnReportSelChanged()
{
    // This will call CSmartObjectsEditorDialog::OnUpdateProperties if needed.
    // It must be done here to ensure applying changes on the edited rule!
    m_Properties->SelectItem(0);

    SmartObjectCondition* focused = m_View->currentIndex().data(Qt::UserRole).value<SmartObjectCondition*>();
    if (!focused)
    {
        m_Properties->setEnabled(false);
        m_Description->clear();
        return;
    }

    m_Properties->setEnabled(true);
    QSignalBlocker sb(m_Description);
    m_Description->setPlainText(focused->sDescription.c_str());

    // Update variables.
    m_Properties->EnableUpdateCallback(false);
    gSmartObjectsUI.SetConditionToUI(*focused, m_Properties, this);
    m_Properties->EnableUpdateCallback(true);

    UpdatePropertyTables();
    ///////m_View->setFocus();

    EnableIfOneSelected(m_pDuplicate);
    EnableIfSelected(m_pDelete);
}

void CSmartObjectsEditorDialog::UpdatePropertyTables()
{
    //bool nav = gSmartObjectsUI.bNavigationRule;

    //gSmartObjectsUI.entranceHelper.SetFlags( nav ? 0 : IVariable::UI_DISABLED );
    //gSmartObjectsUI.exitHelper.SetFlags( nav ? 0 : IVariable::UI_DISABLED );
    //m_Properties.Expand( m_Properties.FindItemByVar( &gSmartObjectsUI.navigationTable ), nav );

    //gSmartObjectsUI.limitsDistanceFrom.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //gSmartObjectsUI.limitsDistanceTo.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //gSmartObjectsUI.limitsOrientation.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //m_Properties.Expand( m_Properties.FindItemByVar( &gSmartObjectsUI.limitsTable ), !nav );

    //gSmartObjectsUI.delayMinimum.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //gSmartObjectsUI.delayMaximum.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //gSmartObjectsUI.delayMemory.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //m_Properties.Expand( m_Properties.FindItemByVar( &gSmartObjectsUI.delayTable ), !nav );

    //gSmartObjectsUI.multipliersProximity.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //gSmartObjectsUI.multipliersOrientation.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //gSmartObjectsUI.multipliersVisibility.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //gSmartObjectsUI.multipliersRandomness.SetFlags( nav ? IVariable::UI_DISABLED : 0 );
    //m_Properties.Expand( m_Properties.FindItemByVar( &gSmartObjectsUI.multipliersTable ), !nav );

    m_Description->setPlainText(gSmartObjectsUI.sDescription);
}

void CSmartObjectsEditorDialog::OnTreeSelChanged()
{
    m_bSinkNeeded = true;
    m_pathModel->setPath(GetCurrentFolderPath());
}

void SmartObjectsEditorModel::MoveAllRules(const QString& from, const QString& to)
{
    QString temp;
    QString from1 = from + QLatin1Char('/');
    int len = from1.length();
    SmartObjectConditions::iterator it, itEnd = CSOLibrary::GetConditions().end();
    int i = 0;
    for (it = CSOLibrary::GetConditions().begin(); it != itEnd; ++it, ++i)
    {
        SmartObjectCondition& condition = *it;
        if (from.compare(condition.sFolder.c_str(), Qt::CaseInsensitive) == 0)
        {
            condition.sFolder = to.toUtf8().data();
        }
        else
        {
            temp = condition.sFolder.substr(0, len).c_str();
            if (from1.compare(temp, Qt::CaseInsensitive) == 0)
            {
                condition.sFolder = (to + QLatin1Char('/') + condition.sFolder.substr(len).c_str()).toUtf8().data();
                setData(index(i, 0), QVariant::fromValue<SmartObjectCondition*>(&(*it)), Qt::UserRole);
            }
        }
    }
}

void CSmartObjectsEditorDialog::OnDescriptionEdit()
{
    QModelIndex index = m_View->currentIndex();
    index = index.sibling(index.row(), SmartObjectsEditorModel::ColumnDescription);
    m_View->model()->setData(index, m_Description->toPlainText());
    gSmartObjectsUI.sDescription = m_Description->toPlainText().toUtf8().data();
}

bool CSmartObjectsEditorDialog::CheckOutLibrary()
{
    // This will call CSmartObjectsEditorDialog::OnUpdateProperties if needed.
    // It must be done here to ensure applying changes on the edited rule!
    m_Properties->SelectItem(0);

    return CSOLibrary::StartEditing();
}

bool CSmartObjectsEditorDialog::SaveSOLibrary(bool updatePropertiesFirst)
{
    if (updatePropertiesFirst)
    {
        // This will call CSmartObjectsEditorDialog::OnUpdateProperties if needed.
        // It must be done here to ensure applying changes on the edited rule!
        m_Properties->SelectItem(0);
    }

    return CSOLibrary::Save();
}

CSOLibrary::~CSOLibrary()
{
    m_Conditions.clear();
    m_vHelpers.clear();
    m_vEvents.clear();
    m_vStates.clear();
    m_vClassTemplates.clear();
    m_vClasses.clear();
}

bool CSOLibrary::StartEditing()
{
    Load();

    AZStd::string fullPath = Path::GetEditingGameDataFolder() + "/" + SMART_OBJECTS_XML;
    CFileUtil::CreateDirectory(Path::GetPath(fullPath.c_str()).toUtf8().data());
    if (!CFileUtil::OverwriteFile(fullPath.c_str()))
    {
        return false;
    }
    m_bSaveNeeded = true;
    return true;
}

bool CSOLibrary::Save()
{
    AZStd::string fullPath = Path::GetEditingGameDataFolder() + "/" + SMART_OBJECTS_XML;
    if (!CFileUtil::OverwriteFile(fullPath.c_str()))
    {
        return false;
    }

    // save to SmartObjects.xml
    QString sSavePath = fullPath.c_str();
    if (!SaveToFile(sSavePath.toUtf8().data()))
    {
        Warning("CSOLibrary::Save() failed to save in %s", SMART_OBJECTS_XML);
        return false;
    }
    Log("CSOLibrary::Save() saved in %s", SMART_OBJECTS_XML);
    return true;
}

struct less_ptr
{
    bool operator() (const SmartObjectCondition* _Left, const SmartObjectCondition* _Right) const
    {
        return *_Left < *_Right;
    }
};

bool CSOLibrary::SaveToFile(const char* sFileName)
{
    typedef std::vector< const SmartObjectCondition* > RulesList;
    RulesList sorted;

    SmartObjectConditions::iterator itUnsorted, itUnsortedEnd = m_Conditions.end();
    for (itUnsorted = m_Conditions.begin(); itUnsorted != itUnsortedEnd; ++itUnsorted)
    {
        SmartObjectCondition* pCondition = &*itUnsorted;
        sorted.push_back(pCondition);
    }
    std::sort(sorted.begin(), sorted.end(), less_ptr());

    XmlNodeRef root(GetISystem()->CreateXmlNode("SmartObjectsLibrary"));
    RulesList::iterator itRules = sorted.begin();
    for (int i = 0; i < sorted.size(); ++i)
    {
        XmlNodeRef node(root->createNode("SmartObject"));
        const SmartObjectCondition& rule = **itRules;
        ++itRules;

        node->setAttr("iTemplateId",           rule.iTemplateId);

        node->setAttr("sName",                 rule.sName);
        node->setAttr("sFolder",               rule.sFolder);
        node->setAttr("sDescription",          rule.sDescription);
        //node->setAttr( "iOrder",              rule.iOrder );

        node->setAttr("sUserClass",            rule.sUserClass);
        node->setAttr("sUserState",            rule.sUserState);
        node->setAttr("sUserHelper",           rule.sUserHelper);

        node->setAttr("sObjectClass",          rule.sObjectClass);
        node->setAttr("sObjectState",          rule.sObjectState);
        node->setAttr("sObjectHelper",         rule.sObjectHelper);

        node->setAttr("iMaxAlertness",         rule.iMaxAlertness);
        node->setAttr("bEnabled",              rule.bEnabled);
        node->setAttr("sEvent",                rule.sEvent);

        node->setAttr("iRuleType",             rule.iRuleType);
        node->setAttr("sEntranceHelper",       rule.sEntranceHelper);
        node->setAttr("sExitHelper",           rule.sExitHelper);

        node->setAttr("fMinDelay",             rule.fMinDelay);
        node->setAttr("fMaxDelay",             rule.fMaxDelay);
        node->setAttr("fMemory",               rule.fMemory);

        node->setAttr("fDistanceFrom",         rule.fDistanceFrom);
        node->setAttr("fDistanceLimit",        rule.fDistanceTo);
        node->setAttr("fOrientationLimit",     rule.fOrientationLimit);
        node->setAttr("bHorizLimitOnly",       rule.bHorizLimitOnly);
        node->setAttr("fOrientToTargetLimit",  rule.fOrientationToTargetLimit);

        node->setAttr("fProximityFactor",      rule.fProximityFactor);
        node->setAttr("fOrientationFactor",    rule.fOrientationFactor);
        node->setAttr("fRandomnessFactor",     rule.fRandomnessFactor);
        node->setAttr("fVisibilityFactor",     rule.fVisibilityFactor);

        node->setAttr("fLookAtOnPerc",         rule.fLookAtOnPerc);
        node->setAttr("sObjectPreActionState", rule.sObjectPreActionState);
        node->setAttr("sUserPreActionState",   rule.sUserPreActionState);
        node->setAttr("sAction",               rule.sAction);
        node->setAttr("eActionType",           rule.eActionType);
        node->setAttr("sObjectPostActionState", rule.sObjectPostActionState);
        node->setAttr("sUserPostActionState",  rule.sUserPostActionState);

        node->setAttr("fApproachSpeed",        rule.fApproachSpeed);
        node->setAttr("iApproachStance",       rule.iApproachStance);
        node->setAttr("sAnimationHelper",      rule.sAnimationHelper);
        node->setAttr("sApproachHelper",       rule.sApproachHelper);
        node->setAttr("fStartWidth",           rule.fStartWidth);
        node->setAttr("fStartDirectionTolerance", rule.fDirectionTolerance);
        node->setAttr("fStartArcAngle",        rule.fStartArcAngle);

        root->addChild(node);
    }

    VectorClassData::iterator itClasses, itClassesEnd = m_vClasses.end();
    for (itClasses = m_vClasses.begin(); itClasses != itClassesEnd; ++itClasses)
    {
        XmlNodeRef node(root->createNode("Class"));
        const CClassData& classData = *itClasses;

        node->setAttr("name",          classData.name.toUtf8().data());
        node->setAttr("description",   classData.description.toUtf8().data());
        node->setAttr("location",      classData.location.toUtf8().data());
        node->setAttr("template",      classData.templateName.toUtf8().data());

        root->addChild(node);
    }

    VectorStateData::iterator itStates, itStatesEnd = m_vStates.end();
    for (itStates = m_vStates.begin(); itStates != itStatesEnd; ++itStates)
    {
        XmlNodeRef node(root->createNode("State"));
        const CStateData& stateData = *itStates;

        node->setAttr("name",          stateData.name.toUtf8().data());
        node->setAttr("description",   stateData.description.toUtf8().data());
        node->setAttr("location",      stateData.location.toUtf8().data());

        root->addChild(node);
    }

    VectorHelperData::const_iterator itHelpers, itHelpersEnd = m_vHelpers.end();
    for (itHelpers = m_vHelpers.begin(); itHelpers != itHelpersEnd; ++itHelpers)
    {
        XmlNodeRef node(root->createNode("Helper"));
        const CHelperData& helper = *itHelpers;

        node->setAttr("className",     helper.className.toUtf8().data());
        node->setAttr("name",          helper.name.toUtf8().data());
        node->setAttr("pos",           helper.qt.t);
        node->setAttr("rot",           helper.qt.q);
        node->setAttr("description",   helper.description.toUtf8().data());

        root->addChild(node);
    }

    VectorEventData::iterator it, itEnd = m_vEvents.end();
    for (it = m_vEvents.begin(); it != itEnd; ++it)
    {
        XmlNodeRef node(root->createNode("Event"));
        const CEventData& event = *it;

        node->setAttr("name",          event.name.toUtf8().data());
        node->setAttr("description",   event.description.toUtf8().data());

        root->addChild(node);
    }

    bool bSucceed = root->saveToFile(sFileName);
    if (bSucceed && gEnv->pAISystem)
    {
        gEnv->pAISystem->GetSmartObjectManager()->ReloadSmartObjectRules();
    }
    return bSucceed;
}

void CSOLibrary::AddAllStates(const string& sStates)
{
    if (!sStates.empty())
    {
        SetStrings setStates;
        String2Classes(sStates, setStates);
        SetStrings::const_iterator it, itEnd = setStates.end();
        for (it = setStates.begin(); it != itEnd; ++it)
        {
            AddState(*it, "", "");
        }
    }
}

bool CSOLibrary::LoadFromFile(const char* sFileName)
{
    XmlNodeRef root = GetISystem()->LoadXmlFromFile(sFileName);
    if (!root)
    {
        return false;
    }

    m_Conditions.clear();
    m_vHelpers.clear();
    m_vEvents.clear();
    m_vStates.clear();
    m_vClasses.clear();

    CHelperData helper;
    SmartObjectCondition rule;
    int count = root->getChildCount();
    for (int i = 0; i < count; ++i)
    {
        XmlNodeRef node = root->getChild(i);
        if (node->isTag("SmartObject"))
        {
            rule.iTemplateId = 0;
            node->getAttr("iTemplateId", rule.iTemplateId);

            rule.sName = node->getAttr("sName");
            rule.sDescription = node->getAttr("sDescription");
            rule.sFolder = node->getAttr("sFolder");
            rule.sEvent = node->getAttr("sEvent");

            node->getAttr("iMaxAlertness", rule.iMaxAlertness);
            node->getAttr("bEnabled", rule.bEnabled);

            rule.iRuleType = 0;
            node->getAttr("iRuleType", rule.iRuleType);

            // this was here to convert older data formats - not needed anymore.
            //float fPassRadius = 0.0f; // default value if not found in XML
            //node->getAttr( "fPassRadius", fPassRadius );
            //if ( fPassRadius > 0 )
            //  rule.iRuleType = 1;

            rule.sEntranceHelper = node->getAttr("sEntranceHelper");
            rule.sExitHelper = node->getAttr("sExitHelper");

            rule.sUserClass = node->getAttr("sUserClass");
            if (!rule.sUserClass.empty())
            {
                AddClass(rule.sUserClass, "", "", "");
            }
            rule.sUserState = node->getAttr("sUserState");
            AddAllStates(rule.sUserState);
            rule.sUserHelper = node->getAttr("sUserHelper");

            rule.sObjectClass = node->getAttr("sObjectClass");
            if (!rule.sObjectClass.empty())
            {
                AddClass(rule.sObjectClass, "", "", "");
            }
            rule.sObjectState = node->getAttr("sObjectState");
            AddAllStates(rule.sObjectState);
            rule.sObjectHelper = node->getAttr("sObjectHelper");

            node->getAttr("fMinDelay", rule.fMinDelay);
            node->getAttr("fMaxDelay", rule.fMaxDelay);
            node->getAttr("fMemory", rule.fMemory);

            rule.fDistanceFrom = 0.0f; // default value if not found in XML
            node->getAttr("fDistanceFrom", rule.fDistanceFrom);
            node->getAttr("fDistanceLimit", rule.fDistanceTo);
            node->getAttr("fOrientationLimit", rule.fOrientationLimit);
            rule.bHorizLimitOnly = false;
            node->getAttr("bHorizLimitOnly", rule.bHorizLimitOnly);
            rule.fOrientationToTargetLimit = 360.0f; // default value if not found in XML
            node->getAttr("fOrientToTargetLimit", rule.fOrientationToTargetLimit);

            node->getAttr("fProximityFactor", rule.fProximityFactor);
            node->getAttr("fOrientationFactor", rule.fOrientationFactor);
            node->getAttr("fVisibilityFactor", rule.fVisibilityFactor);
            node->getAttr("fRandomnessFactor", rule.fRandomnessFactor);

            node->getAttr("fLookAtOnPerc", rule.fLookAtOnPerc);
            rule.sUserPreActionState = node->getAttr("sUserPreActionState");
            AddAllStates(rule.sUserPreActionState);
            rule.sObjectPreActionState = node->getAttr("sObjectPreActionState");
            AddAllStates(rule.sObjectPreActionState);

            rule.sAction = node->getAttr("sAction");
            int iActionType;
            if (node->getAttr("eActionType", iActionType))
            {
                rule.eActionType = (EActionType) iActionType;
            }
            else
            {
                rule.eActionType = eAT_None;

                bool bSignal = !rule.sAction.empty() && rule.sAction[0] == '%';
                bool bAnimationSignal = !rule.sAction.empty() && rule.sAction[0] == '$';
                bool bAnimationAction = !rule.sAction.empty() && rule.sAction[0] == '@';
                bool bAction = !rule.sAction.empty() && !bSignal && !bAnimationSignal && !bAnimationAction;

                bool bHighPriority = rule.iMaxAlertness >= 100;
                node->getAttr("bHighPriority", bHighPriority);

                if (bHighPriority && bAction)
                {
                    rule.eActionType = eAT_PriorityAction;
                }
                else if (bAction)
                {
                    rule.eActionType = eAT_Action;
                }
                else if (bSignal)
                {
                    rule.eActionType = eAT_AISignal;
                }
                else if (bAnimationSignal)
                {
                    rule.eActionType = eAT_AnimationSignal;
                }
                else if (bAnimationAction)
                {
                    rule.eActionType = eAT_AnimationAction;
                }

                if (bSignal || bAnimationSignal || bAnimationAction)
                {
                    rule.sAction.erase(0, 1);
                }
            }
            rule.iMaxAlertness %= 100;

            rule.sUserPostActionState = node->getAttr("sUserPostActionState");
            AddAllStates(rule.sUserPostActionState);
            rule.sObjectPostActionState = node->getAttr("sObjectPostActionState");
            AddAllStates(rule.sObjectPostActionState);

            rule.fApproachSpeed = 1.0f; // default value if not found in XML
            node->getAttr("fApproachSpeed", rule.fApproachSpeed);
            rule.iApproachStance = 4; // default value if not found in XML
            node->getAttr("iApproachStance", rule.iApproachStance);
            rule.sAnimationHelper = ""; // default value if not found in XML
            rule.sAnimationHelper = node->getAttr("sAnimationHelper");
            rule.sApproachHelper = ""; // default value if not found in XML
            rule.sApproachHelper = node->getAttr("sApproachHelper");
            rule.fStartWidth = 0.0f; // default value if not found in XML
            node->getAttr("fStartWidth", rule.fStartWidth);
            rule.fDirectionTolerance = 0.0f; // default value if not found in XML
            node->getAttr("fStartDirectionTolerance", rule.fDirectionTolerance);
            rule.fStartArcAngle = 0.0f; // default value if not found in XML
            node->getAttr("fStartArcAngle", rule.fStartArcAngle);

            if (!node->getAttr("iOrder", rule.iOrder))
            {
                rule.iOrder = i;
            }

            m_Conditions.push_back(rule);
        }
        else if (node->isTag("Helper"))
        {
            helper.className = node->getAttr("className");
            helper.name = node->getAttr("name");
            Vec3 pos;
            node->getAttr("pos", pos);
            Quat rot;
            node->getAttr("rot", rot);
            helper.qt = QuatT(rot, pos);
            helper.description = node->getAttr("description");

            VectorHelperData::iterator it = HelpersUpperBound(helper.className);
            m_vHelpers.insert(it, helper);
        }
        else if (node->isTag("Event"))
        {
            CEventData event;
            event.name = node->getAttr("name");
            event.description = node->getAttr("description");

            VectorEventData::iterator it = std::upper_bound(m_vEvents.begin(), m_vEvents.end(), event, less_name< CEventData >());
            m_vEvents.insert(it, event);
        }
        else if (node->isTag("State"))
        {
            QString name = node->getAttr("name");
            QString description = node->getAttr("description");
            QString location = node->getAttr("location");
            AddState(name.toUtf8().data(), description.toUtf8().data(), location.toUtf8().data());
        }
        else if (node->isTag("Class"))
        {
            QString name = node->getAttr("name");
            QString description = node->getAttr("description");
            QString location = node->getAttr("location");
            QString templateName = node->getAttr("template");
            AddClass(name.toUtf8().data(), description.toUtf8().data(), location.toUtf8().data(), templateName.toUtf8().data());
        }
    }

    return true;
}

void CSmartObjectsEditorDialog::SetTemplateDefaults(SmartObjectCondition& condition, const CSOParamBase* param, QString* message /*= NULL*/) const
{
    if (!param)
    {
        return;
    }

    if (param->bIsGroup)
    {
        const CSOParamGroup* pGroup = static_cast< const CSOParamGroup* >(param);
        SetTemplateDefaults(condition, pGroup->pChildren, message);
    }
    else
    {
        const CSOParam* pParam = static_cast< const CSOParam* >(param);

        if (!message || !pParam->bEditable)
        {
            if (message)
            {
                IVariable* pVar = pParam->pVariable ? pParam->pVariable : gSmartObjectsUI.ResolveVariable(pParam, const_cast<CSmartObjectsEditorDialog*>(this));

                // Report altering on visible properties only
                if (gSmartObjectsUI.m_vars->IsContainsVariable(pVar))
                {
                    QString temp;
                    if (pParam->sName == "bNavigationRule")
                    {
                        temp = condition.iRuleType == 1 ? "1" : "0";
                    }
                    else if (pParam->sName == "sEvent")
                    {
                        temp = condition.sEvent;
                    }
                    else if (pParam->sName == "sChainedUserEvent")
                    {
                        temp = condition.sChainedUserEvent;
                    }
                    else if (pParam->sName == "sChainedObjectEvent")
                    {
                        temp = condition.sChainedObjectEvent;
                    }
                    else if (pParam->sName == "userClass")
                    {
                        temp = condition.sUserClass;
                    }
                    else if (pParam->sName == "userState")
                    {
                        temp = condition.sUserState;
                    }
                    else if (pParam->sName == "userHelper")
                    {
                        temp = condition.sUserHelper;
                    }
                    else if (pParam->sName == "iMaxAlertness")
                    {
                        temp = QString::number(condition.iMaxAlertness);
                    }
                    else if (pParam->sName == "objectClass")
                    {
                        temp = condition.sObjectClass;
                    }
                    else if (pParam->sName == "objectState")
                    {
                        temp = condition.sObjectState;
                    }
                    else if (pParam->sName == "objectHelper")
                    {
                        temp = condition.sObjectHelper;
                    }
                    else if (pParam->sName == "entranceHelper")
                    {
                        temp = condition.sEntranceHelper;
                    }
                    else if (pParam->sName == "exitHelper")
                    {
                        temp = condition.sExitHelper;
                    }
                    else if (pParam->sName == "limitsDistanceFrom")
                    {
                        temp = QString::number(condition.fDistanceFrom);
                    }
                    else if (pParam->sName == "limitsDistanceTo")
                    {
                        temp = QString::number(condition.fDistanceTo);
                    }
                    else if (pParam->sName == "limitsOrientation")
                    {
                        temp = QString::number(condition.fOrientationLimit);
                    }
                    else if (pParam->sName == "horizLimitOnly")
                    {
                        temp = condition.bHorizLimitOnly ? "1" : "0";
                    }
                    else if (pParam->sName == "limitsOrientationToTarget")
                    {
                        temp = QString::number(condition.fOrientationToTargetLimit);
                    }
                    else if (pParam->sName == "delayMinimum")
                    {
                        temp = QString::number(condition.fMinDelay);
                    }
                    else if (pParam->sName == "delayMaximum")
                    {
                        temp = QString::number(condition.fMaxDelay);
                    }
                    else if (pParam->sName == "delayMemory")
                    {
                        temp = QString::number(condition.fMemory);
                    }
                    else if (pParam->sName == "multipliersProximity")
                    {
                        temp = QString::number(condition.fProximityFactor);
                    }
                    else if (pParam->sName == "multipliersOrientation")
                    {
                        temp = QString::number(condition.fOrientationFactor);
                    }
                    else if (pParam->sName == "multipliersVisibility")
                    {
                        temp = QString::number(condition.fVisibilityFactor);
                    }
                    else if (pParam->sName == "multipliersRandomness")
                    {
                        temp = QString::number(condition.fRandomnessFactor);
                    }
                    else if (pParam->sName == "fLookAtOnPerc")
                    {
                        temp = QString::number(condition.fLookAtOnPerc);
                    }
                    else if (pParam->sName == "userPreActionState")
                    {
                        temp = condition.sUserPreActionState;
                    }
                    else if (pParam->sName == "objectPreActionState")
                    {
                        temp = condition.sObjectPreActionState;
                    }
                    else if (pParam->sName == "actionType")
                    {
                        temp = QString::number(condition.eActionType);
                    }
                    else if (pParam->sName == "actionName")
                    {
                        temp = condition.sAction;
                    }
                    else if (pParam->sName == "userPostActionState")
                    {
                        temp = condition.sUserPostActionState;
                    }
                    else if (pParam->sName == "objectPostActionState")
                    {
                        temp = condition.sObjectPostActionState;
                    }
                    else if (pParam->sName == "approachSpeed")
                    {
                        temp = QString::number(condition.fApproachSpeed);
                    }
                    else if (pParam->sName == "approachStance")
                    {
                        temp = QString::number(condition.iApproachStance);
                    }
                    else if (pParam->sName == "animationHelper")
                    {
                        temp = condition.sAnimationHelper;
                    }
                    else if (pParam->sName == "approachHelper")
                    {
                        temp = condition.sApproachHelper;
                    }
                    else if (pParam->sName == "fStartRadiusTolerance")   // backward compatibility
                    {
                        temp = QString::number(condition.fStartWidth);
                    }
                    else if (pParam->sName == "fStartWidth")
                    {
                        temp = QString::number(condition.fStartWidth);
                    }
                    else if (pParam->sName == "fStartDirectionTolerance")
                    {
                        temp = QString::number(condition.fDirectionTolerance);
                    }
                    else if (pParam->sName == "fTargetRadiusTolerance")   // backward compatibility
                    {
                        temp = QString::number(condition.fStartArcAngle);
                    }
                    else if (pParam->sName == "fStartArcAngle")
                    {
                        temp = QString::number(condition.fStartArcAngle);
                    }

                    if (temp != pParam->sValue)
                    {
                        *message += "  - ";
                        *message += pVar->GetHumanName();
                        *message += ";\n";
                    }
                }
            }
            else
            {
                if (pParam->sName == "bNavigationRule")
                {
                    condition.iRuleType = pParam->sValue == "1";
                }
                else if (pParam->sName == "sEvent")
                {
                    condition.sEvent = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "sChainedUserEvent")
                {
                    condition.sChainedUserEvent = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "sChainedObjectEvent")
                {
                    condition.sChainedObjectEvent = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "userClass")
                {
                    condition.sUserClass = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "userState")
                {
                    condition.sUserState = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "userHelper")
                {
                    condition.sUserHelper = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "iMaxAlertness")
                {
                    condition.iMaxAlertness = pParam->sValue.toInt();
                }
                else if (pParam->sName == "objectClass")
                {
                    condition.sObjectClass = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "objectState")
                {
                    condition.sObjectState = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "objectHelper")
                {
                    condition.sObjectHelper = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "entranceHelper")
                {
                    condition.sEntranceHelper = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "exitHelper")
                {
                    condition.sExitHelper = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "limitsDistanceFrom")
                {
                    condition.fDistanceFrom = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "limitsDistanceTo")
                {
                    condition.fDistanceTo = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "limitsOrientation")
                {
                    condition.fOrientationLimit = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "horizLimitOnly")
                {
                    condition.bHorizLimitOnly = pParam->sValue == "1";
                }
                else if (pParam->sName == "limitsOrientationToTarget")
                {
                    condition.fOrientationToTargetLimit = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "delayMinimum")
                {
                    condition.fMinDelay = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "delayMaximum")
                {
                    condition.fMaxDelay = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "delayMemory")
                {
                    condition.fMemory = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "multipliersProximity")
                {
                    condition.fProximityFactor = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "multipliersOrientation")
                {
                    condition.fOrientationFactor = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "multipliersVisibility")
                {
                    condition.fVisibilityFactor = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "multipliersRandomness")
                {
                    condition.fRandomnessFactor = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "fLookAtOnPerc")
                {
                    condition.fLookAtOnPerc = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "userPreActionState")
                {
                    condition.sUserPreActionState = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "objectPreActionState")
                {
                    condition.sObjectPreActionState = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "actionType")
                {
                    condition.eActionType = (EActionType) pParam->sValue.toInt();
                }
                else if (pParam->sName == "actionName")
                {
                    condition.sAction = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "userPostActionState")
                {
                    condition.sUserPostActionState = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "objectPostActionState")
                {
                    condition.sObjectPostActionState = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "approachSpeed")
                {
                    condition.fApproachSpeed = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "approachStance")
                {
                    condition.iApproachStance = pParam->sValue.toInt();
                }
                else if (pParam->sName == "animationHelper")
                {
                    condition.sAnimationHelper = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "approachHelper")
                {
                    condition.sApproachHelper = pParam->sValue.toUtf8().data();
                }
                else if (pParam->sName == "fStartRadiusTolerance")   // backward compatibility
                {
                    condition.fStartArcAngle = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "fStartArcAngle")
                {
                    condition.fStartArcAngle = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "fStartDirectionTolerance")
                {
                    condition.fDirectionTolerance = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "fTargetRadiusTolerance")   // backward compatibility
                {
                    condition.fStartArcAngle = pParam->sValue.toDouble();
                }
                else if (pParam->sName == "fStartArcAngle")
                {
                    condition.fStartArcAngle = pParam->sValue.toDouble();
                }
            }
        }
    }

    SetTemplateDefaults(condition, param->pNext, message);
}

#include <SmartObjects/SmartObjectsEditorDialog.moc>
