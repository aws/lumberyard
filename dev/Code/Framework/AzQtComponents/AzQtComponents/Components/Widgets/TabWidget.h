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

#include <QAction>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QStyle>
#include <QStyledItemDelegate>

class QFrame;
class QSettings;
class QStyleOption;
class QToolButton;

namespace AzQtComponents
{
    class Style;
    class TabBar;
    class TabWidgetActionToolBar;
    class TabWidgetActionToolBarContainer;

    /**
     * Class to provide extra functionality for working with QTabWidget and QTabBar
     * objects.
     *
     * The TabBar class provides the ability to show a custom tear icon on moveable tabs,
     * when an tab is being hovered or clicked.
     *
     * The TabWidget class is just a wrapper class for QTabWidget using an instance of
     * TabBar as a tab bar.
     *
     * TabWidget and TabBar controls are styled in TabWidget.qss and configured in
     * TabWidget.ini
     */
    class AZ_QT_COMPONENTS_API TabWidget
        : public QTabWidget
    {
        Q_OBJECT
        Q_PROPERTY(bool actionToolBarVisible READ isActionToolBarVisible WRITE setActionToolBarVisible)

    public:
        struct Config
        {
            QPixmap tearIcon;
            int tearIconLeftPadding;
            int tabHeight;
            int minimumTabWidth;
            int closeButtonSize;
            int textRightPadding;
            int closeButtonRightPadding;
            int closeButtonMinTabWidth;
            int toolTipTabWidthThreshold;
            bool showOverflowMenu;
        };

        /*!
        * Applies the Secondary styling to a TabWidget.
        */
        static void applySecondaryStyle(TabWidget* tabWidget, bool bordered = true);

        /*!
        * Loads the tab bar config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default tab bar config data.
        */
        static Config defaultConfig();

        explicit TabWidget(QWidget* parent = nullptr);
        // Extra constructor for using an action toolbar to show the actions belonging to the widget.
        // Implicitly sets actionToolBarEnabled.
        TabWidget(TabWidgetActionToolBar* actionToolBar, QWidget* parent = nullptr);

        // Setter for the TabBar in case a custom one is needed
        // Makes sure the connections are set properly
        void setCustomTabBar(TabBar* tabBar);

        // Add or replace the action toolbar of the widget. Implicitly sets actionToolBarEnabled.
        void setActionToolBar(TabWidgetActionToolBar* actionToolBar);
        TabWidgetActionToolBar* actionToolBar() const;

        // Shows the action toolbar for this widget. If no TabWidgetActionToolBar is set either through
        // the constructor or through setActionToolBar, a default one is created and connected.
        void setActionToolBarVisible(bool visible = true);
        bool isActionToolBarVisible() const;

        void resizeEvent(QResizeEvent* resizeEvent) override;

    protected:
        void tabInserted(int index) override;
        void tabRemoved(int index) override;

    private:
        friend class Style;

        TabWidgetActionToolBarContainer* m_actionToolBarContainer = nullptr;
        QMenu* m_overflowMenu = nullptr;
        bool m_overFlowMenuDirty = true;
        bool m_shouldShowOverflowMenu = false;

        void setOverflowMenuVisible(bool visible);
        void resetOverflowMenu();
        void populateMenu();
        void showOverflowMenu();

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const TabWidget::Config& config);
        static bool unpolish(Style* style, QWidget* widget, const TabWidget::Config& config);
    };

    class AZ_QT_COMPONENTS_API TabWidgetActionToolBarContainer
        : public QFrame
    {
        Q_OBJECT
    public:
        explicit TabWidgetActionToolBarContainer(QWidget* parent = nullptr);
        QToolButton* overflowButton() const { return m_overflowButton; }

        TabWidgetActionToolBar* actionToolBar() const { return m_actionToolBar; }
        void setActionToolBar(TabWidgetActionToolBar* actionToolBar);
        void setActionToolBarVisible(bool visible);
        bool isActionToolBarVisible() const;

    private:
        QToolButton* m_overflowButton = nullptr;
        TabWidgetActionToolBar* m_actionToolBar = nullptr;

        void fixTabOrder();
    };

    class AZ_QT_COMPONENTS_API TabBar
        : public QTabBar
    {
        Q_OBJECT

    public:
        explicit TabBar(QWidget* parent = nullptr);

        void tabInserted(int index) override;
        void tabRemoved(int index) override;

    Q_SIGNALS:
        void overflowingChanged(bool overflowing);

    protected:
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* mouseEvent) override;
        void mouseMoveEvent(QMouseEvent* mouseEvent) override;
        void mouseReleaseEvent(QMouseEvent* mouseEvent) override;
        void paintEvent(QPaintEvent* paintEvent) override;
        QSize minimumSizeHint() const override;

    private:
        friend class Style;
        friend class TabWidget;

        enum Overflow {
            OverflowUnchecked,
            NotOverflowing,
            Overflowing
        };

        Overflow m_overflowing = OverflowUnchecked;
        int m_hoveredTab = -1;
        bool m_movingTab = false;
        QPoint m_lastMousePress;

        void resetOverflow();
        void overflowIfNeeded();
        void showCloseButtonAt(int index);
        void setToolTipIfNeeded(int index);

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const TabWidget::Config& config);
        static bool unpolish(Style* style, QWidget* widget, const TabWidget::Config& config);
        static int closeButtonSize(const Style* style, const QStyleOption* option, const QWidget* widget, const TabWidget::Config& config);
        static bool drawTabBarTabLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const TabWidget::Config& config);
        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget, const TabWidget::Config& config);
    };

} // namespace AzQtComponents
