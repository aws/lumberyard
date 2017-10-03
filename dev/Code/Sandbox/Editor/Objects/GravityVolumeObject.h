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

// Description : GravityVolumeObject object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_GRAVITYVOLUMEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_GRAVITYVOLUMEOBJECT_H
#pragma once


#include "EntityObject.h"
#include "SafeObjectsArray.h"

#define GravityVolume_CLOSE_DISTANCE 0.8f
#define GravityVolume_Z_OFFSET 0.1f

// GravityVolume Point
struct CGravityVolumePoint
{
    Vec3 pos;
    Vec3 back;
    Vec3 forw;
    float angle;
    float width;
    bool isDefaultWidth;
    CGravityVolumePoint()
    {
        angle = 0;
        width = 0;
        isDefaultWidth = true;
    }
};

typedef std::vector<CGravityVolumePoint> CGravityVolumePointVector;


/*!
 *  CGravityVolumeObject is an object that represent named 3d position in world.
 *
 */
class SANDBOX_API CGravityVolumeObject
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void InitVariables();
    void Done();
    bool HasMeasurementAxis() const {   return true;    }

    void Display(DisplayContext& dc);
    void DrawBezierSpline(DisplayContext& dc, CGravityVolumePointVector& points, const QColor& col, bool isDrawJoints, bool isDrawGravityVolume);

    bool CreateGameObject();

    //////////////////////////////////////////////////////////////////////////
    QString GetUniqueName() const;

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);

    bool HitTest(HitContext& hc);
    bool HitTestRect(HitContext& hc);

    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    void SetSelected(bool bSelect);

    void OnEvent(ObjectEvent event);
    //////////////////////////////////////////////////////////////////////////

    //void SetClosed( bool bClosed );
    //bool IsClosed() { return mv_closed; };

    //! Insert new point to GravityVolume at given index.
    //! @return index of newly inserted point.
    int InsertPoint(int index, const Vec3& point);
    //! Remove point from GravityVolume at given index.
    void RemovePoint(int index);

    //! Get number of points in GravityVolume.
    int GetPointCount() { return m_points.size(); };
    //! Get point at index.
    const Vec3& GetPoint(int index) const { return m_points[index].pos; };
    //! Set point position at specified index.
    void SetPoint(int index, const Vec3& pos);

    void SelectPoint(int index);
    int GetSelectedPoint() const { return m_selectedPoint; };

    //! Find GravityVolume point nearest to given point.
    int GetNearestPoint(const Vec3& raySrc, const Vec3& rayDir, float& distance);

    //! Find GravityVolume edge nearest to given point.
    void GetNearestEdge(const Vec3& pos, int& p1, int& p2, float& distance, Vec3& intersectPoint);

    //! Find GravityVolume edge nearest to given ray.
    void GetNearestEdge(const Vec3& raySrc, const Vec3& rayDir, int& p1, int& p2, float& distance, Vec3& intersectPoint);
    //////////////////////////////////////////////////////////////////////////

    void CalcBBox();

    Vec3 GetBezierPos(CGravityVolumePointVector& points, int index, float t);
    Vec3 GetSplinePos(CGravityVolumePointVector& points, int index, float t);
    Vec3 GetBezierNormal(int index, float t);
    float GetBezierSegmentLength(int index, float t = 1.0f);
    void BezierAnglesCorrection(CGravityVolumePointVector& points, int index);
    void SplinePointsCorrection(CGravityVolumePointVector& points, int index);
    void BezierCorrection(int index);
    void SplineCorrection(int index);

    float GetPointAngle();
    void SetPointAngle(float angle);

    float GetPointWidth();
    void SetPointWidth(float width);

    bool IsPointDefaultWidth();
    void PointDafaultWidthIs(bool IsDefault);

    void UpdateGameArea();


protected:
    friend class CTemplateObjectClassDesc<CGravityVolumeObject>;

    bool RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt);
    virtual int GetMaxPoints() const { return 1000; };
    virtual int GetMinPoints() const { return 2; };

    // Ignore default draw highlight.
    void DrawHighlight(DisplayContext& dc) {};

    void EndCreation();

    //overrided from CBaseObject.
    void InvalidateTM(int nWhyFlags);

    //! Called when GravityVolume variable changes.
    void OnParamChange(IVariable* var);

    //! Dtor must be protected.
    CGravityVolumeObject();

    void DeleteThis() { delete this; };

    static const GUID& GetClassID()
    {
        // {d1fe2e10-2b8b-4fc7-8893-dd6ae5aed3a7}
        static const GUID guid = {
            0xd1fe2e10, 0x2b8b, 0x4fc7, { 0x88, 0x93, 0xdd, 0x6a, 0xe5, 0xae, 0xd3, 0xa7 }
        };
        return guid;
    }

protected:
    AABB m_bbox;

    CGravityVolumePointVector m_points;
    std::vector<Vec3> m_bezierPoints;

    QString m_lastGameArea;

    //////////////////////////////////////////////////////////////////////////
    // GravityVolume parameters.
    //////////////////////////////////////////////////////////////////////////
    CVariable<float> mv_radius;
    CVariable<float> mv_gravity;
    CVariable<float> mv_falloff;
    CVariable<float> mv_damping;
    CVariable<float> mv_step;
    CVariable<bool> mv_dontDisableInvisible;

    int m_selectedPoint;
    //! Forces GravityVolume to be always 2D. (all vertices lie on XY plane).
    bool m_bForce2D;

    bool m_bIgnoreParamUpdate;

    static int m_rollupId;
    static class CGravityVolumePanel* m_panel;
};
	
#endif // CRYINCLUDE_EDITOR_OBJECTS_GRAVITYVOLUMEOBJECT_H
