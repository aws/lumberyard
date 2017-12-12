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
#ifndef CRYINCLUDE_EDITOR_CDIALOGEMPTY_H
#define CRYINCLUDE_EDITOR_CDIALOGEMPTY_H

#pragma once
// CDialogEmpty.h : header file
//

#include <afxwin.h>
class QWidget;

// This is a placeholder class for porting to Qt and can be removed later. It is used to port
// MFC code that holds and manipulates lists of CDialog. After porting  the CDialogs to QWinWidget,
// we still need to insert back into existing MFC hierarchy that expects CDialogs.
// To use: Create a CDialogEmpty instance, create QWinWidget with CDialogEmpty as parent,
// then add CDialogEmpty to existing code in place of original CDialog.
class CDialogEmpty
    : public CDialog
{
public:
    CDialogEmpty(CWnd* parent);

    CDialogEmpty(CWnd* parent, QWidget* widget);

    template <typename WinWidgetType>
    static CDialogEmpty* create(CWnd* parent)
    {
        auto res = new CDialogEmpty(parent);
        res->setWidget(new WinWidgetType { res });
        res->m_widget->show();
        QSize sh = res->m_widget->sizeHint();
        if (!sh.isEmpty())
        {
            res->MoveWindow(0, 0, sh.width(), sh.height());
        }
        return res;
    }

    template <typename WinWidgetType, typename ... Args>
    static CDialogEmpty* createEx(CWnd* parent, Args&& ... args)
    {
        auto res = new CDialogEmpty(parent);
        res->setWidget(new WinWidgetType { std::forward<Args>(args) ..., res });
        res->m_widget->show();
        QSize sh = res->m_widget->sizeHint();
        if (!sh.isEmpty())
        {
            res->MoveWindow(0, 0, sh.width(), sh.height());
        }
        return res;
    }

    DECLARE_DYNAMIC(CDialogEmpty)

    void setWidget(QWidget* widget);
    QWidget* widget() const;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

    void PostNcDestroy() override;

private:
    void resizeWidget();
    QWidget* m_widget;
};

#endif