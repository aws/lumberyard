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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINDIALOG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINDIALOG_H
#pragma once



#include "SequencerDopeSheet.h"

#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>

#include "MannKeyPropertiesDlgFE.h"
#include "MannequinNodes.h"
#include "MannequinBase.h"
#include "MannErrorReportDialog.h"

#include "Controls/FragmentBrowser.h"
#include "Controls/TransitionBrowser.h"
#include "Controls/FragmentEditorPage.h"
#include "Controls/TransitionEditorPage.h"
#include "Controls/PreviewerPage.h"
#include "FragmentSplitter.h"

#include <QMainWindow>

class QGroupBox;
class QTabWidget;

class CTagSelectionControl;
class CFragmentTagDefFileSelectionControl;
class CSequencerSplitter;
class CMannequinFileChangeWriter;
class CSequenceBrowser;

class CMannequinDialog
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    CMannequinDialog(QWidget* pParent = nullptr);
    virtual ~CMannequinDialog();

    static void RegisterViewClass();
    static const GUID& GetClassID();

    static CMannequinDialog* GetCurrentInstance() { return s_pMannequinDialog; }

    SMannequinContexts* Contexts() { return &m_contexts; }
    CFragmentEditorPage* FragmentEditor() { return m_wndFragmentEditorPage; }
    CTransitionEditorPage* TransitionEditor() { return m_wndTransitionEditorPage; }
    CPreviewerPage* Previewer() { return m_wndPreviewerPage; }
    CFragmentBrowser* FragmentBrowser() { return m_wndFragmentBrowser; }
    CTransitionBrowser* TransitionBrowser() { return m_wndTransitionBrowser; }

    bool PreviewFileLoaded() const { return m_bPreviewFileLoaded; }

    void Update();

    void OnLayerUpdate(int event, CObjectLayer* pLayer);

    void LoadNewPreviewFile(const char* previewFile);

    bool EnableContextData(const EMannequinEditorMode editorMode, const int scopeContext);
    void ClearContextData(const EMannequinEditorMode editorMode, const int scopeContextID);

    void EditFragment(FragmentID fragID, const SFragTagState& tagState, int contextID = -1);
    void EditFragmentByKey(const struct CFragmentKey& key, int contextID = -1);
    void StopEditingFragment();

    void FindFragmentReferences(const QString& fragmentName);
    void FindTagReferences(const QString& tagName);
    void FindClipReferences(const struct CClipKey& key);

    void EditTransitionByKey(const struct CFragmentKey& key, int contextID = -1);

    bool CheckChangedData();

    void OnMoveLocator(EMannequinEditorMode editorMode, uint32 refID, const char* locatorName, const QuatT& transform);

    void SetKeyProperty(const char* propertyName, const char* propertyValue);

    void SetTagState(const TagState& tagState);
    void UpdateTagList(FragmentID fragID = FRAGMENT_ID_INVALID);
    void SetTagStateOnCtrl(const SFragTagState& tagState);

    void ResavePreviewFile();

    template<typename T>
    int defaultTabPosition() { assert(0);  return -1; }

    template<typename T>
    void ShowPane()
    {
        QWidget* w = findDockWidget<T>();
        if (w)
        {
            w->show();
            w->raise();
        }
        else
        {
            showTab<T>();
        }
    }
    template<typename T>
    bool IsPaneSelected()
    {
        QWidget* w = findDockWidget<T>();
        if (w)
        {
            return !w->visibleRegion().isEmpty();
        }
        w = findChild<T>();
        if (w)
        {
            return w->isVisible();
        }
        return false;
    }
    template<typename T>
    QDockWidget* findDockWidget() { return findDockWidget(findChild<T>()); }

protected:
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void OnInitDialog();

    bool SavePreviewFile(const char* filename);
    bool LoadPreviewFile(const char* filename, XmlNodeRef& xmlSequenceNode);

    void EnableContextEditor(bool enable);

    bool InitialiseToPreviewFile(const char* previewFile);

    void PopulateTagList(const CTagDefinition* tagDef = NULL);
    void RefreshTagsPanel();
    SFragTagState GetTagStateFromCtrl() const;
    void OnInternalVariableChange(IVariable* pVar);

    void SaveLayouts();
    void LoadLayouts();

    void SaveMostRecentlyUsed();
    void LoadMostRecentlyUsed();
    bool LoadMostRecentPreview();

    void ShowLaunchDialog();

    void SetCursorPosText(float fTime);

    void PushMostRecentlyUsedEntry(const QString& fileName);

public:
    void Validate();
    void Validate(const SScopeContextData& contextDef);
    bool OpenPreview(const char* filename);

    void OnRender();
    void LaunchTagDefinitionEditor() { OnTagDefinitionEditor(); }

private:
    void OnDestroy();

private slots:
    void OnMenuNewPreview();
    void OnMenuLoadPreviewFile();
    void OnContexteditor();
    void OnAnimDBEditor();
    void OnTagDefinitionEditor();
    void OnSave();
    void OnReexportAll();
    void OnNewSequence();
    void OnLoadSequence();
    void OnSaveSequence();
    void OnViewFragmentEditor();
    void OnViewPreviewer();
    void OnViewTransitionEditor();
    void OnViewErrorReport();

    void OnUpdateActions();
    void OnUpdateNewSequence();
    void OnUpdateLoadSequence();
    void OnUpdateSaveSequence();
    void OnUpdateViewFragmentEditor();
    void OnUpdateViewPreviewer();
    void OnUpdateViewTransitionEditor();
    void OnUpdateViewErrorReport();

    void OnToolsListUsedAnimations();
    void OnToolsListUsedAnimationsForCurrentPreview();

    void OnMostRecentlyUsedEntryTriggered(QAction* action);
    void PopulateMostRecentlyUsed();

    void OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name);
    void OnTagRemoved(const CTagDefinition* tagDef, TagID tagID);
    void OnTagAdded(const CTagDefinition* tagDef, const QString& name);

private:
    void OnFragmentBrowserSelectedItemEdit(FragmentID fragmentId, const QString& tags);
    void OnFragmentBrowserScopeContextChanged();
    void UpdateAnimationDBEditorMenuItemEnabledState();

    void LoadMannequinFolder();

    void RefreshAndActivateErrorReport();

    void ClearContextEntities();
    void ClearContextViewData();
    void ReloadContextEntities();

    void UpdateForFragment(FragmentID fragmentID, const QString& tags);

    void addDockWidget(Qt::DockWidgetArea area, QWidget* widget, const QString& title, bool closable = true);
    void tabifyDockWidget(QWidget* widget1, QWidget* widget2);

    QDockWidget* findDockWidget(QWidget* widget);

    void showTab(QWidget* tab, QAction* ifHidden);
    void toggleTab(int index, QWidget* tab);

    template<typename T>
    void showTab()
    {
        QWidget* tab = findChild<T>();
        if (m_central->indexOf(tab) != -1)
        {
            m_central->setCurrentWidget(tab);
        }
        else
        {
            toggleTab<T>();
        }
    }

    template<typename T>
    void toggleTab() { toggleTab(defaultTabPosition<T>(), findChild<T>()); }

public:
    static const int s_minPanelSize;

private:
    QTabWidget*                 m_central;
    CFragmentEditorPage*        m_wndFragmentEditorPage;
    CTransitionEditorPage*      m_wndTransitionEditorPage;
    CPreviewerPage*             m_wndPreviewerPage;

    ReflectedPropertiesPanel* m_wndTagsPanel;
    CFragmentSplitter*                                          m_wndFragmentSplitter;
    CFragmentBrowser* m_wndFragmentBrowser;
    CSequenceBrowser* m_wndSequenceBrowser;
    CTransitionBrowser* m_wndTransitionBrowser;
    QGroupBox*                                                      m_wndFragmentsRollup;
    CMannErrorReportDialog*         m_wndErrorReport;

    QAction* m_newSequence;
    QAction* m_loadSequence;
    QAction* m_saveSequence;
    QAction* m_animDBEditor;
    QAction* m_contextEditor;
    QAction* m_listUsedAnimationsCurrentPreview;
    QAction* m_viewFragmentEditor;
    QAction* m_viewTransitionEditor;
    QAction* m_viewPreviewer;
    QAction* m_viewErrorReport;

    QStringList m_MostRecentlyUsedPreviews;

    SMannequinContexts m_contexts;
    bool m_bPreviewFileLoaded;

    std::unique_ptr<CMannequinFileChangeWriter> m_pFileChangeWriter;

    static CMannequinDialog* s_pMannequinDialog;
    const CTagDefinition* m_lastFragTagDef;
    bool m_bRefreshingTagCtrl;
    TSmartPtr<CVarBlock> m_tagVars;
    CTagControl m_tagControls;
    CTagControl m_fragTagControls;

    XmlNodeRef m_historyBackup;
    static bool s_bRegisteredCommands;

    bool m_initialShow;
};


template<>
int CMannequinDialog::defaultTabPosition<CFragmentEditorPage*>();
template<>
int CMannequinDialog::defaultTabPosition<CTransitionEditorPage*>();
template<>
int CMannequinDialog::defaultTabPosition<CPreviewerPage*>();
template<>
int CMannequinDialog::defaultTabPosition<CErrorReport*>();

const QString MANNEQUIN_FOLDER = "Animations/Mannequin/ADB/";
const QString TAGS_FILENAME_ENDING = "tags";
const QString TAGS_FILENAME_EXTENSION = ".xml";

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINDIALOG_H
