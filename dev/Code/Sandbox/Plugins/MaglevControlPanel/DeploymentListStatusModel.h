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

class DeploymentListStatusModel
    : public StackStatusListModel<IDeploymentListStatusModel>
{
    Q_OBJECT

public:

    DeploymentListStatusModel(AWSResourceManager* resourceManager);

    void Refresh(bool force = false) override;

    void ProcessOutputDeploymentList(const QVariant& value);

    virtual QString GetMainTitleText() const override;
    virtual QString GetMainMessageText() const override;
    virtual QString GetListTitleText() const override;
    virtual QString GetListMessageText() const override;
    virtual QString GetAddButtonText() const override;
    virtual QString GetAddButtonToolTip() const override;

private:

    void UpdateRow(const QList<QStandardItem*>& row, const QVariantMap& map) override;

};
