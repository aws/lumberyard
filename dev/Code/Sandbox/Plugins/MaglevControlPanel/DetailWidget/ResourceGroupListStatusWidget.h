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

class ResourceManagementView;
class IResourceGroupListStatusModel;
class QPushButton;

class ResourceGroupListStatusWidget
    : public StackEventsSplitter
{
    Q_OBJECT

public:

    ResourceGroupListStatusWidget(
        ResourceManagementView* view,
        QSharedPointer<IResourceGroupListStatusModel> resoruceGroupListStatusModel, QWidget* parent);

private:

    ResourceManagementView* m_view;
    QSharedPointer<IResourceGroupListStatusModel> m_resourceGroupListStatusModel;
    QPushButton* m_updateButton;

    void CreateUI();
    void UpdateUI();
    void UpdateActiveDeployment();
    void OnActiveDeploymentChanged(const QString& deploymentName);
};
