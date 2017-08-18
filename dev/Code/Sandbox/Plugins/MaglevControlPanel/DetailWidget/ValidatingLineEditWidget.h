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

#include <functional>

#include <QDialog>
#include <QLabel>
#include <QLineEdit>

using Bool_QStringFunc = std::function<bool(const QString&)>;
using QString_QStringFunc = std::function<QString(const QString&)>;

class MessageDetailWidget
    : public QDialog
{
    Q_OBJECT

public:

    MessageDetailWidget(QWidget* parent = 0);

    void SetText(const QString& text);

protected:

    QLabel m_label;
};

class ValidatingLineEditWidget
    : public QLineEdit
{
    Q_OBJECT

public:

    ValidatingLineEditWidget(Bool_QStringFunc isLengthValid, Bool_QStringFunc isFormatValid, QString_QStringFunc getInvalidLengthError, QString_QStringFunc getInvalidFormatError);

public slots:

    void OnMoveOrResize();

protected:

    void MoveErrorMessage();

    Bool_QStringFunc m_isTextFormatValid;
    Bool_QStringFunc m_isTextLengthValid;
    QString_QStringFunc m_getInvalidTextFormatErrorString;
    QString_QStringFunc m_getInvalidTextLengthErrorString;

    MessageDetailWidget* m_errorMessage;
    bool m_isTextValid;

protected slots:

    void OnTextEdit(const QString& text);
};