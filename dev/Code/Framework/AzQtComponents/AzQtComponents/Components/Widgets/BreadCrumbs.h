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
#include <QStyle>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QStack>

#include <AzCore/std/typetraits/underlying_type.h>

/**
* AUTOMOC
*/


class QSettings;
class QToolButton;
class QLabel;

namespace AzQtComponents
{
    class Style;

    template <typename E>
    constexpr auto EnumToConstExprInt(E e) noexcept
    {
        return static_cast<typename AZStd::underlying_type<E>::type>(e);
    }

    enum class NavigationButton
    {
        Back,
        Forward,
        Up,

        Count
    };

    using BreadCrumbButtonStates = bool[EnumToConstExprInt(NavigationButton::Count)];

    /**
     * Specialized widget to create html-style clickable links for each of the parts of a path, and provide state management
     * and signals for handling common path related functionality (back, forward and up).
     *
     * Can be styled by modifying BreadCrumbs.qss and BreadCrumbsConfig.ini.
     *
     * Note that Qt doesn't handle HTML embedded in QLabel widgets gracefully - stylesheets are not shared between those
     * and Qt objects. As a result, the color of the HTML styled links in the BreadCrumbs is specified in BreadCrumbsConfig.ini
     */
    class AZ_QT_COMPONENTS_API BreadCrumbs
        : public QWidget
    {
        Q_OBJECT

    public:

        struct Config
        {
            QString linkColor; // this is a color, but loaded as a string as it will be inserted into an HTML/css style spec
            float optimalPathWidth; // this is the portion of the total width that should be considered for display the path
        };

        BreadCrumbs(QWidget* parent = nullptr);
        ~BreadCrumbs();

        bool isBackAvailable() const;
        bool isForwardAvailable() const;
        bool isUpAvailable() const;

        QString currentPath() const;

        QToolButton* createButton(NavigationButton type);

        /*!
         * Loads the button config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default button config data.
         */
        static Config defaultConfig();

    public Q_SLOTS:
        /*!
         * Pushes a path. The current path, if one is set, will be pushed to the top of the back stack and the forward stack will be cleared.
         */
        void pushPath(const QString& fullPath);

        /*!
         * Restores the path on the top of the back stack and pushes the current path onto the forward stack.
         */
        bool back();

        /*!
         * Restores the path on the top of the forward stack and pushes the current path onto the back stack.
         */
        bool forward();

        /*!
         * Moves one segment up the current path. Pushes the new path onto the back stack and clears the forward stack.
         */
        bool up();

    Q_SIGNALS:

        /*!
         * Emitted any time the currently displayed path changes (via any of the above slots).
         */
        void pathChanged(const QString& fullPath);

        /*!
         * Indicates that a change to the back stack has occurred. Can be used to update back buttons.
         */
        void backAvailabilityChanged(bool enabled);

        /*!
         * Indicates that a change to the forward stack has occurred. Can be used to update forward buttons.
         */
        void forwardAvailabilityChanged(bool enabled);

        /*!
         * Indicates that a change to the current path has occurred which affects whether or not the path is a root path. Can be used to update up buttons.
         */
        void upAvailabilityChanged(bool enabled);

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private Q_SLOTS:
        void onLinkActivated(const QString& link);

    private:
        void fillLabel();
        void changePath(const QString& newPath);

        void getButtonStates(BreadCrumbButtonStates buttonStates);
        void emitButtonSignals(BreadCrumbButtonStates previousButtonStates);

        static QString buildPathFromList(const QStringList& fullPath, int pos);

        QLabel* m_label = nullptr;

        QString m_currentPath;
        QStack<QString> m_backPaths;
        QStack<QString> m_forwardPaths;
        Config m_config;
        QStringList m_truncatedPaths;

        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents
