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

#ifndef __EMSTUDIO_STATEFILTERSELECTIONDIALOG_H
#define __EMSTUDIO_STATEFILTERSELECTIONDIALOG_H

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include "../StandardPluginsConfig.h"
#include <QDialog>
#include <QTableWidget>
#include <QTableWidgetItem>

EMFX_FORWARD_DECLARE(AnimGraphNodeGroup);


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;

    class StateFilterSelectionWindow
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(StateFilterSelectionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        StateFilterSelectionWindow(AnimGraphPlugin* plugin, QWidget* parent);
        ~StateFilterSelectionWindow();

        void ReInit(EMotionFX::AnimGraph* animGraph, const MCore::Array<MCore::String>& oldNodeSelection, const MCore::Array<MCore::String>& oldGroupSelection);

        const MCore::Array<MCore::String>& GetSelectedNodeNames() const                 { return mSelectedNodeNames; }
        const MCore::Array<MCore::String>& GetSelectedGroupNames() const                    { return mSelectedGroupNames; }

    protected slots:
        void OnSelectionChanged();

    private:
        struct WidgetLookup
        {
            MCORE_MEMORYOBJECTCATEGORY(StateFilterSelectionWindow::WidgetLookup, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
            QTableWidgetItem*   mWidget;
            MCore::String       mName;
            bool                mIsGroup;

            WidgetLookup(QTableWidgetItem* widget, const char* name, bool isGroup)
            {
                mWidget     = widget;
                mName       = name;
                mIsGroup    = isGroup;
            }
        };

        EMotionFX::AnimGraphNodeGroup* FindGroupByWidget(QTableWidgetItem* widget) const;
        EMotionFX::AnimGraphNode* FindNodeByWidget(QTableWidgetItem* widget) const;
        void AddRow(uint32 rowIndex, const char* name, bool isGroup, bool isSelected, const MCore::RGBAColor& color = MCore::RGBAColor(1.0f, 1.0f, 1.0f));

        MCore::Array<WidgetLookup>  mWidgetTable;
        MCore::Array<MCore::String> mSelectedGroupNames;
        MCore::Array<MCore::String> mSelectedNodeNames;
        AnimGraphPlugin*           mPlugin;
        QTableWidget*               mTableWidget;
        EMotionFX::AnimGraph*      mAnimGraph;
    };
} // namespace EMStudio

#endif
