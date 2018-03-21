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

#include "stdafx.h"

#include <AWSDeploymentModel.h>

#include <AWSDeploymentModel.moc>

#include <QMessageBox>

template<>
const QMap<AWSDeploymentColumn, QString> ColumnEnumToNameMap<AWSDeploymentColumn>::s_columnEnumToNameMap =
{
    {
        AWSDeploymentColumn::Name, "Name"
    },
    {
        AWSDeploymentColumn::Default, "Default"
    },
    {
        AWSDeploymentColumn::PhysicalResourceId, "PhysicalResourceId"
    },
    {
        AWSDeploymentColumn::ProjectDefault, "ProjectDefault"
    },
    {
        AWSDeploymentColumn::Release, "Release"
    },
    {
        AWSDeploymentColumn::ResourceStatus, "ResourceStatus"
    },
    {
        AWSDeploymentColumn::Timestamp, "Timestamp"
    },
    {
        AWSDeploymentColumn::UserDefault, "UserDefault"
    },
    {
        AWSDeploymentColumn::Protected, "Protected"
    }
};

AWSDeploymentModel::AWSDeploymentModel(AWSResourceManager* resourceManager)
    : AWSResourceManagerModel(resourceManager)
{
}

AWSResourceManager::ExecuteAsyncCallback AWSDeploymentModel::CreateCallback(const char* sucessMessage = nullptr)
{
    return [this, sucessMessage](const QString& key, const QVariant& value)
    {
        if (key == "error")
        {
            // TODO: move this to UI
            QMessageBox msgBox(QMessageBox::Critical,
                tr("Cloud Canvas Resource Manager"),
                tr(value.toString().toUtf8().data()),
                QMessageBox::Ok,
                Q_NULLPTR,
                Qt::Popup);
            msgBox.exec();
        }
        if (key == "success")
        {
            if (sucessMessage != nullptr)
            {
                // TODO: move this to UI
                QMessageBox msgBox(QMessageBox::Information,
                    tr("Cloud Canvas Resource Manager"),
                    tr(sucessMessage),
                    QMessageBox::Ok,
                    Q_NULLPTR,
                    Qt::Popup);
                msgBox.exec();
            }
        }
    };
}

void AWSDeploymentModel::SetUserDefaultDeployment(const QString& deploymentName)
{
    QVariantMap args;
    args["set"] = deploymentName;

    ExecuteAsync(CreateCallback(), "default-deployment", args);
}

void AWSDeploymentModel::SetProjectDefaultDeployment(const QString& deploymentName)
{
    QVariantMap args;
    args["set"] = deploymentName;
    args["project"] = true;

    ExecuteAsync(CreateCallback(), "default-deployment", args);
}

void AWSDeploymentModel::SetReleaseDeployment(const QString& deploymentName)
{
    QVariantMap args;
    args["set"] = deploymentName;

    ExecuteAsync(CreateCallback(), "release-deployment", args);
}

void AWSDeploymentModel::ExportLauncherMapping(const QString& deploymentName)
{
    QVariantMap args;
    args["deployment"] = deploymentName;

    ExecuteAsync(CreateCallback("Mapping has been exported to your projects Config folder"), "update-mappings", args);
}

void AWSDeploymentModel::ProtectDeployment(const QString& deploymentName, bool set)
{
    QVariantMap args;
    if (set)
    {
        args["set"] = deploymentName;
    }
    else
    {
        args["clear"] = deploymentName;
    }

    ExecuteAsync(CreateCallback(), "protect-deployment", args);
}

void AWSDeploymentModel::ProcessOutputDeploymentList(const QVariant& value)
{
    auto map = value.value<QVariantMap>();
    QVariantList list = map["Deployments"].toList();
    Sort(list, AWSDeploymentColumn::Name);
    for (int i = 0; i < list.length(); i++)
    {
        if (list[i].toMap()["Default"] == true) {
            QString resourceArn = list[i].toMap()["StackId"].toString();
            QStringList splitList = resourceArn.split(":");
            if (splitList.length() < 4)
            {
                m_region = "";
                QString errorStr{ "Failed to get region from ARN: " };
                errorStr += resourceArn;
                QMessageBox msgBox(QMessageBox::Critical,
                    tr("Cloud Canvas Resource Manager"),
                    errorStr,
                    QMessageBox::Ok,
                    Q_NULLPTR,
                    Qt::Popup);
                msgBox.exec();

            }
            else
            {
                m_region = splitList[3];
            }
            break;
        }
    }    
    // UpdateItems does the begin/end stuff that is necessary, but the view currently depends on the modelReset signal to refresh itself.
    beginResetModel();
    UpdateItems(list);
    endResetModel();
}

QString AWSDeploymentModel::GetActiveDeploymentName() const
{
    int row = FindRow(AWSDeploymentColumn::Default, true);
    if (row == -1)
    {
        return QString {};
    }
    else
    {
        return data(row, AWSDeploymentColumn::Name).toString();
    }
}

bool AWSDeploymentModel::IsActiveDeploymentSet() const
{
    return !GetActiveDeploymentName().isEmpty();
}

QString AWSDeploymentModel::GetActiveDeploymentRegion() const
{
    return m_region;
}
