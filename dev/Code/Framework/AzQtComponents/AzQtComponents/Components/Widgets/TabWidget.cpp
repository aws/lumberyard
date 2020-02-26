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

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzQtComponents/Components/Widgets/TabWidgetActionToolBar.h>

#include <QAction>
#include <QActionEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QStyleOption>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace AzQtComponents
{

    static int g_borderWidth = 1;
    static int g_releaseTabMaxAnimationDuration = 250; // Copied from qtabbar_p.h
    // Will be initialized by TabWidget::loadConfig
    static int g_closeButtonWidth = -1;
    static int g_closeButtonPadding = -1;
    static int g_closeButtonMinTabWidth = -1;
    static int g_toolTipTabWidthThreshold = -1;

    TabWidget::TabWidget(QWidget* parent)
        : QTabWidget(parent)
        , m_overflowMenu(new QMenu(this))
        , m_actionToolBarContainer(new TabWidgetActionToolBarContainer(this))
    {
        auto tabBar = new TabBar(this);
        setTabBar(tabBar);
        // Forcing styled background to allow using background-color from QSS
        setAttribute(Qt::WA_StyledBackground, true);

        m_actionToolBarContainer->overflowButton()->setVisible(false);
        m_actionToolBarContainer->overflowButton()->setMenu(m_overflowMenu);
        setCornerWidget(m_actionToolBarContainer, Qt::TopRightCorner);

        connect(m_overflowMenu, &QMenu::aboutToShow, this, &TabWidget::populateMenu);
        connect(this, &TabWidget::currentChanged, this, &TabWidget::resetOverflowMenu);
        connect(tabBar, &TabBar::overflowingChanged, this, &TabWidget::setOverflowMenuVisible);
    }

    TabWidget::TabWidget(TabWidgetActionToolBar* actionToolBar, QWidget* parent)
        : TabWidget(parent)
    {
        setActionToolBar(actionToolBar);
    }

    TabWidget::Config TabWidget::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();
        ConfigHelpers::read<QPixmap>(settings, QStringLiteral("TearIcon"), config.tearIcon);
        ConfigHelpers::read<int>(settings, QStringLiteral("TearIconLeftPadding"), config.tearIconLeftPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("TabHeight"), config.tabHeight);
        ConfigHelpers::read<int>(settings, QStringLiteral("MinimumTabWidth"), config.minimumTabWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("CloseButtonSize"), config.closeButtonSize);
        ConfigHelpers::read<int>(settings, QStringLiteral("TextRightPadding"), config.textRightPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("CloseButtonRightPadding"), config.closeButtonRightPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("CloseButtonMinTabWidth"), config.closeButtonMinTabWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("ToolTipTabWidthThreshold"), config.toolTipTabWidthThreshold);
        ConfigHelpers::read<bool>(settings, QStringLiteral("ShowOverflowMenu"), config.showOverflowMenu);

        g_closeButtonPadding = config.closeButtonRightPadding;
        g_closeButtonWidth = config.closeButtonSize;
        g_closeButtonMinTabWidth = config.closeButtonMinTabWidth;
        g_toolTipTabWidthThreshold = config.toolTipTabWidthThreshold;

        return config;
    }

    TabWidget::Config TabWidget::defaultConfig()
    {
        return {
            QPixmap(), // TearIcon
            0,         // TearIconLeftPadding
            30,        // TabHeight 28 + 1 (top margin) + 1 (bottom margin)
            32,        // MinimumTabWidth
            16,        // CloseButtonSize
            40,        // TextRightPadding
            4,         // CloseButtonRightPadding
            50,        // CloseButtonMinTabWidth
            50,        // ToolTipTabWidthThreshold
            true      // ShowOverflowMenu
        };
    }

    void TabWidget::setActionToolBar(TabWidgetActionToolBar* actionToolBar)
    {
        m_actionToolBarContainer->setActionToolBar(actionToolBar);
    }

    TabWidgetActionToolBar* TabWidget::actionToolBar() const
    {
        return m_actionToolBarContainer->actionToolBar();
    }

    void TabWidget::setActionToolBarVisible(bool visible)
    {
        m_actionToolBarContainer->setActionToolBarVisible(visible);
    }

    bool TabWidget::isActionToolBarVisible() const
    {
        return m_actionToolBarContainer->isActionToolBarVisible();
    }

    void TabWidget::resizeEvent(QResizeEvent* resizeEvent)
    {
        auto tabBarWidget = qobject_cast<TabBar*>(tabBar());

        if (tabBarWidget)
        {
            tabBarWidget->resetOverflow();
        }

        QTabWidget::resizeEvent(resizeEvent);
    }

    void TabWidget::tabInserted(int index)
    {
        Q_UNUSED(index);
        resetOverflowMenu();
    }

    void TabWidget::tabRemoved(int index)
    {
        Q_UNUSED(index);
        resetOverflowMenu();
    }

    void TabWidget::setOverflowMenuVisible(bool visible)
    {
        if (m_shouldShowOverflowMenu)
        {
            m_actionToolBarContainer->overflowButton()->setVisible(visible);
        }
        else
        {
            m_actionToolBarContainer->overflowButton()->setVisible(false);
        }
    }

    void TabWidget::resetOverflowMenu()
    {
        m_overFlowMenuDirty = true;
    }

    void TabWidget::populateMenu()
    {
        if (!m_overFlowMenuDirty)
        {
            return;
        }

        m_overFlowMenuDirty = false;
        m_overflowMenu->clear();

        const int tabCount = count();
        const int currentTab = currentIndex();
        for (int i = 0; i < tabCount; ++i)
        {
            auto action = new QAction(tabText(i));
            action->setCheckable(true);
            action->setChecked(i == currentTab);
            connect(action, &QAction::triggered, this, std::bind(&TabWidget::setCurrentIndex, this, i));
            m_overflowMenu->addAction(action);
        }
    }

    bool TabWidget::polish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(config);

        if (auto tabWidget = qobject_cast<TabWidget*>(widget))
        {
            style->repolishOnSettingsChange(widget);
            tabWidget->m_shouldShowOverflowMenu = config.showOverflowMenu;
            return true;
        }

        return false;
    }

    bool TabWidget::unpolish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        if (auto tabWidget = qobject_cast<TabWidget*>(widget))
        {
            tabWidget->m_shouldShowOverflowMenu = false;
            return true;
        }

        return false;
    }

    TabWidgetActionToolBarContainer::TabWidgetActionToolBarContainer(QWidget* parent)
        : QFrame(parent)
        , m_overflowButton(new QToolButton(this))
    {
        m_overflowButton->setObjectName(QStringLiteral("tabWidgetOverflowButton"));
        m_overflowButton->setPopupMode(QToolButton::InstantPopup);
        m_overflowButton->setAutoRaise(true);
        m_overflowButton->setIcon(QIcon(QStringLiteral(":/stylesheet/img/UI20/dropdown-button-arrow.svg")));
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins({}); // Defaults to 0
        layout->addWidget(m_overflowButton);
    }

    void TabWidgetActionToolBarContainer::setActionToolBar(TabWidgetActionToolBar* actionToolBar)
    {
        if (!actionToolBar || m_actionToolBar == actionToolBar)
        {
            return;
        }

        auto tabWidget = qobject_cast<TabWidget*>(parent());

        if (m_actionToolBar)
        {
            if (tabWidget)
            {
                tabWidget->removeEventFilter(m_actionToolBar);
            }
            m_actionToolBar->hide();
            layout()->removeWidget(m_actionToolBar);
            m_actionToolBar->setParent(nullptr);
            delete m_actionToolBar;
        }

        m_actionToolBar = actionToolBar;
        if (tabWidget)
        {
            tabWidget->installEventFilter(m_actionToolBar);
        }
        layout()->addWidget(m_actionToolBar);
        m_actionToolBar->show();
    }

    void TabWidgetActionToolBarContainer::setActionToolBarVisible(bool visible)
    {
        if (m_actionToolBar && visible == m_actionToolBar->isVisible())
        {
            return;
        }

        // Creating the action toolbar only when strictly necessary, no need to create it if visible is false.
        if (!m_actionToolBar && visible)
        {
            setActionToolBar(new TabWidgetActionToolBar(this));
        }

        if (m_actionToolBar)
        {
            m_actionToolBar->setVisible(visible);
        }
    }

    bool TabWidgetActionToolBarContainer::isActionToolBarVisible() const
    {
        return (m_actionToolBar && m_actionToolBar->isVisible());
    }

    TabBar::TabBar(QWidget* parent)
        : QTabBar(parent)
    {
        connect(this, &TabBar::currentChanged, this, &TabBar::resetOverflow);
        setMouseTracking(true);
    }

    void TabBar::resetOverflow()
    {
        m_overflowing = OverflowUnchecked;
    }

    void TabBar::enterEvent(QEvent* event)
    {
        auto enterEvent = static_cast<QEnterEvent*>(event);
        m_hoveredTab = tabAt(enterEvent->pos());

        QTabBar::enterEvent(event);
    }

    void TabBar::leaveEvent(QEvent* event)
    {
        m_hoveredTab = -1;

        QTabBar::leaveEvent(event);
    }

    void TabBar::mousePressEvent(QMouseEvent* mouseEvent)
    {
        if (mouseEvent->buttons() & Qt::LeftButton)
        {
            m_lastMousePress = mouseEvent->pos();
        }

        QTabBar::mousePressEvent(mouseEvent);
    }

    void TabBar::mouseMoveEvent(QMouseEvent* mouseEvent)
    {
        if (mouseEvent->buttons() & Qt::LeftButton)
        {
            // When a tab is moved, the actual tab is hidden and a snapshot rendering of the
            // selected tab is moved around. The close button is not explicitly rendered for the
            // moved tab during this operation. We need to make sure not to set it visible again
            // while the tab is moving. This flag makes sure it happens.
            m_movingTab = true;
        }

        m_hoveredTab = tabAt(mouseEvent->pos());

        QTabBar::mouseMoveEvent(mouseEvent);
    }

    void TabBar::mouseReleaseEvent(QMouseEvent* mouseEvent)
    {
        if (m_movingTab && !(mouseEvent->buttons() & Qt::LeftButton))
        {
            // When a moving tab is released, there is a short animation to put the moving tab
            // in place before starting rendering again, need to make sure not to explicitly show
            // any close button before that animation is over, because tabRect(i) will
            // not follow the animated tab, and the close button will be shown in the target tab
            // position.
            // This only pauses any custom close button or tool tip visualization logic during the animation.
            // Formula for animation duration taken from QTabBar::mouseReleaseEvent()
            auto animationDuration = (g_releaseTabMaxAnimationDuration * qAbs(mouseEvent->pos().x() - m_lastMousePress.x())) /
                                     tabRect(currentIndex()).width();
            QTimer::singleShot(animationDuration, [this]() {
                m_movingTab = false;
                update();
            });

        }

        QTabBar::mouseReleaseEvent(mouseEvent);
    }

    void TabBar::paintEvent(QPaintEvent* paintEvent)
    {
        overflowIfNeeded();

        // Close buttons are generated after TabBar is constructed.
        // This means we can't initialize their visibility from the constructor,
        // but need to ensure they are in the right state before painting.
        showCloseButtonAt(m_hoveredTab);
        setToolTipIfNeeded(m_hoveredTab);

        QTabBar::paintEvent(paintEvent);
    }

    void TabBar::tabInserted(int /*index*/)
    {
        resetOverflow();
    }

    void TabBar::tabRemoved(int /*index*/)
    {
        resetOverflow();
    }

    void TabBar::overflowIfNeeded()
    {
        if (m_overflowing != OverflowUnchecked)
        {
            return;
        }

        // HACK: implicitly reset QTabWidgetPrivate::layoutDirty
        // Force recomputing tab width with default overflow settings
        setIconSize(iconSize());

        int tabsWidth = 0;
        for (int i = 0; i < count(); i++)
        {
            tabsWidth += tabRect(i).width();
        }

        auto tabWidget = qobject_cast<TabWidget*>(parent());
        if (!tabWidget)
        {
            return;
        }

        int availableWidth = tabWidget->width();
        if (tabWidget->isActionToolBarVisible())
        {
            availableWidth -= tabWidget->actionToolBar()->width();
        }

        if (tabsWidth >= availableWidth)
        {
            m_overflowing = Overflowing;
        }
        else
        {
            m_overflowing = NotOverflowing;
        }

        emit overflowingChanged(m_overflowing == Overflowing);

        // HACK: implicitly reset QTabWidgetPrivate::layoutDirty
        // Force recomputing tab width with new overflow settings
        setIconSize(iconSize());
    }

    void TabBar::showCloseButtonAt(int index)
    {
        if (m_movingTab)
        {
            return;
        }

        ButtonPosition closeSide = (ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, this);
        for (int i = 0; i < count(); i++)
        {
            QWidget* tabBtn = tabButton(i, closeSide);
            int tabWidth = tabRect(i).width();

            if (tabBtn)
            {
                bool shouldShow = (i == index) && tabWidth >= g_closeButtonMinTabWidth;

                // Need to explicitly move the button if we want to customize its position
                // Don't bother moving it if it won't be shown
                if (shouldShow)
                {
                    QPoint p = tabRect(i).topLeft();
                    p.setX(p.x() + tabRect(i).width() - g_closeButtonPadding - g_closeButtonWidth);
                    p.setY(p.y() + (tabRect(i).height() - g_closeButtonWidth) / 2);
                    tabBtn->move(p);
                }

                tabBtn->setVisible(shouldShow);
            }

            // Remove tooltips from close buttons so we can see the tab ones
            if (tabBtn)
            {
                tabBtn->setToolTip("");
            }
        }
    }

    void TabBar::setToolTipIfNeeded(int index)
    {
        if (m_movingTab)
        {
            return;
        }

        int tabWidth = tabRect(index).width();
        if (tabWidth < g_toolTipTabWidthThreshold)
        {
            setTabToolTip(index, tabText(index));
        }
        else
        {
            setTabToolTip(index, "");
        }
    }

    bool TabBar::polish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        if (auto tabBar = qobject_cast<TabBar*>(widget))
        {
            tabBar->setUsesScrollButtons(false);
            return true;
        }

        return false;
    }

    bool TabBar::unpolish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        if (auto tabBar = qobject_cast<TabBar*>(widget))
        {
            tabBar->setUsesScrollButtons(true);
            return true;
        }

        return false;
    }

    int TabBar::closeButtonSize(const Style* style, const QStyleOption* option, const QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);
        Q_UNUSED(widget);

        return config.closeButtonSize;
    }

    bool TabBar::drawTabBarTabLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const TabWidget::Config& config)
    {
        const QStyleOptionTab* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);

        if (!tabOption)
        {
            return false;
        }

        const TabBar* tabBar = qobject_cast<const TabBar*>(widget);
        if (!tabBar)
        {
            return false;
        }

        // Tear icon drawing can't be done properly using QSS only because text and image can't be positioned differently
        if (tabBar->isMovable() && (option->state & (QStyle::State_MouseOver | QStyle::State_Sunken)))
        {
            painter->save();
            const QRect tearIconRectangle(
                option->rect.left() + config.tearIconLeftPadding + g_borderWidth,
                option->rect.top() + (option->rect.height() - config.tearIcon.height()) / 2,
                config.tearIcon.width(),
                config.tearIcon.height());
            style->baseStyle()->drawItemPixmap(painter, tearIconRectangle, 0, config.tearIcon);
            painter->restore();
        }

        painter->save();
        style->baseStyle()->drawControl(QStyle::CE_TabBarTabLabel, option, painter, widget);
        painter->restore();

        return true;
    }

    QSize TabBar::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget, const TabWidget::Config& config)
    {
        const QStyleOptionTab* tabOption = qstyleoption_cast<const QStyleOptionTab*>(option);

        if (!tabOption)
        {
            return QSize();
        }

        const TabBar* tabBar = qobject_cast<const TabBar*>(widget);
        if (!tabBar)
        {
            return QSize();
        }

        QSize size = style->baseStyle()->sizeFromContents(type, tabOption, contentsSize, widget);

        size.setHeight(config.tabHeight);

        if (tabBar->m_overflowing != Overflowing || (tabOption->state & QStyle::State_Selected))
        {
            size.setWidth(size.width() + config.textRightPadding);
            if (tabBar->tabsClosable()) {
                size -= QSize(config.closeButtonSize + config.closeButtonRightPadding, 0);
            }
        }
        else
        {
            auto tabWidget = qobject_cast<TabWidget*>(tabBar->parent());
            if (!tabWidget)
            {
                return {};
            }

            int availableWidth = tabWidget->width();
            if (tabWidget->isActionToolBarVisible())
            {
                availableWidth -= tabWidget->actionToolBar()->width();
            }

            QStyleOptionTab selectedOptions;
            tabBar->initStyleOption(&selectedOptions, tabBar->currentIndex());
            QSize selectedSize = sizeFromContents(style, type, &selectedOptions, contentsSize, tabBar, config);
            availableWidth -= selectedSize.width();
            int targetWidth = availableWidth / (tabBar->count() - 1);
            size.setWidth(std::max(targetWidth, config.minimumTabWidth));
        }

        return size;
    }

} // namespace AzQtComponents

#include <Components/Widgets/TabWidget.moc>
