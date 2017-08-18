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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <platform.h>
#include <QMainWindow>
#include <IEditor.h>
#include <vector>
#include "Strings.h"
#include "Pointers.h"
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

class QViewport;
class QMainWindow;
class QSplitter;
class QToolButton;
class QTreeView;
class QPropertyTree;
class QBoxLayout;
class QToolBar;
class QDockWidget;
class QResizeEvent;
struct SRenderContext;
struct SViewportState;

namespace Serialization
{
    class IArchive;
}

namespace CharacterTool {
    using std::vector;
    using std::unique_ptr;

    struct ViewportPlaybackHotkeyConsumer;
    struct DisplayAnimationOptions;
    class AnimEventPresetPanel;
    class BlendSpacePreview;
    class DisplayParametersPanel;
    class DockWidgetManager;
    class ExplorerActionHandler;
    class ExplorerPanel;
    class PlaybackPanel;
    class PropertiesPanel;
    class SceneParametersPanel;
    class TransformPanel;
    class QSplitViewport;
    struct IViewportMode;
    struct DisplayOptions;
    struct ExplorerAction;
    struct ExplorerEntry;
    struct ViewportOptions;
    struct System;

    class CharacterToolForm
        : public QMainWindow
    {
        Q_OBJECT
    public:
        CharacterToolForm(QWidget* parent = 0);
        ~CharacterToolForm();

        // you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
        // make sure you pick a unique GUID
        static const GUID& GetClassID()
        {
            // {0E397A4C-B05F-4EC6-86EF-1CA10C9E4DA7}
            static const GUID guid =
            {
                0x0e397a4c, 0xb05f, 0x4ec6, {0x86, 0xef, 0x1c, 0xa1, 0x0c, 0x9e, 0x4d, 0xa7}
            };
            return guid;
        }

        void Serialize(Serialization::IArchive& ar);
        void ExecuteExplorerAction(const ExplorerAction& action, const vector<_smart_ptr<ExplorerEntry> >& entries);
    public slots:

        void OnFileSaveAll();
        void OnFileRecent();
        void OnFileRecentAboutToShow(QMenu* menu);
        void OnFileNewCharacter();
        void OnFileOpenCharacter();
        void OnLayoutReset();
        void OnLayoutSave();
        void OnLayoutSet();
        void OnLayoutRemove();

        void OnIdleUpdate();
        void OnPreRenderCompressed(const SRenderContext& context);
        void OnRenderCompressed(const SRenderContext& context);
        void OnPreRenderOriginal(const SRenderContext& context);
        void OnRenderOriginal(const SRenderContext& context);
        void OnViewportUpdate();
        void OnViewportOptionsChanged();
        void OnDisplayOptionsChanged(const DisplayOptions& displayOptions);
        void OnDisplayParametersButton();
        void OnExplorerSelectionChanged();
        void OnDockWidgetsChanged();
        void OnCharacterLoaded();

        void OnAnimEventPresetPanelPutEvent();

        bool TryToClose() { return true; }

        IViewportMode* ViewportMode() const{ return m_mode; }
        PlaybackPanel* GetPlaybackPanel() { return m_playbackPanel; }
    protected:
        bool event(QEvent* ev) override;
        void closeEvent(QCloseEvent* ev);
        void resizeEvent(QResizeEvent* ev) override;
        bool eventFilter(QObject* sender, QEvent* ev) override;

    private:
        using FilenameToEntryMapping = std::map<string, std::vector<ExplorerEntry*> >;
        void GenerateUnsavedFileTracking(FilenameToEntryMapping& unsavedFileTracking);

        void RunSaveAll();
        void RunSaveAll(AZStd::function<void(bool)> onComplete);
        void SaveState(const char* filename, bool layoutOnly);
        void LoadState(const char* filename, bool layoutOnly);
        void LoadLayout(const char* name);
        void SaveLayout(const char* name);
        void RemoveLayout(const char* name);
        void ResetLayout();
        void Initialize();
        void SplitExplorer(int explorerIndex);
        void UpdateLayoutMenu();
        void UpdatePanesMenu();
        void UpdateViewportMode(ExplorerEntry* newEntry);
        void InstallMode(IViewportMode* mode, ExplorerEntry* modeEntry);
        std::vector<string> FindLayoutNames();
        void CreateDefaultDockWidgets();
        void ReadViewportOptions(const ViewportOptions& options, const DisplayAnimationOptions& animationOptions);
        void UpdatePropertyToolBar();
        QString MakeUniqueExplorerName() const;
        struct SPrivate;
        System* m_system = nullptr;
        QScopedPointer<SPrivate> m_private;
        QSplitViewport* m_splitViewport = nullptr;
        int m_displayParametersSplitterWidths[2];
        QSplitter* m_displayParametersSplitter = nullptr;
        IViewportMode* m_mode = nullptr;
        QScopedPointer<IViewportMode> m_modeCharacter;
        ExplorerEntry* m_modeEntry = nullptr;

        std::vector<QDockWidget*> m_dockWidgets;
        PlaybackPanel* m_playbackPanel = nullptr;
        BlendSpacePreview* m_blendSpacePreview = nullptr;
        SceneParametersPanel* m_sceneParametersPanel = nullptr;
        DisplayParametersPanel* m_displayParametersPanel = nullptr;
        AnimEventPresetPanel* m_animEventPresetPanel = nullptr;
        QTreeView* m_characterTree = nullptr;
        QToolBar* m_modeToolBar = nullptr;
        QToolButton* m_displayParametersButton = nullptr;
        TransformPanel* m_transformPanel = nullptr;

        QMenu* m_menuView = nullptr;
        vector<string> m_recentCharacters;
        unique_ptr<ViewportPlaybackHotkeyConsumer> m_viewportPlaybackHotkeyConsumer;
        unique_ptr<DockWidgetManager> m_dockWidgetManager;
        unique_ptr<QPropertyTree> m_contentLayerPropertyTree;

        QAction* m_actionViewBindPose = nullptr;
        QAction* m_actionViewShowOriginalAnimation = nullptr;
        QAction* m_actionViewShowCompressionFlickerDiff = nullptr;

        QMenu* m_menuLayout = nullptr;
        QAction* m_actionLayoutReset = nullptr;
        QAction* m_actionLayoutLoadState = nullptr;
        QAction* m_actionLayoutSaveState = nullptr;

        bool m_stateLoaded;
        bool m_closed;
        bool m_isRunningAsyncSave;
    };

    void ShowCharacterToolForm();
}
