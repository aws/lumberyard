/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#include "stdafx.h"

#include <QCheckBox>

#include <AWSImporterModel.h>

#include <AWSImporterModel.moc>

AWSImporterModel::AWSImporterModel(AWSResourceManager* resourceManager)
    : m_resourceManager(resourceManager)
    , m_requestId{ resourceManager->AllocateRequestId() }
{
    setHorizontalHeaderItem(CheckableResourceColumn, new QStandardItem{ "Resource" });
    //ResourceColumn is used to show the resource without check box in the configuration window
    setHorizontalHeaderItem(ResourceColumn, new QStandardItem{ "Resource" });
    setHorizontalHeaderItem(TypeColumn, new QStandardItem{ "Type" });
    setHorizontalHeaderItem(ArnColumn, new QStandardItem{ "Arn" });
    setHorizontalHeaderItem(NameColumn, new QStandardItem{ "Name" });
    setHorizontalHeaderItem(SelectedStateColumn, new QStandardItem{ "Resource selected state" });

    connect(resourceManager, &AWSResourceManager::CommandOutput, this, &AWSImporterModel::OnCommandOutput);
}

void AWSImporterModel::ImportResource(const QString& resourceGroup, const QString& resourceName, const QString& resourceArn)
{
    QVariantMap args;
    args["arn"] = resourceArn;
    args["resource_name"] = resourceName;
    args["resource_group"] = resourceGroup;
    m_resourceManager->ExecuteAsync(m_requestId, "import-resource", args);
}

void AWSImporterModel::ProcessOutputImportableResourceList(const QVariant& value)
{
    auto map = value.value<QVariantMap>();
    QVariantList list = map["ImportableResources"].toList();

    // UpdateItems does the begin/end stuff that is necessary, but the view currently depends on the modelReset signal to refresh itself.
    beginResetModel();

    for (int row = 0; row < list.length(); ++row)
    {
        //Get the first row of the importer model and add it to importer list model
        QList<QStandardItem*> rowItems{};
        for (int column = 0; column < columnCount(); ++column)
        {
            rowItems.append(new QStandardItem{});
        }

        rowItems[ResourceColumn]->setText(list[row].toMap()["Name"].toString());
        rowItems[ResourceColumn]->setEditable(false);

        rowItems[CheckableResourceColumn]->setText(list[row].toMap()["Name"].toString());
        rowItems[CheckableResourceColumn]->setCheckable(true);
        rowItems[CheckableResourceColumn]->setEditable(false);

        QString resourceArn = list[row].toMap()["ARN"].toString();
        QString resourceType = resourceArn.split(":")[2];
        if (resourceType == "dynamodb")
            resourceType = "AWS::DynamoDB::Table";
        else if (resourceType == "lambda")
            resourceType = "AWS::Lambda::Function";
        else if (resourceType == "sns")
            resourceType = "AWS::SNS::Topic";
        else if (resourceType == "sqs")
            resourceType = "AWS::SQS::Queue";
        else if (resourceType == "s3")
            resourceType = "AWS::S3::Bucket";

        rowItems[TypeColumn]->setText(resourceType);
        rowItems[TypeColumn]->setEditable(false);

        rowItems[ArnColumn]->setText(resourceArn);
        rowItems[ArnColumn]->setEditable(false);

        rowItems[NameColumn]->setText("");
        //Set the background color to remind users that it needs to be edited
        rowItems[NameColumn]->setBackground(QBrush(QColor("#303030")));

        //Use this item to record whether the resource is selected
        rowItems[SelectedStateColumn]->setText("unselected");
        rowItems[SelectedStateColumn]->setEditable(false);

        appendRow(rowItems);
    }
    endResetModel();
}

void AWSImporterModel::ListImportableResources(QString region)
{
    removeRows(0, rowCount());

    //Use the list to record the supported resources
    QStringList supportedResourceList;
    m_pendingListRequests = 0;
    supportedResourceList = QStringList("dynamodb") << "lambda" << "sns" << "sqs" << "s3";

    for (int i = 0; i < supportedResourceList.length(); i++)
    {
        QString resourceType = supportedResourceList[i];
        QVariantMap args;

        args["type"] = resourceType;
        if (region != "default")
        {
            args["region"] = region;
        }
        //Count the number of the list process
        m_pendingListRequests++;
        m_resourceManager->ExecuteAsync(m_requestId, "list-importable-resources", args);
    }
}

void AWSImporterModel::OnCommandOutput(AWSResourceManager::RequestId outputId, const char* outputType, const QVariant& output)
{
    // ignore output for other requests
    if (outputId != m_requestId)
    {
        return;
    }
    // Send the signal when all the importable resources have been listed 
    if (m_pendingListRequests > 0 && QString(outputType) != "message")
    {
        m_pendingListRequests--;
        if (m_pendingListRequests == 0)
            ListFinished();
    }
    //Send the message when the resources are imported
    else
    {
        ImporterOutput(output, outputType);
    }
}

QVariant AWSImporterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal)
    {
        if (role == Qt::TextAlignmentRole)
        {
            return Qt::AlignLeft;
        }
    }
    return IAWSImporterModel::headerData(section, orientation, role);
}