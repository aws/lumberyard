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

#ifndef __EMSTUDIO_BLENDGRAPHVIEWWIDGET_H
#define __EMSTUDIO_BLENDGRAPHVIEWWIDGET_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include <MysticQt/Source/SearchButton.h>
#include "NavigationLinkWidget.h"
#include "../StandardPluginsConfig.h"
#include "BlendGraphWidget.h"
#include <QAction>
#include <QWidget>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/functional.h>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QMenuBar)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSettings)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class AnimGraphNodeWidget;

    //
    class BlendGraphViewWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendGraphViewWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        enum EOptionFlag
        {
            SELECTION_ALIGNLEFT                 = 0,
            SELECTION_ALIGNRIGHT                = 1,
            SELECTION_ALIGNTOP                  = 2,
            SELECTION_ALIGNBOTTOM               = 3,
            FILE_OPENFILE                       = 4,
            FILE_OPEN                           = 5,
            FILE_SAVE                           = 6,
            FILE_SAVEAS                         = 7,
            NAVIGATION_FORWARD                  = 8,
            NAVIGATION_BACK                     = 9,
            NAVIGATION_ROOT                     = 10,
            NAVIGATION_PARENT                   = 11,
            NAVIGATION_OPENSELECTEDNODE         = 12,
            SELECTION_ZOOMSELECTION             = 13,
            SELECTION_ZOOMALL                   = 14,
            WINDOWS_PARAMETERWINDOW             = 15,
            WINDOWS_HIERARCHYWINDOW             = 16,
            WINDOWS_ATTRIBUTEWINDOW             = 17,
            WINDOWS_PALETTEWINDOW               = 18,
            WINDOWS_GAMECONTROLLERWINDOW        = 19,
            SELECTION_ACTIVATESTATE             = 20,
            //SELECTION_SHOWPROCESSED               = 21,
            SELECTION_SETASENTRYNODE            = 21,
            SELECTION_CUT                       = 22,
            SELECTION_COPY                      = 23,
            SELECTION_SELECTALL                 = 24,
            SELECTION_UNSELECTALL               = 25,
            SELECTION_DELETENODES               = 26,
            SELECTION_ADDWILDCARDTRANSITION     = 27,
            WINDOWS_NODEGROUPWINDOW             = 28,
            VISUALIZATION_PLAYSPEEDS            = 29,
            VISUALIZATION_GLOBALWEIGHTS         = 30,
            VISUALIZATION_SYNCSTATUS            = 31,
            VISUALIZATION_PLAYPOSITIONS         = 32,

            NUM_OPTIONS //automatically gets the next number assigned
        };

        BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget);
        ~BlendGraphViewWidget();

        MCORE_INLINE bool GetOptionFlag(EOptionFlag option)         { return mActions[(uint32)option]->isChecked(); }
        void SetOptionFlag(EOptionFlag option, bool isEnabled);
        void SetOptionEnabled(EOptionFlag option, bool isEnabled);

        void Init(BlendGraphWidget* blendGraphWidget);
        void Update();

        // If there is a specific widget to handle this node returns that.
        // Else, returns nullptr.
        AnimGraphNodeWidget* GetWidgetForNode(const EMotionFX::AnimGraphNode* node);

        void SetCurrentNode(EMotionFX::AnimGraphNode* node);

    public slots:
        void UpdateWindowVisibility();

        void AlignLeft();
        void AlignRight();
        void AlignTop();
        void AlignBottom();

        void NavigateForward();
        void NavigateBackward();
        void NavigateToRoot();
        void NavigateToNode();
        void NavigateToParent();

        void ZoomSelected();
        void ZoomAll();

        void SelectAll();
        void UnselectAll();

        //void OnShowProcessed();
        void OnDisplayPlaySpeeds();
        void OnDisplayGlobalWeights();
        void OnDisplaySyncStatus();
        void OnDisplayPlayPositions();

    protected:
        void SaveOptions(QSettings* settings);
        void LoadOptions(QSettings* settings);

        void Reset();
        void CreateEntry(QMenu* menu, QHBoxLayout* toolbarLayout, const char* entryName, const char* toolbarIconFileName, bool addToToolbar, bool checkable, int32 actionIndex, const QKeySequence& shortcut = 0, bool border = true, bool addToMenu = true);
        void AddSeparator();

        void AlignNodes(uint32 mode);

        void keyReleaseEvent(QKeyEvent* event);
        void keyPressEvent(QKeyEvent* event);

    private:
        QMenuBar*               mMenu;
        QHBoxLayout*            mToolbarLayout;
        QAction*                mActions[NUM_OPTIONS];
        QPushButton*            mToolbarButtons[NUM_OPTIONS];
        AnimGraphPlugin*        mParentPlugin;
        NavigationLinkWidget*   mNavigationLink;
        QStackedWidget          mViewportStack;
        //SearchButton*         mSearchButton;

        // This maps a node's TYPE_ID to a widget that will be used to
        // display the "contents" of that node type.  If no entry for a given
        // node type is found, then a BlendGraphWidget is used by default.
        // For normal blend trees and state machines, the BlendGraphWidget is
        // shown to draw the nodes inside the tree.  For special types like a
        // blendspace, a separate widget is registered to handle the drawing for that
        // node.
        AZStd::unordered_map<uint32, AnimGraphNodeWidget*> mNodeTypeToWidgetMap;
    };
}   // namespace EMStudio


#endif

