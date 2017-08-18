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

#include <PlaceholderDeploymentStatusModel.h>

#include <PlaceholderDeploymentStatusModel.moc>

class PlaceholderDeploymentResourcesModel
    : public StackResourcesModel
{
public:

    PlaceholderDeploymentResourcesModel(AWSResourceManager* resourceManager)
        : StackResourcesModel{resourceManager}
    {}

    void Refresh(bool force = false) override
    {
        // DO NOTHING
    }

protected:

    const char* PrepareRefreshCommand(QVariantMap& args) override
    {
        assert(false);
        return "";
    }

};

PlaceholderDeploymentStatusModel::PlaceholderDeploymentStatusModel(AWSResourceManager* resourceManager)
    : StackStatusModel{resourceManager, ColumnCount, new PlaceholderDeploymentResourcesModel {resourceManager}}
{
    item(0, DeploymentNameColumn)->setData("(PLACEHOLDER)", Qt::DisplayRole);

    setHorizontalHeaderItem(DeploymentNameColumn, new QStandardItem {"Deployment"});

    ClearStackStatus();
}

void PlaceholderDeploymentStatusModel::Refresh(bool force)
{
    // DO NOTHING
}

bool PlaceholderDeploymentStatusModel::UpdateStack()
{
    assert(false);
    return false;
}

bool PlaceholderDeploymentStatusModel::DeleteStack()
{
    assert(false);
    return false;
}

QString PlaceholderDeploymentStatusModel::GetUpdateButtonText() const
{
    return "Upload all resources";
}

QString PlaceholderDeploymentStatusModel::GetUpdateButtonToolTip() const
{
    return
        "Updates resources in AWS as needed to match the local configuration.";
}

QString PlaceholderDeploymentStatusModel::GetUpdateConfirmationTitle() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetUpdateConfirmationMessage() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetStatusTitleText() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetMainMessageText() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetDeleteButtonText() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetDeleteButtonToolTip() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetDeleteConfirmationTitle() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetDeleteConfirmationMessage() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetExportMappingButtonText() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetExportMappingToolTipText() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetProtectedMappingCheckboxText() const
{
    assert(false);
    return "(PLACEHOLDER)";
}

QString PlaceholderDeploymentStatusModel::GetProtectedMappingToolTipText() const
{
    assert(false);
    return "(PLACEHOLDER)";
}
