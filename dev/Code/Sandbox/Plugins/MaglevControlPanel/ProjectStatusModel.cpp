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

#include <ProjectStatusModel.h>
#include <AWSResourceManager.h>
#include <AWSProjectModel.h>

#include <ProjectStatusModel.moc>

class ProjectResourcesModel
    : public StackResourcesModel
{
public:

    ProjectResourcesModel(AWSResourceManager* resourceManager)
        : StackResourcesModel{resourceManager}
    {}

protected:

    const char* PrepareRefreshCommand(QVariantMap& args) override
    {
        return "list-project-resources";
    }

};

ProjectStatusModel::ProjectStatusModel(AWSResourceManager* resourceManager)
    : StackStatusModel{resourceManager, ColumnCount, new ProjectResourcesModel {resourceManager}}
    , m_requestId{resourceManager->AllocateRequestId()}
{
    connect(resourceManager, &AWSResourceManager::ProjectInitializedChanged, this, [this](){ Refresh(); });
    Refresh();
}

void ProjectStatusModel::ProcessOutputProjectStackDescription(const QVariantMap& map)
{
    ProcessOutputStackDescription(map);
}

void ProjectStatusModel::Refresh(bool force)
{
    if (force || IsRefreshTime())
    {
        StackStatusModel::Refresh(force);
        ResourceManager()->ExecuteAsyncWithRetry(m_requestId, "describe-project-stack");
    }
}

bool ProjectStatusModel::UpdateStack()
{
    if (ResourceManager()->GetInitializationState() == IAWSResourceManager::InitializationState::InitializedState ||
        ResourceManager()->GetInitializationState() == IAWSResourceManager::InitializationState::InitializingState)
    {
        ResourceManager()->GetProjectModelInternal()->OnProjectStackInProgress();

        QVariantMap args;
        args["confirm_aws_usage"] = true;
        args["confirm_resource_deletion"] = true;
        args["confirm_security_change"] = true;        
        args["cognito_prod"] = ResourceManager()->GetIdentityId().c_str();
        
        ResourceManager()->ExecuteAsync(GetStackEventsModelInternal()->GetRequestId(), "update-project-stack", args);
        return true;
    }
    else
    {
        return false;
    }
}

bool ProjectStatusModel::CanDeleteStack() const
{
    return false;  // can't delete project stack from in the GUI
}

bool ProjectStatusModel::DeleteStack()
{
    assert(false); // can't delete project stack from in the GUI
    return false;
}

QString ProjectStatusModel::GetUpdateButtonText() const
{
    if (ResourceManager()->GetInitializationState() == IAWSResourceManager::InitializationState::InitializedState)
    {
        return "Upload resources";
    }
    return "Create project stack";
}

QString ProjectStatusModel::GetUpdateButtonToolTip() const
{
    return
        "Updates the resources required by Resource Manager to work with the Lumberyard project.";
}

QString ProjectStatusModel::GetUpdateConfirmationTitle() const
{
    return tr("Update project stack?");
}

QString ProjectStatusModel::GetUpdateConfirmationMessage() const
{
    return
        "Do you want to update the project stack?<br><br>"
        "This operation will update the resources used by the Resource Manager. "
        "<br><br>";
    "This operation may only be performed by an AWS account admin.<br><br>"
    "It may take several minutes to complete this operation.";
}

QString ProjectStatusModel::GetStatusTitleText() const
{
    return tr("Project stack status");
}

QString ProjectStatusModel::GetMainMessageText() const
{
    return tr(
        "The project stack defines all the AWS resources required by the Resource Manager "
        "to work with your Lumberyard project."
        );
}

QString ProjectStatusModel::GetDeleteButtonText() const
{
    assert(false); // can't delete project stack from in the GUI
    return "(Not Supported)";
}

QString ProjectStatusModel::GetDeleteButtonToolTip() const
{
    assert(false); // can't delete project stack from in the GUI
    return "(Not Supported)";
}

QString ProjectStatusModel::GetDeleteConfirmationTitle() const
{
    assert(false); // can't delete project stack from in the GUI
    return "(Not Supported)";
}

QString ProjectStatusModel::GetDeleteConfirmationMessage() const
{
    assert(false); // can't delete project stack from in the GUI
    return "(Not Supported)";
}
