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

#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h> // for the QMargins metatype declarations
#include <QDesktopWidget>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include <QPoint>
#include <QSettings>

namespace AzQtComponents
{
    namespace CardConstants
    {
        static const char* kPropertySelected = "selected";
    }

    Card::Card(QWidget* parent /* = nullptr */)
        // DO NOT SET THE PARENT OF THE CardHeader TO THIS!
        // It will cause a crash, because the this object is not initialized enough to be used as a parent object.
        : Card(new CardHeader(), parent)
    {
    }

    Card::Card(CardHeader* customHeader, QWidget* parent)
        : QFrame(parent)
        , m_header(customHeader)
    {
        // The header is owned by the Card, so set the parent.
        m_header->setParent(this);

        Config defaultConfigValues = defaultConfig();

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_warningIcon = QIcon(":/Cards/img/UI20/Cards/warning.png");

        // create header bar
        Style::addClass(m_header, "primaryCardHeader");
        m_header->setWarningIcon(m_warningIcon);
        m_header->setWarning(false);
        m_header->setReadOnly(false);
        m_header->setExpandable(true);
        m_header->setHasContextMenu(true);

        // has to be done here, otherwise it will affect all tooltips
        // as tooltips do not support any kind of css matchers other than QToolTip
        m_header->setStyleSheet(QStringLiteral("QToolTip { padding: %1px; }").arg(defaultConfigValues.toolTipPaddingInPixels));

        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setMargin(0);
        m_mainLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        m_mainLayout->addWidget(m_header);

        // add a placeholder, so that the ordering of widgets in Cards works
        m_contentWidget = new QWidget(this);
        m_mainLayout->addWidget(m_contentWidget);


        m_separatorContainer = new QFrame(this);
        m_separatorContainer->setObjectName("SeparatorContainer");
        QBoxLayout* separatorLayout = new QHBoxLayout(m_separatorContainer);
        m_separatorContainer->setLayout(separatorLayout);

        m_separator = new QFrame(m_separatorContainer);
        AzQtComponents::Style::addClass(m_separator, "separator");
        m_separator->setFrameShape(QFrame::HLine);
        separatorLayout->addWidget(m_separator);

        m_secondaryHeader = new AzQtComponents::CardHeader(this);
        AzQtComponents::Style::addClass(m_secondaryHeader, "secondaryCardHeader");
        m_secondaryHeader->setExpanded(false); // default the secondary header to not expanded
        m_secondaryHeader->setExpandable(true);

        m_mainLayout->addWidget(m_separatorContainer);
        m_mainLayout->addWidget(m_secondaryHeader);

        updateSecondaryContentVisibility();

        connect(m_secondaryHeader, &CardHeader::expanderChanged, this, &Card::setSecondaryContentExpanded);

        setLayout(m_mainLayout);

        connect(m_header, &CardHeader::expanderChanged, this, &Card::setExpanded);
        connect(m_header, &CardHeader::contextMenuRequested, this, &Card::contextMenuRequested);

        // force the selected property to be set the first time, so the stylesheet works correctly on false
        setProperty(CardConstants::kPropertySelected, m_selected);
    }

    QWidget* Card::contentWidget() const
    {
        return m_contentWidget;
    }

    void Card::setContentWidget(QWidget* contentWidget)
    {
        delete m_contentWidget;

        m_contentWidget = contentWidget;

        m_mainLayout->insertWidget(1, contentWidget);
    }

    void Card::setTitle(const QString& title)
    {
        m_header->setTitle(title);
    }

    QString Card::title() const
    {
        return m_header->title();
    }

    CardHeader* Card::header() const
    {
        return m_header;
    }

    CardNotification* Card::addNotification(QString message)
    {
        CardNotification* notification = new CardNotification(this, message, m_warningIcon);

        notification->setVisible(isExpanded());
        m_notifications.push_back(notification);
        m_mainLayout->addWidget(notification);

        return notification;
    }

    void Card::clearNotifications()
    {
        for (CardNotification* notification : m_notifications)
        {
            delete notification;
        }

        m_notifications.clear();
    }

    void Card::setExpanded(bool expand)
    {
        setUpdatesEnabled(false);

        m_header->setExpanded(expand);

        if (m_contentWidget)
        {
            m_contentWidget->setVisible(expand);
        }

        //toggle notification visibility
        for (CardNotification* notification : m_notifications)
        {
            notification->setVisible(expand);
        }

        updateSecondaryContentVisibility();

        setUpdatesEnabled(true);

        Q_EMIT expandStateChanged(expand);
    }

    bool Card::isExpanded() const
    {
        return m_header->isExpanded();
    }

    void Card::setSelected(bool selected)
    {
        if (m_selected != selected)
        {
            m_selected = selected;
            setProperty(CardConstants::kPropertySelected, m_selected);
            style()->unpolish(this);
            style()->polish(this);
        }
    }

    bool Card::isSelected() const
    {
        return m_selected;
    }

    void Card::setDragging(bool dragging)
    {
        m_dragging = dragging;
    }

    bool Card::isDragging() const
    {
        return m_dragging;
    }

    void Card::setDropTarget(bool dropTarget)
    {
        m_dropTarget = dropTarget;
    }

    bool Card::isDropTarget() const
    {
        return m_dropTarget;
    }


    void Card::setSecondaryContentExpanded(bool expand)
    {
        {
            // No need to respond to the secondary header state changing; we're causing the change
            QSignalBlocker blocker(m_secondaryHeader);
            m_secondaryHeader->setExpanded(expand);
        }

        updateSecondaryContentVisibility();

        Q_EMIT secondaryContentExpandStateChanged(expand);
    }

    bool Card::isSecondaryContentExpanded() const
    {
        return m_secondaryHeader->isExpanded();
    }

    void Card::setSecondaryTitle(const QString& secondaryTitle)
    {
        m_secondaryHeader->setTitle(secondaryTitle);

        updateSecondaryContentVisibility();
    }

    QString Card::secondaryTitle() const
    {
        return m_secondaryHeader->title();
    }

    void Card::setSecondaryContentWidget(QWidget* secondaryContentWidget)
    {
        delete m_secondaryContentWidget;

        m_secondaryContentWidget = secondaryContentWidget;

        // Layout is:
        // 0 - primary header
        // 1 - content widget
        // 2 - separator
        // 3 - secondary header
        // 4 - secondary widget
        // 5, etc - notifications

        m_mainLayout->insertWidget(4, secondaryContentWidget);

        updateSecondaryContentVisibility();
    }

    QWidget* Card::secondaryContentWidget() const
    {
        return m_secondaryContentWidget;
    }

    Card::Config Card::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        config.headerIconSizeInPixels = settings.value("HeaderIconSizeInPixels", config.headerIconSizeInPixels).toInt();
        config.toolTipPaddingInPixels = settings.value("ToolTipPaddingInPixels", config.toolTipPaddingInPixels).toInt();

        return config;
    }

    Card::Config Card::defaultConfig()
    {
        Config config;

        config.toolTipPaddingInPixels = 5;
        config.headerIconSizeInPixels = CardHeader::defaultIconSize();

        return config;
    }

    bool Card::polish(Style* style, QWidget* widget, const Card::Config& config)
    {
        bool polished = false;

        CardHeader* cardHeader = qobject_cast<CardHeader*>(widget);
        if (cardHeader != nullptr)
        {
            CardHeader::setIconSize(config.headerIconSizeInPixels);

            QString newStyleSheet = QStringLiteral("QToolTip { padding: %1px; }").arg(config.toolTipPaddingInPixels);
            if (newStyleSheet != cardHeader->styleSheet())
            {
                cardHeader->setStyleSheet(newStyleSheet);
            }

            cardHeader->configSettingsChanged();

            style->repolishOnSettingsChange(cardHeader);

            polished = true;
        }

        return polished;
    }

    bool Card::hasSecondaryContent() const
    {
        return ((m_secondaryHeader->title().length() > 0) || (m_secondaryContentWidget != nullptr));
    }

    void Card::updateSecondaryContentVisibility()
    {
        bool updatesWereEnabled = updatesEnabled();

        if (updatesWereEnabled)
        {
            setUpdatesEnabled(false);
        }

        bool expanded = m_header->isExpanded();
        bool secondaryExpanded = hasSecondaryContent() && expanded && m_secondaryHeader->isExpanded();

        m_separatorContainer->setVisible(hasSecondaryContent() && expanded);
        m_secondaryHeader->setVisible(hasSecondaryContent() && expanded);

        if (m_secondaryContentWidget != nullptr)
        {
            m_secondaryContentWidget->setVisible(secondaryExpanded);
        }

        if (updatesWereEnabled)
        {
            setUpdatesEnabled(true);
        }
    }

} // namespace AzQtComponents
