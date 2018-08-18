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

#ifndef __EMSTUDIO_BLENDNODESELECTIONWINDOW_H
#define __EMSTUDIO_BLENDNODESELECTIONWINDOW_H

// include MCore
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include "AnimGraphHierarchyWidget.h"
#include <QDialog>



namespace EMStudio
{
    /*
     * How to use this dialog?
     * 1. Use the rejected() signal to catch when the X on the window or the cancel button is pressed.
     * 2. Use the itemSelectionChanged() signal of the GetNodeHierarchyWidget()->GetTreeWidget() to detect when the user adjusts the selection in the node hierarchy widget.
     * 3. Use the OnSelectionDone() in the GetNodeHierarchyWidget() to detect when the user finished selecting and pressed the OK button.
     * Example:
     * connect( mNodeSelectionWindow,                                               SIGNAL(rejected()),             this, SLOT(UserWantsToCancel_1()) );
     * connect( mNodeSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(),    SIGNAL(itemSelectionChanged()), this, SLOT(SelectionChanged_2()) );
     * connect( mNodeSelectionWindow->GetNodeHierarchyWidget(),                     SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(FinishedSelectionAndPressedOK_3(MCore::Array<SelectionItem>)) );
    */
    class BlendNodeSelectionWindow
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(BlendNodeSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        BlendNodeSelectionWindow(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList = nullptr, const AZ::TypeId& visibilityFilterNode = AZ::TypeId::CreateNull(), bool showStatesOnly = false);
        virtual ~BlendNodeSelectionWindow();

        MCORE_INLINE AnimGraphHierarchyWidget* GetAnimGraphHierarchyWidget()                                          { return mHierarchyWidget; }
        void Update(uint32 animGraphID, CommandSystem::SelectionList* selectionList = nullptr)                             { mHierarchyWidget->Update(animGraphID, selectionList); }

    public slots:
        void OnAccept();
        void OnNodeSelected(MCore::Array<AnimGraphSelectionItem> selection);

    private:
        AnimGraphHierarchyWidget*          mHierarchyWidget;
        QPushButton*                        mOKButton;
        QPushButton*                        mCancelButton;
        bool                                mUseSingleSelection;
    };
} // namespace EMStudio

#endif
