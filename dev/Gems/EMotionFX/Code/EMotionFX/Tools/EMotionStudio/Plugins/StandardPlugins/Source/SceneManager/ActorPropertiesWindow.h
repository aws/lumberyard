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

#include "../StandardPluginsConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include "CollisionMeshesSetupWindow.h"
#include <MysticQt/Source/LinkWidget.h>
#include <QWidget>
#include <QLabel>
#include <QPushButton>

QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace EMStudio
{
    // forward declaration
    class SceneManagerPlugin;
    class MirrorSetupWindow;
    class RetargetSetupWindow;

    class ActorPropertiesWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ActorPropertiesWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000005
        };

        ActorPropertiesWindow(QWidget* parent, SceneManagerPlugin* plugin);
        ~ActorPropertiesWindow();

        void Init();

        // helper functions
        static void GetNodeName(const MCore::Array<SelectionItem>& selection, AZStd::string* outNodeName, uint32* outActorID);

    public slots:
        void UpdateInterface();

        // actor name
        void NameEditChanged();

        // motion extraction node slots
        void OnSelectMotionExtractionNode();
        void OnMotionExtractionNodeSelected(MCore::Array<SelectionItem> selection);
        void ResetMotionExtractionNode();
        void OnSelectRetargetRootNode();
        void OnRetargetRootNodeSelected(MCore::Array<SelectionItem> selection);
        void ResetRetargetRootNode();
        void OnFindBestMatchingNode();
        void OnMirrorSetup();
        //void OnRetargetSetup();
        void OnCollisionMeshesSetup();

        // nodes excluded from bounding volume calculations
        void OnSelectExcludedNodes();
        void OnSelectExcludedNodes(MCore::Array<SelectionItem> selection);
        void OnCancelExcludedNodes();
        void OnExcludeNodeSelectionChanged();
        void ResetExcludedNodes();

    private:
        // helper functions
        EMotionFX::Node* GetNode(NodeHierarchyWidget* hierarchyWidget);
        void ResetNode(const char* parameterNodeType);
        void SetToOldExcludeNodeSelection();

        static void PrepareExcludedNodeSelectionList(EMotionFX::Actor* actor, CommandSystem::SelectionList* outSelectionList);

        MCORE_INLINE const char* GetDefaultNodeSelectionLabelText()                             { return "Click to select node"; }
        MCORE_INLINE const char* GetDefaultExcludeBoundNodesLabelText()                         { return "Click to select nodes"; }
        MCORE_INLINE const char* GetDefaultCollisionMeshesLabelText()                           { return "Click to setup"; }

        uint32 CalcNumExcludedNodesFromBounds(EMotionFX::Actor* actor);

        // motion extraction node
        QPushButton*                    mResetMotionExtractionNodeButton;
        MysticQt::LinkWidget*           mMotionExtractionNode;
        QPushButton*                    mFindBestMatchButton;

        // retarget root node
        QPushButton*                    mResetRetargetRootNodeButton;
        MysticQt::LinkWidget*           mRetargetRootNode;

        // nodes excluded from bounding volume calculations
        MysticQt::LinkWidget*           mExcludedNodesLink;
        QPushButton*                    mResetExcludedNodesButton;
        NodeSelectionWindow*            mExcludedNodesSelectionWindow;
        CommandSystem::SelectionList    mExcludedNodeSelectionList;
        MCore::Array<uint32>            mOldExcludedNodeSelection;

        MysticQt::LinkWidget*           mCollisionMeshesSetupLink;
        CollisionMeshesSetupWindow*     mCollisionMeshesSetupWindow;

        MysticQt::LinkWidget*           mMirrorSetupLink;
        MirrorSetupWindow*              mMirrorSetupWindow;

        //MysticQt::LinkWidget*         mRetargetSetupLink;
        //RetargetSetupWindow*          mRetargetSetupWindow;

        // actor name
        QLineEdit*                      mNameEdit;

        SceneManagerPlugin*             mPlugin;
        EMotionFX::Actor*               mActor;
        EMotionFX::ActorInstance*       mActorInstance;
        CommandSystem::SelectionList*   mSelectionList;
    };
} // namespace EMStudio