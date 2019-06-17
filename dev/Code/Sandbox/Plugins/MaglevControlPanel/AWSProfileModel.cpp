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

#include <AWSProfileModel.h>
#include <QFileInfo>
#include <IEditor.h>

#include <CloudCanvas/ICloudCanvasEditor.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

#include <AWSProfileModel.moc>

template<>
const QMap<AWSProfileColumn, QString> ColumnEnumToNameMap<AWSProfileColumn>::s_columnEnumToNameMap =
{
    {
        AWSProfileColumn::Name, "Name"
    },
    {
        AWSProfileColumn::Default, "Default"
    },
    {
        AWSProfileColumn::SecretKey, "SecretKey"
    },
    {
        AWSProfileColumn::AccessKey, "AccessKey"
    }
};

AWSProfileModel::AWSProfileModel(AWSResourceManager* resourceManager)
    : AWSResourceManagerModel(resourceManager)
{
    connect(ResourceManager(), &AWSResourceManager::CommandOutput, this, &AWSProfileModel::OnCommandOutput);
}

void AWSProfileModel::SetDefaultProfile(const QString& profileName)
{
    EBUS_EVENT(CloudCanvas::CloudCanvasEditorRequestBus, SetCredentials, Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>("AWSManager", profileName.toStdString().c_str(), Aws::Auth::REFRESH_THRESHOLD));
    EBUS_EVENT(CloudCanvas::CloudCanvasEditorRequestBus, ApplyConfiguration);

    QVariantMap args;
    args["set"] = profileName;

    int previousDefaultProfileRow = FindRow(AWSProfileColumn::Default, true);
    int newDefaultProfileRow = FindRow(AWSProfileColumn::Name, profileName);
    if (newDefaultProfileRow != previousDefaultProfileRow)
    {
        setData(index(previousDefaultProfileRow, AWSProfileColumn::Default), false);
        setData(index(newDefaultProfileRow, AWSProfileColumn::Default), true);
    }

    // TODO: add error handling
    ExecuteAsync(ResourceManager()->AllocateRequestId(), "default-profile", args);

    GetIEditor()->Notify(eNotify_OnSwitchAWSProfile);
}

void AWSProfileModel::AddProfile(const QString& profileName, const QString& secretKey, const QString& accessKey, bool makeDefault)
{
    m_pendingRequestId = ResourceManager()->AllocateRequestId();
    m_pendingRequestType = PendingRequestType::Add;

    QVariantMap args;
    args["profile"] = profileName;
    args["aws_secret_key"] = secretKey.simplified();
    args["aws_access_key"] = accessKey.simplified();
    args["make_default"] = makeDefault;

    ExecuteAsync(m_pendingRequestId, "add-profile", args);
}

void AWSProfileModel::UpdateProfile(const QString& oldName, const QString& newName, const QString& secretKey, const QString& accessKey)
{
    m_pendingRequestId = ResourceManager()->AllocateRequestId();
    m_pendingRequestType = PendingRequestType::Update;

    QVariantMap args;
    args["old_name"] = oldName;
    args["new_name"] = newName;
    args["aws_secret_key"] = secretKey.simplified();
    args["aws_access_key"] = accessKey.simplified();

    ExecuteAsync(m_pendingRequestId, "update-profile", args);
}

void AWSProfileModel::DeleteProfile(const QString& profileName)
{
    m_pendingRequestId = ResourceManager()->AllocateRequestId();
    m_pendingRequestType = PendingRequestType::Delete;

    QVariantMap args;
    args["profile"] = profileName;

    ExecuteAsync(m_pendingRequestId, "remove-profile", args);
}

bool AWSProfileModel::AWSCredentialsFileExists() const
{
    QFileInfo fileInfo {
        m_credentialsFilePath
    };
    return fileInfo.exists();
}


void AWSProfileModel::ProcessOutputProfileList(const QVariant& value)
{
    auto map = value.value<QVariantMap>();
    for (auto thisElement : map)
    {
        qDebug() << "First: " << thisElement.toString();
    }
    auto list = map["Profiles"].toList();
    for (auto thisElement : list)
    {
        qDebug() << "Name: " << thisElement.toString();
    }
    Sort(list, AWSProfileColumn::Name);

    // UpdateItems does the begin/end stuff that is necessary, but the view currently depends on the modelReset signal to refresh itself.
    beginResetModel();
    UpdateItems(list);
    m_credentialsFilePath = map["CredentialsFilePath"].toString();
    endResetModel();
}

QString AWSProfileModel::GetDefaultProfile() const
{
    int row = FindRow(AWSProfileColumn::Default, true);
    if (row == -1)
    {
        return QString {};
    }
    else
    {
        return data(row, AWSProfileColumn::Name).toString();
    }
}

void AWSProfileModel::OnCommandOutput(AWSResourceManager::RequestId outputId, const char* outputType, const QVariant& output)
{
    if (outputId != m_pendingRequestId)
    {
        return;
    }

    bool isSuccess = strcmp(outputType, "success") == 0;
    bool isFailure = strcmp(outputType, "error") == 0;

    switch (m_pendingRequestType)
    {
    case PendingRequestType::None:
        break;

    case PendingRequestType::Add:
        if (isSuccess)
        {
            AddProfileSucceeded();
        }
        if (isFailure)
        {
            AddProfileFailed(output.toString());
        }
        break;

    case PendingRequestType::Update:
        if (isSuccess)
        {
            UpdateProfileSucceeded();
        }
        if (isFailure)
        {
            UpdateProfileFailed(output.toString());
        }
        break;

    case PendingRequestType::Delete:
        if (isSuccess)
        {
            DeleteProfileSucceeded();
        }
        if (isFailure)
        {
            DeleteProfileFailed(output.toString());
        }
        break;

    default:
        assert(false);
        break;
    }
}

