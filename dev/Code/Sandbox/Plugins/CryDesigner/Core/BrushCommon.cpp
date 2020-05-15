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

#include "StdAfx.h"
#include "ViewManager.h"
#include "Grid.h"
#include "Objects/DesignerObject.h"
#include "BrushHelper.h"
#include "SurfaceInfoPicker.h"
#include <QDir>
#include "Serialization/Enum.h"
#include "Tools/DesignerTool.h"

const BrushFloat kDesignerEpsilon((BrushFloat)1.0e-4);
const BrushFloat kDesignerLooseEpsilon((BrushFloat)1.0e-3);
const BrushFloat kDistanceLimitation((BrushFloat)0.001);
const BrushFloat kLimitForMagnetic((BrushFloat)8.0);
const BrushFloat kInitialPrimitiveHeight((BrushFloat)1.0e-5);

namespace CD
{
    SERIALIZATION_ENUM_BEGIN(EDesignerTool, "DesignerMode")
    SERIALIZATION_ENUM_END()

    BrushVec3 WorldPos2ScreenPos(const BrushVec3& worldPos)
    {
        const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();
        Vec3 screenPos;
        camera.Project(worldPos, screenPos, Vec2i(0, 0), Vec2i(0, 0));
        return ToBrushVec3(screenPos);
    }

    BrushVec3 ScreenPos2WorldPos(const BrushVec3& screenPos)
    {
        BrushVec3 worldPos;

        int vx, vy, vw, vh;
        GetIEditor()->GetRenderer()->GetViewport(&vx, &vy, &vw, &vh);
        const CCamera& camera = GetIEditor()->GetRenderer()->GetCamera();

        Matrix44A mProj, mView;
        mathMatrixPerspectiveFov(&mProj, camera.GetFov(), camera.GetProjRatio(), camera.GetNearPlane(), camera.GetFarPlane());
        mathMatrixLookAt(&mView, camera.GetPosition(), camera.GetPosition() + camera.GetViewdir(), BrushVec3(0, 0, 1));
        Matrix44A mInvViewProj = (mView * mProj).GetInverted();

        Vec4d projectedpos = Vec4d((screenPos.x - vx) / vw * 2.0f - 1.0f,
                -((screenPos.y - vy) / vh) * 2.0f + 1.0f,
                screenPos.z,
                1.0f);

        Vec4d wp = projectedpos * mInvViewProj;
        worldPos.x = wp.x / wp.w;
        worldPos.y = wp.y / wp.w;
        worldPos.z = wp.z / wp.w;

        return worldPos;
    }

    void DrawSpot(DisplayContext& dc, const BrushMatrix34& worldTM, const BrushVec3& pos, const ColorB& color, float fSize)
    {
        const BrushMatrix34& viewTM(dc.view->GetViewTM());
        const BrushMatrix34& invWorldTM(worldTM.GetInverted());

        BrushVec3 positionInScreen = CD::WorldPos2ScreenPos(worldTM.TransformPoint(pos));

        const int kPointCount(4);

        BrushVec3 vAxisXInScreen(fSize, 0, 0);
        BrushVec3 vAxisYInScreen(0, fSize, 0);
        BrushVec3 vPositions[kPointCount] = {
            invWorldTM.TransformPoint(CD::ScreenPos2WorldPos(positionInScreen - vAxisXInScreen - vAxisYInScreen)),
            invWorldTM.TransformPoint(CD::ScreenPos2WorldPos(positionInScreen + vAxisXInScreen - vAxisYInScreen)),
            invWorldTM.TransformPoint(CD::ScreenPos2WorldPos(positionInScreen + vAxisXInScreen + vAxisYInScreen)),
            invWorldTM.TransformPoint(CD::ScreenPos2WorldPos(positionInScreen - vAxisXInScreen + vAxisYInScreen))
        };

        dc.DepthTestOff();
        dc.SetLineWidth(CD::kLineThickness);
        dc.SetColor(color);
        dc.DrawQuad(vPositions[3], vPositions[2], vPositions[1], vPositions[0]);
        dc.DepthTestOn();
    }

    ESplitResult Split(
        const BrushPlane& plane,
        const BrushLine& splitLine,
        const std::vector<BrushEdge3D>& splitHelperEdges,
        const BrushEdge3D& inEdge,
        BrushEdge3D& positiveEdge,
        BrushEdge3D& negativeEdge)
    {
        static const BrushFloat& kEpsilon = kDesignerEpsilon;

        BrushEdge3D correctedInEdge(inEdge);
        std::vector<BrushFloat> xyzList;
        for (int a = 0; a < 2; ++a)
        {
            for (int k = 0; k < 3; ++k)
            {
                for (int i = 0, iSplitHelperEdgeCount(splitHelperEdges.size()); i < iSplitHelperEdgeCount; ++i)
                {
                    if (correctedInEdge.m_v[a][k] != splitHelperEdges[i].m_v[0][k] && std::abs(correctedInEdge.m_v[a][k] - splitHelperEdges[i].m_v[0][k]) < kEpsilon)
                    {
                        correctedInEdge.m_v[a][k] = splitHelperEdges[i].m_v[0][k];
                        break;
                    }
                    else if (correctedInEdge.m_v[a][k] != splitHelperEdges[i].m_v[1][k] && std::abs(correctedInEdge.m_v[a][k] - splitHelperEdges[i].m_v[1][k]) < kEpsilon)
                    {
                        correctedInEdge.m_v[a][k] = splitHelperEdges[i].m_v[1][k];
                        break;
                    }
                }
            }
        }

        BrushVec3 vPivot = splitHelperEdges[0].m_v[0];
        BrushVec3 vDir = splitHelperEdges[0].m_v[1] - splitHelperEdges[0].m_v[0];
        BrushFloat vDirSquaredDistance = vDir.GetLengthSquared();

        BrushVec3 proj0 = (vDir.Dot(correctedInEdge.m_v[0] - vPivot) / vDirSquaredDistance) * vDir;
        BrushVec3 perp0 = correctedInEdge.m_v[0] - vPivot - proj0;
        BrushFloat perp_d0 = perp0.GetLength();

        BrushVec3 proj1 = (vDir.Dot(correctedInEdge.m_v[1] - vPivot) / vDirSquaredDistance) * vDir;
        BrushVec3 perp1 = correctedInEdge.m_v[1] - vPivot - proj1;
        BrushFloat perp_d1 = perp1.GetLength();

        BrushVec2 p0 = plane.W2P(correctedInEdge.m_v[0]);
        BrushVec2 p1 = plane.W2P(correctedInEdge.m_v[1]);
        BrushFloat d0 = splitLine.Distance(p0);
        BrushFloat d1 = splitLine.Distance(p1);

        if (d0 < 0)
        {
            d0 = -perp_d0;
        }
        else
        {
            d0 = perp_d0;
        }

        if (d1 < 0)
        {
            d1 = -perp_d1;
        }
        else
        {
            d1 = perp_d1;
        }

        if (std::abs(d0) < kEpsilon)
        {
            d0 = 0;
        }

        if (std::abs(d1) < kEpsilon)
        {
            d1 = 0;
        }

        if (d0 == 0 && d1 == 0)
        {
            return eSR_COINCIDENCE;
        }

        if (d0 > 0 && d1 < 0 || d0 < 0 && d1 > 0)
        {
            BrushFloat denominator = splitLine.m_Normal.Dot(p1 - p0);
            if (std::abs(denominator) < kEpsilon)
            {
                if (denominator < 0)
                {
                    denominator = -kEpsilon;
                }
                else
                {
                    denominator = kEpsilon;
                }
            }

            BrushFloat t = -d0 / denominator;

            BrushVec3 intersection;
            intersection = correctedInEdge.m_v[0] + t * (correctedInEdge.m_v[1] - correctedInEdge.m_v[0]);

            BrushEdge3D::value_type2 intersection_data;
            intersection_data = correctedInEdge.m_data[0] + (float)t * (correctedInEdge.m_data[1] - correctedInEdge.m_data[0]);

            // +-------------------+
            // [                   ]
            // [                   ]
            // +-------------------+
            // [                   ]
            // [                   ]
            // +-------------------+
            // The intersections between '['']' edges and the middle horizontal edge should be end points of the middle horizontal edge.
            // so the following code carries out a check to make sure if the intersections are on the end points of the horizontal edge.
            for (int i = 0, iEdgeSize(splitHelperEdges.size()); i < iEdgeSize; ++i)
            {
                if (splitHelperEdges[i].m_v[0].IsEquivalent(intersection, kEpsilon))
                {
                    intersection = splitHelperEdges[i].m_v[0];
                    intersection_data = splitHelperEdges[i].m_data[0];
                    break;
                }
                if (splitHelperEdges[i].m_v[1].IsEquivalent(intersection, kEpsilon))
                {
                    intersection = splitHelperEdges[i].m_v[1];
                    intersection_data = splitHelperEdges[i].m_data[1];
                    break;
                }
            }
            /////////////////////////////////////////////////

            if (d1 > 0)
            {
                negativeEdge = BrushEdge3D(correctedInEdge.m_v[0], intersection, correctedInEdge.m_data[0], intersection_data);
                positiveEdge = BrushEdge3D(intersection, correctedInEdge.m_v[1], intersection_data, correctedInEdge.m_data[1]);
            }
            else
            {
                positiveEdge = BrushEdge3D(correctedInEdge.m_v[0], intersection, correctedInEdge.m_data[0], intersection_data);
                negativeEdge = BrushEdge3D(intersection, correctedInEdge.m_v[1], intersection_data, correctedInEdge.m_data[1]);
            }
        }
        else if (d0 > 0 && d1 >= 0 || d0 >= 0 && d1 > 0)
        {
            positiveEdge = correctedInEdge;
            negativeEdge = BrushEdge3D(BrushVec3(0, 0, 0), BrushVec3(0, 0, 0));
        }
        else if (d0 < 0 && d1 <= 0 || d0 <= 0 && d1 < 0)
        {
            positiveEdge = BrushEdge3D(BrushVec3(0, 0, 0), BrushVec3(0, 0, 0));
            negativeEdge = correctedInEdge;
        }

        bool bValidPositiveSide = positiveEdge.m_v[0].GetDistance(positiveEdge.m_v[1]) > kEpsilon;
        bool bValidNegativeSide = negativeEdge.m_v[0].GetDistance(negativeEdge.m_v[1]) > kEpsilon;

        if (bValidPositiveSide && bValidNegativeSide)
        {
            return eSR_CROSS;
        }
        else if (bValidPositiveSide)
        {
            positiveEdge = correctedInEdge;
            return eSR_POSITIVE;
        }
        else if (bValidNegativeSide)
        {
            negativeEdge = correctedInEdge;
            return eSR_NEGATIVE;
        }

        return eSR_COINCIDENCE;
    }

    EOperationResult IntersectEdge3D(const BrushEdge3D& inEdge0, const BrushEdge3D& inEdge1, BrushEdge3D& outEdge)
    {
        static const BrushFloat& kEpsilon = kDesignerEpsilon;

        BrushFloat projectedEdge0[2];
        BrushFloat projectedEdge1[2];
        bool bSwapEdge0 = false;
        bool bSwapEdge1 = false;
        if (!inEdge0.ProjectedEdges(inEdge1, projectedEdge0, projectedEdge1, bSwapEdge0, bSwapEdge1, kEpsilon))
        {
            return eOR_Invalid;
        }

        if (projectedEdge1[0] <= projectedEdge0[0] && projectedEdge1[1] >= projectedEdge0[0] && projectedEdge1[1] <= projectedEdge0[1])
        {
            if (!bSwapEdge0)
            {
                outEdge.m_v[0] = inEdge0.m_v[0];
                outEdge.m_data[0] = inEdge0.m_data[0];
            }
            else
            {
                outEdge.m_v[0] = inEdge0.m_v[1];
                outEdge.m_data[0] = inEdge0.m_data[1];
            }

            if (!bSwapEdge1)
            {
                outEdge.m_v[1] = inEdge1.m_v[1];
                outEdge.m_data[1] = inEdge1.m_data[1];
            }
            else
            {
                outEdge.m_v[1] = inEdge1.m_v[0];
                outEdge.m_data[1] = inEdge1.m_data[0];
            }
        }
        else if (projectedEdge1[1] >= projectedEdge0[1] && projectedEdge1[0] >= projectedEdge0[0] && projectedEdge1[0] <= projectedEdge0[1])
        {
            if (!bSwapEdge1)
            {
                outEdge.m_v[0] = inEdge1.m_v[0];
                outEdge.m_data[0] = inEdge1.m_data[0];
            }
            else
            {
                outEdge.m_v[0] = inEdge1.m_v[1];
                outEdge.m_data[0] = inEdge1.m_data[1];
            }

            if (!bSwapEdge0)
            {
                outEdge.m_v[1] = inEdge0.m_v[1];
                outEdge.m_data[1] = inEdge0.m_data[1];
            }
            else
            {
                outEdge.m_v[1] = inEdge0.m_v[0];
                outEdge.m_data[1] = inEdge0.m_data[0];
            }
        }
        else if (projectedEdge1[0] >= projectedEdge0[0] && projectedEdge1[0] <= projectedEdge0[1] && projectedEdge1[1] >= projectedEdge0[0] && projectedEdge1[1] <= projectedEdge0[1])
        {
            if (!bSwapEdge1)
            {
                outEdge.m_v[0] = inEdge1.m_v[0];
                outEdge.m_v[1] = inEdge1.m_v[1];
                outEdge.m_data[0] = inEdge1.m_data[0];
                outEdge.m_data[1] = inEdge1.m_data[1];
            }
            else
            {
                outEdge.m_v[0] = inEdge1.m_v[1];
                outEdge.m_v[1] = inEdge1.m_v[0];
                outEdge.m_data[0] = inEdge1.m_data[1];
                outEdge.m_data[1] = inEdge1.m_data[0];
            }
        }
        else if (projectedEdge0[0] >= projectedEdge1[0] && projectedEdge0[0] <= projectedEdge1[1] && projectedEdge0[1] >= projectedEdge1[0] && projectedEdge0[1] <= projectedEdge1[1])
        {
            if (!bSwapEdge0)
            {
                outEdge.m_v[0] = inEdge0.m_v[0];
                outEdge.m_v[1] = inEdge0.m_v[1];
                outEdge.m_data[0] = inEdge0.m_data[0];
                outEdge.m_data[1] = inEdge0.m_data[1];
            }
            else
            {
                outEdge.m_v[0] = inEdge0.m_v[1];
                outEdge.m_v[1] = inEdge0.m_v[0];
                outEdge.m_data[0] = inEdge0.m_data[1];
                outEdge.m_data[1] = inEdge0.m_data[0];
            }
        }
        else
        {
            return eOR_Zero;
        }

        if (bSwapEdge1)
        {
            outEdge.Invert();
        }

        return eOR_One;
    }

    EOperationResult SubtractEdge3D(const BrushEdge3D& inEdge0, const BrushEdge3D& inEdge1, BrushEdge3D outEdge[2])
    {
        static const BrushFloat& kEpsilon = kDesignerEpsilon;

        BrushFloat projectedEdge0[2];
        BrushFloat projectedEdge1[2];
        bool bSwapEdge0 = false;
        bool bSwapEdge1 = false;
        if (!inEdge0.ProjectedEdges(inEdge1, projectedEdge0, projectedEdge1, bSwapEdge0, bSwapEdge1, kEpsilon))
        {
            return eOR_Invalid;
        }

        if (projectedEdge1[0] <= projectedEdge0[0] && projectedEdge1[1] >= projectedEdge0[1])
        {
            return eOR_Zero;
        }

        EOperationResult eResult(eOR_Zero);

        if (projectedEdge1[0] <= projectedEdge0[0] && projectedEdge1[1] >= projectedEdge0[0] && projectedEdge1[1] <= projectedEdge0[1])
        {
            if (!bSwapEdge1)
            {
                outEdge[0].m_v[0] = inEdge1.m_v[1];
                outEdge[0].m_data[0] = inEdge1.m_data[1];
            }
            else
            {
                outEdge[0].m_v[0] = inEdge1.m_v[0];
                outEdge[0].m_data[0] = inEdge1.m_data[0];
            }

            if (!bSwapEdge0)
            {
                outEdge[0].m_v[1] = inEdge0.m_v[1];
                outEdge[0].m_data[1] = inEdge0.m_data[1];
            }
            else
            {
                outEdge[0].m_v[1] = inEdge0.m_v[0];
                outEdge[0].m_data[1] = inEdge0.m_data[0];
            }

            eResult = eOR_One;
        }
        else if (projectedEdge1[1] >= projectedEdge0[1] && projectedEdge1[0] >= projectedEdge0[0] && projectedEdge1[0] <= projectedEdge0[1])
        {
            if (!bSwapEdge0)
            {
                outEdge[0].m_v[0] = inEdge0.m_v[0];
                outEdge[0].m_data[0] = inEdge0.m_data[0];
            }
            else
            {
                outEdge[0].m_v[0] = inEdge0.m_v[1];
                outEdge[0].m_data[0] = inEdge0.m_data[1];
            }
            if (!bSwapEdge1)
            {
                outEdge[0].m_v[1] = inEdge1.m_v[0];
                outEdge[0].m_data[1] = inEdge1.m_data[0];
            }
            else
            {
                outEdge[0].m_v[1] = inEdge1.m_v[1];
                outEdge[0].m_data[1] = inEdge1.m_data[1];
            }
            eResult = eOR_One;
        }
        else if (projectedEdge1[0] >= projectedEdge0[0] && projectedEdge1[0] <= projectedEdge0[1] && projectedEdge1[1] >= projectedEdge0[0] && projectedEdge1[1] <= projectedEdge0[1])
        {
            if (!bSwapEdge0)
            {
                outEdge[0].m_v[0] = inEdge0.m_v[0];
                outEdge[0].m_data[0] = inEdge0.m_data[0];
            }
            else
            {
                outEdge[0].m_v[0] = inEdge0.m_v[1];
                outEdge[0].m_data[0] = inEdge0.m_data[1];
            }
            if (!bSwapEdge1)
            {
                outEdge[0].m_v[1] = inEdge1.m_v[0];
                outEdge[0].m_data[1] = inEdge1.m_data[0];
            }
            else
            {
                outEdge[0].m_v[1] = inEdge1.m_v[1];
                outEdge[0].m_data[1] = inEdge1.m_data[1];
            }
            if (!bSwapEdge1)
            {
                outEdge[1].m_v[0] = inEdge1.m_v[1];
                outEdge[1].m_data[0] = inEdge1.m_data[1];
            }
            else
            {
                outEdge[1].m_v[0] = inEdge1.m_v[0];
                outEdge[1].m_data[0] = inEdge1.m_data[0];
            }
            if (!bSwapEdge0)
            {
                outEdge[1].m_v[1] = inEdge0.m_v[1];
                outEdge[1].m_data[1] = inEdge0.m_data[1];
            }
            else
            {
                outEdge[1].m_v[1] = inEdge0.m_v[0];
                outEdge[1].m_data[1] = inEdge0.m_data[0];
            }
            eResult = eOR_Two;
        }
        else
        {
            eResult = eOR_One;
            outEdge[0] = inEdge0;
        }

        if (bSwapEdge1)
        {
            outEdge[0].Invert();
            outEdge[1].Invert();
        }

        return eResult;
    }

    void STexInfo::Load(XmlNodeRef& xmlNode)
    {
        xmlNode->getAttr("texshift_u", shift[0]);
        xmlNode->getAttr("texshift_v", shift[1]);
        xmlNode->getAttr("texscale_u", scale[0]);
        xmlNode->getAttr("texscale_v", scale[1]);
        xmlNode->getAttr("texrotate", rotate);
    }

    void STexInfo::Save(XmlNodeRef& xmlNode)
    {
        xmlNode->setAttr("texshift_u", shift[0]);
        xmlNode->setAttr("texshift_v", shift[1]);
        xmlNode->setAttr("texscale_u", scale[0]);
        xmlNode->setAttr("texscale_v", scale[1]);
        xmlNode->setAttr("texrotate", rotate);
    }

    void CalcTextureBasis(const SBrushPlane<float>& p, const CD::STexInfo& ti, Vec3& u, Vec3& v)
    {
        Vec3  pvecs[2];
        float sinv, cosv;

        u.Set(0, 0, 0);
        v.Set(0, 0, 0);

        p.CalcTextureAxis(pvecs[0], pvecs[1]);

        u = pvecs[0];
        v = pvecs[1];

        if (ti.rotate == 0)
        {
            sinv = 0;
            cosv = 1;
        }
        else if (ti.rotate == 90)
        {
            sinv = 1;
            cosv = 0;
        }
        else if (ti.rotate == 180)
        {
            sinv = 0;
            cosv = -1;
        }
        else if (ti.rotate == 270)
        {
            sinv = -1;
            cosv = 0;
        }
        else
        {
            float ang = ti.rotate / 180 * gf_PI;
            sinv = sin(ang);
            cosv = cos(ang);
        }

        int sv, tv;
        if (pvecs[0].x)
        {
            sv = 0;
        }
        else if (pvecs[0].y)
        {
            sv = 1;
        }
        else
        {
            sv = 2;
        }

        if (pvecs[1].x)
        {
            tv = 0;
        }
        else if (pvecs[1].y)
        {
            tv = 1;
        }
        else
        {
            tv = 2;
        }

        for (int i = 0; i < 2; i++)
        {
            float ns = cosv * pvecs[i][sv] - sinv * pvecs[i][tv];
            float nt = sinv * pvecs[i][sv] + cosv * pvecs[i][tv];
            if (!i)
            {
                u[sv] = ns;
                u[tv] = nt;
            }
            else
            {
                v[sv] = ns;
                v[tv] = nt;
            }
        }
    }

    void CalcTexCoords(const SBrushPlane<float>& p, const CD::STexInfo& ti, const Vec3& pos, float& tu, float& tv)
    {
        Vec3 tVecx, tVecy;
        CalcTextureBasis(p, ti, tVecx, tVecy);

        float scale_u = fabs(ti.scale[0]) < kDesignerEpsilon ? aznumeric_cast<float>(kDesignerEpsilon) : ti.scale[0];
        float scale_v = fabs(ti.scale[1]) < kDesignerEpsilon ? aznumeric_cast<float>(kDesignerEpsilon) : ti.scale[1];

        if (ti.scale[0] < 0)
        {
            scale_u = -scale_u;
        }

        if (ti.scale[1] < 0)
        {
            scale_v = -scale_v;
        }

        tu = pos.Dot(tVecx) / scale_u + ti.shift[0];
        tv = pos.Dot(tVecy) / scale_v + ti.shift[1];
    }

    void FitTexture(const SBrushPlane<float>& plane, const Vec3& vMin, const Vec3& vMax, float tileU, float tileV, STexInfo& outTexInfo)
    {
        int i;
        float width, height;
        float rot_width, rot_height;
        float cosv, sinv, ang;
        float min_t, min_s, max_t, max_s;
        float s, t;
        Vec3 vecs[2];
        Vec3 coords[4];

        ang = outTexInfo.rotate / 180 * gf_PI;
        sinv = sinf(ang);
        cosv = cosf(ang);

        plane.CalcTextureAxis(vecs[0], vecs[1]);

        min_s = vMin | vecs[0];
        min_t = vMin | vecs[1];
        max_s = vMax | vecs[0];
        max_t = vMax | vecs[1];

        width = max_s - min_s;
        height = max_t - min_t;

        coords[0][0] = min_s;
        coords[0][1] = min_t;
        coords[1][0] = max_s;
        coords[1][1] = min_t;
        coords[2][0] = min_s;
        coords[2][1] = max_t;
        coords[3][0] = max_s;
        coords[3][1] = max_t;

        min_s = min_t = 99999;
        max_s = max_t = -99999;

        for (i = 0; i < 4; i++)
        {
            s = cosv * coords[i][0] - sinv * coords[i][1];
            t = sinv * coords[i][0] + cosv * coords[i][1];
            if (i & 1)
            {
                if (s > max_s)
                {
                    max_s = s;
                }
            }
            else
            {
                if (s < min_s)
                {
                    min_s = s;
                }

                if (i < 2)
                {
                    if (t < min_t)
                    {
                        min_t = t;
                    }
                }
                else
                {
                    if (t > max_t)
                    {
                        max_t = t;
                    }
                }
            }
        }

        rot_width =  (max_s - min_s);
        rot_height = (max_t - min_t);

        float lastTileU = GetAdjustedFloatToAvoidZero(tileU);
        float lastTileV = GetAdjustedFloatToAvoidZero(tileV);

        outTexInfo.scale[0] = -rot_width / lastTileU;
        outTexInfo.scale[1] = -rot_height / lastTileV;

        float lastScaleU = GetAdjustedFloatToAvoidZero(outTexInfo.scale[0]);
        float lastScaleV = GetAdjustedFloatToAvoidZero(outTexInfo.scale[1]);

        outTexInfo.shift[0] = min_s / lastScaleU;
        outTexInfo.shift[1] = min_t / lastScaleV;
    }

    void FillMesh(const FlexibleMesh& mesh, IIndexedMesh* pMesh)
    {
        if (pMesh == NULL)
        {
            return;
        }

        int vertexCount = mesh.vertexList.size();
        int faceCount = mesh.faceList.size();

        pMesh->FreeStreams();
        pMesh->SetVertexCount(vertexCount);
        pMesh->SetFaceCount(faceCount);
        pMesh->SetIndexCount(0);
        pMesh->SetTexCoordCount(vertexCount);

        Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
        Vec3* const normals = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);
        SMeshTexCoord* const texcoords = pMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
        SMeshFace* const faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);

        for (int i = 0; i < vertexCount; ++i)
        {
            positions[i] = ToVec3(mesh.vertexList[i].pos);
            normals[i] = ToVec3(mesh.normalList[i]);
            texcoords[i] = SMeshTexCoord(mesh.vertexList[i].uv);
        }

        memcpy(faces, &mesh.faceList[0], faceCount * sizeof(SMeshFace));

        if (!mesh.matId2SubsetMap.empty())
        {
            pMesh->SetSubSetCount(mesh.matId2SubsetMap.size());
            std::map<int, int>::const_iterator ii = mesh.matId2SubsetMap.begin();
            for (; ii != mesh.matId2SubsetMap.end(); ++ii)
            {
                pMesh->SetSubsetMaterialId(ii->second, ii->first);
            }
        }
        else
        {
            pMesh->SetSubSetCount(1);
            pMesh->SetSubsetMaterialId(0, 0);
        }

        pMesh->CalcBBox();
    }

    void MakeSectorOfCircle(BrushFloat fRadius, const BrushVec2& vCenter, BrushFloat startRadian, BrushFloat diffRadian, int nSegmentCount, std::vector<BrushVec2>& outVertexList)
    {
        for (int i = nSegmentCount - 1; i >= 0; --i)
        {
            BrushFloat theta = startRadian + diffRadian * ((BrushFloat)i / (BrushFloat)(nSegmentCount - 1));
            BrushVec2 v(vCenter.x + fRadius * cosf(aznumeric_cast<float>(theta)), vCenter.y + fRadius * sinf(aznumeric_cast<float>(theta)));
            outVertexList.push_back(v);
        }
    }

    float ComputeAnglePointedByPos(const BrushVec2& vCenter, const BrushVec2& vPointedPos)
    {
        BrushVec2 vCenterToPos2D = (vPointedPos - vCenter).GetNormalized();
        BrushVec2 vAxis2D(1.0f, 0.0f);
        BrushLine vAxisLine(vCenter, vCenter + vAxis2D);
        float fAngle = aznumeric_cast<float>(acos(vAxis2D.Dot(vCenterToPos2D)));
        if (vAxisLine.Distance(vPointedPos) < 0)
        {
            fAngle = -fAngle;
        }
        return fAngle;
    }

    bool DoesEquivalentExist(std::vector<BrushEdge3D>& edgeList, const BrushEdge3D& edge, int* pOutIndex)
    {
        BrushEdge3D invEdge = edge.GetInverted();

        if (CD::IsEquivalent(edge.m_v[0], edge.m_v[1]))
        {
            for (int i = 0, iEdgeSize(edgeList.size()); i < iEdgeSize; ++i)
            {
                if (CD::IsEquivalent(edgeList[i].m_v[0], edge.m_v[0]) || CD::IsEquivalent(edgeList[i].m_v[1], edge.m_v[0]))
                {
                    if (pOutIndex)
                    {
                        *pOutIndex = i;
                    }
                    return true;
                }
            }
        }
        else
        {
            for (int i = 0, iEdgeSize(edgeList.size()); i < iEdgeSize; ++i)
            {
                if (edgeList[i].IsEquivalent(edge) || edgeList[i].IsEquivalent(invEdge))
                {
                    if (pOutIndex)
                    {
                        *pOutIndex = i;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool DoesEquivalentExist(std::vector<BrushVec3>& vertexList, const BrushVec3& vertex)
    {
        for (int i = 0, iVertexSize(vertexList.size()); i < iVertexSize; ++i)
        {
            if (CD::IsEquivalent(vertexList[i], vertex))
            {
                return true;
            }
        }
        return false;
    }

    bool ComputePlane(const std::vector<SVertex>& vList, BrushPlane& outPlane)
    {
        if (vList.size() < 3)
        {
            return false;
        }

        int nElement = -1;
        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(vList[0].pos[i] - vList[1].pos[i]) > kDesignerEpsilon)
            {
                nElement = i;
                break;
            }
        }

        if (nElement == -1)
        {
            return false;
        }

        BrushFloat fMinimum = (BrushFloat)3e10;
        int nMinimumIndex = -1;
        int iVListCount(vList.size());
        for (int i = 0; i < iVListCount; ++i)
        {
            if (vList[i].pos[nElement] < fMinimum)
            {
                nMinimumIndex = i;
                fMinimum = vList[i].pos[nElement];
            }
        }
        if (nMinimumIndex == -1)
        {
            return false;
        }

        int nPrevIndex = ((nMinimumIndex - 1) + iVListCount) % iVListCount;
        int nNextIndex = ((nMinimumIndex + 1) + iVListCount) % iVListCount;
        outPlane = BrushPlane(vList[nPrevIndex].pos, vList[nMinimumIndex].pos, vList[nNextIndex].pos);
        return true;
    }

    BrushVec3 ComputeNormal(const BrushVec3& v0, const BrushVec3& v1, const BrushVec3& v2)
    {
        BrushVec3 v10 = (v0 - v1).GetNormalized();
        BrushVec3 v12 = (v2 - v1).GetNormalized();
        return v10.Cross(v12).GetNormalized();
    }

    void GetLocalViewRay(const BrushMatrix34& worldTM, IDisplayViewport* view, const QPoint& point, BrushVec3& outRaySrc, BrushVec3& outRayDir)
    {
        Vec3 raySrc, rayDir;
        view->ViewToWorldRay(point, raySrc, rayDir);

        BrushMatrix34 invWorldTM(worldTM.GetInverted());

        outRaySrc = invWorldTM.TransformPoint(ToBrushVec3(raySrc));
        outRayDir = invWorldTM.TransformVector(ToBrushVec3(rayDir));
    }

    void DrawPlane(DisplayContext& dc, const BrushVec3& vPivot, const BrushPlane& plane, float s)
    {
        Matrix33 orthogonalTM = Matrix33::CreateOrthogonalBase(ToVec3(plane.Normal()));

        Vec3 u = orthogonalTM.GetColumn1();
        Vec3 v = orthogonalTM.GetColumn2();

        BrushVec3 p0 = ToBrushVec3(vPivot - u * s - v * s);
        BrushVec3 p1 = ToBrushVec3(vPivot + u * s - v * s);
        BrushVec3 p2 = ToBrushVec3(vPivot + u * s + v * s);
        BrushVec3 p3 = ToBrushVec3(vPivot - u * s + v * s);

        plane.HitTest(p0, p0 + plane.Normal(), NULL, &p0);
        plane.HitTest(p1, p1 + plane.Normal(), NULL, &p1);
        plane.HitTest(p2, p2 + plane.Normal(), NULL, &p2);
        plane.HitTest(p3, p3 + plane.Normal(), NULL, &p3);

        //      dc.DepthTestOff();
        dc.SetColor(RGB(255, 255, 0), 0.5f);
        dc.DrawQuad(ToVec3(p0), ToVec3(p1), ToVec3(p2), ToVec3(p3));
        dc.DrawQuad(ToVec3(p3), ToVec3(p2), ToVec3(p1), ToVec3(p0));
        //      dc.DepthTestOn();
    }

    BrushFloat SnapGrid(BrushFloat fValue)
    {
        CGrid* pGrid = GetIEditor()->GetViewManager()->GetGrid();
        if (!pGrid->IsEnabled())
        {
            return fValue;
        }
        return floor((fValue / pGrid->size) / pGrid->scale + 0.5) * pGrid->size * pGrid->scale;
    }

    void Write2Buffer(std::vector<char>& buffer, const void* pData, int nDataSize)
    {
        DESIGNER_ASSERT(pData && nDataSize);
        if (pData == 0 || nDataSize == 0)
        {
            return;
        }
        for (int i = 0; i < nDataSize; ++i)
        {
            buffer.push_back(((char*)pData)[i]);
        }
    }

    int ReadFromBuffer(std::vector<char>& buffer, int nPos, void* pData, int nDataSize)
    {
        DESIGNER_ASSERT(pData && nDataSize);
        if (pData == 0 || nDataSize == 0)
        {
            return -1;
        }
        DESIGNER_ASSERT(nPos >= 0 && nPos < buffer.size());
        if (nPos < 0 || nPos >= buffer.size())
        {
            return -1;
        }

        memcpy(pData, &buffer[nPos], nDataSize);

        return nPos + nDataSize;
    }

    bool GetIntersectionOfRayAndAABB(const BrushVec3& vPoint, const BrushVec3& vDir, const AABB& aabb, BrushFloat* pOutDistance)
    {
        Vec3 vHitPos;
        if (Intersect::Ray_AABB(ToVec3(vPoint), ToVec3(vDir), aabb, vHitPos))
        {
            if (pOutDistance)
            {
                *pOutDistance = vPoint.GetDistance(ToBrushVec3(vHitPos));
            }
            return true;
        }
        return false;
    }

    int FindShortestEdge(std::vector<BrushEdge3D>& edges)
    {
        int nShortestIndex = 0;
        BrushFloat fShortestDistance = edges[0].GetLength();

        for (int i = 1, iEdgeSize(edges.size()); i < iEdgeSize; ++i)
        {
            BrushFloat distance = edges[i].GetLength();
            if (edges[i].GetLength() < fShortestDistance)
            {
                nShortestIndex = i;
                fShortestDistance = distance;
            }
        }

        return nShortestIndex;
    }

    std::vector<BrushEdge3D> FindNearestEdges(CViewport* pViewport, const BrushMatrix34& worldTM, CD::EdgeQueryResult& edges)
    {
        std::map< BrushFloat, std::vector<BrushEdge3D> > mapDistance2Edge;
        BrushVec3 cameraPos = worldTM.GetInverted().TransformPoint(ToBrushVec3(pViewport->GetViewTM().GetTranslation()));

        for (int i = 0, iEdgeCount(edges.size()); i < iEdgeCount; ++i)
        {
            bool bFound = false;
            if (!mapDistance2Edge.empty())
            {
                std::map< BrushFloat, std::vector<BrushEdge3D> >::iterator ii = mapDistance2Edge.begin();
                for (; ii != mapDistance2Edge.end(); ++ii)
                {
                    BrushLine3D line(ii->second[0].m_v[0], ii->second[0].m_v[1]);
                    BrushFloat d0 = std::sqrt(line.GetSquaredDistance(edges[i].first.m_v[0]));
                    BrushFloat d1 = std::sqrt(line.GetSquaredDistance(edges[i].first.m_v[1]));
                    if (d0 < kDesignerEpsilon && d1 < kDesignerEpsilon)
                    {
                        ii->second.push_back(edges[i].first);
                        bFound = true;
                        break;
                    }
                }
            }
            if (bFound)
            {
                continue;
            }
            BrushFloat d = cameraPos.GetDistance(edges[i].second);
            mapDistance2Edge[d].push_back(edges[i].first);
        }

        return mapDistance2Edge.begin()->second;
    }

    BrushVec3 GetElementBoxSize(IDisplayViewport* pView, bool b2DViewport, const BrushVec3& vWorldPos)
    {
        Vec3 vElementBoxSize(gDesignerSettings.fHighlightBoxSize, gDesignerSettings.fHighlightBoxSize, gDesignerSettings.fHighlightBoxSize);
        if (b2DViewport)
        {
            return vElementBoxSize * (BrushFloat)0.5 * pView->GetScreenScaleFactor(vWorldPos);
        }
        return vElementBoxSize * (BrushFloat)0.25 * pView->GetScreenScaleFactor(vWorldPos);
    }

    bool AreTwoPositionsNear(const BrushVec3& modelPos0, const BrushVec3& modelPos1, const BrushMatrix34& worldTM, IDisplayViewport* pViewport, BrushFloat fBoundRadiuse, BrushFloat* pOutDistance)
    {
        QPoint screenPos0 = pViewport->WorldToView(worldTM.TransformPoint(modelPos0));
        QPoint screenPos1 = pViewport->WorldToView(worldTM.TransformPoint(modelPos1));

        BrushFloat screenLength = (BrushVec2(screenPos0.x(), screenPos0.y()) - BrushVec2(screenPos1.x(), screenPos1.y())).GetLength();

        if (pOutDistance)
        {
            *pOutDistance = screenLength;
        }

        return screenLength <= fBoundRadiuse;
    }

    bool HitTestEdge(const BrushVec3& raySrc, const BrushVec3& rayDir, const BrushVec3& vNormal, const BrushEdge3D& edge, IDisplayViewport* pView)
    {
        Ray ray(ToVec3(raySrc), ToVec3(rayDir));
        const Vec3& v0 = edge.m_v[0];
        const Vec3& v1 = edge.m_v[1];

        float fScaleScale = pView ? pView->GetScreenScaleFactor(ToVec3(edge.GetCenter())) * 0.001f : 0.01f;
        Vec3 vRight = (v1 - v0).Cross(vNormal).GetNormalized() * fScaleScale;

        Vec3 p_v0 = v0 + vRight;
        Vec3 p_v1 = v1 + vRight;
        Vec3 p_v2 = v1 - vRight;
        Vec3 p_v3 = v0 - vRight;

        Vec3 hitPos;
        if (Intersect::Ray_Triangle(ray, p_v0, p_v1, p_v2, hitPos) || Intersect::Ray_Triangle(ray, p_v0, p_v2, p_v3, hitPos) ||
            Intersect::Ray_Triangle(ray, p_v0, p_v2, p_v1, hitPos) || Intersect::Ray_Triangle(ray, p_v0, p_v3, p_v2, hitPos))
        {
            return true;
        }

        return false;
    }

    BrushFloat Snap(BrushFloat pos)
    {
        BrushFloat gridSize = GetIEditor()->GetViewManager()->GetGrid()->size;
        BrushFloat gridScale = GetIEditor()->GetViewManager()->GetGrid()->scale;
        return floor((pos / gridSize) / gridScale + 0.5) * gridSize * gridScale;
    }

    void JoinTwoMeshes(FlexibleMesh& mesh0, const FlexibleMesh& mesh1)
    {
        int nVertexOffset = mesh0.vertexList.size();

        mesh0.vertexList.insert(mesh0.vertexList.end(), mesh1.vertexList.begin(), mesh1.vertexList.end());
        mesh0.normalList.insert(mesh0.normalList.end(), mesh1.normalList.begin(), mesh1.normalList.end());

        for (int i = 0, iFaceCount(mesh1.faceList.size()); i < iFaceCount; ++i)
        {
            SMeshFace f;
            for (int k = 0; k < 3; ++k)
            {
                f.v[k] = mesh1.faceList[i].v[k] + nVertexOffset;
            }
            int nMatID = mesh1.FindMatIdFromSubsetNum(mesh1.faceList[i].nSubset);
            f.nSubset = mesh0.AddMatID(nMatID);
            mesh0.faceList.push_back(f);
        }
    }

    bool PickPosFromWorld(IDisplayViewport* view, const QPoint& point, Vec3& outPickedPos)
    {
        CSurfaceInfoPicker worldPicker;
        worldPicker.SetActiveView(view);
        worldPicker.SetPickOptionFlag(CSurfaceInfoPicker::ePickOption_IncludeFrozenObject);
        SRayHitInfo hitInfo;
        if (worldPicker.Pick(point, hitInfo, NULL, CSurfaceInfoPicker::ePOG_GeneralObjects | CSurfaceInfoPicker::ePOG_Terrain))
        {
            outPickedPos = hitInfo.vHitPos;
            return true;
        }
        return false;
    }

    BrushVec3 CorrectVec3(const BrushVec3& v)
    {
        BrushVec3 vResult = v;
        for (int i = 0; i < 3; ++i)
        {
            BrushFloat flooredNum = std::floor(vResult[i]);
            BrushFloat ceiledNum = std::ceil(vResult[i]);
            if (std::abs(vResult[i] - flooredNum) < kDesignerEpsilon)
            {
                vResult[i] = flooredNum;
            }
            else if (std::abs(vResult[i] - ceiledNum) < kDesignerEpsilon)
            {
                vResult[i] = ceiledNum;
            }
        }
        return vResult;
    }

    bool IsConvex(std::vector<BrushVec2>& vList)
    {
        for (int i = 0, iVListCount(vList.size()); i < iVListCount; ++i)
        {
            const BrushVec2& v0 = vList[i];
            const BrushVec2& v1 = vList[(i + 1) % iVListCount];
            const BrushVec2& v2 = vList[(i + 2) % iVListCount];
            BrushVec3 v10 = (v0 - v1).GetNormalized();
            BrushVec3 v12 = (v2 - v1).GetNormalized();
            if (v10.Cross(v12).z < kDesignerEpsilon)
            {
                return false;
            }
        }
        return true;
    }

    bool HasEdgeInEdgeList(const std::vector<BrushEdge3D>& edgeList, const BrushEdge3D& edge)
    {
        BrushEdge3D invertedEdge = edge.GetInverted();

        for (int i = 0, iEdgeCount(edgeList.size()); i < iEdgeCount; ++i)
        {
            if (edgeList[i].IsEquivalent(edge) || edgeList[i].IsEquivalent(invertedEdge))
            {
                return true;
            }
        }

        return false;
    }

    bool HasVertexInVertexList(const std::vector<BrushVec3>& vertexList, const BrushVec3& v)
    {
        for (int i = 0, iVertexCount(vertexList.size()); i < iVertexCount; ++i)
        {
            if (CD::IsEquivalent(vertexList[i], v))
            {
                return true;
            }
        }
        return false;
    }

    stack_string GetSaveStateFilePath(const char* name)
    {
        const QByteArray userFolder = Path::GetResolvedUserSandboxFolder().toUtf8();
        stack_string filePath = stack_string().Format("%s/%s.json", CRYDESIGNER_USER_DIRECTORY, name).c_str();
        stack_string saveFilePath = stack_string().Format("%s%s", userFolder.data(), filePath.c_str()).c_str();
        QDir().mkdir(stack_string().Format("%s%s", userFolder.data(), CRYDESIGNER_USER_DIRECTORY).c_str());
        return saveFilePath;
    }

    bool LoadSettings(const Serialization::SStruct& outObj, const char* name)
    {
        return GetIEditor()->GetSystem()->GetArchiveHost()->LoadJsonFile(outObj, GetSaveStateFilePath(name).c_str());
    }

    bool SaveSettings(const Serialization::SStruct& outObj, const char* name)
    {
        return GetIEditor()->GetSystem()->GetArchiveHost()->SaveJsonFile(GetSaveStateFilePath(name).c_str(), outObj);
    }
}
