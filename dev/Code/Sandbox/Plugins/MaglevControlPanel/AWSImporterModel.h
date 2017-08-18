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