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

#pragma once

#include "TextDetailWidget.h"
#include "FocusButtonWidget.h"

#include <IEditor.h>
#include <IFileUtil.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <IAWSResourceManager.h>
#include <AWSResourceManager.h>
#include <FilePathLabel.h>
#include <FileSourceControlModel.h>

#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QDebug>

class FileContentDetailWidget
    : public TextDetailWidget, private AzToolsFramework::SourceControlNotificationBus::Handler
{
    Q_OBJECT

public:

    FileContentDetailWidget(ResourceManagementView* view, QSharedPointer<IFileContentModel> fileContentModel, QWidget* parent = nullptr)
        : TextDetailWidget{view, fileContentModel->GetStackStatusModel(), parent}
        , m_fileContentModel{fileContentModel}
        , m_stackStatusModel{fileContentModel->GetStackStatusModel()}
        , m_sourceControlModel{new SourceControlStatusModel}
    {
        connectUntilDeleted(m_fileContentModel.data(), &IFileContentModel::dataChanged, this, &FileContentDetailWidget::OnFileContentModelDataChanged);
        m_textEdit->setPlainText(GetDisplayedContent());

        if (m_stackStatusModel)
        {
            m_updateStackButton = new FocusButtonWidget {};
            m_updateStackButton->setObjectName("UpdateButton");
            m_updateStackButton->setProperty("class", "Primary");
            connectUntilDeleted(m_updateStackButton, &FocusButtonWidget::FocusGained, m_view, &ResourceManagementView::OnUpdateResourceGroupButtonGainedFocus);
            connectUntilDeleted(m_updateStackButton, &FocusButtonWidget::FocusLost, m_view, &ResourceManagementView::OnUpdateResourceGroupButtonLostFocus);
            AddButton(m_updateStackButton);
            connectUntilDeleted(m_stackStatusModel.data(), &IStackStatusModel::modelReset, this, &FileContentDetailWidget::UpdateUI);
            connectUntilDeleted(m_view->GetResourceManager(), &IAWSResourceManager::OperationInProgressChanged, this, &FileContentDetailWidget::UpdateUI);
            m_uploadLambdaCodeButton = new QPushButton{};
            m_uploadLambdaCodeButton->setText("Upload function code");
            AddButton(m_uploadLambdaCodeButton);
            m_uploadLambdaCodeButton->show();
        }
    }

    void ConnectivityStateChanged(const AzToolsFramework::SourceControlState connected) override
    {
        m_sourceControlEnabled = (connected == AzToolsFramework::SourceControlState::Active);
        UpdateDeleteButton();
        UpdateSourceControlState();
    }

    void OnUploadCode()
    {
        m_view->UploadLambdaCode(m_stackStatusModel, "");
    }

    void UpdateUI()
    {
        if (m_updateStackButton)
        {
            m_updateStackButton->setText(m_stackStatusModel->GetUpdateButtonText());
            m_updateStackButton->setToolTip(m_stackStatusModel->GetUpdateButtonToolTip());
            m_updateStackButton->setDisabled(m_view->GetResourceManager()->IsOperationInProgress());
        }
    }

    void show() override
    {
        TextDetailWidget::show();

        AzToolsFramework::SourceControlState state = AzToolsFramework::SourceControlState::Disabled;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(state, &AzToolsFramework::SourceControlConnectionRequestBus::Events::GetSourceControlState);
        m_sourceControlEnabled = (state == AzToolsFramework::SourceControlState::Active);

        AzToolsFramework::SourceControlNotificationBus::Handler::BusConnect();
        UpdateSourceControlState();

        connectUntilHidden(m_textEdit, &DetailTextEditWidget::OnKeyPressed, this, &FileContentDetailWidget::OnTextEditKeyPress);

        connectUntilHidden(m_textEdit, &QTextEdit::textChanged, this, &FileContentDetailWidget::OnTextEditChanged);

        connectUntilHidden(m_view->m_saveButton, &ResourceManagementView::ToolbarButton::clicked, this, &FileContentDetailWidget::OnSaveRequested);
        connectUntilHidden(m_view->m_sourceControlButton, &ResourceManagementView::ToolbarButton::clicked, this, &FileContentDetailWidget::OnSourceControlClicked);

        connectUntilHidden(m_view->m_saveShortcut, &QAction::triggered, this, &FileContentDetailWidget::OnSaveRequested);

        connectUntilHidden(m_view->m_menuSave, &QAction::triggered, this, &FileContentDetailWidget::OnMenuSave);
        connectUntilHidden(m_view->m_menuSaveAs, &QAction::triggered, this, &FileContentDetailWidget::OnMenuSaveAs);

        m_view->m_menuSaveAs->setEnabled(true);

        connectUntilHidden(m_sourceControlModel.data(), &IFileSourceControlModel::SourceControlStatusChanged, this, &FileContentDetailWidget::OnSourceControlStatusChanged);

        if (m_updateStackButton)
        {
            connectUntilHidden(m_updateStackButton, &QPushButton::clicked, this, &FileContentDetailWidget::OnUpdateStackButtonClicked);
        }

        if (!IsContentDoNotDelete())
        {
            auto deleteButton = m_view->GetDeleteButton();
            connectUntilHidden(deleteButton, &QPushButton::clicked, this, &FileContentDetailWidget::OnDeleteRequested);
            UpdateDeleteButton();
        }

        connectUntilHidden(m_uploadLambdaCodeButton, &QPushButton::clicked, this, &FileContentDetailWidget::OnUploadCode);

        UpdateUI();
    }

    void hide() override
    {
        TextDetailWidget::hide();

        AzToolsFramework::SourceControlNotificationBus::Handler::BusDisconnect();
    }

    void Save()
    {
        DoSaveAction();
    }

    QMenu* GetTreeContextMenu() override
    {
        auto menu = new ToolTipMenu {};

        auto saveFile = menu->addAction("Save file");
        saveFile->setToolTip(m_view->m_menuSave->toolTip());
        saveFile->setEnabled(ShouldAllowSave());
        connectUntilDeleted(saveFile, &QAction::triggered, this, &FileContentDetailWidget::OnSaveRequested);

        if (!m_fileContentModel->DoNotDelete())
        {
            auto deleteFile = menu->addAction("Delete file");
            deleteFile->setToolTip(tr("Delete the file from disk."));
            connectUntilDeleted(deleteFile, &QAction::triggered, this, &FileContentDetailWidget::OnDeleteRequested);
        }

        menu->addSeparator();

        auto openFile = menu->addAction("Open in script editor");
        openFile->setToolTip(tr("Open file in the default script editor."));
        connectUntilDeleted(openFile, &QAction::triggered, this, &FileContentDetailWidget::OnOpenInScriptEditor);

        auto openPathInExplorer = menu->addAction("View in Explorer");
        openPathInExplorer->setToolTip(tr("View the file in Windows Explorer."));
        connectUntilDeleted(openPathInExplorer, &QAction::triggered, this, &FileContentDetailWidget::OnOpenLocationInExplorer);

        menu->addSeparator();

        auto copyPathToClipboard = menu->addAction("Copy to clipboard");
        copyPathToClipboard->setToolTip(tr("Copy the file's path to the clipboard."));
        connectUntilDeleted(copyPathToClipboard, &QAction::triggered, this, &FileContentDetailWidget::OnCopyPathToClipboard);

        return menu;
    }

    void OnSaveRequested()
    {
        connectUntilDeleted(&*m_sourceControlModel, &IFileSourceControlModel::SourceControlStatusUpdated, this, &FileContentDetailWidget::OnSourceStatusUpdated);
        AWSResourceManager::RequestUpdateSourceModel(m_sourceControlModel, m_fileContentModel);
    }

    void OnSourceStatusUpdated()
    {
        QObject::disconnect(&*m_sourceControlModel, &IFileSourceControlModel::SourceControlStatusUpdated, this, &FileContentDetailWidget::OnSourceStatusUpdated);
        if (FileNeedsCheckout())
        {
            ShowSourceControlCheckoutWarning();
            return;
        }

        DoSaveAction();
    }

    void OnCopyPathToClipboard()
    {
        auto clipboard = QApplication::clipboard();
        clipboard->setText(m_fileContentModel->Path());
    }

    void OnOpenLocationInExplorer()
    {
        // GetIEditor()->GetFileUtil()->ShowInExplorer doesn't handle full paths. Using openUrl instead.
        QFileInfo fileInfo {
            m_fileContentModel->Path()
        };
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
    }

    void OnOpenInScriptEditor()
    {
        GetIEditor()->GetFileUtil()->EditTextFile(m_fileContentModel->Path().toStdString().c_str(), 0, IFileUtil::FILE_TYPE_SCRIPT);
    }

    void OnDeleteRequested()
    {
        if (m_sourceControlEnabled)
        {
            if (m_sourceControlModel->IsReady())
            {
                if (m_sourceControlModel->GetFlags() & AzToolsFramework::SCF_Tracked)
                {
                    QMessageBox msg(QMessageBox::NoIcon,
                        "File under source control",
                        "Please delete the file from your source control tool.",
                        QMessageBox::Ok,
                        Q_NULLPTR,
                        Qt::Popup);
                    msg.exec();

                    return;
                }
            }
            else
            {
                QMessageBox msg(QMessageBox::NoIcon,
                    "Source control busy",
                    "Please wait until source control information has been updated for this file, then try deleting again.",
                    QMessageBox::Ok,
                    Q_NULLPTR,
                    Qt::Popup);
                msg.exec();

                return;
            }
        }

        auto reply = QMessageBox::question(
                this,
                "Delete file",
                "Delete the following file?<br><br>" + m_fileContentModel->Path(),
                QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            if (!QFile::remove(m_fileContentModel->Path()))
            {
                QMessageBox box(QMessageBox::NoIcon,
                    "Delete file",
                    "File could not be deleted. Check that it is not open by other programs, that you have sufficient permissions to delete it, and that it is not being managed by a source control system.",
                    QMessageBox::Ok,
                    Q_NULLPTR,
                    Qt::Popup);
                box.exec();
            }
            else {
                GetIEditor()->GetAWSResourceManager()->RequestDeleteFile(m_fileContentModel->Path());
            }
        }
    }
protected:
    virtual QString GetDisplayedContent() const
    {
        return m_fileContentModel->GetContent();
    }

    virtual QString GetSavedContent() const
    {
        return m_textEdit->toPlainText();
    }

    virtual QString GetHelpLabelText() const
    {
        return {};
    }

    virtual void OnTextEditChanged()
    {
        UpdateFileSaveControls();
    }

    void OnTextEditKeyPress(QKeyEvent* event)
    {
        if (event->text().length() && FileNeedsCheckout())
        {
            auto reply = QMessageBox::warning(
                    this,
                    "Check out file",
                    "This file needs to be checked out before it can be edited. Check out now?", QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes)
            {
                DoRequestEdit();
            }
        }
    }

    bool ShouldAllowSave() const
    {
        if (FileNeedsCheckout())
        {
            return false;
        }
        return ((m_textEdit && m_textEdit->document() && m_textEdit->document()->isModified()) ||
                (m_fileContentModel && m_fileContentModel->IsModified()));
    }

    bool FileNeedsCheckout() const
    {
        AzToolsFramework::SourceControlStatus sccStatus = m_sourceControlModel->GetStatus();
        unsigned int sccFlags = m_sourceControlModel->GetFlags();

        return (sccStatus == AzToolsFramework::SCS_OpSuccess && (sccFlags & AzToolsFramework::SCF_OpenByUser) == 0);
    }

    void SetSavePending(bool newValue)
    {
        m_savePending = newValue;
    }

    void ShowSaveFailedDialog()
    {
        QString failString = "Failed to save";
        if (m_fileContentModel)
        {
            failString += " " + m_fileContentModel->Path();
        }
        else
        {
            failString += " (No file)";
        }
        failString += ".  Check to be sure the file is writable.";
        auto warningBox = QMessageBox::warning(
                m_view,
                "Save Failed",
                failString, QMessageBox::Ok);
    }

    void ShowSourceControlCheckoutWarning()
    {
        auto reply = QMessageBox::warning(
                this,
                "Check out file",
                "This file needs to be checked out before it can be saved. Check out now?", QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            SetSavePending(true);
            DoRequestEdit();
        }
    }

    virtual void DoSaveAction()
    {
        if (ShouldAllowSave())
        {
            m_textEdit->document()->setModified(false);
            m_fileContentModel->setData(m_fileContentModel->ContentIndex(), GetSavedContent());
            if (!m_fileContentModel->Save())
            {
                ShowSaveFailedDialog();
            }
        }
        UpdateFileSaveControls();
    }

    void UpdateFileSaveControls()
    {
        m_textEdit->setReadOnly(FileNeedsCheckout());

        if (ShouldAllowSave())
        {
            EnableFileSaveContextControls();
        }
        else
        {
            DisableFileSaveContextControls();
        }
    }

    bool IsContentDoNotDelete() const
    {
        return m_fileContentModel->DoNotDelete();
    }

    QSharedPointer<IFileSourceControlModel> GetSourceControlModel()
    {
        return m_sourceControlModel;
    }

    void DoRequestEdit()
    {
        // Create the file if it does not already exist
        if (!GetIEditor()->GetFileUtil()->FileExists(m_fileContentModel->Path()))
        {
            m_fileContentModel->Save();
        }

        QSharedPointer<IFileSourceControlModel> sourceModel = m_sourceControlModel;
        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, m_fileContentModel->Path().toStdString().c_str(), true, [this, sourceModel](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
            {
                if (sourceModel->GetFlags() != fileInfo.m_flags || sourceModel->GetStatus() != fileInfo.m_status)
                {
                    // We should send out this new status in case anyone else is interested.
                    m_view->SendUpdatedSourceStatus(m_fileContentModel->Path());
                }
                sourceModel->SetFlags(fileInfo.m_flags);
                sourceModel->SetStatus(fileInfo.m_status);
                OnSourceControlStatusChanged();
            }
            );
    }

    QPushButton* m_uploadLambdaCodeButton{
        nullptr
    };
private:

    QSharedPointer<IFileContentModel> m_fileContentModel;
    QSharedPointer<IStackStatusModel> m_stackStatusModel;
    QSharedPointer<IFileSourceControlModel> m_sourceControlModel;
    bool m_sourceControlEnabled{ false };

    FocusButtonWidget* m_updateStackButton {
        nullptr
    };
    bool m_savePending {
        false
    };


    void OnSourceControlClicked()
    {
        if (FileNeedsCheckout())
        {
            DoRequestEdit();
        }
        OnSourceControlStatusChanged();
    }

    void OnSourceControlStatusChanged()
    {
        if (!m_sourceControlModel->IsReady())
        {
            m_view->SetSourceControlState(ResourceManagementView::SourceControlState::QUERYING);
        }

        AzToolsFramework::SourceControlStatus sourceStatus = m_sourceControlModel->GetStatus();
        unsigned int sourceFlags = m_sourceControlModel->GetFlags();

        if (sourceStatus != AzToolsFramework::SCS_OpSuccess)
        {
            m_view->SetSourceControlState(ResourceManagementView::SourceControlState::UNAVAILABLE_CHECK_OUT);
        }
        else
        {
            ResourceManagementView::SourceControlState targetState = ResourceManagementView::SourceControlState::ENABLED_CHECK_OUT;
            QString tooltipOverride;
            bool doSave = true;

            if ((sourceFlags & AzToolsFramework::SCF_Tracked) == 0)
            {
                targetState = ResourceManagementView::SourceControlState::ENABLED_ADD;
            }
            else
            {
                if (sourceFlags & AzToolsFramework::SCF_OpenByUser)
                {
                    targetState = ResourceManagementView::SourceControlState::DISABLED_CHECK_IN;

                    if (sourceFlags & AzToolsFramework::SCF_PendingAdd)
                    {
                        targetState = ResourceManagementView::SourceControlState::DISABLED_ADD;
                        doSave = false;
                    }
                    else if (sourceFlags & AzToolsFramework::SCF_PendingDelete)
                    {
                        tooltipOverride = tr("File is currently marked for delete, check in in source control to complete delete.");
                        doSave = false;
                    }
                }
            }

            m_view->SetSourceControlState(targetState, tooltipOverride);
            if (doSave)
            {
                DoSaveAction();
            }
        }

        UpdateDeleteButton();
        SetSavePending(false);
        UpdateFileSaveControls();
    }

    void UpdateSourceControlState()
    {
        if (m_sourceControlEnabled)
        {
            QSharedPointer<IFileSourceControlModel> sourceModel = m_sourceControlModel;

            using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
            SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo, m_fileContentModel->Path().toStdString().c_str(), [this, sourceModel](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
            {
                sourceModel->SetFlags(fileInfo.m_flags);
                sourceModel->SetStatus(fileInfo.m_status);
                OnSourceControlStatusChanged();
            }
            );
        }
        else
        {
            m_view->SetSourceControlState(ResourceManagementView::SourceControlState::NOT_APPLICABLE);
        }
    }

    void OnMenuSave()
    {
        DoSaveAction();
    }

    void OnMenuSaveAs()
    {
        // TODO
    }

    void DisableFileSaveContextControls()
    {
        m_view->m_menuSave->setEnabled(false);
        m_view->DisableSaveButton(tr("The file has not been modified."));
        m_view->m_saveShortcut->setEnabled(false);
    }

    void EnableFileSaveContextControls()
    {
        m_view->m_menuSave->setEnabled(true);
        m_view->EnableSaveButton(tr("Save the selected file to disk."));
        m_view->m_saveShortcut->setEnabled(true);
    }

    void OnFileContentModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        // NOTE: this signal is received even when this widget isn't visible. It
        // still needs to prompt the user to reload files changed on disk even
        // when it isn't visible.

        // File content changed, check if we are modified and need to prompt the user
        // with an option to update content - if not modified we'll just update
        if (roles.contains(Qt::DisplayRole))
        {
            OnFileContentChanged();
        }
    }

    void OnFileContentChanged()
    {
        if (m_textEdit->IsModified())
        {
            QString path = m_fileContentModel->Path();

            // TODO: shorten path?

            auto reply = QMessageBox::question(
                    this,
                    "File contents changed",
                    "The contents of the following file have been changed both on disk and in the editor. Do you want to <u><b>lose</b></u> the changes made in the editor and reload the file's new content from disk?<br><br>" + path,
                    QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::No)
            {
                return;
            }
        }
        m_textEdit->setPlainText(GetDisplayedContent());
    }

    void OnUpdateStackButtonClicked()
    {
        m_view->UpdateStack(m_stackStatusModel);
    }

    void OnStackStatusModelUpdatableStatusChanged()
    {
        m_updateStackButton->setDisabled(m_stackStatusModel->StackIsBusy());
    }

    void UpdateDeleteButton()
    {
        if (IsContentDoNotDelete())
        {
            m_view->DisableDeleteButton(tr("File cannot be deleted."));
        }
        else if (m_sourceControlEnabled && 
            m_sourceControlModel->GetStatus() == AzToolsFramework::SCS_OpSuccess && 
            m_sourceControlModel->GetFlags() & AzToolsFramework::SCF_Tracked)
        {
            m_view->DisableDeleteButton(tr("Cannot delete file while it is under source control."));
        }
        else
        {
            m_view->EnableDeleteButton(tr("Delete the selected file from disk."));
        }
    }

signals:
    void ResourceGroupUpdateButtonGainedFocus();
    void ResourceGroupUpdateButtonLostFocus();
};

