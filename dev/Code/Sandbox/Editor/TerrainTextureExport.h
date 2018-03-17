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

#ifndef CRYINCLUDE_EDITOR_TERRAINTEXTUREEXPORT_H
#define CRYINCLUDE_EDITOR_TERRAINTEXTUREEXPORT_H

#pragma once
// TerrainTextureExport.h : header file
//

#include <QScopedPointer>

#include <QDialog>

namespace Ui {
    class CTerrainTextureExport;
}

class TerrainTextureWidget;
class CTerrainTexGen;

/////////////////////////////////////////////////////////////////////////////
// CTerrainTextureExport dialog

class CTerrainTextureExport
    : public QDialog
{
    Q_OBJECT

    // Construction
public:
    CTerrainTextureExport(QWidget* pParent = NULL);   // standard constructor
    ~CTerrainTextureExport();

    void ImportExport(bool bIsImport, bool bIsClipboard = false);

protected slots:
    void OnExport();
    void OnImport();
    void OnChangeResolutionBtn();

    // Implementation
protected:
    void UpdateTotalResolution();

    TerrainTextureWidget* m_terrainTexture;

    QScopedPointer<Ui::CTerrainTextureExport> ui;
};

#endif // CRYINCLUDE_EDITOR_TERRAINTEXTUREEXPORT_H
