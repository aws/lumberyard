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

#include "stdafx.h"
#include "TcbPreviewCtrl.h"

#include <IMovieSystem.h>
#include <QPainter>

// CTcbPreviewCtrl

CTcbPreviewCtrl::CTcbPreviewCtrl(QWidget* parent)
    : QWidget(parent)
{
    m_tens = 0;
    m_cont = 0;
    m_bias = 0;
    m_easefrom = 0;
    m_easeto = 0;

    m_spline = GetIEditor()->GetMovieSystem()->CreateTrack(eAnimCurveType_TCBVector);
    m_spline->SetNumKeys(3);
}

CTcbPreviewCtrl::~CTcbPreviewCtrl()
{
}

QSize CTcbPreviewCtrl::sizeHint() const
{
    return QSize(100, 100);
}


void CTcbPreviewCtrl::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.fillRect(0, 0, width(), height(), Qt::white);
    DrawSpline(&p);
}

void CTcbPreviewCtrl::DrawSpline(QPainter* painter)
{
    Vec3 v[3];

    QRect rc(0, 0, width(), height());
    float xoffset = 0;

    int xbase = rc.left() + 2;
    int ybase = rc.bottom() + 2;

    float w = rc.width() - 4;
    float h = rc.height() - 8;

    v[0](xbase, ybase, 0);
    v[1](xbase + w / 2, ybase - h, 0);
    v[2](xbase + w, ybase, 0);

    // Initialize 3 keys.
    ITcbKey key[3];
    key[0].time = 0;
    key[0].SetValue(v[0]);

    key[1].time = 0.5f;
    key[1].SetValue(v[1]);
    key[1].tens = m_tens;
    key[1].bias = m_bias;
    key[1].cont = m_cont;
    key[1].easefrom = m_easefrom;
    key[1].easeto = m_easeto;

    key[2].time = 1.0f;
    key[2].SetValue(v[2]);

    // Set keys to spline.
    m_spline->SetKey(0, &key[0]);
    m_spline->SetKey(1, &key[1]);
    m_spline->SetKey(2, &key[2]);

    QPen grayPen(QColor(150, 150, 150));
    QPen bluePen(QColor(0, 0, 255));
    QPen redPen(QColor(255, 0, 0));


    float dt = 1.0f / rc.width();
    float time = 0;

    // Draw spline.
    QPainterPath path;
    path.moveTo(xbase, ybase);
    for (time = 0; time < 1.0f; time += dt)
    {
        Vec3 vec;
        m_spline->GetValue(time, vec);

        int px = vec.x;
        int py = vec.y;
        if (px > rc.left() && px < rc.right() &&
            py > rc.top() && py < rc.bottom())
        {
            path.lineTo(px, py);
        }
    }
    painter->setPen(grayPen);
    painter->drawPath(path);

    dt = 4.0f / m_rcClient.width();
    // Draw spline ticks.
    painter->setPen(bluePen);
    for (time = 0; time < 1.0f; time += dt)
    {
        Vec3 vec;
        m_spline->GetValue(time, vec);

        int px = vec.x;
        int py = vec.y;
        if (px > rc.left() && px < rc.right() &&
            py > rc.top() && py < rc.bottom())
        {
            painter->drawLine(px, py - 1, px, py + 2);
        }
    }

    // Draw red cross at middle key.
    {
        painter->setPen(redPen);

        Vec3 vec;
        m_spline->GetValue(0.5f, vec);
        int px = vec.x;
        int py = vec.y;
        painter->drawLine(px, py - 2, px, py + 3);
        painter->drawLine(px - 2, py, px + 3, py);
    }
}

void CTcbPreviewCtrl::SetTcb(float tens, float cont, float bias, float easeto, float easefrom)
{
    bool bModified = false;
    if (m_tens != tens || m_cont != cont || m_bias != bias || m_easeto != easeto || m_easefrom != easefrom)
    {
        bModified = true;
    }

    m_tens = tens;
    m_cont = cont;
    m_bias = bias;
    m_easefrom = easefrom;
    m_easeto = easeto;

    if (bModified)
    {
        update();
    }
}