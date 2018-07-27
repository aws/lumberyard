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

#include <ResourceGroupStatusModel.h>
#include <DeploymentStatusModel.h>
#include <StackResourcesModel.h>
#include <AWSProjectModel.h>

#include <ResourceGroupStatusModel.moc>

#include <QMessageBox>

class ResourceGroupResourcesModel
    : public StackResourcesModel
{
public:

    ResourceGroupResourcesModel(AWSResourceManager* resourceManager, const QString& resourceGroupName)
        : StackResourcesModel{resourceManager}
        , m_resourceGroupName{resourceGroupName}
    {}

protected:

    const char* PrepareRefreshCommand(QVariantMap& args) override
    {
        args["resource_group"] = m_resourceGroupName;
        return "list-resource-group-resources";
    }

    QString m_resourceGroupName;

};

ResourceGroupStatusModel::ResourceGroupStatusModel(AWSResourceManager* resourceManager, const QString& resourceGroup)
    : StackStatusModel{resourceManager, ColumnCount, new ResourceGroupResourcesModel {resourceManager, resourceGroup}}
    , m_requestId{ resourceManager->AllocateRequestId() }
{
    item(0, ResourceGroupNameColumn)->setData(resourceGroup, Qt::DisplayRole);

    setHorizontalHeaderItem(ActiveDeploymentNameColumn, new QStandardItem {"Deployment"});
    setHorizontalHeaderItem(ResourceGroupNameColumn, new QStandardItem {"Resource Group"});
    setHorizontalHeaderItem(UserDefinedResourceCountColumn, new QStandardItem {"Resource Count"});

    Refresh();
}

void ResourceGroupStatusModel::ProcessOutputResourceGroupStackDescription(const QVariantMap& map)
{
    auto userDefinedResourceCount = map["UserDefinedResourceCount"];
    if (userDefinedResourceCount.isValid())
    {
        item(0, UserDefinedResourceCountColumn)->setData(userDefinedResourceCount.toInt(), Qt::DisplayRole);
    }

    ProcessOutputStackDescription(map);
}

void ResourceGroupStatusModel::Refresh(bool force)
{
    if (force || IsRefreshTime())
    {
        StackStatusModel::Refresh(force);

        item(0, ActiveDeploymentNameColumn)->setData(ResourceManager()->GetActiveDeploymentStatusModel()->DeploymentName(), Qt::DisplayRole);

        QVariantMap args;
        args["deployment"] = ActiveDeploymentName();
        args["resource_group"] = ResourceGroupName();
        ResourceManager()->ExecuteAsyncWithRetry(m_requestId, "describe-resource-group-stack", args);
    }
}

bool ResourceGroupStatusModel::UpdateStack()
{
    if (!ActiveDeploymentName().isEmpty() && ResourceManager()->IsProjectInitialized())
    {
        QString deploymentName = ActiveDeploymentName();
        ResourceManager()->GetProjectModelInternal()->OnResourceGroupInProgress(deploymentName, ResourceGroupName());
        ResourceManager()->GetProjectModelInternal()->OnDeploymentInProgress(deploymentName);

        QVariantMap args;
        args["deployment"] = deploymentName;
        args["resource_group"] = ResourceGroupName();
        args["confirm_aws_usage"] = true;
        args["confirm_resource_deletion"] = true;
        args["confirm_security_change"] = true;
        ResourceManager()->ExecuteAsync(GetStackEventsModelInternal()->GetRequestId(), "upload-resources", args);
        return true;
    }
    else
    {
        return false;
    }
}

bool ResourceGroupStatusModel::UploadLambdaCode(QString functionName)
{
    if (ActiveDeploymentName().isEmpty() || !ResourceManager()->IsProjectInitialized())
    {
        return false;
    }

    QString deploymentName = ActiveDeploymentName();
    ResourceManager()->GetProjectModelInternal()->OnResourceGroupInProgress(deploymentName, ResourceGroupName());
    ResourceManager()->GetProjectModelInternal()->OnDeploymentInProgress(deploymentName);

    QVariantMap args;
    args["deployment"] = deploymentName;
    args["resource_group"] = ResourceGroupName();
    if (functionName != "")
        args["function"] = functionName;
    ResourceManager()->ExecuteAsync(GetStackEventsModelInternal()->GetRequestId(), "upload-lambda-code", args);
    return true;
}

bool ResourceGroupStatusModel::DeleteStack()
{

    auto callback = [](const QString& errorMessage)
    {
        if (!errorMessage.isEmpty())
        {
            // TODO move to UI
            QMessageBox::critical(
                nullptr,
                "Remove Resource Group Error",
                errorMessage, QMessageBox::Ok);
        }
    };

    ResourceManager()->GetProjectModel()->DisableResourceGroup(ResourceGroupName(), callback);

    return true;

}

bool ResourceGroupStatusModel::EnableResourceGroup()
{

    auto callback = [](const QString& errorMessage)
    {
        if (!errorMessage.isEmpty())
        {
            QMessageBox::critical(
                nullptr,
                "Enable Resource Group Error",
                errorMessage, QMessageBox::Ok);
        }
    };

    ResourceManager()->GetProjectModel()->EnableResourceGroup(ResourceGroupName(), callback);

    return true;

}

QSharedPointer<ICloudFormationTemplateModel> ResourceGroupStatusModel::GetTemplateModel()
{
    return ResourceManager()->GetProjectModelInternal()->GetResourceGroupTemplateModel(ResourceGroupName());
}

QString ResourceGroupStatusModel::GetUpdateButtonText() const
{
    if (IsPendingDelete())
    {
        return tr("Delete resources");
    }
    else if (IsPendingCreate())
    {
        return tr("Create resources");
    }
    else
    {
        return tr("Upload resources");
    }
}

QString ResourceGroupStatusModel::GetUpdateButtonToolTip() const
{
    if (IsPendingDelete())
    {
        return tr("Delete resources in AWS as needed to match the local configuration.");
    }
    else if (IsPendingCreate())
    {
        return tr("Create resources in AWS as needed to match the local configuration.");
    }
    else
    {
        return tr("Modify resources in AWS as needed to match the local configuration.");
    }
}

QString ResourceGroupStatusModel::GetUpdateConfirmationTitle() const
{
    if (IsPendingDelete())
    {
        return tr("Delete group resources?");
    }
    else if (IsPendingCreate())
    {
        return tr("Create group resources?");
    }
    else
    {
        return tr("Upload group resources?");
    }
}

QString ResourceGroupStatusModel::GetUpdateConfirmationMessage() const
{
    if (IsPendingDelete())
    {
        return tr(
            "Do you want to delete the %1 resource group in the %2 deployment?<br><br>"
            "This operation will delete resources in AWS "
            "as needed to make the configuration in AWS match the local configuration.<br><br>"
            "It may take several minutes for this operation to complete."
            ).arg(ResourceGroupName(), ActiveDeploymentName());
    }
    else if (IsPendingCreate())
    {
        return tr(
            "Do you want to create the %1 resource group in the %2 deployment?<br><br>"
            "This operation will create resources in AWS "
            "as needed to make the configuration in AWS match the local configuration.<br><br>"
            "It may take several minutes for this operation to complete."
            ).arg(ResourceGroupName(), ActiveDeploymentName());
    }
    else
    {
        return tr(
            "Do you want to update the %1 resource group in the %2 deployment?<br><br>"
            "This operation will modify, create, and delete resources in AWS "
            "as needed to make the configuration in AWS match the local configuration.<br><br>"
            "It may take several minutes for this operation to complete."
            ).arg(ResourceGroupName(), ActiveDeploymentName());
    }
}

QString ResourceGroupStatusModel::GetStatusTitleText() const
{
    return tr("%1 resource group status").arg(ResourceGroupName());
}

QString ResourceGroupStatusModel::GetMainMessageText() const
{
    return tr(
        "Resource groups are used to define the AWS resources you will use in your game. "
        "Each resource group represents a single game feature, such as a high score system. "
        "The resources will be created in the cloud as part of a deployment. "
        "Each deployment is an independent copy of the resources defined in the project's resource groups."
        );
}

QString ResourceGroupStatusModel::GetDeleteButtonText() const
{
    return tr("Disable resource group");
}

QString ResourceGroupStatusModel::GetDeleteButtonToolTip() const
{
    return tr("Remove the resource group from the local configuration.");
}

QString ResourceGroupStatusModel::GetDeleteConfirmationTitle() const
{
    return tr("Disable resource group?");
}

QString ResourceGroupStatusModel::GetDeleteConfirmationMessage() const
{
    return tr(
        "Do you want to disable the %1 resource group from the local configuration?"
        "<br><br>"
        "This operation does not immediatly delete the resources in AWS. Each deployment "
        "must be updated to delete the resoures in AWS."
        "<br><br>"
        "This operation does not remove the resource group configuration data from "
        "the local disk. As long as that data exists, the resource group can be "
        "re-enabled."
        ).arg(ResourceGroupName(), ActiveDeploymentName());
}

QString ResourceGroupStatusModel::GetEnableButtonText() const
{
    return tr("Enable resource group");
}

QString ResourceGroupStatusModel::GetEnableButtonToolTip() const
{
    return tr("Enable the resource group to the Lumberyard project.");
}
