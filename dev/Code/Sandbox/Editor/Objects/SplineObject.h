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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_SPLINEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_SPLINEOBJECT_H
#pragma once


const float kSplinePointSelectionRadius = 0.8f;



// Spline Point
struct CSplinePoint
{
    Vec3 pos;
    Vec3 back;
    Vec3 forw;
    float angle;
    float width;
    bool isDefaultWidth;
    CSplinePoint()
    {
        angle = 0;
        width = 0;
        isDefaultWidth = true;
    }
};



//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CSplineObject
    : public CBaseObject
{
    Q_OBJECT
protected:
    CSplineObject();

public:
    virtual void SetPoint(int index, const Vec3& pos);
    int InsertPoint(int index, const Vec3& point);
    void RemovePoint(int index);
    int GetPointCount() const { return m_points.size(); }
    const Vec3& GetPoint(int index) const { return m_points[index].pos; }
    Vec3 GetBezierPos(int index, float t) const;
    float GetBezierSegmentLength(int index, float t = 1.0f) const;
    Vec3 GetBezierNormal(int index, float t) const;
    Vec3 GetLocalBezierNormal(int index, float t) const;
    Vec3 GetBezierTangent(int index, float t) const;
    int GetNearestPoint(const Vec3& raySrc, const Vec3& rayDir, float& distance);
    float GetSplineLength() const;
    float GetPosByDistance(float distance, int& outIndex) const;

    float GetPointAngle() const;
    void SetPointAngle(float angle);
    float GetPointWidth() const;
    void SetPointWidth(float width);
    bool IsPointDefaultWidth() const;
    void PointDafaultWidthIs(bool isDefault);

    void SelectPoint(int index);
    int GetSelectedPoint() const { return m_selectedPoint; }

    void SetEditMode(bool isEditMode) { m_isEditMode = isEditMode; }

    void SetMergeIndex(int index) { m_mergeIndex = index; }
    void ReverseShape();
    void Split(int index, const Vec3& point);
    void Merge(CSplineObject* pSpline);

    void CalcBBox();
    virtual float CreationZOffset() const { return 0.1f; }

    void GetNearestEdge(const Vec3& raySrc, const Vec3& rayDir, int& p1, int& p2, float& distance, Vec3& intersectPoint);

    virtual void OnUpdate() {}
    virtual void SetLayerId(uint16 nLayerId) {}
    virtual void SetPhysics(bool isPhysics) {}
    virtual float GetAngleRange() const {return 180.0f; }

protected:
    void OnUpdateUI();
    void UpdateTransformManipulator();

    void DrawJoints(DisplayContext& dc);
    bool RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt);

    virtual int GetMaxPoints() const { return 1000; }
    virtual int GetMinPoints() const { return 2; }
    virtual float GetWidth() const { return 0.0f; }
    virtual float GetStepSize() const { return 1.0f; }

    void BezierCorrection(int index);
    void BezierAnglesCorrection(int index);

    // from CBaseObject
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file) override;
    void Done() override;
    void BeginEditParams(IEditor* pEditor, int flags) override;
    void EndEditParams(IEditor* pEditor) override;
    void GetBoundBox(AABB& box) override;
    void GetLocalBounds(AABB& box) override;
    void Display(DisplayContext& dc) override;
    bool HitTest(HitContext& hc) override;
    bool HitTestRect(HitContext& hc) override;
    void Serialize(CObjectArchive& ar) override;
    int MouseCreateCallback(CViewport* pView, EMouseEvent event, QPoint& point, int flags) override;
    virtual bool IsScalable() const override { return !m_isEditMode; }
    virtual bool IsRotatable() const override { return !m_isEditMode; }

protected:
    std::vector<CSplinePoint> m_points;
    AABB m_bbox;

    int m_selectedPoint;
    int m_mergeIndex;

    bool m_isEditMode;

    static class CSplinePanel* m_pSplinePanel;
    static int m_splineRollupID;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SPLINEOBJECT_H
