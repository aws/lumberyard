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

#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/CommandManager.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <QWidget>
#include <MysticQt/Source/SearchButton.h>


QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QContextMenuEvent)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class NodeGraph;


    class NavigateWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigateWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

        friend class AnimGraphPlugin;

    public:
        enum
        {
            COLUMN_NAME             = 0,
            COLUMN_TYPE             = 1
        };

        NavigateWidget(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~NavigateWidget();

        void RemoveTreeRow(const QString& name);
        void Rename(const char* oldName, const char* newName);

        MCORE_INLINE QTreeWidget* GetTreeWidget()   { return mTreeWidget; }
        MCORE_INLINE QTreeWidgetItem* GetRootItem() { return mRootItem; }

        QTreeWidgetItem* FindItem(const QString& name);

        void ShowGraph(const char* nodeName, bool updateInterface);
        void ShowGraph(EMotionFX::AnimGraphNode* node, bool updateInterface);

        static QTreeWidgetItem* InsertNode(QTreeWidget* treeWidget, QTreeWidgetItem* parentItem, EMotionFX::AnimGraphNode* node, bool recursive, uint32 visibilityFilterNodeID = MCORE_INVALIDINDEX32, bool showStatesOnly = false, const char* searchFilterString = nullptr);

        // returns the root item
        static QTreeWidgetItem* UpdateTreeWidget(QTreeWidget* treeWidget, EMotionFX::AnimGraph* animGraph, uint32 visibilityFilterNodeID = MCORE_INVALIDINDEX32, bool showStatesOnly = false, const char* searchFilterString = nullptr);

        void UpdateTreeWidget(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* nodeToShow = nullptr);

        EMotionFX::AnimGraphNode* GetSingleSelectedNode();
        MCore::Array<EMotionFX::AnimGraphNode*> GetSelectedNodes(EMotionFX::AnimGraph* animGraph);
        bool GetIsReadyForPaste();

        void SetVisualOptionsNode(EMotionFX::AnimGraphNode* node)      { mVisualOptionsNode = node; }


    public slots:
        void NavigateToNode(QTreeWidgetItem* item, int column);
        void OnSelectionChangedWithoutGraphUpdate();
    public slots:
        void OnSelectionChanged();
        void OnItemClicked(QTreeWidgetItem* item, int column);
        void OnContextMenuRename();
        void OnDeleteSelectedNodes();
        void OnActivateState();
        void OnSetAsEntryState();
        void OnAddWildCardTransition();
        void OnVisualizeOptions();
        void OnVisualizeColorChanged(const QColor& color);
        void NavigateToNode();
        void MakeVirtualFinalNode();
        void RestoreVirtualFinalNode();
        void EnableSelected()                               { SetEnabledSelectedNodes(true); }
        void DisableSelected()                              { SetEnabledSelectedNodes(false); }
        void Cut();
        void Copy();
        void Paste();
        void OnFilterStringChanged(const QString& text);

    private:
        QTreeWidget*        mTreeWidget;
        AnimGraphPlugin*    mPlugin;
        QTreeWidgetItem*    mItemClicked;
        QString             mItemClickedName;
        QTreeWidgetItem*    mVisibleItem;
        QTreeWidgetItem*    mRootItem;
        EMotionFX::AnimGraphNode*      mVisualColorNode;
        EMotionFX::AnimGraphNode*      mVisualOptionsNode;
        MysticQt::SearchButton*         mSearchFilter;
        MCore::String       mSearchString;

        void SetEnabledSelectedNodes(bool flag);
        void FillPasteCommandGroup(bool cutMode);
        MCore::CommandGroup                         mPasteCommandGroup;
        MCore::CommandGroup                         mPasteCommandGroupNoConnections;
        AZStd::vector<AZStd::string>                mNodeNamesToCopy;
        AZStd::vector<AZStd::string>                mNodeNamesToCopyNoConnections;
        uint32                                      mCopyParentNodeTypeID;
        bool                                        mCutMode;

        void ChangeVisibleItem(EMotionFX::AnimGraphNode* node);

        void contextMenuEvent(QContextMenuEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
        void dropEvent(QDropEvent* event);
        void dragEnterEvent(QDragEnterEvent* event);
        void dragLeaveEvent(QDragLeaveEvent* event);
        void dragMoveEvent(QDragMoveEvent* event);
    };
} // namespace EMStudio