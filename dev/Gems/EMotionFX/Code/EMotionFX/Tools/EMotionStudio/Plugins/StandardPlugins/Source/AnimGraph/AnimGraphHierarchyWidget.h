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
#include <EMotionFX/Source/AnimGraph.h>
#include "../StandardPluginsConfig.h"
#include <MysticQt/Source/ButtonGroup.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <QDialog>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QIcon)
QT_FORWARD_DECLARE_CLASS(QTreeWidget)
QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

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
        AnimGraphHierarchyWidget(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList = nullptr, const AZ::TypeId& visibilityFilterNodeType = AZ::TypeId::CreateNull(), bool showStatesOnly = false);
        virtual ~AnimGraphHierarchyWidget();

        void SetSelectionMode(bool useSingleSelection);
        void Update(uint32 animGraphID, CommandSystem::SelectionList* selectionList = nullptr);
        void FireSelectionDoneSignal();
        MCORE_INLINE QTreeWidget* GetTreeWidget()                                                               { return mHierarchy; }
        MCORE_INLINE AzQtComponents::FilteredSearchWidget* GetSearchWidget()                                    { return m_searchWidget; }

        // Calls UpdateSelection() and then returns the member array containing the selected items.
        AZStd::vector<AnimGraphSelectionItem>& GetSelectedItems();

    signals:
        void OnSelectionDone(AZStd::vector<AnimGraphSelectionItem> selectedNodes);

    public slots:
        void Update();
        void UpdateSelection();
        void ItemDoubleClicked(QTreeWidgetItem* item, int column);
        void OnTextFilterChanged(const QString& text);

    private:
        QTreeWidget*                            mHierarchy;
        AzQtComponents::FilteredSearchWidget*   m_searchWidget;
        AZStd::string                           m_searchWidgetText;
        AZ::TypeId                              mFilterNodeType;
        uint32                                  mAnimGraphID;
        bool                                    mShowStatesOnly;
        AZStd::vector<AnimGraphSelectionItem>   mSelectedNodes;
        CommandSystem::SelectionList*           mCurrentSelectionList;
        bool                                    mUseSingleSelection;
    };
} // namespace EMStudio