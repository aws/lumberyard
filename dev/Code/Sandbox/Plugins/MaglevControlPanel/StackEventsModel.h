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
#include <AwsUtil.h>
#include <IEditor.h>

class StackEventsModel
    : public IStackEventsModel
{
    Q_OBJECT

public:

    StackEventsModel(AWSResourceManager* resourceManager)
        : m_resourceManager{resourceManager}
        , m_requestId{resourceManager->AllocateRequestId()}
        , m_commandStatus{CommandStatus::NoCommandStatus}
    {
        setColumnCount(ColumnCount);
        setHorizontalHeaderItem(OperationColumn, new QStandardItem {"Operation"});
        setHorizontalHeaderItem(StatusColumn, new QStandardItem {"Status"});
        setHorizontalHeaderItem(TimeColumn, new QStandardItem {"Time"});
        connect(resourceManager, &AWSResourceManager::CommandOutput, this, &StackEventsModel::OnCommandOutput);
    }

    AWSResourceManager::RequestId GetRequestId()
    {
        return m_requestId;
    }

    CommandStatus GetCommandStatus() const override
    {
        return m_commandStatus;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal)
        {
            if (role == Qt::TextAlignmentRole)
            {
                return Qt::AlignLeft;
            }
        }
        return IStackEventsModel::headerData(section, orientation, role);
    }

private:

    static const int MAX_ENTRIES = 500;

    AWSResourceManager* m_resourceManager;
    AWSResourceManager::RequestId m_requestId;
    CommandStatus m_commandStatus;

    void OnCommandOutput(AWSResourceManager::RequestId outputId, const char* outputType, const QVariant& output)
    {
        // ignore output for other requests

        if (outputId != m_requestId)
        {
            return;
        }

        // reset contents when a new command starts

        if (m_commandStatus != CommandStatus::CommandInProgress)
        {
            removeRows(0, rowCount());
            m_commandStatus = CommandStatus::CommandInProgress;
            CommandStatusChanged(GetCommandStatus());
            m_resourceManager->OperationStarted();
        }

        // limit number of rows

        if (rowCount() == MAX_ENTRIES)
        {
            removeRow(0);
        }

        // create a new row
        //
        // TODO: the "message" interface between python and the gui is... awkward.
        // Need to rethink how this works. Should probabally route based on stack
        // id. That would allow logs for resource group child stacks to update
        // along with the parent stack's logs when the parent stack is updated.

        QList<QStandardItem*> row;
        for (int i = 0; i < ColumnCount; ++i)
        {
            row.append(new QStandardItem {});
        }

        QString operation;
        QString status;
        QVariant statusTextColor;

        if (strcmp(outputType, "stack-event-errors") == 0)
        {
            operation.append("The following errors occured:\n");
            auto list = output.toMap()["Errors"].toList();
            for (auto it = list.constBegin(); it != list.constEnd(); ++it)
            {
                operation.append("    ");
                operation.append(it->toString());
                operation.append("\n");
            }

            status = "Error";
            statusTextColor = AWSUtil::MakePrettyColor("Error");
        }
        else
        {
            operation = output.toString();
            if (operation.contains("ERROR") || strcmp(outputType, "error") == 0)
            {
                status = "Error";
                statusTextColor = AWSUtil::MakePrettyColor("Error");
            }
            else
            {
                status = "OK";
                statusTextColor = AWSUtil::MakePrettyColor("Positive");
            }
        }

        row[OperationColumn]->setText(operation);
        row[StatusColumn]->setText(status);
        row[StatusColumn]->setData(statusTextColor, Qt::TextColorRole);
        row[TimeColumn]->setText(QTime::currentTime().toString());

        this->appendRow(row);

        // update the command status if necessary

        if (strcmp(outputType, "error") == 0)
        {
            m_commandStatus = CommandStatus::CommandFailed;
            CommandStatusChanged(GetCommandStatus());
            m_resourceManager->OperationFinished();
        }
        else if (strcmp(outputType, "success") == 0)
        {
            m_commandStatus = CommandStatus::CommandSucceeded;
            CommandStatusChanged(GetCommandStatus());
            m_resourceManager->OperationFinished();
        }
    }
};
