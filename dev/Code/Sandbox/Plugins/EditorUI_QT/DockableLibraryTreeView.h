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
#include <QDockWidget>
#include <QDrag>

class CLibraryTreeView;
class CLibraryTreeViewItem;
class RenameableTitleBar;
class DefaultViewWidget;
struct IDataBaseLibrary;
class CBaseLibraryItem;
class ContextMenu;
class QVBoxLayout;
class DockableLibraryPanel;

class EDITOR_QT_UI_API DockableLibraryTreeView
    : public QDockWidget
{
    Q_OBJECT
public:
    DockableLibraryTreeView(QWidget* parent);
    ~DockableLibraryTreeView();
    bool Init(IDataBaseLibrary* lib);
    void Collapse();
    void Expand();
    void ToggleCollapseState();
    // Return true if the library is collapsed
    bool GetCollapseState();

    void ResetSelection();
    void UpdateColors(const QColor& enabledColor, const QColor& disabledColor);
    void UpdateIconStyle(CLibraryTreeViewItem* item);

    //this will break any pointers present
    void SyncFromEngine();
    void RefreshView();

    CLibraryTreeViewItem* GetItemFromName(const QString& nameWithoutLibrary);
    void SelectItemFromName(const QString& nameWithoutLibrary, bool forceSelection = false);
    bool IsModified();
    bool Reload();
    bool CanItemHaveFolderAsChild(const QString& nameWithoutLibrary);
    //Rename item by open text editor
    void RenameItem(CLibraryTreeViewItem* item, const QString forceNameOverride = "", const bool overrideSafety = false);

    QString StoreExpansionData();
    void RestoreExpansionData(const QString& value);
    void StoreSelectionData();
    void RestoreSelectionData();
    void ExpandItem(CLibraryTreeViewItem* item);
    void CollapseItem(CLibraryTreeViewItem* item);
    void ExpandAll();
    void CollapseAll();

    const QString StoreExpandItems();
    void RestoreExpandItems(const QString);


    //exposed for search functionality
    void ShowOnlyStringsMatching(const QString& filter);
    void RemoveFilter();

    QVector<CLibraryTreeViewItem*> GetSelectedItems();
    QVector<CBaseLibraryItem*> GetSelectedItemsWithChildren();
    QVector<CLibraryTreeViewItem*> GetSelectedTreeItemsWithChildren();
    QVector<CBaseLibraryItem*> GetChildrenOfItem(const QString& itemPath);
    QVector<CLibraryTreeViewItem*> GetChildrenOfTreeItem(const QString& itemPath);

    void SetAllItemsEnabled(const bool& enabled = true);

    QString AddLibFolder(QString path, const bool overrideSafety = false);
    QString AddLibItem(QString path, const bool overrideSafety = false);
    CBaseLibraryItem* AddItem(const QString& nameWithoutLibrary);

    void RenameLibrary();
    IDataBaseLibrary* GetLibrary() const { return m_library; }
    DockableLibraryPanel* GetLibraryPanel() const { return m_libPanel; }
    virtual QString NameItemDialog(const QString& dialogTitle, const QString& prompt, const QString& preexistingPath = "");
signals:
    void SignalItemSelected(CBaseLibraryItem* item);
    void SignalItemAdded(CBaseLibraryItem* item, const QString& name);
    void SignalItemAboutToBeRenamed(CBaseLibraryItem* item, const QString& currentName, const QString& nextName, bool& proceed);
    void SignalItemRenamed(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newlib);
    void SignalItemCheckLod(CBaseLibraryItem* item, bool& hasLod);
    void SignalContentResized(const QSize& size);
    void SignalPopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo);
    void SignalTreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view);
    void SignalItemAboutToBeDragged(CLibraryTreeViewItem* item);
    void SignalDragOperationFinished();
    void SignalItemEnableStateChanged(CBaseLibraryItem* item, const bool& state);
    void SignalPopulateLibraryContextMenu(ContextMenu* toAddTo, const QString& libName);
    void SignalShowEvent();
    void SignalFocused(DockableLibraryTreeView* self);
    void SignalLibraryRenamed(const QString& lib, const QString& title);
    void SignalTitleValidationCheck(const QString& text, bool& validName);
    void SignalDecorateDefaultView(const QString& lib, DefaultViewWidget* view);
    void SignalPopulateSelectionList(QVector<CLibraryTreeViewItem*>& listOut);
    void SignalStartDrag(QDrag* drag, Qt::DropActions supportedActions);
    void SignalMoveItem(const QString& destPath, QString selections, CBaseLibraryItem*& newItem);
    void SignalCopyItem(IDataBaseItem*);
    void SignalPasteItem(IDataBaseItem* item, bool overrideSelection);
    void SignalUpdateTabName(const QString& fullOriginalName, const QString& fullNewName);

private slots:
    void PassThroughItemSelection(CBaseLibraryItem* item);
    void PassThroughItemAboutToBeRenamed(CBaseLibraryItem* item, const QString& currentName, const QString& nextName, bool& proceed);
    void PassThroughItemRenamed(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newlib);
    void PassThroughItemCheckLod(CBaseLibraryItem* item, bool& checkLod);
    void PassThroughPopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo);
    void PassThroughTreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view);
    void PassThroughItemAboutToBeDragged(CLibraryTreeViewItem* item);
    void PassThroughDragOperationFinished();
    void PassThroughItemEnableStateChanged(CBaseLibraryItem* item, const bool& state);
    void PassThroughPopulateSelectionList(QVector<CLibraryTreeViewItem*>& listOut);
    void PassThroughStartDrag(QDrag* drag, Qt::DropActions supportedActions);
    void ProcessDelayedWarning(QString warning);
protected:
    virtual void DecorateDefaultView();

private:
    void LogWarning(QString warning);
    void ShowDefaultView();
    void ShowTreeView();
    void OnContentSizeChanged();
    void OnFirstShowing();
    void OnMenuRequested(const QPoint& pos);
    QString MakeFullItemName();

    virtual void showEvent(QShowEvent* e) override;
    virtual void focusInEvent(QFocusEvent* e) override;
    void OnLibraryRename(const QString& str);
    void OnLibraryNameValidation(const QString& str, bool& isValid);

    virtual void dragEnterEvent(QDragEnterEvent*) override;

    virtual void dragMoveEvent(QDragMoveEvent*) override;

    virtual void dropEvent(QDropEvent*) override;

    CLibraryTreeView* m_treeView;
    RenameableTitleBar* m_titleBar;
    DefaultViewWidget* m_defaultView;
    IDataBaseLibrary* m_library;
    QVBoxLayout* m_layout;
    QWidget* m_centralWidget;
    QWidget* m_collapsedWidget;
    DockableLibraryPanel* m_libPanel;
};
