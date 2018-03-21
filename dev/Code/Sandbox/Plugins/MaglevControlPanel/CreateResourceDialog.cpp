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
#include <stdafx.h>

#include <CreateResourceDialog.h>

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QRegExp>
#include <QLineEdit>
#include <QRegExpValidator>

#include <ResourceManagementView.h>

#include <AWSResourceManager.h>
#include <FileSourceControlModel.h>

#include "CreateResourceDialog.moc"

CreateResourceDialog::CreateResourceDialog(QWidget* parent, QSharedPointer<ICloudFormationTemplateModel> templateModel, ResourceManagementView* view)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Dialog | Qt::WindowCloseButtonHint)
    , m_templateModel{templateModel}
    , m_sourceStatus{new SourceControlStatusModel}
    , m_view{view}
{
}

void CreateResourceDialog::accept()
{
    if (!ValidateResourceName(GetResourceName()))
    {
        return;
    }

    ValidateSourceSaveResource();
}

void CreateResourceDialog::ValidateSourceSaveResource()
{
    QObject::connect(&*m_sourceStatus, &IFileSourceControlModel::SourceControlStatusUpdated, this, &CreateResourceDialog::OnSourceStatusUpdated);
    AWSResourceManager::RequestUpdateSourceModel(m_sourceStatus, m_templateModel);
}

void CreateResourceDialog::OnSourceStatusUpdated()
{
    QObject::disconnect(&*m_sourceStatus, &IFileSourceControlModel::SourceControlStatusUpdated, this, 0);
    if (m_sourceStatus->FileNeedsCheckout())
    {
        QString fileName = m_templateModel->Path();
        QString checkoutString = fileName + ResourceManagementView::GetSourceCheckoutString();
        auto reply = QMessageBox::question(
                this,
                "File needs checkout",
                checkoutString,
                QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            QObject::connect(&*m_sourceStatus, &IFileSourceControlModel::SourceControlStatusUpdated, this, &CreateResourceDialog::OnSourceStatusChanged);
            AWSResourceManager::RequestEditSourceModel(m_sourceStatus, m_templateModel);
            return;
        }
        CloseDialog();
        return;
    }
    SaveAndClose();
}

void CreateResourceDialog::SaveAndClose()
{
    BeginSaveResource();
}

void CreateResourceDialog::CloseDialog()
{
    hide();
}

void CreateResourceDialog::OnSourceStatusChanged()
{
    QObject::disconnect(&*m_sourceStatus, &IFileSourceControlModel::SourceControlStatusChanged, this, 0);
    SaveAndClose();
}

void CreateResourceDialog::BeginSaveResource()
{
    bool result = SaveResource();
    FinishSaveResource(result);
}

bool CreateResourceDialog::SaveResource()
{
    return true;
}

void CreateResourceDialog::FinishSaveResource(bool result)
{

    if (result)
    {
        m_templateModel->AddResource(GetResourceName(), GetVariantMap());
        m_templateModel->AddParameters(GetParameterVariantMap());
        if (!m_templateModel->Save())
        {
            QString saveString = m_templateModel->Path() + " failed to save.  Check to be sure the file is writable.";
            auto warningBox = QMessageBox::warning(
                this,
                "Check out file",
                saveString, QMessageBox::Ok);
            result = false;
        }
    }

    if (result)
    {
        CloseDialog();
    }

}

void CreateResourceDialog::reject()
{
    CloseDialog();
}

void CreateResourceDialog::OnResourceHelpActivated(const QString& linkString)
{
    QDesktopServices::openUrl(QUrl { linkString });
}

bool CreateResourceDialog::ValidateResourceName(const QString& resourceName)
{

    if (!resourceName.length())
    {
        QMessageBox::critical(this, tr("Invalid resource name"), tr("A resource name must be provided."));
        return false;
    }

    if (m_templateModel->HasResource(resourceName))
    {
        QMessageBox::critical(this, "Invalid resource name", QString{tr("The resource group already has a resource named %1.")}.arg(resourceName));
        return false;
    }

    if (!TypeSpecificValidateResource(resourceName))
    {
        return false;
    }

    return true;
}

QRegExpValidator* CreateResourceDialog::GetValidator(const QString& regEx)
{
    QRegExp rx(regEx);
    return new QRegExpValidator(rx, this);
}

int CreateResourceDialog::SetValidatorOnLineEdit(QLineEdit* target, const QString& resourceName, const QString& fieldName)
{
    QString regex;
    QString help;
    int minLen;
    m_view->GetResourceValidationData(resourceName, fieldName, regex, help, minLen);
    target->setValidator(GetValidator(regex));
    target->setToolTip(help);
    return minLen;
}

