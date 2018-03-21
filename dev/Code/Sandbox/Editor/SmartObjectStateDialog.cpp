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
#include "ItemDescriptionDlg.h"
#include "SmartObjects/SmartObjectsEditorDialog.h"
#include "SmartObjectStateDialog.h"
#include "AI/AIManager.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QMessageBox>

#include <Util/AbstractGroupProxyModel.h>

#include <ui_SmartObjectStateDialog.h>

// CSmartObjectStateDialog dialog

namespace {
    enum class Inclusion
    {
        Ignored,
        Included,
        Excluded
    };

    class SmartObjectState
    {
    public:
        SmartObjectState(CSOLibrary::CStateData& actual, Inclusion state = Inclusion::Ignored)
            : m_actual(&actual)
            , m_state(state)
        {
        }

        QString Name() const
        {
            return m_actual->name;
        }

        void SetName(const QString& name)
        {
            m_actual->name = name;
        }

        QString Location() const
        {
            return m_actual->location;
        }

        void SetLocation(const QString& location)
        {
            m_actual->location = location;
        }

        QStringList LocationPath() const
        {
            return Location().split('/', QString::SkipEmptyParts);
        }

        QString Description() const
        {
            return m_actual->description;
        }

        void SetDescription(const QString& description)
        {
            m_actual->description = description;
        }

        Inclusion State() const
        {
            return m_state;
        }

        void SetState(Inclusion state)
        {
            m_state = state;
        }

    private:
        CSOLibrary::CStateData* m_actual;
        Inclusion m_state;
    };

    class SmartObjectListModel
        : public QAbstractListModel
    {
    public:
        enum Roles
        {
            NameRole = Qt::UserRole,
            LocationRole,
            LocationPathRole,
            DescriptionRole,
            InclusionStateRole
        };

    public:
        SmartObjectListModel(QObject* parent, bool enableCheckStates)
            : QAbstractListModel(parent)
            , m_enableCheckStates(enableCheckStates)
        {
        }

        virtual ~SmartObjectListModel()
        {
        }

        int rowCount(const QModelIndex& parent = {}) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

        QModelIndex Add(const QString& location, const QString& name, const QString& description);
        void Remove(const QString& name);
        void Reset(const QStringList& included, const QStringList& excluded);

        QModelIndex Find(const QString& name) const;

        QStringList Included() const;
        QStringList Excluded() const;
        QString SmartObjectStateString() const;

    private:
        QStringList CollectNames(Inclusion state) const
        {
            // initialize destination
            QStringList d;

            // apply transformation
            std::for_each(m_states.cbegin(), m_states.cend(), [&](const SmartObjectState& v)
                {
                    if (v.State() == state)
                    {
                        d.push_back(std::move(v.Name()));
                    }
                });

            return d;
        }

        std::vector<SmartObjectState> m_states;
        const bool m_enableCheckStates;
    };


    int SmartObjectListModel::rowCount(const QModelIndex& parent /* = {} */) const
    {
        return parent.isValid() ? 0 : m_states.size();
    }

    QVariant SmartObjectListModel::data(const QModelIndex& index, int role /* = Qt::DisplayRole */) const
    {
        const int row = index.row();
        if (row < 0 || row >= m_states.size() || index.column() != 0)
        {
            return {};
        }

        auto& state = m_states[row];

        switch (role)
        {
        case NameRole:
        case Qt::DisplayRole:
            return state.Name();
        case LocationRole:
            return state.Location();
        case LocationPathRole:
            return state.LocationPath();
        case DescriptionRole:
            return state.Description();
        case Qt::CheckStateRole:
        {
            if (m_enableCheckStates)
            {
                switch (state.State())
                {
                case Inclusion::Ignored:
                    return Qt::Unchecked;
                case Inclusion::Included:
                    return Qt::Checked;
                case Inclusion::Excluded:
                    return Qt::PartiallyChecked;
                }
            }
            return {};
        }
        case Qt::FontRole:
        {
            if (state.State() != Inclusion::Ignored)
            {
                QFont font;
                font.setBold(true);
                return font;
            }
        }
        default:
            return {};
        }
    }

    bool SmartObjectListModel::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
    {
        const int row = index.row();
        if (row < 0 || row >= m_states.size() || index.column() != 0)
        {
            return {};
        }

        auto& state = m_states[row];

        switch (role)
        {
        case Qt::CheckStateRole:
            if (m_enableCheckStates)
            {
                switch (state.State())
                {
                case Inclusion::Ignored:
                    state.SetState(Inclusion::Included);
                    break;
                case Inclusion::Included:
                    state.SetState(Inclusion::Excluded);
                    break;
                case Inclusion::Excluded:
                    state.SetState(Inclusion::Ignored);
                    break;
                }
                emit dataChanged(index, index, { Qt::CheckStateRole });
                return true;
            }
        case InclusionStateRole:
            state.SetState(value.value<Inclusion>());
            emit dataChanged(index, index, { Qt::CheckStateRole });
            return true;
        }

        return false;
    }

    QStringList SmartObjectListModel::Included() const
    {
        return CollectNames(Inclusion::Included);
    }

    QStringList SmartObjectListModel::Excluded() const
    {
        return CollectNames(Inclusion::Excluded);
    }

    QString SmartObjectListModel::SmartObjectStateString() const
    {
        QStringList in = Included();
        QStringList out = Excluded();

        return in.join(',') + (out.isEmpty() ? QString() : QString("-%1").arg(out.join(',')));
    }

    QModelIndex SmartObjectListModel::Add(const QString& location, const QString& name, const QString& description)
    {
        CSOLibrary::AddState(name.toLocal8Bit().data(), description.toLocal8Bit().data(), location.toLocal8Bit().data());
        int row = std::distance(CSOLibrary::GetStates().begin(), CSOLibrary::FindState(name.toLocal8Bit().data()));

        beginInsertRows({}, row, row);
        endInsertRows();
        return createIndex(row, 0);
    }

    QModelIndex SmartObjectListModel::Find(const QString& name) const
    {
        auto position = std::find_if(m_states.cbegin(), m_states.cend(), [&](const SmartObjectState& s) { return s.Name() == name; });
        if (position != m_states.cend())
        {
            return createIndex(std::distance(m_states.cbegin(), position), 0);
        }

        return {};
    }

    void SmartObjectListModel::Remove(const QString& name)
    {
        auto position = CSOLibrary::FindState(name.toLocal8Bit().data());

        if (position != CSOLibrary::GetStates().end())
        {
            int row = std::distance(CSOLibrary::GetStates().begin(), position);
            beginRemoveRows({}, row, row);
            CSOLibrary::GetStates().erase(position);
            auto target = m_states.begin();
            std::advance(target, row);
            m_states.erase(target);
            endRemoveRows();
        }
    }

    void SmartObjectListModel::Reset(const QStringList& included, const QStringList& excluded)
    {
        emit beginResetModel();

        std::map<QString, Inclusion> inclusions;
        foreach(const QString&in, included)
        {
            inclusions[in] = Inclusion::Included;
        }
        foreach(const QString&out, excluded)
        {
            inclusions[out] = Inclusion::Excluded;
        }

        m_states.clear();
        auto& states = CSOLibrary::GetStates();
        m_states.reserve(states.size());

        for (auto& state : states)
        {
            m_states.emplace_back(SmartObjectState(state, inclusions[state.name]));
        }

        emit endResetModel();
    }

    class SmartObjectTreeModel
        : public AbstractGroupProxyModel
    {
    public:
        SmartObjectTreeModel(QWidget* parent = nullptr);

        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        //CEditorDialogScript* GetScriptAt( const QModelIndex &idx ) const;
    protected:
        QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override;
    };

    SmartObjectTreeModel::SmartObjectTreeModel(QWidget* parent /* = nullptr */)
        : AbstractGroupProxyModel(parent)
    {
    }

    Qt::ItemFlags SmartObjectTreeModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid() || index.column() != 0)
        {
            return Qt::NoItemFlags;
        }

        if (!hasChildren(index))
        {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren;
        }
        else
        {
            return Qt::ItemIsEnabled;
        }
    }

    QVariant SmartObjectTreeModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole */) const
    {
        if (role == Qt::DecorationRole)
        {
            if (hasChildren(index))
            {
                return QIcon(":/SmartObjectState/folder.png");
            }
            return {};
        }

        return AbstractGroupProxyModel::data(index, role);
    }

    QStringList SmartObjectTreeModel::GroupForSourceIndex(const QModelIndex& sourceIndex) const
    {
        return sourceIndex.isValid() ? sourceIndex.data(SmartObjectListModel::LocationPathRole).toStringList() : QStringList();
    }
}

Q_DECLARE_METATYPE(Inclusion)

CSmartObjectStateDialog::CSmartObjectStateDialog(const QString& soState, QWidget* parent /*=nullptr*/, bool multi /*=false*/, bool hasAdvanced /*= false*/)
    : QDialog(parent)
    , m_ui(new Ui::SmartObjectStateDialog)
    , m_model(new SmartObjectListModel(this, multi))
    , m_treeModel(new SmartObjectTreeModel(this))
    , m_bMultiple(multi)
{
    m_ui->setupUi(this);
    OnInitDialog(soState, hasAdvanced);
}

CSmartObjectStateDialog::~CSmartObjectStateDialog()
{
}

QString CSmartObjectStateDialog::GetSOState()
{
    return m_model->SmartObjectStateString();
}

void CSmartObjectStateDialog::Expand(QModelIndex index)
{
    while (index.isValid())
    {
        m_ui->statesTreeView->expand(index);
        index = index.parent();
    }
}

void CSmartObjectStateDialog::OnNewBtn()
{
    CItemDescriptionDlg dlg(nullptr, true, false, true);
    dlg.setWindowTitle(tr("New State"));
    dlg.setItemCaption(tr("State name:"));

    auto index = m_ui->statesTreeView->selectionModel()->selectedIndexes().first();
    dlg.setLocation(index.data(SmartObjectListModel::LocationRole).toString());

    if (dlg.exec() && CSOLibrary::StartEditing())
    {
        auto base = m_model->Find(dlg.item());
        if (base.isValid())
        {
            index = m_treeModel->mapFromSource(base);
            Expand(index);
            m_ui->statesTreeView->scrollTo(index);
            m_ui->statesTreeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

            if (!m_bMultiple)
            {
                m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
                m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
            QMessageBox::warning(this, tr("State Name Already Exists"), tr("Entered state name already exists...\n\nA new state with the same name can not be created!"));
        }
        else
        {
            if (dlg.location().size() == 0)
            {
                QMessageBox::warning(this, tr("Data Required"), tr("You must specify a location!"));
                return;
            }
            auto source = m_model->Add(dlg.location(), dlg.item(), dlg.description());
            auto index = m_treeModel->mapFromSource(source);

            Expand(index);
            m_ui->statesTreeView->scrollTo(index);
            m_ui->statesTreeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

            if (!m_bMultiple)
            {
                m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
                m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            }
        }
    }
}

void CSmartObjectStateDialog::OnEditBtn()
{
    auto index = m_ui->statesTreeView->selectionModel()->selectedIndexes().first();

    QString name = index.data(Qt::DisplayRole).toString();
    CSOLibrary::VectorStateData::iterator it = CSOLibrary::FindState(name.toUtf8().data());
    assert(it != CSOLibrary::GetStates().end());

    CItemDescriptionDlg dlg(this, false, false, true);
    dlg.setWindowTitle(tr("Edit Smart Object State"));
    dlg.setItemCaption(tr("State name:"));
    dlg.setItem(name);
    dlg.setDescription(it->description);
    dlg.setLocation(it->location);
    if (dlg.exec())
    {
        if (CSOLibrary::StartEditing())
        {
            it->description = dlg.description();
            if (it->location != dlg.location())
            {
                it->location = dlg.location();

                m_model->Remove(dlg.item());
                auto base = m_model->Add(dlg.location(), dlg.item(), dlg.description());
                auto index = m_treeModel->mapFromSource(base);
                Expand(index);
                m_ui->statesTreeView->scrollTo(index);
                m_ui->statesTreeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
            }
            m_ui->descriptionLabel->setText(dlg.description());
        }
    }
}

void CSmartObjectStateDialog::OnDeleteBtn()
{
    auto index = m_ui->statesTreeView->selectionModel()->selectedIndexes().first();

    QString name = index.data(Qt::DisplayRole).toString();
    CSOLibrary::VectorStateData::iterator it = CSOLibrary::FindState(name.toUtf8().data());
    assert(it != CSOLibrary::GetStates().end());
    if (CSOLibrary::StartEditing())
    {
        m_model->Remove(name);
    }
}

void CSmartObjectStateDialog::UpdateCSOLibrary(const QStringList& tokens)
{
    foreach(const QString&token, tokens)
    {
        if (CItemDescriptionDlg::ValidateItem(token))
        {
            if (CSOLibrary::FindState(token.toUtf8().data()) == CSOLibrary::GetStates().end())
            {
                CSOLibrary::AddState(token.toUtf8().data(), "", "");
            }
        }
    }
}

void CSmartObjectStateDialog::OnRefreshBtn(const QStringList& included, const QStringList& excluded)
{
    // KDAB the dialog was always being invoked in "multi" mode, even when a single state was desired
    // This was apparently some kind of bug workaround.
    UpdateCSOLibrary(included);
    UpdateCSOLibrary(excluded);

    m_model->Reset(included, excluded);

    QStringList selected = included + excluded;

    foreach(const QString&chosen, selected)
    {
        auto index = m_treeModel->mapFromSource(m_model->Find(chosen));

        Expand(index);
        m_ui->statesTreeView->scrollTo(index);
        m_ui->statesTreeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    }

    UpdateIncludeExcludeEdits();
}

void CSmartObjectStateDialog::OnTVSelChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QModelIndex chosen = selected.indexes().isEmpty() ? QModelIndex() : selected.indexes().first();
    QModelIndex rejected = deselected.indexes().isEmpty() ? QModelIndex() : deselected.indexes().first();

    if (!m_bMultiple)
    {
        m_treeModel->setData(rejected, QVariant::fromValue(Inclusion::Ignored), SmartObjectListModel::InclusionStateRole);
    }

    if (chosen.isValid())
    {
        m_ui->editButton->setEnabled(true);
        m_ui->deleteButton->setEnabled(true);
        m_ui->descriptionLabel->setText(chosen.data(SmartObjectListModel::DescriptionRole).toString());

        if (!m_bMultiple)
        {
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

            m_treeModel->setData(chosen, QVariant::fromValue(Inclusion::Included), SmartObjectListModel::InclusionStateRole);
            setWindowTitle(tr("Smart Object State: %1").arg(chosen.data(SmartObjectListModel::NameRole).toString()));
        }
    }
    else
    {
        m_ui->editButton->setEnabled(false);
        m_ui->deleteButton->setEnabled(false);
        m_ui->descriptionLabel->clear();

        if (!m_bMultiple)
        {
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            setWindowTitle(tr("Smart Object State"));
        }
    }
}

void CSmartObjectStateDialog::OnInitDialog(const QString& soString, bool hasAdvanced)
{
    setWindowTitle(tr("Smart Object States"));

    if (m_bMultiple)
    {
        m_ui->listLabel->setText(tr("Choose Smart Object States:"));
    }
    else
    {
        m_ui->listLabel->setText(tr("Choose Smart Object State:"));
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));

    m_ui->advancedButton->setVisible(hasAdvanced);

    m_treeModel->setSourceModel(m_model);
    m_ui->statesTreeView->setModel(m_treeModel);

    connect(m_model, &QAbstractItemModel::dataChanged, this, &CSmartObjectStateDialog::UpdateIncludeExcludeEdits);
    connect(m_ui->statesTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSmartObjectStateDialog::OnTVSelChanged);
    connect(m_ui->statesTreeView, &QTreeView::doubleClicked, this, [=](const QModelIndex& i) { m_treeModel->setData(i, {}, Qt::CheckStateRole); });
    connect(m_ui->newButton, &QAbstractButton::clicked, this, &CSmartObjectStateDialog::OnNewBtn);
    connect(m_ui->editButton, &QAbstractButton::clicked, this, &CSmartObjectStateDialog::OnEditBtn);
    connect(m_ui->deleteButton, &QAbstractButton::clicked, this, &CSmartObjectStateDialog::OnDeleteBtn);
    connect(m_ui->refreshButton, &QAbstractButton::clicked, this, [ = ] { OnRefreshBtn(m_model->Included(), m_model->Excluded());
        });
    connect(m_ui->advancedButton, &QAbstractButton::clicked, this, [this] { done(ADVANCED_MODE_REQUESTED);
        });
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &CSmartObjectStateDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const QRegularExpression slicer("^(?<included>[^-]*)(-(?<excluded>.*))?$");
    auto match = slicer.match(soString);
    if (match.hasMatch())
    {
        QStringList included = match.captured("included").split(',', QString::SkipEmptyParts);
        QStringList excluded = match.captured("excluded").split(',', QString::SkipEmptyParts);
        OnRefreshBtn(included, excluded);
    }
}

void CSmartObjectStateDialog::UpdateIncludeExcludeEdits()
{
    m_ui->includeEdit->setText(m_model->Included().join(", "));
    m_ui->excludeEdit->setText(m_model->Excluded().join(", "));
}

#include <SmartObjectStateDialog.moc>
