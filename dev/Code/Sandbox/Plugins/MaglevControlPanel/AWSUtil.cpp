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

#include "AWSUtil.h"
#include <IEditor.h>

namespace AWSUtil
{
    namespace TextColor
    {
        static QColor s_default = "#FFFFFF";
        static QColor s_error = "#F44642";
        static QColor s_highlight = "#D9822E";
        static QColor s_positive = "#43D96A";
        static QColor s_warning = "#F0C32D";
    }

    QColor MakePrettyColor(const QString& colorName)
    {
        if (colorName == "Default")
        {
            return TextColor::s_default;
        }
        else if (colorName == "Error")
        {
            return TextColor::s_error;
        }
        else if (colorName == "Highlight")
        {
            return TextColor::s_highlight;
        }
        else if (colorName == "Positive")
        {
            return TextColor::s_positive;
        }
        else if (colorName == "Warning")
        {
            return TextColor::s_warning;
        }
        return QColor();
    }

    QString MakePrettyResourceStatusText(const QString& origionalStatus)
    {
        // Cloud Formation's documented status codes are:
        //
        // CREATE_IN_PROGRESS
        // CREATE_FAILED
        // CREATE_COMPLETE
        // ROLLBACK_IN_PROGRESS
        // ROLLBACK_FAILED
        // ROLLBACK_COMPLETE
        // DELETE_IN_PROGRESS
        // DELETE_FAILED
        // DELETE_COMPLETE
        // UPDATE_IN_PROGRESS
        // UPDATE_COMPLETE_CLEANUP_IN_PROGRESS
        // UPDATE_COMPLETE
        // UPDATE_ROLLBACK_IN_PROGRESS
        // UPDATE_ROLLBACK_FAILED
        // UPDATE_ROLLBACK_COMPLETE_CLEANUP_IN_PROGRESS
        // UPDATE_ROLLBACK_COMPLETE
        //

        QString prettyStatus = origionalStatus;
        prettyStatus = prettyStatus.replace("UPDATE_ROLLBACK", "ROLLBACK");
        prettyStatus = prettyStatus.replace("COMPLETE_CLEANUP_", "");
        prettyStatus = prettyStatus.replace('_', ' ');
        prettyStatus = prettyStatus.toLower();
        prettyStatus[0] = prettyStatus[0].toUpper();

        return prettyStatus;
    }

    QString MakePrettyResourceStatusTooltip(const QString& status, const QString& statusReason)
    {
        QString statusToolTip {};
        statusToolTip += "<span>";
        if (!statusReason.isEmpty())
        {
            statusToolTip += statusReason;
            statusToolTip += " ";
        }
        statusToolTip += "(";
        statusToolTip += status;
        statusToolTip += ")";
        statusToolTip += "</span>";
        return statusToolTip;
    }

    QColor MakePrettyResourceStatusColor(const QString& status)
    {
        QColor statusColor;
        if (status.contains("FAILED") || status.contains("ROLLBACK"))
        {
            statusColor = TextColor::s_error;
        }
        else if (status.contains("COMPLETE"))
        {
            statusColor = TextColor::s_positive;
        }
        else if (status.contains("IN_PROGRESS"))
        {
            statusColor = TextColor::s_warning;
        }
        else
        {
            // unknown/unexpected
            statusColor = TextColor::s_default;
        }
        return statusColor;
    }

    QString MakeShortResourceId(const QString& id)
    {
        return "..." + id.right(17);
    }

    QString MakePrettyPendingActionText(const QString& origionalAction)
    {
        QString prettyAction = origionalAction.toLower();
        prettyAction[0] = prettyAction[0].toUpper();
        return prettyAction;
    }

    QColor MakePrettyPendingActionColor(const QString& action)
    {
        QColor statusColor;
        if (!action.isEmpty())
        {
            statusColor = TextColor::s_warning;
        }
        else
        {
            // unneeded (no text)
            statusColor = TextColor::s_default;
        }
        return statusColor;
    }

    QString MakePrettyPendingReasonTooltip(const QString& pendingReason)
    {
        QString tooltip{};
        tooltip += pendingReason;
        return tooltip;
    }


}
