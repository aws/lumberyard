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

#include <QLineEdit>
#include <QAction>
#include <QSettings>
#include <QDynamicPropertyChangeEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QPainter>
#include <QStyleOption>
#include <QDebug>

namespace AzQtComponents
{
    constexpr const char* HasSearchAction = "HasSearchAction";
    constexpr const char* HasError = "HasError";

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

    QPointer<LineEditWatcher> LineEdit::m_lineEditWatcher = nullptr;

    void LineEdit::applySearchStyle(QLineEdit* lineEdit)
    {
        QAction* searchAction = lineEdit->addAction(QIcon(":/stylesheet/img/16x16/Search.png"), QLineEdit::LeadingPosition);
        searchAction->setEnabled(false);
        lineEdit->setProperty(HasSearchAction, true);
    }

    LineEdit::Config LineEdit::loadConfig(QSettings& settings)
    {
        auto ReadInteger = [](QSettings& settings, const QString& name, int& value)
        {
            // only overwrite the value if it's set; otherwise, it'll stay the default
            if (settings.contains(name))
            {
                value = settings.value(name).toInt();
            }
        };

        auto ReadColor = [](QSettings& settings, const QString& name, QColor& color)
        {
            // only overwrite the value if it's set; otherwise, it'll stay the default
            if (settings.contains(name))
            {
                color = QColor(settings.value(name).toString());
            }
        };

        Config config = defaultConfig();

         // Keep radius in sync with css, it is not possible to get it back from the style option :/
        ReadInteger(settings, "BorderRadius", config.borderRadius);
        ReadColor(settings, "BorderColor", config.borderColor);
        ReadColor(settings, "FocusedBorderColor", config.focusedBorderColor);
        ReadColor(settings, "ErrorBorderColor", config.errorBorderColor);
        ReadColor(settings, "PlaceHolderTextColor", config.placeHolderTextColor);

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
        Q_ASSERT(m_lineEditWatcher.isNull());
        m_lineEditWatcher = new LineEditWatcher;
    }

    void LineEdit::uninitializeWatcher()
    {
        delete m_lineEditWatcher;
    }

    bool LineEdit::polish(Style* style, QWidget* widget, const LineEdit::Config& config)
    {
        Q_ASSERT(!m_lineEditWatcher.isNull());

        auto lineEdit = qobject_cast<QLineEdit*>(widget);

        if (lineEdit)
        {
            QObject::connect(lineEdit, &QLineEdit::textChanged, m_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::connect(lineEdit, &QLineEdit::returnPressed, m_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::connect(lineEdit, &QLineEdit::editingFinished, m_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            QPalette pal = lineEdit->palette();
            pal.setColor(QPalette::PlaceholderText, config.placeHolderTextColor);

            lineEdit->setPalette(pal);
            lineEdit->installEventFilter(m_lineEditWatcher);

            // initial state checks
            m_lineEditWatcher->updateErrorState(lineEdit);
            m_lineEditWatcher->updateClearButtonState(lineEdit);

            style->repolishOnSettingsChange(lineEdit);
            m_lineEditWatcher->m_config = config;
        }

        return lineEdit;
    }

    bool LineEdit::unpolish(Style* style, QWidget* widget, const LineEdit::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        Q_ASSERT(!m_lineEditWatcher.isNull());

        auto lineEdit = qobject_cast<QLineEdit*>(widget);

        if (lineEdit)
        {
            QObject::disconnect(lineEdit, &QLineEdit::textChanged, m_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::returnPressed, m_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);
            QObject::disconnect(lineEdit, &QLineEdit::editingFinished, m_lineEditWatcher, &LineEditWatcher::updateClearButtonStateSlot);

            lineEdit->removeEventFilter(m_lineEditWatcher);
        }

        return lineEdit;
    }

    bool LineEdit::drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const LineEdit::Config& config)
    {
        const auto lineEdit = qobject_cast<const QLineEdit*>(widget);

        if (lineEdit)
        {
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
        }

        return lineEdit;
    }

    QIcon LineEdit::clearButtonIcon(const QStyleOption* option, const QWidget* widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        return QIcon(Style::cachedPixmap(QStringLiteral(":/stylesheet/img/titlebar/titlebar_close_dark.png")));
    }
} // namespace AzQtComponents

#include <Components/Widgets/LineEdit.moc>
