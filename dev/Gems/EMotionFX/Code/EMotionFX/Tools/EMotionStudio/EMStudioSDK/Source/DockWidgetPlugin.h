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

#ifndef __EMSTUDIO_DOCKWIDGETPLUGIN_H
#define __EMSTUDIO_DOCKWIDGETPLUGIN_H

// include MCore
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"
#include <MysticQt/Source/DockWidget.h>
#include <assert.h>


namespace EMStudio
{
    /**
     *
     *
     */
    class EMSTUDIO_API DockWidgetPlugin
        : public EMStudioPlugin
    {
        MCORE_MEMORYOBJECTCATEGORY(DockWidgetPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        DockWidgetPlugin();
        virtual ~DockWidgetPlugin();

        EMStudioPlugin::EPluginType GetPluginType() const override              { return EMStudioPlugin::PLUGINTYPE_DOCKWIDGET; }

        virtual bool GetIsClosable() const                                  { return true; }
        virtual bool GetIsFloatable() const                                 { return true;  }
        virtual bool GetIsVertical() const                                  { return false; }
        virtual bool GetIsMovable() const                                   { return true;  }

        virtual void SetInterfaceTitle(const char* name);
        virtual void CreateBaseInterface(const char* objectName) override;

        virtual QString GetObjectName() const override                          { assert(mDock); return mDock->objectName(); }
        virtual void SetObjectName(const QString& name) override                    { mDock->setObjectName(name); }

        virtual QSize GetInitialWindowSize() const                      { return QSize(500, 650); }

        virtual bool GetHasWindowWithObjectName(const MCore::String& objectName) override;

        MCORE_INLINE MysticQt::DockWidget* GetDockWidget()              { return mDock; }

    protected:
        MysticQt::DockWidget*   mDock;
    };
}   // namespace EMStudio

#endif
