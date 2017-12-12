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
#include "MannNewContextDialog.h"
#include "MannequinDialog.h"
#include "QtUtil.h"
#include "QtUtilWin.h"
#include <Mannequin/ui_MannNewContextDialog.h>

#include <ICryMannequin.h>
#include <IGameFramework.h>
#include <ICryMannequinEditor.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QDoubleValidator>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

namespace
{
    const char* noDB = "<no animation database (adb)>";
    const char* noControllerDef = "<no controller def>";
};

//////////////////////////////////////////////////////////////////////////
CMannNewContextDialog::CMannNewContextDialog(SScopeContextData* context, const EContextDialogModes mode, QWidget* parent)
    : QDialog(parent)
    , m_mode(mode)
    , ui(new Ui::MannNewContextDialog)
{
    ui->setupUi(this);
    ui->m_tagsPanel->Setup();
    switch (m_mode)
    {
    case eContextDialog_New:
        m_pData = new SScopeContextData;
        break;
    case eContextDialog_Edit:
        assert(context);
        m_pData = context;
        break;
    case eContextDialog_CloneAndEdit:
        assert(context);
        m_pData = new SScopeContextData;
        CloneScopeContextData(context);
        break;
    }

    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CMannNewContextDialog::OnAcceptButton);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CMannNewContextDialog::reject);
    connect(ui->m_browseModelButton, &QAbstractButton::clicked, this, &CMannNewContextDialog::OnBrowsemodelButton);
    connect(ui->m_newADBButton, &QAbstractButton::clicked, this, &CMannNewContextDialog::OnNewAdbButton);

    ui->m_startPosXEdit->setValidator(new QDoubleValidator(-1e10, 1e10, 6, ui->m_startPosXEdit));
    ui->m_startPosYEdit->setValidator(new QDoubleValidator(-1e10, 1e10, 6, ui->m_startPosYEdit));
    ui->m_startPosZEdit->setValidator(new QDoubleValidator(-1e10, 1e10, 6, ui->m_startPosZEdit));
    ui->m_startRotXEdit->setValidator(new QDoubleValidator(-1000.0, 1000.0, 3, ui->m_startRotXEdit));
    ui->m_startRotYEdit->setValidator(new QDoubleValidator(-1000.0, 1000.0, 3, ui->m_startRotYEdit));
    ui->m_startRotZEdit->setValidator(new QDoubleValidator(-1000.0, 1000.0, 3, ui->m_startRotZEdit));

    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CMannNewContextDialog::~CMannNewContextDialog()
{
    if (m_mode == eContextDialog_Edit)
    {
        m_pData = nullptr;
    }
    else
    {
        SAFE_DELETE(m_pData);   // ?? not sure if i want to be doing this, revist after adding new contexts to the dialog
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannNewContextDialog::OnInitDialog()
{
    m_tagVars.reset(new CVarBlock());
    ui->m_tagsPanel->setFixedSize(325, 320);
    PopulateControls();

    switch (m_mode)
    {
    case eContextDialog_New:
        setWindowTitle(tr("New Context"));
        break;
    case eContextDialog_Edit:
        setWindowTitle(tr("Edit Context"));
        break;
    case eContextDialog_CloneAndEdit:
        setWindowTitle(tr("Clone Context"));
        break;
    }

    ui->m_nameEdit->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CMannNewContextDialog::CloneScopeContextData(const SScopeContextData* context)
{
    // Don't ever start active by default because there should only be one default (i.e. startActive) option
    // If there's an already someone marked as startActive then you won't be able to switch to them.
    m_pData->startActive = false;//context->startActive;
    m_pData->name = "";//context->name;

    // fill in per detail values now
    m_pData->contextID = context->contextID;
    m_pData->fragTags = context->fragTags;

    for (int i = 0; i < eMEM_Max; i++)
    {
        if (context->viewData[i].charInst)
        {
            m_pData->viewData[i].charInst = context->viewData[i].charInst.get();
        }
        else if (context->viewData[i].pStatObj)
        {
            m_pData->viewData[i].pStatObj = context->viewData[i].pStatObj;
        }
        m_pData->viewData[i].enabled = false;
    }

    m_pData->database = context->database;
    m_pData->attachmentContext = context->attachmentContext;
    m_pData->attachment = context->attachment;
    m_pData->attachmentHelper = context->attachmentHelper;
    m_pData->startLocation = context->startLocation;
    m_pData->startRotationEuler = context->startRotationEuler;
    m_pData->pControllerDef = context->pControllerDef;
    m_pData->pSlaveDatabase = context->pSlaveDatabase;

    m_pData->tags = context->tags;
}

//////////////////////////////////////////////////////////////////////////
void CMannNewContextDialog::PopulateControls()
{
    // Setup the combobox lookup values
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const SControllerDef* pControllerDef = pContexts->m_controllerDef;
    if (pControllerDef == NULL)
    {
        return;
    }

    // Context combos
    for (int i = 0; i < pControllerDef->m_scopeContexts.GetNum(); i++)
    {
        ui->m_contextCombo->addItem(pControllerDef->m_scopeContexts.GetTagName(i));
        ui->m_attachmentContextCombo->addItem(pControllerDef->m_scopeContexts.GetTagName(i));
    }

    // additional string that will be ignored by the validation checking in ::OnOK() function
    const char* noAttachmentContext = "<no attachment context>";
    ui->m_attachmentContextCombo->addItem(noAttachmentContext);

    PopulateDatabaseCombo();
    PopulateControllerDefCombo();

    // fill in per detail values now
    ui->m_startActiveCheckBox->setChecked(m_pData->startActive);
    ui->m_nameEdit->setText(m_pData->name);
    ui->m_contextCombo->setCurrentText(m_pData->contextID == CONTEXT_DATA_NONE ? "" : pControllerDef->m_scopeContexts.GetTagName(m_pData->contextID));
    ui->m_fragTagsEdit->setText(QtUtil::ToQString(m_pData->fragTags));

    if (m_pData->viewData[eMEM_FragmentEditor].charInst)
    {
        ui->m_modelEdit->setText(m_pData->viewData[eMEM_FragmentEditor].charInst->GetFilePath());
    }
    else if (m_pData->viewData[eMEM_FragmentEditor].pStatObj)
    {
        ui->m_modelEdit->setText(m_pData->viewData[eMEM_FragmentEditor].pStatObj->GetFilePath());
    }

    if (m_pData->database)
    {
        ui->m_databaseCombo->setCurrentText(m_pData->database->GetFilename());
    }
    else
    {
        ui->m_databaseCombo->setCurrentText(noDB);
    }

    if (m_pData->attachmentContext.length() > 0)
    {
        ui->m_attachmentContextCombo->setCurrentText(QtUtil::ToQString(m_pData->attachmentContext));
    }
    else
    {
        ui->m_attachmentContextCombo->setCurrentText(noAttachmentContext);
    }

    ui->m_attachmentEdit->setText(m_pData->attachment.c_str());
    ui->m_attachmentHelperEdit->setText(m_pData->attachmentHelper.c_str());
    ui->m_startPosXEdit->setText(QtUtil::ToQString(ToString(m_pData->startLocation.t.x)));
    ui->m_startPosYEdit->setText(QtUtil::ToQString(ToString(m_pData->startLocation.t.y)));
    ui->m_startPosZEdit->setText(QtUtil::ToQString(ToString(m_pData->startLocation.t.z)));
    ui->m_startRotXEdit->setText(QtUtil::ToQString(ToString(m_pData->startRotationEuler.x)));
    ui->m_startRotYEdit->setText(QtUtil::ToQString(ToString(m_pData->startRotationEuler.y)));
    ui->m_startRotZEdit->setText(QtUtil::ToQString(ToString(m_pData->startRotationEuler.z)));

    if (m_pData->pSlaveDatabase)
    {
        ui->m_slaveDatabaseCombo->setCurrentText(m_pData->pSlaveDatabase->GetFilename());
    }
    else
    {
        ui->m_slaveDatabaseCombo->setCurrentText(noDB);
    }

    if (m_pData->pControllerDef)
    {
        ui->m_slaveControllerDefCombo->setCurrentText(m_pData->pControllerDef->m_filename.c_str());
    }
    else
    {
        ui->m_slaveControllerDefCombo->setCurrentText(noControllerDef);
    }

    // Tags
    m_tagVars->DeleteAllVariables();
    m_tagControls.Init(m_tagVars, pControllerDef->m_tags);
    m_tagControls.Set(m_pData->tags);

    ui->m_tagsPanel->DeleteVars();
    ui->m_tagsPanel->SetVarBlock(m_tagVars.get(), nullptr);
    ui->m_tagsPanel->ExpandAll();
}

void CMannNewContextDialog::accept()
{
    // Setup the combobox lookup values
    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
    SMannequinContexts* pContexts = pMannequinDialog->Contexts();
    const SControllerDef* pControllerDef = pContexts->m_controllerDef;

    // some of m_pData members are of type string (CryString<char>) so need to be retrieved via a temporary
    QString tempString;

    // retrieve per-detail values now
    m_pData->startActive = ui->m_startActiveCheckBox->isChecked();
    m_pData->name = ui->m_nameEdit->text();

    m_pData->contextID = ui->m_contextCombo->currentIndex();
    if (m_pData->contextID >= pControllerDef->m_scopeContexts.GetNum())
    {
        m_pData->contextID = CONTEXT_DATA_NONE;
    }
    else
    {
        tempString = ui->m_contextCombo->itemText(m_pData->contextID);
        const char* idname = pControllerDef->m_scopeContexts.GetTagName(m_pData->contextID);
        assert(tempString == idname);
    }

    m_pData->tags = m_tagControls.Get();

    tempString = ui->m_fragTagsEdit->text();
    m_pData->fragTags = QtUtil::ToString(tempString);

    // lookup the model view data we've chosen
    tempString = ui->m_modelEdit->text();
    if (!tempString.isEmpty())
    {
        const QString ext = QFileInfo(tempString).completeSuffix().toLower();
        const bool isCharInst = (ext == "chr") || (ext == "cdf") || (ext == "cga");

        if (isCharInst)
        {
            if (!m_pData->CreateCharInstance(tempString.toLocal8Bit().data()))
            {
                const QString strBuffer = tr("[CMannNewContextDialog::OnOk] Invalid file name %1, couldn't find character instance").arg(tempString);

                QMessageBox::warning(this, tr("Missing file"), strBuffer);
                return;
            }
        }
        else
        {
            if (!m_pData->CreateStatObj(tempString.toLocal8Bit().data()))
            {
                const QString strBuffer = tr("[CMannNewContextDialog::OnOk] Invalid file name %1, couldn't find stat obj").arg(tempString);

                QMessageBox::warning(this, tr("Missing file"), strBuffer);
                return;
            }
        }
    }

    // need to dig around to get to the (animation) databases
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManager = mannequinSys.GetMannequinEditorManager();
    DynArray<const IAnimationDatabase*> databases;
    pManager->GetLoadedDatabases(databases);
    const size_t dbSize = databases.size();
    const int dbSel = ui->m_databaseCombo->currentIndex();
    if (dbSel >= 0 && dbSel < dbSize)
    {
        m_pData->database = const_cast<IAnimationDatabase*>(databases[dbSel]);
    }
    else
    {
        m_pData->database = NULL;
    }

    const int attCont = ui->m_attachmentContextCombo->currentIndex();
    // NB: m_scopeContexts.GetNum() is poorly named but it means the number of tags within a scope context.
    if (attCont >= 0 && attCont < pControllerDef->m_scopeContexts.GetNum())
    {
        m_pData->attachmentContext = pControllerDef->m_scopeContexts.GetTagName(attCont);
    }
    else
    {
        m_pData->attachmentContext = "";
    }

    tempString = ui->m_attachmentEdit->text();
    m_pData->attachment = QtUtil::ToString(tempString);

    tempString = ui->m_attachmentHelperEdit->text();
    m_pData->attachmentHelper = QtUtil::ToString(tempString);

    m_pData->startLocation.t.x = ui->m_startPosXEdit->text().toDouble();
    m_pData->startLocation.t.y = ui->m_startPosYEdit->text().toDouble();
    m_pData->startLocation.t.z = ui->m_startPosZEdit->text().toDouble();

    m_pData->startRotationEuler.x = ui->m_startRotXEdit->text().toDouble();
    m_pData->startRotationEuler.y = ui->m_startRotYEdit->text().toDouble();
    m_pData->startRotationEuler.z = ui->m_startRotZEdit->text().toDouble();

    const int dbSlaveSel = ui->m_slaveDatabaseCombo->currentIndex();
    if (dbSlaveSel >= 0 && dbSlaveSel < dbSize)
    {
        m_pData->pSlaveDatabase = const_cast<IAnimationDatabase*>(databases[dbSlaveSel]);
    }
    else
    {
        m_pData->pSlaveDatabase = NULL;
    }

    DynArray<const SControllerDef*> controllerDefs;
    pManager->GetLoadedControllerDefs(controllerDefs);
    const size_t dbContDefSize = controllerDefs.size();
    const int dbSlaveContDefSel = ui->m_slaveControllerDefCombo->currentIndex();
    if (dbSlaveContDefSel >= 0 && dbSlaveContDefSel < dbContDefSize)
    {
        m_pData->pControllerDef = controllerDefs[dbSlaveContDefSel];
    }
    else
    {
        m_pData->pControllerDef = NULL;
    }

    if (m_pData->database)
    {
        pMannequinDialog->Validate(*m_pData);
    }

    if (m_mode != eContextDialog_Edit)
    {
        // dataID is initialised sequentially in MannequinDialog::LoadPreviewFile,
        // however deletions might have left gaps so I need to find the highest index
        // and add the ID after that.
        int newDataID = 0;
        for (int i = 0; i < pContexts->m_contextData.size(); i++)
        {
            const int currDataID = pContexts->m_contextData[i].dataID;
            if (newDataID <= currDataID)
            {
                newDataID = currDataID + 1;
            }
        }
        m_pData->dataID = newDataID;

        pContexts->m_contextData.push_back(*m_pData);
    }

    // if everything is ok with the data entered.
    QDialog::accept();
}

void CMannNewContextDialog::OnBrowsemodelButton()
{
    // Find a geometry type file for the model
    QString file = ui->m_modelEdit->text();
    // cdf, cga, chr are characters == viewData[0].charInst
    // cgf are static geometry      == viewData[0].pStatObj
    AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        ui->m_modelEdit->setText(QString(Path::FullPathToGamePath(selection.GetResult()->GetFullPath().c_str())));
    }
}

void CMannNewContextDialog::OnAcceptButton()
{
    // A name is required.
    if (ui->m_nameEdit->text().isEmpty())
    {
        QMessageBox::warning(this, tr("No Name Specified"), tr("A name for the context must be provided."));
        return;
    }

    // A character context must be selected.
    const int currentIndex = ui->m_contextCombo->currentIndex();
    const QString selectedContext = currentIndex == -1 ? QString() : ui->m_contextCombo->itemText(currentIndex);
    if (selectedContext.isEmpty())
    {
        QMessageBox::warning(this, tr("No Context Selected"), tr("A parent context must be selected."));
        return;
    }

    accept();
}

void CMannNewContextDialog::PopulateDatabaseCombo()
{
    // Populate database combo
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManager = mannequinSys.GetMannequinEditorManager();
    DynArray<const IAnimationDatabase*> databases;
    pManager->GetLoadedDatabases(databases);
    ui->m_databaseCombo->clear();
    ui->m_slaveDatabaseCombo->clear();
    const int numEntries = ui->m_databaseCombo->count();
    assert(numEntries == 0);
    for (int i = 0; i < databases.size(); i++)
    {
        ui->m_databaseCombo->addItem(databases[i]->GetFilename());
        ui->m_slaveDatabaseCombo->addItem(databases[i]->GetFilename());
    }
    // additional string that will be ignored by the validation checking in ::OnOK() function
    ui->m_databaseCombo->addItem(noDB);
    ui->m_slaveDatabaseCombo->addItem(noDB);
}

void CMannNewContextDialog::PopulateControllerDefCombo()
{
    // Populate controller def combo
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManager = mannequinSys.GetMannequinEditorManager();
    DynArray<const SControllerDef*> controllerDefs;
    pManager->GetLoadedControllerDefs(controllerDefs);
    ui->m_slaveControllerDefCombo->clear();
    const int numEntries = ui->m_slaveControllerDefCombo->count();
    assert(numEntries == 0);
    for (int i = 0; i < controllerDefs.size(); i++)
    {
        ui->m_slaveControllerDefCombo->addItem(controllerDefs[i]->m_filename.c_str());
    }
    // additional string that will be ignored by the validation checking in ::OnOK() function
    ui->m_slaveControllerDefCombo->addItem(noControllerDef);
}

void CMannNewContextDialog::OnNewAdbButton()
{
    // TODO: Add your control notification handler code here
    // Setup the combobox lookup values
    const SControllerDef* pControllerDef = CMannequinDialog::GetCurrentInstance()->Contexts()->m_controllerDef;
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    const QString adbNameBuffer = QInputDialog::getText(this, tr("New Database"), tr("New Database Name"));
    if (adbNameBuffer.isEmpty())
    {
        return;
    }

    // create new database name
    string adbFilenameBuffer("Animations/Mannequin/ADB/");
    // add the filename+extension onto the path.
    adbFilenameBuffer += QtUtil::ToString(adbNameBuffer);
    adbFilenameBuffer += ".adb";

    adbFilenameBuffer = PathUtil::MakeGamePath(adbFilenameBuffer);

    // add the new one, re-populate the database combo box...
    const IAnimationDatabase* pADB = mannequinSys.GetAnimationDatabaseManager().Load(adbFilenameBuffer);
    if (!pADB)
    {
        pADB = mannequinSys.GetAnimationDatabaseManager().Create(adbFilenameBuffer, pControllerDef->m_filename.c_str());
    }

    if (!pADB)
    {
        return;
    }

    PopulateDatabaseCombo();
    // ...and select it in the list
    ui->m_databaseCombo->setCurrentText(pADB->GetFilename());
}
