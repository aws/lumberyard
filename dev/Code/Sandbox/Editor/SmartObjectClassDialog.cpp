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
#include "SmartObjectClassDialog.h"
#include <ui_SmartObjectClassDialog.h>
#include "AI/AIManager.h"

#include "Util/AbstractGroupProxyModel.h"

#include <QMessageBox>
#include <QShowEvent>
#include <QStyledItemDelegate>

// CSmartObjectClassDialog dialog

class SmartObjectItemDelegate
    : public QStyledItemDelegate
{
public:
    SmartObjectItemDelegate(QObject* parent = nullptr)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItemV4 opt(option);
        const bool hasCheckState = index.data(Qt::CheckStateRole).isValid();
        const bool isSelected = option.state & QStyle::State_Selected;
        const bool hasChildren = index.model()->hasChildren(index);
        opt.font.setBold((!hasCheckState && isSelected && !hasChildren) || index.data(Qt::CheckStateRole).toInt() == Qt::Checked);
        QStyledItemDelegate::paint(painter, opt, index);
    }

private:
};

class SmartObjectClassModel
    : public QAbstractListModel
{
public:
    SmartObjectClassModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
        m_checkedRows.clear();
        m_checkedRows.resize(rowCount());
    }

    void setCheckable(bool checkable)
    {
        m_checkable = checkable;
    }

    bool appendRow(const QString& itemName, const QString& description, const QString& location, const QString& templateName)
    {
        CSOLibrary::CClassData newData = { itemName };
        auto it = std::lower_bound(CSOLibrary::GetClasses().begin(), CSOLibrary::GetClasses().end(), newData,
                [&](const CSOLibrary::CClassData& lhs, const CSOLibrary::CClassData& rhs)
                {
                    return lhs.name < rhs.name;
                });
        const int row = it - CSOLibrary::GetClasses().begin();
        beginInsertRows(QModelIndex(), row, row);
        m_checkedRows.insert(row, Qt::Unchecked);
        CSOLibrary::AddClass(itemName.toUtf8().data(), description.toUtf8().data(), location.toUtf8().data(), templateName.toUtf8().data());
        endInsertRows();
        return true;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : CSOLibrary::GetClasses().size();
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QAbstractListModel::flags(index);
        f &= ~Qt::ItemIsEditable;
        if (m_checkable)
        {
            f |= Qt::ItemIsUserCheckable;
        }
        return f;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.row() >= rowCount(index.parent()) || !index.isValid())
        {
            return QVariant();
        }

        auto c = CSOLibrary::GetClasses().at(index.row());

        switch (role)
        {
        case Qt::DisplayRole:
            return c.location + QStringLiteral("/") + c.name;
        case Qt::CheckStateRole:
            return m_checkable ? m_checkedRows[index.row()] : QVariant();
        case Qt::UserRole:
            return QVariant::fromValue(c);
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
    {
        if (index.row() >= rowCount(index.parent()) || !index.isValid())
        {
            return false;
        }

        auto c = CSOLibrary::GetClasses().at(index.row());

        if (role == Qt::EditRole || role == Qt::DisplayRole)
        {
            QStringList parts = value.toString().split(QStringLiteral("/"), QString::SkipEmptyParts);
            if (parts.isEmpty())
            {
                return false;
            }
            c.name = parts.last();
            parts.pop_back();
            c.location = parts.join(QStringLiteral("/"));
            if (!CSOLibrary::StartEditing())
            {
                return false;
            }
            CSOLibrary::GetClasses()[index.row()] = c;
            CSOLibrary::InvalidateSOEntities();
            emit dataChanged(index, index);
            return true;
        }
        else if (role == Qt::CheckStateRole)
        {
            m_checkedRows[index.row()] = static_cast<Qt::CheckState>(value.toInt());
            emit dataChanged(index, index);
            return true;
        }
        else if (role == Qt::UserRole)
        {
            if (!CSOLibrary::StartEditing())
            {
                return false;
            }
            CSOLibrary::GetClasses()[index.row()] = value.value<CSOLibrary::CClassData>();
            CSOLibrary::InvalidateSOEntities();
            emit dataChanged(index, index);
            return true;
        }
        return false;
    }

private:
    QVector<Qt::CheckState> m_checkedRows;
    bool m_checkable;
};

class SmartObjectClassTreeModel
    : public AbstractGroupProxyModel
{
public:
    SmartObjectClassTreeModel(QObject* parent = nullptr)
        : AbstractGroupProxyModel(parent)
    {
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        QVariant d = AbstractGroupProxyModel::data(index, role);
        if (role == Qt::DisplayRole)
        {
            return d.toString().split(QStringLiteral("/")).last();
        }
        return d;
    }

    QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override
    {
        const QString path = sourceIndex.data().toString();
        QStringList parts = path.split(QStringLiteral("/"), QString::SkipEmptyParts);
        parts.pop_back();
        for (int i = 0; i < parts.count(); ++i)
        {
            parts[i] = QStringLiteral("/") + parts[i];
        }
        return parts;
    }
};

CSmartObjectClassDialog::CSmartObjectClassDialog(QWidget* pParent /*=NULL*/, bool multi /*=false*/)
    : QDialog(pParent)
    , m_model(new SmartObjectClassModel(this))
    , m_ui(new Ui::SmartObjectClassDialog)
{
    m_ui->setupUi(this);
    m_ui->m_widgetMulti->setVisible(multi);
    setFixedSize(width(), sizeHint().height());

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_bMultiple = multi;
    m_model->setCheckable(multi);

    auto proxy = new SmartObjectClassTreeModel(this);
    proxy->setSourceModel(m_model);
    m_ui->m_TreeCtrl->setModel(proxy);
    m_ui->m_TreeCtrl->setItemDelegate(new SmartObjectItemDelegate(this));

    connect(m_model, &QAbstractItemModel::dataChanged, this, &CSmartObjectClassDialog::UpdateListCurrentClasses);

    OnInitDialog();

    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Close);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(m_ui->m_btnNew, &QPushButton::clicked, this, &CSmartObjectClassDialog::OnNewBtn);
    connect(m_ui->m_btnEdit, &QPushButton::clicked, this, &CSmartObjectClassDialog::OnEditBtn);
    connect(m_ui->m_btnRefresh, &QPushButton::clicked, this, &CSmartObjectClassDialog::OnRefreshBtn);
    //ON_NOTIFY(TVN_KEYDOWN, IDC_TREE, OnTVKeyDown)
    connect(m_ui->m_TreeCtrl, &QAbstractItemView::doubleClicked, this, &CSmartObjectClassDialog::OnTVDblClk);
    connect(m_ui->m_TreeCtrl->selectionModel(), &QItemSelectionModel::currentChanged, this, &CSmartObjectClassDialog::OnTVSelChanged);
}

CSmartObjectClassDialog::~CSmartObjectClassDialog()
{
}

void CSmartObjectClassDialog::EnableOK()
{
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

// CSmartObjectClassDialog message handlers

void CSmartObjectClassDialog::OnTVDblClk(const QModelIndex& item)
{
    if (item.isValid() && !m_ui->m_TreeCtrl->model()->hasChildren(item))
    {
        if (!m_bMultiple)
        {
            accept();
        }
        else
        {
            Qt::CheckState check = item.data(Qt::CheckStateRole).toInt() == Qt::Checked ? Qt::Unchecked : Qt::Checked;
            m_ui->m_TreeCtrl->model()->setData(item, check, Qt::CheckStateRole);
        }
    }
}

void CSmartObjectClassDialog::OnTVSelChanged()
{
    const QModelIndex item = m_ui->m_TreeCtrl->currentIndex();
    if (!item.isValid() || m_ui->m_TreeCtrl->model()->hasChildren(item))
    {
        m_ui->m_description->clear();
        m_ui->m_btnEdit->setEnabled(false);
    }
    else
    {
        CSOLibrary::VectorClassData::iterator it = CSOLibrary::FindClass(item.data().toString().toUtf8().data());
        m_ui->m_description->setPlainText(it->description);
        m_ui->m_btnEdit->setEnabled(true);
    }

    if (m_bMultiple || !item.isValid())
    {
        return;
    }
    if (!m_ui->m_TreeCtrl->model()->hasChildren(item))
    {
        EnableOK();


        m_sSOClass = item.data().toString();

        setWindowTitle(tr("Smart Object Class: %1").arg(m_sSOClass));
    }
}

void CSmartObjectClassDialog::OnInitDialog()
{
    if (m_bMultiple)
    {
        setWindowTitle(tr("Smart Object Classes"));
        m_ui->m_labelListCaption->setText(tr("&Choose Smart Object Classes:"));
        //. m_wndClassList.ModifyStyle( 0, LBS_MULTIPLESEL );
    }
    else
    {
        setWindowTitle(tr("Smart Object Class"));
        m_ui->m_labelListCaption->setText(tr("&Choose Smart Object Class:"));
        //. m_wndClassList.ModifyStyle( LBS_MULTIPLESEL, 0 );
    }

    //m_ui->m_btnNew->setEnabled(false);
    m_ui->m_btnEdit->setEnabled(false);
    m_ui->m_btnDelete->setEnabled(false);
    m_ui->m_btnRefresh->setEnabled(false);

    //OnRefreshBtn();
}

void CSmartObjectClassDialog::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    OnRefreshBtn();
}

void CSmartObjectClassDialog::OnRefreshBtn()
{
    if (m_bMultiple)
    {
        static QRegularExpression exp(QStringLiteral("[ !\"#$%&'()*+,\\-./:;<=>?@\\[\\\\\\]^`{|}~]"));
        for (auto token : m_sSOClass.split(exp, QString::SkipEmptyParts))
        {
            if (CItemDescriptionDlg::ValidateItem(token))
            {
                if (CSOLibrary::FindClass(token.toUtf8().data()) == CSOLibrary::GetClasses().end())
                {
                    CSOLibrary::AddClass(token.toUtf8().data(), "", "", "");
                }
                QModelIndexList indexes = m_ui->m_TreeCtrl->model()->match(m_ui->m_TreeCtrl->model()->index(0, 0), Qt::DisplayRole, token, 1, Qt::MatchExactly | Qt::MatchRecursive);
                if (!indexes.isEmpty())
                {
                    m_ui->m_TreeCtrl->model()->setData(indexes.first(), Qt::Checked, Qt::CheckStateRole);
                }
            }
        }
    }
    else
    {
        QModelIndexList indexes = m_ui->m_TreeCtrl->model()->match(m_ui->m_TreeCtrl->model()->index(0, 0), Qt::DisplayRole, m_sSOClass, 1, Qt::MatchExactly | Qt::MatchRecursive);
        if (!indexes.isEmpty())
        {
            m_ui->m_TreeCtrl->setCurrentIndex(indexes.first());
        }
    }
}

void CSmartObjectClassDialog::UpdateListCurrentClasses()
{
    m_sSOClass.clear();
    QStringList classes;
    for (auto index : m_ui->m_TreeCtrl->model()->match(m_ui->m_TreeCtrl->model()->index(0, 0), Qt::CheckStateRole, Qt::Checked, -1, Qt::MatchExactly | Qt::MatchRecursive))
    {
        classes.push_back(index.data().toString());
    }
    m_sSOClass = classes.join(QStringLiteral(", "));
    m_ui->m_selectionList->setPlainText(m_sSOClass);
    EnableOK();
}

void CSmartObjectClassDialog::OnEditBtn()
{
    const QModelIndex item = m_ui->m_TreeCtrl->currentIndex();
    if (!item.isValid() || m_ui->m_TreeCtrl->model()->hasChildren(item))
    {
        return;
    }

    QString name = item.data().toString();
    CSOLibrary::VectorClassData::iterator it = CSOLibrary::FindClass(name.toUtf8().data());
    assert(it != CSOLibrary::GetClasses().end());

    CItemDescriptionDlg dlg(this, false, false, true, true);
    dlg.setWindowTitle(QObject::tr("Edit Smart Object Class"));
    dlg.setItemCaption(QObject::tr("Class name:"));
    dlg.setItem(name);
    dlg.setDescription(it->description);
    dlg.setLocation(it->location);
    dlg.setTemplateName(it->templateName);
    if (dlg.exec())
    {
        auto data = *it;
        data.description = dlg.description();
        data.location = dlg.location();
        data.templateName = dlg.templateName();
        m_ui->m_TreeCtrl->model()->setData(item, QVariant::fromValue(data), Qt::UserRole);
        OnTVSelChanged();
        const QModelIndexList item = m_ui->m_TreeCtrl->model()->match(m_ui->m_TreeCtrl->model()->index(0, 0), Qt::DisplayRole, name, 1, Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
        assert(!item.isEmpty());
        m_ui->m_TreeCtrl->scrollTo(item.first());
        m_ui->m_TreeCtrl->setCurrentIndex(item.first());
    }
}

void CSmartObjectClassDialog::OnNewBtn()
{
    CItemDescriptionDlg dlg(this, true, false, true, true);
    dlg.setWindowTitle(tr("New Class"));
    dlg.setItemCaption(tr("Class name:"));
    QStringList location;
    QModelIndex current = m_ui->m_TreeCtrl->currentIndex();
    while (current.isValid())
    {
        if (m_ui->m_TreeCtrl->model()->hasChildren(current))
        {
            location.push_front(current.data().toString());
        }
        current = m_ui->m_TreeCtrl->model()->parent(current);
    }
    dlg.setLocation(location.join('/'));
    if (dlg.exec())
    {
        QString itemName = dlg.item();

        if (!itemName.isEmpty())
        {
            const QModelIndexList item = m_ui->m_TreeCtrl->model()->match(m_ui->m_TreeCtrl->model()->index(0, 0), Qt::DisplayRole, itemName, 1, Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
            if (!item.isEmpty())
            {
                m_ui->m_TreeCtrl->scrollTo(item.first());
                m_ui->m_TreeCtrl->setCurrentIndex(item.first());
                QMessageBox::information(this, QString(), tr("Entered class name already exists...\n\nA new class with the same name can not be created!"));

                EnableOK();
            }
            else
            {
                QString location = dlg.location();

                if (!location.isEmpty())
                {
                    m_model->appendRow(itemName, dlg.description(), location, dlg.templateName());
                    const QModelIndexList item = m_ui->m_TreeCtrl->model()->match(m_ui->m_TreeCtrl->model()->index(0, 0), Qt::DisplayRole, itemName, 1, Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
                    assert(!item.isEmpty());
                    m_ui->m_TreeCtrl->scrollTo(item.first());
                    m_ui->m_TreeCtrl->setCurrentIndex(item.first());
                    EnableOK();
                }
                else
                {
                    QMessageBox::information(this, QString(), tr("Invalid location given.\nCould not create class."));
                }
            }            
        }
        else
        {
            QMessageBox::information(this, QString(), tr("Invalid name given.\nCould not create class."));
        }
    }
}

#include <SmartObjectClassDialog.moc>
