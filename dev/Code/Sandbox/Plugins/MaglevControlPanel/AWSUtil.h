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

#include <QString>
#include <QColor>

namespace AWSUtil
{
    QColor MakePrettyColor(const QString& colorName);
    QString MakePrettyResourceStatusText(const QString& status);
    QString MakePrettyResourceStatusTooltip(const QString& status, const QString& statusReason);
    QColor MakePrettyResourceStatusColor(const QString& status);
    QString MakeShortResourceId(const QString& id);
    QString MakePrettyPendingActionText(const QString& origionalAction);
    QColor MakePrettyPendingActionColor(const QString& action);
    QString MakePrettyPendingReasonTooltip(const QString& pendingReason);
}
