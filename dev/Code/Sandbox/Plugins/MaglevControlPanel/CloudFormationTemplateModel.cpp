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

#include <CloudFormationTemplateModel.h>

#include <CloudFormationTemplateModel.moc>
#include <ResourceParsing.h>

#include <assert.h>

class CloudFormationTemplateItem
    : public QStandardItem
{
public:

    CloudFormationTemplateItem(QSharedPointer<IFileContentModel> fileContentModel)
        : m_fileContentModel{fileContentModel}
    {
    }

    QVariant data(int role) const override
    {
        if (role == ICloudFormationTemplateModel::ValidationErrorRole)
        {
            return data(role);
        }
        else
        {
            return m_fileContentModel->ContentIndex().data(role);
        }
    }

    void setData(const QVariant& value, int role) override
    {
        if (role == ICloudFormationTemplateModel::ValidationErrorRole)
        {
            setData(value, role);
        }
        else
        {
            m_fileContentModel->setData(m_fileContentModel->ContentIndex(), value, role);
        }
    }

private:

    QSharedPointer<IFileContentModel> m_fileContentModel;
};

CloudFormationTemplateModel::CloudFormationTemplateModel(QSharedPointer<IFileContentModel> fileContentModel)
    : m_fileContentModel{fileContentModel},
    m_canAddResources{ false }
{
    appendRow(new CloudFormationTemplateItem {m_fileContentModel});
    connect(m_fileContentModel.data(), &IFileContentModel::dataChanged, this, &CloudFormationTemplateModel::OnFileContentModelDataChanged);
}

void CloudFormationTemplateModel::ValidateContent()
{
    // TODO
}

bool CloudFormationTemplateModel::Save()
{
    return m_fileContentModel->Save();
}

void CloudFormationTemplateModel::Reload()
{
    m_fileContentModel->Reload();
}

CloudFormationTemplateItem* CloudFormationTemplateModel::GetContentItem()
{
    return static_cast<CloudFormationTemplateItem*>(itemFromIndex(ContentIndex()));
}

void CloudFormationTemplateModel::OnFileContentModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    assert(topLeft == bottomRight);
    dataChanged(ContentIndex(), ContentIndex(), roles);
}

QSharedPointer<IStackStatusModel> CloudFormationTemplateModel::GetStackStatusModel() const
{
    return m_fileContentModel->GetStackStatusModel();
}

QString CloudFormationTemplateModel::GetResourceContent(const QString& resourceName, bool addRelated) const
{
    return ResourceParsing::GetResourceContent(m_fileContentModel->GetContent(), resourceName, addRelated);
}

bool CloudFormationTemplateModel::SetResourceContent(const QString& resourceName, const QString& contentData)
{
    m_fileContentModel->SetContent(ResourceParsing::SetResourceContent(GetContent(), resourceName, contentData));
    return true;
}

bool CloudFormationTemplateModel::ReadResources(const QString& contentData)
{
    m_fileContentModel->SetContent(ResourceParsing::ReadResources(GetContent(), contentData));
    return true;
}

bool CloudFormationTemplateModel::DeleteResource(const QString& resourceName)
{
    m_fileContentModel->SetContent(ResourceParsing::DeleteResource(GetContent(), resourceName));
    return true;
}

bool CloudFormationTemplateModel::AddResource(const QString& resourceName, const QVariantMap& variantMap)
{
    m_fileContentModel->SetContent(ResourceParsing::AddResource(GetContent(), resourceName, variantMap));
    return true;
}

bool CloudFormationTemplateModel::AddParameters(const QVariantMap& variantMap)
{
    m_fileContentModel->SetContent(ResourceParsing::AddParameters(GetContent(), variantMap));
    return true;
}

bool CloudFormationTemplateModel::AddLambdaAccess(const QString& resourceName, const QString& functionName)
{
    m_fileContentModel->SetContent(ResourceParsing::AddLambdaAccess(GetContent(), resourceName, functionName));
    return true;
}

bool CloudFormationTemplateModel::HasResource(const QString& resourceName)
{
    return ResourceParsing::HasResource(GetContent(), resourceName);
}

bool CloudFormationTemplateModel::CanAddResources() 
{ 
    return m_canAddResources; 
}

void CloudFormationTemplateModel::SetCanAddResources(bool canAdd)
{ 
    m_canAddResources = canAdd; 
}