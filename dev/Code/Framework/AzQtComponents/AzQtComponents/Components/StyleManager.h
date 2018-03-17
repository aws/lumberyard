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
    class StyleSheetCache;
    class StylesheetPreprocessor;
    class TitleBarOverdrawHandler;

    /**
     * Wrapper around classes dealing with Lumberyard style.
     *
     * New applications should work like this:
     *   
     *   int main(int argv, char **argc)
     *   {
     *           QApplication app(argv, argc);
     *
     *           AzQtComponents::StyleManager styleManager(&app);
     *           styleManager.Initialize(&app);
     *           .
     *           .
     *           .
     *   }
     *
     */
    class AZ_QT_COMPONENTS_API StyleManager
        : public QObject
    {
        Q_OBJECT

    public:
        StyleManager(QObject* parent);
        ~StyleManager() override;

        /*!
        * Call to initialize the StyleManager, allowing it to hook into the application and apply the global style
        */
        void Initialize(QApplication* application, bool useUI10 = true);

        /*!
        * Switches the UI between 2.0 to 1.0
        */
        void SwitchUI(QApplication* application, bool useUI10);

        /*!
        * Call this to force a refresh of the global stylesheet and a reload of any settings files.
        * Note that you should never need to do this manually.
        */
        void Refresh(QApplication* application);

        /*!
        * Used to get a global color value by name.
        * Deprecated; do not use.
        */
        const QColor& GetColorByName(const QString& name);

    private:
        void InitializeFonts();
        void InitializeSearchPaths(QApplication* application);

        StylesheetPreprocessor* m_stylesheetPreprocessor = nullptr;
        StyleSheetCache* m_stylesheetCache = nullptr;
        TitleBarOverdrawHandler* m_titleBarOverdrawHandler = nullptr;
    };
} // namespace AzQtComponents

