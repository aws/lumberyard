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

#include <QObject>
#include <QSharedPointer>
#include <AWSResourceManagerModel.h>
#include <AWSResourceManager.h>

class PythonWorker;

class AWSProfileModel
    : public AWSResourceManagerModel<IAWSProfileModel>
{
    Q_OBJECT

public:

    AWSProfileModel(AWSResourceManager* resourceManager);

    void SetDefaultProfile(const QString& profileName) override;
    QString GetDefaultProfile() const override;
    void AddProfile(const QString& profileName, const QString& secretKey, const QString& accessKey, bool makeDefault) override;
    void UpdateProfile(const QString& oldName, const QString& newName, const QString& secretKey, const QString& accessKey) override;
    void DeleteProfile(const QString& profileName) override;

    bool AWSCredentialsFileExists() const override;

    void ProcessOutputProfileList(const QVariant& value);

private:

    QString m_credentialsFilePath;
    AWSResourceManager::RequestId m_pendingRequestId{0};
    enum class PendingRequestType { None, Add, Update, Delete };
    PendingRequestType m_pendingRequestType{PendingRequestType::None};

    void OnCommandOutput(AWSResourceManager::RequestId outputId, const char* outputType, const QVariant& output);

};

