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
#include "MannequinDialog.h"

#include "SequencerNode.h"

#include "ViewPane.h"
#include "StringDlg.h"

#include "MannequinModelViewport.h"

#include <IGameFramework.h>

#include <ICryMannequinEditor.h>

#include "FragmentEditor.h"
#include "FragmentTrack.h"

#include "MannErrorReportDialog.h"
#include "Helper/MannequinFileChangeWriter.h"
#include "SequencerSequence.h"
#include "SequenceAnalyzerNodes.h"
#include "MannTagDefEditorDialog.h"
#include "MannContextEditorDialog.h"
#include "MannAnimDBEditorDialog.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include "MannequinConfigFileEditor.h"

#include "Controls/FragmentBrowser.h"
#include "Controls/SequenceBrowser.h"
#include "Controls/FragmentEditorPage.h"
#include "Controls/TransitionEditorPage.h"
#include "Controls/PreviewerPage.h"
#include "Objects/ObjectLayerManager.h"
#include "../Objects/EntityObject.h"
#include "QtViewPaneManager.h"

#include <QCloseEvent>
#include <QGroupBox>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QCryptographicHash>
#include <QDebug>

#include "QtUtilWin.h"

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

//////////////////////////////////////////////////////////////////////////

#define MANNEQUIN_EDITOR_VERSION "1.00"
#define MANNEQUIN_LAYOUT_SECTION "MannequinLayout"

const int CMannequinDialog::s_minPanelSize = 200;
static const char* kMannequin_setkeyproperty = "e_mannequin_setkeyproperty";
static const int kMaxRecentEntries = 10;

//////////////////////////////////////////////////////////////////////////
void SetMannequinDialogKeyPropertyCmd(IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 3)
    {
        return;
    }

    const char* propertyName = pArgs->GetArg(1);
    const char* propertyValue = pArgs->GetArg(2);

    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();

    assert(pMannequinDialog != NULL);
    pMannequinDialog->SetKeyProperty(propertyName, propertyValue);
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::RegisterViewClass()
{
#ifdef ENABLE_LEGACY_ANIMATION
    AzToolsFramework::ViewPaneOptions opt;
    opt.isPreview = true;
    opt.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    AzToolsFramework::RegisterViewPane<CMannequinDialog>(LyViewPane::LegacyMannequin, LyViewPane::CategoryAnimation, opt);
    GetIEditor()->GetSettingsManager()->AddToolName(MANNEQUIN_LAYOUT_SECTION, LyViewPane::LegacyMannequin);
    GetIEditor()->GetSettingsManager()->AddToolVersion(LyViewPane::LegacyMannequin, MANNEQUIN_EDITOR_VERSION);
#endif // ENABLE_LEGACY_ANIMATION

}

const GUID& CMannequinDialog::GetClassID()
{
    static const GUID guid =
    {
        0xd7777b3d, 0xb21e, 0x4d8a, { 0x9a, 0xee, 0x92, 0x93, 0x17, 0xb4, 0xb1, 0x6d }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////

CMannequinDialog* CMannequinDialog::s_pMannequinDialog = NULL;
bool CMannequinDialog::s_bRegisteredCommands = false;

//////////////////////////////////////////////////////////////////////////
CMannequinDialog::CMannequinDialog(QWidget* pParent /*=NULL*/)
    : QMainWindow(pParent)
    , m_pFileChangeWriter(new CMannequinFileChangeWriter())
    , m_initialShow(false)
{
    s_pMannequinDialog = this;

    GetIEditor()->RegisterNotifyListener(this);

    SEventLog toolEvent(LyViewPane::LegacyMannequin, "", MANNEQUIN_EDITOR_VERSION);
    GetIEditor()->GetSettingsManager()->RegisterEvent(toolEvent);

    GetIEditor()->GetObjectManager()->GetLayersManager()->AddUpdateListener(functor(*this, &CMannequinDialog::OnLayerUpdate));

    if (gEnv->pConsole && !gEnv->pConsole->GetCVar("e_mannequin_setkeyproperty") && !s_bRegisteredCommands)
    {
        REGISTER_COMMAND("e_mannequin_setkeyproperty", (ConsoleCommandFunc)SetMannequinDialogKeyPropertyCmd, VF_CHEAT, "");
        s_bRegisteredCommands = true;
    }

    OnInitDialog();
}

CMannequinDialog::~CMannequinDialog()
{
    OnDestroy();

    GetISystem()->GetIConsole()->RemoveCommand(kMannequin_setkeyproperty);

    s_pMannequinDialog = 0;
    GetIEditor()->UnregisterNotifyListener(this);
    GetIEditor()->GetObjectManager()->GetLayersManager()->RemoveUpdateListener(functor(*this, &CMannequinDialog::OnLayerUpdate));

    for (uint32 i = 0; i < eMEM_Max; i++)
    {
        SMannequinContextViewData& viewData = m_contexts.viewData[i];

        if (viewData.m_pEntity)
        {
            gEnv->pEntitySystem->RemoveEntity(viewData.m_pEntity->GetId());
            viewData.m_pEntity = NULL;
        }

        uint32 numContexts = m_contexts.m_contextData.size();
        for (uint32 j = 0; j < numContexts; j++)
        {
            SScopeContextData& contextData = m_contexts.m_contextData[j];
            SScopeContextViewData& contextViewData = contextData.viewData[i];

            if (contextViewData.entity)
            {
                gEnv->pEntitySystem->RemoveEntity(contextViewData.entity->GetId());
                contextViewData.entity = NULL;
            }
        }
    }

    ClearContextEntities();
    ClearContextViewData();
}


//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnLayerUpdate(int event, CObjectLayer* pLayer)
{
    switch (event)
    {
    case CObjectLayerManager::ON_LAYER_MODIFY:
    {
        //UpdateSequenceLockStatus();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMannequinDialog::LoadPreviewFile(const char* filename, XmlNodeRef& xmlSequenceNode)
{
    AZStd::string fileFullPath = Path::MakeModPathFromGamePath(filename);
    XmlNodeRef xmlData = GetISystem()->LoadXmlFromFile(fileFullPath.c_str());
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    m_wndErrorReport->Clear();

    if (xmlData && (strcmp(xmlData->getTag(), "MannequinPreview") == 0))
    {
        XmlNodeRef xmlControllerDef  = xmlData->findChild("controllerDef");
        if (!xmlControllerDef)
        {
            CryLog("[CMannequinDialog::LoadPreviewFile] <ControllerDef> missing in %s", filename);
            return false;
        }
        XmlNodeRef xmlContexts           = xmlData->findChild("contexts");

        xmlSequenceNode = xmlData->findChild("History");
        if (!xmlSequenceNode)
        {
            xmlSequenceNode = XmlHelpers::CreateXmlNode("History");
            xmlSequenceNode->setAttr("StartTime", 0);
            xmlSequenceNode->setAttr("EndTime", 0);
        }
        m_historyBackup = xmlSequenceNode->clone();

        const SControllerDef* controllerDef = mannequinSys.GetAnimationDatabaseManager().LoadControllerDef(xmlControllerDef->getAttr("filename"));
        if (!controllerDef)
        {
            CryLog("[CMannequinDialog::LoadPreviewFile] Invalid controllerDef file %s", xmlControllerDef->getAttr("filename"));
            return false;
        }

        const uint32 numContexts     = xmlContexts ? xmlContexts->getChildCount() : 0;
        const uint32 numScopes       = controllerDef->m_scopeIDs.GetNum();

        std::vector<SScopeContextData> newContextData;
        newContextData.reserve(numContexts);

        for (uint32 i = 0; i < numContexts; i++)
        {
            SScopeContextData contextDef;
            XmlNodeRef xmlContext = xmlContexts->getChild(i);
            contextDef.name = xmlContext->getAttr("name");
            contextDef.startActive = false;
            xmlContext->getAttr("enabled", contextDef.startActive);
            for (uint32 j = 0; j < eMEM_Max; j++)
            {
                contextDef.viewData[j].enabled = contextDef.startActive;
            }
            const char* databaseFilename = xmlContext->getAttr("database");
            const bool hasDatabase = (databaseFilename && (*databaseFilename != '\0'));
            if (hasDatabase)
            {
                contextDef.database = const_cast<IAnimationDatabase*>(mannequinSys.GetAnimationDatabaseManager().Load(databaseFilename));
            }
            else
            {
                contextDef.database = NULL;
            }
            contextDef.dataID = i;
            const char* contextData = xmlContext->getAttr("context");
            if (!contextData || !contextData[0])
            {
                contextDef.contextID = CONTEXT_DATA_NONE;
            }
            else
            {
                contextDef.contextID = controllerDef->m_scopeContexts.Find(contextData);
                if (contextDef.contextID == -1)
                {
                    CryLog("[CMannequinDialog::LoadPreviewFile] Invalid preview file %s, couldn't find context scope %s in controllerDef %s", filename, xmlContext->getAttr("context"), xmlControllerDef->getAttr("filename"));
                    return false;
                }
            }
            contextDef.changeCount = 0;
            const char* tagList = xmlContext->getAttr("tags");
            controllerDef->m_tags.TagListToFlags(tagList, contextDef.tags);
            contextDef.fragTags = xmlContext->getAttr("fragtags");
            const char* modelFName = xmlContext->getAttr("model");

            const char* controllerDefFilename = xmlContext->getAttr("controllerDef");
            if (controllerDefFilename && controllerDefFilename[0])
            {
                contextDef.pControllerDef = mannequinSys.GetAnimationDatabaseManager().LoadControllerDef(controllerDefFilename);
                if (!contextDef.pControllerDef)
                {
                    CryLog("[CMannequinDialog::LoadPreviewFile] Invalid controllerDef file %s", controllerDefFilename);
                    return false;
                }
            }
            const char* slaveDatabaseFilename = xmlContext->getAttr("slaveDatabase");
            if (slaveDatabaseFilename && slaveDatabaseFilename[0])
            {
                contextDef.pSlaveDatabase = mannequinSys.GetAnimationDatabaseManager().Load(slaveDatabaseFilename);
                if (!contextDef.pSlaveDatabase)
                {
                    CryLog("[CMannequinDialog::LoadPreviewFile] Invalid slave database file %s", slaveDatabaseFilename);
                    return false;
                }
            }

            if (hasDatabase && !contextDef.database)
            {
                if (QMessageBox::question(this, tr("Create new ADB?"), tr("Missing database %1 create?").arg(databaseFilename), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
                {
                    contextDef.database = mannequinSys.GetAnimationDatabaseManager().Create(databaseFilename, controllerDef->m_filename.c_str());
                }
                else
                {
                    return false;
                }
            }

            if (i == 0)
            {
                m_wndFragmentEditorPage->ModelViewport()->LoadObject(modelFName);
                m_wndTransitionEditorPage->ModelViewport()->LoadObject(modelFName);
                m_wndPreviewerPage->ModelViewport()->LoadObject(modelFName);

                if (m_wndFragmentEditorPage->ModelViewport()->GetCharacterBase() == NULL)
                {
                    QMessageBox::critical(this, tr("Missing file"), tr("[CMannequinDialog::LoadPreviewFile] Invalid file name %1, couldn't find character instance").arg(modelFName));
                    return false;
                }

                contextDef.SetCharacter(eMEM_FragmentEditor, m_wndFragmentEditorPage->ModelViewport()->GetCharacterBase());
                contextDef.SetCharacter(eMEM_TransitionEditor, m_wndTransitionEditorPage->ModelViewport()->GetCharacterBase());
                contextDef.SetCharacter(eMEM_Previewer, m_wndPreviewerPage->ModelViewport()->GetCharacterBase());

                contextDef.startLocation.SetIdentity();
            }
            else if (modelFName && modelFName[0])
            {
                const char* ext = PathUtil::GetExt(modelFName);
                const bool isCharInst = ((_stricmp(ext, "chr") == 0) || (_stricmp(ext, "cdf") == 0) || (_stricmp(ext, "cga") == 0));

                if (isCharInst)
                {
                    if (!contextDef.CreateCharInstance(modelFName))
                    {
                        QMessageBox::critical(this, tr("Missing file"), tr("[CMannequinDialog::LoadPreviewFile] Invalid file name %1, couldn't find character instance").arg(modelFName));

                        continue;
                    }
                }
                else
                {
                    if (!contextDef.CreateStatObj(modelFName))
                    {
                        QMessageBox::critical(this, tr("Missing file"), tr("[CMannequinDialog::LoadPreviewFile] Invalid file name %1, couldn't find stat obj").arg(modelFName));

                        continue;
                    }
                }

                const char* attachment = xmlContext->getAttr("attachment");
                contextDef.attachment = attachment;

                const char* attachmentContext = xmlContext->getAttr("attachmentContext");
                contextDef.attachmentContext = attachmentContext;

                const char* attachmentHelper = xmlContext->getAttr("attachmentHelper");
                contextDef.attachmentHelper = attachmentHelper;

                contextDef.startLocation.SetIdentity();
                xmlContext->getAttr("startPosition", contextDef.startLocation.t);
                if (xmlContext->getAttr("startRotation", contextDef.startRotationEuler))
                {
                    Ang3 radRotation(DEG2RAD(contextDef.startRotationEuler.x), DEG2RAD(contextDef.startRotationEuler.y), DEG2RAD(contextDef.startRotationEuler.z));
                    contextDef.startLocation.q.SetRotationXYZ(radRotation);
                }
            }

            if (contextDef.database)
            {
                Validate(contextDef);
            }

            newContextData.push_back(contextDef);
        }

        m_contexts.backGroundObjects = NULL;
        m_contexts.backgroundProps.clear();
        XmlNodeRef xmlBackgroundNode = xmlData->findChild("Background");
        if (xmlBackgroundNode)
        {
            XmlNodeRef xmlBackgroundObjects = xmlBackgroundNode->findChild("Objects");
            if (xmlBackgroundObjects)
            {
                m_contexts.backGroundObjects = xmlBackgroundObjects->clone();
            }
            xmlBackgroundNode->getAttr("Pos", m_contexts.backgroundRootTran.t);
            xmlBackgroundNode->getAttr("Rotate", m_contexts.backgroundRootTran.q);

            XmlNodeRef xmlBackgroundProps = xmlBackgroundNode->findChild("Props");
            if (xmlBackgroundProps)
            {
                const uint32 numProps = xmlBackgroundProps->getChildCount();
                for (uint32 i = 0; i < numProps; i++)
                {
                    XmlNodeRef xmlProp = xmlBackgroundProps->getChild(i);

                    SMannequinContexts::SProp prop;
                    prop.name = xmlProp->getAttr("Name");
                    prop.entity = xmlProp->getAttr("Entity");

                    m_contexts.backgroundProps.push_back(prop);
                }
            }
        }

        ClearContextViewData();

        m_contexts.previewFilename = filename;
        m_contexts.m_controllerDef = controllerDef;
        m_contexts.m_contextData   = newContextData;
        m_contexts.m_scopeData.resize(numScopes);
        for (uint32 i = 0; i < numScopes; i++)
        {
            SScopeData& scopeDef = m_contexts.m_scopeData[i];

            scopeDef.name                   = controllerDef->m_scopeIDs.GetTagName(i);
            scopeDef.scopeID            = i;
            scopeDef.contextID      = controllerDef->m_scopeDef[i].context;
            scopeDef.layer              = controllerDef->m_scopeDef[i].layer;
            scopeDef.numLayers      = controllerDef->m_scopeDef[i].numLayers;
            scopeDef.mannContexts = &m_contexts;

            for (uint32 j = 0; j < eMEM_Max; j++)
            {
                scopeDef.fragTrack[j] = NULL;
                scopeDef.animNode[j] = NULL;
                scopeDef.context[j] = NULL;
            }
        }

        if (m_wndFragmentEditorPage->KeyProperties())
        {
            m_wndFragmentEditorPage->KeyProperties()->CreateAllVars();
        }

        if (m_wndTransitionEditorPage->KeyProperties())
        {
            m_wndTransitionEditorPage->KeyProperties()->CreateAllVars();
        }

        if (m_wndPreviewerPage->KeyProperties())
        {
            m_wndPreviewerPage->KeyProperties()->CreateAllVars();
        }

        return true;
    }

    return false;
}

void CMannequinDialog::ResavePreviewFile()
{
    SavePreviewFile(m_contexts.previewFilename);
}

//////////////////////////////////////////////////////////////////////////
bool CMannequinDialog::SavePreviewFile(const char* filename)
{
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    const SControllerDef* pControllerDef = m_contexts.m_controllerDef;
    if (pControllerDef == NULL)
    {
        return false;
    }

    XmlNodeRef historyNode = m_historyBackup->clone();

    XmlNodeRef contextsNode = XmlHelpers::CreateXmlNode("contexts");
    const size_t numContexts = m_contexts.m_contextData.size();
    for (uint32 i = 0; i < numContexts; i++)
    {
        XmlNodeRef contextDefNode = XmlHelpers::CreateXmlNode("contextData");
        SScopeContextData& contextDef = m_contexts.m_contextData[i];

        contextDefNode->setAttr("name", contextDef.name.toUtf8().data());
        if (contextDef.startActive)
        {
            contextDefNode->setAttr("enabled", contextDef.startActive);
        }
        if (contextDef.database != NULL)
        {
            contextDefNode->setAttr("database", contextDef.database->GetFilename());
        }
        if (contextDef.pControllerDef != NULL)
        {
            contextDefNode->setAttr("controllerDef", contextDef.pControllerDef->m_filename.c_str());
        }
        if (contextDef.pSlaveDatabase != NULL)
        {
            contextDefNode->setAttr("slaveDatabase", contextDef.pSlaveDatabase->GetFilename());
        }

        const char* contextDataName = (contextDef.contextID == CONTEXT_DATA_NONE) ? "" : pControllerDef->m_scopeContexts.GetTagName(contextDef.contextID);
        contextDefNode->setAttr("context", contextDataName);

        CryStackStringT<char, 512> sTags;
        pControllerDef->m_tags.FlagsToTagList(contextDef.tags, sTags);
        if (!sTags.empty())
        {
            contextDefNode->setAttr("tags", sTags.c_str());
        }
        if (contextDef.fragTags.size() > 0)
        {
            contextDefNode->setAttr("fragtags", contextDef.fragTags);
        }

        if (i == 0)
        {
            assert(contextDef.viewData[0].charInst != NULL);
            if (contextDef.viewData[0].charInst != NULL)
            {
                contextDefNode->setAttr("model", contextDef.viewData[0].charInst->GetFilePath());
            }
        }
        else
        {
            if (contextDef.viewData[0].charInst)
            {
                contextDefNode->setAttr("model", contextDef.viewData[0].charInst->GetFilePath());
            }
            else if (contextDef.viewData[0].pStatObj)
            {
                contextDefNode->setAttr("model", contextDef.viewData[0].pStatObj->GetFilePath());
            }

            if (contextDef.attachment.size() > 0)
            {
                contextDefNode->setAttr("attachment", contextDef.attachment);
            }
            if (contextDef.attachmentContext.size() > 0)
            {
                contextDefNode->setAttr("attachmentContext", contextDef.attachmentContext);
            }
            if (contextDef.attachmentHelper.size() > 0)
            {
                contextDefNode->setAttr("attachmentHelper", contextDef.attachmentHelper);
            }

            if (!contextDef.startLocation.t.IsZero())
            {
                contextDefNode->setAttr("startPosition", contextDef.startLocation.t);
            }
            if (!contextDef.startRotationEuler.IsEquivalent(Ang3(ZERO)))
            {
                contextDefNode->setAttr("startRotation", contextDef.startRotationEuler);
            }
        }

        contextsNode->addChild(contextDefNode);
    }

    XmlNodeRef xmlBackground = NULL;
    if (m_contexts.backGroundObjects)
    {
        xmlBackground = contextsNode->createNode("background");
        xmlBackground->setAttr("Pos", m_contexts.backgroundRootTran.t);
        xmlBackground->setAttr("Rotate", m_contexts.backgroundRootTran.q);
        XmlNodeRef bkObjects = m_contexts.backGroundObjects->clone();
        xmlBackground->addChild(bkObjects);

        uint32 numProps = m_contexts.backgroundProps.size();
        if (numProps > 0)
        {
            XmlNodeRef xmlProps = XmlHelpers::CreateXmlNode("Props");
            xmlBackground->addChild(xmlProps);
            for (uint32 i = 0; i < numProps; i++)
            {
                SMannequinContexts::SProp& prop = m_contexts.backgroundProps[i];

                XmlNodeRef xmlProp = XmlHelpers::CreateXmlNode("Prop");
                xmlProp->setAttr("Name", prop.name.toUtf8().data());
                xmlProp->setAttr("Entity", prop.entity.toUtf8().data());
                xmlProps->addChild(xmlProp);
            }
        }
    }

    XmlNodeRef controllerDefNode = XmlHelpers::CreateXmlNode("controllerDef");
    controllerDefNode->setAttr("filename", pControllerDef->m_filename.c_str());

    // add all the children we've built above then save out the new/updated preview xml file
    XmlNodeRef previewNode = XmlHelpers::CreateXmlNode("MannequinPreview");
    previewNode->addChild(controllerDefNode);
    previewNode->addChild(contextsNode);
    previewNode->addChild(historyNode);
    if (xmlBackground)
    {
        previewNode->addChild(xmlBackground);
    }

    AZStd::string fileFullPath = Path::MakeModPathFromGamePath(filename);
    const bool saveSuccess = XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), previewNode, fileFullPath.c_str());
    return saveSuccess;
}

void CMannequinDialog::ClearContextData(const EMannequinEditorMode editorMode, const int scopeContextID)
{
    m_contexts.viewData[editorMode].m_pActionController->ClearScopeContext(scopeContextID);

    const uint32 numScopes = m_contexts.m_scopeData.size();
    SScopeContextData* pCurData = NULL;
    for (uint32 s = 0; s < numScopes; s++)
    {
        SScopeData& scopeData = m_contexts.m_scopeData[s];
        if (scopeData.contextID == scopeContextID)
        {
            pCurData = scopeData.context[editorMode];
            scopeData.context[editorMode] = NULL;
        }
    }

    if (pCurData)
    {
        assert(pCurData->viewData[editorMode].enabled);

        pCurData->viewData[editorMode].enabled = false;

        if (pCurData->attachment.length() > 0)
        {
            uint32 parentContextDataID = 0;

            uint32 parentContextID = 0;
            if (pCurData->attachmentContext.length() > 0)
            {
                parentContextID = m_contexts.m_controllerDef->m_scopeContexts.Find(pCurData->attachmentContext.c_str());

                const uint32 numScopes = m_contexts.m_scopeData.size();
                for (uint32 s = 0; s < numScopes; s++)
                {
                    const SScopeData& scopeData = m_contexts.m_scopeData[s];
                    if ((scopeData.contextID == parentContextID) && scopeData.context[editorMode])
                    {
                        parentContextDataID = scopeData.context[editorMode]->dataID;
                        break;
                    }
                }
            }

            ICharacterInstance* masterChar = m_contexts.m_contextData[parentContextDataID].viewData[editorMode].charInst;
            if (masterChar)
            {
                IAttachmentManager* attachmentManager = masterChar->GetIAttachmentManager();
                IAttachment* attachPt = attachmentManager->GetInterfaceByName(pCurData->attachment.c_str());

                if (attachPt)
                {
                    attachPt->ClearBinding();

                    attachPt->SetAttRelativeDefault(pCurData->viewData[editorMode].oldAttachmentLoc);
                }
            }
        }
        else
        {
            pCurData->viewData[editorMode].entity->Hide(true);
        }
    }
}

bool CMannequinDialog::EnableContextData(const EMannequinEditorMode editorMode, const int scopeContextDataID)
{
    const uint32 numContexts = m_contexts.m_contextData.size();

    for (uint32 i = 0; i < numContexts; i++)
    {
        SScopeContextData& contextData = m_contexts.m_contextData[i];
        if (contextData.dataID == scopeContextDataID)
        {
            if (!contextData.viewData[editorMode].enabled)
            {
                if (contextData.contextID != CONTEXT_DATA_NONE)
                {
                    ClearContextData(editorMode, contextData.contextID);
                }

                if (contextData.attachment.length() > 0)
                {
                    uint32 parentContextDataID = 0;
                    if (contextData.attachmentContext.length() > 0)
                    {
                        const uint32 parentContextID = m_contexts.m_controllerDef->m_scopeContexts.Find(contextData.attachmentContext.c_str());

                        const uint32 numScopes = m_contexts.m_scopeData.size();
                        for (uint32 s = 0; s < numScopes; s++)
                        {
                            const SScopeData& scopeData = m_contexts.m_scopeData[s];
                            if (scopeData.contextID == parentContextID)
                            {
                                if (scopeData.context[editorMode] != NULL)
                                {
                                    parentContextDataID = scopeData.context[editorMode]->dataID;
                                    break;
                                }
                                else
                                {
                                    // Failed to find a parent context, commonly weapons scopes/attachments need a weapon selecting first.
                                    return false;
                                }
                            }
                        }
                    }

                    ICharacterInstance* masterChar = m_contexts.m_contextData[parentContextDataID].viewData[editorMode].charInst;
                    if (masterChar)
                    {
                        IAttachmentManager* attachmentManager = masterChar->GetIAttachmentManager();
                        IAttachment* attachPt = attachmentManager->GetInterfaceByName(contextData.attachment.c_str());

                        if (attachPt)
                        {
                            attachPt->AlignJointAttachment();
                            contextData.viewData[editorMode].oldAttachmentLoc = attachPt->GetAttRelativeDefault();

                            if (contextData.viewData[editorMode].charInst)
                            {
                                CSKELAttachment* chrAttachment = new CSKELAttachment();
                                chrAttachment->m_pCharInstance  = contextData.viewData[editorMode].charInst;

                                attachPt->AddBinding(chrAttachment);
                            }
                            else
                            {
                                CCGFAttachment* cfgAttachment = new CCGFAttachment();
                                cfgAttachment->pObj = contextData.viewData[editorMode].pStatObj;
                                attachPt->AddBinding(cfgAttachment);

                                if (contextData.attachmentHelper.length() > 0)
                                {
                                    Matrix34 helperMat = contextData.viewData[editorMode].pStatObj->GetHelperTM(contextData.attachmentHelper.c_str());
                                    helperMat.InvertFast();

                                    QuatT requiredRelativeLocation = contextData.viewData[editorMode].oldAttachmentLoc * QuatT(helperMat.GetTranslation(), Quat(helperMat).GetNormalized());
                                    requiredRelativeLocation.q.Normalize();

                                    attachPt->SetAttRelativeDefault(requiredRelativeLocation);
                                }
                            }
                        }
                    }
                }
                else
                {
                    contextData.viewData[editorMode].entity->Hide(false);
                }

                if (contextData.viewData[editorMode].m_pActionController)
                {
                    m_contexts.viewData[editorMode].m_pActionController->SetSlaveController(*contextData.viewData[editorMode].m_pActionController, contextData.contextID, true, NULL);
                }
                else if (contextData.database)
                {
                    m_contexts.viewData[editorMode].m_pActionController->SetScopeContext(contextData.contextID, *contextData.viewData[editorMode].entity, contextData.viewData[editorMode].charInst, contextData.database);
                }

                const uint32 numScopes = m_contexts.m_scopeData.size();
                for (uint32 s = 0; s < numScopes; s++)
                {
                    SScopeData& scopeData = m_contexts.m_scopeData[s];
                    if (scopeData.contextID == contextData.contextID)
                    {
                        scopeData.context[editorMode] = &contextData;
                    }
                }
                contextData.viewData[editorMode].enabled = true;
            }
        }
    }

    return true;
}

bool CMannequinDialog::InitialiseToPreviewFile(const char* previewFile)
{
    XmlNodeRef sequenceNode;
    const bool loadedSetup = LoadPreviewFile(previewFile, sequenceNode);

    EnableContextEditor(loadedSetup);

    if (loadedSetup)
    {
        IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

        const char* PAGE_POSTFIX[eMEM_Max] = {"", "_Prv", "_Trn"};

        for (uint32 em = 0; em < eMEM_Max; em++)
        {
            if (!m_contexts.m_contextData.empty())
            {
                ICharacterInstance* pIChar = m_contexts.m_contextData[0].viewData[em].charInst;
                if (pIChar)
                {
                    pIChar->GetISkeletonAnim()->SetAnimationDrivenMotion(true);

                    ISkeletonPose& skelPose = *pIChar->GetISkeletonPose();
                    IAnimationPoseBlenderDir* pPoseBlenderAim = skelPose.GetIPoseBlenderAim();
                    if (pPoseBlenderAim)
                    {
                        pPoseBlenderAim->SetState(false);
                        pPoseBlenderAim->SetLayer(3);
                        pPoseBlenderAim->SetTarget(Vec3(0.0f, 0.0f, 0.0f));
                        pPoseBlenderAim->SetFadeoutAngle(DEG2RAD(180.0f));
                    }

                    IAnimationPoseBlenderDir* pPoseBlenderLook = skelPose.GetIPoseBlenderLook();
                    if (pPoseBlenderLook)
                    {
                        pPoseBlenderLook->SetState(false);
                    }
                }
            }

            IEntity* pMannEntity = NULL;
            IEntityClass* pEntityClass =  gEnv->pEntitySystem->GetClassRegistry()->FindClass("Default");
            assert(pEntityClass);
            for (uint32 i = 0; i < m_contexts.m_contextData.size(); i++)
            {
                SScopeContextData& context = m_contexts.m_contextData[i];

                SEntitySpawnParams params;
                params.pClass = pEntityClass;
                QByteArray name = (context.name + PAGE_POSTFIX[em]).toUtf8();
                params.sName = name.data();
                params.vPosition.Set(0.0f, 0.0f, 0.0f);
                params.qRotation.SetIdentity();
                params.nFlags |= (ENTITY_FLAG_NEVER_NETWORK_STATIC | ENTITY_FLAG_CLIENT_ONLY);

                context.viewData[em].entity = gEnv->pEntitySystem->SpawnEntity(params, true);
                assert(context.viewData[em].entity);
                if (context.viewData[em].charInst)
                {
                    context.viewData[em].entity->SetCharacter(context.viewData[em].charInst, 0);
                }
                else
                {
                    context.viewData[em].entity->SetStatObj(context.viewData[em].pStatObj, 0, false);
                }

                if (context.pControllerDef)
                {
                    context.viewData[em].m_pAnimContext = new SEditorAnimationContext(this, *context.pControllerDef);
                    context.viewData[em].m_pActionController = mannequinSys.CreateActionController(context.viewData[em].entity, *context.viewData[em].m_pAnimContext);
                    context.viewData[em].m_pActionController->SetScopeContext(0, *context.viewData[em].entity, context.viewData[em].charInst, context.pSlaveDatabase ? context.pSlaveDatabase : context.database);
                }

                if (!pMannEntity)
                {
                    pMannEntity = context.viewData[em].entity;
                }
                else
                {
                    context.viewData[em].entity->Hide(true);
                }
            }
            m_contexts.viewData[em].m_pEntity = pMannEntity;

            m_contexts.viewData[em].m_pAnimContext = new SEditorAnimationContext(this, *m_contexts.m_controllerDef);
            m_contexts.viewData[em].m_pActionController = mannequinSys.CreateActionController(pMannEntity, *m_contexts.viewData[em].m_pAnimContext);

            m_contexts.potentiallyActiveScopes = 0;
            const uint32 numContextData = m_contexts.m_contextData.size();
            uint32 installedContexts = 0;
            for (uint32 i = 0; i < numContextData; i++)
            {
                SScopeContextData& contextData = m_contexts.m_contextData[i];
                const uint32 contextFlag = (contextData.contextID == CONTEXT_DATA_NONE) ? 0 : (1 << contextData.contextID);
                const uint32 numScopes = m_contexts.m_scopeData.size();
                for (uint32 s = 0; s < numScopes; s++)
                {
                    if (m_contexts.m_scopeData[s].contextID == contextData.contextID)
                    {
                        m_contexts.potentiallyActiveScopes |= (1 << m_contexts.m_scopeData[s].scopeID);
                    }
                }

                if (((installedContexts & contextFlag) == 0) && contextData.viewData[em].enabled)
                {
                    contextData.viewData[em].enabled = false;
                    EnableContextData((EMannequinEditorMode)em, i);
                    installedContexts |= contextFlag;
                }
            }

            //--- Create background objects
            SMannequinContextViewData::TBackgroundObjects& backgroundObjects = m_contexts.viewData[em].backgroundObjects;
            backgroundObjects.clear();
            if (m_contexts.backGroundObjects)
            {
                QuatT inverseRoot = m_contexts.backgroundRootTran.GetInverted();
                GetIEditor()->SuspendUndo();
                CObjectArchive ar(GetIEditor()->GetObjectManager(), m_contexts.backGroundObjects, true);
                ar.EnableProgressBar(false); // No progress bar is shown when loading objects.
                ar.MakeNewIds(true);
                ar.LoadObjects(m_contexts.backGroundObjects);
                ar.ResolveObjects();
                const int numObjects = ar.GetLoadedObjectsCount();
                for (int i = 0; i < numObjects; i++)
                {
                    CBaseObject* pObj = ar.GetLoadedObject(i);
                    if (pObj)
                    {
                        // TODO Jean: review
                        // pObj->SetFlags(OBJFLAG_NOTINWORLD);

                        if (!pObj->GetParent())
                        {
                            QuatT childTran(pObj->GetPos(), pObj->GetRotation());
                            QuatT adjustedTran = inverseRoot * childTran;
                            pObj->SetPos(adjustedTran.t);
                            pObj->SetRotation(adjustedTran.q);
                            backgroundObjects.push_back(pObj);

                            //--- Store off the IDs for each prop
                            if (qobject_cast<CEntityObject*>(pObj))
                            {
                                CEntityObject* pEntObject = (CEntityObject*)pObj;
                                const uint32 numProps = m_contexts.backgroundProps.size();
                                for (uint32 i = 0; i < numProps; i++)
                                {
                                    if (pObj->GetName() == m_contexts.backgroundProps[i].entity)
                                    {
                                        m_contexts.backgroundProps[i].entityID[em] = pEntObject->GetIEntity()->GetId();
                                    }
                                }
                            }
                        }
                    }
                }
                GetIEditor()->ResumeUndo();
            }
        }

        PopulateTagList();

        m_wndFragmentEditorPage->InitialiseToPreviewFile(sequenceNode);
        m_wndTransitionEditorPage->InitialiseToPreviewFile(sequenceNode);
        m_wndPreviewerPage->InitialiseToPreviewFile();

        m_pFileChangeWriter->SetControllerDef(m_contexts.m_controllerDef);

        m_listUsedAnimationsCurrentPreview->setEnabled(true);
    }

    return loadedSetup;
}

void CMannequinDialog::LoadNewPreviewFile(const char* previewFile)
{
    m_bPreviewFileLoaded = false;

    if (previewFile == NULL)
    {
        previewFile = m_contexts.previewFilename.c_str();
    }

    if (InitialiseToPreviewFile(previewFile))
    {
        m_wndFragmentBrowser->SetScopeContext(-1);
        m_wndFragmentBrowser->SetContext(m_contexts);
        m_wndTransitionBrowser->SetContext(m_contexts);

        m_wndFragmentEditorPage->ModelViewport()->ClearCharacters();
        m_wndTransitionEditorPage->ModelViewport()->ClearCharacters();
        m_wndPreviewerPage->ModelViewport()->ClearCharacters();

        for (uint32 i = 0; i < m_contexts.m_contextData.size(); i++)
        {
            const SScopeContextData& contextData = m_contexts.m_contextData[i];
            if (contextData.attachment.length() == 0)
            {
                m_wndFragmentEditorPage->ModelViewport()->AddCharacter(contextData.viewData[eMEM_FragmentEditor].entity, contextData.startLocation);
                m_wndTransitionEditorPage->ModelViewport()->AddCharacter(contextData.viewData[eMEM_TransitionEditor].entity, contextData.startLocation);
                m_wndPreviewerPage->ModelViewport()->AddCharacter(contextData.viewData[eMEM_Previewer].entity, contextData.startLocation);
            }
        }

        m_wndFragmentEditorPage->ModelViewport()->OnLoadPreviewFile();
        m_wndTransitionEditorPage->ModelViewport()->OnLoadPreviewFile();
        m_wndPreviewerPage->ModelViewport()->OnLoadPreviewFile();

        m_wndFragmentEditorPage->TrackPanel()->SetFragment(FRAGMENT_ID_INVALID);

        PushMostRecentlyUsedEntry(QString::fromLatin1(previewFile));

        m_bPreviewFileLoaded = true;
    }
}

void CMannequinDialog::addDockWidget(Qt::DockWidgetArea area, QWidget* widget, const QString& title, bool closable)
{
    QDockWidget* w = new AzQtComponents::StyledDockWidget(title);
    w->setObjectName(widget->metaObject()->className());
    widget->setParent(w);
    w->setWidget(widget);
    if (!closable)
    {
        w->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    }
    if (widget->metaObject()->indexOfSlot("slotVisibilityChanged(bool)") != -1)
    {
        connect(w, SIGNAL(visibilityChanged(bool)), widget, SLOT(slotVisibilityChanged(bool)));
    }
    QMainWindow::addDockWidget(area, w);
}

void CMannequinDialog::tabifyDockWidget(QWidget* widget1, QWidget* widget2)
{
    QMainWindow::tabifyDockWidget(findDockWidget(widget1), findDockWidget(widget2));
}

QDockWidget* CMannequinDialog::findDockWidget(QWidget* widget)
{
    if (widget == nullptr)
    {
        return nullptr;
    }
    QDockWidget* w = qobject_cast<QDockWidget*>(widget);
    if (w)
    {
        return w;
    }
    return findDockWidget(widget->parentWidget());
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnInitDialog()
{
    m_bPreviewFileLoaded = false;
    m_bRefreshingTagCtrl = false;

    m_tagVars.reset(new CVarBlock());

    // Transition editor page
    m_wndTransitionEditorPage = new CTransitionEditorPage(this);
    m_wndTransitionEditorPage->setWindowTitle(tr("Transition Editor"));

    // Transition browser
    m_wndTransitionBrowser = new CTransitionBrowser(*m_wndTransitionEditorPage, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_wndTransitionBrowser, tr("Transititons"), false);

    // Previewer page
    m_wndPreviewerPage = new CPreviewerPage(this);
    m_wndPreviewerPage->setWindowTitle(tr("Previewer"));

    // Create the sequence browser
    m_wndSequenceBrowser = new CSequenceBrowser(*m_wndPreviewerPage, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_wndSequenceBrowser, tr("Sequences"), false);

    // Fragment editor page
    m_wndFragmentEditorPage = new CFragmentEditorPage(this);
    m_wndFragmentEditorPage->setWindowTitle(tr("Fragment Editor"));

    // Fragment browser
    m_wndFragmentBrowser = new CFragmentBrowser(*m_wndFragmentEditorPage->TrackPanel(), this);

    // Tags panel
    m_wndTagsPanel = new ReflectedPropertiesPanel(this);
    m_wndTagsPanel->Setup();
    m_wndTagsPanel->setEnabled(false);
    m_wndTagsPanel->setMinimumHeight(75);

    // Fragment rollup panel
    m_wndFragmentsRollup = new QGroupBox(this);
    m_wndFragmentsRollup->setLayout(new QHBoxLayout);
    m_wndFragmentsRollup->setTitle(tr("No Fragment Selected"));
    QFont f;
    f.setPixelSize(16);
    f.setBold(true);
    m_wndFragmentsRollup->setFont(f);
    m_wndFragmentsRollup->layout()->addWidget(m_wndTagsPanel);

    // Fragment panel splitter
    m_wndFragmentSplitter = new CFragmentSplitter;
    m_wndFragmentSplitter->addWidget(m_wndFragmentBrowser);
    m_wndFragmentSplitter->addWidget(m_wndFragmentsRollup);
    addDockWidget(Qt::LeftDockWidgetArea, m_wndFragmentSplitter, tr("Fragments"), false);

    tabifyDockWidget(m_wndTransitionBrowser, m_wndSequenceBrowser);
    tabifyDockWidget(m_wndTransitionBrowser, m_wndFragmentSplitter);
    m_wndFragmentSplitter->raise();

    // Error report
    m_wndErrorReport = new CMannErrorReportDialog(this);
    m_wndErrorReport->setWindowTitle(tr("Mannequin Error Report"));

    m_central = new QTabWidget(this);
    m_central->setTabPosition(QTabWidget::South);
    m_central->setMovable(true);
    m_central->setSizePolicy({ QSizePolicy::Expanding, QSizePolicy::Expanding });
    m_central->addTab(m_wndFragmentEditorPage, m_wndFragmentEditorPage->windowTitle());
    m_central->addTab(m_wndTransitionEditorPage, m_wndTransitionEditorPage->windowTitle());
    m_central->addTab(m_wndPreviewerPage, m_wndPreviewerPage->windowTitle());
    m_central->addTab(m_wndErrorReport, m_wndErrorReport->windowTitle());
    connect(m_central, &QTabWidget::currentChanged, m_central, [=]()
        {
		QWidget* current = m_central->currentWidget();
		for (int i = 0; i < m_central->count(); ++i)
		{
			QWidget* widget = m_central->widget(i);
			if (widget->metaObject()->indexOfSlot("slotVisibilityChanged(bool)") != -1)
                {
				QMetaObject::invokeMethod(widget, "slotVisibilityChanged", Q_ARG(bool, widget == current));
		}
            }
	});

    setCentralWidget(m_central);

    // Create the menu bar
    QMenuBar* pMenuBar = menuBar();
    QMenu* menuFile = pMenuBar->addMenu(tr("&File"));
    menuFile->addAction(tr("&New Preview..."), this, SLOT(OnMenuNewPreview()));
    menuFile->addAction(tr("&Load Preview Setup..."), this, SLOT(OnMenuLoadPreviewFile()));
    menuFile->addSeparator();
    QMenu* menuMostRecentlyUsed = menuFile->addMenu(tr("&Recent Files"));
    menuFile->addSeparator();
    m_contextEditor = menuFile->addAction(tr("&Context Editor..."), this, SLOT(OnContexteditor()));
    m_animDBEditor = menuFile->addAction(tr("&Animation DB Editor..."), this, SLOT(OnAnimDBEditor()));
    menuFile->addAction(tr("&Tag Definition Editor..."), this, SLOT(OnTagDefinitionEditor()));
    menuFile->addSeparator();
    menuFile->addAction(tr("&Save Changes..."), this, SLOT(OnSave()));
    menuFile->addAction(tr("&Re-export All..."), this, SLOT(OnReexportAll()));
    QMenu* menuPreviewer = pMenuBar->addMenu(tr("&Previewer"));
    m_newSequence = menuPreviewer->addAction(tr("&New Sequence"), this, SLOT(OnNewSequence()));
    m_loadSequence = menuPreviewer->addAction(tr("&Load Sequence..."), this, SLOT(OnLoadSequence()));
    m_saveSequence = menuPreviewer->addAction(tr("&Save Sequence..."), this, SLOT(OnSaveSequence()));
    QMenu* menuView = pMenuBar->addMenu(tr("&View"));
    m_viewFragmentEditor = menuView->addAction(tr("&Fragment Editor"), this, SLOT(OnViewFragmentEditor()));
    m_viewPreviewer = menuView->addAction(tr("&Previewer"), this, SLOT(OnViewPreviewer()));
    m_viewTransitionEditor = menuView->addAction(tr("&Transition Editor"), this, SLOT(OnViewTransitionEditor()));
    m_viewErrorReport = menuView->addAction(tr("&Error Report"), this, SLOT(OnViewErrorReport()));
    QMenu* menuTools = pMenuBar->addMenu(tr("&Tools"));
    menuTools->addAction(tr("&List Used Animations..."), this, SLOT(OnToolsListUsedAnimations()));
    m_listUsedAnimationsCurrentPreview = menuTools->addAction(tr("List &Used Animations (Current Preview)..."), this, SLOT(OnToolsListUsedAnimationsForCurrentPreview()));

    connect(menuFile, &QMenu::aboutToShow, this, &CMannequinDialog::OnUpdateActions);
    connect(menuPreviewer, &QMenu::aboutToShow, this, &CMannequinDialog::OnUpdateActions);
    connect(menuView, &QMenu::aboutToShow, this, &CMannequinDialog::OnUpdateActions);
    connect(menuMostRecentlyUsed, &QMenu::aboutToShow, this, &CMannequinDialog::PopulateMostRecentlyUsed);
    connect(menuMostRecentlyUsed, &QMenu::triggered, this, &CMannequinDialog::OnMostRecentlyUsedEntryTriggered);

    m_listUsedAnimationsCurrentPreview->setEnabled(false);

    m_wndFragmentBrowser->SetOnSelectedItemEditCallback(functor(*this, &CMannequinDialog::OnFragmentBrowserSelectedItemEdit));
    m_wndFragmentBrowser->SetOnScopeContextChangedCallback(functor(*this, &CMannequinDialog::OnFragmentBrowserScopeContextChanged));
    UpdateAnimationDBEditorMenuItemEnabledState();
    EnableContextEditor(false);

    {
        const ICVar* const pCVarOverridePreviewFile = gEnv->pConsole->GetCVar("mn_override_preview_file");
        QString szDefaultPreviewFile;
        if (pCVarOverridePreviewFile && pCVarOverridePreviewFile->GetString() && pCVarOverridePreviewFile->GetString()[0])
        {
            szDefaultPreviewFile = pCVarOverridePreviewFile->GetString();
        }

        if (!szDefaultPreviewFile.isEmpty())
        {
            LoadNewPreviewFile(szDefaultPreviewFile.toUtf8().data());
        }

        RefreshAndActivateErrorReport();
    }

    // Initialize actions
    OnUpdateActions();
}

void CMannequinDialog::SaveLayouts()
{
    QByteArray state = saveState();
    QSettings settings;
    settings.beginGroup("MannequinEditor");
    settings.setValue("state", state);
    settings.endGroup();
    settings.sync();

    m_wndFragmentEditorPage->SaveLayout();
    m_wndTransitionEditorPage->SaveLayout();
    m_wndPreviewerPage->SaveLayout();

    MannUtils::SaveSplitterToRegistry(QStringLiteral("Settings\\Mannequin\\FragmentBrowserLayout"), QStringLiteral("FragmentSplitter"), m_wndFragmentSplitter);
}

void CMannequinDialog::LoadLayouts()
{
    QSettings settings;
    settings.beginGroup("MannequinEditor");
    if (settings.contains("state"))
    {
        QByteArray state = settings.value("state").toByteArray();
        if (!state.isEmpty())
        {
            restoreState(state);
        }
    }

    m_wndFragmentEditorPage->LoadLayout();
    m_wndTransitionEditorPage->LoadLayout();
    m_wndPreviewerPage->LoadLayout();

    MannUtils::LoadSplitterFromRegistry(QStringLiteral("Settings\\Mannequin\\FragmentBrowserLayout"), QStringLiteral("FragmentSplitter"), m_wndFragmentSplitter, s_minPanelSize);
}

void CMannequinDialog::SaveMostRecentlyUsed()
{
    const auto pathHash = QCryptographicHash::hash(Path::GetEditingGameDataFolder().c_str(),
        QCryptographicHash::Md5);

    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("MannequinEditor");
    settings.beginGroup("MRU");
    settings.setValue(pathHash.toHex(), m_MostRecentlyUsedPreviews);
    settings.endGroup();
    settings.endGroup();
    settings.sync();
}

void CMannequinDialog::LoadMostRecentlyUsed()
{
    const auto pathHash = QCryptographicHash::hash(Path::GetEditingGameDataFolder().c_str(),
        QCryptographicHash::Md5);

    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("MannequinEditor");
    settings.beginGroup("MRU");
    if (settings.contains(pathHash.toHex()))
    {
        m_MostRecentlyUsedPreviews = settings.value(pathHash.toHex()).toStringList();
    }
    settings.endGroup();
    settings.endGroup();
}

bool CMannequinDialog::LoadMostRecentPreview()
{
    while (!m_MostRecentlyUsedPreviews.isEmpty() && !m_bPreviewFileLoaded)
    {
        if (!OpenPreview(qPrintable(m_MostRecentlyUsedPreviews.first())))
        {
            m_MostRecentlyUsedPreviews.pop_front();
        }
    }

    return m_bPreviewFileLoaded;
}

void CMannequinDialog::ShowLaunchDialog()
{
    QMessageBox dlg(this);

    dlg.setWindowTitle(QStringLiteral("Mannequin Preview"));
    dlg.setText(QStringLiteral("Create or load a Mannequin preview file."));
    dlg.setEscapeButton(QMessageBox::Cancel);

    auto createBtn = dlg.addButton(QStringLiteral("Create..."), QMessageBox::ActionRole);
    auto loadBtn = dlg.addButton(QStringLiteral("Load..."), QMessageBox::ActionRole);
    dlg.addButton(QMessageBox::Cancel);
    
    do
    {
        dlg.exec();

        if (dlg.clickedButton() == createBtn)
        {
            OnMenuNewPreview();
        }
        else if (dlg.clickedButton() == loadBtn)
        {
            OnMenuLoadPreviewFile();
        }
        else
        {
            break;
        }
    }
    while (!m_bPreviewFileLoaded);
}

void CMannequinDialog::PushMostRecentlyUsedEntry(const QString& fileName)
{
    const int mruIndex = m_MostRecentlyUsedPreviews.indexOf(fileName);
    if (mruIndex > 0)
    {
        m_MostRecentlyUsedPreviews.move(mruIndex, 0);
    }
    else if (mruIndex == -1)
    {
        m_MostRecentlyUsedPreviews.push_front(fileName);
    }

    /* cleanup overflow */
    if (m_MostRecentlyUsedPreviews.count() > kMaxRecentEntries)
    {
        m_MostRecentlyUsedPreviews.pop_back();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (m_wndFragmentEditorPage->TrackPanel())
    {
        m_wndFragmentEditorPage->TrackPanel()->OnEditorNotifyEvent(event);
    }
    switch (event)
    {
    case eNotify_OnCloseScene:
        ClearContextEntities();
        ClearContextViewData();
        break;
    case eNotify_OnBeginSceneOpen:
    case eNotify_OnBeginNewScene:
    case eNotify_OnEndLoad:
        ReloadContextEntities();
        break;
    //case eNotify_OnBeginNewScene:
    //case eNotify_OnCloseScene:
    //  SetCurrentSequence(NULL);
    //  break;
    //case eNotify_OnBeginSceneOpen:
    //  m_bInLevelLoad = true;
    //  m_wndTrackProps.Reset();
    //  break;
    //case eNotify_OnEndSceneOpen:
    //  m_bInLevelLoad = false;
    //  ReloadSequences();
    //  break;
    //case eNotify_OnMissionChange:
    //  if (!m_bInLevelLoad)
    //      ReloadSequences();
    //  break;
    case eNotify_OnUpdateSequencerKeySelection:
        m_wndFragmentEditorPage->KeyProperties()->OnKeySelectionChange();
        m_wndPreviewerPage->KeyProperties()->OnKeySelectionChange();
        m_wndTransitionEditorPage->KeyProperties()->OnKeySelectionChange();
        m_wndFragmentEditorPage->CreateLocators();
        break;
    case eNotify_OnUpdateSequencer:
        m_wndPreviewerPage->OnUpdateTV();
        break;
    case eNotify_OnUpdateSequencerKeys:
        break;
    case eNotify_OnIdleUpdate:
        Update();
        break;
    }
}

void CMannequinDialog::OnRender()
{
    //if (m_sequenceAnalyser)
    //{
    //  bool isDraggingTime = m_wndKeys->IsDraggingTime();
    //  float dsTime = m_wndKeys->GetTime();

    //  const float XPOS = 200.0f, YPOS = 60.0f, FONT_SIZE = 2.0f, FONT_COLOUR[4] = {1,1,1,1};
    //  gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, FONT_SIZE, FONT_COLOUR, false, "%s %f", isDraggingTime ? "Dragging Time" : "Not Dragging", dsTime);
    //}
}

void CMannequinDialog::Update()
{
    m_wndFragmentBrowser->Update();

    if (m_bPreviewFileLoaded)
    {
        m_wndFragmentEditorPage->Update();
        m_wndPreviewerPage->Update();
        m_wndTransitionEditorPage->Update();
    }
}

bool CMannequinDialog::CheckChangedData()
{
    return m_pFileChangeWriter->ShowFileManager() != eFMR_Cancel;
}

void CMannequinDialog::OnMoveLocator(EMannequinEditorMode editorMode, uint32 refID, const char* locatorName, const QuatT& transform)
{
    SFragmentHistoryContext* historyContext = NULL;
    if (editorMode == eMEM_Previewer)
    {
        historyContext = m_wndPreviewerPage->FragmentHistory();
    }
    else if (editorMode == eMEM_TransitionEditor)
    {
        historyContext = m_wndTransitionEditorPage->FragmentHistory();
    }
    else
    {
        historyContext = &m_wndFragmentEditorPage->TrackPanel()->GetFragmentHistory();
    }

    if (refID < historyContext->m_history.m_items.size())
    {
        historyContext->m_history.m_items[refID].param.value = transform;

        const uint32 numTracks = historyContext->m_tracks.size();
        for (uint32 i = 0; i < numTracks; i++)
        {
            CSequencerTrack* animTrack = historyContext->m_tracks[i];
            if (animTrack->GetParameterType() == SEQUENCER_PARAM_PARAMS)
            {
                CParamKey paramKey;
                const int numKeys = animTrack->GetNumKeys();
                for (int k = 0; k < numKeys; k++)
                {
                    animTrack->GetKey(k, &paramKey);

                    if (paramKey.historyItem == refID)
                    {
                        paramKey.parameter.value = transform;
                        animTrack->SetKey(k, &paramKey);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        CSequencerUtils::SelectedKeys selectedKeys;
        CSequencerUtils::GetSelectedKeys(m_wndFragmentEditorPage->TrackPanel()->GetSequence(), selectedKeys);

        uint iIndex = refID - historyContext->m_history.m_items.size();

        assert(iIndex < selectedKeys.keys.size());

        CSequencerUtils::SelectedKey& selectedKey = selectedKeys.keys[iIndex];

        const ESequencerParamType paramType = selectedKey.pTrack->GetParameterType();
        assert(paramType == SEQUENCER_PARAM_PROCLAYER);

        CProcClipKey* key = (CProcClipKey*)selectedKey.pTrack->GetKey(selectedKey.nKey);

        assert(key != NULL);
        if (key->pParams)
        {
            key->pParams->OnEditorMovedTransformGizmo(transform);
            selectedKey.pTrack->OnChange();
        }
    }

    FragmentEditor()->KeyProperties()->ForceUpdate();
}

void CMannequinDialog::EditFragmentByKey(const CFragmentKey& key, int contextID)
{
    if (key.fragmentID != FRAGMENT_ID_INVALID)
    {
        EditFragment(key.fragmentID, key.tagStateFull, contextID);
    }
}

void CMannequinDialog::EditFragment(FragmentID fragID, const SFragTagState& tagState, int contextID)
{
    m_wndFragmentEditorPage->SetTagState(fragID, tagState);

    ActionScopes scopeMask = m_contexts.m_controllerDef->GetScopeMask(fragID, tagState);
    ActionScopes filteredScopeMask = scopeMask & m_contexts.viewData[eMEM_FragmentEditor].m_pActionController->GetActiveScopeMask();

    if (filteredScopeMask)
    {
        if (contextID >= 0)
        {
            const uint32 numScopes = m_contexts.m_scopeData.size();
            for (uint32 s = 0; s < numScopes; s++)
            {
                if (m_contexts.m_scopeData[s].contextID == contextID)
                {
                    if (m_contexts.m_scopeData[s].context[eMEM_FragmentEditor])
                    {
                        m_wndFragmentBrowser->SetScopeContext(m_contexts.m_scopeData[s].context[eMEM_FragmentEditor]->dataID);
                    }
                    break;
                }
            }
        }
        m_wndFragmentBrowser->SelectFragment(fragID, tagState);
        m_contexts.viewData[eMEM_FragmentEditor].m_pAnimContext->state = tagState.globalTags;
        m_wndFragmentEditorPage->Nodes()->SetSequence(m_wndFragmentEditorPage->TrackPanel()->GetSequence());
    }
}

void CMannequinDialog::EditTransitionByKey(const struct CFragmentKey& key, int contextID)
{
    if (key.transition && (key.tranFragFrom != FRAGMENT_ID_INVALID) && (key.tranFragTo != FRAGMENT_ID_INVALID))
    {
        TTransitionID transition(key.tranFragFrom, key.tranFragTo, key.tranTagFrom, key.tranTagTo, key.tranBlendUid);
        m_wndTransitionBrowser->SelectAndOpenRecord(transition);
    }
}

void CMannequinDialog::OnSave()
{
    m_wndFragmentEditorPage->TrackPanel()->FlushChanges();

    if (m_pFileChangeWriter->ShowFileManager() == eFMR_NoChanges)
    {
        CryMessageBox("There are no modified files", "No Changes", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        // Force a reload of the key properties to clear out the per-property "modified" flag
        m_wndFragmentEditorPage->KeyProperties()->ForceUpdate();

        // If the user clicked on the "Undo Changes to Selected Files" button
        // in the save dialog, the animdb will have been reloaded from disk.
        // The transition browser's state doesn't match what is in the animdb,
        // so refresh it to sync the state.
        m_wndTransitionBrowser->Refresh();
    }

    m_wndFragmentEditorPage->TrackPanel()->SetCurrentFragment();
}

void CMannequinDialog::OnReexportAll()
{
    m_wndFragmentEditorPage->TrackPanel()->FlushChanges();

    LoadMannequinFolder();

    if (m_pFileChangeWriter->ShowFileManager(true) == eFMR_NoChanges)
    {
        CryMessageBox("There are no modified files", "No Changes", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        // Force a reload of the key properties to clear out the per-property "modified" flag
        m_wndFragmentEditorPage->KeyProperties()->ForceUpdate();
    }

    m_wndFragmentEditorPage->TrackPanel()->SetCurrentFragment();
}

void CMannequinDialog::Validate()
{
    const uint32 numContexts = m_contexts.m_contextData.size();
    m_wndErrorReport->Clear();
    for (uint32 i = 0; i < numContexts; i++)
    {
        SScopeContextData& contextDef = m_contexts.m_contextData[i];
        Validate(contextDef);
    }

    RefreshAndActivateErrorReport();
}

void CMannequinDialog::OnNewSequence()
{
    m_wndPreviewerPage->OnNewSequence();
    showTab(m_wndPreviewerPage, m_viewPreviewer);
}

void CMannequinDialog::OnLoadSequence()
{
    m_wndPreviewerPage->OnLoadSequence();
    showTab(m_wndPreviewerPage, m_viewPreviewer);
}

void CMannequinDialog::OnSaveSequence()
{
    m_wndPreviewerPage->OnSaveSequence();
}

void CMannequinDialog::OnUpdateActions()
{
    OnUpdateNewSequence();
    OnUpdateLoadSequence();
    OnUpdateSaveSequence();
    OnUpdateViewFragmentEditor();
    OnUpdateViewPreviewer();
    OnUpdateViewTransitionEditor();
    OnUpdateViewErrorReport();
}

void CMannequinDialog::OnUpdateNewSequence()
{
    m_newSequence->setEnabled(m_bPreviewFileLoaded);
}

void CMannequinDialog::OnUpdateLoadSequence()
{
    m_loadSequence->setEnabled(m_bPreviewFileLoaded);
}

void CMannequinDialog::OnUpdateSaveSequence()
{
    m_saveSequence->setEnabled(m_bPreviewFileLoaded);
}

void CMannequinDialog::showTab(QWidget* tab, QAction* ifHidden)
{
    if (tab->isVisible())
    {
        m_central->setCurrentWidget(tab);
    }
    else
    {
        ifHidden->trigger();
    }
}

void CMannequinDialog::toggleTab(int index, QWidget* tab)
{
    if (m_central->indexOf(tab) == -1)
    {
        m_central->insertTab(index, tab, tab->windowTitle());
        m_central->setCurrentWidget(tab);
    }
    else
    {
        m_central->removeTab(m_central->indexOf(tab));
    }
}

void CMannequinDialog::OnViewFragmentEditor()
{
    toggleTab<CFragmentEditorPage*>();
}

void CMannequinDialog::OnViewPreviewer()
{
    toggleTab<CPreviewerPage*>();
}

void CMannequinDialog::OnViewTransitionEditor()
{
    toggleTab<CTransitionEditorPage*>();
}

void CMannequinDialog::OnViewErrorReport()
{
    toggleTab<CMannErrorReportDialog*>();
}

void CMannequinDialog::OnUpdateViewFragmentEditor()
{
    m_viewFragmentEditor->setCheckable(true);
    m_viewFragmentEditor->setChecked(m_central->indexOf(m_wndFragmentEditorPage) != -1);
}

void CMannequinDialog::OnUpdateViewPreviewer()
{
    m_viewPreviewer->setCheckable(true);
    m_viewPreviewer->setChecked(m_central->indexOf(m_wndPreviewerPage) != -1);
}

void CMannequinDialog::OnUpdateViewTransitionEditor()
{
    m_viewTransitionEditor->setCheckable(true);
    m_viewTransitionEditor->setChecked(m_central->indexOf(m_wndTransitionEditorPage) != -1);
}

void CMannequinDialog::OnUpdateViewErrorReport()
{
    m_viewErrorReport->setCheckable(true);
    m_viewErrorReport->setChecked(m_central->indexOf(m_wndErrorReport) != -1);
}

struct SErrorContext
{
    CMannErrorReportDialog* errorReport;
    const SScopeContextData* contextDef;
};

void OnErrorCallback(const SMannequinErrorReport& errorReport, CMannErrorRecord::ESeverity errorSeverity, void* _context)
{
    SErrorContext* errContext = (SErrorContext*) _context;

    CMannErrorRecord err;
    err.error                   = errorReport.error;
    err.type                    = errorReport.errorType;
    err.file                    = errContext->contextDef->database->GetFilename();
    err.severity            = errorSeverity;
    err.flags                   = CMannErrorRecord::FLAG_NOFILE;
    err.fragmentID      = errorReport.fragID;
    err.tagState            = errorReport.tags;
    err.fragmentIDTo    = errorReport.fragIDTo;
    err.tagStateTo      = errorReport.tagsTo;
    err.fragOptionID    = errorReport.fragOptionID;
    err.contextDef      = errContext->contextDef;
    errContext->errorReport->AddError(err);
}

void ErrorCallback(const SMannequinErrorReport& errorReport, void* _context)
{
    OnErrorCallback(errorReport, CMannErrorRecord::ESEVERITY_ERROR, _context);
}

void WarningCallback(const SMannequinErrorReport& errorReport, void* _context)
{
    OnErrorCallback(errorReport, CMannErrorRecord::ESEVERITY_WARNING, _context);
}

void CMannequinDialog::Validate(const SScopeContextData& contextDef)
{
    SErrorContext errorContext;
    errorContext.contextDef  = &contextDef;
    errorContext.errorReport = m_wndErrorReport;

    IAnimationDatabase* animDB = contextDef.database;

    if (animDB)
    {
        //--- TODO: Add in an option to perform the full warning level of the error checking
        animDB->Validate(contextDef.animSet, &ErrorCallback, NULL, &errorContext);
    }
}

bool CMannequinDialog::OpenPreview(const char* filename)
{
    LoadNewPreviewFile(Path::FullPathToGamePath(filename));
    RefreshAndActivateErrorReport();
    if (!m_wndErrorReport->HasErrors())
    {
        showTab(m_wndFragmentEditorPage, m_viewFragmentEditor);
    }

    return m_bPreviewFileLoaded;
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnDestroy()
{
    SaveLayouts();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::closeEvent(QCloseEvent* event)
{
    const bool canClose = CheckChangedData();
    event->setAccepted(canClose);

    SaveMostRecentlyUsed();
}

void CMannequinDialog::showEvent(QShowEvent* event)
{
    if (event->spontaneous() || m_initialShow)
    {
        return;
    }

    const bool restored = property("restored").toBool();

    LoadLayouts();
    LoadMostRecentlyUsed();

    if (!m_bPreviewFileLoaded && !LoadMostRecentPreview())
    {
        if (!restored)
        {
            ShowLaunchDialog();
        }

        if (!m_bPreviewFileLoaded)
        {
            QMetaObject::invokeMethod(parent(), "close", Qt::QueuedConnection);
            event->ignore();
        }
    }

    m_initialShow = true;
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnFragmentBrowserSelectedItemEdit(FragmentID fragmentId, const QString& tags)
{
    UpdateForFragment(fragmentId, tags);
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::StopEditingFragment()
{
    m_wndFragmentEditorPage->setWindowTitle(tr("Fragment Editor"));
    m_wndFragmentsRollup->setTitle(tr("No Fragment Selected"));
    m_wndTagsPanel->setEnabled(false);
    m_wndFragmentEditorPage->Reset();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::FindFragmentReferences(const QString& fragmentName)
{
    findDockWidget(m_wndTransitionBrowser)->raise();
    m_wndTransitionBrowser->FindFragmentReferences(fragmentName);

    if (!m_wndTransitionEditorPage->isVisible())
    {
        m_wndTransitionEditorPage->ClearSequence();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::FindTagReferences(const QString& tagName)
{
    QString filteredName = tagName;
    filteredName.replace('+', ' ');

    findDockWidget(m_wndTransitionBrowser)->raise();
    m_wndTransitionBrowser->FindTagReferences(filteredName);

    if (!m_wndTransitionEditorPage->isVisible())
    {
        m_wndTransitionEditorPage->ClearSequence();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::FindClipReferences(const CClipKey& key)
{
    QString icafName = key.GetFileName();
    int startIndex = icafName.lastIndexOf(QLatin1Char('/')) + 1;
    icafName.remove(0, startIndex);

    findDockWidget(m_wndTransitionBrowser)->raise();
    m_wndTransitionBrowser->FindClipReferences(icafName);

    if (!m_wndTransitionEditorPage->isVisible())
    {
        m_wndTransitionEditorPage->ClearSequence();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::UpdateForFragment(FragmentID fragmentID, const QString& tags)
{
    if (fragmentID == FRAGMENT_ID_INVALID)
    {
        StopEditingFragment();
    }
    else
    {
        const char* fragmentName = m_contexts.m_controllerDef->m_fragmentIDs.GetTagName(fragmentID);
        m_wndFragmentEditorPage->setWindowTitle(tr("Fragment Editor : %1 [%2]").arg(fragmentName).arg(tags));

        m_wndFragmentsRollup->setTitle(tr("%1 [%2]").arg(fragmentName).arg(tags));

        m_wndTagsPanel->setEnabled(true);

        m_wndFragmentEditorPage->UpdateSelectedFragment();
    }

    const int tabIndex = m_central->indexOf(m_wndFragmentEditorPage);
    if (tabIndex != -1)
    {
        m_central->setTabText(tabIndex, m_wndFragmentEditorPage->windowTitle());
    }
}

void CMannequinDialog::OnMenuNewPreview()
{
    AZStd::string generatedPreviewFileName;
    if (MannequinConfigFileHelper::CreateNewConfig(generatedPreviewFileName, this))
    {
        OpenPreview(generatedPreviewFileName.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnMenuLoadPreviewFile()
{
    const bool canDiscardChanges = CheckChangedData();
    if (!canDiscardChanges)
    {
        return;
    }

    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Mannequin Preview");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    OpenPreview(selection.GetResult()->GetFullPath().c_str());
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnContexteditor()
{
    CMannContextEditorDialog dialog(this);
    dialog.exec();

    // Brute force approach for the time being.
    SavePreviewFile(m_contexts.previewFilename);
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnAnimDBEditor()
{
    CRY_ASSERT(FragmentBrowser());
    const int scopeContextId = FragmentBrowser()->GetScopeContextId();
    if (scopeContextId < 0 || m_contexts.m_contextData.size() <= scopeContextId)
    {
        return;
    }

    IAnimationDatabase* const pAnimDB = m_contexts.m_contextData[scopeContextId].database;
    if (pAnimDB == NULL)
    {
        return;
    }

    CMannAnimDBEditorDialog dialog(pAnimDB);
    dialog.exec();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnTagDefinitionEditor()
{
    const QString filename(m_contexts.previewFilename);
    CMannTagDefEditorDialog dialog(Path::GetFile(filename));

    connect(&dialog, &CMannTagDefEditorDialog::TagRenamed, this, &CMannequinDialog::OnTagRenamed);
    connect(&dialog, &CMannTagDefEditorDialog::TagRemoved, this, &CMannequinDialog::OnTagRemoved);
    connect(&dialog, &CMannTagDefEditorDialog::TagAdded, this, &CMannequinDialog::OnTagAdded);

    connect(&dialog, &CMannTagDefEditorDialog::TagRenamed, FragmentBrowser(), &CFragmentBrowser::OnTagRenamed);
    connect(&dialog, &CMannTagDefEditorDialog::TagRemoved, FragmentBrowser(), &CFragmentBrowser::OnTagRemoved);
    connect(&dialog, &CMannTagDefEditorDialog::TagAdded, FragmentBrowser(), &CFragmentBrowser::OnTagAdded);

    connect(&dialog, &CMannTagDefEditorDialog::TagRenamed, m_wndPreviewerPage, &CPreviewerPage::OnTagRenamed);
    connect(&dialog, &CMannTagDefEditorDialog::TagRemoved, m_wndPreviewerPage, &CPreviewerPage::OnTagRemoved);
    connect(&dialog, &CMannTagDefEditorDialog::TagAdded, m_wndPreviewerPage, &CPreviewerPage::OnTagAdded);

    connect(&dialog, &CMannTagDefEditorDialog::TagRenamed, m_wndPreviewerPage->KeyProperties(), &CMannKeyPropertiesDlgFE::OnTagRenamed);
    connect(&dialog, &CMannTagDefEditorDialog::TagRemoved, m_wndPreviewerPage->KeyProperties(), &CMannKeyPropertiesDlgFE::OnTagRemoved);
    connect(&dialog, &CMannTagDefEditorDialog::TagAdded, m_wndPreviewerPage->KeyProperties(), &CMannKeyPropertiesDlgFE::OnTagAdded);

    connect(&dialog, &CMannTagDefEditorDialog::TagRenamed, m_wndTransitionEditorPage, &CTransitionEditorPage::OnTagRenamed);
    connect(&dialog, &CMannTagDefEditorDialog::TagRemoved, m_wndTransitionEditorPage, &CTransitionEditorPage::OnTagRemoved);
    connect(&dialog, &CMannTagDefEditorDialog::TagAdded, m_wndTransitionEditorPage, &CTransitionEditorPage::OnTagAdded);

    connect(&dialog, &CMannTagDefEditorDialog::TagRenamed, m_wndTransitionEditorPage->KeyProperties(), &CMannKeyPropertiesDlgFE::OnTagRenamed);
    connect(&dialog, &CMannTagDefEditorDialog::TagRemoved, m_wndTransitionEditorPage->KeyProperties(), &CMannKeyPropertiesDlgFE::OnTagRemoved);
    connect(&dialog, &CMannTagDefEditorDialog::TagAdded, m_wndTransitionEditorPage->KeyProperties(), &CMannKeyPropertiesDlgFE::OnTagAdded);

    connect(&dialog, &CMannTagDefEditorDialog::TagRenamed, m_wndFragmentEditorPage, &CFragmentEditorPage::OnTagRenamed);
    connect(&dialog, &CMannTagDefEditorDialog::TagRemoved, m_wndFragmentEditorPage, &CFragmentEditorPage::OnTagRemoved);
    connect(&dialog, &CMannTagDefEditorDialog::TagAdded, m_wndFragmentEditorPage, &CFragmentEditorPage::OnTagAdded);

    dialog.exec();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name)
{
    if (tagDef == &m_contexts.viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RenameTag(tagID, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnTagRemoved(const CTagDefinition* tagDef, TagID tagID)
{
    if (tagDef == &m_contexts.viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.RemoveTag(m_tagVars, tagID);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnTagAdded(const CTagDefinition* tagDef, const QString& name)
{
    if (tagDef == &m_contexts.viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef())
    {
        m_tagControls.AddTag(m_tagVars, name);
        RefreshTagsPanel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::ReloadContextEntities()
{
    // Brute force approach for the time being.
    LoadNewPreviewFile(m_contexts.previewFilename);
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::ClearContextEntities()
{
    // Clear context entities
    m_contexts.m_contextData.clear();
    m_contexts.m_scopeData.clear();
    m_contexts.backgroundProps.clear();
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::ClearContextViewData()
{
    for (uint32 em = 0; em < eMEM_Max; em++)
    {
        SAFE_RELEASE(m_contexts.viewData[em].m_pActionController);
        SAFE_DELETE(m_contexts.viewData[em].m_pAnimContext);

        for (uint32 i = 0; i < m_contexts.m_contextData.size(); i++)
        {
            SScopeContextData& context = m_contexts.m_contextData[i];
            SAFE_RELEASE(context.viewData[em].m_pActionController);
            SAFE_DELETE(context.viewData[em].m_pAnimContext);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::SetKeyProperty(const char* propertyName, const char* propertyValue)
{
    if (m_wndFragmentEditorPage->KeyProperties())
    {
        m_wndFragmentEditorPage->KeyProperties()->SetVarValue(propertyName, propertyValue);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::UpdateTagList(FragmentID fragID)
{
    const CTagDefinition* tagDef = (fragID != FRAGMENT_ID_INVALID) ? m_contexts.m_controllerDef->GetFragmentTagDef(fragID) : NULL;

    if (tagDef != m_lastFragTagDef)
    {
        PopulateTagList(tagDef);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::PopulateTagList(const CTagDefinition* tagDef)
{
    m_lastFragTagDef = tagDef;

    m_tagVars->DeleteAllVariables();
    m_tagControls.Init(m_tagVars, m_contexts.viewData[eMEM_FragmentEditor].m_pAnimContext->state.GetDef());

    if (tagDef)
    {
        m_fragTagControls.Init(m_tagVars, *tagDef);
    }

    RefreshTagsPanel();

    SetTagStateOnCtrl(SFragTagState());
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::RefreshTagsPanel()
{
	m_wndTagsPanel->DeleteVars();
	m_wndTagsPanel->SetVarBlock(m_tagVars.get(), functor(*this, &CMannequinDialog::OnInternalVariableChange));
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::OnInternalVariableChange(IVariable* pVar)
{
    if (!m_bRefreshingTagCtrl)
    {
        SFragTagState tagState = GetTagStateFromCtrl();

        m_wndFragmentEditorPage->TrackPanel()->FlushChanges();

        Previewer()->SetUIFromHistory();
        TransitionEditor()->SetUIFromHistory();
        FragmentBrowser()->SetTagState(tagState);

        m_wndFragmentEditorPage->UpdateSelectedFragment();

        UpdateForFragment(FragmentBrowser()->GetSelectedFragmentId(), FragmentBrowser()->GetSelectedNodeText());
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannequinDialog::SetTagStateOnCtrl(const SFragTagState& tagState)
{
    m_bRefreshingTagCtrl = true;
    m_tagControls.Set(tagState.globalTags);
    if (m_lastFragTagDef)
    {
        m_fragTagControls.Set(tagState.fragmentTags);
    }
    m_bRefreshingTagCtrl = false;
}

//////////////////////////////////////////////////////////////////////////
SFragTagState CMannequinDialog::GetTagStateFromCtrl() const
{
    SFragTagState ret;
    ret.globalTags = m_tagControls.Get();
    if (m_lastFragTagDef)
    {
        ret.fragmentTags = m_fragTagControls.Get();
    }

    return ret;
}

void CMannequinDialog::LoadMannequinFolder()
{
    // Load all files from the mannequin folder
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    // Animation databases
    IFileUtil::FileArray adbFiles;
    CFileUtil::ScanDirectory(MANNEQUIN_FOLDER, "*.adb", adbFiles);
    for (IFileUtil::FileArray::const_iterator itAdbs = adbFiles.begin(); itAdbs != adbFiles.end(); ++itAdbs)
    {
        mannequinSys.GetAnimationDatabaseManager().Load((MANNEQUIN_FOLDER + itAdbs->filename).toUtf8().data());
    }

    // XMLs: can be tag definitions or controller definition files
    IFileUtil::FileArray xmlFiles;
    CFileUtil::ScanDirectory(MANNEQUIN_FOLDER, "*.xml", xmlFiles);
    for (IFileUtil::FileArray::const_iterator itXmls = xmlFiles.begin(); itXmls != xmlFiles.end(); ++itXmls)
    {
        const CTagDefinition* pTagDef = mannequinSys.GetAnimationDatabaseManager().LoadTagDefs((MANNEQUIN_FOLDER + itXmls->filename).toUtf8().data(), true);
        if (!pTagDef)
        {
            // Failed to load as a tag definition: try as a controller definition file
            mannequinSys.GetAnimationDatabaseManager().LoadControllerDef((MANNEQUIN_FOLDER + itXmls->filename).toUtf8().data());
        }
    }
}

void CMannequinDialog::RefreshAndActivateErrorReport()
{
    if (m_wndErrorReport->HasErrors())
    {
        m_central->setCurrentWidget(m_wndErrorReport);
        m_wndErrorReport->Refresh();
    }
}

void CMannequinDialog::OnFragmentBrowserScopeContextChanged()
{
    UpdateAnimationDBEditorMenuItemEnabledState();
}

void CMannequinDialog::UpdateAnimationDBEditorMenuItemEnabledState()
{
    CRY_ASSERT(FragmentBrowser());
    const int scopeContextId = FragmentBrowser()->GetScopeContextId();

    const bool validScopeContextId = (0 <= scopeContextId || scopeContextId < m_contexts.m_contextData.size());
    const IAnimationDatabase* const pAnimDB = validScopeContextId ? m_contexts.m_contextData[scopeContextId].database : NULL;

    m_animDBEditor->setEnabled((pAnimDB != NULL));
}

void CMannequinDialog::EnableContextEditor(bool enable)
{
    m_contextEditor->setEnabled(enable);
}


namespace MannequinInfoHelper
{
    struct SAnimAssetReportData
    {
        SAnimAssetReportData()
            : szBspace(""){}
        SAnimAssetReport assetReport;
        const char* szBspace;
    };
    typedef std::vector<SAnimAssetReportData> TAnimAssetsList;
    struct SAnimAssetCollector
    {
        SAnimAssetCollector(IAnimationSet& _animSet)
            : animSet(_animSet){}
        TAnimAssetsList assetList;
        IAnimationSet& animSet;
    };

    void OnAnimAssetReport(const SAnimAssetReport& animAssetReport, void* const pCallbackData)
    {
        SAnimAssetCollector* const pCollector = static_cast<SAnimAssetCollector*>(pCallbackData);
        SAnimAssetReportData reportData;
        reportData.assetReport = animAssetReport;
        pCollector->assetList.push_back(reportData);

        // Also register sub-animations (i.e. examples within blend-spaces)
        DynArray<int> subAnimIDs;
        pCollector->animSet.GetSubAnimations(subAnimIDs, animAssetReport.animID);
        const uint32 numSubAnims = subAnimIDs.size();
        for (uint32 itSubAnims = 0; itSubAnims < numSubAnims; ++itSubAnims)
        {
            const int subAnimID = subAnimIDs[itSubAnims];
            SAnimAssetReportData subReportData;
            subReportData.szBspace              = reportData.assetReport.pAnimPath;
            subReportData.assetReport.animID    = subAnimID;
            subReportData.assetReport.pAnimName = pCollector->animSet.GetNameByAnimID(subAnimID);
            subReportData.assetReport.pAnimPath = pCollector->animSet.GetFilePathByID(subAnimID);
            pCollector->assetList.push_back(subReportData);
        }
    }

    void ListUsedAnimationsHeader(QIODevice* pFile)
    {
        assert(pFile);

        pFile->write("Animation Name,");
        pFile->write("Blend-space,");
        pFile->write("Animation Path,");
        pFile->write("Skeleton Path,");
        pFile->write("Animation Events Path,");
        pFile->write("Animation Database Path,");
        pFile->write("Fragment Name,");
        pFile->write("Fragment TagState,");
        pFile->write("Destination Fragment Name,");
        pFile->write("Destination Fragment TagState,");
        pFile->write("Option Index,");
        pFile->write("Preview File Path");
        pFile->write("\n");
    }

    void ListUsedAnimations(QIODevice* pFile, const SMannequinContexts& contexts)
    {
        assert(pFile);

        const size_t scopeContextDataCount = contexts.m_contextData.size();
        for (size_t contextDataIndex = 0; contextDataIndex < scopeContextDataCount; ++contextDataIndex)
        {
            const SScopeContextData& scopeContextData = contexts.m_contextData[contextDataIndex];
            IAnimationDatabase* pAnimationDatabase = scopeContextData.database;
            IAnimationSet* pAnimationSet = scopeContextData.animSet;
            ICharacterInstance* pCharacterInstance = scopeContextData.viewData[eMEM_FragmentEditor].charInst.get();

            if (pAnimationDatabase == NULL || pAnimationSet == NULL  || pCharacterInstance == NULL)
            {
                continue;
            }

            SAnimAssetCollector animAssetCollector(*pAnimationSet);
            pAnimationDatabase->EnumerateAnimAssets(pAnimationSet, &OnAnimAssetReport, &animAssetCollector);

            for (size_t i = 0; i < animAssetCollector.assetList.size(); ++i)
            {
                const SAnimAssetReport& animAssetReport = animAssetCollector.assetList[i].assetReport;

                pFile->write(animAssetReport.pAnimName);
                pFile->write(",");

                pFile->write(animAssetCollector.assetList[i].szBspace);
                pFile->write(",");

                pFile->write(animAssetReport.pAnimPath ? animAssetReport.pAnimPath : "");
                pFile->write(",");

                const char* const modelPath = pCharacterInstance->GetIDefaultSkeleton().GetModelFilePath();
                pFile->write(modelPath);
                pFile->write(",");

                const char* const animEventsFile = pCharacterInstance->GetModelAnimEventDatabase();
                pFile->write(animEventsFile ? animEventsFile : "");
                pFile->write(",");

                const char* const animationDatabasePath = pAnimationDatabase->GetFilename();
                pFile->write(animationDatabasePath);
                pFile->write(",");

                const char* const fragmentName = scopeContextData.database->GetFragmentDefs().GetTagName(animAssetReport.fragID);
                pFile->write(fragmentName);
                pFile->write(",");

                QString tagList;
                MannUtils::FlagsToTagList(tagList, animAssetReport.tags, animAssetReport.fragID, *contexts.m_controllerDef, "");
                pFile->write(tagList.toUtf8().data());
                pFile->write(",");

                const char* const fragmentNameTo = animAssetReport.fragIDTo != FRAGMENT_ID_INVALID ? scopeContextData.database->GetFragmentDefs().GetTagName(animAssetReport.fragIDTo) : "";
                pFile->write(fragmentNameTo);
                pFile->write(",");

                QString tagListTo;
                if (animAssetReport.fragIDTo != FRAGMENT_ID_INVALID)
                {
                    MannUtils::FlagsToTagList(tagList, animAssetReport.tagsTo, animAssetReport.fragIDTo, *contexts.m_controllerDef, "");
                }
                pFile->write(tagListTo.toUtf8().data());
                pFile->write(",");

                pFile->write(QString::number(animAssetReport.fragOptionID).toUtf8().data());
                pFile->write(",");

                pFile->write(contexts.previewFilename.c_str());

                pFile->write("\n");
            }
        }
    }
}


void CMannequinDialog::OnToolsListUsedAnimations()
{
    QString outputFilename = "MannequinUsedAnimations.csv";
    const QString saveFilters = "CSV Files (*.csv)";
    const bool userSaveOk = CFileUtil::SelectSaveFile(saveFilters, "csv", QtUtil::ToQString(Path::GetEditingGameDataFolder().c_str()), outputFilename);
    if (!userSaveOk)
    {
        return;
    }

    CWaitProgress waitProgress("Gathering list of used animations");

    QFile pFile(outputFilename);
    if (!pFile.open(QIODevice::WriteOnly))
    {
        qWarning() << Q_FUNC_INFO << "Failed to open file" << outputFilename;
        return;
    }

    // This function mutates the dialog state to gather data about all the
    // different animations.  User interaction with the dialog will make the
    // exported data incorrect, so open a modal dialog during the export.
    QMessageBox waitingMessageBox(
        QMessageBox::Information,
        "Mannequin",
        "Writing used animations...",
        QMessageBox::NoButton,
        this
    );
    // QMessageBox::NoButton is the default value for the QMessageBox
    // constructor, and Qt assumes you want a default set of buttons in this
    // case.  So we have to explicitly disable the buttons, so that the user
    // cannot dismiss the dialog.
    waitingMessageBox.setStandardButtons(QMessageBox::NoButton);
    waitingMessageBox.show();

    // Ensure the newly-created message box is visible now, since this is
    // running inside a slot callback and will block GUI updates.
    QApplication::instance()->processEvents();

    MannequinInfoHelper::ListUsedAnimationsHeader(&pFile);

    std::vector<string> previewFiles;
    const char* const filePattern = "*.xml";

    SDirectoryEnumeratorHelper dirHelper;
    dirHelper.ScanDirectoryRecursive("", "Animations/Mannequin/Preview/", filePattern, previewFiles);

    string currentPreviewFilename = m_contexts.previewFilename;

    const size_t previewFileCount = previewFiles.size();
    for (size_t previewFileIndex = 0; previewFileIndex < previewFileCount; ++previewFileIndex)
    {
        const bool continueProcessing = waitProgress.Step((previewFileIndex * 100) / previewFileCount);
        if (!continueProcessing)
        {
            break;
        }

        const string& previewFile = previewFiles[previewFileIndex];
        const bool previewFileLoadSuccess = InitialiseToPreviewFile(previewFile.c_str());
        if (previewFileLoadSuccess)
        {
            MannequinInfoHelper::ListUsedAnimations(&pFile, m_contexts);
        }
    }

    LoadNewPreviewFile(currentPreviewFilename.c_str()); // Make the last loaded preview file valid
    waitingMessageBox.close();
}


void CMannequinDialog::OnToolsListUsedAnimationsForCurrentPreview()
{
    QString outputFilename = QString::fromLatin1("MannequinUsedAnimations_%1.csv").arg(PathUtil::GetFileName(m_contexts.previewFilename).c_str());
    const QString saveFilters = "CSV Files (*.csv)";
    const bool userSaveOk = CFileUtil::SelectSaveFile(saveFilters, "csv", QtUtil::ToQString(Path::GetEditingGameDataFolder().c_str()), outputFilename);
    if (!userSaveOk)
    {
        return;
    }

    CWaitProgress waitProgress("Gathering list of used animations");

    QFile pFile(outputFilename);
    if (!pFile.open(QIODevice::WriteOnly))
    {
        return;
    }

    MannequinInfoHelper::ListUsedAnimationsHeader(&pFile);
    MannequinInfoHelper::ListUsedAnimations(&pFile, m_contexts);
}

void CMannequinDialog::OnMostRecentlyUsedEntryTriggered(QAction* action)
{
    const QVariant entry = action->data();
    Q_ASSERT(entry.type() == QVariant::String);

    const bool canOpen = CheckChangedData();
    if (!canOpen)
    {
        return;
    }

    const QString filename = entry.toString();
    const QString gamePath = Path::FullPathToGamePath(filename);
    const AZStd::string fullPath = Path::MakeModPathFromGamePath(gamePath.toUtf8().data());
    if (!QFileInfo::exists(QString::fromLatin1(fullPath.c_str()))) {
        QMessageBox::warning(this, QStringLiteral("File not found"),
            QStringLiteral("The file '%1', could not be found.").arg(filename));
        m_MostRecentlyUsedPreviews.removeAll(filename);
    }

    OpenPreview(qPrintable(filename));
}

void CMannequinDialog::PopulateMostRecentlyUsed()
{
    QMenu* menu = qobject_cast<QMenu*>(sender());
    Q_ASSERT(menu);

    menu->clear();

    if (m_MostRecentlyUsedPreviews.isEmpty())
    {
        QAction* placeholder = new QAction(tr("No Entries"), menu);
        placeholder->setEnabled(false);
        menu->addAction(placeholder);
    }
    else
    {
        for (const QString &file : m_MostRecentlyUsedPreviews)
        {
            QFileInfo finfo(file);
            QAction* entry = new QAction(finfo.fileName(), menu);
            entry->setData(file);
            menu->addAction(entry);
        }
    }
}

template<>
int CMannequinDialog::defaultTabPosition<CFragmentEditorPage*>() {
    return 0;
}
template<>
int CMannequinDialog::defaultTabPosition<CTransitionEditorPage*>() {
    return 1;
}
template<>
int CMannequinDialog::defaultTabPosition<CPreviewerPage*>() {
    return 2;
}
template<>
int CMannequinDialog::defaultTabPosition<CErrorReport*>() {
    return 3;
}
#include <Mannequin/MannequinDialog.moc>