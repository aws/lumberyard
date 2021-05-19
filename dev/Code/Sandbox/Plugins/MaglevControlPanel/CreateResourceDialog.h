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

#include <QDialog>
#include <QString>
#include <QVariantMap>

#include <AzCore/std/smart_ptr/unique_ptr.h>

class ICloudFormationTemplateModel;
class IFileSourceControlModel;
class ResourceManagementView;
class QRegExpValidator;
class QLineEdit;

class CreateResourceDialog
    : public QDialog
{
    Q_OBJECT

public:

    CreateResourceDialog(QWidget* parent, QSharedPointer<ICloudFormationTemplateModel> templateModel, ResourceManagementView* view);
    virtual ~CreateResourceDialog() {}

    virtual QString GetResourceType() const = 0;
    virtual QString GetResourceName() const = 0;

    virtual bool ValidateResourceName(const QString& resourceName);

    virtual void accept() override;
    virtual void reject() override;
public slots:

    void OnResourceHelpActivated(const QString& linkString);

    void OnSourceStatusUpdated();
    void OnSourceStatusChanged();

protected:
    // Override to provide specific validation criteria for a resource
    // Note that the dialog box with the specific error should be raised in this function
    virtual bool TypeSpecificValidateResource(const QString& resourceName) { return true; }

    virtual QVariantMap GetVariantMap() const = 0;
    virtual QVariantMap GetParameterVariantMap() const { return {}; }

    // For async save, override BeginSaveResource to start and on completion call FinishSaveResource.
    // For sync save, override SaveResource. A true value passed to FinishSaveResource or returned 
    // from SaveResult indicates that the save was successful. 
    virtual void BeginSaveResource();
    void FinishSaveResource(bool result);
    virtual bool SaveResource();

    void ValidateSourceSaveResource();

    virtual void SaveAndClose();
    virtual void CloseDialog();

    QRegExpValidator* GetValidator(const QString& regEx);
    int SetValidatorOnLineEdit(QLineEdit* target, const QString& resourceName, const QString& fieldName);

    QSharedPointer<ICloudFormationTemplateModel>& GetTemplateModel() { return m_templateModel; }
    ResourceManagementView* m_view;

private:

    QSharedPointer<ICloudFormationTemplateModel> m_templateModel;
    QSharedPointer<IFileSourceControlModel> m_sourceStatus;
};


