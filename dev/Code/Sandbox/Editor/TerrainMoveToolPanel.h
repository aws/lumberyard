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

// Description : Terrain modification tool.


#ifndef CRYINCLUDE_EDITOR_TERRAINMOVETOOLPANEL_H
#define CRYINCLUDE_EDITOR_TERRAINMOVETOOLPANEL_H


#pragma once
#include <QWidget>
#include <QScopedPointer>

namespace Ui {
    class CTerrainMoveToolPanel;
}
class QPushButton;
class QSpinBox;
class QComboBox;
class QLabel;

/////////////////////////////////////////////////////////////////////////////
// CTerrainMoveToolPanel dialog
class CTerrainMoveTool;

class CTerrainMoveToolPanel
    : public QWidget
{
    // Construction
public:
    CTerrainMoveToolPanel(CTerrainMoveTool* tool, QWidget* pParent = nullptr);   // standard constructor

    void UpdateButtons();
    void UpdateOffsetText(const Vec3& offset, bool bReset);

    // Dialog Data
    QPushButton*    m_selectSource;
    QPushButton*    m_selectTarget;

    // Implementation
protected:
    void SetOffsetText(QLabel* label, QString title, float offset, bool bReset);

    void OnSelectSource();
    void OnSelectTarget();
    void OnCopyButton();
    void OnMoveButton();
    void OnUpdateNumbers();
    void OnChangeTargetRot();
    void OnSyncHeight();

    QSpinBox* m_dymX;
    QSpinBox* m_dymY;
    QSpinBox* m_dymZ;

    QComboBox* m_cbTargetRot;

    CTerrainMoveTool*   m_tool;
    QScopedPointer<Ui::CTerrainMoveToolPanel> ui;
};

#endif // CRYINCLUDE_EDITOR_TERRAINMOVETOOLPANEL_H
