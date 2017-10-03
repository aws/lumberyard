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

#include <FileSourceControlModel.h>

#include <IFileSourceControlModel.moc>

SourceControlStatusModel::SourceControlStatusModel()
{
    setRowCount(NextRow);

    for (int row = 0; row < NextRow; ++row)
    {
        setItem(row, 0, new QStandardItem {});
    }
}

AzToolsFramework::SourceControlStatus SourceControlStatusModel::GetStatus() const
{
    return static_cast<AzToolsFramework::SourceControlStatus>(index(FileSourceControlStatus, 0).data(Qt::DisplayRole).toUInt());
}

unsigned int SourceControlStatusModel::GetFlags() const
{
    return index(FileSourceControlFlags, 0).data().toUInt();
}

void SourceControlStatusModel::SetStatus(AzToolsFramework::SourceControlStatus newStatus)
{
    AzToolsFramework::SourceControlStatus oldStatus = GetStatus();
    SetReady(true);
    if (newStatus != oldStatus)
    {
        setData(index(FileSourceControlStatus, 0), QVariant(newStatus), Qt::DisplayRole);
        SourceControlStatusChanged();
    }
    SourceControlStatusUpdated();
}

void SourceControlStatusModel::SetFlags(unsigned int newFlags)
{
    setData(index(FileSourceControlFlags, 0), QVariant(newFlags), Qt::DisplayRole);
}


void SourceControlStatusModel::SetReady(bool readyState)
{
    setData(index(ReadyState, 0), QVariant(readyState), Qt::DisplayRole);
}

bool SourceControlStatusModel::IsReady() const
{
    return index(ReadyState, 0).data().toBool();
}

bool SourceControlStatusModel::FileNeedsCheckout() const
{
    AzToolsFramework::SourceControlStatus curStatus = GetStatus();

    return (curStatus == AzToolsFramework::SCS_Tracked);
}