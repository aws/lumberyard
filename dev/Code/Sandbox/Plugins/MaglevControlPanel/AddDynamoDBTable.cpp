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

#include <AddDynamoDBTable.h>

#include <QComboBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QMessageBox>
#include <QRegExp>

#include "UI/ui_AddDynamoDBTable.h"
#include "ResourceManagementView.h"

AddDynamoDBTable::AddDynamoDBTable(QWidget* parent, QSharedPointer<ICloudFormationTemplateModel> templateModel, ResourceManagementView* view)
    : CreateResourceDialog(parent, templateModel, view)
    , m_ui{new Ui::AddDynamoDBTable()}
{
    m_ui->setupUi(this);

    QObject::connect(m_ui->label, &QLabel::linkActivated, this, &CreateResourceDialog::OnResourceHelpActivated);

    static const char* RESOURCE_NAME = "dynamodb";
    static const char* KEY_NAME = "name";
    static const char* KEY_HASH_NAME = "hash_name";
    static const char* KEY_RANGE_NAME = "range_name";
    m_minNameLength = SetValidatorOnLineEdit(m_ui->tableName, RESOURCE_NAME, KEY_NAME);
    SetValidatorOnLineEdit(m_ui->hashName, RESOURCE_NAME, KEY_HASH_NAME);
    SetValidatorOnLineEdit(m_ui->rangeName, RESOURCE_NAME, KEY_RANGE_NAME);
}

QString AddDynamoDBTable::GetResourceName() const
{
    return m_ui->tableName->text();
}

bool AddDynamoDBTable::TypeSpecificValidateResource(const QString& resourceName)
{
    if (resourceName.length() < m_minNameLength)
    {
        QMessageBox::critical(this, tr("Invalid resource name"), tr("DynamoDB table name must be no shorter than %n character(s).", "", m_minNameLength));
        return false;
    }

    if (m_ui->hashName->text().length() < 1)
    {
        QMessageBox::critical(this, tr("Invalid field"), tr("DynamoDB hash name attribute must be set"));
        return false;
    }

    return true;
}

QVariantMap AddDynamoDBTable::GetVariantMap() const
{
    QVariantMap variantMap;

    variantMap["Type"] = GetResourceType();

    QJsonArray attributeArray;
    QJsonArray keyArray;

    auto addElement = [&attributeArray, &keyArray](const QString& keyName, const QString& attributeType, const QString& keyType)
        {
            QJsonObject hashKeyAttr;
            hashKeyAttr["AttributeName"] = keyName;

            if (attributeType == "Numeric")
            {
                hashKeyAttr["AttributeType"] = "N";
            }
            else if (attributeType == "Binary")
            {
                hashKeyAttr["AttributeType"] = "B";
            }
            else
            {
                hashKeyAttr["AttributeType"] = "S";
            }

            attributeArray.push_back(hashKeyAttr);

            QJsonObject hashKey;
            hashKey["AttributeName"] = keyName;
            hashKey["KeyType"] = keyType;

            keyArray.push_back(hashKey);
        };

    addElement(m_ui->hashName->text(), m_ui->hashType->currentText(), "HASH");

    if (m_ui->rangeName->text().length())
    {
        addElement(m_ui->rangeName->text(), m_ui->rangeType->currentText(), "RANGE");
    }

    QJsonObject propertyObject;

    propertyObject["AttributeDefinitions"] = attributeArray;
    propertyObject["KeySchema"] = keyArray;

    QJsonObject throughputObject;
    QJsonObject readObject;
    readObject["Ref"] = GetResourceName() + "ReadCapacityUnits";
    throughputObject["ReadCapacityUnits"] = readObject;

    QJsonObject writeObject;
    writeObject["Ref"] = GetResourceName() + "WriteCapacityUnits";
    throughputObject["WriteCapacityUnits"] = writeObject;

    propertyObject["ProvisionedThroughput"] = throughputObject;

    variantMap["Properties"] = propertyObject.toVariantMap();
    return variantMap;
}

QVariantMap AddDynamoDBTable::GetParameterVariantMap() const
{
    QVariantMap variantMap;

    QJsonObject readObject;
    readObject["Default"] = "1";
    readObject["Description"] = "Number of reads per second.";
    readObject["Type"] = "Number";

    variantMap[GetResourceName() + "ReadCapacityUnits"] = readObject;

    QJsonObject writeObject;
    readObject["Default"] = "1";
    readObject["Description"] = "Number of writes per second.";
    readObject["Type"] = "Number";

    variantMap[GetResourceName() + "WriteCapacityUnits"] = readObject;

    return variantMap;
}





