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

#include <QObject>
#include <QColor>

class QApplication;

namespace AzQtComponents
{
    class StylesheetPreprocessor;

    class AZ_QT_COMPONENTS_API LumberyardStylesheet
        : public QObject
    {
        Q_OBJECT

    public:
        LumberyardStylesheet(QObject* parent);
        ~LumberyardStylesheet();

        void Initialize(QApplication* application);

        void Refresh(QApplication* application);

        const QColor& GetColorByName(const QString& name);

    private:
        void InitializeFonts();
        void InitializeSearchPaths(QApplication* application);

        bool WriteStylesheetForQtDesigner(const QString& processedStyle);

        StylesheetPreprocessor* m_stylesheetPreprocessor;
    };
} // namespace AzQtComponents

