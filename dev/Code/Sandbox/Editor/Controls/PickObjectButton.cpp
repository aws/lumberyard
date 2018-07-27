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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "PickObjectButton.h"

//////////////////////////////////////////////////////////////////////////
QPickObjectButton::QPickObjectButton(QWidget* parent /* = nullptr */)
    : QPushButton(parent)
    , m_styleSheet(styleSheet())
{
    connect(this, &QPushButton::clicked, this, &QPickObjectButton::OnClicked);
}

QPickObjectButton::~QPickObjectButton()
{
}

void QPickObjectButton::SetSelected(bool selected)
{
    if (selected)
    {
        setStyleSheet(QStringLiteral("QPushButton { background-color: palette(highlight); color: palette(highlighted-text); }"));
    }
    else
    {
        setStyleSheet(m_styleSheet);
    }
    m_selected = selected;
}

void QPickObjectButton::OnClicked()
{
    if (IsSelected() == true)
    {
        SetSelected(false);
        GetIEditor()->CancelPick();
        return;
    }

    SetSelected(true);
    GetIEditor()->PickObject(this, m_targetClass, m_statusText.toUtf8().data(), m_bMultipick);
}

//////////////////////////////////////////////////////////////////////////
void QPickObjectButton::OnPick(CBaseObject* picked)
{
    if (!m_bMultipick)
    {
        SetSelected(false);
    }
    if (m_pickCallback)
    {
        m_pickCallback->OnPick(picked);
    }
}

//////////////////////////////////////////////////////////////////////////
void QPickObjectButton::OnCancelPick()
{
    SetSelected(false);
    if (m_pickCallback)
    {
        m_pickCallback->OnCancelPick();
    }
}

//////////////////////////////////////////////////////////////////////////
void QPickObjectButton::SetPickCallback(IPickObjectCallback* callback, const QString& statusText, const QMetaObject* targetClass, bool bMultiPick)
{
    m_statusText = statusText;
    m_pickCallback = callback;
    m_targetClass = targetClass;
    m_bMultipick = bMultiPick;
}

//////////////////////////////////////////////////////////////////////////
bool QPickObjectButton::OnPickFilter(CBaseObject* filterObject)
{
    if (m_pickCallback)
    {
        return m_pickCallback->OnPickFilter(filterObject);
    }
    return true;
}

#include <Controls/PickObjectButton.moc>
