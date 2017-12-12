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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_WATERSHAPEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_WATERSHAPEOBJECT_H
#pragma once


#include "ShapeObject.h"


struct IWaterVolumeRenderNode;


class CWaterShapeObject
    : public CShapeObject
{
    Q_OBJECT
public:
    CWaterShapeObject();

    static const GUID& GetClassID()
    {
        // {3CC1CF42-917A-4c4d-80D2-2A81E6A32BDB}
        static const GUID guid = {
            0x3cc1cf42, 0x917a, 0x4c4d, { 0x80, 0xd2, 0x2a, 0x81, 0xe6, 0xa3, 0x2b, 0xdb }
        };
        return guid;
    }

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////

    virtual void SetName(const QString& name);
    virtual void SetMaterial(CMaterial* mtl);
    virtual void Serialize(CObjectArchive& ar);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    virtual void SetMinSpec(uint32 nSpec, bool bSetChildren = true);
    virtual void SetMaterialLayersMask(uint32 nLayersMask);
    //void Display( DisplayContext& dc );
    virtual void Validate(CErrorReport* report)
    {
        CBaseObject::Validate(report);
    }
    virtual void SetHidden(bool bHidden);
    virtual void UpdateVisibility(bool visible);

    virtual IRenderNode* GetEngineNode() const { return m_pWVRN; }

protected:
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables();
    virtual bool CreateGameObject();
    virtual void Done();
    virtual void UpdateGameArea(bool remove = false);

    virtual void OnParamChange(IVariable* var);
    virtual void OnWaterParamChange(IVariable* var);
    virtual void OnWaterPhysicsParamChange(IVariable* var);

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
    CVariable<float> mv_viewDistanceMultiplier;
    CVariable<bool> mv_waterCaustics;
    CVariable<float> mv_waterCausticIntensity;
    CVariable<float> mv_waterCausticTiling;
    CVariable<float> mv_waterCausticHeight;

    CSmartVariableArray mv_GroupAdv;
    CVariable<float> mv_waterVolume;
    CVariable<float> mv_accVolume;
    CVariable<float> mv_borderPad;
    CVariable<bool> mv_convexBorder;
    CVariable<float> mv_objVolThresh;
    CVariable<float> mv_waterCell;
    CVariable<float> mv_waveSpeed;
    CVariable<float> mv_waveDamping;
    CVariable<float> mv_waveTimestep;
    CVariable<float> mv_simAreaGrowth;
    CVariable<float> mv_minVel;
    CVariable<float> mv_simDepth;
    CVariable<float> mv_velResistance;
    CVariable<float> mv_hlimit;

    IWaterVolumeRenderNode* m_pWVRN;

    uint64 m_waterVolumeID;
    Plane m_fogPlane;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_WATERSHAPEOBJECT_H
