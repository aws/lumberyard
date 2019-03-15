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

#pragma once

#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_FORWARD_DECLARE_CLASS(QPixmap)

namespace EMStudio
{
    class AnimGraphPlugin;
    class AnimGraphItemDelegate;
    class RoleFilterProxyModel;

    class NavigationItemWidget
        : public QPushButton
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigationItemWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        NavigationItemWidget(const QModelIndex& modelIndex, QWidget* parent = nullptr);

    protected:
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        QSize sizeHint() const override;

    private slots:
        void OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

    private:
        QPersistentModelIndex  m_modelIndex;
        AnimGraphItemDelegate* m_itemDelegate;
    };

    class NavigationLinkWidget
        : public QWidget
    {
        MCORE_MEMORYOBJECTCATEGORY(NavigationLinkWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        NavigationLinkWidget(AnimGraphPlugin* plugin, QWidget* parent);
        ~NavigationLinkWidget();

    private slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);
        void OnItemClicked(const QModelIndex& newModelIndex);

    private:
        void AddToNavigation(const QModelIndex& modelIndex, bool isLastWidget = false);

        AnimGraphPlugin* m_plugin;
        RoleFilterProxyModel* m_roleFilterProxyModel;
        QPixmap m_navigationPathImg;
    };

} // namespace EMStudio
