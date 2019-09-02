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

#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   newleveldialog.h
//  Version:     v1.00
//  Created:     24/7/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_NEWLEVELDIALOG_H
#define CRYINCLUDE_EDITOR_NEWLEVELDIALOG_H

#include <QScopedPointer>

#include <vector>

#include <QDialog>

namespace Ui {
    class CNewLevelDialog;
}

class CNewLevelDialog
    : public QDialog
{
    Q_OBJECT

public:
    CNewLevelDialog(QWidget* pParent = nullptr);   // standard constructor
    ~CNewLevelDialog();

    void SetTerrainResolution(int res);
    void SetTerrainUnits(int units);

    QString GetLevel() const;
    int GetTerrainType() const;
    int GetTerrainSize(size_t index) const;
    int GetTerrainResolution() const;
    int GetTerrainUnits() const;
    bool IsUseTerrain() const;
    void IsResize(bool bIsResize);


protected:
    void UpdateData(bool fromUi = true);
    void OnInitDialog();

    void UpdateTerrainUi();
    void UpdateTerrainUnits();
    void UpdateTerrainInfo();
    void ReloadLevelFolders();
    void ReloadLevelFoldersRec(const QString& currentFolder);

    void ShowWarning(const QString& message = "");

    void showEvent(QShowEvent* event);

protected slots:
    void OnBnClickedUseTerrain();
    void OnCbnSelendokTerrainType();
    void OnTerrainSizeXChanged(QString &value);
    void OnTerrainSizeYChanged(QString &value);
    void OnTerrainSizeZChanged(QString &value);
    void OnCbnSelendokTerrainResolution();
    void OnCbnSelendokTerraniUnits();
    void OnCbnSelendokLevelFolders();
    void OnLevelNameChange();

public:
    QString         m_level;
    QString         m_levelFolders;
    bool                m_useTerrain;
    int                 m_terrainType;
    int                 m_terrainResolution;
    int                 m_terrainSizeX;
    int                 m_terrainSizeY;
    int                 m_terrainSizeZ;
    int                 m_ilevelFolders;
    int                 m_terrainUnits;
    bool                m_bIsResize;
    bool                m_bUpdate;

    std::vector<QString>    m_itemFolders;

    QScopedPointer<Ui::CNewLevelDialog> ui;
    bool m_initialized;
};
#endif // CRYINCLUDE_EDITOR_NEWLEVELDIALOG_H
