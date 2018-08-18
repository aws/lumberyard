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

#include "DetailWidget.h"
#include "StatefulLayout.h"

class ResourceManagementView;
class IResourceGroupStatusModel;

class ResourceGroupDetailWidget
    : public DetailWidget
{
    Q_OBJECT

public:

    ResourceGroupDetailWidget(ResourceManagementView* view, QSharedPointer<IResourceGroupStatusModel> resourceGroupStatusModel, QWidget* parent);

    void show() override;

    QMenu* GetTreeContextMenu() override;

private:

    QSharedPointer<IResourceGroupStatusModel> m_resourceGroupStatusModel;

    enum class State
    {
        Loading,
        Status,
        NoProfile,
        NoActiveDeployment,
        NoResource,
        Error
    };

    StatefulLayout<State> m_layout {
        this
    };

    void UpdateUI();
    void OnUpdate();
    void OnDelete();
    void OnEnable();
    void OnUploadCode();
};

