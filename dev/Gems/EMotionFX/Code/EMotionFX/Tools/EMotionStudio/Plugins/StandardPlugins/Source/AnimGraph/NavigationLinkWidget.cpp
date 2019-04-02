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

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphItemDelegate.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationLinkWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/RoleFilterProxyModel.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QHBoxLayout>
#include <QIcon>
#include <QStylePainter>

namespace EMStudio
{
    NavigationItemWidget::NavigationItemWidget(const QModelIndex& modelIndex, QWidget* parent)
        : QPushButton(parent)
        , m_modelIndex(modelIndex)
    {
        
        m_itemDelegate = new AnimGraphItemDelegate(parent);
        m_itemDelegate->setModelData(this, const_cast<QAbstractItemModel*>(modelIndex.model()), modelIndex);

        setStyleSheet("border: none; font-size: 11px; color: #e9e9e9;");

        connect(m_modelIndex.model(), &QAbstractItemModel::dataChanged, this, &NavigationItemWidget::OnDataChanged);
    }

    void NavigationItemWidget::OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        const QItemSelectionRange range(topLeft, bottomRight);
        if (range.contains(m_modelIndex))
        {
            setText(m_modelIndex.data(Qt::DisplayRole).toString());
        }
    }

    void NavigationItemWidget::enterEvent(QEvent *event)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void NavigationItemWidget::leaveEvent(QEvent *event)
    {
        setCursor(Qt::ArrowCursor);
    }

    void NavigationItemWidget::paintEvent(QPaintEvent* event)
    {
        QStylePainter painter(this);
        QStyleOptionViewItem options;
        options.initFrom(this);
        options.displayAlignment = Qt::AlignCenter;
        options.decorationAlignment = Qt::AlignCenter;
        m_itemDelegate->paint(&painter, options, m_modelIndex);
    }

    QSize NavigationItemWidget::sizeHint() const
    {
        QStyleOptionViewItem options;
        options.initFrom(this);
        options.displayAlignment = Qt::AlignCenter;
        options.decorationAlignment = Qt::AlignCenter;
        return m_itemDelegate->sizeHint(options, m_modelIndex);
    }

    NavigationLinkWidget::NavigationLinkWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
        , m_plugin(plugin)
    {
        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(2, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        mainLayout->setAlignment(Qt::AlignLeft);
        setLayout(mainLayout);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        setMaximumHeight(28);
        setFocusPolicy(Qt::ClickFocus);

        m_navigationPathImg = MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/NavPath.png").pixmap(QSize(16, 16));

        m_roleFilterProxyModel = new RoleFilterProxyModel(m_plugin->GetAnimGraphModel(), this);
        m_roleFilterProxyModel->setFilteredRoles({ Qt::DecorationRole });

        connect(&m_plugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &NavigationLinkWidget::OnFocusChanged);
    }


    NavigationLinkWidget::~NavigationLinkWidget()
    {
    }


    void NavigationLinkWidget::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        if (newFocusParent != oldFocusParent)
        {
            // TODO: we could do better and remove from the right, if we hit the newFocusParent then we can stop and not recreate the whole list
            // However, the arrow in between makes it tricky 

            // Remove all the child widgets
            QLayoutItem* item = layout()->takeAt(0);
            while (item)
            {
                delete item->widget();
                delete item;
                item = layout()->takeAt(0);
            }

            // Add all the hierarchy 
            if (newFocusParent.isValid())
            {
                AddToNavigation(newFocusParent, true);
            }
        }
    }

    void NavigationLinkWidget::OnItemClicked(const QModelIndex& newModelIndex)
    {
        m_plugin->GetAnimGraphModel().Focus(newModelIndex);
    }
    

    void NavigationLinkWidget::AddToNavigation(const QModelIndex& modelIndex, bool isLastWidget)
    {
        QModelIndex parent = modelIndex.parent();
        if (parent.isValid())
        {
            AddToNavigation(parent);
        }

        QModelIndex proxyItem = m_roleFilterProxyModel->mapFromSource(modelIndex);

        NavigationItemWidget* item = new NavigationItemWidget(proxyItem, this);
        connect(item, &QPushButton::clicked, [this, modelIndex](bool) { OnItemClicked(modelIndex); });
        layout()->addWidget(item);

        if (!isLastWidget)
        {
            QLabel* spacer = new QLabel("", this);
            spacer->setFixedSize(QSize(16, 16));
            spacer->setPixmap(m_navigationPathImg);
            layout()->addWidget(spacer);
        }
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationLinkWidget.moc>
