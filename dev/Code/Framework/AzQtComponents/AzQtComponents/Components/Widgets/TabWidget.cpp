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
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QStyleOption>
#include <QTabWidget>

namespace AzQtComponents
{

    TabWidget::TabWidget(QWidget* parent)
        : QTabWidget(parent)
    {
        setTabBar(new TabBar(this));
        // Forcing styled background to allow using background-color from QSS
        setAttribute(Qt::WA_StyledBackground, true);
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
        return config;
    }

    TabWidget::Config TabWidget::defaultConfig()
    {
        return {
            QPixmap(), // TearIcon
            0,         // TearIconLeftPadding
            28,        // TabHeight
            160,       // MinimumTabWidth
            14         // CloseButtonSize
        };
    }

    void TabWidget::setActionToolBar(TabWidgetActionToolBar* actionToolBar)
    {
        if (!actionToolBar || m_actionToolBar == actionToolBar)
        {
            return;
        }

        // Replacing the corner widget immediately so that the old one can be removed without any issues if needed
        setCornerWidget(actionToolBar, Qt::TopRightCorner);
        installEventFilter(actionToolBar);

        if (m_actionToolBar)
        {
            removeEventFilter(m_actionToolBar);
            m_actionToolBar->setParent(nullptr);
            delete m_actionToolBar;
        }

        m_actionToolBar = actionToolBar;
    }

    TabWidgetActionToolBar* TabWidget::actionToolBar() const
    {
        return m_actionToolBar;
    }

    void TabWidget::setActionToolBarVisible(bool visible)
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
            // Need to remove the corner widget when it's not visible, so that the layout can adjust properly.
            // m_actionToolBar visibility is automatically handled from within setCornerWidget.
            if (visible)
            {
                setCornerWidget(m_actionToolBar);
            }
            else
            {
                setCornerWidget(nullptr);
            }

            // HACK: setCornerWidget is not idempotent, the corner widget gets hidden if it's called twice with the
            // same parameter. Manually forcing corner widget visibility should cover all the cases.
            m_actionToolBar->setVisible(visible);
        }
    }

    bool TabWidget::isActionToolBarVisible() const
    {
        return (m_actionToolBar && m_actionToolBar->isVisible());
    }

    TabBar::TabBar(QWidget* parent)
        : QTabBar(parent)
    {
    }


    bool TabBar::polish(Style* style, QWidget* widget, const TabWidget::Config& config)
    {
        Q_UNUSED(config);

        if (qobject_cast<TabBar*>(widget) || qobject_cast<TabWidget*>(widget))
        {
            style->repolishOnSettingsChange(widget);
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
                option->rect.left() + config.tearIconLeftPadding,
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
        if (size.width() < config.minimumTabWidth)
        {
            size.setWidth(config.minimumTabWidth);
        }

        return size;
    }

} // namespace AzQtComponents

#include <Components/Widgets/TabWidget.moc>
