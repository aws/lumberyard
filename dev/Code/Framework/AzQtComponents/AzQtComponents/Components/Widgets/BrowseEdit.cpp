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

#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>
#include <AzQtComponents/Components/Style.h>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QIcon>
#include <QPainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QSettings>
#include <QToolButton>
#include <QToolTip>
#include <QAction>

namespace AzQtComponents
{
    static const char clearButtonActionNameC[] = "_q_qlineeditclearaction";

    struct BrowseEdit::InternalData
    {
        QLineEdit* m_lineEdit = nullptr;
        QToolButton* m_attachedButton = nullptr;
        QString m_errorToolTip;
        bool m_hasAcceptableInput = true;
    };

    BrowseEdit::BrowseEdit(QWidget* parent /* = nullptr */)
        : QFrame(parent)
        , m_data(new BrowseEdit::InternalData)
    {
        AzQtComponents::Style::addClass(this, "AzQtComponentsBrowseEdit");
        setObjectName("browse-edit");
        setToolTipDuration(1000 * 10);

        QHBoxLayout* boxLayout = new QHBoxLayout(this);
        setLayout(boxLayout);

        boxLayout->setMargin(0);
        boxLayout->setContentsMargins(0, 0, 0, 0);
        boxLayout->setSpacing(0);

        m_data->m_lineEdit = new QLineEdit(this);
        Style::doNotStyle(m_data->m_lineEdit);
        setFocusProxy(m_data->m_lineEdit);
        m_data->m_lineEdit->setObjectName("line-edit");
        m_data->m_lineEdit->installEventFilter(this);
        boxLayout->addWidget(m_data->m_lineEdit);

        connect(m_data->m_lineEdit, &QLineEdit::returnPressed, this, &BrowseEdit::returnPressed);
        connect(m_data->m_lineEdit, &QLineEdit::editingFinished, this, &BrowseEdit::editingFinished);
        connect(m_data->m_lineEdit, &QLineEdit::textChanged, this, &BrowseEdit::onTextChanged);
        connect(m_data->m_lineEdit, &QLineEdit::textEdited, this, &BrowseEdit::textEdited);

        m_data->m_attachedButton = new QToolButton(this);
        m_data->m_attachedButton->setObjectName("attached-button");
        AzQtComponents::PushButton::applyAttachedStyle(m_data->m_attachedButton);
        boxLayout->addWidget(m_data->m_attachedButton);
        connect(m_data->m_attachedButton, &QToolButton::pressed, this, &BrowseEdit::attachedButtonTriggered);
    }

    BrowseEdit::~BrowseEdit()
    {
    }

    // double click of the line edit will trigger the attached button
    void BrowseEdit::setAttachedButtonIcon(const QIcon& icon)
    {
        m_data->m_attachedButton->setIcon(icon);

        m_data->m_attachedButton->setVisible(!icon.isNull());
    }

    QIcon BrowseEdit::attachedButtonIcon() const
    {
        return m_data->m_attachedButton->icon();
    }

    // use a separate button, because UX spec calls for a clear button to be enabled when the line edit
    // is not
    // connect this to the text changed signal
    bool BrowseEdit::isClearButtonEnabled() const
    {
        return m_data->m_lineEdit->isClearButtonEnabled();
    }

    void BrowseEdit::setClearButtonEnabled(bool enable)
    {
        if (isClearButtonEnabled() != enable)
        {
            m_data->m_lineEdit->setClearButtonEnabled(enable);
            if (enable)
            {
                QAction* action = m_data->m_lineEdit->findChild<QAction*>(clearButtonActionNameC);
                if (action)
                {
                    const QPixmap iconPixmap = Style::cachedPixmap(QStringLiteral(":/stylesheet/img/titlebar/titlebar_close_dark.png"));
                    QIcon icon = QIcon(iconPixmap);
                    icon.addPixmap(iconPixmap, QIcon::Disabled);
                    action->setIcon(icon);
                }
            }
        }
    }

    bool BrowseEdit::isLineEditReadOnly() const
    {
        return m_data->m_lineEdit->isReadOnly();
    }

    void BrowseEdit::setLineEditReadOnly(bool readOnly)
    {
        m_data->m_lineEdit->setReadOnly(readOnly);
        QAction* action = m_data->m_lineEdit->findChild<QAction*>(clearButtonActionNameC);
        if (action)
        {
            action->setEnabled(action);
        }
    }

    void BrowseEdit::setPlaceholderText(const QString& placeholderText)
    {
        m_data->m_lineEdit->setPlaceholderText(placeholderText);
    }

    QString BrowseEdit::placeholderText() const
    {
        return m_data->m_lineEdit->placeholderText();
    }

    void BrowseEdit::setText(const QString& text)
    {
        m_data->m_lineEdit->setText(text);
    }

    QString BrowseEdit::text() const
    {
        return m_data->m_lineEdit->text();
    }

    void BrowseEdit::setErrorToolTip(const QString& errorToolTip)
    {
        if (errorToolTip != m_data->m_errorToolTip)
        {
            m_data->m_errorToolTip = errorToolTip;
        }
    }

    QString BrowseEdit::errorToolTip() const
    {
        return m_data->m_errorToolTip;
    }

    void BrowseEdit::setValidator(const QValidator* validator)
    {
        m_data->m_lineEdit->setValidator(validator);
        m_data->m_hasAcceptableInput = m_data->m_lineEdit->hasAcceptableInput();
    }

    const QValidator* BrowseEdit::validator() const
    {
        return m_data->m_lineEdit->validator();
    }

    bool BrowseEdit::hasAcceptableInput() const
    {
        return m_data->m_lineEdit->hasAcceptableInput();
    }

    QLineEdit* BrowseEdit::lineEdit() const
    {
        return m_data->m_lineEdit;
    }

    BrowseEdit::Config BrowseEdit::loadConfig(QSettings& settings)
    {
        return LineEdit::loadConfig(settings);
    }

    BrowseEdit::Config BrowseEdit::defaultConfig()
    {
        return LineEdit::defaultConfig();
    }

    bool BrowseEdit::eventFilter(QObject* watched, QEvent* event)
    {
        if (isEnabled() && (m_data->m_lineEdit == watched))
        {
            switch (event->type())
            {
                case QEvent::MouseButtonDblClick:
                    if (isLineEditReadOnly())
                    {
                        Q_EMIT attachedButtonTriggered();
                        return true;
                    }
                    break;
                case QEvent::FocusIn:
                case QEvent::FocusOut:
                    update();
                    break;
                default:
                    break;
            }
        }

        return QFrame::eventFilter(watched, event);
    }

    void BrowseEdit::mouseDoubleClickEvent(QMouseEvent* event)
    {
        event->setAccepted(true);

        Q_EMIT attachedButtonTriggered();
    }

    bool BrowseEdit::event(QEvent* event)
    {
        switch (event->type())
        {
            case QEvent::ToolTip:
            {
                const auto he = static_cast<QHelpEvent*>(event);
                const QString tooltipText = hasAcceptableInput() ? toolTip() : m_data->m_errorToolTip;
                QToolTip::showText(he->globalPos(), tooltipText, this, QRect(), toolTipDuration());
                return true;
            }
            default:
                break;
        }
        return QFrame::event(event);
    }

    void BrowseEdit::onTextChanged(const QString& text)
    {
        if (m_data->m_hasAcceptableInput != hasAcceptableInput())
        {
            m_data->m_hasAcceptableInput = hasAcceptableInput();
            update();
        }
        Q_EMIT textChanged(text);
    }

    bool BrowseEdit::drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const BrowseEdit::Config& config)
    {
        const auto browserEdit = qobject_cast<const BrowseEdit*>(widget);
        if (!browserEdit)
        {
            return false;
        }

        const QStyleOptionFrame* frameOption = qstyleoption_cast<const QStyleOptionFrame*>(option);
        const bool isFocused = browserEdit->m_data->m_lineEdit->hasFocus() || browserEdit->m_data->m_attachedButton->hasFocus();
        const QBrush backgroundColor = option->palette.brush(widget->backgroundRole());
        const QPen frameColor(!browserEdit->hasAcceptableInput()
                                  ? config.errorBorderColor
                                  : (isFocused
                                            ? config.focusedBorderColor
                                            : (config.borderColor.isValid()
                                                      ? config.borderColor
                                                      : backgroundColor)),
            frameOption->lineWidth);

        const auto frameRect = style->borderLineEditRect(browserEdit->rect(), frameOption->lineWidth, config.borderRadius + (frameOption->lineWidth / 2));
        QPen framePen(frameColor);
        if (frameColor.color().rgba64().isTransparent())
        {
            framePen = QPen(Qt::NoPen);
        }

        Style::drawFrame(painter, frameRect, framePen, QBrush(Qt::NoBrush));
        return true;
    }

} // namespace AzQtComponents
