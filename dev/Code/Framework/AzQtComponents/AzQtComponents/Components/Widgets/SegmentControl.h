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

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QFrame>

class QBoxLayout;
class QStackedLayout;

namespace AzQtComponents
{
    class SegmentBar;

    class AZ_QT_COMPONENTS_API SegmentControl : public QFrame
    {
        Q_OBJECT //AUTOMOC

        Q_PROPERTY(Qt::Orientation tabOrientation READ tabOrientation WRITE setTabOrientation)
        Q_PROPERTY(SegmentControl::TabPosition tabPosition READ tabPosition WRITE setTabPosition)
        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
        Q_PROPERTY(int count READ count)
        Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)

    public:
        enum TabPosition { North,
                           South,
                           West,
                           East };
        Q_ENUM(TabPosition)

        explicit SegmentControl(SegmentControl::TabPosition position, QWidget* parent = nullptr);
        explicit SegmentControl(SegmentBar* bar, SegmentControl::TabPosition position, QWidget* parent = nullptr);
        explicit SegmentControl(QWidget* parent = nullptr);

        Qt::Orientation tabOrientation() const;
        void setTabOrientation(Qt::Orientation orientation);

        SegmentControl::TabPosition tabPosition() const;
        void setTabPosition(SegmentControl::TabPosition position);

        QSize iconSize() const;
        void setIconSize(const QSize& size);

        int addTab(QWidget* widget, const QString& text);
        int addTab(QWidget* widget, const QIcon& icon, const QString& text);

        int insertTab(int index, QWidget* widget, const QString& text);
        int insertTab(int index, QWidget* widget, const QIcon& icon, const QString& text);

        void removeTab(int index);
        void moveTab(int from, int to);

        bool isTabEnabled(int index) const;
        void setTabEnabled(int index, bool enabled);

        QString tabText(int index) const;
        void setTabText(int index, const QString& text);

        QIcon tabIcon(int index) const;
        void setTabIcon(int index, const QIcon& icon);

#ifndef QT_NO_TOOLTIP
        void setTabToolTip(int index, const QString& tip);
        QString tabToolTip(int index) const;
#endif

#ifndef QT_NO_WHATSTHIS
        void setTabWhatsThis(int index, const QString& text);
        QString tabWhatsThis(int index) const;
#endif

        int currentIndex() const;
        QWidget* currentWidget() const;
        QWidget* widget(int index) const;
        int indexOf(QWidget* widget) const;
        int count() const;

        void clear();

        SegmentBar* tabBar() const;

    public Q_SLOTS:
        void setCurrentIndex(int index);
        void setCurrentWidget(QWidget* widget);

    Q_SIGNALS:
        void currentChanged(int index);
        void tabBarClicked(int index);

    private:
        QBoxLayout* m_layout;
        SegmentBar* m_segmentBar;
        QStackedLayout* m_widgets;

        void initialize(Qt::Orientation tabOrientation, SegmentControl::TabPosition position);
        void initialize(SegmentBar* bar, SegmentControl::TabPosition position);
    };
}
