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
class QButtonGroup;
class QAbstractButton;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API SegmentBar : public QFrame
    {
        Q_OBJECT //AUTOMOC

        Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
        Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
        Q_PROPERTY(int count READ count)
        Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)

    public:
        explicit SegmentBar(Qt::Orientation orientation, QWidget* parent = nullptr);
        explicit SegmentBar(QWidget* parent = nullptr);

        Qt::Orientation orientation() const;
        void setOrientation(Qt::Orientation orientation);

        int addTab(const QString& text);
        int addTab(const QIcon& icon, const QString& text);

        int insertTab(int index, const QString& text);
        int insertTab(int index, const QIcon& icon, const QString& text);

        void removeTab(int index);
        void moveTab(int from, int to);

        bool isTabEnabled(int index) const;
        void setTabEnabled(int index, bool enabled);

        QString tabText(int index) const;
        void setTabText(int index, const QString& text);

        QColor tabTextColor(int index) const;
        void setTabTextColor(int index, const QColor& color);

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

        QSize iconSize() const;
        void setIconSize(const QSize& size);

        int currentIndex() const;
        int count() const;

    public Q_SLOTS:
        void setCurrentIndex(int index);

    Q_SIGNALS:
        void currentChanged(int index);
        void tabBarClicked(int index);

    protected:
        virtual QAbstractButton* createButton(const QIcon& icon, const QString& text);
        QAbstractButton* button(int index) const;
        void updateTabs();

    private:
        QBoxLayout* m_alignmentLayout;
        QBoxLayout* m_buttonsLayout;
        QButtonGroup* m_buttons;
        QSize m_iconSize;
        int m_minimumTabSize;

        void initialize(Qt::Orientation orientation);
    };
}
