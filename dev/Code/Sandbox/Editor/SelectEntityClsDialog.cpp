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

#include "StdAfx.h"
#include "SelectEntityClsDialog.h"

#include "Objects/EntityScript.h"

#include <QAbstractItemModel>

class SelectEntityClsModel
    : public QAbstractItemModel
{
    struct EntryItem;
    typedef QSharedPointer<EntryItem> SharedEntryItem;

    struct EntryItem
    {
        QString name;
        EntryItem* parent;
        QVector<SharedEntryItem> children;
    } m_root;

public:
    SelectEntityClsModel(QObject* parent);
    ~SelectEntityClsModel();

    void ReloadEntities();

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

protected:
    EntryItem* ItemFromIndex(const QModelIndex& index) const;
    EntryItem* CreateItem(EntryItem* parent = nullptr) const;

private:
    QImage m_imgLevel;
    QImage m_imgFolder;
};

SelectEntityClsModel::SelectEntityClsModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_imgLevel(":/img/tree_view_level.png")
    , m_imgFolder(":/img/tree_view_folder.png")
{
    m_root = { QString(), nullptr, QVector<SharedEntryItem>() };
}

SelectEntityClsModel::~SelectEntityClsModel()
{
}

QModelIndex SelectEntityClsModel::index(int row, int column, const QModelIndex& parent) const
{
    const EntryItem* pParent = ItemFromIndex(parent);
    Q_ASSERT(pParent);

    if (row < 0 || row >= pParent->children.count())
    {
        return QModelIndex();
    }

    return createIndex(row, column, pParent->children.at(row).data());
}

QModelIndex SelectEntityClsModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }

    EntryItem* pItem = ItemFromIndex(index);
    Q_ASSERT(pItem);

    EntryItem* pParent = pItem->parent;
    EntryItem* pGrandParent = pParent ? pParent->parent : nullptr;
    if (!pParent || !pGrandParent)
    {
        return QModelIndex();
    }

    int row;
    const int siblingCount = pGrandParent->children.count();
    for (row = 0; row < siblingCount; ++row)
    {
        if (pGrandParent->children.at(row).data() == pParent)
        {
            break;
        }
    }

    if (row >= siblingCount)
    {
        return QModelIndex();
    }

    return createIndex(row, index.column(), pParent);
}

int SelectEntityClsModel::rowCount(const QModelIndex& parent) const
{
    EntryItem* pParent = ItemFromIndex(parent);
    Q_ASSERT(pParent);
    return pParent->children.count();
}

int SelectEntityClsModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

QVariant SelectEntityClsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    EntryItem* pItem = ItemFromIndex(index);
    Q_ASSERT(pItem);

    switch (role)
    {
    case Qt::DisplayRole:
        return pItem->name;

    case Qt::UserRole:
        if (pItem->children.isEmpty())
        {
            return pItem->name;
        }

    case Qt::DecorationRole:
        if (pItem->children.isEmpty())
        {
            return m_imgLevel;
        }
        else
        {
            return m_imgFolder;
        }
    }

    return QVariant();
}

void SelectEntityClsModel::ReloadEntities()
{
    beginResetModel();

    m_root.children.clear();

    QHash<QString, EntryItem*> items;
    // Entity scripts.
    std::vector<CEntityScript*> scripts;
    CEntityScriptRegistry::Instance()->GetScripts(scripts);
    for (int i = 0; i < scripts.size(); i++)
    {
        // If class is not usable simply skip it.
        if (!scripts[i]->IsUsable())
        {
            continue;
        }

        const QString name = scripts[i]->GetName();

        EntryItem* hRoot = &m_root;

        QString clsFile = scripts[i]->GetFile();
        clsFile.replace("Scripts/Default/Entities/", "");

        QStringList tokens = clsFile.split('/', QString::SkipEmptyParts);
        if (!tokens.isEmpty())
        {
            tokens.pop_back();
        }

        QString itemName;
        for (const QString& strToken : tokens)
        {
            itemName += strToken + '/';
            if (!items.contains(itemName))
            {
                hRoot = CreateItem(hRoot);
                hRoot->name = strToken;
                items.insert(itemName, hRoot);
            }
            else
            {
                hRoot = items.value(itemName);
            }
        }

        EntryItem* hNewItem = CreateItem(hRoot);
        hNewItem->name = name;
    }

    endResetModel();
}

//////////////////////////////////////////////////////////////////////////
SelectEntityClsModel::EntryItem* SelectEntityClsModel::ItemFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return const_cast<EntryItem*>(&m_root);
    }
    return reinterpret_cast<EntryItem*>(index.internalPointer());
}

//////////////////////////////////////////////////////////////////////////
SelectEntityClsModel::EntryItem* SelectEntityClsModel::CreateItem(EntryItem* parent) const
{
    EntryItem* item = new EntryItem;
    item->parent = parent;
    if (parent)
    {
        parent->children.push_back(SharedEntryItem(item));
    }
    return item;
}

// CSelectEntityClsDialog dialog

CSelectEntityClsDialog::CSelectEntityClsDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui_CSelectEntityClsDialog)
    , m_model(new SelectEntityClsModel(this))
{
    m_ui->setupUi(this);

    m_ui->treeView->setModel(m_model);

    connect(m_ui->treeView, &QTreeView::doubleClicked,
        this, &CSelectEntityClsDialog::OnTvnDoubleClick);
}

CSelectEntityClsDialog::~CSelectEntityClsDialog()
{
}


//////////////////////////////////////////////////////////////////////////
QString CSelectEntityClsDialog::GetEntityClass() const
{
    if (m_ui->treeView->currentIndex().isValid())
    {
        return m_ui->treeView->currentIndex().data(Qt::UserRole).toString();
    }
    return QString();
}

//////////////////////////////////////////////////////////////////////////
void CSelectEntityClsDialog::OnTvnDoubleClick(const QModelIndex& index)
{
    if (index.isValid() && index.data(Qt::UserRole).type() == QVariant::String)
    {
        accept();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSelectEntityClsDialog::showEvent(QShowEvent* event)
{
    m_model->ReloadEntities();
    m_ui->treeView->expandAll();
    QDialog::showEvent(event);
}

#include "SelectEntityClsDialog.moc"
