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

#include <AzQtComponents/Components/StyledLineEdit.h>

#include <QAction>
#include <QValidator>

namespace AzQtComponents
{
    StyledLineEdit::StyledLineEdit(QWidget* parent)
        : QLineEdit(parent)
        , m_flavor(Plain)
    {
        connect(this, &StyledLineEdit::textChanged, this, &StyledLineEdit::validateEntry);
    }

    StyledLineEdit::~StyledLineEdit()
    {
    }

    StyledLineEdit::Flavor StyledLineEdit::flavor() const
    {
        return m_flavor;
    }

    void StyledLineEdit::setFlavor(StyledLineEdit::Flavor flavor)
    {
        if (flavor != m_flavor)
        {
            m_flavor = flavor;
            adaptColorText();
            emit flavorChanged();
        }
    }

    void StyledLineEdit::focusInEvent(QFocusEvent* event)
    {
        Q_UNUSED(event);
        adaptColorText(true);
    }

    void StyledLineEdit::focusOutEvent(QFocusEvent* event)
    {
        Q_UNUSED(event);
        adaptColorText();
    }

    void StyledLineEdit::validateEntry()
    {
        QString textToValidate = text();
        int length = textToValidate.length();

        if (!validator() || (m_flavor != Valid && m_flavor != Invalid))
        {
            return;
        }

        if (validator()->validate(textToValidate, length) == QValidator::Acceptable)
        {
            setFlavor(StyledLineEdit::Valid);
        }
        else
        {
            setFlavor(StyledLineEdit::Invalid);
        }
    }

    void StyledLineEdit::adaptColorText(bool focus)
    {
        if (m_flavor == Invalid && focus)
        {
            setStyleSheet("color: rgb(226, 82, 67);");
        }
        else
        {
            setStyleSheet("color: black;");
        }
    }

#include <Components/StyledLineEdit.moc>
} // namespace AzQtComponents
