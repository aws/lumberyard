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

#ifndef __EMSTUDIO_NODEWINDOWPLUGIN_H
#define __EMSTUDIO_NODEWINDOWPLUGIN_H

// include MCore
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DialogStack.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <MysticQt/Source/PropertyWidget.h>


namespace EMStudio
{
    // forward declaration
    class NodeHierarchyWidget;

    /**
     *
     *
     */
    class NodeWindowPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
                           MCORE_MEMORYOBJECTCATEGORY(NodeWindowPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000357
        };

        NodeWindowPlugin();
        ~NodeWindowPlugin();

        // overloaded
        const char* GetCompileDate() const override         { return MCORE_DATE; }
        const char* GetName() const override                { return "Nodes"; }
        uint32 GetClassID() const override                  { return CLASS_ID; }
        const char* GetCreatorName() const override         { return "MysticGD"; }
        float GetVersion() const override                   { return 1.0f;  }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;
        void ReInit();

        void FillNodeInfo(EMotionFX::Node* node, EMotionFX::ActorInstance* actorInstance);
        void FillActorInfo(EMotionFX::ActorInstance* actorInstance);
        void FillMeshInfo(const char* groupName, EMotionFX::Mesh* mesh, EMotionFX::Node* node, EMotionFX::ActorInstance* actorInstance, int lodLevel, bool colMesh);

    public slots:
        void OnNodeChanged();
        void VisibilityChanged(bool isVisible);
        void FilterStringChanged(const QString& text);

        void UpdateVisibleNodeIndices();

    private:
        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);

        CommandSelectCallback*              mSelectCallback;
        CommandUnselectCallback*            mUnselectCallback;
        CommandClearSelectionCallback*      mClearSelectionCallback;

        MysticQt::DialogStack*              mDialogStack;
        NodeHierarchyWidget*                mHierarchyWidget;
        MysticQt::PropertyWidget*           mPropertyWidget;

        MCore::String                       mString;
        MCore::String                       mTempGroupName;
        MCore::Array<uint32>                mVisibleNodeIndices;
        MCore::Array<uint32>                mSelectedNodeIndices;
    };
} // namespace EMStudio


#endif
