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
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   Settings.h
//  Created:     Jan/15/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Serialization/IArchive.h"

class DesignerObject;

struct SConsoleVarsForExcluisveMode
{
    SConsoleVarsForExcluisveMode()
    {
        memset(this, 0, sizeof(*this));
    }
    int r_DisplayInfo;
    int r_PostProcessEffects;
    int r_ssdo;
    int e_Vegetation;
    int e_WaterOcean;
    int e_WaterVolumes;
    int e_Terrain;
    int e_Shadows;
    int e_Particles;
    int e_Clouds;
    int r_Beams;
    int e_SkyBox;
};

struct SDesignerExclusiveMode
{
    SDesignerExclusiveMode()
    {
        m_OldTimeOfDay = NULL;
        m_OldTimeOfTOD = 0;
        m_OldCameraTM = Matrix34::CreateIdentity();
        m_OldObjectHideMask = 0;
        m_bExclusiveMode = false;
    }

    void EnableExclusiveMode(bool bEnable);
    void SetTimeOfDayForExclusiveMode();
    void SetCVForExclusiveMode();
    void SetObjectsFlagForExclusiveMode();

    void RestoreTimeOfDay();
    void RestoreCV();
    void RestoreObjectsFlag();

    void CenterCameraForExclusiveMode();

    void SetTime(ITimeOfDay* pTOD, float fTime);

    SConsoleVarsForExcluisveMode m_OldConsoleVars;
    std::map<CBaseObject*, bool> m_ObjectHiddenFlagMap;
    XmlNodeRef m_OldTimeOfDay;
    float m_OldTimeOfTOD;
    Matrix34 m_OldCameraTM;
    int m_OldObjectHideMask;
    bool m_bExclusiveMode;
};

struct SDesignerSettings
{
    bool  bExclusiveMode;
    bool  bSeamlessEdit;
    bool  bDisplayBackFaces;
    bool  bKeepPivotCenter;
    bool  bHighlightElements;
    float fHighlightBoxSize;
    bool  bDisplayDimensionHelper;
    bool  bDisplayTriangulation;
    bool  bDisplaySubdividedResult;

    void Load();
    void Save();

    void Update(bool continuous);

    SDesignerSettings();
    void Serialize(Serialization::IArchive& ar);
};

namespace CD
{
    extern SDesignerExclusiveMode gExclusiveModeSettings;
    extern SDesignerSettings gDesignerSettings;
};

using CD::gExclusiveModeSettings;
using CD::gDesignerSettings;