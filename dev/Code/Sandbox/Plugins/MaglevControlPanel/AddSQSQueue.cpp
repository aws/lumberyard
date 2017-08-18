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

#include <AddSQSQueue.h>

#include <QVariantMap>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegExp>

#include "UI/ui_AddSQSQueue.h"
#include "ResourceManagementView.h"

AddSQSQueue::AddSQSQueue(QWidget* parent, QSharedPointer<ICloudFormationTemplateModel> templateModel, ResourceManagementView* view)
    : CreateResourceDialog(parent, templateModel, view)
    , m_ui{new Ui::AddSQSQueue()}
{
    m_ui->setupUi(this);

    QObject::connect(m_ui->label, &QLabel::linkActivated, this, &CreateResourceDialog::OnResourceHelpActivated);

    static const char* RESOURCE_NAME = "sqs";
    static const char* KEY_NAME = "name";
    SetValidatorOnLineEdit(m_ui->textEdit, RESOURCE_NAME, KEY_NAME);

}

QString AddSQSQueue::GetResourceName() const
{
    return m_ui->textEdit->text();
}

QVariantMap AddSQSQueue::GetVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Type"] = GetResourceType();

    QJsonObject propertyObject;

    variantMap["Properties"] = propertyObject.toVariantMap();
    return variantMap;
}




