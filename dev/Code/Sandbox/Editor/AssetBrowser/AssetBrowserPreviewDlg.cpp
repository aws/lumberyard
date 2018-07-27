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

// Description : Implementation of AssetBrowserPreviewDlg.h

#include "StdAfx.h"
#include "AssetBrowserPreviewDlg.h"

#include <AssetBrowser/ui_AssetBrowserPreviewDlg.h>

#include <QBoxLayout>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

// CAssetBrowserPreviewDlg dialog

namespace AssetBrowser
{
    const UINT  kPreview_ViewportMargin = 5;
};

CAssetBrowserPreviewDlg::CAssetBrowserPreviewDlg(QWidget* pParent)
    : QWidget(pParent)
    , m_ui(new Ui::AssetBrowserPreviewDialog)
{
    m_ui->setupUi(this);
    m_ui->m_wndAssetRender->installEventFilter(this);

    m_pAssetItem = nullptr;
    m_pAssetPreviewHeaderDlg = nullptr;
    m_pAssetPreviewFooterDlg = nullptr;
    m_noPreviewTextFont = QFont(QStringLiteral("Arial Bold"), 9);

    /*
       In MFC OnInitDialog was delayed behind the scenes, being called at some
       time after the constructor but just before the window is shown for the
       first time. Using QTimer is simple way to accomplish the same thing. 
       
       QTimer relies on the event loop, so by calling QTimer::singleShot with 0
       timeout, the constructor will finish immediately, followed by a call to
       OnInitDialog shortly after, once Qt's event loop is up and running. 
    */
    QTimer::singleShot(0, [&]() { OnInitDialog(); });
}

CAssetBrowserPreviewDlg::~CAssetBrowserPreviewDlg()
{
    SetAssetPreviewFooterDlg(nullptr);
    SetAssetPreviewHeaderDlg(nullptr);
    OnDestroy();
    //Confetti Begin
    if (m_pAssetItem)
    {
        m_pAssetItem->OnEndPreview();
    }
    //Confetti End
}

// CAssetBrowserPreviewDlg message handlers

void CAssetBrowserPreviewDlg::OnInitDialog()
{
    m_piRenderer = gEnv->pRenderer;

    if (m_piRenderer->CreateContext(reinterpret_cast<HWND>(m_ui->m_wndAssetRender->winId())))
    {
        m_piRenderer->MakeMainContextActive();
    }
}

void CAssetBrowserPreviewDlg::StartPreviewAsset(IAssetItem* pAsset)
{
    m_pAssetItem = pAsset;

    if (m_pAssetItem)
    {
        SetAssetPreviewHeaderDlg(m_pAssetItem->GetCustomPreviewPanelHeader(this));
        SetAssetPreviewFooterDlg(m_pAssetItem->GetCustomPreviewPanelFooter(this));
        m_pAssetItem->OnBeginPreview(m_ui->m_wndAssetRender);
    }
    else
    {
        m_pAssetItem = NULL;
        SetAssetPreviewHeaderDlg(nullptr);
        SetAssetPreviewFooterDlg(nullptr);
    }

    update();
}

void CAssetBrowserPreviewDlg::EndPreviewAsset()
{
    // lets remake the small thumb image by rendering the asset again, before closing the preview
    if (m_pAssetItem)
    {
        m_pAssetItem->OnEndPreview();

        SetAssetPreviewHeaderDlg(m_pAssetItem->GetCustomPreviewPanelHeader(this));

        if (m_pAssetPreviewHeaderDlg)
        {
            m_pAssetPreviewHeaderDlg->setVisible(false);
        }

        SetAssetPreviewFooterDlg(m_pAssetItem->GetCustomPreviewPanelFooter(this));

        if (m_pAssetPreviewFooterDlg)
        {
            m_pAssetPreviewFooterDlg->setVisible(false);
        }
    }

    m_pAssetItem = NULL;
    SetAssetPreviewHeaderDlg(nullptr);
    SetAssetPreviewFooterDlg(nullptr);
}

static void ensureContains(QWidget* parent, QWidget* child)
{
    if (!parent->layout())
    {
        parent->setLayout(new QHBoxLayout);
        parent->layout()->setMargin(0);
    }

    while (parent->layout()->takeAt(0) != nullptr)
    {
    }

    if (child)
    {
        parent->layout()->addWidget(child);
        child->setVisible(true);
    }
    if (parent->parentWidget())
    {
        parent->parentWidget()->layout()->activate();
    }
}

void CAssetBrowserPreviewDlg::SetAssetPreviewHeaderDlg(QWidget* pDlg)
{
    if (m_pAssetPreviewHeaderDlg == pDlg)
    {
        return;
    }
    m_pAssetPreviewHeaderDlg = pDlg;
    ensureContains(m_ui->m_pAssetPreviewHeaderDlgContainer, m_pAssetPreviewHeaderDlg);
}

void CAssetBrowserPreviewDlg::SetAssetPreviewFooterDlg(QWidget* pDlg)
{
    if (m_pAssetPreviewFooterDlg == pDlg)
    {
        return;
    }
    m_pAssetPreviewFooterDlg = pDlg;
    ensureContains(m_ui->m_pAssetPreviewFooterDlgContainer, m_pAssetPreviewFooterDlg);
}

void CAssetBrowserPreviewDlg::mousePressEvent(QMouseEvent* event)
{
    m_lastPanDragPt = event->pos();
    update();
}

void CAssetBrowserPreviewDlg::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        if (m_pAssetItem)
        {
            const QPoint pt = m_ui->m_wndAssetRender->mapFromParent(event->pos());
            const QRect rc = m_ui->m_wndAssetRender->rect();

            const bool bAltClick = (event->modifiers() & Qt::AltModifier);
            const bool bCtrlClick = (event->modifiers() & Qt::ControlModifier);
            const bool bShiftClick = (event->modifiers() & Qt::ShiftModifier);
            const int nFlags = (bCtrlClick ? MK_CONTROL : 0) |
                (bShiftClick ? MK_SHIFT : 0) |
                ((event->buttons() & Qt::LeftButton) ? MK_LBUTTON : 0) |
                ((event->buttons() & Qt::MiddleButton) ? MK_MBUTTON : 0) |
                ((event->buttons() & Qt::RightButton) ? MK_RBUTTON : 0);

            m_pAssetItem->PreviewRender(m_ui->m_wndAssetRender, rc, pt.x(), pt.y(), event->x() - m_lastPanDragPt.x(), event->y() - m_lastPanDragPt.y(), 0, nFlags);

            m_lastPanDragPt = event->pos();
        }
    }

    update();
}

void CAssetBrowserPreviewDlg::OnDestroy()
{
    if (m_piRenderer)
    {
        m_piRenderer->DeleteContext(reinterpret_cast<HWND>(m_ui->m_wndAssetRender->winId()));
    }
}

void CAssetBrowserPreviewDlg::Render()
{
    if (m_pAssetItem && m_ui->m_wndAssetRender->isVisible())
    {
        const QRect rc = m_ui->m_wndAssetRender->rect();
        m_pAssetItem->PreviewRender(m_ui->m_wndAssetRender, rc);
    }
}

bool CAssetBrowserPreviewDlg::eventFilter(QObject* object, QEvent* event)
{
    if (object != m_ui->m_wndAssetRender)
    {
        return QWidget::eventFilter(object, event);
    }

    switch (event->type())
    {
    case QEvent::Paint:
    {
        const QRect rc = m_ui->m_wndAssetRender->rect();
        if (m_pAssetItem)
        {
            Render();
        }
        else
        {
            QPainter painter(m_ui->m_wndAssetRender);
            QLinearGradient gradient(rc.topLeft(), rc.bottomLeft());
            gradient.setColorAt(0, QColor(70, 70, 70));
            gradient.setColorAt(1, QColor(110, 110, 110));
            painter.fillRect(rc, gradient);
            painter.setFont(m_noPreviewTextFont);
            painter.setPen(QColor(150, 150, 150));
            painter.drawText(rc, Qt::AlignCenter | Qt::TextWordWrap, tr("<No preview available>"));
        }
        return true;
    }
    case QEvent::Wheel:
    {
        QWheelEvent* ev = static_cast<QWheelEvent*>(event);
        if (m_pAssetItem)
        {
            const int zDelta = ev->delta();
            const bool bAltClick = (ev->modifiers() & Qt::AltModifier);
            const bool bCtrlClick = (ev->modifiers() & Qt::ControlModifier);
            const bool bShiftClick = (ev->modifiers() & Qt::ShiftModifier);
            const int fwKeys = (bCtrlClick ? MK_CONTROL : 0) |
                (bShiftClick ? MK_SHIFT : 0) |
                ((ev->buttons() & Qt::LeftButton) ? MK_LBUTTON : 0) |
                ((ev->buttons() & Qt::MiddleButton) ? MK_MBUTTON : 0) |
                ((ev->buttons() & Qt::RightButton) ? MK_RBUTTON : 0);

            const QRect rc = m_ui->m_wndAssetRender->rect();

            m_pAssetItem->PreviewRender(m_ui->m_wndAssetRender, rc, 0, 0, 0, 0, zDelta, fwKeys);
            update();
        }
        break;
    }
    case QEvent::KeyPress:
    {
        QKeyEvent* ev = static_cast<QKeyEvent*>(event);
        if (m_pAssetItem)
        {
            const bool bAltClick = (ev->modifiers() & Qt::AltModifier);
            const bool bCtrlClick = (ev->modifiers() & Qt::ControlModifier);
            const bool bShiftClick = (ev->modifiers() & Qt::ShiftModifier);
            const int nFlags = (bCtrlClick ? MK_CONTROL : 0) |
                (bShiftClick ? MK_SHIFT : 0) |
                (bAltClick ? MK_ALT : 0);

            m_pAssetItem->OnPreviewRenderKeyEvent(true, ev->nativeVirtualKey(), nFlags);
            update();

            return true;
        }
        break;
    }
    case QEvent::KeyRelease:
    {
        QKeyEvent* ev = static_cast<QKeyEvent*>(event);
        if (m_pAssetItem)
        {
            const bool bAltClick = (ev->modifiers() & Qt::AltModifier);
            const bool bCtrlClick = (ev->modifiers() & Qt::ControlModifier);
            const bool bShiftClick = (ev->modifiers() & Qt::ShiftModifier);
            const int nFlags = (bCtrlClick ? MK_CONTROL : 0) |
                (bShiftClick ? MK_SHIFT : 0) |
                (bAltClick ? MK_ALT : 0);

            m_pAssetItem->OnPreviewRenderKeyEvent(false, ev->nativeVirtualKey(), nFlags);
            update();

            return true;
        }
        break;
    }
    }

    return false;
}

#include <AssetBrowser/AssetBrowserPreviewDlg.moc>
