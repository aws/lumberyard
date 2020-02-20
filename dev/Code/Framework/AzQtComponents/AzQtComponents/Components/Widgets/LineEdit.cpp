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

#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QLineEdit>
#include <QAction>
#include <QSettings>
#include <QDynamicPropertyChangeEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QPainter>
#include <QStyleOption>
#include <QDebug>
#include <QToolButton>
#include <QTimer>

namespace AzQtComponents
{
    class LineEditWatcher : public QObject
    {
    public:
        explicit LineEditWatcher(QObject* parent = nullptr)
            : QObject(parent)
        {
        }

        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (auto le = qobject_cast<QLineEdit*>(watched))
            {
                switch (event->type())
                {
                    case QEvent::DynamicPropertyChange:
                    {
                        le->update();
                    }
                    break;

                    case QEvent::FocusIn:
                    case QEvent::FocusOut:
                    {
                        updateClearButtonState(le);
                    }
                    break;

                    case QEvent::ToolTip:
                    {
                        if (le->property(HasError).toBool())
                        {
                            const auto he = static_cast<QHelpEvent*>(event);
                            QString message = le->property(ErrorMessage).toString();
                            if (message.isEmpty())
                            {
                                message = QLineEdit::tr("Invalid input");
                            }
                            QToolTip::showText(he->globalPos(),
                                               message,
                                               le,
                                               QRect(),
                                               1000 *10);
                            return true;
                        }
                    }
                    break;

                    case QEvent::KeyPress:
                    {
                        const auto ke = static_cast<QKeyEvent*>(event);

                        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)
                        {
                            updateErrorState(le);
                        }
                    }
                    break;

                    case QEvent::LayoutDirectionChange:
                    case QEvent::Resize:
                    {
                        const auto filtered = QObject::eventFilter(watched, event);
                        if (!filtered)
                        {
                            // Deliver the event
                            le->event(event);
                            // Set our own side widget positions
                            positionSideWidgets(le);
                        }
                        return true;
                    }
                }
            }

            return QObject::eventFilter(watched, event);
        }

    private:
        friend class LineEdit;
        LineEdit::Config m_config;

        bool isError(QLineEdit* le) const
        {
            return !le->hasAcceptableInput();
        }

        bool isClearButtonNeeded(QLineEdit* le) const
        {
            return !le->text().isEmpty();
        }

        void updateErrorState(QLineEdit* le)
        {
            const bool error = isError(le);

            if (error != le->property(HasError).toBool())
            {
                le->setProperty(HasError, error);
            }
            if (auto errorToolButton = le->findChild<QToolButton*>(ErrorToolButton))
            {
                errorToolButton->setVisible(error);
                positionSideWidgets(le);
            }
        }

        void updateClearButtonState(QLineEdit* le)
        {
            const bool clear = isClearButtonNeeded(le);

            // We don't want to impose the clear button visibility if user did not called
            // lineEdit->setClearButtonEnabled on its own.
            // So let get the clear action if available to update it only.
            QAction* clearAction = le->findChild<QAction*>(ClearAction);
            if (clearAction)
            {
                clearAction->setVisible(clear);
                positionSideWidgets(le);
            }
        }

        void updateErrorStateSlot()
        {
            if (auto le = qobject_cast<QLineEdit*>(sender()))
            {
                updateErrorState(le);
                positionSideWidgets(le);
            }
        }

        void updateClearButtonStateSlot()
        {
            if (auto le = qobject_cast<QLineEdit*>(sender()))
            {
                updateClearButtonState(le);
            }
        }

        void storeClearButtonState(QLineEdit* lineEdit)
        {
            if (!lineEdit || hasClearButtonState(lineEdit))
            {
                return;
            }

            lineEdit->setProperty(StoredClearButtonState, lineEdit->isClearButtonEnabled());
        }

        void restoreClearButtonState(QLineEdit* lineEdit)
        {
            if (!lineEdit || !hasClearButtonState(lineEdit))
            {
                return;
            }

            // Delay property change until the next event loop iteration. This prevents
            // widgets from being destroyed (and pointers invalidated) whilst the app is
            // being repolished.
            QTimer::singleShot(0, lineEdit, [lineEdit] {
                const auto clearButtonState = lineEdit->property(StoredClearButtonState);
                lineEdit->setClearButtonEnabled(clearButtonState.toBool());
            });
        }

        bool hasClearButtonState(QLineEdit* lineEdit)
        {
            if (!lineEdit)
            {
                return false;
            }

            const auto clearButtonState = lineEdit->property(StoredClearButtonState);
            return clearButtonState.isValid() && clearButtonState.canConvert<bool>();
        }

        void storeHoverAttributeState(QLineEdit* lineEdit)
        {
            if (!lineEdit || hasHoverAttributeState(lineEdit))
            {
                return;
            }

            lineEdit->setProperty(StoredHoverAttributeState, lineEdit->testAttribute(Qt::WA_Hover));
        }

        void restoreHoverAttributeState(QLineEdit* lineEdit)
        {
            if (!lineEdit || !hasClearButtonState(lineEdit))
            {
                return;
            }

            // Delay property change until the next event loop iteration. This prevents
            // widgets from being destroyed (and pointers invalidated) whilst the app is
            // being repolished.
            QTimer::singleShot(0, lineEdit, [lineEdit] {
                const auto hovereState = lineEdit->property(StoredHoverAttributeState);
                lineEdit->setAttribute(Qt::WA_Hover, hovereState.toBool());
            });
        }

        bool hasHoverAttributeState(QLineEdit* lineEdit)
        {
            if (!lineEdit)
            {
                return false;
            }

            const auto hoverState = lineEdit->property(StoredClearButtonState);
            return hoverState.isValid() && hoverState.canConvert<bool>();
        }

        void positionSideWidgets(QLineEdit* lineEdit)
        {
            const auto contentsRect = lineEdit->rect();
            int delta = m_config.iconMargin;

            auto clearToolButton = lineEdit->findChild<QToolButton*>(ClearToolButton);
            auto errorToolButton = lineEdit->findChild<QToolButton*>(ErrorToolButton);
            const QList<QToolButton*> buttons = {clearToolButton, errorToolButton};

            for (auto button : buttons)
            {
                if (button)
                {
                    QRect geometry = button->geometry();
                    geometry.moveRight(contentsRect.right() - delta);
                    button->setGeometry(geometry);
                    delta += geometry.width() + m_config.iconSpacing;
                }
            }
        }
    };

    QPointer<LineEditWatcher> LineEdit::s_lineEditWatcher = nullptr;
    unsigned int LineEdit::s_watcherReferenceCount = 0;

    int LineEdit::Config::getLineWidth(const QStyleOption* option, bool hasError) const
    {
        const QStyleOptionFrame* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frameOption)
        {
            return 0;
        }

        const bool isFocused = option->state.testFlag(QStyle::State_HasFocus);
        const bool isHovered = option->state.testFlag(QStyle::State_MouseOver) && option->state.testFlag(QStyle::State_Enabled);

        return isFocused
                ? focusedLineWidth
                : (hasError
                    ? errorLineWidth
                    : (isHovered
                        ? hoverLineWidth
                        : 0));
    }

    QColor LineEdit::Config::getBorderColor(const QStyleOption* option, bool hasError) const
    {
        const QStyleOptionFrame* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frameOption)
        {
            return {};
        }

        const bool isFocused = option->state.testFlag(QStyle::State_HasFocus);
        const bool isHovered = option->state.testFlag(QStyle::State_MouseOver) && option->state.testFlag(QStyle::State_Enabled);

        return isFocused
                ? focusedBorderColor
                : (hasError
                    ? errorBorderColor
                    : (isHovered
                        ? hoverBorderColor
                        :(borderColor.isValid()
                            ? borderColor
                            : Qt::transparent)));
    }

    QColor LineEdit::Config::getBackgroundColor(const QStyleOption* option, bool hasError, const QWidget* widget) const
    {
        Q_UNUSED(hasError);
        const QStyleOptionFrame* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        if (!frameOption)
        {
            return {};
        }

        const bool isFocused = option->state.testFlag(QStyle::State_HasFocus);
        return isFocused ? hoverBackgroundColor : option->palette.color(widget->backgroundRole());
    }

    void LineEdit::applySearchStyle(QLineEdit* lineEdit)
    {
        if (lineEdit->property(HasSearchAction).toBool())
        {
            return;
        }

        lineEdit->setProperty(HasSearchAction, true);

        auto searchToolButton = lineEdit->findChild<QToolButton*>(SearchToolButton);
        if (!searchToolButton)
        {
            // Adding a QAction to a QLineEdit results in a private QToolButton derived widget being
            // added as a child of the QLineEdit.
            // We cannot simply call QWidget::removeAction in LineEdit::removeSearchStyle because
            // this results in a crash when LineEdit::removeSearchStyle is called during
            // QApplication::setStyle. QWidget::removeAction deletes the private widget and then
            // QApplication attempts to unpolish the deleted private widget.
            // To work around this we find the private widget after it has been added and set it's
            // object name so we can find it again in LineEdit::removeSearchStyle.
            auto searchAction = lineEdit->addAction(QIcon(":/stylesheet/img/search-dark.svg"), QLineEdit::LeadingPosition);
            searchAction->setEnabled(false);

            auto childToolButtons = lineEdit->findChildren<QToolButton*>();
            for (auto toolButton : childToolButtons)
            {
                if (toolButton->defaultAction() == searchAction)
                {
                    toolButton->setObjectName(SearchToolButton);
                    break;
                }
            }
            return;
        }

        searchToolButton->show();
    }

    void LineEdit::removeSearchStyle(QLineEdit* lineEdit)
    {
        if (!lineEdit->property(HasSearchAction).toBool())
        {
            return;
        }

        lineEdit->setProperty(HasSearchAction, false);

        auto searchToolButton = lineEdit->findChild<QToolButton*>(SearchToolButton);
        searchToolButton->hide();
    }

    LineEdit::Config LineEdit::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

         // Keep radius in sync with css, it is not possible to get it back from the style option :/
        ConfigHelpers::read<int>(settings, QStringLiteral("BorderRadius"), config.borderRadius);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("BorderColor"), config.borderColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("HoverBackgroundColor"), config.hoverBackgroundColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("HoverBorderColor"), config.hoverBorderColor);
        ConfigHelpers::read<int>(settings, QStringLiteral("HoverLineWidth"), config.hoverLineWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("FocusedBorderColor"), config.focusedBorderColor);
        ConfigHelpers::read<int>(settings, QStringLiteral("FocusedLineWidth"), config.focusedLineWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("ErrorBorderColor"), config.errorBorderColor);
        ConfigHelpers::read<int>(settings, QStringLiteral("ErrorLineWidth"), config.errorLineWidth);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("PlaceHolderTextColor"), config.placeHolderTextColor);
        ConfigHelpers::read<bool>(settings, QStringLiteral("ClearButtonAutoEnabled"), config.clearButtonAutoEnabled);
        ConfigHelpers::read<QString>(settings, QStringLiteral("ClearImage"), config.clearImage);
        ConfigHelpers::read<QSize>(settings, QStringLiteral("ClearImageSize"), config.clearImageSize);
        ConfigHelpers::read<QString>(settings, QStringLiteral("ErrorImage"), config.errorImage);
        ConfigHelpers::read<QSize>(settings, QStringLiteral("ErrorImageSize"), config.errorImageSize);
        ConfigHelpers::read<int>(settings, QStringLiteral("IconSpacing"), config.iconSpacing);
        ConfigHelpers::read<int>(settings, QStringLiteral("IconMargin"), config.iconMargin);

        return config;
    }

    LineEdit::Config LineEdit::defaultConfig()
    {
        Config config;

        // Keep radius in sync with css, it is not possible to get it back from the style option :/
        config.borderRadius = 2;
        // An invalid color will take current background color
        config.borderColor = QColor(Qt::transparent);
        config.hoverBackgroundColor = QColor("#FFFFFF");
        config.hoverBorderColor = QColor("#222222");
        config.hoverLineWidth = 1;
        config.focusedBorderColor = QColor("#00A1C9");
        config.focusedLineWidth = 1;
        config.errorBorderColor = QColor("#E25243");
        config.errorLineWidth = 2;
        config.placeHolderTextColor = QColor("#888888");
        config.clearButtonAutoEnabled = true;
        config.clearImage = QStringLiteral(":/stylesheet/img/UI20/lineedit-close.svg");
        config.clearImageSize = {14, 14};
        config.errorImage = QStringLiteral(":/stylesheet/img/UI20/lineedit-error.svg");
        config.errorImageSize = {14, 14};
        config.iconSpacing = 0; // Due to 2px padding in icons, use 0 here to give 4px spacing between images
        config.iconMargin = 2;

        return config;
    }

    void LineEdit::setErrorMessage(QLineEdit* lineEdit, const QString& error)
    {
        lineEdit->setProperty(ErrorMessage, error);
    }

    void LineEdit::setSideButtonsEnabled(QLineEdit* lineEdit, bool enabled)
    {
        lineEdit->setProperty(SideButtonsEnabled, enabled);
    }

    void LineEdit::initializeWatcher()
    {
        if (!s_lineEditWatcher)
        {
            Q_ASSERT(s_watcherReferenceCount == 0);
            s_lineEditWatcher = new LineEditWatcher;
        }

        ++s_watcherReferenceCount;
    }

    void LineEdit::uninitializeWatcher()
    {
        Q_ASSERT(!s_lineEditWatcher.isNull());
        Q_ASSERT(s_watcherReferenceCount > 0);

        --s_watcherReferenceCount;

        if (s_watcherReferenceCount == 0)
        {
            delete s_lineEditWatcher;
            s_lineEditWatcher = nullptr;
        }
    }

    bool LineEdit::polish(Style* style, QWidget* widget, const LineEdit::Config& config)
    {
        Q_ASSERT(!s_lineEditWatcher.isNull());

        auto lineEdit = qobject_cast<QLineEdit*>(widget);

        if (lineEdit)
        {
            QObject::connect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::connect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateErrorStateSlot);
            QObject::connect(lineEdit, &QLineEdit::returnPressed, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::connect(lineEdit, &QLineEdit::editingFinished, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            QPalette pal = lineEdit->palette();
#if !defined(AZ_PLATFORM_LINUX)
            pal.setColor(QPalette::PlaceholderText, config.placeHolderTextColor);
#endif // !defined(AZ_PLATFORM_LINUX)

            s_lineEditWatcher->storeHoverAttributeState(lineEdit);
            lineEdit->setAttribute(Qt::WA_Hover, true);
            s_lineEditWatcher->storeClearButtonState(lineEdit);
            if (LineEdit::sideButtonsEnabled(lineEdit))
            {
                LineEdit::applyClearButtonStyle(lineEdit, config);
                LineEdit::applyErrorStyle(lineEdit, config);
                s_lineEditWatcher->positionSideWidgets(lineEdit);
            }

            lineEdit->setPalette(pal);
            lineEdit->installEventFilter(s_lineEditWatcher);

            // initial state checks
            s_lineEditWatcher->updateErrorState(lineEdit);
            s_lineEditWatcher->updateClearButtonState(lineEdit);

            style->repolishOnSettingsChange(lineEdit);
            s_lineEditWatcher->m_config = config;
        }

        return lineEdit;
    }

    bool LineEdit::unpolish(Style* style, QWidget* widget, const LineEdit::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        Q_ASSERT(!s_lineEditWatcher.isNull());

        auto lineEdit = qobject_cast<QLineEdit*>(widget);

        if (lineEdit)
        {
            QObject::disconnect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::textChanged, s_lineEditWatcher, &LineEditWatcher::updateErrorStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::returnPressed, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::editingFinished, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            lineEdit->removeEventFilter(s_lineEditWatcher);
            s_lineEditWatcher->restoreClearButtonState(lineEdit);
            s_lineEditWatcher->restoreHoverAttributeState(lineEdit);
        }

        return lineEdit;
    }

    bool LineEdit::drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const LineEdit::Config& config)
    {
        auto lineEdit = qobject_cast<const QLineEdit*>(widget);
        if (!lineEdit)
        {
            return false;
        }

        if (lineEdit->hasFrame())
        {
            const bool hasError = lineEdit->property(HasError).toBool();
            const int lineWidth = config.getLineWidth(option, hasError);
            const QColor backgroundColor = config.getBackgroundColor(option, hasError, lineEdit);

            if (lineWidth > 0)
            {
                const QColor frameColor = config.getBorderColor(option, hasError);
                const auto borderRect = style->borderLineEditRect(lineEdit->rect(), lineWidth, config.borderRadius);
                Style::drawFrame(painter, borderRect, Qt::NoPen, frameColor);
            }

            const auto frameRect = style->lineEditRect(lineEdit->rect(), config.borderRadius, config.borderRadius);
            Style::drawFrame(painter, frameRect, Qt::NoPen, backgroundColor);
        }
        else
        {
            return false;
        }

        return true;
    }

    void LineEdit::applyClearButtonStyle(QLineEdit* lineEdit, const Config& config)
    {
        if (!config.clearButtonAutoEnabled)
        {
            return;
        }

        lineEdit->setClearButtonEnabled(config.clearButtonAutoEnabled);

        auto clearToolButton = lineEdit->findChild<QToolButton*>(ClearToolButton);
        if (!clearToolButton)
        {
            QAction* clearAction = lineEdit->findChild<QAction*>(ClearAction);
            if (!clearAction)
            {
                return;
            }

            auto childToolButtons = lineEdit->findChildren<QToolButton*>();
            for (auto toolButton : childToolButtons)
            {
                if (toolButton->defaultAction() == clearAction)
                {
                    toolButton->setObjectName(ClearToolButton);
                    break;
                }
            }
        }
    }

    void LineEdit::applyErrorStyle(QLineEdit* lineEdit, const Config& config)
    {
        auto errorToolButton = lineEdit->findChild<QToolButton*>(ErrorToolButton);
        if (!errorToolButton)
        {
            QIcon icon;
            icon.addFile(config.errorImage, config.errorImageSize);
            auto errorAction = lineEdit->addAction(icon, QLineEdit::TrailingPosition);

            auto childToolButtons = lineEdit->findChildren<QToolButton*>();
            for (auto toolButton : childToolButtons)
            {
                if (toolButton->defaultAction() == errorAction)
                {
                    toolButton->setObjectName(ErrorToolButton);
                    break;
                }
            }
        }
    }

    QIcon LineEdit::clearButtonIcon(const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        QIcon icon;
        icon.addFile(config.clearImage, config.clearImageSize, QIcon::Normal);
        icon.addFile(config.clearImage, config.clearImageSize, QIcon::Disabled);
        return icon;
    }

    bool LineEdit::sideButtonsEnabled(QLineEdit* lineEdit)
    {
        const auto enabled = lineEdit->property(SideButtonsEnabled);
        return enabled.isValid() ? enabled.toBool() : true;
    }
} // namespace AzQtComponents

#include <Components/Widgets/LineEdit.moc>
