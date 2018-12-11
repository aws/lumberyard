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

    m_recommendButtons.push_back(m_ui->ButtonZero);
    m_recommendButtons.push_back(m_ui->ButtonOne);
    m_recommendButtons.push_back(m_ui->ButtonTwo);
    m_recommendButtons.push_back(m_ui->ButtonThree);
    m_recommendButtons.push_back(m_ui->ButtonFour);
    m_recommendButtons.push_back(m_ui->ButtonFive);
    m_recommendButtons.push_back(m_ui->ButtonSix);
    m_recommendButtons.push_back(m_ui->ButtonSeven);
    m_recommendButtons.push_back(m_ui->ButtonEight);
    m_recommendButtons.push_back(m_ui->ButtonNine);
    m_recommendButtons.push_back(m_ui->ButtonTen);

    for (int i = 0; i < m_recommendButtons.size(); ++i)
    {
        signalMapper->setMapping(m_recommendButtons.at(i), i);

        // The use of signal and slot Macros used to be the only way to make connections, before Qt 5.
        // The signal and slot mechanism is part of the C++ extensions that are provided by Qt
        // and make use of the Meta Object Compiler(moc).
        // This way of connecting the signal and slot will be checked in the runtime, but
        // using the address of a function can be checked at the time of compilation, which is safer.
        // So we should always consider using function pointer syntax instead of macros.
        // In addition, by using the address of a function, you can refer to any class function,
        // not just those in the section marked slots.
        // This case we connect the QPushButton's signal to the QSignalMapper's map() signal, and this is an exception
        connect(m_recommendButtons.at(i), SIGNAL(clicked()), signalMapper, SLOT(map()));

        m_recommendButtons.at(i)->installEventFilter(this);
    }

    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(SetRatingScore(int)));

    // set up the productivity rating radio buttons
    auto setConfirmed = [this]() 
    { 
        // allow the submit button to be pressed
        m_isConfirmed = true; 
        UpdateSubmitButtonState();
    };
    auto setupProductivityButton = [&](QRadioButton* theButton) 
    { 
        // group the radio buttons to be auto-exclusive and enable the submit button when clicked
        m_productiveButtonGroup.addButton(theButton);
        QObject::connect(theButton, &QRadioButton::clicked, this, setConfirmed);
    };

    setupProductivityButton(m_ui->PriorVersion);
    setupProductivityButton(m_ui->StronglyDisagree);
    setupProductivityButton(m_ui->Disagree);
    setupProductivityButton(m_ui->Neutral);
    setupProductivityButton(m_ui->Agree);
    setupProductivityButton(m_ui->StronglyAgree);

    connect(m_ui->ButtonSubmit, &QPushButton::clicked, this, &NetPromoterScoreDialog::accept);
    connect(m_ui->ButtonClose, &QPushButton::clicked, this, &NetPromoterScoreDialog::reject);

    m_ui->stackedWidget->setCurrentIndex(0);
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
                UpdateRatingState(buttonNumber + 1, m_recommendButtons.size(), ratingButtonLeave);
            }

            break;

        case QEvent::HoverLeave:
            UpdateRatingState(0, m_recommendButtons.size(), ratingButtonLeave);

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
        if (m_ui->stackedWidget->currentIndex() == 0)
        {
            // submitting from first page, advance to the productivity page
            m_ui->stackedWidget->setCurrentIndex(1);
            m_ui->ButtonSubmit->setEnabled(false);
        }
        else
        {
            // submitting from second page
            QString comment = m_ui->CommentBox->toPlainText();
            int textLength = comment.length();

            // trim the comments to 10000 characters
            if (textLength > maxCharacters)
            {
                comment.resize(maxCharacters);
            }

            int productiveScore = -1;
            QList<QAbstractButton*> orderedButtons = m_productiveButtonGroup.buttons();
            for (int buttonIndex = 0, numButtons = orderedButtons.size(); buttonIndex < numButtons; ++buttonIndex)
            {
                QAbstractButton* currButton = orderedButtons[buttonIndex];
                if (currButton->isChecked())
                {
                    productiveScore = buttonIndex;
                    break;
                }
            }

            // Send MetricsEvent
            LyMetrics_SendRating(m_ratingScore, m_ratingInterval, productiveScore, comment.toUtf8());

            Q_EMIT UserInteractionCompleted();
            QDialog::accept();
        }
    }
}

void NetPromoterScoreDialog::reject()
{
    // if we're still on the first page, the ratings score hasn't been accepted and should not be used
    if (m_ui->stackedWidget->currentIndex() == 0)
    {
        m_ratingScore = -1;
    }

    // Send Metrics Event
    LyMetrics_SendRating(m_ratingScore, m_ratingInterval, -1, nullptr);

    Q_EMIT UserInteractionCompleted();
    QDialog::reject();
}

void NetPromoterScoreDialog::SetMessage()
{
    // set the version number into the message
    QVersionNumber version = QVersionNumber::fromString(METRICS_VERSION);
    QString versionNumber = QString::number(version.majorVersion()) + "." + QString::number(version.minorVersion());

    if (strcmp(METRICS_VERSION, "0.0.0.0") == 0)
    {
        m_ui->ProductivityPrompt->setText(tr("Lastly, do you agree this Lumberyard has helped you to be more productive than the last version you used?"));
        if (m_ratingInterval <= 5)
        {
            m_ui->Message->setText(tr("Before you go, tell us how likely is it that you would recommend Lumberyard to a friend or colleague?"));
        }
        else
        {
            m_ui->Message->setText(tr("Now that you've used Lumberyard a while longer, how likely is it that you would recommend Lumberyard to a friend or colleague?"));
        }
    }
    else
    {
        m_ui->ProductivityPrompt->setText(tr("Lastly, do you agree Lumberyard v%1 has helped you to be more productive than the last version you used?").arg(versionNumber));
        if (m_ratingInterval <= 5)
        {
            m_ui->Message->setText(tr("Before you go, tell us how likely is it that you would recommend Lumberyard v%1 to a friend or colleague?").arg(versionNumber));
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
        m_recommendButtons.at(i)->setProperty("class", state);
        m_recommendButtons.at(i)->style()->unpolish(m_recommendButtons.at(i));
        m_recommendButtons.at(i)->style()->polish(m_recommendButtons.at(i));
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
