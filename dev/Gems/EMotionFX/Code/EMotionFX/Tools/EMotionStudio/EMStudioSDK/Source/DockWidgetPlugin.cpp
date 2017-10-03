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

// include the required headers
#include "EMStudioManager.h"
#include "MainWindow.h"
#include "DockWidgetPlugin.h"
#include <MysticQt/Source/DockHeader.h>
#include <MCore/Source/LogManager.h>


namespace EMStudio
{
    class RemovePluginOnCloseDockWidget
        : public MysticQt::DockWidget
    {
    public:
        RemovePluginOnCloseDockWidget(const QString& name, EMStudio::EMStudioPlugin* plugin)
            : MysticQt::DockWidget(name)
        {
            mPlugin = plugin;
        }

    protected:
        void closeEvent(QCloseEvent* event) override
        {
            MCORE_UNUSED(event);
            EMStudio::GetPluginManager()->RemoveActivePlugin(mPlugin);
            GetMainWindow()->UpdateCreateWindowMenu();
        }

    private:
        EMStudio::EMStudioPlugin* mPlugin;
    };


    // constructor
    DockWidgetPlugin::DockWidgetPlugin()
        : EMStudioPlugin()
    {
        mDock = nullptr;
    }


    // destructor
    DockWidgetPlugin::~DockWidgetPlugin()
    {
        if (mDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mDock);
            delete mDock;
        }
    }


    // check if we have a window that uses this object name
    bool DockWidgetPlugin::GetHasWindowWithObjectName(const MCore::String& objectName)
    {
        if (mDock == nullptr)
        {
            return false;
        }

        // check if the object name is equal to the one of the dock widget
        return (objectName.CheckIfIsEqual(FromQtString(mDock->objectName()).AsChar()));
    }


    // create the base interface
    void DockWidgetPlugin::CreateBaseInterface(const char* objectName)
    {
        // get the main window
        QMainWindow* mainWindow = (QMainWindow*)GetMainWindow();

        // create a window for the plugin
        mDock = new RemovePluginOnCloseDockWidget(GetName(), this);
        mDock->setAllowedAreas(Qt::AllDockWidgetAreas);

        // set the custom dock widget header
        MysticQt::DockHeader* titleBar = new MysticQt::DockHeader(mDock);
        mDock->setTitleBarWidget(titleBar);

        QDockWidget::DockWidgetFeatures features  = QDockWidget::NoDockWidgetFeatures;
        if (GetIsClosable())
        {
            features |= QDockWidget::DockWidgetClosable;
        }
        if (GetIsVertical())
        {
            features |= QDockWidget::DockWidgetVerticalTitleBar;
        }
        if (GetIsMovable())
        {
            features |= QDockWidget::DockWidgetMovable;
        }
        if (GetIsFloatable())
        {
            features |= QDockWidget::DockWidgetFloatable;
        }

        mDock->setFeatures(features);

        if (objectName == nullptr)
        {
            QString newName = GetPluginManager()->GenerateObjectName();
            SetObjectName(newName);
        }
        else
        {
            SetObjectName(objectName);
        }

        //  mDock->setFloating( true );
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mDock);

        titleBar->UpdateIcons();
    }


    // set the interface title
    void DockWidgetPlugin::SetInterfaceTitle(const char* name)
    {
        if (mDock)
        {
            mDock->setWindowTitle(name);
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.moc>