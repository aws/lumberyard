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

#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/Source/AnimGraph.h>
#include "../StandardPluginsConfig.h"
#include <MysticQt/Source/SearchButton.h>
#include <MysticQt/Source/ButtonGroup.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <QDialog>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QLineEdit)


struct AnimGraphSelectionItem
{
    uint32          mAnimGraphID;
    AZStd::string   mNodeName;
};

namespace EMStudio
{
    class AnimGraphHierarchyWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphHierarchyWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        AnimGraphHierarchyWidget(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList = nullptr, uint32 visibilityFilterNodeID = MCORE_INVALIDINDEX32, bool showStatesOnly = false);
        virtual ~AnimGraphHierarchyWidget();

        void SetSelectionMode(bool useSingleSelection);
        void Update(uint32 animGraphID, CommandSystem::SelectionList* selectionList = nullptr);
        void FireSelectionDoneSignal();
        MCORE_INLINE QTreeWidget* GetTreeWidget()                                                               { return mHierarchy; }
        MCORE_INLINE MysticQt::SearchButton* GetSearchButton()                                                  { return mFindWidget; }

        // Calls UpdateSelection() and then returns the member array containing the selected items.
        AZStd::vector<AnimGraphSelectionItem>& GetSelectedItems();

    signals:
        void OnSelectionDone(AZStd::vector<AnimGraphSelectionItem> selectedNodes);

    public slots:
        void Update();
        void UpdateSelection();
        void ItemDoubleClicked(QTreeWidgetItem* item, int column);
        void TextChanged(const QString& text);

    private:
        QTreeWidget*                            mHierarchy;
        MysticQt::SearchButton*                 mFindWidget;
        MCore::String                           mFindString;
        uint32                                  mFilterNodeID;
        uint32                                  mAnimGraphID;
        bool                                    mShowStatesOnly;
        AZStd::vector<AnimGraphSelectionItem>   mSelectedNodes;
        CommandSystem::SelectionList*           mCurrentSelectionList;
        bool                                    mUseSingleSelection;
    };
} // namespace EMStudio