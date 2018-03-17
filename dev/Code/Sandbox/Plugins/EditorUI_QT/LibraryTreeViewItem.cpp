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

#include "stdafx.h"

#include "LibraryTreeViewItem.h"
//Editor
#include <EditorDefs.h>
#include <BaseLibraryItem.h>
#include <BaseLibraryManager.h>
#include <Particles/ParticleItem.h>

CLibraryTreeViewItem::CLibraryTreeViewItem(QTreeWidgetItem* parent, CBaseLibraryManager* mngr, const QString& lookup, int nameColumn, int checkBoxColumn)
    : QTreeWidgetItem(parent)
    , m_libManager(mngr)
    , m_previousPath("")
    , m_isEnabled(true)
    , m_nameColumn(nameColumn)
    , m_checkBoxColumn(checkBoxColumn)
    , m_lookup(lookup)
{
}

void CLibraryTreeViewItem::SetItem(CBaseLibraryItem* item)
{
    if (item)
    {
        m_lookup = item->GetFullName();
    }
    else
    {
        m_lookup = "";
    }
    setData(m_nameColumn, Qt::UserRole, m_lookup);
}

bool CLibraryTreeViewItem::IsVirtualItem()
{
    CBaseLibraryItem* item = GetItem();
    if (!item)
    {
        return true;
    }
    else if (item->IsParticleItem)
    {
        return false;
    }
    return true;
}

QString CLibraryTreeViewItem::GetPreviousPath()
{
    if (m_previousPath.isEmpty())
    {
        m_previousPath = BuildPath();
    }
    return m_previousPath;
}

QString CLibraryTreeViewItem::GetPath()
{
    return BuildPath();
}

void CLibraryTreeViewItem::SetName(QString name, bool validPrevName)
{
    if (text(m_nameColumn).isEmpty() || !validPrevName)
    {
        m_previousPath = name;
    }
    else
    {
        m_previousPath = BuildPath();
    }
    QStringList nameList = name.split('.');
    setText(m_nameColumn, nameList.last());
    CBaseLibraryItem* item = GetItem();
    if (item != nullptr)
    {
        item->SetName(name);
        SetLookup(item->GetLibrary()->GetName(), name);
    }
}

void CLibraryTreeViewItem::SetLookup(QString libraryName, QString name)
{
    if (libraryName.isEmpty())
    {
        m_lookup = name;
    }
    else
    {
        m_lookup = libraryName + '.' + name;
    }
}

bool CLibraryTreeViewItem::IsEnabled() const
{
    return m_isEnabled;
}

void CLibraryTreeViewItem::SetEnabled(bool val)
{
    m_isEnabled = val;
}

CBaseLibraryItem* CLibraryTreeViewItem::GetItem()
{
    if (m_lookup.isEmpty())
    {
        return nullptr;
    }
    if (m_libManager)
    {
        CBaseLibraryItem* item = static_cast<CBaseLibraryItem*>(m_libManager->FindItemByName(m_lookup));
        if (item)
        {
            return item;
        }
    }
    return nullptr;
}

QString CLibraryTreeViewItem::BuildPath()
{
    //walks up the tree items to build out the current path
    QString ret;
    if (parent() != nullptr)
    {
        ret += static_cast<CLibraryTreeViewItem*>(parent())->BuildPath();
        ret += ".";
    }
    ret += text(m_nameColumn);
    return ret;
}

QString CLibraryTreeViewItem::GetFullPath()
{
    return m_libraryName + "." + BuildPath();
}

void CLibraryTreeViewItem::ExpandToThis(bool unhide)
{
    if (parent() && !parent()->text(m_nameColumn).isEmpty())
    {
        static_cast<CLibraryTreeViewItem*>(parent())->ExpandToThis(unhide);
    }
    setExpanded(true);
    if (unhide)
    {
        setHidden(false);
    }
}

void CLibraryTreeViewItem::SetVirtual(bool val)
{
    CBaseLibraryItem* item = GetItem();
    if (item != nullptr)
    {
        item->IsParticleItem = !val;
    }
}

void CLibraryTreeViewItem::SetLibraryName(QString name)
{
    m_libraryName = name;
}

QString CLibraryTreeViewItem::GetLibraryName()
{
    return m_libraryName;
}

void CLibraryTreeViewItem::ExpandToParent(bool unhide)
{
    bool expanded = isExpanded();
    ExpandToThis(unhide);
    setExpanded(expanded);
}

QString CLibraryTreeViewItem::ToString()
{
    return m_lookup;
}

bool CLibraryTreeViewItem::FromString(const QString& value)
{
    m_lookup = value;
    if (GetItem())
    {
        return true;
    }
    return false;
}