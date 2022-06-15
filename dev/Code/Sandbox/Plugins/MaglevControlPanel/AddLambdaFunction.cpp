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

#include <AddLambdaFunction.h>

#include <IAWSResourceManager.h>

#include <QVariantMap>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidgetItem>
#include <QStringList>
#include <QMessageBox>
#include <QRegExp>

#include <ResourceParsing.h>

#include "PythonWorker.h"
#include "UI/ui_AddLambdaFunction.h"
#include "ResourceManagementView.h"


AddLambdaFunction::AddLambdaFunction(QWidget* parent, QSharedPointer<ICloudFormationTemplateModel> templateModel, ResourceManagementView* view)
    : CreateResourceDialog(parent, templateModel, view)
    , m_ui{new Ui::AddLambdaFunction()}
{
    m_ui->setupUi(this);

    QObject::connect(m_ui->label, &QLabel::linkActivated, this, &CreateResourceDialog::OnResourceHelpActivated);

    SetupResourceList();

    static const char* RESOURCE_NAME = "lambda";
    static const char* KEY_NAME = "name";
    static const char* KEY_HANDLER = "handler";
    SetValidatorOnLineEdit(m_ui->functionName, RESOURCE_NAME, KEY_NAME);
    SetValidatorOnLineEdit(m_ui->handler, RESOURCE_NAME, KEY_HANDLER);

}

bool AddLambdaFunction::TypeSpecificValidateResource(const QString& resourceName)
{
    if (m_ui->handler->text().length() < 1)
    {
        QMessageBox::critical(this, tr("Invalid field"), tr("Lambda handler name must be set"));
        return false;
    }

    return true;
}


QString AddLambdaFunction::GetResourceName() const
{
    return m_ui->functionName->text();
}

void AddLambdaFunction::BeginSaveResource()
{
    GetTemplateModel()->AddResource(GetConfigurationName(), GetConfigurationVariantMap());
    ConfigureSelectedResources();
    BeginAddFunctionCodeFolder();
}

QString AddLambdaFunction::GetConfigurationName() const
{
    return GetResourceName() + "Configuration";
}

QVariantMap AddLambdaFunction::GetVariantMap() const
{
    QVariantMap variantMap;
    QString configurationName(GetConfigurationName());

    // Type
    variantMap["Type"] = GetResourceType();

    // Metadata
    if (m_ui->invokeYes->isChecked())
    {
        QJsonObject permissionObject;
        permissionObject["Action"] = "lambda:InvokeFunction";
        permissionObject["AbstractRole"] = "Player";

        QJsonObject cloudCanvasObject;
        cloudCanvasObject["Permissions"] = permissionObject;

        QJsonObject metadataObject;
        metadataObject["CloudCanvas"] = cloudCanvasObject;

        variantMap["Metadata"] = metadataObject.toVariantMap();
    }

    // Properties
    QJsonObject propertyObject;

    // Properties - Code
    QJsonArray bucketArray;
    bucketArray.push_back(configurationName);
    bucketArray.push_back("ConfigurationBucket");

    QJsonObject bucketObject;
    bucketObject["Fn::GetAtt"] = bucketArray;

    QJsonObject codeObject;
    codeObject["S3Bucket"] = bucketObject;

    QJsonArray keyArray;
    keyArray.push_back(configurationName);
    keyArray.push_back("ConfigurationKey");

    QJsonObject keyObject;
    keyObject["Fn::GetAtt"] = keyArray;

    codeObject["S3Key"] = keyObject;

    propertyObject["Code"] = codeObject;


    // Properties - Handler
    propertyObject["Handler"] = m_ui->handler->text();

    // Properties - Role
    QJsonArray roleArray;
    roleArray.push_back(configurationName);
    roleArray.push_back("Role");

    QJsonObject roleObject;
    roleObject["Fn::GetAtt"] = roleArray;

    propertyObject["Role"] = roleObject;

    // Properties - Runtime
    QJsonArray runtimeArray;
    runtimeArray.push_back(configurationName);
    runtimeArray.push_back("Runtime");

    QJsonObject runtimeObject;
    runtimeObject["Fn::GetAtt"] = runtimeArray;

    // Properties - Runtime
    propertyObject["Runtime"] = runtimeObject;

    // Properties - Timeout
    propertyObject["Timeout"] = 10;

    variantMap["Properties"] = propertyObject.toVariantMap();
    return variantMap;
}

QVariantMap AddLambdaFunction::GetConfigurationVariantMap() const
{
    QVariantMap variantMap;
    QString configurationName(GetConfigurationName());

    // Type
    variantMap["Type"] = "Custom::LambdaConfiguration";

    // Properties
    QJsonObject propertiesObject;

    // Properties - ConfigurationBucket
    QJsonObject configBucketObject;
    configBucketObject["Ref"] = "ConfigurationBucket";
    propertiesObject["ConfigurationBucket"] = configBucketObject;

    // Properties - ConfigurationKey
    QJsonObject configKeyObject;
    configKeyObject["Ref"] = "ConfigurationKey";
    propertiesObject["ConfigurationKey"] = configKeyObject;

    // FunctionName
    propertiesObject["FunctionName"] = GetResourceName();

    // Runtime
    propertiesObject["Runtime"] = m_ui->runtime->currentText();

    // ServiceToken
    QJsonObject serviceObject;
    serviceObject["Ref"] = "ProjectResourceHandler";
    propertiesObject["ServiceToken"] = serviceObject;

    // Settings
    QJsonObject settingsObject;
    QStringList selectedList = GetSelectedResourceNames();
    for (auto thisResource : selectedList)
    {
        QJsonObject thisObject;
        thisObject["Ref"] = thisResource;
        settingsObject[thisResource] = thisObject;
    }
    propertiesObject["Settings"] = settingsObject;
    variantMap["Properties"] = propertiesObject.toVariantMap();

    return variantMap;
}

QStringList AddLambdaFunction::GetSelectedResourceNames() const
{
    QStringList returnList;
    for (int thisRow = 0; thisRow < m_ui->resourceList->count(); ++thisRow)
    {
        QListWidgetItem* thisResource = m_ui->resourceList->item(thisRow);

        if (thisResource && thisResource->checkState() == Qt::Checked)
        {
            returnList.push_back(thisResource->text());
        }
    }
    return returnList;
}

void AddLambdaFunction::ConfigureSelectedResources()
{
    QStringList selectionList = GetSelectedResourceNames();

    for (auto thisResource : selectionList)
    {
        GetTemplateModel()->AddLambdaAccess(thisResource, GetResourceName());
    }
}


void AddLambdaFunction::SetupResourceList()
{

    QVariantMap resourceMap = ResourceParsing::ParseResourceMap(GetTemplateModel()->GetContent());

    for (auto mapIter = resourceMap.constBegin(); mapIter != resourceMap.constEnd(); ++mapIter)
    {
        auto resourceName = mapIter.key();
        auto resourceDefinition = mapIter.value().toMap();
        auto typeIter = resourceDefinition.find("Type");
        if(typeIter != resourceDefinition.end())
        {
            auto type = typeIter.value().toString();
            if(AddLambdaFunction::ACCESSIBLE_TYPES.contains(type)) 
            {
                QListWidgetItem* thisItem = new QListWidgetItem(resourceName);
                thisItem->setCheckState(Qt::Unchecked);
                m_ui->resourceList->addItem(thisItem);
            }
        }
    }
}

void AddLambdaFunction::BeginAddFunctionCodeFolder()
{

    auto callback = [this](const QString& errorMessage)
    {
        if (errorMessage.isEmpty())
        {
            OnAddFunctionCodeFolderFinished();
        }
        else
        {
            QMessageBox::critical(m_view, "Error", errorMessage);
        }
    };

    QString functionName = GetResourceName();
    QString resourceGroupName = m_view->GetSelectedResourceGroupName();
    m_view->GetResourceManager()->GetProjectModel()->CreateFunctionFolder(functionName, resourceGroupName, callback);

}

void AddLambdaFunction::OnAddFunctionCodeFolderFinished()
{
    FinishSaveResource(true);
}

/*

{Game}\AWS\project-code\discovery_utils.py defines a mapping in RESOURCE_ARN_PATTERNS for each of 
these resource types. To support other resource types, an entry needs to be added to that mapping.

Note that Tools\lmbr_aws\AWSResourceManager\default-project-content\project-code\discovery-utils.py
is the original source for the discovery_utils.py file found in the game project directory.

*/

QList<QString> list = {
    "AWS::DynamoDB::Table",
    "AWS::Lambda::Function",
    "AWS::SQS::Queue",
    "AWS::SNS::Topic",
    "AWS::S3::Bucket"
};

const QSet<QString> AddLambdaFunction::ACCESSIBLE_TYPES = QSet<QString>(
    list.begin(), list.end()
);
