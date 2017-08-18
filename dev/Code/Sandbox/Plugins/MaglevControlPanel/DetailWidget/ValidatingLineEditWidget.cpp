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

#include "ValidatingLineEditWidget.h"

#include <QVBoxLayout>
#include <QStyle>

#include <DetailWidget/ValidatingLineEditWidget.moc>

MessageDetailWidget::MessageDetailWidget(QWidget* parent)
    : QDialog{parent}
{
    setWindowModality(Qt::NonModal);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    hide();

    m_label.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_label.setWordWrap(true);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(&m_label);
    setLayout(layout);
}

void MessageDetailWidget::SetText(const QString& text)
{
    m_label.setText(text);
}

ValidatingLineEditWidget::ValidatingLineEditWidget(Bool_QStringFunc isLengthValid, Bool_QStringFunc isFormatValid, QString_QStringFunc getInvalidLengthError, QString_QStringFunc getInvalidFormatError)
    : QLineEdit{}
    , m_isTextFormatValid {isFormatValid}
    , m_isTextLengthValid {isLengthValid}
    , m_getInvalidTextFormatErrorString {getInvalidFormatError}
    , m_getInvalidTextLengthErrorString {getInvalidLengthError}
    , m_errorMessage {new MessageDetailWidget {this}}
    , m_isTextValid {
    true
}
{
    connect(this, &ValidatingLineEditWidget::textEdited, this, &ValidatingLineEditWidget::OnTextEdit);
}

void ValidatingLineEditWidget::OnMoveOrResize()
{
    MoveErrorMessage();
}

void ValidatingLineEditWidget::MoveErrorMessage()
{
    // Move to the right side and center vertically
    m_errorMessage->move(mapToGlobal(QPoint(width(), (height() - m_errorMessage->height()) / 2)));
}

void ValidatingLineEditWidget::OnTextEdit(const QString& text)
{
    QString errorText;
    bool isValid = true;

    if (!m_isTextFormatValid(text))
    {
        isValid = false;
        errorText += m_getInvalidTextFormatErrorString(text);
    }

    if (!m_isTextLengthValid(text))
    {
        if (!isValid)
        {
            errorText += "\n";
        }

        isValid = false;
        errorText += m_getInvalidTextLengthErrorString(text);
    }

    if (!isValid)
    {
        m_errorMessage->SetText(errorText);
        MoveErrorMessage();
    }

    if (isValid != m_isTextValid)
    {
        m_isTextValid = isValid;

        setProperty("inputValid", m_isTextValid);
        style()->unpolish(this);
        style()->polish(this);

        if (m_isTextValid)
        {
            m_errorMessage->hide();
        }
        else
        {
            m_errorMessage->show();
            // Height is not reported correctly until after the widget is shown, so we re-move it if it's just been shown.
            MoveErrorMessage();
        }
    }
}