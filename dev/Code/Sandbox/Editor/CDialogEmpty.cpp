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
// CDialogEmpty.cpp : implementation file
//

#include "StdAfx.h"
#include "CDialogEmpty.h"
#include "QtWinMigrate/qwinwidget.h"
#include <QWidget>
#include <QVBoxLayout>

IMPLEMENT_DYNAMIC(CDialogEmpty, CWnd)
BEGIN_MESSAGE_MAP(CDialogEmpty, CWnd)
//{{AFX_MSG_MAP(CWnd)
ON_WM_SIZE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

typedef struct
{
    DLGTEMPLATE lpDlg;
    WORD fill[3];   // menu class title
} MINDLGTEMPLATE, * PMINDLGTEMPLATE;

static MINDLGTEMPLATE Dlg = {
    { WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, 0, 0 }
};

CDialogEmpty::CDialogEmpty(CWnd* parent)
    : m_widget(nullptr)
{
    CreateIndirect((LPCDLGTEMPLATE)&Dlg, parent);
}

CDialogEmpty::CDialogEmpty(CWnd* parent, QWidget* widget)
    : CDialogEmpty(parent)
{
    if (qobject_cast<QWinWidget*>(widget))
    {
        setWidget(widget);
    }
    else
    {
        QWinWidget* bridge = new QWinWidget(this);
        bridge->setLayout(new QVBoxLayout);
        bridge->layout()->setContentsMargins(0, 0, 0, 0);
        bridge->layout()->addWidget(widget);
        setWidget(bridge);
    }

    CRect current;
    GetWindowRect(&current);
    SetWindowPos(nullptr, -1, -1, current.Width(), m_widget->sizeHint().height(), SWP_NOMOVE);
    m_widget->show();
}

void CDialogEmpty::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    resizeWidget();
}

void CDialogEmpty::PostNcDestroy()
{
    m_widget->deleteLater();
}

void CDialogEmpty::setWidget(QWidget* widget)
{
    m_widget = widget;
    resizeWidget();
}

QWidget* CDialogEmpty::widget() const
{
    return m_widget;
}

void CDialogEmpty::resizeWidget()
{
    if (!m_widget)
    {
        return;
    }

    RECT rect;
    GetClientRect(&rect);
    m_widget->setGeometry(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}
