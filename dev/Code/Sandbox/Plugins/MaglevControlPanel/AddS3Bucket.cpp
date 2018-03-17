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

#include <AddS3Bucket.h>

#include <QVariantMap>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegExp>


#include "UI/ui_AddS3Bucket.h"
#include "ResourceManagementView.h"

AddS3Bucket::AddS3Bucket(QWidget* parent, QSharedPointer<ICloudFormationTemplateModel> templateModel, ResourceManagementView* view)
    : CreateResourceDialog(parent, templateModel, view)
    , m_ui{new Ui::AddS3Bucket()}
{
    m_ui->setupUi(this);

    QObject::connect(m_ui->label, &QLabel::linkActivated, this, &CreateResourceDialog::OnResourceHelpActivated);

    static const char* RESOURCE_NAME = "s3";
    static const char* KEY_NAME = "name";
    SetValidatorOnLineEdit(m_ui->textEdit, RESOURCE_NAME, KEY_NAME);
}

QString AddS3Bucket::GetResourceName() const
{
    return m_ui->textEdit->text();
}

bool AddS3Bucket::TypeSpecificValidateResource(const QString& resourceName)
{
    // note that QValidator can't handle this complexity in its regexp, so it needs to be in post accept validation
    if (!resourceName.at(0).isLower())
    {
        QMessageBox::critical(this, tr("Invalid resource name"), tr("S3 bucket name must begin with a lowercase a-z"));
        return false;
    }
    if (resourceName.contains("-"))
    {
        QMessageBox::critical(this, tr("Invalid resource name"), tr("S3 bucket name cannot contain '-'"));
        return false;
    }
    const QChar last = resourceName.at(resourceName.size() - 1);
    if (!last.isLower() && !last.isDigit())
    {
        QMessageBox::critical(this, tr("Invalid resource name"), tr("S3 bucket name must end with a lowercase a-z or digit"));
        return false;
    }
    return true;
}

QVariantMap AddS3Bucket::GetVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Type"] = GetResourceType();

    QJsonObject propertyObject;

    variantMap["Properties"] = propertyObject.toVariantMap();
    return variantMap;
}




