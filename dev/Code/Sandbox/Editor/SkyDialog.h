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

#ifndef CRYINCLUDE_EDITOR_SKYDIALOG_H
#define CRYINCLUDE_EDITOR_SKYDIALOG_H

#pragma once
// SkyDialog.h : header file
//

#include <QDialog>

namespace Ui {
    class CSkyDialog;
}

enum PreviewDirection
{
    PDNorth,
    PDEast,
    PDSouth,
    PDWest
};

class CloudPreviewWidget;

/////////////////////////////////////////////////////////////////////////////
// CSkyDialog dialog

class CSkyDialog
    : public QDialog
{
    Q_OBJECT

    // Construction
public:
    CSkyDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CSkyDialog();


    // Implementation
protected:
    void OnInitDialog();
    void OnSkyNorth();
    void OnSkySouth();
    void OnSkyWest();
    void OnSkyEast();
    void OnSkyClouds();
    void showEvent(QShowEvent* event) override;

    PreviewDirection m_PD;
    CloudPreviewWidget* m_preview;

    bool m_initialized;
    QScopedPointer<Ui::CSkyDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_SKYDIALOG_H
