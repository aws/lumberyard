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

#include <AWSResourceManager.h>

#include <StackStatusListModel.h>

class ResourceGroupListStatusModel
    : public StackStatusListModel<IResourceGroupListStatusModel>
{
    Q_OBJECT

public:

    ResourceGroupListStatusModel(AWSResourceManager* resourceManager);

    void Refresh(bool force = false) override;

    void ProcessOutputResourceGroupList(const QVariant& value);

    QSharedPointer<IDeploymentStatusModel> GetActiveDeploymentStatusModel() const override;

    QString GetMainTitleText() const override;
    QString GetMainMessageText() const override;
    QString GetListTitleText() const override;
    QString GetListMessageText() const override;
    QString GetUpdateButtonText() const override;
    QString GetUpdateButtonToolTip() const override;
    QString GetAddButtonText() const override;
    QString GetAddButtonToolTip() const override;

private:

    void UpdateRow(const QList<QStandardItem*>& row, const QVariantMap& map) override;

    AWSResourceManager::RequestId m_requestId;

};
