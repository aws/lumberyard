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

class QPushButton;
class ResourceManagementView;
class IResourceGroupStatusModel;

class ResourceGroupStatusWidget
    : public StackEventsSplitter
{
    Q_OBJECT

public:

    ResourceGroupStatusWidget(ResourceManagementView* view, QSharedPointer<IResourceGroupStatusModel> resourceGroupStatusModel, QWidget* parent);

private:

    ResourceManagementView* m_view;
    QSharedPointer<IResourceGroupStatusModel> m_resourceGroupStatusModel;
    QPushButton* m_addResourceButton;
    QPushButton* m_importResourceButton;
    QPushButton* m_uploadLambdaCodeButton;
    QPushButton* m_enableResourceGroupButton;

    void CreateUI();
    void UpdateUI();

    void OnAddResource();
    void OnImportResource();
    void OnUploadLambdaCode();
    void EnableResourceGroup();
};

