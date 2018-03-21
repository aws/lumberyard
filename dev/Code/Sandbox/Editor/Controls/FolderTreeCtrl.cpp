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

#include <StdAfx.h>
#include "FolderTreeCtrl.h"
#include <QtUtil.h>
#include <QMenu>
#include <QContextMenuEvent>
#if defined (Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif
#include <QProcess>

#include <QRegularExpression>

enum ETreeImage
{
    eTreeImage_Folder = 0,
    eTreeImage_File = 2
};

enum CustomRoles
{
    IsFolderRole = Qt::UserRole
};

//////////////////////////////////////////////////////////////////////////
// CFolderTreeCtrl
//////////////////////////////////////////////////////////////////////////
CFolderTreeCtrl::CFolderTreeCtrl(const QStringList& folders, const QString& fileNameSpec,
    const QString& rootName, bool bDisableMonitor, bool bFlatTree, QWidget* parent)
    : QTreeWidget(parent)
    , m_rootTreeItem(nullptr)
    , m_folders(folders)
    , m_fileNameSpec(fileNameSpec)
    , m_rootName(rootName)
    , m_bDisableMonitor(bDisableMonitor)
    , m_bFlatStyle(bFlatTree)
{
    init(folders, fileNameSpec, rootName, bDisableMonitor, bFlatTree);
}

CFolderTreeCtrl::CFolderTreeCtrl(QWidget* parent)
    : QTreeWidget(parent)
    , m_rootTreeItem(nullptr)
    , m_bDisableMonitor(false)
    , m_bFlatStyle(true)

{
}


void CFolderTreeCtrl::init(const QStringList& folders, const QString& fileNameSpec, const QString& rootName, bool bDisableMonitor /*= false*/, bool bFlatTree /*= true*/)
{
    m_folders = folders;
    m_fileNameSpec = fileNameSpec;
    m_rootName = rootName;
    m_bDisableMonitor = bDisableMonitor;
    m_bFlatStyle = bFlatTree;

    for (auto item = m_folders.begin(), end = m_folders.end(); item != end; ++item)
    {
        (*item) = Path::RemoveBackslash(Path::ToUnixPath((*item)));

        if (CFileUtil::PathExists(*item))
        {
            m_foldersSegments.insert(std::make_pair((*item), Path::SplitIntoSegments((*item)).size()));
        }
        else
        {
            (*item).clear();
        }
    }

    headerItem()->setHidden(true);
}

QString CFolderTreeCtrl::GetPath(QTreeWidgetItem* item) const
{
    CTreeItem* treeItem = static_cast<CTreeItem*>(item);

    if (treeItem)
    {
        return treeItem->GetPath();
    }

    return "";
}

bool CFolderTreeCtrl::IsFolder(QTreeWidgetItem* item) const
{
    return item->data(0, IsFolderRole).toBool();
}

bool CFolderTreeCtrl::IsFile(QTreeWidgetItem* item) const
{
    return !IsFolder(item);
}

void CFolderTreeCtrl::showEvent(QShowEvent* event)
{
    m_filePixmap = QPixmap(QStringLiteral("arhitype_tree_%1.png").arg(static_cast<int>(eTreeImage_File), 2, 10, QLatin1Char('0')));
    m_folderPixmap = QPixmap(QStringLiteral("arhitype_tree_%1.png").arg(static_cast<int>(eTreeImage_Folder), 2, 10, QLatin1Char('0')));

    InitTree();

    QTreeWidget::showEvent(event);
}


CFolderTreeCtrl::~CFolderTreeCtrl()
{
    // Obliterate tree items before destroying the controls
    m_rootTreeItem.reset(nullptr);

    // Unsubscribe from file change notifications
    if (!m_bDisableMonitor)
    {
        CFileChangeMonitor::Instance()->Unsubscribe(this);
    }
}

void CFolderTreeCtrl::OnFileMonitorChange(const SFileChangeInfo& rChange)
{
    const QString filePath = Path::ToUnixPath(Path::GetRelativePath(rChange.filename));
    for (auto item = m_folders.begin(), end = m_folders.end(); item != end; ++item)
    {
        // Only look for changes in folder
        if (filePath.indexOf((*item)) != 0)
        {
            return;
        }

        if (rChange.changeType == rChange.eChangeType_Created || rChange.changeType == rChange.eChangeType_RenamedNewName)
        {
            if (CFileUtil::PathExists(filePath))
            {
                LoadTreeRec(filePath);
            }
            else
            {
                AddItem(filePath);
            }
        }
        else if (rChange.changeType == rChange.eChangeType_Deleted || rChange.changeType == rChange.eChangeType_RenamedOldName)
        {
            RemoveItem(filePath);
        }
    }
}

void CFolderTreeCtrl::contextMenuEvent(QContextMenuEvent* e)
{
    QTreeWidgetItem* item = itemAt(e->pos());

    if (item == NULL)
    {
        return;
    }

    const QString path = GetPath(item);

    QMenu menu;
    QAction* editAction = menu.addAction(tr("Edit"));
    connect(editAction, &QAction::triggered, [=]()
        {
            this->Edit(path);
        });
    QAction* showInExplorerAction = menu.addAction(tr("Show In Explorer"));
    connect(showInExplorerAction, &QAction::triggered, [=]()
        {
            this->ShowInExplorer(path);
        });
    menu.exec(QCursor::pos());
}

void CFolderTreeCtrl::InitTree()
{
    m_rootTreeItem.reset(new CTreeItem(*this, m_rootName));

    for (auto item = m_folders.begin(), end = m_folders.end(); item != end; ++item)
    {
        if (!(*item).isEmpty())
        {
            LoadTreeRec((*item));
        }
    }

    if (!m_bDisableMonitor)
    {
        CFileChangeMonitor::Instance()->Subscribe(this);
    }


    m_rootTreeItem->setExpanded(true);
}

void CFolderTreeCtrl::LoadTreeRec(const QString& currentFolder)
{
    CFileEnum fileEnum;
    QFileInfo fileData;

    const QString currentFolderSlash = Path::AddSlash(currentFolder);

    for (bool bFoundFile = fileEnum.StartEnumeration(currentFolder, "*", &fileData);
         bFoundFile; bFoundFile = fileEnum.GetNextFile(&fileData))
    {
        const QString fileName = fileData.fileName();
        const QString ext = Path::GetExt(fileName);

        // Have we found a folder?
        if (fileData.isDir())
        {
            // Skip the parent folder entries
            if (fileName == "." || fileName == "..")
            {
                continue;
            }

            LoadTreeRec(currentFolderSlash + fileName);
        }

        AddItem(currentFolderSlash + fileName);
    }
}

void CFolderTreeCtrl::AddItem(const QString& path)
{
    QString folder;
    QString fileNameWithoutExtension;
    QString ext;

    Path::Split(path, folder, fileNameWithoutExtension, ext);

    if (path.contains(QRegExp(m_fileNameSpec, Qt::CaseInsensitive, QRegExp::Wildcard)))
    {
        CTreeItem* folderTreeItem = CreateFolderItems(folder);
        CTreeItem* fileTreeItem = folderTreeItem->AddChild(fileNameWithoutExtension, path, eTreeImage_File);
    }
}

void CFolderTreeCtrl::RemoveItem(const QString& path)
{
    if (!CFileUtil::FileExists(path))
    {
        auto findIter = m_pathToTreeItem.find(path);

        if (findIter != m_pathToTreeItem.end())
        {
            CTreeItem* foundItem = findIter->second;
            foundItem->Remove();
            RemoveEmptyFolderItems(Path::GetPath(path));
        }
    }
}

CFolderTreeCtrl::CTreeItem* CFolderTreeCtrl::GetItem(const QString& path)
{
    auto findIter = m_pathToTreeItem.find(path);

    if (findIter == m_pathToTreeItem.end())
    {
        return nullptr;
    }

    return findIter->second;
}

QString CFolderTreeCtrl::CalculateFolderFullPath(const QStringList& splittedFolder, int idx)
{
    QString path;
    for (int segIdx = 0; segIdx <= idx; ++segIdx)
    {
        if (segIdx != 0)
        {
            path.append(QLatin1Char('/'));
        }
        path.append(splittedFolder[segIdx]);
    }
    return path;
}

CFolderTreeCtrl::CTreeItem* CFolderTreeCtrl::CreateFolderItems(const QString& folder)
{
    QStringList splittedFolder = Path::SplitIntoSegments(folder);
    CTreeItem* currentTreeItem = m_rootTreeItem.get();

    if (!m_bFlatStyle)
    {
        QString currentFolder;
        QString fullpath;
        const int splittedFoldersCount = splittedFolder.size();
        for (int idx = 0; idx < splittedFoldersCount; ++idx)
        {
            currentFolder = Path::RemoveBackslash(splittedFolder[idx]);
            fullpath = CalculateFolderFullPath(splittedFolder, idx);

            CTreeItem* folderItem = GetItem(fullpath);
            if (!folderItem)
            {
                currentTreeItem = currentTreeItem->AddChild(currentFolder, fullpath, eTreeImage_Folder);
            }
            else
            {
                currentTreeItem = folderItem;
            }

            currentTreeItem->setExpanded(true);
        }
    }

    m_rootTreeItem->setExpanded(true);
    return currentTreeItem;
}

void CFolderTreeCtrl::RemoveEmptyFolderItems(const QString& folder)
{
    QStringList splittedFolder = Path::SplitIntoSegments(folder);
    const int splittedFoldersCount = splittedFolder.size();
    QString fullpath;
    for (int idx = 0; idx < splittedFoldersCount; ++idx)
    {
        fullpath = CalculateFolderFullPath(splittedFolder, idx);
        CTreeItem* folderItem = GetItem(fullpath);
        if (!folderItem)
        {
            continue;
        }

        if (!folderItem->HasChildren())
        {
            folderItem->Remove();
        }
    }
}

void CFolderTreeCtrl::Edit(const QString& path)
{
    CFileUtil::EditTextFile(QtUtil::ToString(path), 0, IFileUtil::FILE_TYPE_SCRIPT);
}

void CFolderTreeCtrl::ShowInExplorer(const QString& path)
{
    QString absolutePath = QDir::currentPath();

    CTreeItem* root = m_rootTreeItem.get();
    CTreeItem* item = GetItem(path);

    if (item != root)
    {
        absolutePath += QStringLiteral("/%1").arg(path);
    }

#if defined(AZ_PLATFORM_WINDOWS)
    // Explorer command lines needs windows style paths
    absolutePath.replace('/', '\\');

    const QString parameters = "/select," + absolutePath;
    QProcess::startDetached(QStringLiteral("explorer.exe"), { parameters });
#else
    QProcess::startDetached("/usr/bin/osascript", {"-e",
        QStringLiteral("tell application \"Finder\" to reveal POSIX file \"%1\"").arg(absolutePath)});
    QProcess::startDetached("/usr/bin/osascript", {"-e",
        QStringLiteral("tell application \"Finder\" to activate")});
#endif
}

QPixmap CFolderTreeCtrl::GetPixmap(int image) const
{
    return image == eTreeImage_File ? m_filePixmap : m_folderPixmap;
}


//////////////////////////////////////////////////////////////////////////
// CFolderTreeCtrl::CTreeItem
//////////////////////////////////////////////////////////////////////////
CFolderTreeCtrl::CTreeItem::CTreeItem(CFolderTreeCtrl& folderTreeCtrl, const QString& path)
    : m_folderTreeCtrl(folderTreeCtrl)
    , m_path(path)
{
    setData(0, Qt::DisplayRole, m_folderTreeCtrl.m_rootName);
    setData(0, Qt::DecorationRole, folderTreeCtrl.GetPixmap(eTreeImage_Folder));
    setData(0, IsFolderRole, true);

    folderTreeCtrl.addTopLevelItem(this);
    m_folderTreeCtrl.m_pathToTreeItem[ m_path ] = this;
}

CFolderTreeCtrl::CTreeItem::CTreeItem(CFolderTreeCtrl& folderTreeCtrl, CFolderTreeCtrl::CTreeItem* parent,
    const QString& name, const QString& path, const int image)
    : m_folderTreeCtrl(folderTreeCtrl)
    , m_path(path)
{
    parent->addChild(this);
    setData(0, Qt::DisplayRole, name);
    setData(0, Qt::DecorationRole, folderTreeCtrl.GetPixmap(image));
    setData(0, IsFolderRole, image == eTreeImage_Folder);

    m_folderTreeCtrl.m_pathToTreeItem[ m_path ] = this;
}

CFolderTreeCtrl::CTreeItem::~CTreeItem()
{
    m_folderTreeCtrl.m_pathToTreeItem.erase(m_path);
}

void CFolderTreeCtrl::CTreeItem::Remove()
{
    // Root can't be deleted this way
    if (parent())
    {
        parent()->removeChild(this);
    }
}

CFolderTreeCtrl::CTreeItem* CFolderTreeCtrl::CTreeItem::AddChild(const QString& name, const QString& path, const int image)
{
    CTreeItem* newItem = new CTreeItem(m_folderTreeCtrl, this, name, path, image);
    sortChildren(0, Qt::AscendingOrder);
    return newItem;
}
