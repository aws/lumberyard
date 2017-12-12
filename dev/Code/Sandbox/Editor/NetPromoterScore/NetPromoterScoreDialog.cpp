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

#include "StdAfx.h"
#include "NetPromoterScoreDialog.h"
#include <NetPromoterScore/ui_NetPromoterScoreDialog.h>
#include <QtWidgets>
#include <LyMetricsProducer/LyMetricsAPI.h>
#include <AzQtComponents/Utilities/AutoSettingsGroup.h>

static const char* ratingButton = "RatingButton";
static const char* ratingButtonLeave = "RatingButtonLeave";
static const char* ratingButtonHover = "RatingButtonHover";
static const char* ratingButtonPress = "RatingButtonPress";
static const char* commentBoxBefore = "CommentBoxBefore";
static const char* commentBoxAfter = "CommentBoxAfter";
static int maxCharacters = 10000;

NetPromoterScoreDialog::NetPromoterScoreDialog(QWidget* pParent /*= nullptr*/)
    : QDialog(pParent)
    , m_ui(new Ui::NetPromoterScoreDialog)
{
    m_ui->setupUi(this);

    // make sure at the initial stage the comment text box hides the cursor
    UpdateCommentBoxState(false);

    m_ui->CommentBox->installEventFilter(this);

    QSignalMapper* signalMapper = new QSignalMapper(this);

    m_buttonGroup.push_back(m_ui->ButtonZero);
    m_buttonGroup.push_back(m_ui->ButtonOne);
    m_buttonGroup.push_back(m_ui->ButtonTwo);
    m_buttonGroup.push_back(m_ui->ButtonThree);
    m_buttonGroup.push_back(m_ui->ButtonFour);
    m_buttonGroup.push_back(m_ui->ButtonFive);
    m_buttonGroup.push_back(m_ui->ButtonSix);
    m_buttonGroup.push_back(m_ui->ButtonSeven);
    m_buttonGroup.push_back(m_ui->ButtonEight);
    m_buttonGroup.push_back(m_ui->ButtonNine);
    m_buttonGroup.push_back(m_ui->ButtonTen);

    for (int i = 0; i < m_buttonGroup.size(); ++i)
    {
        signalMapper->setMapping(m_buttonGroup.at(i), i);

        // The use of signal and slot Macros used to be the only way to make connections, before Qt 5.
        // The signal and slot mechanism is part of the C++ extensions that are provided by Qt
        // and make use of the Meta Object Compiler(moc).
        // This way of connecting the signal and slot will be checked in the runtime, but
        // using the address of a function can be checked at the time of compilation, which is safer.
        // So we should always consider using function pointer syntax instead of macros.
        // In addition, by using the address of a function, you can refer to any class function,
        // not just those in the section marked slots.
        // This case we connect the QPushButton's signal to the QSignalMapper's map() signal, and this is an exception
        connect(m_buttonGroup.at(i), SIGNAL(clicked()), signalMapper, SLOT(map()));

        m_buttonGroup.at(i)->installEventFilter(this);
    }

    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(SetRatingScore(int)));

    connect(m_ui->ButtonSubmit, &QPushButton::clicked, this, &NetPromoterScoreDialog::accept);
    connect(m_ui->ButtonClose, &QPushButton::clicked, this, &NetPromoterScoreDialog::reject);
}

NetPromoterScoreDialog::~NetPromoterScoreDialog()
{
}

bool NetPromoterScoreDialog::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj->property(ratingButton).isValid())
    {
        int buttonNumber = qobject_cast<QPushButton*>(obj)->text().toInt();

        switch (ev->type())
        {
        case QEvent::HoverEnter:
            UpdateRatingState(0, buttonNumber + 1, ratingButtonHover);

            if (m_isConfirmed)
            {
                UpdateRatingState(buttonNumber + 1, m_buttonGroup.size(), ratingButtonLeave);
            }

            break;

        case QEvent::HoverLeave:
            UpdateRatingState(0, m_buttonGroup.size(), ratingButtonLeave);

            if (m_isConfirmed)
            {
                UpdateRatingState(0, m_prevRatingScore + 1, ratingButtonPress);
            }

            break;
        }
    }

    if (obj == m_ui->CommentBox)
    {
        switch (ev->type())
        {
        case QEvent::FocusIn:
            UpdateCommentBoxState(true);
            break;
        case QEvent::FocusOut:
            UpdateCommentBoxState(false);
            break;
        }
    }

    return false;
}

void NetPromoterScoreDialog::SetRatingInterval(int ratingInterval)
{
    m_ratingInterval = ratingInterval;
    SetMessage();
}

void NetPromoterScoreDialog::accept()
{
    if (m_ui->ButtonSubmit->isEnabled())
    {
        QString comment = m_ui->CommentBox->toPlainText();
        int textLength = comment.length();

        // trim the comments to 10000 characters
        if (textLength > maxCharacters)
        {
            comment.resize(maxCharacters);
        }

        // Send MetricsEvent
        LyMetrics_SendRating(m_ratingScore, m_ratingInterval, comment.toUtf8());

        Q_EMIT UserInteractionCompleted();
        QDialog::accept();
    }
}

void NetPromoterScoreDialog::reject()
{
    // Send Metrics Event
    m_ratingScore = -1;
   
    LyMetrics_SendRating(m_ratingScore, m_ratingInterval, nullptr);

    Q_EMIT UserInteractionCompleted();
    QDialog::reject();
}

void NetPromoterScoreDialog::SetMessage()
{
    // set the version number into the message
    QVersionNumber version = QVersionNumber::fromString(METRICS_VERSION);
    QString versionNumber = QString::number(version.majorVersion()) + "." + QString::number(version.minorVersion());

    if (m_ratingInterval <= 5)
    {
        if (strcmp(METRICS_VERSION, "0.0.0.0") == 0)
        {
            m_ui->Message->setText(tr("Before you go, tell us how likely is it that you would recommend Lumberyard to a friend or colleague?"));
        }
        else
        {
            m_ui->Message->setText(tr("Before you go, tell us how likely is it that you would recommend Lumberyard v%1 to a friend or colleague?").arg(versionNumber));
        }
    }
    else
    {
        if (strcmp(METRICS_VERSION, "0.0.0.0") == 0)
        {
            m_ui->Message->setText(tr("Now that you've used Lumberyard a while longer, how likely is it that you would recommend Lumberyard to a friend or colleague?"));
        }
        else
        {
            m_ui->Message->setText(tr("Now that you've used Lumberyard a while longer, how likely is it that you would recommend Lumberyard v%1 to a friend or colleague?").arg(versionNumber));
        }
    }
}

void NetPromoterScoreDialog::UpdateRatingState(int start, int end, const char* state)
{
    for (int i = start; i < end; ++i)
    {
        m_buttonGroup.at(i)->setProperty("class", state);
        m_buttonGroup.at(i)->style()->unpolish(m_buttonGroup.at(i));
        m_buttonGroup.at(i)->style()->polish(m_buttonGroup.at(i));
    }
}

void NetPromoterScoreDialog::closeEvent(QCloseEvent* ev)
{
    Q_EMIT UserInteractionCompleted();

    reject();
    QDialog::closeEvent(ev);
}

void NetPromoterScoreDialog::SetRatingScore(int score)
{
    m_ratingScore = score;

    UpdateRatingState(0, score + 1, ratingButtonPress);

    m_isConfirmed = true;
    m_prevRatingScore = score;
    m_ui->CommentBox->setFocus();

    UpdateSubmitButtonState();
    UpdateCommentBoxState(true);
}

void NetPromoterScoreDialog::UpdateSubmitButtonState()
{
    m_ui->ButtonSubmit->setEnabled(m_isConfirmed);
}

void NetPromoterScoreDialog::UpdateCommentBoxState(bool isFocused)
{
    int textLength = m_ui->CommentBox->toPlainText().length();

    if (m_isConfirmed || isFocused || textLength > 0)
    {
        m_ui->CommentBox->setProperty("class", commentBoxAfter);
        m_ui->CommentBox->setCursorWidth(1);
    }
    else
    {
        m_ui->CommentBox->setProperty("class", commentBoxBefore);
        m_ui->CommentBox->setCursorWidth(0);
    }


    m_ui->CommentBox->style()->unpolish(m_ui->CommentBox);
    m_ui->CommentBox->style()->polish(m_ui->CommentBox);
}

#include <NetPromoterScore/NetPromoterScoreDialog.moc>
