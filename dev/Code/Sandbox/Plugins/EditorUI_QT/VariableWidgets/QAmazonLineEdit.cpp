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
#include "EditorUI_QT_Precompiled.h"
#include "QAmazonLineEdit.h"
#include "IEditorParticleUtils.h"

void QAmazonLineEdit::focusOutEvent(QFocusEvent* e)
{
    GetIEditor()->GetParticleUtils()->HotKey_SetEnabled(true);
    return QLineEdit::focusOutEvent(e);
}

void QAmazonLineEdit::focusInEvent(QFocusEvent* e)
{
    GetIEditor()->GetParticleUtils()->HotKey_SetEnabled(false);
    return QLineEdit::focusInEvent(e);
}

void QAmazonLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
    selectAll();
    e->accept();
}

QAmazonLineEdit::QAmazonLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
}
