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
#ifndef CRYINCLUDE_EDITORUI_QT_LIBRARY_TREEVIEW_H
#define CRYINCLUDE_EDITORUI_QT_LIBRARY_TREEVIEW_H

#include "api.h"
#include "Utils.h"
#include <QTreeWidget>
#include <functional>
#include <QPen>
#include <QPainter>
#include <QMap>
#include <QProxyStyle>

//TreeView Drop Indicator Color
#define CTreeViewDropIndicatorColor  0x1b75cf

#ifndef VALID_LIBRARY_ITEM_CHARACTERS
#define VALID_LIBRARY_ITEM_CHARACTERS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_."
#endif

#define VALID_LIBRARY_CHARACTERS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_"

#define LIBRARY_TREEVIEW_NAME_COLUMN 0
#define LIBRARY_TREEVIEW_INDICATOR_COLUMN 1

struct IDataBaseLibrary;
class CBaseLibraryItem;
class CLibraryTreeViewItem;
class ContextMenu;

class CLibraryTreeViewExpandState;
class EDITOR_QT_UI_API CLibraryTreeView
    : public QTreeWidget
{
    Q_OBJECT
public:
    CLibraryTreeView(QWidget* parent, IDataBaseLibrary* library);
    virtual ~CLibraryTreeView();

    QString GetLibName();

    void enableReordering();
    void fillFromLibrary(bool alphaSort = false);
    void refreshActiveState();
    QString storeExpandState();
    void restoreExpandState(const QString& data);
    void StartRename(CLibraryTreeViewItem* item, const QString& forceNameSelected = "", const bool overrideSafety = false);
    void EndRename();
    QString getItemPath(QTreeWidgetItem* item, bool includeLibrary = false);

    void ExpandItem(CLibraryTreeViewItem* item);
    void CollapseItem(CLibraryTreeViewItem* item);
    QModelIndex getIndexFromItem(QTreeWidgetItem* item);
    CLibraryTreeViewItem* getItemFromIndex(const QModelIndex& item);
    CLibraryTreeViewItem* getItemFromName(const QString& name);
    QVector<CLibraryTreeViewItem*> GetSelectedItems();
    QVector<CBaseLibraryItem*> GetChildrenOfItem(const QString& path);
    QVector<CBaseLibraryItem*> GetSelectedItemsWithChildren();
    QVector<CLibraryTreeViewItem*> GetChildrenOfTreeItem(const QString& path);
    QVector<CLibraryTreeViewItem*> GetSelectedTreeItemsWithChildren();
    CLibraryTreeViewItem* GetFirstSelectedItem();
    //pass "" to clear selection
    void SelectItem(QString pathWithoutLibrary, bool forceSelection = false);
    void UpdateColors(const QColor& enabledColor, const QColor& disabledColor);
    void setReloading(bool val);
    void SetLibrary(IDataBaseLibrary* lib, bool alphaSort = false);

    /// Returns true if the given library item name and item full-name are valid.
    /// \param itemName The name of the single item. Corresponds to a single folder's name or single leaf item's name. Ex: "myItem".
    /// \param itemPath The path of the item, including any prefixes for parent folders and the item name itself (but not including the library name). Ex: "folderA.folderB.myItem".
    /// \param item     The item being renamed.
    bool ValidateItemName(const QString& itemName, const QString& itemPath, const CBaseLibraryItem* item);

    //tree view inherits from scroll area so it's size calculations are hard coded and ignores content size
    //this function will calculate the size of the contents
    QSize GetSizeOfContents();
    bool IsLibraryModified();

    bool ApplyFilter(const QString& filter);
    void SetAllItemsEnabled(const bool& enabled = true);

    void StoreSelectionData();
    void RestoreSelectionData();
    //made public to allow dockablelibrarytreeview to request the context menu
    void OnMenuRequested(const QPoint& pos);
    void DropMetaData(QDropEvent* event, bool dropToLocation = true);
    virtual void dropEvent(QDropEvent* event);
    void UpdateItemStyle(CLibraryTreeViewItem* item);
    // This function could only used to reorder item with the same parent. Hierarchy issue must
    // be taken care before this function get called.
    void ReorderItem(CBaseLibraryItem* neighbor, CBaseLibraryItem* item, DropIndicatorPosition position);

signals:
    void SignalTreeViewEmpty();
    void SignalItemSelected(CBaseLibraryItem* selectedItem);
    void SignalItemReselected(CBaseLibraryItem* selectedItem);
    void SignalItemAboutToBeRenamed(CBaseLibraryItem* item, const QString& currentName, const QString& nextName, bool& proceed);
    void SignalItemRenamed(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newLib = "");
    void SignalItemAboutToBeDragged(CLibraryTreeViewItem* item);
    void SignalDragOperationFinished();
    void SignalTreeFilledFromLibrary(IDataBaseLibrary* lib, CLibraryTreeView* view);
    void SignalPopulateItemContextMenu(CLibraryTreeViewItem* focusedItem, ContextMenu* toAddTo);
    void SignalItemEnableStateChanged(CBaseLibraryItem* item, const bool& state);
    void SignalPopulateSelectionList(QVector<CLibraryTreeViewItem*>& listOut);
    void SignalStartDrag(QDrag* drag, Qt::DropActions supportedActions);
    void SignalCheckLod(CBaseLibraryItem* item, bool& hasLod);
    void SignalMoveItem(const QString& destPath, QString selections, CBaseLibraryItem*& newItem);
    void SignalCopyItem(IDataBaseItem*);
    void SignalPasteSelected(IDataBaseItem* item, bool overrideSelection);
    void SignalUpdateTabName(const QString& fullOriginalName, const QString& fullNewName);

protected:
    virtual bool event(QEvent* e);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual bool RenameItem(const QString& oldName, const QString& nextName);

private:
    void OnCurrentItemChanged(QTreeWidgetItem* item, QTreeWidgetItem* previous);
    void OnSelectionChanged(); //keeps the active item cache up to date
    void OnRowInserted(const QModelIndex& parentIndex, int first, int last);
    void OnItemChanged(QTreeWidgetItem* item, int column);

    QString GetPathFromItemFullName(const QString& name);

    //Help function to grab final item name for drag/drop
    void GetFinalItemName(QString& finalItemName, CBaseLibraryItem* libItem, CBaseLibraryItem* dropLibItem, const QVector<QString>& parents);

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;

    virtual void leaveEvent(QEvent* event) override;
    QString GetReverseMatchedStrings(const QString& a, const QString& b);

    virtual QStringList mimeTypes() const override;

    virtual void dragEnterEvent(QDragEnterEvent* e) override;
    
    //After drag/drop operation. The order of particle item in this view may change and same as the items in particle's baseDataLibrary.
    //But the order of children in IParticleEffect won't match the new order. We are using this patch function to sync the order in IParticleEffect with particle library.
    void FixParticleEffectOrder(IDataBaseLibrary* lib);

private:
    QMap<QString, QTreeWidgetItem*> m_nameToNode;
    IDataBaseLibrary* m_baseLibrary;
    QIcon* m_iconFolderClosed;
    QIcon* m_iconFolderOpen;
    QIcon* m_iconShowLOD;
    QIcon* m_iconIsGroup;
    QIcon* m_iconGroupWithLOD;
    QIcon* m_iconEmpty;
    //item selection doesn't always survive changes
    //added this list to track what is selected when a change happens
    QVector<CLibraryTreeViewItem*> m_activeItems;
    //used during rename and reloading to prevent problematic code
    bool m_allowSelection;
    bool m_isInDrop;
    //used for the "draggable" checkbox
    Qt::CheckState m_lastCheckboxState;
    bool m_amSettingCheckboxStates;
    QColor m_enabledItemTextColor;
    QColor m_disabledItemTextColor;
    QString m_filter;
    QString m_cachedSelectonData; //used for storeselectiondata/restoreselectiondata
};

class dropIndicatorStyle
    : public QProxyStyle
{
public:
    dropIndicatorStyle(QStyle* baseStyle = 0)
        : QProxyStyle(baseStyle) {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        if (element == QStyle::PE_IndicatorItemViewItemDrop)
        {
            painter->setPen(QPen(QColor(CTreeViewDropIndicatorColor), 2));
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};
#endif // CRYINCLUDE_EDITORUI_QT_LIBRARY_TREEVIEW_H
