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

#include <QPointer>

class QSettings;
class QWidget;

namespace AzQtComponents
{
    class ScrollBarWatcher;
    class Style;

    /**
    * Class to provide extra functionality for working with ScrollBar controls.
    *
    * QScrollBar controls are styled in ScrollBar.qss
    *
    */
    class AZ_QT_COMPONENTS_API ScrollBar
    {
    public:
        struct Config
        {
        };

        /*!
        * Loads the config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default config data.
        */
        static Config defaultConfig();

    private:
        friend class Style;

        static void initializeWatcher();
        static void uninitializeWatcher();

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);

        static QPointer<ScrollBarWatcher> m_scrollBarWatcher;
    };

} // namespace AzQtComponents
