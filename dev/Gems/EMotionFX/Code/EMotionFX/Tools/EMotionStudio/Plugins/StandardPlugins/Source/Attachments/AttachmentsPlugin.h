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

#ifndef __EMSTUDIO_ATTACHMENTSPLUGIN_H
#define __EMSTUDIO_ATTACHMENTSPLUGIN_H

// include MCore
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DialogStack.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include "AttachmentsWindow.h"
#include "AttachmentsHierarchyWindow.h"
#include "AttachmentNodesWindow.h"

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace EMStudio
{
    /**
     *
     *
     */
    class AttachmentsPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
                           MCORE_MEMORYOBJECTCATEGORY(AttachmentsPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000017
        };

        AttachmentsPlugin();
        ~AttachmentsPlugin();

        // overloaded
        const char* GetCompileDate() const override         { return MCORE_DATE; }
        const char* GetName() const override                { return "Attachments"; }
        uint32 GetClassID() const override                  { return AttachmentsPlugin::CLASS_ID; }
        const char* GetCreatorName() const override         { return "Amazon"; }
        float GetVersion() const override                   { return 1.0f;  }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;
        void ReInit();
        AttachmentsWindow* GetAttachmentsWindow() const     { return mAttachmentsWindow; }

    public slots:
        void WindowReInit(bool visible);

    private:
        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAddAttachmentCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAddDeformableAttachmentCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveAttachmentCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearAttachmentsCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveActorInstanceCallback);

        CommandSelectCallback*                      mSelectCallback;
        CommandUnselectCallback*                    mUnselectCallback;
        CommandClearSelectionCallback*              mClearSelectionCallback;
        CommandAddAttachmentCallback*               mAddAttachmentCallback;
        CommandAddDeformableAttachmentCallback*     mAddDeformableAttachmentCallback;
        CommandRemoveAttachmentCallback*            mRemoveAttachmentCallback;
        CommandClearAttachmentsCallback*            mClearAttachmentsCallback;
        CommandAdjustActorCallback*                 mAdjustActorCallback;
        CommandRemoveActorInstanceCallback*         mRemoveActorInstanceCallback;

        QWidget*                                    mNoSelectionWidget;
        MysticQt::DialogStack*                      mDialogStack;
        AttachmentsWindow*                          mAttachmentsWindow;
        AttachmentsHierarchyWindow*                 mAttachmentsHierarchyWindow;
        AttachmentNodesWindow*                      mAttachmentNodesWindow;
    };
} // namespace EMStudio


#endif
