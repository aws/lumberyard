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


#include "StdAfx.h"
#include "ObjectSelectorTableView.h"
#include "ObjectSelectorModel.h"

#include <QtUtil.h>
#include <QtUtilWin.h>
#include <StringUtils.h>

#include <QMenu>
#include <QHeaderView>
#include <QEvent>
#include <QContextMenuEvent>
#include <QItemSelectionModel>
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QString>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QPainter>

enum
{
    IndentWidth = 20 // pixels
};

class IndentedIconDelegate
    : public QStyledItemDelegate
{
public:

    explicit IndentedIconDelegate(ObjectSelectorTableView* parent)
        : QStyledItemDelegate(parent)
        , m_tableView(parent)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (!index.isValid())
        {
            return;
        }

        QStyleOptionViewItem adjustedOption = option;
        if (m_tableView->IsTreeMode())
        {
            const int indentLevel = index.data(ObjectSelectorModel::IdentationRole).toInt();
            adjustedOption.rect.adjust(indentLevel * IndentWidth, 0, 0, 0);
        }
        QStyledItemDelegate::paint(painter, adjustedOption, index);
    }

private:
    const ObjectSelectorTableView* m_tableView;
};

ObjectSelectorTableView::ObjectSelectorTableView(QWidget* parent)
    : QTableView(parent)
    , m_headerPopup(new QMenu(this))
    , m_model(nullptr)
{
    connect(m_headerPopup, &QMenu::triggered, this, &ObjectSelectorTableView::ToggleColumnVisibility);

    horizontalHeader()->installEventFilter(this);
    horizontalHeader()->setDragEnabled(true);
    horizontalHeader()->setSectionsMovable(true);

    verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(25);

    qRegisterMetaType<QVector<int> >("QVector<int>");
    qRegisterMetaTypeStreamOperators<QVector<int> >("QVector<int>");

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSortingEnabled(true);
    setItemDelegateForColumn(ObjectSelectorModel::NameColumn, new IndentedIconDelegate(this));
}

ObjectSelectorTableView::~ObjectSelectorTableView()
{
}

void ObjectSelectorTableView::ToggleColumnVisibility(QAction* action)
{
    if (action->isChecked())
    {
        showColumn(action->data().toInt());
    }
    else
    {
        hideColumn(action->data().toInt());
    }

    SaveColumnsVisibility();
}

void ObjectSelectorTableView::setModel(QAbstractItemModel* model)
{
    m_model = qobject_cast<ObjectSelectorModel*>(model);
    QTableView::setModel(model);
    InitializeColumnWidths();
    CreateHeaderPopup();
}

void ObjectSelectorTableView::CreateHeaderPopup()
{
    m_settings.beginGroup("ObjectSelector");
    QVector<int> hiddenColumns = m_settings.value("hiddenColumns").value<QVector<int> >();
    m_settings.endGroup();

    const QStringList colNames = ObjectSelectorModel::ColumnNames();
    for (int c = 0; c < ObjectSelectorModel::NumberOfColumns; ++c)
    {
        // skip if it's the Name column. There is no reason for users 
        // to hide the Name column in Object Selector.
        if (c != ObjectSelectorModel::NameColumn)
        {
            QAction* action = m_headerPopup->addAction(colNames[c]);
            action->setData(QVariant(c));
            const bool isHidden = hiddenColumns.contains(c);
            action->setCheckable(true);
            action->setChecked(!isHidden);
            ToggleColumnVisibility(action);
        }
    }
}

void ObjectSelectorTableView::SaveColumnsVisibility()
{
    m_settings.beginGroup("ObjectSelector");

    QVector<int> hiddenColumns;
    auto actions = m_headerPopup->actions();

    for (int i = 0; i < actions.size(); ++i)
    {
        if (!actions.at(i)->isChecked())
        {
            hiddenColumns.push_back(actions.at(i)->data().toInt());
        }
    }

    m_settings.setValue("hiddenColumns", QVariant::fromValue(hiddenColumns));
    m_settings.endGroup();
    m_settings.sync();
}

void ObjectSelectorTableView::InitializeColumnWidths()
{
    static const int widths[] = { 100, 60, 60, 100, 80, 80, 80, 80, 120, 250, 80, 80, 50, 80 };
    static_assert(sizeof(widths) / sizeof(int) == ObjectSelectorModel::NumberOfColumns, "Wrong array size");

    for (int c = 0; c < ObjectSelectorModel::NumberOfColumns; ++c)
    {
        setColumnWidth(c, widths[c]);
    }
}

bool ObjectSelectorTableView::eventFilter(QObject* watched, QEvent* ev)
{
    if (ev->type() == QEvent::ContextMenu)
    {
        QContextMenuEvent* e = static_cast<QContextMenuEvent*>(ev);
        m_headerPopup->popup(mapToGlobal(e->pos()));
        return true;
    }
    return false;
}

void ObjectSelectorTableView::EnableTreeMode(bool enable)
{
    m_treeMode = enable;
}

bool ObjectSelectorTableView::IsTreeMode() const
{
    return m_treeMode;
}

#include <ObjectSelectorTableView.moc>
