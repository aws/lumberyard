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

#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>
#include <AzQtComponents/Components/Style.h>

#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QSettings>
#include <QMenu>
#include <QResizeEvent>

namespace AzQtComponents
{
    const QChar g_separator = '/';
    const QChar g_windowsSeparator = '\\';
    const QString g_labelName = QStringLiteral("BreadCrumbLabel");
    const QString g_TruncationLabel = "...";

    BreadCrumbs::BreadCrumbs(QWidget* parent)
        : QWidget(parent)
        , m_config(defaultConfig())
    {
        // create the layout
        QHBoxLayout* boxLayout = new QHBoxLayout(this);
        boxLayout->setContentsMargins(0, 0, 0, 0);

        // create the label
        m_label = new QLabel(this);
        m_label->setObjectName(g_labelName);
        boxLayout->addWidget(m_label);
        m_label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        connect(m_label, &QLabel::linkActivated, this, &BreadCrumbs::onLinkActivated);
    }

    BreadCrumbs::~BreadCrumbs()
    {
    }

    bool BreadCrumbs::isBackAvailable() const
    {
        return !m_backPaths.isEmpty();
    }

    bool BreadCrumbs::isForwardAvailable() const
    {
        return !m_forwardPaths.isEmpty();
    }

    bool BreadCrumbs::isUpAvailable() const
    {
        int separatorIndex = m_currentPath.lastIndexOf(g_separator);
        return (separatorIndex > 0);
    }

    QString BreadCrumbs::currentPath() const
    {
        return m_currentPath;
    }

    QToolButton* BreadCrumbs::createButton(NavigationButton type)
    {
        QToolButton* button = new QToolButton(this);

        switch (type)
        {
        case NavigationButton::Back:
            button->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
            button->setEnabled(isBackAvailable());
            connect(button, &QToolButton::released, this, &BreadCrumbs::back);
            connect(this, &BreadCrumbs::backAvailabilityChanged, button, &QWidget::setEnabled);
            break;

        case NavigationButton::Forward:
            button->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
            button->setEnabled(isForwardAvailable());
            connect(button, &QToolButton::released, this, &BreadCrumbs::forward);
            connect(this, &BreadCrumbs::forwardAvailabilityChanged, button, &QWidget::setEnabled);
            break;

        case NavigationButton::Up:
            button->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
            button->setEnabled(isUpAvailable());
            connect(button, &QToolButton::released, this, &BreadCrumbs::up);
            connect(this, &BreadCrumbs::upAvailabilityChanged, button, &QWidget::setEnabled);
            break;

        default:
            Q_UNREACHABLE();
            break;
        }

        return button;
    }

    void BreadCrumbs::pushPath(const QString& fullPath)
    {
        BreadCrumbButtonStates buttonStates;
        getButtonStates(buttonStates);

        if (!m_currentPath.isEmpty())
        {
            m_backPaths.push(m_currentPath);
        }

        m_forwardPaths.clear();

        changePath(fullPath);

        emitButtonSignals(buttonStates);
    }

    bool BreadCrumbs::back()
    {
        if (!isBackAvailable())
        {
            return false;
        }

        BreadCrumbButtonStates buttonStates;
        getButtonStates(buttonStates);

        m_forwardPaths.push(m_currentPath);

        QString newPath = m_backPaths.pop();

        changePath(newPath);

        emitButtonSignals(buttonStates);

        return true;
    }

    bool BreadCrumbs::forward()
    {
        if (!isForwardAvailable())
        {
            return false;
        }

        BreadCrumbButtonStates buttonStates;
        getButtonStates(buttonStates);

        m_backPaths.push(m_currentPath);

        QString newPath = m_forwardPaths.pop();

        changePath(newPath);

        emitButtonSignals(buttonStates);

        return true;
    }

    bool BreadCrumbs::up()
    {
        // split out the current path
        int separatorIndex = m_currentPath.lastIndexOf(g_separator);
        if (separatorIndex <= 0)
        {
            return false;
        }

        // don't have to worry about handling signals because pushPath will do it

        pushPath(m_currentPath.left(separatorIndex));

        return true;
    }

    void BreadCrumbs::resizeEvent(QResizeEvent* event)
    {
        if (event->oldSize().width() != event->size().width())
        {
            fillLabel();
        }
        QWidget::resizeEvent(event);
    }

    void BreadCrumbs::fillLabel()
    {
        QString htmlString = "";
        const QStringList fullPath = m_currentPath.split(g_separator, QString::SkipEmptyParts);
        m_truncatedPaths = fullPath;

        // used to measure the width used by the path
        const int availableWidth = width() * m_config.optimalPathWidth;
        const QFontMetricsF fm(m_label->font());
        QString plainTextPath = "";

        auto formatLink = [this](const QString& fullPath, const QString& shortPath) -> QString {
            return QString("<a href=\"%1\" style=\"color: %2\">%3</a>").arg(fullPath, m_config.linkColor, shortPath);
        };

        const QString nonBreakingSpace = QStringLiteral("&nbsp;");
        auto prependSeparators = [&]() {
            htmlString.prepend(QStringLiteral("%1%1>%1%1").arg(nonBreakingSpace));
        };

        // last section is not clickable
        plainTextPath = m_truncatedPaths.takeLast();
        htmlString.prepend(plainTextPath);

        while (!m_truncatedPaths.isEmpty())
        {
            prependSeparators();
            plainTextPath.append(g_separator + m_truncatedPaths.last());

            if (fm.width(plainTextPath) > availableWidth)
            {
                htmlString.prepend(QString("%1").arg(formatLink(g_TruncationLabel, g_TruncationLabel)));
                break;
            }

            const QString linkPath = buildPathFromList(fullPath, m_truncatedPaths.size());
            const QString& part = m_truncatedPaths.takeLast();
            htmlString.prepend(QString("%1").arg(formatLink(linkPath, part)));
        }

        m_label->setText(htmlString);
    }

    void BreadCrumbs::changePath(const QString& newPath)
    {
        m_currentPath = newPath;

        // clean up the path to use all the first separator in the list of separators
        m_currentPath = m_currentPath.replace(g_windowsSeparator, g_separator);

        fillLabel();

        Q_EMIT pathChanged(m_currentPath);
    }

    void BreadCrumbs::getButtonStates(BreadCrumbButtonStates buttonStates)
    {
        buttonStates[EnumToConstExprInt(NavigationButton::Back)] = isBackAvailable();
        buttonStates[EnumToConstExprInt(NavigationButton::Forward)] = isForwardAvailable();
        buttonStates[EnumToConstExprInt(NavigationButton::Up)] = isUpAvailable();
    }

    void BreadCrumbs::emitButtonSignals(BreadCrumbButtonStates previousButtonStates)
    {
        if (isBackAvailable() != previousButtonStates[EnumToConstExprInt(NavigationButton::Back)])
        {
            Q_EMIT backAvailabilityChanged(isBackAvailable());
        }

        if (isForwardAvailable() != previousButtonStates[EnumToConstExprInt(NavigationButton::Forward)])
        {
            Q_EMIT forwardAvailabilityChanged(isForwardAvailable());
        }

        if (isUpAvailable() != previousButtonStates[EnumToConstExprInt(NavigationButton::Up)])
        {
            Q_EMIT upAvailabilityChanged(isUpAvailable());
        }
    }

    BreadCrumbs::Config BreadCrumbs::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        const QString linkColorKey = QStringLiteral("LinkColor");
        if (settings.contains(linkColorKey))
        {
            config.linkColor = settings.value(linkColorKey).toString();
        }

        const QString optimalPathWidthKey = QStringLiteral("OptimalPathWidth");
        if (settings.contains(optimalPathWidthKey))
        {
            config.optimalPathWidth = settings.value(optimalPathWidthKey).toFloat();
        }

        return config;
    }

    BreadCrumbs::Config BreadCrumbs::defaultConfig()
    {
        Config config;

        config.linkColor = QStringLiteral("white");
        config.optimalPathWidth = 0.8;

        return config;
    }

    bool BreadCrumbs::polish(Style* style, QWidget* widget, const Config& config)
    {
        BreadCrumbs* breadCrumbs = qobject_cast<BreadCrumbs*>(widget);
        if (breadCrumbs != nullptr)
        {
            breadCrumbs->m_config = config;
            breadCrumbs->fillLabel();

            style->repolishOnSettingsChange(breadCrumbs);

            return true;
        }

        return false;
    }

    void BreadCrumbs::onLinkActivated(const QString& link)
    {
        if (link == g_TruncationLabel)
        {
            QMenu hiddenPaths;
            for (int i = m_truncatedPaths.size() - 1; i >= 0; i--)
            {
                hiddenPaths.addAction(m_truncatedPaths.at(i), [this, i]() {
                    Q_EMIT pathChanged(buildPathFromList(m_truncatedPaths, i + 1));
                });
            }

            hiddenPaths.exec(QCursor::pos());
        }
        else
        {
            Q_EMIT pathChanged(link);
        }
    }

    QString BreadCrumbs::buildPathFromList(const QStringList& fullPath, int pos)
    {
        if (fullPath.size() < pos)
        {
            return QString();
        }

        QString path;
        for (int i = 0; i < pos; i++)
        {
            path.append(fullPath.value(i) + g_separator);
        }

        return path;
    }
} // namespace AzQtComponents
