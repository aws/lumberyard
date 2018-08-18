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
#include "SelectGameTokenDialog.h"
#include <IDataBaseItem.h>
#include <IDataBaseManager.h>
#include <IDataBaseLibrary.h>
#include <GameTokens/GameTokenItem.h>
#include <GameTokens/GameTokenManager.h>
#include "HyperGraph/HyperGraphDialog.h"
#include "HyperGraph/FlowGraph.h"

#include <QAbstractItemModel>
#include <QMap>

// CSelectGameTokenDialog dialog

class SelectGameTokenModel
    : public QAbstractItemModel
{
    struct TokenItem;

    enum TokenType
    {
        RootType,
        GraphType,
        LibraryType,
        GroupType,
        ItemType
    };

    typedef QSharedPointer<TokenItem> SharedTokenItem;
    struct TokenItem
    {
        TokenType type;
        QString text;
        QVariant value;
        TokenItem* parent;
        QVector<SharedTokenItem> children;
    } m_root;

public:
    SelectGameTokenModel(QObject* parent);
    ~SelectGameTokenModel();

    void ReloadGameTokens();

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

protected:
    TokenItem* ItemFromIndex(const QModelIndex& index) const;
    TokenItem* CreateItem(TokenItem* parent) const;

private:
    QImage m_imgLevel;
    QImage m_imgFolder;
};

//////////////////////////////////////////////////////////////////////////
SelectGameTokenModel::SelectGameTokenModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_imgLevel(":/img/tree_view_level.png")
    , m_imgFolder(":/img/tree_view_folder.png")
{
    m_root = {RootType, QString(), QVariant(), nullptr, QVector<SharedTokenItem>()};
}

//////////////////////////////////////////////////////////////////////////
SelectGameTokenModel::~SelectGameTokenModel()
{
}

//////////////////////////////////////////////////////////////////////////
SelectGameTokenModel::TokenItem* SelectGameTokenModel::ItemFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return const_cast<TokenItem*>(&m_root);
    }
    return reinterpret_cast<TokenItem*>(index.internalPointer());
}

//////////////////////////////////////////////////////////////////////////
SelectGameTokenModel::TokenItem* SelectGameTokenModel::CreateItem(TokenItem* parent) const
{
    TokenItem* item = new TokenItem;

    item->parent = parent;
    if (parent)
    {
        parent->children.append(SharedTokenItem(item));
    }
    return item;
}

//////////////////////////////////////////////////////////////////////////
void SelectGameTokenModel::ReloadGameTokens()
{
    beginResetModel();

    m_root.children.clear();

    CGameTokenManager* pMgr = GetIEditor()->GetGameTokenManager();
    IDataBaseItemEnumerator* pEnum = pMgr->GetItemEnumerator();
    IDataBaseItem* pItem = pEnum->GetFirst();

    QHash<QString, TokenItem*> treeItems;
    QMap<QString, CGameTokenItem*> items;

    while (pItem)
    {
        CGameTokenItem* pToken = (CGameTokenItem*) pItem;
        items[pToken->GetFullName()] = pToken;
        pItem = pEnum->GetNext();
    }
    pEnum->Release();

    // items now sorted, make the tree
    CHyperGraphDialog* pHyperGraphDialog = CHyperGraphDialog::instance();
    if (pHyperGraphDialog)
    {
        if (CHyperGraph* pGraph = pHyperGraphDialog->GetGraph())
        {
            if (pGraph->IsFlowGraph())
            {
                IFlowGraph* pIFlowGraph = ((CFlowGraph*)pGraph)->GetIFlowGraph();

                if (size_t tokenCount = pIFlowGraph->GetGraphTokenCount())
                {
                    TokenItem* pRootGraphTokens = CreateItem(&m_root);
                    pRootGraphTokens->type = GraphType;
                    pRootGraphTokens->text = QStringLiteral("Graph Tokens");

                    for (size_t i = 0; i < tokenCount; ++i)
                    {
                        TokenItem* pNewItem = CreateItem(pRootGraphTokens);
                        pNewItem->type = GraphType;
                        pNewItem->text = pIFlowGraph->GetGraphToken(i)->name;
                        pNewItem->value = pNewItem->text;
                    }
                }
            }
        }
    }

    QMap<QString, CGameTokenItem*>::iterator iter = items.begin();
    while (iter != items.end())
    {
        QString libName;
        IDataBaseLibrary* pLib = iter.value()->GetLibrary();
        if (pLib != 0)
        {
            libName = pLib->GetName();
        }
        const QString groupName = iter.value()->GetGroupName();

        // for now circumvent a bug in the database GetShortName when the item name
        // contains a ".". then short name returns only the last piece of it
        // which is wrong (say group: GROUP1 and name "NAME.SUBNAME" then
        // short name returns only SUBNAME.
        QString shortName = iter.value()->GetName(); // incl. group
        int i = shortName.indexOf(groupName + ".");
        if (i >= 0)
        {
            shortName = shortName.mid(groupName.length() + 1);
        }

        TokenItem* pRoot = &m_root;

        if (!libName.isEmpty())
        {
            if (!treeItems.contains(libName))
            {
                TokenItem* pNewItem = CreateItem(&m_root);
                pNewItem->type = LibraryType;
                pNewItem->text = libName;
                pRoot = pNewItem;
                treeItems.insert(libName, pRoot);
            }
            else
            {
                pRoot = treeItems.value(libName);
            }
        }

        if (!groupName.isEmpty())
        {
            QString combinedName = libName;
            if (!libName.isEmpty())
            {
                combinedName += ".";
            }
            combinedName += groupName;

            if (!treeItems.contains(combinedName))
            {
                TokenItem* pNewItem = CreateItem(pRoot);
                pNewItem->type = GroupType;
                pNewItem->text = groupName;
                pRoot = pNewItem;
                treeItems.insert(combinedName, pRoot);
            }
            else
            {
                pRoot = treeItems.value(combinedName);
            }
        }

        TokenItem* pNewItem = CreateItem(pRoot);
        pNewItem->type = ItemType;
        pNewItem->text = shortName;
        pNewItem->value = iter.value()->GetFullName();

        ++iter;
    }

    endResetModel();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex SelectGameTokenModel::index(int row, int column, const QModelIndex& parent) const
{
    const TokenItem* pParent = ItemFromIndex(parent);
    Q_ASSERT(pParent);

    if (row < 0 || row >= pParent->children.count())
    {
        return QModelIndex();
    }

    return createIndex(row, column, pParent->children.at(row).data());
}

//////////////////////////////////////////////////////////////////////////
QModelIndex SelectGameTokenModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }

    TokenItem* pItem = ItemFromIndex(index);
    Q_ASSERT(pItem);

    TokenItem* pParent = pItem->parent;
    TokenItem* pGrandParent = pParent ? pParent->parent : nullptr;
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

//////////////////////////////////////////////////////////////////////////
int SelectGameTokenModel::rowCount(const QModelIndex& parent) const
{
    TokenItem* pParent = ItemFromIndex(parent);
    Q_ASSERT(pParent);
    return pParent->children.count();
}

//////////////////////////////////////////////////////////////////////////
int SelectGameTokenModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

//////////////////////////////////////////////////////////////////////////
QVariant SelectGameTokenModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    TokenItem* pItem = ItemFromIndex(index);
    Q_ASSERT(pItem);

    switch (role)
    {
    case Qt::DisplayRole:
        return pItem->text;
    case Qt::UserRole:
        return pItem->value;
    case Qt::DecorationRole:
        switch (pItem->type)
        {
        case GraphType:
        case ItemType:
            return m_imgLevel;
        case LibraryType:
        case GroupType:
            return m_imgFolder;
        }
        break;
    }

    return QVariant();
}


//////////////////////////////////////////////////////////////////////////
CSelectGameTokenDialog::CSelectGameTokenDialog(QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui_CSelectGameTokenDialog)
    , m_model(new SelectGameTokenModel(this))
{
    m_ui->setupUi(this);

    m_ui->treeView->setModel(m_model);

    connect(m_ui->treeView, &QTreeView::doubleClicked,
        this, &CSelectGameTokenDialog::OnTvnDoubleClick);
}

CSelectGameTokenDialog::~CSelectGameTokenDialog()
{
}

// CSelectGameTokenDialog message handlers

//////////////////////////////////////////////////////////////////////////
void CSelectGameTokenDialog::showEvent(QShowEvent* event)
{
    /* refresh tokens */
    m_model->ReloadGameTokens();
    m_ui->treeView->expandAll();

    /* search for preselected token */
    if (!m_preselect.isEmpty())
    {
        QModelIndexList select =
            m_model->match(m_model->index(0, 0), Qt::UserRole, m_preselect, 1,
                Qt::MatchFixedString | Qt::MatchWrap | Qt::MatchRecursive);
        if (!select.isEmpty())
        {
            m_ui->treeView->setCurrentIndex(select.at(0));
        }
    }

    QDialog::showEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CSelectGameTokenDialog::OnTvnDoubleClick(const QModelIndex& index)
{
    if (index.isValid() && index.data(Qt::UserRole).type() == QVariant::String)
    {
        accept();
    }
}

//////////////////////////////////////////////////////////////////////////
QString CSelectGameTokenDialog::GetSelectedGameToken() const
{
    if (m_ui->treeView->currentIndex().isValid())
    {
        return m_ui->treeView->currentIndex().data(Qt::UserRole).toString();
    }
    return QString();
}

#include "SelectGameTokenDialog.moc"
