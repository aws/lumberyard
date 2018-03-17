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
#include "EntityLinksPanel.h"
#include "Objects/EntityObject.h"
#include "Controls/PickObjectButton.h"
#include "StringDlg.h"

#include "Mission.h"
#include "MissionScript.h"
#include "EntityPrototype.h"

#include <ui_EntityLinksPanel.h>

#include <QAbstractTableModel>
#include <QMenu>
#include "QtUtil.h"

class LinksModel
    : public QAbstractTableModel
{
public:
    LinksModel(QObject* parent)
        : QAbstractTableModel(parent)
        , m_entity(nullptr) {}

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void SetEntity(class CEntityObject* entity) { m_entity = entity; ReloadLinks(); }
    void ReloadLinks();

private:
    CEntityObject* m_entity;
};

Qt::ItemFlags LinksModel::flags(const QModelIndex& index) const
{
    auto res = QAbstractTableModel::flags(index);
    if (index.column() == 0)
    {
        res |= Qt::ItemIsEditable;
    }
    return res;
}

QModelIndex LinksModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
    return createIndex(row, column);
}

int LinksModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return m_entity ? m_entity->GetEntityLinkCount() : 0;
}

int LinksModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return 2;
}

QVariant LinksModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    if (!m_entity || index.row() < 0 || index.row() >= m_entity->GetEntityLinkCount())
    {
        return {};
    }

    CEntityLink& link = m_entity->GetEntityLink(index.row());

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
        case 0:
            return link.name;
        case 1:
            return link.target->GetName();
        }
    }

    return {};
}

bool LinksModel::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
{
    if (!m_entity || index.row() < 0 || index.row() >= m_entity->GetEntityLinkCount() || role != Qt::EditRole || index.column() != 0)
    {
        return false;
    }
    if (value.type() != QVariant::String || value.toString().isEmpty())
    {
        return false;
    }

    m_entity->RenameEntityLink(index.row(), value.toString());
    return true;
}

QVariant LinksModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0:
            return "Link Name";
        case 1:
            return "Entity";
        }
    }

    return {};
}

void LinksModel::ReloadLinks()
{
    beginResetModel();
    endResetModel();
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CEntityLinksPanel dialog


CEntityLinksPanel::CEntityLinksPanel(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , ui(new Ui::CEntityLinksPanel)
    , m_entity(nullptr)
    , m_currentLinkIndex(-1)
{
    ui->setupUi(this);

    m_pickButton = ui->PICK;
    m_pickButton->SetPickCallback(this, tr("Pick Target Entity"), &CEntityObject::staticMetaObject, true);

    m_links = ui->LINKS;
    m_links->setModel(new LinksModel(this));
    m_links->setContextMenuPolicy(Qt::CustomContextMenu);
    m_links->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_links, &QTableView::customContextMenuRequested, this, &CEntityLinksPanel::OnRclickLinks);
    connect(m_links, &QTableView::doubleClicked, this, &CEntityLinksPanel::OnDblClickLinks);
}

CEntityLinksPanel::~CEntityLinksPanel()
{
    if (m_pickButton->IsSelected())
    {
        GetIEditor()->SetEditTool(0);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CEntityLinksPanel message handlers
void CEntityLinksPanel::SetEntity(CEntityObject* entity)
{
    assert(entity);

    m_entity = entity;
    static_cast<LinksModel*>(m_links->model())->SetEntity(entity);
    ReloadLinks();
}

void CEntityLinksPanel::ReloadLinks()
{
    static_cast<LinksModel*>(m_links->model())->ReloadLinks();
}

//////////////////////////////////////////////////////////////////////////
void CEntityLinksPanel::OnBnClickedRemove(int nCurrentIndex)
{
    CUndo undo("Remove Entity Link");
    m_entity->RemoveEntityLink(nCurrentIndex);
    ReloadLinks();
}

//////////////////////////////////////////////////////////////////////////
void CEntityLinksPanel::OnPick(CBaseObject* picked)
{
    CEntityObject* pickedEntity = (CEntityObject*)picked;
    if (!pickedEntity)
    {
        return;
    }

    CUndo undo("Add EntityLink");

    CEntityLink* pLink = 0;

    QString linkName = "NewLink";
    // Replace previous link.
    if (m_currentLinkIndex >= 0 && m_currentLinkIndex < m_entity->GetEntityLinkCount())
    {
        linkName = m_entity->GetEntityLink(m_currentLinkIndex).name;
        m_entity->RemoveEntityLink(m_currentLinkIndex);
    }

    if (m_entity->EntityLinkExists(linkName, pickedEntity->GetId()) == false)
    {
        m_entity->AddEntityLink(linkName, pickedEntity->GetId());
    }
    else
    {
        CryLog("Attempt to add duplicate entity link was blocked, linkname '%s' to entity '%s'", linkName.toUtf8().data(), pickedEntity->GetName().toUtf8().data());
    }

    ReloadLinks();
}

void CEntityLinksPanel::OnCancelPick()
{
    m_currentLinkIndex = -1;
    ui->PICK->setChecked(false);
}

#define CMD_PICK      1
#define CMD_PICK_NEW  2
#define CMD_DELETE    3
#define CMD_RENAME    4

void CEntityLinksPanel::OnRclickLinks(QPoint localPoint)
{
    int nIndex = m_links->indexAt(localPoint).row();
    CEntityLink* pLink = 0;

    if (nIndex >= 0 && nIndex < m_entity->GetEntityLinkCount())
    {
        pLink = &m_entity->GetEntityLink(nIndex);
    }

    QMenu menu;

    m_currentLinkIndex = -1;

    if (pLink)
    {
        menu.addAction("Change Target Entity")->setData(CMD_PICK);
        menu.addAction("Rename Link")->setData(CMD_RENAME);
        menu.addAction("Delete Link")->setData(CMD_DELETE);

        menu.addSeparator();
        menu.addAction("Pick New Target")->setData(CMD_PICK_NEW);
    }
    else
    {
        menu.addAction("Pick New Target")->setData(CMD_PICK);
    }

    QAction* action = menu.exec(QCursor::pos());
    int res = action ? action->data().toInt() : 0;
    switch (res)
    {
    case CMD_PICK:
        m_currentLinkIndex = nIndex;
        m_pickButton->click();
        break;
    case CMD_PICK_NEW:
        m_currentLinkIndex = -1;
        m_pickButton->click();
        break;
    case CMD_DELETE:
        OnBnClickedRemove(nIndex);
        break;
    case CMD_RENAME:
        m_links->edit(m_links->model()->index(nIndex, 0));
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityLinksPanel::OnDblClickLinks(const QModelIndex& index)
{
    if (m_entity)
    {
        if (index.row() >= 0 && index.row() < m_entity->GetEntityLinkCount())
        {
            if (index.column() == 1)
            {
                CEntityLink& link = m_entity->GetEntityLink(index.row());
                if (link.target)
                {
                    // Select entity.
                    CUndo undo("Select Object(s)");
                    GetIEditor()->ClearSelection();
                    GetIEditor()->SelectObject(link.target);
                }
            }
            else
            {
                m_links->edit(index);
            }
        }
    }
}
