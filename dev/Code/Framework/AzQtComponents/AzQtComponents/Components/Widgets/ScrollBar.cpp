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

#include <AzQtComponents/Components/Widgets/ScrollBar.h>
#include <AzQtComponents/Components/Style.h>

#include <QObject>
#include <QAbstractScrollArea>
#include <QMap>
#include <QSettings>
#include <QScrollBar>
#include <QEvent>

namespace AzQtComponents
{
    class ScrollBarWatcher : public QObject
    {
    public:
        explicit ScrollBarWatcher(QObject* parent = nullptr)
            : QObject(parent)
        {
        }

        bool install(QObject* widget)
        {
            auto scrollArea = qobject_cast<QAbstractScrollArea*>(widget);
            if (scrollArea && !m_widgets.contains(scrollArea))
            {
                auto scrollBars = scrollArea->findChildren<QScrollBar*>();
                if (!scrollBars.isEmpty())
                {
                    ScrollAreaData data;
                    data.hoverWasEnabled = scrollArea->testAttribute(Qt::WA_Hover);
                    for (auto scrollBar : scrollBars)
                    {
                        scrollBar->hide();
                        data.scrollBars.append(scrollBar);
                    }
                    scrollArea->setAttribute(Qt::WA_Hover);
                    scrollArea->installEventFilter(this);
                    m_widgets.insert(widget, data);
                    return true;
                }
            }
            return false;
        }

        bool uninstall(QObject* widget)
        {
            auto scrollArea = qobject_cast<QAbstractScrollArea*>(widget);
            if (scrollArea && m_widgets.contains(widget))
            {
                ScrollAreaData data = m_widgets.take(widget);
                scrollArea->setAttribute(Qt::WA_Hover, data.hoverWasEnabled);
                return true;
            }
            return false;
        }

        bool eventFilter(QObject* obj, QEvent* event) override
        {
            switch (event->type())
            {
                case QEvent::HoverEnter:
                    for (auto scrollBar : m_widgets.value(obj).scrollBars)
                    {
                        scrollBar->show();
                    }
                    break;
                case QEvent::HoverLeave:
                    for (auto scrollBar : m_widgets.value(obj).scrollBars)
                    {
                        scrollBar->hide();
                    }
                    break;
                default:
                    break;
            }
            return QObject::eventFilter(obj, event);
        }

    private:
        struct ScrollAreaData
        {
            bool hoverWasEnabled;
            QList<QScrollBar*> scrollBars;
        };
        QMap<QObject*, ScrollAreaData> m_widgets;
    };

    QPointer<ScrollBarWatcher> ScrollBar::m_scrollBarWatcher;

    ScrollBar::Config ScrollBar::loadConfig(QSettings& settings)
    {
        Q_UNUSED(settings);
        return defaultConfig();
    }

    ScrollBar::Config ScrollBar::defaultConfig()
    {
        return Config();
    }

    void ScrollBar::initializeWatcher()
    {
        Q_ASSERT(m_scrollBarWatcher.isNull());
        m_scrollBarWatcher = new ScrollBarWatcher;
    }

    void ScrollBar::uninitializeWatcher()
    {
        delete m_scrollBarWatcher;
    }

    bool ScrollBar::polish(Style* style, QWidget* widget, const ScrollBar::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        return m_scrollBarWatcher->install(widget);
    }

    bool ScrollBar::unpolish(Style* style, QWidget* widget, const ScrollBar::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        return m_scrollBarWatcher->uninstall(widget);
    }

} // namespace AzQtComponents

#include <Components/Widgets/ScrollBar.moc>
