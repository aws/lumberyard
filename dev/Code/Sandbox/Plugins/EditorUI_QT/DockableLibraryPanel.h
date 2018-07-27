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
#include "api.h"
#include "Utils.h"
#include "FloatableDockPanel.h"
#include <EditorUI_QTDLLBus.h>

struct IDataBaseLibrary;
class CBaseLibraryManager;
class CBaseLibraryItem;
class DockWidgetTitleBar;
class DockableLibraryTreeView;
class CLibraryTreeView;
class PanelWidget;
class CLibraryTreeViewItem;
class QMenu;
class ContextMenu;
class QScrollArea;
class QVBoxLayout;
class QComboBox;
class QPushButton;
class QHBoxLayout;
class DefaultViewWidget;
class QDrag;
struct QTUIEditorSettings;


#define QTUI_EDITOR_ENDRENAME_FUNCTIONNAME "EndRename"


class EDITOR_QT_UI_API DockableLibraryPanel
    : public FloatableDockPanel
    , public EditorUIPlugin::LibraryPanelRequests::Bus::Handler
{
    Q_OBJECT
public:
    DockableLibraryPanel(QWidget* parent);
    ~DockableLibraryPanel();

    void Init(const QString& panelName, CBaseLibraryManager* libraryManager);
    void ResetSelection();
    void RebuildFromEngineData(bool ignoreExpansionStates = false);//this will destroy all pointers
    void ForceLibrarySync(const QString& name);//this will destroy libraryItem pointers for the specified library

    enum class LibraryActions
    {
        ADD = 0,
        IMPORT,
        IMPORT_LEVEL_LIBRARY,
        SAVE_AS,
        SAVE,
        SAVE_ALL,
        ENABLE_ALL,
        DISABLE_ALL,
        REMOVE,
        RELOAD,
        DUPLICATE_LIB
    };

    enum class ItemActions
    {
        ADD_ITEM = 0,
        ADD_FOLDER,
        SHOW_PATH,
        RENAME,
        DESTROY,
        COPY_PATH,
        OPEN_IN_NEW_TAB,
        EXPAND_ALL,
        COLLAPSE_ALL
    };

    enum class TreeActions
    {
        EXPAND_ALL = 0,
        COLLAPSE_ALL,
    };

    // The ownership of the action affect on the scope of hotkeys.
    virtual QAction* GetMenuAction(LibraryActions action, QString displayAlias, bool overrideSafety = false, QWidget* owner = nullptr, Qt::ConnectionType connection = Qt::AutoConnection);
    virtual QAction* GetMenuAction(ItemActions action, QString item, QString displayAlias, bool overrideSafety = false, QWidget* owner = nullptr, Qt::ConnectionType connection = Qt::AutoConnection);
    virtual QAction* GetMenuAction(TreeActions action, QString displayAlias, bool overrideSafety = false, QWidget* owner = nullptr, Qt::ConnectionType connection = Qt::AutoConnection);

    void UpdateColors(const QMap<QString, QColor>& colorMap);

    void AddLibraryListToMenu(QMenu* subMenu);
    //duplicates item location and provides a unique name for the new item
    //i.e. duplicate environment.test results in environment.test1
    CLibraryTreeViewItem* AddDuplicateTreeItem(const QString& itemPath, const bool isVirtual = false, QString pasteTo = "");

    void StoreAllSelectionData();
    void RestoreAllSelectionData();
    void StoreSelectionData(const QString& libName);
    void RestoreSelectionData(const QString& libName);

    void StoreAllExpandItemInfo();
    void RestoreAllExpandItemInfo();
    void RestoreExpandItemInfoForLibrary(const QString m_libName, const QString data);
    void RestoreExpandLibInfoForLibrary(const QString m_libName, const QString data);
    
    QString GetLibExpandInfo(const QString libName);
    QString GetLibItemExpandInfo(const QString libName);
    
    QVector<CBaseLibraryItem*> GetSelectedItems();
    QString GetSelectedLibraryName(); // Access the library for Manager Undo

    void SetItemEnabled(const QString& libName, const QString& itemName, bool val);
    void Validate();

    QVector<CLibraryTreeViewItem*> GetSelectedTreeItems();
    QString GetLastAddedItemName();

    QVector<CBaseLibraryItem*> GetChildrenOfItem(CBaseLibraryItem* item);
    /*
    Actions assume they apply to all selected items
    If this is not the case, pass in the libitems they apply to with a string i.e. "Level.fu&&Level.bar"
    */
    QVector<CLibraryTreeViewItem*> GetActionItems(const QString& overrideSelection);
    
    void RenameLibraryItem(const QString& fullNameOfItem, const QString& newPath, const bool overrideSafety = false);
    void RemapHotKeys();

    //LibraryPanelRequests::Bus
    EditorUIPlugin::LibTreeExpandInfo GetLibTreeExpandInfo(const AZStd::string& libId);
    void LoadLibTreeExpandInfo(const AZStd::string& libId, const EditorUIPlugin::LibTreeExpandInfo& expandInfo);
    //end LibraryPanelRequests::Bus

signals:
    void SignalItemSelected(CBaseLibraryItem* item);
    void SignalItemAboutToBeRenamed(CBaseLibraryItem* item, const QString& currentName, const QString& nextName, bool& proceed);
    void SignalItemRenamed(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newlib);
    void SignalItemCheckLod(CBaseLibraryItem* item, bool& hasLod);
    void SignalPopulateTitleBarMenu(QMenu* toAddTo);
    void SignalPopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo);
    void SignalItemCopied(CBaseLibraryItem* item);
    void SignalItemAdded(CBaseLibraryItem* item, const QString& name);
    void SignalTreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view);
    void SignalItemAboutToBeDragged(CLibraryTreeViewItem* item);
    void SignalItemDeleted(const QString& name);
    void SignalDragOperationFinished();
    void SignalOpenInNewTab(CBaseLibraryItem*);
    void SignalItemEnableStateChanged(CBaseLibraryItem* item, const bool& state);
    void SignalPopulateLibraryContextMenu(ContextMenu* toAddTo, const QString& libName);
    void SignalDecorateDefaultView(const QString& lib, DefaultViewWidget* view);
    void SignalDuplicateItem(const QString& itemPath, QString pasteTo);
    void SignalItemPasted(IDataBaseItem* target, bool overrideSafety = false);
    void SignalPasteItemsToFolder(IDataBaseLibrary* lib, const QStringList& pasteList);
    void SignalCopyItems(QVector<CLibraryTreeViewItem*> items, bool copyAsChild = false);
    void SignalCopyItem(IDataBaseItem* item);
    void SignalSaveAllLibs();
    void SignalUpdateTabName(const QString& fullOriginalName, const QString& fullNewName);

public slots:
    void SelectLibraryAndItemByName(const QString& libraryName, const QString& itemName, bool forceSelection = false);
    CLibraryTreeViewItem* GetTreeItemFromPath(const QString& fullName);
    void UpdateIconStyle(QString libName, QString  itemName);

private slots:
    void PassThroughItemSelection(CBaseLibraryItem* item);
    void PassThroughItemAboutToBeRenamed(CBaseLibraryItem* item, const QString& currentName, const QString& nextName, bool& proceed);
    void PassThroughItemCheckLod(CBaseLibraryItem* item, bool& hasLod);
    void PassThroughItemRenamed(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newlib);
    void PassThroughPopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo);
    void PassThroughTreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view);
    void PassThroughItemAboutToBeDragged(CLibraryTreeViewItem* item);
    void PassThroughDragOperationFinished();
    void PassThroughItemEnableStateChanged(CBaseLibraryItem* item, const bool& state);
    void PassThroughItemAdded(CBaseLibraryItem* item, const QString& name);
    void PassThroughDecorateDefaultView(const QString& lib, DefaultViewWidget* view);

protected:
    virtual void RegisterActions();
    virtual void UnregisterActions();

    virtual QString PromptImportLibrary() = 0;

private: //Functions
    void ImportLibrary(const QString& file);
    void AddLibrary();
    void AddLevelCopyLibrary();
    void UpdateLibSelectionStyle();
    void DecorateDefaultLibraryView();
    void TriggerDefaultView();
    void ShowDefaultView();
    void HideDefaultView();

#ifdef LoadLibrary
#undef LoadLibrary
#endif
    void LoadLibrary(const QString& libName);
    void ReloadLibrary(const QString& libName);
    void SelectSingleLibrary(const QString& libName);
    void OnSearchFilterChanged(const QString& searchFilter);
    void OnWindowTitleUpdate();
    void OnLibraryAdded(IDataBaseLibrary* addLib);
    void OnWindowTitleChanged(const QString& title);
    void OnLibraryToggleButtonPushed();
    void OnDockLocationChanged(DockableLibraryTreeView* dock);
    QString GetTitleNameWithCount();
    QMenu* GetTitleBarMenu();
    void ExportLibrary(const QString& currentLib);
    void RemoveLibrary(const QString& currentLib);
    void DuplicateLibrary(const QString& currentLib);
    bool RemoveItemFromLibrary(QString overrideSelection = "", bool overrideSafety = false);
    void LibraryRenamed(const QString& lib, const QString& title);
    void OnLibraryTitleValidationCheck(const QString& str, bool& isValid);
    void OnStartDragRequested(QDrag* drag, Qt::DropActions supportedActions);

    QVector<DockableLibraryTreeView*> GetVisibleLibraries();

    virtual void resizeEvent(QResizeEvent* e) override;
    virtual void mousePressEvent(QMouseEvent* event);
    QVector<CLibraryTreeViewItem*> GetSelectedTreeItemsWithChildren();
    QVector<CBaseLibraryItem*> GetSelectedItemsWithChildren();
protected:
    QMap<QString, DockableLibraryTreeView*> m_libraryTreeViews;
    QString m_lastAddedItem;
    CBaseLibraryManager* m_libraryManager;

    QVector<CLibraryTreeViewItem*> GetTreeItemsAndChildrenOf(const QVector<CLibraryTreeViewItem*>& selection);

    //////////////////////////////////////////////////////////////////////////
    //Move private to protected so that DockableParticelLibraryPanel could access the functions
    //ACTIONS - void ActionFunctionName(const QString& overrideSelection = "", const bool overrideSafety = false)
    void ActionDelete(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionRename(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionExportLib(const QString& librarySelection = "");
    void ActionOpenInNewTab(const QString& librarySelection = "");
    void ActionDuplicateItem(const QString& librarySelection = "");
    void ActionAddItem(const QString& librarySelection = "");
    void ActionAddFolder(const QString& librarySelection = "");
    //This action only move folder item (not including the children)
    void ActionMoveOneFolder(const QString& destPath, const QString& Selection);
    //END ACTIONS
    //////////////////////////////////////////////////////////////////////////

private: //Variables
    DockWidgetTitleBar* m_libraryTitleBar;
    QMenu* m_titleBarMenu;

    QWidget* m_searchArea;
    QComboBox* m_searchField;
    QPushButton* m_AddNewLibraryButton;
    QHBoxLayout* m_searchLayout;
    DefaultViewWidget* m_defaultView;

    PanelWidget* m_dockableArea;
    QScrollArea* m_scrollArea;

    QVBoxLayout* m_layout;
    QWidget* m_layoutWidget;

    QString m_panelName;
    QMap<QString, bool> m_libExpandData;
    QMap<QString, QString> m_ItemExpandState;

    QColor m_enabledItemTextColor;
    QColor m_disabledItemTextColor;
    QString m_selectedLibraryKey;
    QMap<QString, QAction*> m_actions;
    
    //used for disable library order change when load a library
    bool m_loadingLibrary;
   
};
