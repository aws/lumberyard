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
#include "AIEntityClassesDialog.h"

#include <ui_AIEntityClassesDialog.h>

#include "AI/AIManager.h"
#include "IAgent.h"

class AIEntityClassesModel
    : public QAbstractListModel
{
public:
    AIEntityClassesModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    void Reload();
    QString GetAIEntityClasses() const;
    void SetAIEntityClasses(const QString& sAIEntityClasses);

    int rowCount(const QModelIndex& parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

private:
    std::vector<std::pair<QString, bool> > m_entityClasses;
};

void AIEntityClassesModel::Reload()
{
    beginResetModel();

    m_entityClasses.clear();

    const QString sStartPath1 = "Scripts/Entities/AI/NewHumans/Human";
    const QString sStartPath2 = "Scripts/Entities/AI/NewAliens/Alien";

    IEntityClass* pEntityClass;
    IEntityClassRegistry* pEntityClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
    for (pEntityClassRegistry->IteratorMoveFirst(); pEntityClass = pEntityClassRegistry->IteratorNext(); )
    {
        QString sFileName = pEntityClass->GetScriptFile();

        assert(sStartPath1.length() == sStartPath2.length()); // We implicitly use it in the next line.
        if (!sFileName.isEmpty() && (sFileName.length() > sStartPath1.length()))
        {
            if ((sFileName.left(sStartPath1.length()) == sStartPath1) ||
                (sFileName.left(sStartPath2.length()) == sStartPath2))
            {
                // check the table has properties (a good filter)
                SmartScriptTable pSSTable;
                gEnv->pScriptSystem->GetGlobalValue(pEntityClass->GetName(), pSSTable);
                if (pSSTable && pSSTable->HaveValue("Properties"))
                {
                    QString sClassName = QtUtil::ToQString(pEntityClass->GetName());
                    m_entityClasses.push_back(std::make_pair(sClassName, false));
                }
            }
        }
    }

    std::sort(std::begin(m_entityClasses), std::end(m_entityClasses),
        [](const std::pair<QString, bool>& a, const std::pair<QString, bool>& b)
        {
            return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
        });

    endResetModel();
}

int AIEntityClassesModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_entityClasses.size();
}

Qt::ItemFlags AIEntityClassesModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if (index.isValid())
    {
        flags |= Qt::ItemIsUserCheckable;
    }

    return flags;
}

QVariant AIEntityClassesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_entityClasses.size())
    {
        return {};
    }

    auto& item = m_entityClasses[index.row()];

    switch (role)
    {
    case Qt::DisplayRole:
        return item.first;

    case Qt::CheckStateRole:
        return item.second ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

bool AIEntityClassesModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.row() >= m_entityClasses.size())
    {
        return false;
    }

    auto& item = m_entityClasses[index.row()];

    switch (role)
    {
    case Qt::CheckStateRole:
        item.second = !item.second;
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

QString AIEntityClassesModel::GetAIEntityClasses() const
{
    QString entityClasses;

    for (auto it = std::begin(m_entityClasses), end = std::end(m_entityClasses); it != end; ++it)
    {
        if (it->second)
        {
            if (!entityClasses.isEmpty())
            {
                entityClasses += ", ";
            }
            entityClasses += it->first;
        }
    }

    return entityClasses;
}

void AIEntityClassesModel::SetAIEntityClasses(const QString& sAIEntityClasses)
{
    for (auto& item : m_entityClasses)
    {
        item.second = false;
    }

    QStringList entityClasses = sAIEntityClasses.split(QRegularExpression(QStringLiteral("[ ,]")), QString::SkipEmptyParts);

    for (auto& entityClass : entityClasses)
    {
        auto it = std::find_if(std::begin(m_entityClasses), std::end(m_entityClasses),
                [&](std::pair<QString, bool>& item)
                {
                    return item.first == entityClass;
                });

        if (it != std::end(m_entityClasses))
        {
            it->second = true;
        }
    }
}

CAIEntityClassesDialog::CAIEntityClassesDialog(QWidget* pParent)
    : QDialog(pParent)
    , m_model(new AIEntityClassesModel(this))
    , ui(new Ui::CAIEntityClassesDialog)
{
    ui->setupUi(this);

    ui->m_TreeCtrl->setModel(m_model);

    UpdateList();

    connect(ui->m_TreeCtrl->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CAIEntityClassesDialog::OnTVSelChanged);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &CAIEntityClassesDialog::UpdateAIEntityClassesString);
    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CAIEntityClassesDialog::accept);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CAIEntityClassesDialog::reject);
}

CAIEntityClassesDialog::~CAIEntityClassesDialog()
{
}

void CAIEntityClassesDialog::OnTVSelChanged()
{
    UpdateDescription();
}


void CAIEntityClassesDialog::UpdateList()
{
    m_model->Reload();
    UpdateDescription();
}


void CAIEntityClassesDialog::UpdateDescription()
{
    auto index = ui->m_TreeCtrl->currentIndex();
    if (index.isValid())
    {
        QString sAIEntityClass = index.data().toString();
        for (size_t i = 0; i < sAIEntityClass.size(); ++i)
        {
            if (sAIEntityClass.at(i).isUpper())
            {
                sAIEntityClass.insert(i, ' ');
                i += 2;
            }
        }
        ui->m_description->setText(sAIEntityClass);
    }
}


void CAIEntityClassesDialog::UpdateAIEntityClassesString()
{
    ui->m_selectionList->setText(GetAIEntityClasses());
        
}


QString CAIEntityClassesDialog::GetAIEntityClasses() const
{
    return m_model->GetAIEntityClasses();
}

void CAIEntityClassesDialog::SetAIEntityClasses(const QString& sAIEntityClasses)
{
    m_model->SetAIEntityClasses(sAIEntityClasses);
}

#include <AIEntityClassesDialog.moc>
