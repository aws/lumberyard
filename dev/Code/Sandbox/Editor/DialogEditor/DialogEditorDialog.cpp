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

#include "DialogEditorDialog.h"
#include "DialogManager.h"

#include <IAIAction.h>
#include <IAISystem.h>
#include <IAgent.h>

#include "IViewPane.h"
#include "StringDlg.h"
#include "QtViewPaneManager.h"

#include "Util/FileUtil.h"

#include <ISourceControl.h>
#include <QtUI/ClickableLabel.h>
#include <QtUI/QCollapsibleGroupBox.h>
#include <QtUtil.h>
#include <QtUtilWin.h>

#include <QMessageBox>
#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QDebug>
#include <QPalette>
#include <QMenu>
#include <QContextMenuEvent>
#include <QStringList>

#include <QtViewPane.h>

#define DE_USE_SOURCE_CONTROL

const char* SCToName(CDialogEditorDialog::ESourceControlOp op)
{
    switch (op)
    {
    case CDialogEditorDialog::ESCM_CHECKOUT:
        return "Checkout";
    case CDialogEditorDialog::ESCM_GETLATEST:
        return "Get Latest";
    case CDialogEditorDialog::ESCM_UNDO_CHECKOUT:
        return "Undo checkout";
    case CDialogEditorDialog::ESCM_IMPORT:
        return "Import";
    default:
        return "Unknown";
    }
}

CDialogEditorDialog::CDialogEditorDialog(QWidget* parent)
    : QMainWindow(parent)
    , m_actionDock(new ActionsDock(this))
    , m_helpDock(new HelpDock(this))
    , m_descDock(new DescriptionDock(this))
    , m_model(new DialogScriptModel(this))
    , m_view(new DialogScriptView(this))
    , m_dialogManager(new CDialogManager())
    , m_scriptTreeModel(new ScriptTreeModel(m_dialogManager, this))
    , m_scriptListModel(new ScriptListModel(m_dialogManager, this))
    , m_scriptDock(new ScriptsDock(this, m_scriptTreeModel, this))
{
    setWindowTitle(DIALOG_EDITOR_NAME);
    m_scriptTreeModel->setSourceModel(m_scriptListModel);
    m_view->setModel(m_model);
    setCentralWidget(m_view);
    addDockWidget(Qt::LeftDockWidgetArea, m_scriptDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_actionDock);
    addDockWidget(Qt::TopDockWidgetArea, m_descDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_helpDock);
    resize(1150, 500);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    QToolBar* toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addToolBar(toolBar);
    m_addAction = toolBar->addAction(QIcon(":/icons/new.png"), tr("Add ScriptLine"));
    m_deleteAction = toolBar->addAction(QIcon(":/icons/delete.png"), tr("Delete ScriptLine"));
    connect(m_addAction, &QAction::triggered, this, &CDialogEditorDialog::AddScriptLine);
    connect(m_deleteAction, &QAction::triggered, this, &CDialogEditorDialog::DeleteScriptLine);
    toolBar->setMovable(false);

    connect(m_view, &DialogScriptView::helpTextChanged, m_helpDock->GetTextEdit(), &QTextEdit::setText);
    connect(m_view, &DialogScriptView::CanDeleteRowChanged, this, &CDialogEditorDialog::UpdateToolbarActions);
    connect(m_view, &DialogScriptView::CanAddRowChanged, this, &CDialogEditorDialog::UpdateToolbarActions);
    connect(m_view, &DialogScriptView::modifiedChanged, this, &CDialogEditorDialog::OnScriptLineModified);
    connect(m_descDock, &DescriptionDock::textChanged, this, &CDialogEditorDialog::OnDescChanged);

    m_dialogManager->ReloadScripts();
    m_scriptListModel->Reload();

    connect(m_scriptDock, &ScriptsDock::scriptSelected, this, &CDialogEditorDialog::OnScriptSelected);
    UpdateToolbarActions();
}

CDialogEditorDialog::~CDialogEditorDialog()
{
    GetIEditor()->UnregisterNotifyListener(this);
    SetCurrentScript(nullptr, false);
    delete m_dialogManager;
}

CDialogManager* CDialogEditorDialog::GetManager() const
{
    return m_dialogManager;
}

void CDialogEditorDialog::closeEvent(QCloseEvent* ev)
{
    SetCurrentScript(nullptr, false);
    QMainWindow::closeEvent(ev);
}

void CDialogEditorDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;
    AzToolsFramework::RegisterViewPane<CDialogEditorDialog>(DIALOG_EDITOR_NAME, LyViewPane::CategoryOther, options);
}

bool CDialogEditorDialog::DoSourceControlOp(CEditorDialogScript* script, ESourceControlOp scOp)
{
    if (!script || !GetIEditor()->IsSourceControlAvailable())
    {
        return false;
    }

    CEditorDialogScript* curScript = m_model->GetScript();
    if (curScript != script)
    {
        Warning("CCDialogEditorDialog::DoSourceControlOp: Wrong edited item.");
        assert(false);
        return false;
    }

    bool result = true;
    bool needsLoad = false;

    QString scriptId = QString::fromLatin1(script->GetID().c_str());
    QString gamePath = m_dialogManager->ScriptToFilename(scriptId);
    QString path = gamePath;

    CryLogAlways("[DialogEditor] Doing SC-Op: %s for %s", SCToName(scOp), path);

#ifdef  DE_USE_SOURCE_CONTROL
    switch (scOp)
    {
    case ESCM_IMPORT:
    case ESCM_CHECKOUT:
        result = CFileUtil::CheckoutFile(path.toUtf8().data(), this);
        break;
    case ESCM_UNDO_CHECKOUT:
        result = CFileUtil::RevertFile(path.toUtf8().data(), this);
        needsLoad = true;
        break;
    case ESCM_GETLATEST:
        result = CFileUtil::GetLatestFromSourceControl(path.toUtf8().data(), this);
        needsLoad = true;
        break;
    }
#endif

    if (result)
    {
        if (needsLoad)
        {
            string id = scriptId.toUtf8().data();
            script = m_dialogManager->LoadScript(id, true);
        }

        // ok
        m_scriptListModel->Reload(script);
        SetCurrentScript(script, false, false, true);
    }
    else
    {
        QMessageBox box(this);
        box.setText(tr("Source Control Operation Failed.\r\nCheck if Source Control Provider correctly setup and working directory is correct."));
        box.setIcon(QMessageBox::Critical);
        box.exec();
    }
    return result;
}

static QString GetSCStatusText(uint32 scStatus)
{
    if (scStatus & SCC_FILE_ATTRIBUTE_INPAK)
    {
        return QObject::tr("[In PAK]");
    }
    else if (scStatus & SCC_FILE_ATTRIBUTE_ADD)
    {
        return QObject::tr("[Marked For Add]");
    }
    else if (scStatus & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
    {
        return QObject::tr("[Checked Out]");
    }
    else if ((scStatus & SCC_FILE_ATTRIBUTE_MANAGED) && (scStatus & SCC_FILE_ATTRIBUTE_NORMAL) && !(scStatus & SCC_FILE_ATTRIBUTE_READONLY))
    {
        return QObject::tr("[Under SourceControl - Not Checked In Yet]");
    }
    else if (scStatus & SCC_FILE_ATTRIBUTE_MANAGED)
    {
        return QObject::tr("[Under SourceControl]");
    }
    else if (scStatus & SCC_FILE_ATTRIBUTE_READONLY)
    {
        return QObject::tr("[Local File - Read Only]");
    }
    else
    {
        return QObject::tr("[Local File]");
    }
}

void CDialogEditorDialog::UpdateWindowText(CEditorDialogScript* pScript, bool bTitleOnly)
{
    if (pScript)
    {
        bool bEditable = m_dialogManager->CanModify(pScript);

        if (!bTitleOnly)
        {
            QString desc = pScript->GetDecription();
            desc.replace("\\n", "\r\n");
            m_descDock->SetText(desc);
            m_descDock->widget()->setEnabled(bEditable);
        }

        QString title;
        QString status;
        QString scStatus = GetSCStatusText(m_dialogManager->GetFileAttrs(pScript));
        if (!scStatus.isEmpty())
        {
            status += "  -  ";
            status += scStatus;
        }
        title = QString("%1: %2%3%4").arg("Description", pScript->GetScriptName()).arg(m_view->IsModified() ? "  *" : QString(), status);
        m_descDock->setWindowTitle(title);
    }
    else
    {
        if (!bTitleOnly)
        {
            m_descDock->SetText(QString());
            m_descDock->widget()->setEnabled(false);
        }
        m_descDock->setWindowTitle(tr("Description"));
    }
}

void CDialogEditorDialog::OnDescChanged()
{
    m_view->SetModified(true);
}

QMenu* CDialogEditorDialog::createPopupMenu()
{
    QMenu* menu = new QMenu(this);
    menu->addAction(m_actionDock->toggleViewAction());
    menu->addAction(m_descDock->toggleViewAction());
    menu->addAction(m_scriptDock->toggleViewAction());
    menu->addAction(m_helpDock->toggleViewAction());
    return menu;
}

// Called by the editor to notify the listener about the specified event.
void CDialogEditorDialog::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    if (ev == eNotify_OnIdleUpdate)    // Sent every frame while editor is idle.
    {
        return;
    }

    switch (ev)
    {
    case eNotify_OnBeginGameMode:           // Sent when editor goes to game mode.
    case eNotify_OnBeginSimulationMode:     // Sent when simulation mode is started.
    case eNotify_OnBeginSceneSave:          // Sent when document is about to be saved.
    case eNotify_OnQuit:                    // Sent before editor quits.
    case eNotify_OnBeginNewScene:           // Sent when the document is begin to be cleared.
    case eNotify_OnBeginSceneOpen:          // Sent when document is about to be opened.
    case eNotify_OnCloseScene:              // Send when the document is about to close.
        //      if ( m_bSaveNeeded && SaveLibrary() )
        //          m_bSaveNeeded = false;
        SaveCurrent();

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
    case eNotify_OnUpdateSequencer:         // Sent when editor needs to update the CryMannequin sequencer view.
    case eNotify_OnUpdateSequencerKeys:     // Sent when editor needs to update keys in the CryMannequin track view.
    case eNotify_OnUpdateSequencerKeySelection: // Sent when CryMannequin sequencer view changes selection of keys.
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

void CDialogEditorDialog::OnScriptLineModified(bool modified)
{
    Q_UNUSED(modified);
    UpdateWindowText(m_model->GetScript(), /*titleOnly=*/ true);
}

void CDialogEditorDialog::OnScriptSelected(CEditorDialogScript* script)
{
    SetCurrentScript(script, false, true, false);
}

void CDialogEditorDialog::AddScriptLine()
{
    m_view->AddNewRow(false);
}

void CDialogEditorDialog::DeleteScriptLine()
{
    m_view->TryDeleteRow();
}

void CDialogEditorDialog::UpdateToolbarActions()
{
    m_addAction->setEnabled(m_view->CanAddRow());
    m_deleteAction->setEnabled(m_view->CanDeleteRow());
}

void CDialogEditorDialog::CreateScript()
{
    StringGroupDlg dlg(tr("New DialogScript Name"), this);
    QString preGroup = m_scriptDock->GetScriptTreeView()->GetCurrentGroup();
    if (!preGroup.isEmpty())
    {
        dlg.SetGroup(preGroup);
    }
    while (true)
    {
        if (dlg.exec() != QDialog::Accepted || dlg.GetString().isEmpty())
            return;
        QString groupName = dlg.GetGroup();
        QString itemName = dlg.GetString();
        QRegularExpression regex(QStringLiteral(R"([:./])"));
        if (groupName.contains(regex) || itemName.contains(regex))
        {
            Error(tr("Group/Name may not contain characters :./").toUtf8().data());
        }
        else
        {
            QString id;
            if (!groupName.isEmpty())
            {
                id = groupName;
                id += ".";
            }
            id += itemName;

            CEditorDialogScript* newScript = m_dialogManager->GetScript(id, false);
            if (newScript != nullptr)
            {
                Error(tr("Dialog with ID '%1' already exists.").arg(id).toUtf8().data());
            }
            else
            {
                newScript = m_dialogManager->GetScript(id, true);
                m_dialogManager->SaveScript(newScript, true);
                m_scriptListModel->Reload();
                SetCurrentScript(newScript, true);
                return;
            }
        }
    }
}

void CDialogEditorDialog::DeleteScript()
{
    CEditorDialogScript* pCurScript = m_model->GetScript();
    if (!pCurScript || !m_dialogManager->CanModify(pCurScript))
    {
        return;
    }

    QString question = QString("Are you sure you want to remove the DialogScript '%1' ?\r\n(Note: DialogScript file also be deleted from disk/source control)").arg(QtUtil::ToQString(pCurScript->GetID()));
    QMessageBox questionBox(this);
    questionBox.setText(question);
    questionBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    questionBox.setIcon(QMessageBox::Question);

    if (questionBox.exec() == QMessageBox::Yes)
    {
        QString scriptId = QtUtil::ToQString(pCurScript->GetID());
        QString fullPath = m_dialogManager->ScriptToFilename(scriptId);
        if (!CFileUtil::DeleteFromSourceControl(fullPath.toUtf8().data(), this))
        {
            QMessageBox box(this);
            box.setText(tr("Could not delete file from source control!"));
            box.setIcon(QMessageBox::Critical);
            box.exec();
        }

        m_dialogManager->DeleteScript(pCurScript, true); // delete from disk
        m_dialogManager->SyncCryActionScripts();
        ReloadDialogBrowser();
        SetCurrentScript(nullptr, false, false);
    }
}

void CDialogEditorDialog::RenameScript()
{
    CEditorDialogScript* pCurScript = m_model->GetScript();
    if (pCurScript == 0)
    {
        return;
    }
    if (!m_dialogManager->CanModify(pCurScript))
    {
        return;
    }
    // Here we save the script as it is before renaming it, so it will
    // have the latest changes preserved before renaming.
    if (!SaveCurrent())
        return;

    StringGroupDlg dlg(tr("Change DialogScript Name"), this);
    QString curName = QString::fromLatin1(pCurScript->GetID().c_str());
    int n = curName.lastIndexOf('.');
    if (n >= 0)
    {
        dlg.SetGroup(curName.left(n));
        if (n + 1<curName.length())
            dlg.SetString(curName.mid(n + 1));
    }
    else
    {
        dlg.SetString(curName);
    }

    while (true)
    {
        if (dlg.exec() != QDialog::Accepted || dlg.GetString().isEmpty())
        {
            return;
        }
        QString groupName = dlg.GetGroup();
        QString itemName = dlg.GetString();
        QRegularExpression regex(QStringLiteral(R"([:./])"));
        if (groupName.contains(regex) || itemName.contains(regex))
        {
            QMessageBox box(this);
            box.setText(tr("Group/Name may not contain characters :./"));
            box.setIcon(QMessageBox::Critical);
            box.exec();
        }
        else
        {
            QString id;
            if (groupName.isEmpty() == false)
            {
                id = groupName;
                id += ".";
            }
            id += itemName;
            if (id == curName)
            {
                return;
            }

            QString oldPath = m_dialogManager->ScriptToFilename(curName);
            uint32 scAttr = m_dialogManager->GetFileAttrs(pCurScript);
            string newName = id.toUtf8().data();

            // Rename with source control will move-delete/move-add the file, but won't
            // rename it in the Manager.
            // After the source control operation, the manager is updated.
        #if defined(DE_USE_SOURCE_CONTROL)
            if (GetIEditor()->IsSourceControlAvailable() && (scAttr & SCC_FILE_ATTRIBUTE_MANAGED))
            {
                QString newId = id;
                QString newPath = m_dialogManager->ScriptToFilename(newId);
                CryLogAlways("[DialogEditor] Renaming file in SourceControl:"
                    "\nOld: %s"
                    "\nNew: %s", oldPath, newPath);

                if (!CFileUtil::RenameFile(oldPath.toUtf8().data(), newPath.toUtf8().data()))
                {
                    QMessageBox box(this);
                    box.setText(tr("Could not rename file in Source Control."));
                    box.setIcon(QMessageBox::Critical);
                    box.exec();
                }
            }
        #endif // DE_USE_SOURCE_CONTROL

            {
                bool ok = m_dialogManager->RenameScript(pCurScript, newName, false);
                if (ok)
                {
                    QFile::remove(oldPath);
                    m_dialogManager->SyncCryActionScripts();
                    ReloadDialogBrowser();
                    SetCurrentScript(pCurScript, true, false, true);
                    m_view->SetModified(false);
                    return;
                }
                else
                {
                    QMessageBox box(this);
                    box.setText(tr("Dialog with ID '%1' already exists.").arg(id));
                    box.setIcon(QMessageBox::Critical);
                    box.exec();
                }
            }
        }
    }
}

void CDialogEditorDialog::ReloadDialogBrowser()
{
    m_scriptListModel->Reload();
}

void CDialogEditorDialog::OnLocalEdit()
{
    CEditorDialogScript* curScript = m_model->GetScript();
    if (!curScript || m_dialogManager->CanModify(curScript))
    {
        return;
    }

    m_dialogManager->SaveScript(curScript, true);
    SetCurrentScript(curScript, false, false, true);
    m_view->SetModified(false);
}

bool CDialogEditorDialog::SetCurrentScript(CEditorDialogScript* script, bool bSelectInTree, bool bSaveModified, bool bForceUpdate)
{
    if (!bForceUpdate && script == m_model->GetScript())
    {
        return true;
    }

    if (bSaveModified)
    {
        SaveCurrent();
    }

    m_descDock->SetText(script ? script->GetDecription() : QString());

    m_view->SetScript(script);
    UpdateWindowText(script, /* titleOnly=*/ false);

    bool bEditable = script != 0 ? m_dialogManager->CanModify(script) : false;
    m_view->AllowEdit(bEditable);
    m_descDock->widget()->setEnabled(bEditable);

    if (bSelectInTree)
    {
        m_scriptDock->GetScriptTreeView()->SelectScript(script);
    }
    return true;
}

QString CDialogEditorDialog::GetCurrentDescription() const
{
    return m_descDock->GetText();
}

bool CDialogEditorDialog::SaveCurrent()
{
    CEditorDialogScript* curScript = m_model->GetScript();
    if (curScript)
    {
        QString desc = GetCurrentDescription();
        desc.replace("\r", "");
        desc.replace("\n", "\\n");
        curScript->SetDescription(desc);
    }

    if (curScript && m_view->IsModified())
    {
        m_model->SaveToScript();
        // also save a backup of previous file
        if (!m_dialogManager->SaveScript(curScript, true, true))
        {
            string id = curScript->GetID();
            curScript = m_dialogManager->LoadScript(id, true);
            m_view->SetScript(curScript);
        }
        else
        {
            m_view->SetModified(false);
        }
    }
    return true;
}

ScriptsDock::ScriptsDock(CDialogEditorDialog* editor, ScriptTreeModel* scriptTreeModel, QWidget* parent)
    : AzQtComponents::StyledDockWidget(parent)
    , m_scriptView(new ScriptTreeView(editor, this))
    , m_scriptTreeModel(scriptTreeModel)
{
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setWindowTitle(tr("Dialogs"));
    setWidget(m_scriptView);
    m_scriptView->setModel(scriptTreeModel);
    connect(m_scriptView->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &ScriptsDock::OnSelectionChanged);
}

void ScriptsDock::OnSelectionChanged()
{
    auto indexes = m_scriptView->selectionModel()->selectedIndexes();
    CEditorDialogScript* script = nullptr;
    if (!indexes.isEmpty())
    {
        script = m_scriptTreeModel->GetScriptAt(indexes[0]);
    }
    emit scriptSelected(script);
}

QSize ScriptsDock::sizeHint() const
{
    return QSize(176, -1);
}

ScriptTreeView* ScriptsDock::GetScriptTreeView() const
{
    return m_scriptView;
}

ActionsDock::ActionsDock(CDialogEditorDialog* dialog, QWidget* parent)
    : AzQtComponents::StyledDockWidget(parent)
    , m_groupBox(new QCollapsibleGroupBox(this))
    , m_dialog(dialog)
{
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setWindowTitle(tr("Tasks"));

    QWidget* container = new QWidget(this);
    QPalette pal(palette());
    pal.setColor(QPalette::Background, pal.color(QPalette::Base));
    container->setAutoFillBackground(true);
    container->setPalette(pal);

    setWidget(container);

    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setMargin(10);

    QVBoxLayout* innerLayout = new QVBoxLayout();

    QLabel* newLabel = new ClickableLabel(tr("New..."), this);
    QLabel* deleteLabel = new ClickableLabel(tr("Delete..."), this);
    QLabel* renameLabel = new ClickableLabel(tr("Rename..."), this);
    connect(newLabel, &QLabel::linkActivated, m_dialog, &CDialogEditorDialog::CreateScript);
    connect(deleteLabel, &QLabel::linkActivated, m_dialog, &CDialogEditorDialog::DeleteScript);
    connect(renameLabel, &QLabel::linkActivated, m_dialog, &CDialogEditorDialog::RenameScript);

    innerLayout->addWidget(newLabel);
    innerLayout->addWidget(deleteLabel);
    innerLayout->addWidget(renameLabel);
    m_groupBox->setLayout(innerLayout);
    m_groupBox->setTitle(tr("Dialog"));

    layout->addWidget(m_groupBox);
    layout->addStretch();
}

QSize ActionsDock::sizeHint() const
{
    return QSize(176, -1);
}


class HelpTextEdit
    : public QTextEdit
{
public:
    HelpTextEdit(QWidget* parent = nullptr)
        : QTextEdit(parent) {}
    QSize sizeHint() const override { return QSize(-1, 65); }
};

HelpDock::HelpDock(QWidget* parent)
    : AzQtComponents::StyledDockWidget(parent)
    , m_textEdit(new HelpTextEdit(this))
{
    setAllowedAreas(Qt::BottomDockWidgetArea);
    setWindowTitle(tr("Dynamic Help"));
    setWidget(m_textEdit);
    m_textEdit->setEnabled(false);
    m_textEdit->setMinimumHeight(10);
    m_textEdit->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
}

QTextEdit* HelpDock::GetTextEdit() const
{
    return m_textEdit;
}

DescriptionDock::DescriptionDock(QWidget* parent)
    : AzQtComponents::StyledDockWidget(parent)
    , m_textEdit(new DescriptionTextEdit())
{
    setAllowedAreas(Qt::TopDockWidgetArea);
    setWindowTitle(tr("Description"));
    setWidget(m_textEdit);
    m_textEdit->setEnabled(false);
    connect(m_textEdit, &QTextEdit::textChanged, this, &DescriptionDock::textChanged);
}

QString DescriptionDock::GetText() const
{
    return m_textEdit->toPlainText();
}

void DescriptionDock::SetText(const QString& text)
{
    m_textEdit->blockSignals(true); // Don't set modified when initializing the text editor
    m_textEdit->setText(text);
    m_textEdit->blockSignals(false); // Don't set modified when initializing the text editor
}

ScriptListModel::ScriptListModel(CDialogManager* dialogManager, QObject* parent)
    : QAbstractListModel(parent)
    , m_dialogManager(dialogManager)
{
}

int ScriptListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant ScriptListModel::data(const QModelIndex& index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= m_items.count())
    {
        return QVariant();
    }

    auto script = m_items.at(row);
    Q_ASSERT(script != nullptr);

    switch (role)
    {
    case Qt::DisplayRole:
        return script->GetScriptName();
    case GroupPathRole:
        return script->GetGroupPath();
    case ScriptRole:
        return QVariant::fromValue<CEditorDialogScript*>(script);
    }

    return QVariant();
}

void ScriptListModel::Reload()
{
    beginResetModel();
    m_items.clear();
    const TEditorDialogScriptMap& scripts = m_dialogManager->GetAllScripts();
    for (auto it = scripts.cbegin(), e = scripts.cend(); it != e; ++it)
    {
        m_items.push_back(const_cast<CEditorDialogScript*>(it->second));
    }

    endResetModel();
}

void ScriptListModel::Reload(CEditorDialogScript* script)
{
    if (script)
    {
        int idx = m_items.indexOf(script);
        if (idx != -1)
        {
            emit dataChanged(index(idx, 0), index(idx, 0));
        }
    }
    else
    {
        Reload();
    }
}

ScriptTreeView::ScriptTreeView(CDialogEditorDialog* editor, QWidget* parent)
    : QTreeView(parent)
    , m_editor(editor)
    , m_menu(new QMenu(this))
    , m_label(new QLabel(this))
{
    setHeaderHidden(true);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_label);
    layout->setMargin(4);
    m_label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_label->setText(tr("No Dialogs yet.\nUse [Add Dialog] to create new one."));
    updateLabel();
}

void ScriptTreeView::updateLabel()
{
    m_label->setVisible(!model() || model()->rowCount() == 0);
}

QSize ScriptTreeView::sizeHint() const
{
    return QSize(195, 1);
}

static QModelIndex indexForScript(CEditorDialogScript* target, const QModelIndex& candidate, QAbstractItemModel* model)
{
    auto script = model->data(candidate, ScriptListModel::ScriptRole).value<CEditorDialogScript*>();
    if (target == script)
    {
        return candidate;
    }

    const int count = model->rowCount(candidate);

    for (int i = 0; i < count; ++i)
    {
        QModelIndex childIndex = model->index(i, 0, candidate);
        if (!childIndex.isValid())
        {
            continue;
        }

        QModelIndex result = indexForScript(target, childIndex, model);
        if (result.isValid())
        {
            return result;
        }
    }
    return {};
}

void ScriptTreeView::SelectScript(CEditorDialogScript* script)
{
    if (!model() || !script)
    {
        return;
    }

    QModelIndex index = indexForScript(script, QModelIndex(), model());
    if (index.isValid())
    {
        selectionModel()->select(index, QItemSelectionModel::Select);
        do
        {
            expand(index);
            index = index.parent();
        } while (index.isValid());
    }
}

void ScriptTreeView::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);
    if (model)
    {
        connect(model, &QAbstractItemModel::dataChanged, this, &ScriptTreeView::updateLabel, Qt::UniqueConnection);
        connect(model, &QAbstractItemModel::rowsInserted, this, &ScriptTreeView::updateLabel, Qt::UniqueConnection);
        connect(model, &QAbstractItemModel::rowsRemoved, this, &ScriptTreeView::updateLabel, Qt::UniqueConnection);
        connect(model, &QAbstractItemModel::modelReset, this, &ScriptTreeView::updateLabel, Qt::UniqueConnection);
    }
}

QString ScriptTreeView::GetCurrentGroup() const
{
    // TODO
    return {};
}

void ScriptTreeView::contextMenuEvent(QContextMenuEvent* ev)
{
    QModelIndex index = indexAt(ev->pos());
    bool isGroup = model()->hasChildren(index);
    if (!index.isValid() || isGroup)
    {
        return;
    }

    CEditorDialogScript* script = index.data(ScriptListModel::ScriptRole).value<CEditorDialogScript*>();
    if (!script)
    {
        return;
    }

    auto dialogManager = m_editor->GetManager();

    m_menu->clear();
    QAction* action = nullptr;
    if (dialogManager->CanModify(script))
    {
        action = m_menu->addAction(tr("Rename"));
        connect(action, &QAction::triggered, m_editor, &CDialogEditorDialog::RenameScript);
        action = m_menu->addAction(tr("Delete"));
        connect(action, &QAction::triggered, m_editor, &CDialogEditorDialog::DeleteScript);
        m_menu->addSeparator();
    }

    uint32 nFileAttr = dialogManager->GetFileAttrs(script, false);
    if (nFileAttr & SCC_FILE_ATTRIBUTE_INPAK)
    {
        action = m_menu->addAction(tr("Dialog In Pak (Read Only)"));
        action->setEnabled(false);
        if (nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED)
        {
            action = m_menu->addAction(tr("Get Latest Version"));//  MF_STRING, MENU_SCM_GET_LATEST, );
            connect(action, &QAction::triggered, m_editor, &CDialogEditorDialog::RenameScript);
        }
        else
        {
            action = m_menu->addAction(tr("Local Edit"));
            connect(action, &QAction::triggered, m_editor, &CDialogEditorDialog::OnLocalEdit);
        }
    }
    else
    {
        action = m_menu->addAction(tr(" Source Control"));
        action->setEnabled(false);
        if (!(nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED))
        {
            // If not managed.
            action = m_menu->addAction(tr("Add To Source Control"));
            connect(action, &QAction::triggered, [this, script]() { m_editor->DoSourceControlOp(script, CDialogEditorDialog::ESCM_IMPORT); });
        }
        else
        {
            // If managed.
            if (nFileAttr & SCC_FILE_ATTRIBUTE_READONLY)
            {
                action = m_menu->addAction(tr("Check Out"));
                connect(action, &QAction::triggered, [this, script]() { m_editor->DoSourceControlOp(script, CDialogEditorDialog::ESCM_CHECKOUT); });
            }
            if (nFileAttr & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
            {
                action = m_menu->addAction(tr("Undo Check Out"));
                connect(action, &QAction::triggered, [this, script]() { m_editor->DoSourceControlOp(script, CDialogEditorDialog::ESCM_UNDO_CHECKOUT); });
            }
            if (nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED)
            {
                action = m_menu->addAction(tr("Get Latest Version"));
                connect(action, &QAction::triggered, [this, script]() { m_editor->DoSourceControlOp(script, CDialogEditorDialog::ESCM_GETLATEST); });
            }
        }
    }
    m_menu->popup(mapToGlobal(ev->pos()));
}

ScriptTreeModel::ScriptTreeModel(CDialogManager* dialogManager, QWidget* parent)
    : AbstractGroupProxyModel(parent)
    , m_dialogManager(dialogManager)
{
}

enum FileIcon
{
    FileIcon_None = -1,
    FileIcon_CGF = 0,
    FileIcon_InPak,
    FileIcon_ReadOnly,
    FileIcon_OnDisk,
    FileIcon_Locked,
    FileIcon_CheckedOut,
    FileIcon_NoCheckout,
    FileIcon_Last // keep at end, for iteration purposes
};

QVariant ScriptTreeModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole)
    {
        if (hasChildren(index))
        {
            return QIcon(":/icons/open.png");
        }
        else
        {
            auto script = data(index, ScriptListModel::ScriptRole).value<CEditorDialogScript*>();
            if (!script)
            {
                return QVariant();
            }

            uint32 scAttr = m_dialogManager->GetFileAttrs(script, false);
            int index = -1;
            if (scAttr & SCC_FILE_ATTRIBUTE_INPAK)
            {
                index = FileIcon_InPak;
            }
            else if (scAttr & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
            {
                index = FileIcon_CheckedOut;
            }
            else if ((scAttr & SCC_FILE_ATTRIBUTE_MANAGED) && (scAttr & SCC_FILE_ATTRIBUTE_NORMAL) && !(scAttr & SCC_FILE_ATTRIBUTE_READONLY))
            {
                index = FileIcon_NoCheckout;
            }
            else if (scAttr & SCC_FILE_ATTRIBUTE_MANAGED)
            {
                index = FileIcon_Locked;
            }
            else if (scAttr & SCC_FILE_ATTRIBUTE_READONLY)
            {
                index = FileIcon_ReadOnly;
            }
            else
            {
                index = FileIcon_OnDisk;
            }

            if (index > FileIcon_None && index < FileIcon_Last)
            {
                return QIcon(QString(":/icons/tile_%1.png").arg(index));
            }
            else
            {
                return QVariant();
            }
        }
    }
    else if (role == Qt::SizeHintRole)
    {
        if (hasChildren(index))
        {
            QSize size = AbstractGroupProxyModel::data(index, role).toSize();
            size.setHeight(19);
            return size;
        }
    }

    return AbstractGroupProxyModel::data(index, role);
}

QStringList ScriptTreeModel::GroupForSourceIndex(const QModelIndex& sourceIndex) const
{
    return sourceIndex.isValid() ? sourceIndex.data(ScriptListModel::GroupPathRole).toStringList() : QStringList();
}

CEditorDialogScript* ScriptTreeModel::GetScriptAt(const QModelIndex& idx) const
{
    QModelIndex sourceIndex = mapToSource(idx);
    return sourceIndex.isValid() ? sourceIndex.data(ScriptListModel::ScriptRole).value<CEditorDialogScript*>() : nullptr;
}

#include <DialogEditor/DialogEditorDialog.moc>
