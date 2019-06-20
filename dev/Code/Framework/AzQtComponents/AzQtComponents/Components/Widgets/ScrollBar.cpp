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
#include <AzCore/std/functional.h>
#include <AzCore/std/function/invoke.h>

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

                connect(scrollArea, &QObject::destroyed, this, &ScrollBarWatcher::scrollAreaDestroyed);
            }
            return false;
        }

        bool uninstall(QObject* widget)
        {
            auto scrollArea = qobject_cast<QAbstractScrollArea*>(widget);
            if (scrollArea && m_widgets.contains(widget))
            {
                disconnect(scrollArea, &QObject::destroyed, this, &ScrollBarWatcher::scrollAreaDestroyed);

                ScrollAreaData data = m_widgets.take(widget);
                scrollArea->removeEventFilter(this);
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
                {
                    perScrollBar(obj, &QScrollBar::show);
                    break;
                }

                case QEvent::HoverLeave:
                {
                    perScrollBar(obj, &QScrollBar::hide);
                    break;
                }

                default:
                    break;
            }
            return QObject::eventFilter(obj, event);
        }

    private:
        struct ScrollAreaData
        {
            bool hoverWasEnabled;

            // Store QPointers so that if the scrollbars are cleaned up before the scroll area, we don't continue to reference it
            QList<QPointer<QScrollBar>> scrollBars;
        };
        QMap<QObject*, ScrollAreaData> m_widgets;

        void perScrollBar(QObject* scrollArea, void (QScrollBar::*callback)(void))
        {
            auto iterator = m_widgets.find(scrollArea);
            if (iterator != m_widgets.end())
            {
                for (auto& scrollBar : iterator.value().scrollBars)
                {
                    if (scrollBar)
                    {
                        AZStd::invoke(callback, scrollBar.data());
                    }
                }
            }
        }

        void scrollAreaDestroyed(QObject* scrollArea)
        {
            m_widgets.remove(scrollArea);
        }
    };

    QPointer<ScrollBarWatcher> ScrollBar::s_scrollBarWatcher = nullptr;
    unsigned int ScrollBar::s_watcherReferenceCount = 0;

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
        if (!s_scrollBarWatcher)
        {
            Q_ASSERT(s_watcherReferenceCount == 0);
            s_scrollBarWatcher = new ScrollBarWatcher;
        }

        ++s_watcherReferenceCount;
    }

    void ScrollBar::uninitializeWatcher()
    {
        Q_ASSERT(!s_scrollBarWatcher.isNull());
        Q_ASSERT(s_watcherReferenceCount > 0);

        --s_watcherReferenceCount;

        if (s_watcherReferenceCount == 0)
        {
            delete s_scrollBarWatcher;
            s_scrollBarWatcher = nullptr;
        }
    }

    bool ScrollBar::polish(Style* style, QWidget* widget, const ScrollBar::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        return s_scrollBarWatcher->install(widget);
    }

    bool ScrollBar::unpolish(Style* style, QWidget* widget, const ScrollBar::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        return s_scrollBarWatcher->uninstall(widget);
    }

} // namespace AzQtComponents

#include <Components/Widgets/ScrollBar.moc>
