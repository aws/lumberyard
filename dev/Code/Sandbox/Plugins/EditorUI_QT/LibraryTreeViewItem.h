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
#include <QTreeWidgetItem>
#include <QMimeData>

class CBaseLibraryItem;
class CBaseLibraryManager;
class CLibraryTreeView;

class EDITOR_QT_UI_API CLibraryTreeViewItem
    : public QTreeWidgetItem
{
public:
    explicit CLibraryTreeViewItem(QTreeWidgetItem* parent, CBaseLibraryManager* mngr, const QString& lookup, int nameColumn, int checkBoxColumn);

    void SetItem(CBaseLibraryItem* item);
    CBaseLibraryItem* GetItem();

    /*The ability to create folders by themselves was an item that spawned from the demo day presentations.
    The ask was to be able to create folders for organization purposes.
    Previously the only way to create a folder was to create a item with a
    name path as demonstrated above then add or remove items from that folder
    (though if all items were removed from the folder it would be gone from the tree view).
    Changing the entire DB code structure and all editors for all DB managed types is something that was out of scope for that project.

    To get empty folder functionality working for the time being we added a value to CBaseLibraryItem
    that would be used to identify folder items in the tree view
    and is used to filter these items out from the save process.

    The IsVirtualItem function is used to make sure the items we're dealing with are real items or not,
    anything that returns true for IsVirtualItem will not be saved and should not have permanent changes made.

    an exception is renaming, when you rename an item in the tree the rename should be recursive, so renaming a
    folder holding items will cause the item paths to be permanently changed.*/
    bool IsVirtualItem();
    QString GetPreviousPath();
    QString GetPath();
    QString GetFullPath();
    QString GetLibraryName();
    void SetName(QString name, bool validPrevName);
    void SetLibraryName(QString name);

    void SetVirtual(bool val);

    bool IsEnabled() const;
    void SetEnabled(bool val);
    void ExpandToThis(bool unhide = false);
    void ExpandToParent(bool unhide = false);

    QString ToString();
    bool FromString(const QString& value);

    QVariant data(int column, int role) const override;

protected:
    QString BuildPath();

private:
    int m_nameColumn;
    bool m_isEnabled;
    QString m_previousPath;
    QString m_libraryName;
    QString m_lookup;
    CBaseLibraryManager* m_libManager;
    int m_checkBoxColumn;

    void SetLookup(QString libraryName, QString name);
};
