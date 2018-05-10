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

// Description : RoadObject object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_ROADOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_ROADOBJECT_H
#pragma once



#include "SplineObject.h"


// Road Sector
struct CRoadSector
{
    std::vector<Vec3> points;
    IRenderNode* m_pRoadSector;
    float t0, t1;

    CRoadSector()
    {
        m_pRoadSector = 0;
    }
    ~CRoadSector()
    {
        Release();
    }

    CRoadSector(const CRoadSector& old)
    {
        points = old.points;
        t0 = old.t0;
        t1 = old.t1;

        m_pRoadSector = 0;
    }

    void Release();
};

typedef std::vector<CRoadSector> CRoadSectorVector;

namespace RoadsAndRivers
{
    class RoadsAndRiversConverter;
}

/*!
 *  CRoadObject is an object that represent named 3d position in world.
 *
 */
class SANDBOX_API CRoadObject
    : public CSplineObject
{
    Q_OBJECT
    friend class RoadsAndRivers::RoadsAndRiversConverter;
protected:
    friend class CRoadObjectClassDesc;
    CRoadObject();


public:

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    void InitVariables();
    void Done();

    void Display(DisplayContext& dc);
    void DrawRoadObject(DisplayContext& dc, const QColor& col);
    void DrawSectorLines(DisplayContext& dc);

    //////////////////////////////////////////////////////////////////////////

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    using CBaseObject::SetHidden;
    void SetHidden(bool bHidden);
    void UpdateVisibility(bool visible);

    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    virtual void SetMaterial(CMaterial* pMaterial);
    virtual void SetMaterialLayersMask(uint32 nLayersMask);
    virtual void SetMinSpec(uint32 nSpec, bool bSetChildren = true);

    void SetSelected(bool bSelect);
    //////////////////////////////////////////////////////////////////////////

    void SetRoadSectors();
    int GetRoadSectorCount(int index);

    void AlignHeightMap();
    void EraseVegetation();

    // from CSplineObject
    void OnUpdate() override;
    void SetLayerId(uint16 nLayerId) override;
    void SetPhysics(bool isPhysics) override;
    float GetAngleRange() const override {return 25.0f; }

protected:
    // from CSplineObject
    float GetWidth() const override { return mv_width; }
    float GetStepSize() const override { return mv_step; }

    virtual void UpdateSectors();

    // Ignore default draw highlight.
    void DrawHighlight(DisplayContext& dc) {};

    float GetLocalWidth(int index, float t);

    //overrided from CBaseObject.
    void InvalidateTM(int nWhyFlags);

    //! Called when Road variable changes.
    void OnParamChange(IVariable* var);

    void DeleteThis() { delete this; };

    void InitBaseVariables();

protected:
    CRoadSectorVector m_sectors;

    CVariable<float> mv_width;
    CVariable<float> mv_borderWidth;
    CVariable<float> mv_eraseVegWidth;
    CVariable<float> mv_eraseVegWidthVar;
    CVariable<float> mv_step;
    CVariable<float> mv_tileLength;
    CVariable<float> mv_viewDistanceMultiplier;
    CVariable<int> mv_sortPrio;
    CVariable<bool> m_ignoreTerrainHoles;
    CVariable<bool> m_physicalize;

    //! Forces Road to be always 2D. (all vertices lie on XY plane).
    bool m_bIgnoreParamUpdate;
    bool m_bNeedUpdateSectors;

    static int m_rollupId;
    static class CRoadPanel* m_panel;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_ROADOBJECT_H
