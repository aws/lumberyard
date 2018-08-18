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

#ifndef __EMSTUDIO_NAVIGATIONLINKWIDGET_H
#define __EMSTUDIO_NAVIGATIONLINKWIDGET_H

//
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include <MysticQt/Source/LinkWidget.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include "AnimGraphPlugin.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QToolButton>
#include <QDialog>
#include <QListWidget>

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    class NavigationLinkWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigationLinkWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        /**
         * Constructor.
         */
        NavigationLinkWidget(AnimGraphPlugin* plugin, QWidget* parent);

        /**
         * Destructor.
         */
        ~NavigationLinkWidget();

        void Update(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node);

    private slots:
        void OnHierarchyNavigationLinkClicked(const QString& text);
        void StartSearchMode()                                          { OnModeChanged(true); }
        void DropDownChildren();
        void DropDownHistory();
        void OnShowNode();

    private:
        QWidget* AddNodeToHierarchyNavigationLink(EMotionFX::AnimGraphNode* node, QHBoxLayout* hLayout);
        void focusOutEvent(QFocusEvent* event);
        void OnModeChanged(bool searchMode);

        // navigation bar
        EMotionFX::AnimGraph*              mAnimGraph;
        EMotionFX::AnimGraphNode*          mNode;

        bool                                mLastSearchMode;
        QWidget*                            mInnerWidget;
        QWidget*                            mNavigationLink;
        QHBoxLayout*                        mLinksLayout;
        AzQtComponents::FilteredSearchWidget* m_searchWidget;
        AnimGraphPlugin*                   mPlugin;
    };


    class NavigationLinkDropdownHistory
        : public QDialog
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigationLinkDropdownHistory, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        /**
         * Constructor.
         */
        NavigationLinkDropdownHistory(NavigationLinkWidget* parentWidget, AnimGraphPlugin* plugin);

        /**
         * Destructor.
         */
        ~NavigationLinkDropdownHistory();

    private slots:
        void OnShowNode(QListWidgetItem* item);

    private:
        AnimGraphPlugin* mPlugin;
    };
} // namespace EMStudio


#endif
