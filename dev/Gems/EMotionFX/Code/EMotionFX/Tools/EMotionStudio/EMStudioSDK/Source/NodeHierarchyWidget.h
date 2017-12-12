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

#ifndef __EMSTUDIO_NODEHIERARCHYWIDGET_H
#define __EMSTUDIO_NODEHIERARCHYWIDGET_H

// include MCore
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "EMStudioConfig.h"
#include <MysticQt/Source/SearchButton.h>
#include <MysticQt/Source/ButtonGroup.h>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

struct EMSTUDIO_API SelectionItem
{
    uint32          mActorInstanceID;
    //MCore::String mNodeName;
    uint32          mNodeNameID;

    MCORE_INLINE void SetNodeName(const char* nodeName)     { mNodeNameID = MCore::GetStringIDGenerator().GenerateIDForString(nodeName); }
    MCORE_INLINE const char* GetNodeName() const                { return MCore::GetStringIDGenerator().GetName(mNodeNameID).AsChar(); }
    MCORE_INLINE const MCore::String& GetNodeNameString() const { return MCore::GetStringIDGenerator().GetName(mNodeNameID); }
};

namespace EMStudio
{
    class EMSTUDIO_API NodeHierarchyWidget
        : public QWidget
    {
        Q_OBJECT
                       MCORE_MEMORYOBJECTCATEGORY(NodeHierarchyWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        NodeHierarchyWidget(QWidget* parent, bool useSingleSelection);
        virtual ~NodeHierarchyWidget();

        void SetSelectionMode(bool useSingleSelection);
        void Update(uint32 actorInstanceID, CommandSystem::SelectionList* selectionList = nullptr);
        void Update(const MCore::Array<uint32>& actorInstanceIDs, CommandSystem::SelectionList* selectionList = nullptr);
        void FireSelectionDoneSignal();
        MCORE_INLINE QTreeWidget* GetTreeWidget()                                                               { return mHierarchy; }
        MCORE_INLINE MysticQt::SearchButton* GetSearchButton()                                                  { return mFindWidget; }

        // is node shown in the hierarchy widget?
        bool CheckIfNodeVisible(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        bool CheckIfNodeVisible(const MCore::String& nodeName, bool isMeshNode, bool isBone, bool isNode);

        // this calls UpdateSelection() and then returns the member array containing the selected items
        MCore::Array<SelectionItem>& GetSelectedItems();

        MCore::String GetFilterString()                                                                         { return FromQtString(mFindWidget->GetSearchEdit()->text()); }
        bool GetDisplayNodes() const                                                                            { return mDisplayNodesButton->isChecked(); }
        bool GetDisplayBones() const                                                                            { return mDisplayBonesButton->isChecked(); }
        bool GetDisplayMeshes() const                                                                           { return mDisplayMeshesButton->isChecked(); }

        QPushButton* GetDisplayNodesButton()                                                                    { return mDisplayNodesButton; }
        QPushButton* GetDisplayBonesButton()                                                                    { return mDisplayBonesButton; }
        QPushButton* GetDisplayMeshesButton()                                                                   { return mDisplayMeshesButton; }

        bool CheckIfNodeSelected(const char* nodeName, uint32 actorInstanceID);
        bool CheckIfActorInstanceSelected(uint32 actorInstanceID);

    signals:
        void OnSelectionDone(MCore::Array<SelectionItem> selectedNodes);
        void OnDoubleClicked(MCore::Array<SelectionItem> selectedNodes);

    public slots:
        //void OnVisibilityChanged(bool isVisible);
        void Update();
        void UpdateSelection();
        void ItemDoubleClicked(QTreeWidgetItem* item, int column);
        void TreeContextMenu(const QPoint& pos);
        void TextChanged(const QString& text);

    private:
        /*MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);

        CommandSelectCallback*              mSelectCallback;
        CommandUnselectCallback*            mUnselectCallback;
        CommandClearSelectionCallback*      mClearSelectionCallback;*/

        void RecursivelyAddChilds(QTreeWidgetItem* parent, EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node);
        void AddActorInstance(EMotionFX::ActorInstance* actorInstance);

        QTreeWidget*                        mHierarchy;
        MysticQt::ButtonGroup*              mDisplayButtonGroup;
        QPushButton*                        mDisplayBonesButton;
        QPushButton*                        mDisplayNodesButton;
        QPushButton*                        mDisplayMeshesButton;
        MysticQt::SearchButton*             mFindWidget;
        QIcon*                              mBoneIcon;
        QIcon*                              mNodeIcon;
        QIcon*                              mMeshIcon;
        QIcon*                              mCharacterIcon;
        MCore::Array<uint32>                mBoneList;
        MCore::String                       mFindString;
        MCore::Array<uint32>                mActorInstanceIDs;
        MCore::String                       mTempString;
        MCore::String                       mItemName;
        MCore::String                       mActorInstanceIDString;

        void ConvertFromSelectionList(CommandSystem::SelectionList* selectionList);
        void RemoveNodeFromSelectedNodes(const char* nodeName, uint32 actorInstanceID);
        void RemoveActorInstanceFromSelectedNodes(uint32 actorInstanceID);
        void AddNodeToSelectedNodes(const char* nodeName, uint32 actorInstanceID);
        void RecursiveRemoveUnselectedItems(QTreeWidgetItem* item);
        MCore::Array<SelectionItem>         mSelectedNodes;

        bool                                mUseSingleSelection;
    };
} // namespace EMStudio

#endif
