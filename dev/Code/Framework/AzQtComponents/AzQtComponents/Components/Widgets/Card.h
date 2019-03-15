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
#include <QPoint>
#include <QVector>
#include <QIcon>
#include <QMargins>
#include <QString>

class QPoint;
class QVBoxLayout;
class QBoxLayout;
class QSettings;

namespace AzQtComponents
{
    class CardHeader;
    class CardNotification;
    class Style;

    /**
     * Card widget - provides a title/header bar that can be expanded/collapsed, dragged around,
     * and contains another widget, when expanded.
     *
     * To make use of a Card, set its title and set a content widget. The content widget will be
     * automatically shown and hidden as the Card is expanded and collapsed.
     *
     * Beyond that, a Card can also include a secondary expandable widget (for something such as
     * Advanced Settings) and multiple "Notifications" which are labels appended to the bottom
     * that can optionally include "feature" widgets (such as a button to allow the user to
     * respond to the Notification).
     *
     * Style information is stored in Card.qss, for Card, CardHeader and CardNotification objects.
     * Config information is stored in CardConfig.ini
     */
    class AZ_QT_COMPONENTS_API Card
        : public QFrame
    {
        Q_OBJECT //AUTOMOC

    public:
        struct Config
        {
            int toolTipPaddingInPixels;
            int headerIconSizeInPixels;
        };

        Card(QWidget* parent = nullptr);

        void setContentWidget(QWidget* contentWidget);
        QWidget* contentWidget() const;

        void setTitle(const QString& title);
        QString title() const;

        CardHeader* header() const;

        CardNotification* addNotification(QString message);
        void clearNotifications();

        void setExpanded(bool expand);
        bool isExpanded() const;

        void setSelected(bool selected);
        bool isSelected() const;

        void setDragging(bool dragging);
        bool isDragging() const;

        void setDropTarget(bool dropTarget);
        bool isDropTarget() const;

        void setSecondaryContentExpanded(bool expand);
        bool isSecondaryContentExpanded() const;

        void setSecondaryTitle(const QString& secondaryTitle);
        QString secondaryTitle() const;

        void setSecondaryContentWidget(QWidget* secondaryContentWidget);
        QWidget* secondaryContentWidget() const;

        void hideFrame();

        /*!
        * Loads the Card config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default Card config data.
        */
        static Config defaultConfig();

    Q_SIGNALS:
        void expandStateChanged(bool expanded);
        void secondaryContentExpandStateChanged(bool expanded);

        void contextMenuRequested(const QPoint& position);

    protected:
        // implemented this way so that AzToolsFramework::ComponentEditorHeader can still be used
        Card(CardHeader* customHeader, QWidget* parent = nullptr);

    private:
        bool hasSecondaryContent() const;
        void updateSecondaryContentVisibility();

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);

        QWidget* m_contentWidget = nullptr;
        QWidget* m_secondaryContentWidget = nullptr;
        QFrame* m_separator = nullptr;
        QFrame* m_separatorContainer = nullptr;
        CardHeader* m_header = nullptr;
        CardHeader* m_secondaryHeader = nullptr;
        QVBoxLayout* m_mainLayout = nullptr;
        bool m_selected = false;
        bool m_dragging = false;
        bool m_dropTarget = false;
        QVector<CardNotification*> m_notifications;
        QIcon m_warningIcon;
        QMargins m_separatorMargins;

        friend class Style;
    };
} // namespace AzQtComponents
