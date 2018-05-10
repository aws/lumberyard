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

#include "StackEventsSplitter.h"

class IDeploymentStatusModel;
class ResourceManagementView;
class QCheckBox;

class DeploymentStatusWidget
    : public StackEventsSplitter
{
    Q_OBJECT

public:

    DeploymentStatusWidget(ResourceManagementView* view, QSharedPointer<IDeploymentStatusModel> deploymentStatusModel, QWidget* parent);

private:

    ResourceManagementView* m_view;
    QSharedPointer<IDeploymentStatusModel> m_deploymentStatusModel;
    QCheckBox* m_chkProtectedMapping;

    void CreateUI();
    void UpdateUI();
    void OnExportMapping();
    void OnProtectDeploymentToggled(bool checked);
};
