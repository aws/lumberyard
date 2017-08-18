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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TCBPREVIEWCTRL_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TCBPREVIEWCTRL_H
#pragma once

#include <QWidget>

struct IAnimTrack;

// CTcbPreviewCtrl

class CTcbPreviewCtrl
    : public QWidget
{
public:
    CTcbPreviewCtrl(QWidget* parent = 0);
    virtual ~CTcbPreviewCtrl();

    void SetTcb(float tens, float cont, float bias, float easeto, float easefrom);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void DrawSpline(QPainter* painter);

    QRect m_rcClient;

    // Tcb params,
    float m_tens;
    float m_cont;
    float m_bias;
    float m_easeto;
    float m_easefrom;

    TSmartPtr<IAnimTrack> m_spline;
};


#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TCBPREVIEWCTRL_H
