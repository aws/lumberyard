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

#ifndef __EMSTUDIO_TOOLBARPLUGIN_H
#define __EMSTUDIO_TOOLBARPLUGIN_H

// include MCore
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"
#include <QToolBar>
#include <QPointer>


namespace EMStudio
{
    /**
     *
     *
     */
    class EMSTUDIO_API ToolBarPlugin
        : public EMStudioPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ToolBarPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)
    public:
        ToolBarPlugin();
        virtual ~ToolBarPlugin();

        EMStudioPlugin::EPluginType GetPluginType() const override          { return EMStudioPlugin::PLUGINTYPE_TOOLBAR; }
        virtual void OnMainWindowClosed();

        virtual bool GetIsFloatable() const                                 { return true;  }
        virtual bool GetIsVertical() const                                  { return false; }
        virtual bool GetIsMovable() const                                   { return true;  }
        virtual Qt::ToolBarAreas GetAllowedAreas() const                    { return Qt::AllToolBarAreas; }
        virtual Qt::ToolButtonStyle GetToolButtonStyle() const              { return Qt::ToolButtonIconOnly; }

        virtual void SetInterfaceTitle(const char* name);
        virtual void CreateBaseInterface(const char* objectName) override;

        virtual QString GetObjectName() const override                      { AZ_Assert(!mBar.isNull(), "Unexpected null bar"); return mBar->objectName(); }
        virtual void SetObjectName(const QString& name) override            { GetToolBar()->setObjectName(name); }

        virtual bool GetHasWindowWithObjectName(const MCore::String& objectName) override;

        virtual Qt::ToolBarArea GetToolBarCreationArea() const              { return Qt::BottomToolBarArea; }

        QToolBar* GetToolBar();

    protected:
        QPointer<QToolBar>   mBar;
    };
}   // namespace EMStudio

#endif
