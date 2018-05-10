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

// Description : RiverObject object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_RIVEROBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_RIVEROBJECT_H
#pragma once


#include "RoadObject.h"

/*!
 *  CRiverObject used to create rivers.
 *
 */
class SANDBOX_API CRiverObject
    : public CRoadObject
{
    Q_OBJECT
    friend class RoadsAndRivers::RoadsAndRiversConverter;

public:
    CRiverObject();

    virtual void Serialize(CObjectArchive& ar);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    virtual void SetMaterial(CMaterial* mtl);

protected:
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables();
    virtual void UpdateSectors();
    virtual void UpdateSector(CRoadSector& sector);

    virtual void OnRiverParamChange(IVariable* var);
    virtual void OnRiverPhysicsParamChange(IVariable* var);

    Vec3 GetPremulFogColor() const;

    void Physicalize();

protected:
    CVariable<float> mv_waterVolumeDepth;
    CVariable<float> mv_waterStreamSpeed;
    CVariable<float> mv_waterFogDensity;
    CVariable<Vec3> mv_waterFogColor;
    CVariable<float> mv_waterFogColorMultiplier;
    CVariable<bool> mv_waterFogColorAffectedBySun;
    CVariable<float> mv_waterFogShadowing;
    CVariable<float> mv_waterSurfaceUScale;
    CVariable<float> mv_waterSurfaceVScale;
    CVariable<bool> mv_waterCapFogAtVolumeDepth;
    CVariable<bool> mv_waterCaustics;
    CVariable<float> mv_waterCausticIntensity;
    CVariable<float> mv_waterCausticTiling;
    CVariable<float> mv_waterCausticHeight;

    uint64 m_waterVolumeID;
    Plane m_fogPlane;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_RIVEROBJECT_H
