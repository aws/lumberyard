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

//Editor
#include <IEditor.h>
#include <Undo/IUndoManagerListener.h>

//Editor UI
#include <EditorUI_QTDLLBus.h>

//QT
#include <QMainWindow>
#include <QMap>
#include <QShortcut>
#include <QShortcutEvent>

//Local
#include "ParticleLibraryAutoRecovery.h"

class QCloseEvent;
class QActionGroup;
class DockableAttributePanel;
class DockableParticleLibraryPanel;
class DockablePreviewPanel;
class DockableLODPanel;
class ContextMenu;
class CLibraryTreeViewItem;
class CLibraryTreeView;
struct IDataBaseLibrary;
class DefaultViewWidget;
struct IParticleEffect;
struct SLodInfo;
class QAttributePresetWidget;
class CAttributeViewConfig;
class CUndoStep;

namespace EditorUIPlugin
{
    class EditorLibraryUndoManager;
};

namespace AzQtComponents
{
    class StylesheetPreprocessor;
};

class CMainWindow
    : public QMainWindow
    , public IEditorNotifyListener
    , public ISystemEventListener
    , public EditorUIPlugin::LibraryItemUIRequests::Bus::Handler
    , public EditorUIPlugin::LibraryChangeEvents::Bus::Handler
{
    Q_OBJECT
public:
    explicit CMainWindow(QWidget* parent = nullptr);
    virtual ~CMainWindow(void);

    static const GUID& GetClassID();

    ////////////////////////////////////////////////////////
    //IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent e) override;
    ////////////////////////////////////////////////////////

private:
    ////////////////////////////////////////////////////////
    //QMainWindow
    void closeEvent(QCloseEvent* event) override; 
    void showEvent(QShowEvent * event) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool event(QEvent* e) override;
    bool ShortcutEvent(QShortcutEvent* e);
    ////////////////////////////////////////////////////////

    void UpdateEditActions(bool menuShown);
    bool OnCloseWindowCheck();
    void CleanupOnClose();
    void OnAddNewLayout(QString path, bool loading);
    void OnRefreshViewMenu();
    void OnActionViewImportLayout();
    void OnActionViewExportLayout();
    void OnActionViewHideAttributes();
    void OnActionViewHideLibrary();
    void OnActionViewHidePreview();
    void OnActionViewHideLOD();
    void OnActionClose();
    void OnActionStandardUndo();
    void OnActionStandardRedo();
    void OnActionViewDocumentation();
    void UpdatePalette();
    void CreateMenuBar();
    void CreateShortcuts();
    void CreateDockWindows();
    void ResetToDefaultEditorLayout(); //Resets the layout as if the used never touched it (doesn't change the properties/attribute layout settings)
    void RefreshLibraries();
    void StateSave();
    void StateSave_layoutMenu();
    void StateLoad();
    void StateLoad_Properties();
    void StateLoad_Preview();
    void StateLoad_layoutMenu();
    void RestoreAllLayout(QString path);
    void CorrectLayout(QByteArray& data);
    void NotifyExceptSelf(EEditorNotifyEvent notification);

    // substitute named defenitions in style files
    QString ProcessStyleSheet(QString const& fileName);
    QString GetColorStringByName(QString const& name);

    // overrides from ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    void PrepareForParticleSystemUnload();

    //LibraryItemUIRequests::Bus
    void UpdateItemUI(const AZStd::string& itemId, bool selected, int lodIdx) override;
    void RefreshItemUI() override;
 
    //end LibraryItemUIRequests::Bus

    //LibraryChangeEvents::Bus
    void LibraryChangedInManager(const char* libraryName) override;
    //end LibraryChangeEvents::Bus

private slots:
    void Preview_PopulateTitleBarMenu(QMenu* toAddTo);
    void Library_PopulateTitleBarMenu(QMenu* toAddTo);
    void Attribute_PopulateTitleBarMenu(QMenu* toAddTo);
    void LOD_PopulateTitleBarMenu(QMenu* toAddTo);

    void Library_PopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo);
    void Library_PopulateLibraryContextMenu(ContextMenu* toAddTo, const QString& libName);
    void Attribute_PopulateTabBarContextMenu(const QString& libraryName, const QString& itemName, ContextMenu* toAddTo);

    void Library_ItemNameValidationRequired(IDataBaseItem* item, const QString& currentName, const QString& nextName, bool& proceed);
    void Library_ItemRenamed(IDataBaseItem* item, const QString& oldName, const QString& currentName, const QString newlib = "");

    void Library_UpdateTreeItemStyle(IDataBaseItem* item, int coloum);

    void Library_ItemCopied(IDataBaseItem* item);
    void Library_CopyTreeItems(QVector < CLibraryTreeViewItem* > items, bool copyAsChild = false);
    void Library_ItemPasted(IDataBaseItem* target, bool overrideSafety = false);
    void Library_ItemsPastedToFolder(IDataBaseLibrary* lib, const QStringList& PasteList);
    void Library_ItemDuplicated(const QString& itemPath, QString pasteTo);

    void Library_TreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view);
    void Library_ItemDragged(CLibraryTreeViewItem* item);
    void Library_ItemAdded(CBaseLibraryItem* item, const QString& name);
    void Library_DragOperationFinished();
    void Library_ItemEnabledStateChanged(CBaseLibraryItem* item, const bool& state);
    void Library_DecorateTreesDefaultView(const QString& lib, DefaultViewWidget* view);
    void Library_SelecteItem(const QString& fullname);

    //POSSIBLE LEGACY ONLY SYNC USAGE HERE
    void Attribute_ParameterChanged(const QString& libraryName, const QString& itemName);
    //POSSIBLE LEGACY ONLY SYNC USAGE HERE

    void OnItemUndoPoint(const QString& itemFullName, bool selected, SLodInfo* lod);

    void File_PopulateMenu();
    void Edit_PopulateMenu();
    void View_PopulateMenu();
    void Library_ItemReset(IDataBaseItem* target, SLodInfo *curLod = nullptr);

    void RegisterActions();
    void UnregisterActions();
private: // Internal particle editor members
    ParticleLibraryAutoRecovery m_AutoRecovery;

    QVector < QShortcut* > m_shortcuts;
    QByteArray m_defaultEditorLayout;
    QMap < QString, QColor > m_StyleColors;

    DockableParticleLibraryPanel* m_libraryTreeViewDock;
    DockableAttributePanel* m_attributeViewDock;
    DockablePreviewPanel* m_previewDock;
    DockableLODPanel* m_LoDDock;
    QAttributePresetWidget* m_preset;

    QMenuBar* m_menuBar;
    QMenu* m_libraryMenu;
    QMenu* m_layoutMenu;
    QMenu* m_viewMenu;
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_showLayoutMenu;
    QActionGroup* m_libraryMenuActionGroup;

    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    QAction* m_copyAction = nullptr;
    QAction* m_duplicateAction = nullptr;
    QAction* m_addLODAction = nullptr;
    QAction* m_resetSelectedItem = nullptr;
    QAction* m_renameSelectedItem = nullptr;
    QAction* m_deleteSelectedItem = nullptr;

    QAction* m_lodAction;
    QList < QAction* > m_actionsRequiringASelection;
    AzQtComponents::StylesheetPreprocessor* m_stylesheetPreprocessor;

    bool m_bIgnoreSelectionChange;
    bool m_bLibsLoaded;
    bool m_bAutoAssignToSelection;

    int m_iRefreshDelay;

    bool m_bIsRefreshing;
    bool m_RequestedClose;
    bool m_isFirstSceneSinceLaunch;
    bool m_needLibraryRefresh;
    bool m_requireLayoutReload;

    EditorUIPlugin::EditorLibraryUndoManager *m_undoManager;

};
