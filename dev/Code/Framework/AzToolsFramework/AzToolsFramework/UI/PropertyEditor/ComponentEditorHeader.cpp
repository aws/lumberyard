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
#include "StdAfx.h"
#include "ComponentEditorHeader.hxx"

#include <QDesktopServices>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>

namespace AzToolsFramework
{
    namespace HeaderBarConstants
    {
        static const int kIconSize = 16;

        // names for widgets so they can be found in stylesheet
        static const char* kBackgroundId = "Background";
        static const char* kExpanderId = "Expander";
        static const char* kIconId = "Icon";
        static const char* kWarningIconId = "WarningIcon";
        static const char* kTitleId = "Title";
        static const char* kContextMenuId = "ContextMenu";
        static const char* khelpButtonId = "Help";
    }

    ComponentEditorHeader::ComponentEditorHeader(QWidget* parent /*= nullptr*/)
        : QFrame(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_backgroundFrame = aznew QFrame(this);
        m_backgroundFrame->setObjectName(HeaderBarConstants::kBackgroundId);
        m_backgroundFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_backgroundFrame->setAutoFillBackground(true);

        // expander widget
        m_expanderButton = aznew QPushButton(m_backgroundFrame);
        m_expanderButton->setObjectName(HeaderBarConstants::kExpanderId);
        m_expanderButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_expanderButton->setCheckable(true);
        m_expanderButton->setChecked(true);
        m_expanderButton->show();
        connect(m_expanderButton, &QPushButton::toggled, this, &ComponentEditorHeader::OnExpanderChanged);

        // icon widget
        m_iconLabel = aznew QLabel(m_backgroundFrame);
        m_iconLabel->setObjectName(HeaderBarConstants::kIconId);
        m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_iconLabel->hide();

        // title widget
        m_titleLabel = aznew QLabel(m_backgroundFrame);
        m_titleLabel->setObjectName(HeaderBarConstants::kTitleId);
        m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_titleLabel->hide();

        // warning widget
        m_warningLabel = aznew QLabel(m_backgroundFrame);
        m_warningLabel->setObjectName(HeaderBarConstants::kWarningIconId);
        m_warningLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_warningLabel->hide();

        m_helpButton = aznew QPushButton(m_backgroundFrame);
        m_helpButton->setObjectName(HeaderBarConstants::khelpButtonId);
        m_helpButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_helpButton->hide();
        connect(m_helpButton, &QPushButton::clicked, this, &ComponentEditorHeader::TriggerHelpButton);


        // context menu widget
        m_contextMenuButton = aznew QPushButton(m_backgroundFrame);
        m_contextMenuButton->setObjectName(HeaderBarConstants::kContextMenuId);
        m_contextMenuButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_contextMenuButton->hide();
        connect(m_contextMenuButton, &QPushButton::clicked, this, &ComponentEditorHeader::TriggerContextMenuUnderButton);

        m_backgroundLayout = aznew QHBoxLayout(m_backgroundFrame);
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

        m_mainLayout = aznew QVBoxLayout(this);
        m_mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_mainLayout->setSpacing(0);
        m_mainLayout->setMargin(0);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->addWidget(m_backgroundFrame);
        setLayout(m_mainLayout);

        SetExpanded(true);
        SetWarning(false);
        SetReadOnly(false);
        UpdateStyleSheets();
    }

    void ComponentEditorHeader::SetTitle(const QString& title)
    {
        m_titleLabel->setText(title);
        m_titleLabel->setVisible(!title.isEmpty());
    }

    void ComponentEditorHeader::SetIcon(const QIcon& icon)
    {
        m_iconLabel->setPixmap(icon.pixmap(HeaderBarConstants::kIconSize, HeaderBarConstants::kIconSize));
        m_iconLabel->setVisible(!icon.isNull());
    }

    void ComponentEditorHeader::SetExpandable(bool expandable)
    {
        m_expanderButton->setEnabled(expandable);
    }

    bool ComponentEditorHeader::IsExpandable() const
    {
        return m_expanderButton->isEnabled();
    }

    void ComponentEditorHeader::SetExpanded(bool expanded)
    {
        m_expanderButton->setChecked(expanded);
    }

    bool ComponentEditorHeader::IsExpanded() const
    {
        return m_expanderButton->isChecked();
    }

    void ComponentEditorHeader::SetWarningIcon(const QIcon& icon)
    {
        m_warningLabel->setPixmap(icon.pixmap(HeaderBarConstants::kIconSize, HeaderBarConstants::kIconSize));
    }

    void ComponentEditorHeader::SetWarning(bool warning)
    {
        if (m_warning != warning)
        {
            m_warning = warning;
            m_warningLabel->setVisible(m_warning);
            UpdateStyleSheets();
        }
    }

    bool ComponentEditorHeader::IsWarning() const
    {
        return m_warning;
    }

    void ComponentEditorHeader::SetReadOnly(bool readOnly)
    {
        if (m_readOnly != readOnly)
        {
            m_readOnly = readOnly;
            UpdateStyleSheets();
        }
    }

    bool ComponentEditorHeader::IsReadOnly() const
    {
        return m_readOnly;
    }

    void ComponentEditorHeader::SetHasContextMenu(bool showContextMenu)
    {
        m_contextMenuButton->setVisible(showContextMenu);
    }

    void ComponentEditorHeader::mousePressEvent(QMouseEvent *event)
    {
        QFrame::mousePressEvent(event);
    }

    void ComponentEditorHeader::mouseDoubleClickEvent(QMouseEvent *event)
    {
        //allow double click to expand/contract
        if (event->button() == Qt::LeftButton && IsExpandable())
        {
            bool expand = !IsExpanded();
            SetExpanded(expand);
            emit OnExpanderChanged(expand);
        }
        QFrame::mouseDoubleClickEvent(event);
    }

    void ComponentEditorHeader::contextMenuEvent(QContextMenuEvent *event)
    {
        emit OnContextMenuClicked(event->globalPos());
        event->accept();
    }

    void ComponentEditorHeader::TriggerContextMenuUnderButton()
    {
        emit OnContextMenuClicked(mapToGlobal(m_contextMenuButton->pos() + QPoint(0, m_contextMenuButton->height())));
    }

    void ComponentEditorHeader::UpdateStyleSheets()
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

    void ComponentEditorHeader::TriggerHelpButton()
    {
        QString webLink = tr(m_helpUrl.c_str());
        QDesktopServices::openUrl(QUrl(webLink));
    }

    void ComponentEditorHeader::SetHelpURL(AZStd::string& url)
    {
        m_helpUrl = url;
        m_helpButton->setVisible(m_helpUrl.length() > 0);
    }

    void ComponentEditorHeader::ClearHelpURL()
    {
        m_helpUrl = "";
        m_helpButton->setVisible(false);
    }
}

#include <UI/PropertyEditor/ComponentEditorHeader.moc>