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
#include <QIcon>
#include <QString>

class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QLabel;

namespace AzQtComponents
{
    /**
    * Header bar for Card widgets.
    *
    * Basically an expander arrow with a text title and a button to trigger a content menu.
    * Also has an optional icon and optional help button. 
    *
    * The sub widgets are hidden by default and will show once they're configured
    * via the appropriate setter (ex: setIcon causes the icon widget to appear).
    */
    class AZ_QT_COMPONENTS_API CardHeader
        : public QFrame
    {
        Q_OBJECT //AUTOMOC

    public:
        CardHeader(QWidget* parent = nullptr);

        /// Set a title. Passing an empty string will hide the widget.
        void setTitle(const QString& title);
        void refreshTitle();
        void setTitleProperty(const char *name, const QVariant &value);
        QString title() const;
        QLabel* titleLabel() const;

        /// Set an icon. Passing a null icon will hide the icon.
        void setIcon(const QIcon& icon);

        /// Set whether the header has an expand/contract button.
        /// Note that the header itself will not change size or hide, it
        /// simply causes the onExpanderChanged signal to fire.
        void setExpandable(bool expandable);
        bool isExpandable() const;

        void setExpanded(bool expanded);
        bool isExpanded() const;

        void setWarningIcon(const QIcon& icon);

        void setWarning(bool warning);
        bool isWarning() const;

        void setReadOnly(bool readOnly);
        bool isReadOnly() const;

        /// Set whether the header has a context menu widget - determines whether or not the contextMenuRequested signal fires on right-mouse-button click / when the context menu button is pressed.
        void setHasContextMenu(bool showContextMenu);

        void setHelpURL(QString url);
        void clearHelpURL();
        QString helpURL() const;

        void configSettingsChanged();

        static void setIconSize(int iconSize);
        static int defaultIconSize();

    Q_SIGNALS:
        void contextMenuRequested(const QPoint& position);
        void expanderChanged(bool expanded);

    protected:
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void triggerContextMenuUnderButton();
        void updateStyleSheets();
        void triggerHelpButton();

        // Widgets in header
        QVBoxLayout* m_mainLayout = nullptr;
        QHBoxLayout* m_backgroundLayout = nullptr;
        QFrame* m_backgroundFrame = nullptr;
        QPushButton* m_expanderButton = nullptr;
        QLabel* m_iconLabel = nullptr;
        QLabel* m_titleLabel = nullptr;
        QLabel* m_warningLabel = nullptr;
        QPushButton* m_contextMenuButton = nullptr;
        QPushButton* m_helpButton = nullptr;
        bool m_warning = false;
        bool m_readOnly = false;

        QIcon m_warningIcon;
        QIcon m_icon;
        QString m_helpUrl;

        static int s_iconSize;
    };
} // namespace AzQtComponents
