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
#include "AIPFPropertiesListDialog.h"

#include "AI/AIManager.h"
#include "IAgent.h"

#include <QAbstractListModel>
#include <QMessageBox>

class AIPFPropertiesListModel
    : public QAbstractListModel
{
    struct PropertyItem
    {
        QString text;
        const AgentPathfindingProperties* prop;
        bool checked;
    };

    QVector<PropertyItem*> m_properties;
public:
    AIPFPropertiesListModel(QObject* parent = nullptr);
    ~AIPFPropertiesListModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void UpdateList(const QStringList& precheckList);

    const AgentPathfindingProperties* PropertyFromIndex(const QModelIndex& index) const;
};

AIPFPropertiesListModel::AIPFPropertiesListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

AIPFPropertiesListModel::~AIPFPropertiesListModel()
{
    qDeleteAll(m_properties);
}

int AIPFPropertiesListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return m_properties.count();
}

QVariant AIPFPropertiesListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    Q_ASSERT(index.row() < m_properties.count());
    PropertyItem* item = m_properties.at(index.row());

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return item->text;
    case Qt::CheckStateRole:
        return item->checked ? Qt::Checked : Qt::Unchecked;
    }

    return QVariant();
}

bool AIPFPropertiesListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
    {
        return false;
    }

    Q_ASSERT(index.row() < m_properties.count());
    PropertyItem* item = m_properties.at(index.row());

    switch (role)
    {
    case Qt::CheckStateRole:
        item->checked = value.toBool();
        emit dataChanged(index, index, QVector<int>{role});
        return true;
    }

    return false;
}

Qt::ItemFlags AIPFPropertiesListModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = QAbstractListModel::flags(index);
    flags |= Qt::ItemIsUserCheckable;
    return flags;
}

void AIPFPropertiesListModel::UpdateList(const QStringList& precheckList)
{
    beginResetModel();

    qDeleteAll(m_properties);
    m_properties.clear();

    typedef QHash<QString, const AgentPathfindingProperties*> PropertyMap;
    PropertyMap propMap;
    QSet<QString> propCheck;

    for (const auto& token : precheckList)
    {
        propMap.insert(token, nullptr);
        propCheck.insert(token);
    }

    CAIManager* pAIManager = GetIEditor()->GetAI();
    QStringList pathTypeNames = QString(pAIManager->GetPathTypeNames()).split(" ");

    for (const auto& token : pathTypeNames)
    {
        const AgentPathfindingProperties* prop =
            pAIManager->GetPFPropertiesOfPathType(qPrintable(token));
        propMap.insert(token, prop);
    }

    /*
     * Generate sorted list structure
     */

    const auto e = propMap.constEnd();
    for (auto i = propMap.constBegin(); i != e; ++i)
    {
        PropertyItem* item = new PropertyItem;
        item->text = i.key();
        item->checked = propCheck.contains(i.key());
        item->prop = i.value();
        m_properties.append(item);
    }

    endResetModel();
}

const AgentPathfindingProperties* AIPFPropertiesListModel::PropertyFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }

    Q_ASSERT(index.row() < m_properties.count());
    PropertyItem* item = m_properties.at(index.row());
    return item->prop;
}

CAIPFPropertiesListDialog::CAIPFPropertiesListDialog(QWidget* pParent)
    : QDialog(pParent)
    , m_ui(new Ui_CAIPFPropertiesListDialog)
    , m_model(new AIPFPropertiesListModel(this))
    , m_scriptFileName(QStringLiteral("Scripts/AI/pathfindProperties.lua"))
{
    m_ui->setupUi(this);
    m_ui->listView->setModel(m_model);

    connect(m_ui->editButton, &QPushButton::clicked,
        this, &CAIPFPropertiesListDialog::OnBnClickedEdit);
    connect(m_ui->refreshButton, &QPushButton::clicked,
        this, &CAIPFPropertiesListDialog::OnBnClickedRefresh);
    connect(m_ui->listView->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &CAIPFPropertiesListDialog::OnTVSelChanged);
    connect(m_model, &AIPFPropertiesListModel::dataChanged,
        this, &CAIPFPropertiesListDialog::OnTVDataChanged);
}

void CAIPFPropertiesListDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    OnBnClickedRefresh();
}

void CAIPFPropertiesListDialog::OnBnClickedEdit()
{
    CFileUtil::EditTextFile(qPrintable(m_scriptFileName));
}

void CAIPFPropertiesListDialog::OnBnClickedRefresh()
{
    bool success = CFileUtil::CompileLuaFile(qPrintable(m_scriptFileName))
        && GetIEditor()->GetSystem()->GetIScriptSystem()->ReloadScript(qPrintable(m_scriptFileName));

    if (success)
    {
        m_model->UpdateList(m_sPFPropertiesList.split(" ,"));
    }
    else
    {
        QMessageBox::critical(this, tr("Sandbox Error"), tr("Script file %1 reload failure!").arg(m_scriptFileName));
    }
}

void CAIPFPropertiesListDialog:: OnTVDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (!roles.contains(Qt::CheckStateRole))
    {
        return;
    }
    UpdatePFPropertiesString();
}

void CAIPFPropertiesListDialog::OnTVSelChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);
    UpdateDescription(current);
}

void CAIPFPropertiesListDialog::UpdateDescription(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    QString description;
    const QString pathTypeName = index.data().toString();
    const AgentPathfindingProperties* p = m_model->PropertyFromIndex(index);
    if (p)
    {
        description = tr(
                "%1:\r\n"
                "navCapMask = %2\r\n"
                "triangularResistanceFactor = %3\r\n"
                "waypointResistanceFactor = %4\r\n"
                "flightResistanceFactor = %5\r\n"
                "volumeResistanceFactor = %6\r\n"
                "roadResistanceFactor = %7\r\n"
                "waterResistanceFactor = %8\r\n"
                "maxWaterDepth = %9\r\n"
                "minWaterDepth = %10\r\n"
                "exposureFactor = %11\r\n"
                "dangerCost = %12\r\n"
                "zScale = %13\r\n"
                "customNavCapsMask = %14\r\n"
                "radius = %15\r\n"
                "height = %16\r\n"
                "maxSlope = %17")
                .arg(pathTypeName)
                .arg(p->navCapMask)
                .arg(p->triangularResistanceFactor)
                .arg(p->waypointResistanceFactor)
                .arg(p->flightResistanceFactor)
                .arg(p->volumeResistanceFactor)
                .arg(p->roadResistanceFactor)
                .arg(p->waterResistanceFactor)
                .arg(p->maxWaterDepth)
                .arg(p->minWaterDepth)
                .arg(p->exposureFactor)
                .arg(p->dangerCost)
                .arg(p->zScale)
                .arg(p->customNavCapsMask)
                .arg(p->radius)
                .arg(p->height)
                .arg(p->maxSlope);
    }
    else
    {
        description =
            tr("%1:\r\n\r\nPathfinding properties unknown.")
                .arg(pathTypeName);
    }

    m_ui->descriptionEdit->setPlainText(description);
}

void CAIPFPropertiesListDialog::UpdatePFPropertiesString()
{
    QStringList tokens;

    QModelIndexList indexes = m_model->match(m_model->index(0), Qt::CheckStateRole, Qt::Checked, -1);
    for (const auto& index : indexes)
    {
        tokens.append(index.data().toString());
    }

    m_sPFPropertiesList = tokens.join(", ");
    m_ui->selectionListEdit->setPlainText(m_sPFPropertiesList);
}

#include "AIPFPropertiesListDialog.moc"