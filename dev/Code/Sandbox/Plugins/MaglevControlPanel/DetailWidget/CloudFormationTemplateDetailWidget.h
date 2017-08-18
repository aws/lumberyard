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

#include "FileContentDetailWidget.h"

#include <CreateResourceDialog.h>
#include <ResourceParsing.h>

#include <AddDynamoDBTable.h>
#include <AddLambdaFunction.h>
#include <AddSQSQueue.h>
#include <AddSNSTopic.h>
#include <AddS3Bucket.h>

#include "UI/ui_AddDynamoDBTable.h"
#include "UI/ui_AddLambdaFunction.h"
#include "UI/ui_AddS3Bucket.h"
#include "UI/ui_AddSQSQueue.h"
#include "UI/ui_AddSNSTopic.h"

class CloudFormationTemplateDetailWidget
    : public FileContentDetailWidget
{
public:

    CloudFormationTemplateDetailWidget(ResourceManagementView* view, QSharedPointer<ICloudFormationTemplateModel> templateModel)
        : FileContentDetailWidget{view, templateModel}
        , m_templateModel{templateModel}
    {
        m_helpLabel->setHidden(false);
    }

    void show() override
    {
        FileContentDetailWidget::show();
        connectUntilHidden(m_view->m_validateTemplateButton, &ResourceManagementView::ToolbarButton::clicked, this, &CloudFormationTemplateDetailWidget::OnValidateTemplateClicked);
        m_helpLabel->setText(GetHelpLabelText());
        m_helpLabel->setWordWrap(true);
        connectUntilHidden(m_helpLabel, &QLabel::linkActivated, m_view, &ResourceManagementView::OnResourceHelpActivated);

        if(m_templateModel->CanAddResources())
        {
            m_view->SetAddResourceMenuTemplateModel(m_templateModel);
        }
            
        if (m_uploadLambdaCodeButton) {
            m_uploadLambdaCodeButton->hide();
        }
    }

    virtual void hide() override
    {
        FileContentDetailWidget::hide();
        UpdateModifiedContent();
    }


    QMenu* GetTreeContextMenu() override
    {
        auto menu = new ToolTipMenu {};

        if (m_templateModel->CanAddResources())
        {
            auto addResource = m_view->CreateAddResourceMenu(m_templateModel, this);
            addResource->setTitle("Add resource");
            menu->addMenu(addResource);
        }

        auto stackStatusModel = m_templateModel->GetStackStatusModel();
        if (stackStatusModel)
        {
            auto updateStack = menu->addAction(stackStatusModel->GetUpdateButtonText());
            updateStack->setToolTip(stackStatusModel->GetUpdateButtonToolTip());
            updateStack->setDisabled(stackStatusModel->StackIsBusy());
            connectUntilDeleted(updateStack, &QAction::triggered, this, &CloudFormationTemplateDetailWidget::OnUpdateStack);
        }

        menu->addSeparator();

        auto saveFile = menu->addAction("Save file");
        saveFile->setToolTip(m_view->m_menuSave->toolTip());
        saveFile->setEnabled(ShouldAllowSave());
        connectUntilDeleted(saveFile, &QAction::triggered, this, &FileContentDetailWidget::OnSaveRequested);

        if (!IsContentDoNotDelete())
        {
            auto deleteFile = menu->addAction("Delete file");
            deleteFile->setToolTip(tr("Delete the file from disk."));
            connectUntilDeleted(deleteFile, &QAction::triggered, this, &FileContentDetailWidget::OnDeleteRequested);
        }

        auto openFile = menu->addAction("Open in script editor");
        openFile->setToolTip(tr("Open file in the default script editor."));
        connectUntilDeleted(openFile, &QAction::triggered, this, &FileContentDetailWidget::OnOpenInScriptEditor);

        auto openPathInExplorer = menu->addAction("View in Explorer");
        openPathInExplorer->setToolTip(tr("View the file in Windows Explorer."));
        connectUntilDeleted(openPathInExplorer, &QAction::triggered, this, &FileContentDetailWidget::OnOpenLocationInExplorer);

        menu->addSeparator();

        auto copyPathToClipboard = menu->addAction("Copy path to clipboard");
        copyPathToClipboard->setToolTip(tr("Copy the file's path to the clipboard."));
        connectUntilDeleted(copyPathToClipboard, &QAction::triggered, this, &FileContentDetailWidget::OnCopyPathToClipboard);

        return menu;
    }
protected:

    void OnUpdateStack()
    {
        m_view->UpdateStack(m_templateModel->GetStackStatusModel());
    }

    virtual void UpdateModifiedContent()
    {
        if (m_textEdit && m_textEdit->IsModified())
        {
            // We have unsaved changes we need to push to the model to reflect
            m_templateModel->SetContent(m_textEdit->toPlainText());
        }
    }

    QSharedPointer<ICloudFormationTemplateModel> GetTemplateModel() const { return m_templateModel; }

    virtual QString GetHelpLabelText() const
    {
        return "Learn more about the syntax used to add and edit AWS resources to your game in the <a style='color:#4285F4; text-decoration:none;' href='https://aws.amazon.com/cloudformation/'>AWS CloudFormation documentation</a>.";
    }
private:

    QSharedPointer<ICloudFormationTemplateModel> m_templateModel;

    void OnValidateTemplateClicked()
    {
        // TODO
    }
};
