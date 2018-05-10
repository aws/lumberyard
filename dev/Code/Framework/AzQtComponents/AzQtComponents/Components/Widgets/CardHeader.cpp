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

#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDesktopServices>
#include <QMenu>
#include <QStyle>
#include <QMouseEvent>

namespace AzQtComponents
{
        
    namespace HeaderBarConstants
    {
        // names for widgets so they can be found in stylesheet
        static const char* kBackgroundId = "Background";
        static const char* kExpanderId = "Expander";
        static const char* kIconId = "Icon";
        static const char* kWarningIconId = "WarningIcon";
        static const char* kTitleId = "Title";
        static const char* kContextMenuId = "ContextMenu";
        static const char* khelpButtonId = "Help";
    }

    int CardHeader::s_iconSize = CardHeader::defaultIconSize();

    CardHeader::CardHeader(QWidget* parent /*= nullptr*/)
        : QFrame(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_backgroundFrame = new QFrame(this);
        m_backgroundFrame->setObjectName(HeaderBarConstants::kBackgroundId);
        m_backgroundFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_backgroundFrame->setAutoFillBackground(true);

        // expander widget
        m_expanderButton = new QPushButton(m_backgroundFrame);
        m_expanderButton->setObjectName(HeaderBarConstants::kExpanderId);
        m_expanderButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_expanderButton->setCheckable(true);
        m_expanderButton->setChecked(true);
        m_expanderButton->show();
        connect(m_expanderButton, &QPushButton::toggled, this, &CardHeader::expanderChanged);

        // icon widget
        m_iconLabel = new QLabel(m_backgroundFrame);
        m_iconLabel->setObjectName(HeaderBarConstants::kIconId);
        m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_iconLabel->hide();

        // title widget
        m_titleLabel = new QLabel(m_backgroundFrame);
        m_titleLabel->setObjectName(HeaderBarConstants::kTitleId);
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_titleLabel->hide();

        // warning widget
        m_warningLabel = new QLabel(m_backgroundFrame);
        m_warningLabel->setObjectName(HeaderBarConstants::kWarningIconId);
        m_warningLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_warningLabel->hide();

        m_helpButton = new QPushButton(m_backgroundFrame);
        m_helpButton->setObjectName(HeaderBarConstants::khelpButtonId);
        m_helpButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_helpButton->hide();
        connect(m_helpButton, &QPushButton::clicked, this, &CardHeader::triggerHelpButton);


        // context menu widget
        m_contextMenuButton = new QPushButton(m_backgroundFrame);
        m_contextMenuButton->setObjectName(HeaderBarConstants::kContextMenuId);
        m_contextMenuButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_contextMenuButton->hide();
        connect(m_contextMenuButton, &QPushButton::clicked, this, &CardHeader::triggerContextMenuUnderButton);

        m_backgroundLayout = new QHBoxLayout(m_backgroundFrame);
        m_backgroundLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_backgroundLayout->setSpacing(0);
        m_backgroundLayout->setMargin(0);
        m_backgroundLayout->setContentsMargins(0, 0, 0, 0);
        m_backgroundLayout->addWidget(m_expanderButton);
        m_backgroundLayout->addWidget(m_iconLabel);
        m_backgroundLayout->addWidget(m_titleLabel);
        m_backgroundLayout->addStretch(1);
        m_backgroundLayout->addWidget(m_warningLabel);
        m_backgroundLayout->addWidget(m_helpButton);
        m_backgroundLayout->addWidget(m_contextMenuButton);
        m_backgroundFrame->setLayout(m_backgroundLayout);

        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setSpacing(0);
        m_mainLayout->setMargin(0);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->addWidget(m_backgroundFrame);
        setLayout(m_mainLayout);

        setExpanded(true);
        setWarning(false);
        setReadOnly(false);
        updateStyleSheets();
    }

    void CardHeader::setTitle(const QString& title)
    {
        m_titleLabel->setText(title);
        m_titleLabel->setVisible(!title.isEmpty());
    }

    QString CardHeader::title() const
    {
        return m_titleLabel->text();
    }

    void CardHeader::setIcon(const QIcon& icon)
    {
        m_icon = icon;

        if (!icon.isNull())
        {
            m_iconLabel->setPixmap(icon.pixmap(s_iconSize, s_iconSize));
        }

        m_iconLabel->setVisible(!icon.isNull());
    }

    void CardHeader::configSettingsChanged()
    {
        if (!m_icon.isNull() && (m_iconLabel != nullptr))
        {
            m_iconLabel->setPixmap(m_icon.pixmap(s_iconSize, s_iconSize));
        }

        if (!m_warningIcon.isNull())
        {
            m_warningLabel->setPixmap(m_warningIcon.pixmap(s_iconSize, s_iconSize));
        }
    }

    void CardHeader::setIconSize(int iconSize)
    {
        s_iconSize = iconSize;
    }

    int CardHeader::defaultIconSize()
    {
        return 16;
    }

    void CardHeader::setExpandable(bool expandable)
    {
        m_expanderButton->setEnabled(expandable);
    }

    bool CardHeader::isExpandable() const
    {
        return m_expanderButton->isEnabled();
    }

    void CardHeader::setExpanded(bool expanded)
    {
        m_expanderButton->setChecked(expanded);
    }

    bool CardHeader::isExpanded() const
    {
        return m_expanderButton->isChecked();
    }

    void CardHeader::setWarningIcon(const QIcon& icon)
    {
        m_warningIcon = icon;

        m_warningLabel->setPixmap(!m_warningIcon.isNull() ? m_warningIcon.pixmap(s_iconSize, s_iconSize) : QPixmap());
    }

    void CardHeader::setWarning(bool warning)
    {
        if (m_warning != warning)
        {
            m_warning = warning;
            m_warningLabel->setVisible(m_warning);
            updateStyleSheets();
        }
    }

    bool CardHeader::isWarning() const
    {
        return m_warning;
    }

    void CardHeader::setReadOnly(bool readOnly)
    {
        if (m_readOnly != readOnly)
        {
            m_readOnly = readOnly;
            updateStyleSheets();
        }
    }

    bool CardHeader::isReadOnly() const
    {
        return m_readOnly;
    }

    void CardHeader::setHasContextMenu(bool showContextMenu)
    {
        m_contextMenuButton->setVisible(showContextMenu);
    }

    void CardHeader::mouseDoubleClickEvent(QMouseEvent* event)
    {
        //allow double click to expand/contract
        if (event->button() == Qt::LeftButton && isExpandable())
        {
            bool expand = !isExpanded();
            setExpanded(expand);
            emit expanderChanged(expand);
        }

        QFrame::mouseDoubleClickEvent(event);
    }

    void CardHeader::contextMenuEvent(QContextMenuEvent* event)
    {
        Q_EMIT contextMenuRequested(event->globalPos());
        event->accept();
    }

    void CardHeader::triggerContextMenuUnderButton()
    {
        Q_EMIT contextMenuRequested(mapToGlobal(m_contextMenuButton->pos() + QPoint(0, m_contextMenuButton->height())));
    }

    void CardHeader::updateStyleSheets()
    {
        setProperty("warning", m_warning);
        setProperty("readOnly", m_readOnly);
        style()->unpolish(this);
        style()->polish(this);

        m_backgroundFrame->setProperty("warning", m_warning);
        m_backgroundFrame->setProperty("readOnly", m_readOnly);
        m_backgroundFrame->style()->unpolish(m_backgroundFrame);
        m_backgroundFrame->style()->polish(m_backgroundFrame);
    }

    void CardHeader::triggerHelpButton()
    {
        QDesktopServices::openUrl(QUrl(m_helpUrl));
    }

    void CardHeader::setHelpURL(QString url)
    {
        m_helpUrl = url;
        m_helpButton->setVisible(m_helpUrl.length() > 0);
    }

    void CardHeader::clearHelpURL()
    {
        m_helpUrl = "";
        m_helpButton->setVisible(false);
    }

    QString CardHeader::helpURL() const
    {
        return m_helpUrl;
    }

} // namespace AzQtComponents
