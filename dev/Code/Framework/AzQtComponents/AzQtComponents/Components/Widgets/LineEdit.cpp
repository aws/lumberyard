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

namespace AzQtComponents
{
    constexpr const char* HasSearchAction = "HasSearchAction";
    constexpr const char* HasError = "HasError";
    constexpr const char* SearchToolButton = "SearchToolButton";

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
                            QToolTip::showText(he->globalPos(),
                                               QLineEdit::tr("Invalid input"),
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
            return le->text().length() > 0 && le->hasFocus();
        }

        void updateErrorState(QLineEdit* le)
        {
            const bool error = isError(le);

            if (error != le->property(HasError).toBool())
            {
                le->setProperty(HasError, error);
            }
        }

        void updateClearButtonState(QLineEdit* le)
        {
            const bool clear = isClearButtonNeeded(le);

            // We don't want to impose the clear button visibility if user did not called
            // lineEdit->setClearButtonEnabled on its own.
            // So let get the clear action if available to update it only.
            QAction* clearAction = le->findChild<QAction*>("_q_qlineeditclearaction");
            if (clearAction)
            {
                clearAction->setVisible(clear);
            }
        }

        void updateErrorStateSlot()
        {
            if (auto le = qobject_cast<QLineEdit*>(sender()))
            {
                updateErrorState(le);
            }
        }

        void updateClearButtonStateSlot()
        {
            if (auto le = qobject_cast<QLineEdit*>(sender()))
            {
                updateClearButtonState(le);
            }
        }
    };

    QPointer<LineEditWatcher> LineEdit::s_lineEditWatcher = nullptr;
    unsigned int LineEdit::s_watcherReferenceCount = 0;

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
        ConfigHelpers::read<QColor>(settings, QStringLiteral("FocusedBorderColor"), config.focusedBorderColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("ErrorBorderColor"), config.errorBorderColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("PlaceHolderTextColor"), config.placeHolderTextColor);

        return config;
    }

    LineEdit::Config LineEdit::defaultConfig()
    {
        Config config;

        // Keep radius in sync with css, it is not possible to get it back from the style option :/
        config.borderRadius = 2;
        // An invalid color will take current background color
        config.borderColor = QColor(Qt::transparent);
        config.focusedBorderColor = QColor("#C19CFE");
        config.errorBorderColor = QColor("#E25243");
        config.placeHolderTextColor = QColor("#999999");

        return config;
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
            QObject::connect(lineEdit, &QLineEdit::returnPressed, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::connect(lineEdit, &QLineEdit::editingFinished, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            QPalette pal = lineEdit->palette();
            pal.setColor(QPalette::PlaceholderText, config.placeHolderTextColor);

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
            QObject::disconnect(lineEdit, &QLineEdit::returnPressed, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::editingFinished, s_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            lineEdit->removeEventFilter(s_lineEditWatcher);
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
            const QStyleOptionFrame *fo = qstyleoption_cast<const QStyleOptionFrame*>(option);
            const bool isFocused = option->state.testFlag(QStyle::State_HasFocus);
            const QBrush backgroundColor = option->palette.brush(widget->backgroundRole());
            const QPen frameColor(lineEdit->property(HasError).toBool()
                                  ? config.errorBorderColor
                                  : (isFocused
                                     ? config.focusedBorderColor
                                     : (config.borderColor.isValid()
                                        ? config.borderColor
                                        : backgroundColor)),
                                  fo->lineWidth);

            // Drawing a color border may require some shift for the pen width,
            // this shift is not required if the border is to be transparent (it serve as a margin).
            if (frameColor.color().rgba64().isTransparent())
            {
                const auto frameRect = style->lineEditRect(lineEdit->rect(), fo->lineWidth, config.borderRadius + (fo->lineWidth /2));
                Style::drawFrame(painter, frameRect, QPen(Qt::NoPen), backgroundColor);
            }
            else
            {
                const auto frameRect = style->borderLineEditRect(lineEdit->rect(), fo->lineWidth, config.borderRadius + (fo->lineWidth /2));
                Style::drawFrame(painter, frameRect, frameColor, backgroundColor);
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    QIcon LineEdit::clearButtonIcon(const QStyleOption* option, const QWidget* widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        return QIcon(Style::cachedPixmap(QStringLiteral(":/stylesheet/img/titlebar/titlebar_close_dark.png")));
    }
} // namespace AzQtComponents

#include <Components/Widgets/LineEdit.moc>
