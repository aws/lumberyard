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

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "LogWindowPlugin.h"
#include "LogWindowCallback.h"
#include <MysticQt/Source/MysticQtManager.h>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>


namespace EMStudio
{
    // constructor
    LogWindowPlugin::LogWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mLogCallback = nullptr;
    }


    // destructor
    LogWindowPlugin::~LogWindowPlugin()
    {
        // remove the callback from the log manager (automatically deletes from memory as well)
        const uint32 index = MCore::GetLogManager().FindLogCallback(mLogCallback);
        if (index != MCORE_INVALIDINDEX32)
        {
            MCore::GetLogManager().RemoveLogCallback(index);
        }
    }


    // get the compile date
    const char* LogWindowPlugin::GetCompileDate() const
    {
        return MCORE_DATE;
    }


    // get the name
    const char* LogWindowPlugin::GetName() const
    {
        return "Log Window";
    }


    // get the plugin type id
    uint32 LogWindowPlugin::GetClassID() const
    {
        return LogWindowPlugin::CLASS_ID;
    }


    // get the creator name
    const char* LogWindowPlugin::GetCreatorName() const
    {
        return "MysticGD";
    }


    // get the version
    float LogWindowPlugin::GetVersion() const
    {
        return 1.0f;
    }


    // clone the log window
    EMStudioPlugin* LogWindowPlugin::Clone()
    {
        LogWindowPlugin* newPlugin = new LogWindowPlugin();
        return newPlugin;
    }


    // init after the parent dock window has been created
    bool LogWindowPlugin::Init()
    {
        // create the widget
        QWidget* windowWidget = new QWidget(mDock);

        // create the layout
        QVBoxLayout* windowWidgetLayout = new QVBoxLayout();
        windowWidgetLayout->setSpacing(3);
        windowWidgetLayout->setMargin(3);

        // create the filter button group
        mFilterButtonGroup = new MysticQt::ButtonGroup(nullptr, 1, 6);
        mFilterButtonGroup->GetButton(0, 0)->setText("Fatal");
        mFilterButtonGroup->GetButton(0, 1)->setText("Error");
        mFilterButtonGroup->GetButton(0, 2)->setText("Warning");
        mFilterButtonGroup->GetButton(0, 3)->setText("Info");
        mFilterButtonGroup->GetButton(0, 4)->setText("Detailed Info");
        mFilterButtonGroup->GetButton(0, 5)->setText("Debug");
        mFilterButtonGroup->GetButton(0, 0)->setChecked(true);
        mFilterButtonGroup->GetButton(0, 1)->setChecked(true);
        mFilterButtonGroup->GetButton(0, 2)->setChecked(true);
        mFilterButtonGroup->GetButton(0, 3)->setChecked(true);
    #ifdef MCORE_DEBUG
        mFilterButtonGroup->GetButton(0, 4)->setChecked(true);
        mFilterButtonGroup->GetButton(0, 5)->setChecked(true);
    #else
        mFilterButtonGroup->GetButton(0, 4)->setChecked(false);
        mFilterButtonGroup->GetButton(0, 5)->setChecked(false);
    #endif
        connect(mFilterButtonGroup, SIGNAL(ButtonPressed()), this, SLOT(OnFilterButtonPressed()));

        // create the spacer widget
        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        // create the find widget
        AzQtComponents::FilteredSearchWidget* searchWidget = new AzQtComponents::FilteredSearchWidget(windowWidget);
        connect(searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &LogWindowPlugin::OnTextFilterChanged);

        // create the filter layout
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->addWidget(new QLabel("Filter:"));
        topLayout->addWidget(mFilterButtonGroup);
        topLayout->addWidget(spacerWidget);
        topLayout->addWidget(searchWidget);
        topLayout->setSpacing(6);

        // add the filter layout
        windowWidgetLayout->addLayout(topLayout);

        // create and add the table and callback
        mLogCallback = new LogWindowCallback(nullptr);
        windowWidgetLayout->addWidget(mLogCallback);

        // set the layout
        windowWidget->setLayout(windowWidgetLayout);

        // set the table as content
        mDock->SetContents(windowWidget);

        // create the callback
        mLogCallback->SetLogLevels(MCore::LogCallback::LOGLEVEL_ALL);
        MCore::GetLogManager().AddLogCallback(mLogCallback);

        // return true because the plugin is correctly initialized
        return true;
    }


    // find changed
    void LogWindowPlugin::OnTextFilterChanged(const QString& text)
    {
        mLogCallback->SetFind(text);
    }


    // filter button pressed
    void LogWindowPlugin::OnFilterButtonPressed()
    {
        uint32 newFilter = 0;
        if (mFilterButtonGroup->GetIsButtonChecked(0, 0))
        {
            newFilter |= MCore::LogCallback::LOGLEVEL_FATAL;
        }
        if (mFilterButtonGroup->GetIsButtonChecked(0, 1))
        {
            newFilter |= MCore::LogCallback::LOGLEVEL_ERROR;
        }
        if (mFilterButtonGroup->GetIsButtonChecked(0, 2))
        {
            newFilter |= MCore::LogCallback::LOGLEVEL_WARNING;
        }
        if (mFilterButtonGroup->GetIsButtonChecked(0, 3))
        {
            newFilter |= MCore::LogCallback::LOGLEVEL_INFO;
        }
        if (mFilterButtonGroup->GetIsButtonChecked(0, 4))
        {
            newFilter |= MCore::LogCallback::LOGLEVEL_DETAILEDINFO;
        }
        if (mFilterButtonGroup->GetIsButtonChecked(0, 5))
        {
            newFilter |= MCore::LogCallback::LOGLEVEL_DEBUG;
        }
        mLogCallback->SetFilter(newFilter);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/LogWindow/LogWindowPlugin.moc>