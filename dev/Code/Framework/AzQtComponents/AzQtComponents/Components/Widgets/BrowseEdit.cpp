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
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QTextFormat::d': class 'QSharedDataPointer<QTextFormatPrivate>' needs to have dll-interface to be used by clients of class 'QTextFormat'
#include <QLineEdit>
AZ_POP_DISABLE_WARNING
#include <QIcon>
#include <QPainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QSettings>
#include <QPushButton>
#include <QToolTip>
#include <QAction>
#include <QStyleOptionFrame>

namespace AzQtComponents
{
    static const char clearButtonActionNameC[] = "_q_qlineeditclearaction";

    struct BrowseEdit::InternalData
    {
        QLineEdit* m_lineEdit = nullptr;
        QPushButton* m_attachedButton = nullptr;
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
        setAttribute(Qt::WA_Hover, true);

        QHBoxLayout* boxLayout = new QHBoxLayout(this);

        boxLayout->setContentsMargins(0, 0, 0, 0);
        boxLayout->setSpacing(0);

        m_data->m_lineEdit = new QLineEdit(this);
        // Do not draw custom style, but we polish/unpolish manually to use
        // custom LineEdit behaviour
        Style::flagToIgnore(m_data->m_lineEdit);
        setFocusProxy(m_data->m_lineEdit);
        m_data->m_lineEdit->setObjectName("line-edit");
        m_data->m_lineEdit->installEventFilter(this);
        setClearButtonEnabled(true);
        boxLayout->addWidget(m_data->m_lineEdit);

        connect(m_data->m_lineEdit, &QLineEdit::returnPressed, this, &BrowseEdit::returnPressed);
        connect(m_data->m_lineEdit, &QLineEdit::editingFinished, this, &BrowseEdit::editingFinished);
        connect(m_data->m_lineEdit, &QLineEdit::textChanged, this, &BrowseEdit::onTextChanged);
        connect(m_data->m_lineEdit, &QLineEdit::textEdited, this, &BrowseEdit::textEdited);

        m_data->m_attachedButton = new QPushButton(this);
        m_data->m_attachedButton->setObjectName("attached-button");
        boxLayout->addWidget(m_data->m_attachedButton);
        connect(m_data->m_attachedButton, &QPushButton::clicked, this, &BrowseEdit::attachedButtonTriggered);
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
            action->setEnabled(true);
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
            LineEdit::setErrorMessage(m_data->m_lineEdit, errorToolTip);
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

    void BrowseEdit::setHasAcceptableInput(bool acceptable)
    {
        if (m_data->m_hasAcceptableInput != acceptable)
        {
            m_data->m_hasAcceptableInput = acceptable;
            emit hasAcceptableInputChanged(m_data->m_hasAcceptableInput);
            update();
        }
    }

    void BrowseEdit::onTextChanged(const QString& text)
    {
        setHasAcceptableInput(hasAcceptableInput());
        emit textChanged(text);
    }

    bool BrowseEdit::polish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig)
    {
        Q_UNUSED(config);
        auto browseEdit = qobject_cast<BrowseEdit*>(widget);

        if (browseEdit)
        {
            style->repolishOnSettingsChange(browseEdit);
            auto lineEdit = browseEdit->m_data->m_lineEdit;
            LineEdit::polish(style, lineEdit, lineEditConfig);

            QAction* action = lineEdit->findChild<QAction*>(clearButtonActionNameC);
            if (action)
            {
                QStyleOptionFrame option;
                option.initFrom(lineEdit);
                action->setIcon(LineEdit::clearButtonIcon(&option, lineEdit, lineEditConfig));
            }
        }
        return browseEdit;
    }

    bool BrowseEdit::unpolish(Style* style, QWidget* widget, const Config& config, const LineEdit::Config& lineEditConfig)
    {
        Q_UNUSED(config);
        auto browseEdit = qobject_cast<BrowseEdit*>(widget);

        if (browseEdit)
        {
            auto lineEdit = browseEdit->m_data->m_lineEdit;
            LineEdit::unpolish(style, lineEdit, lineEditConfig);

            QAction* action = lineEdit->findChild<QAction*>(clearButtonActionNameC);
            if (action)
            {
                QStyleOptionFrame option;
                option.initFrom(lineEdit);
                action->setIcon(lineEdit->style()->standardIcon(QStyle::SP_LineEditClearButton, &option, lineEdit));
            }
        }
        return browseEdit;
    }

    bool BrowseEdit::drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const BrowseEdit::Config& config)
    {
        const auto browserEdit = qobject_cast<const BrowseEdit*>(widget);
        if (!browserEdit)
        {
            return false;
        }

        const bool hasError = !browserEdit->hasAcceptableInput();
        const int lineWidth = config.getLineWidth(option, hasError);

        if (lineWidth > 0)
        {
            const QColor frameColor = config.getBorderColor(option, hasError);
            const auto borderRect = style->borderLineEditRect(browserEdit->rect(), lineWidth, config.borderRadius);
            Style::drawFrame(painter, borderRect, Qt::NoPen, frameColor);
        }
        else
        {
            // Draw attached button border
            const int borderRadius = config.borderRadius;
            const int borderWidth = 1;

            const auto borderAdjustment = borderRadius - borderWidth;
            const auto radius = borderRadius + borderWidth;

            auto contentsRect = browserEdit->rect();
            auto buttonRect = browserEdit->m_data->m_attachedButton->rect();
            buttonRect.moveCenter(contentsRect.center());
            buttonRect.moveRight(contentsRect.right() - borderRadius);
            buttonRect.adjust(0, -borderAdjustment, borderAdjustment, borderAdjustment);

            const auto diameter = radius * 2;
            const auto adjustedDiamter = diameter - borderAdjustment;
            QRect tr(buttonRect.right() - adjustedDiamter, buttonRect.top(), diameter, diameter);
            QRect br(buttonRect.right() - adjustedDiamter, buttonRect.bottom() - adjustedDiamter, diameter, diameter);

            QPainterPath pathRect;
            pathRect.moveTo(buttonRect.topLeft());
            pathRect.lineTo(tr.topLeft());
            pathRect.arcTo(tr, 90, -90);
            pathRect.lineTo(br.right() + borderWidth, br.top());
            pathRect.arcTo(br, 0, -90);
            pathRect.lineTo(buttonRect.left(), buttonRect.bottom() + borderWidth);

            Style::drawFrame(painter, pathRect, Qt::NoPen, config.hoverBorderColor);
        }

        return true;
    }

} // namespace AzQtComponents
