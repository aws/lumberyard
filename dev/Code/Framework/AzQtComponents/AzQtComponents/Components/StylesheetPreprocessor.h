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

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QColor>
#include <QHash>
#include <QObject>

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API StylesheetPreprocessor
        : public QObject
    {
        Q_OBJECT

    public:
        explicit StylesheetPreprocessor(QObject* pParent);
        ~StylesheetPreprocessor();

        void ClearVariables();
        void ReadVariables(const QString& variables);
        QString ProcessStyleSheet(const QString& stylesheetData);

        const QColor& GetColorByName(const QString& name);

    private:
        QHash<QString, QString> m_namedVariables;
        QHash<QString, QColor> m_cachedColors;

        QString GetValueByName(const QString& name);
    };
}
