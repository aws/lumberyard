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

// Description : interface for the CRollupBar class.


#ifndef CRYINCLUDE_EDITOR_CONTROLS_ROLLUPBAR_H
#define CRYINCLUDE_EDITOR_CONTROLS_ROLLUPBAR_H
#pragma once

class CTerrainPanel;

class CPanelDisplayLayer;
class QRollupCtrl;
class CMainTools;

#include <QTabWidget>

class CRollupBar
    : public QTabWidget
{
    Q_OBJECT
public:
    explicit CRollupBar(QWidget* parent = nullptr);
    static void RegisterViewClass();
    void insertControl(int index, QWidget* w, const QString& tooltip);
    QRollupCtrl* GetRollUpControl(int rollupBarId) const;
private:
    CPanelDisplayLayer* m_pLayerPanel;
    QRollupCtrl* m_objectRollupCtrl;
    QRollupCtrl* m_modellingRollupCtrl;
    QRollupCtrl* m_displayRollupCtrl;
    CMainTools* const m_mainTools;

    CTerrainPanel* m_terrainPanel;
    QRollupCtrl* m_terrainRollupCtrl;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_ROLLUPBAR_H
