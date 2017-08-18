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

#ifndef __BrushPanel_h__
#define __BrushPanel_h__

#pragma once

#include <QWidget>
#include <QScopedPointer>

class CBrushObject;
class QPushButton;
class QEditorToolButton;
namespace Ui {
    class CBrushPanel;
}

// CBrushPanel dialog

class CBrushPanel
    : public QWidget
{
public:
    CBrushPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CBrushPanel();

    void SetBrush(CBrushObject* obj);

protected:
    void OnCbnSelendokSides();
    void OnBnClickedReload();
    void OnBnClickedSavetoCGF();
    void OnBnClickedEditGeometry();

    CBrushObject* m_brushObj;
    QScopedPointer<Ui::CBrushPanel> ui;
};

#endif // __BrushPanel_h__