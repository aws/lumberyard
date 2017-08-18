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

class CloudFormationTemplateItem;

class CloudFormationTemplateModel
    : public ICloudFormationTemplateModel
{
    Q_OBJECT

public:

    CloudFormationTemplateModel(QSharedPointer<IFileContentModel> fileContentModel);

    void ValidateContent() override;

    bool Save() override;

    void Reload() override;

    QSharedPointer<IStackStatusModel> GetStackStatusModel() const override;

    bool SetResourceContent(const QString& resourceName, const QString& contentData) override;
    QString GetResourceContent(const QString& resourceName, bool addRelated) const override;
    bool DeleteResource(const QString& resourceName) override;
    bool ReadResources(const QString& contentData) override;
    bool AddResource(const QString& resourceName, const QVariantMap& variantMap) override;
    bool AddLambdaAccess(const QString& resourceName, const QString& functionName) override;
    bool AddParameters(const QVariantMap& variantMap) override;
    bool HasResource(const QString& resourceName) override;
    bool CanAddResources() override;
    void SetCanAddResources(bool canAdd) override;
private:

    CloudFormationTemplateItem* GetContentItem();
    QSharedPointer<IFileContentModel> m_fileContentModel;
    bool m_canAddResources;

private slots:

    void OnFileContentModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
};
