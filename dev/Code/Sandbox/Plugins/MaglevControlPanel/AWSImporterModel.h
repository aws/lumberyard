/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
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
#include <AWSResourceManager.h>
#include <AWSImporterModel.h>

class AWSImporterModel
    : public IAWSImporterModel
{
    Q_OBJECT

public:

    AWSImporterModel(AWSResourceManager* resourceManager);

    void ProcessOutputImportableResourceList(const QVariant& value);
    void ImportResource(const QString& resourceGroup, const QString& resourceName, const QString& resourceArn) override;
    void ListImportableResources(QString region) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:

void OnCommandOutput(AWSResourceManager::RequestId outputId, const char* outputType, const QVariant& output);

private:

    AWSResourceManager* m_resourceManager;
    AWSResourceManager::RequestId m_requestId;

    int m_pendingListRequests;
};