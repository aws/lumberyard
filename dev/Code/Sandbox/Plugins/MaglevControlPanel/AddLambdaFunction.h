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

#include <CreateResourceDialog.h>
#include <QScopedPointer>
#include <QStringList>
#include <QSet>

namespace Ui {
    class AddLambdaFunction;
}

class AddLambdaFunction
    : public CreateResourceDialog
{
public:

    AddLambdaFunction(QWidget* parent, QSharedPointer<ICloudFormationTemplateModel> templateModel, ResourceManagementView* view);
    virtual ~AddLambdaFunction() {}

    QString GetResourceType() const override { return "AWS::Lambda::Function"; }
    QString GetResourceName() const override;

protected:
    virtual bool TypeSpecificValidateResource(const QString& resourceName) override;

    QVariantMap GetVariantMap() const override;

    void BeginSaveResource() override;

private:

    QStringList AddLambdaFunction::GetSelectedResourceNames() const;
    QString GetConfigurationName() const;
    QVariantMap GetConfigurationVariantMap() const;
    void SetupResourceList();
    void ConfigureSelectedResources();
    void BeginAddFunctionCodeFolder();
    void OnAddFunctionCodeFolderFinished();

    QScopedPointer<Ui::AddLambdaFunction> m_ui;
    QStringList m_selectionList;

    static const QSet<QString> ACCESSIBLE_TYPES;
};


