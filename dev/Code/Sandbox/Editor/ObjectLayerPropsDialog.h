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

#ifndef CRYINCLUDE_EDITOR_OBJECTLAYERPROPSDIALOG_H
#define CRYINCLUDE_EDITOR_OBJECTLAYERPROPSDIALOG_H
#pragma once

#include <QDialog>
#include <QScopedPointer>

namespace Ui {
    class CObjectLayerPropsDialog;
}


// CObjectLayerPropsDialog dialog

class CObjectLayerPropsDialog
    : public QDialog
{
public:
    CObjectLayerPropsDialog(QWidget* parent = 0);   // standard constructor
    virtual ~CObjectLayerPropsDialog();

protected:
    void showEvent(QShowEvent*) override;
    void UpdateData(bool fromUi = true);
    void accept() override;

    void OnBnClickedPlatform();

    void UpdateSpecsUI();
    void ReadSpecsUI();

public:
    bool m_bVisible;
    bool m_bFrozen;
    bool m_bExternal;
    bool m_bExportToGame;
    bool m_bExportLayerPak;
    bool m_bHavePhysics;
    bool m_bMainLayer;
    bool m_bDefaultLoaded;
    QString m_name;
    QColor m_color;
    int m_specs;

private:
    QScopedPointer<Ui::CObjectLayerPropsDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_OBJECTLAYERPROPSDIALOG_H
