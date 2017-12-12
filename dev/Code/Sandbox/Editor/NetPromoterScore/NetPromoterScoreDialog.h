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

#include <QDialog>

namespace Ui {
    class NetPromoterScoreDialog;
}

class NetPromoterScoreDialog
    : public QDialog
{
    Q_OBJECT

public:
    explicit NetPromoterScoreDialog(QWidget* pParent = nullptr);
    ~NetPromoterScoreDialog();

    bool eventFilter(QObject* obj, QEvent* ev) override;
    void SetRatingInterval(int ratingInterval);
    
Q_SIGNALS:
    void UserInteractionCompleted();

public Q_SLOTS:
    void SetRatingScore(int score);
    void UpdateSubmitButtonState();
    void UpdateCommentBoxState(bool isFocused);
    void accept() override;
    void reject() override;

private:
    void SetMessage();
    void UpdateRatingState(int start, int end, const char* state);
    void closeEvent(QCloseEvent* ev) override;

    QScopedPointer<Ui::NetPromoterScoreDialog> m_ui;
    int m_ratingScore;
    int m_ratingInterval = 0;
    bool m_isConfirmed = false;
    int m_prevRatingScore;
    AZStd::vector<QPushButton*> m_buttonGroup;   
};