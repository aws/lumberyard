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

#include <IAWSResourceManager.h>
#include <AWSResourceManagerModel.h>

class AWSDeploymentModel
    : public AWSResourceManagerModel<IAWSDeploymentModel>
{
    Q_OBJECT

public:

    AWSDeploymentModel(AWSResourceManager* resourceManager);

    void SetUserDefaultDeployment(const QString& deploymentName);
    void SetProjectDefaultDeployment(const QString& deploymentName);

    void SetReleaseDeployment(const QString& deploymentName);

    void ExportLauncherMapping(const QString& deploymentName);

    void ProtectDeployment(const QString& deploymentName, bool set);

    void ProcessOutputDeploymentList(const QVariant& value);

    QString GetActiveDeploymentName() const override;
    bool IsActiveDeploymentSet() const override;
    QString GetActiveDeploymentRegion() const override;

private:

    AWSResourceManager::ExecuteAsyncCallback CreateCallback(const char* operation);
    QString m_region;
};
