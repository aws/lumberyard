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

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <QStandardItemModel>

class IFileSourceControlModel
    : public QStandardItemModel
{
    Q_OBJECT

public:

    enum Rows
    {
        FileSourceControlStatus,
        FileSourceControlFlags,

        ReadyState,
        NextRow
    };

    virtual AzToolsFramework::SourceControlStatus GetStatus() const = 0;
    virtual unsigned int GetFlags() const = 0;

    virtual void SetStatus(AzToolsFramework::SourceControlStatus newStatus) = 0;
    virtual void SetFlags(unsigned int newFlags) = 0;

    virtual bool IsReady() const = 0;
    virtual void SetReady(bool readyState) = 0;

    virtual bool FileNeedsCheckout() const = 0;
signals:

    void SourceControlStatusChanged();
    void SourceControlStatusUpdated();
};

