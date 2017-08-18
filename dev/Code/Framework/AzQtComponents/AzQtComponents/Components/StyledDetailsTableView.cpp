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

#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>

#include <QStyledItemDelegate>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QProxyStyle>
#include <QPainter>
#include <QApplication>
#include <QStyleFactory>
#include <QDebug>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMimeData>
#include <QClipboard>
#include <QMenu>

namespace AzQtComponents
{
    static const int StyledTreeDetailsPadding = 20;

    struct StyledDetailsTableDetailsInfo
    {
        StyledDetailsTableDetailsInfo(const QStyleOptionViewItem& opt, const QModelIndex& index)
        {
            if (auto view = qobject_cast<const QTableView*>(opt.widget))
            {
                auto firstCol = index.sibling(index.row(), 0);
                const auto data = firstCol.data(StyledDetailsTableModel::Details);
                if (data.isValid())
                {
                    option = opt;
                    option.rect.setLeft(StyledTreeDetailsPadding);
                    option.rect.setRight(view->horizontalHeader()->length() - StyledTreeDetailsPadding);

                    // for some strange reason, and not well-documented, you have to use QChar::LineSeparator
                    // so that the view actually renders newlines.
                    option.text = data.toString().replace(QChar('\n'), QChar::LineSeparator);

                    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
                    option.state &= ~(QStyle::State_Selected);
                    option.icon = {};
                    sizeHint = view->style()->sizeFromContents(QStyle::CT_ItemViewItem, &option, {}, view);
                    sizeHint.rheight() += StyledTreeDetailsPadding * 2;
                    option.rect.setTop(option.rect.bottom() - sizeHint.height());
                }
            }
        }

        QSize sizeHint = { 0, 0 };
        QStyleOptionViewItem option;
    };

    class StyledTableStyle : public QProxyStyle
    {
    public:
        explicit StyledTableStyle(QObject* parent)
            : QProxyStyle()
        {
            setParent(parent);
        }

        QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override
        {
            auto rect = QProxyStyle::subElementRect(element, option, widget);
            switch (element)
            {
                case SE_ItemViewItemText:
                case SE_ItemViewItemCheckIndicator:
                case SE_ItemViewItemDecoration:
                    if (option->state.testFlag(State_Selected))
                    {
                        auto vOpt = static_cast<const QStyleOptionViewItem*>(option);
                        const auto offset = StyledDetailsTableDetailsInfo(*vOpt, vOpt->index).sizeHint.height();
                        if (element == SE_ItemViewItemText)
                        {
                            rect.setBottom(rect.bottom() - offset);
                        }
                        else
                        {
                            rect.setTop(rect.top() - offset);
                        }
                    }
                    if (element == SE_ItemViewItemDecoration)
                    {
                        rect.moveLeft(rect.left() + StyledTreeDetailsPadding);
                    }
                    break;
                default:
                    break;
            }
            return rect;
        }

        void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                           QPainter* painter, const QWidget* widget) const override
        {
            switch (element)
            {
            case PE_PanelItemViewItem:
            case PE_PanelItemViewRow:
            {
                const auto cg = !option->state.testFlag(State_Enabled) ? QPalette::Disabled
                              : option->state.testFlag(State_Active) ? QPalette::Normal
                              : QPalette::Inactive;

                if (option->state.testFlag(State_Selected))
                {
                    painter->fillRect(option->rect, option->palette.brush(cg, QPalette::Highlight));
                }
                else if (auto vOpt = qstyleoption_cast<const QStyleOptionViewItem*>(option))
                {
                    if (vOpt->features.testFlag(QStyleOptionViewItem::Alternate))
                    {
                        painter->fillRect(option->rect, option->palette.brush(cg, QPalette::AlternateBase));
                    }
                }
                break;
            }
            default:
                QProxyStyle::drawPrimitive(element, option, painter, widget);
                break;
            }
        }
    };

    class StyledDetailsTableDelegate : public QStyledItemDelegate
    {
    public:
        StyledDetailsTableDelegate(QObject *parent)
            : QStyledItemDelegate(parent)
        {
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override
        {
            auto copy = opt;
            initStyleOption(&copy, index);
            DrawItemViewItem(painter, copy);

            if (!m_detailsOptions.contains(index.row()) &&
                    (opt.state.testFlag(QStyle::State_Selected) ||
                     index.data(StyledDetailsTableModel::HasOnlyDetails).toBool()))
            {
                m_detailsOptions.insert(index.row(), StyledDetailsTableDetailsInfo(copy, index).option);
            }
        }

        void DrawDetails(QWidget* viewport) const
        {
            QPainter painter(viewport);
            for (const auto &opt: m_detailsOptions)
            {
                DrawItemViewItem(&painter, opt);
            }
            m_detailsOptions.clear();
        }

        void DrawItemViewItem(QPainter* painter, const QStyleOptionViewItem& option) const
        {
            const auto widget = option.widget;
            const auto style = widget ? widget->style() : qApp->style();
            
            QStyleOptionViewItem copy = option;
            copy.state &= ~(QStyle::State_Selected);
            style->drawControl(QStyle::CE_ItemViewItem, &copy, painter, widget);
        }

        QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override
        {
            QStyleOptionViewItem copy = opt;
            initStyleOption(&copy, index);
            const auto widget = opt.widget;
            const auto style = widget ? widget->style() : qApp->style();
            auto size = style->sizeFromContents(QStyle::CT_ItemViewItem, &copy, QSize(), widget);

            // option not fully initialized (no State_Selected), use the widget
            const auto view = qobject_cast<const QAbstractItemView*>(widget);
            if (index.data(StyledDetailsTableModel::HasOnlyDetails).toBool())
            {
                size.rheight() = StyledDetailsTableDetailsInfo(opt, index).sizeHint.height();
            }
            else if (view && view->selectionModel()->isSelected(index))
            {
                size.rheight() += StyledDetailsTableDetailsInfo(opt, index).sizeHint.height();
            }

            if (copy.features.testFlag(QStyleOptionViewItem::HasDecoration))
            {
                size.rwidth() += StyledTreeDetailsPadding * 2;
            }

            return size;
        }

    protected:

        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
        {
            QStyledItemDelegate::initStyleOption(option, index);
            // Don't show focused state
            option->state &= ~(QStyle::State_HasFocus);
        }

    private:
        mutable QHash<int, QStyleOptionViewItem> m_detailsOptions;
    };

    StyledDetailsTableView::StyledDetailsTableView(QWidget* parent)
        : QTableView(parent)
    {
        setStyle(new StyledTableStyle(qApp));
        setAlternatingRowColors(true);
        setSelectionMode(SingleSelection);
        setSelectionBehavior(SelectRows);
        setShowGrid(false);
        setItemDelegate(new StyledDetailsTableDelegate(this));
        setSortingEnabled(true);

        verticalHeader()->hide();

        auto font = horizontalHeader()->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        horizontalHeader()->setFont(font);
        horizontalHeader()->setStyle(qApp->style());
        horizontalHeader()->setHighlightSections(false);
        horizontalHeader()->setStretchLastSection(true);
        connect(horizontalHeader(), &QHeaderView::geometriesChanged, this, &StyledDetailsTableView::resizeRowsToContents);
        connect(horizontalHeader(), &QHeaderView::sectionResized, this, &StyledDetailsTableView::resizeRowsToContents);

        setContextMenuPolicy(Qt::ActionsContextMenu);

        QAction* copyAction = new QAction(tr("Copy"), this);
        copyAction->setShortcut(QKeySequence::Copy);
        connect(copyAction, &QAction::triggered, this, &StyledDetailsTableView::copySelectionToClipboard);
        addAction(copyAction);
    }

    void StyledDetailsTableView::setModel(QAbstractItemModel* model)
    {
        if (selectionModel())
        {
            selectionModel()->disconnect(this);
        }

        if (model)
        {
            model->disconnect(this);
        }

        QTableView::setModel(model);

        if (model)
        {
            connect(model, &QAbstractItemModel::layoutChanged, this, &StyledDetailsTableView::resizeRowsToContents);
            connect(model, &QAbstractItemModel::rowsAboutToBeInserted, this, [this]
            {
                m_scrollOnInsert = !selectionModel()->hasSelection()
                    && (verticalScrollBar()->value() == verticalScrollBar()->maximum());
            });
            connect(model, &QAbstractItemModel::rowsInserted, this, [this]
            {
                if (m_scrollOnInsert)
                {
                    m_scrollOnInsert = false;
                    scrollToBottom();
                }
            });
            connect(model, &QAbstractItemModel::dataChanged, this,
                [this](const QModelIndex&, const QModelIndex&, const QVector<int>& roles)
            {
                if (roles.contains(StyledDetailsTableModel::Details))
                {
                    updateItemSelection(selectionModel()->selection());
                }
            });
        }

        if (selectionModel())
        {
            connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
                    [this](const QItemSelection& sel, const QItemSelection& desel)
            {
                updateItemSelection(desel);
                updateItemSelection(sel);
            });
        }
    }

    void StyledDetailsTableView::paintEvent(QPaintEvent* ev)
    {
        auto delegate = static_cast<StyledDetailsTableDelegate*>(itemDelegate());
        QTableView::paintEvent(ev);
        delegate->DrawDetails(viewport());
    }

    void StyledDetailsTableView::keyPressEvent(QKeyEvent* ev)
    {
        QTableView::keyPressEvent(ev);
    }

    QItemSelectionModel::SelectionFlags StyledDetailsTableView::selectionCommand(
        const QModelIndex& index, const QEvent* event) const
    {
        auto base = QTableView::selectionCommand(index, event);
        if (!selectionModel()->isSelected(index) || event->type() == QEvent::MouseMove ||
            !base.testFlag(QItemSelectionModel::ClearAndSelect))
        {
            return base;
        }
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto mEv = static_cast<const QMouseEvent*>(event);
            if (mEv->button() != Qt::LeftButton)
            {
                return base;
            }
        }
        // Toggle selection off during selection event if already selected
        return QItemSelectionModel::Rows | QItemSelectionModel::Deselect;
    }

    void StyledDetailsTableView::copySelectionToClipboard()
    {
        const auto selection = selectionModel()->selection();
        if (selection.isEmpty())
        {
            return;
        }


        auto index = selection.first().topLeft();

        const QString details = index.data(StyledDetailsTableModel::Details).toString();
        QStringList cells;
        if (!index.data(StyledDetailsTableModel::HasOnlyDetails).toBool())
        {
            while (index.isValid())
            {
                cells.append(index.data(Qt::DisplayRole).toString());
                index = index.sibling(index.row(), index.column() + 1);
            }
        }

        auto clipboard = qApp->clipboard();
        auto data = new QMimeData();
        {
            const static auto textFormat = QStringLiteral("%1\n%2");
            data->setText(textFormat.arg(cells.join(QChar::fromLatin1('\t')), details).trimmed());
        }
        {
            const static auto htmlFormat = QStringLiteral("<table><tr>%1</tr><tr colspan=%2>%3</tr></table>");
            const static auto htmlCellFormat = QStringLiteral("<td>%1</td>");
            const static auto cellsToHtml = [](const QStringList cells)
            {
                QString row;
                for (const auto &cell: cells)
                {
                    row += htmlCellFormat.arg(cell);
                }
                return row;
            };
            data->setHtml(htmlFormat.arg(cellsToHtml(cells),
                                         QString::number(model()->columnCount()),
                                         htmlCellFormat.arg(details)));
        }
        clipboard->setMimeData(data);
    }

    void StyledDetailsTableView::updateItemSelection(const QItemSelection& selection)
    {
        for (const auto& range : selection)
        {
            for (int i = range.top(); i <= range.bottom(); ++i)
            {
                resizeRowToContents(i);
            }
        }
    }

#include <Components/StyledDetailsTableView.moc>
} // namespace AzQtComponents
