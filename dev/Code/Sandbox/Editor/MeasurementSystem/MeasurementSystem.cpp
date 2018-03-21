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

#include "stdafx.h"
#include "MeasurementSystem.h"
#include <MeasurementSystem/ui_MeasurementSystem.h>
#include "QtViewPaneManager.h"
#include "Viewport.h"

CMeasurementSystemDialog::CMeasurementSystemDialog()
    : QWidget()
    , ui(new Ui::CMeasurementSystemDialog)
{
    ui->setupUi(this);

    connect(ui->OPTIMIZE_OBJECTS, &QAbstractButton::clicked,
        this, &CMeasurementSystemDialog::Optimize);

    UpdateObjectLength();

    if (CMeasurementSystem::GetMeasurementSystem().GetStartPointIndex() != CMeasurementSystem::GetMeasurementSystem().GetEndPointIndex())
    {
        UpdateSelectedLength(CMeasurementSystem::GetMeasurementSystem().GetSelectionLength());
    }
    else
    {
        UpdateSelectedLength(0);
    }

    CMeasurementSystem::GetMeasurementSystem().SetMeasurementToolActivation(true);

    // I can't find IDC_MEASUREMENT_PAINT_BTN, to connect to UpdateObjectLength
}

CMeasurementSystemDialog::~CMeasurementSystemDialog()
{
    CMeasurementSystem::GetMeasurementSystem().SetMeasurementToolActivation(false);
}

void CMeasurementSystemDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    AzToolsFramework::RegisterViewPane<CMeasurementSystemDialog>(MEASUREMENT_SYSTEM_WINDOW_NAME, LyViewPane::CategoryOther, options);
}

CMeasurementSystemDialog* CMeasurementSystem::GetMeasurementDialog()
{
    return FindViewPane<CMeasurementSystemDialog>(QLatin1String(MEASUREMENT_SYSTEM_WINDOW_NAME));
}

void CMeasurementSystemDialog::UpdateObjectLength()
{
    CBaseObject* selObj = GetIEditor()->GetSelectedObject();

    if (selObj)
    {
        if (qobject_cast<CRoadObject*>(selObj))
        {
            CRoadObjectEnhanced* selectedObj = (CRoadObjectEnhanced*)selObj;

            int lastIndexEnd = CMeasurementSystem::GetMeasurementSystem().GetEndPointIndex();
            int lastSegmentIndex = selectedObj->GetPointCount();

            if (lastIndexEnd > lastSegmentIndex)
            {
                return;
            }

            // Get total length
            float objectLength = 0;
            selectedObj->GetObjectLength(objectLength);
            UpdateTotalLength(objectLength);
            return;
        }
        ;
    }

    UpdateTotalLength(0.0);
};

void CMeasurementSystemDialog::UpdateSelectedLength(const float& fNewValue)
{
    ui->MEASUREMENT_DIALOG_SEL_LENGTH->setText(QString::number(fNewValue, 'f', 2));
};

void CMeasurementSystemDialog::UpdateTotalLength(const float& fNewValue)
{
    ui->MEASUREMENT_DIALOG_TOTAL_LENGTH->setText(QString::number(fNewValue, 'f', 2));
};
//////////////////////////////////////////////////////////////////////////

void CRoadObjectEnhanced::DrawMeasurementSystemInfo(DisplayContext& dc, const QColor& c)
{
    QColor col = c;

    //Note: texts are currently not translatable in here
    const Matrix34& wtm = GetWorldTM();
    QString segmentDistanceStr;
    float segmentDistance = 0;
    float totalDistance = 0;
    float fPointSize = 0.5f;

    int startPosIndex = CMeasurementSystem::GetMeasurementSystem().GetStartPointIndex();
    int endPosIndex = CMeasurementSystem::GetMeasurementSystem().GetEndPointIndex();

    if (startPosIndex > endPosIndex)
    {
        int iStartValue = startPosIndex;
        startPosIndex = endPosIndex;
        endPosIndex = iStartValue;
    }

    dc.SetColor(col);

    if (endPosIndex == GetPointCount())
    {
        endPosIndex -= 1;
    }

    for (int i = startPosIndex; i < endPosIndex; ++i)
    {
        if (endPosIndex > m_points.size())
        {
            return;
        }

        dc.SetColor(QColor(184, 245, 0));
        Vec3 p0 = wtm.TransformPoint(m_points[i].pos);

        dc.SetLineWidth(8);
        segmentDistance = 0.0f;

        // Draw center line and calculate segment length
        if (i < endPosIndex)
        {
            DrawCenterLine(i, i + 1, segmentDistance, totalDistance, dc, true);
        }

        if (i == startPosIndex)
        {
            dc.SetColor(QColor(184, 245, 0));
        }
        else
        if (i == endPosIndex)
        {
            dc.SetColor(QColor(0, 153, 0));
        }
        else
        {
            dc.SetLineWidth(1);
        }

        float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0) * 0.01f;
        Vec3 sz(fScale, fScale, fScale);
        if (IsSelected())
        {
            dc.DepthTestOff();
        }
        dc.DrawWireBox(p0 - sz, p0 + sz);
        if (IsSelected())
        {
            dc.DepthTestOn();
        }

        // Green
        dc.SetColor(QColor(26, 255, 26));

        // Info drawing point is at the end of the current segment
        p0 = wtm.TransformPoint(m_points[i + 1].pos);

        if (i >= startPosIndex)
        {
            dc.SetLineWidth(1);
            Vec3 vsegmentlDistStrPos = p0;
            segmentDistanceStr = QStringLiteral("Segment %1 Length: %2").arg(i + 1).arg(segmentDistance, 0, 'f', 2, QLatin1Char('0'));
            dc.DrawTextLabel(vsegmentlDistStrPos - sz, 1.3f, segmentDistanceStr.toUtf8().data());

            vsegmentlDistStrPos = p0;
            vsegmentlDistStrPos = Vec3(vsegmentlDistStrPos.x, vsegmentlDistStrPos.y, vsegmentlDistStrPos.z + 0.3f);
            segmentDistanceStr = QStringLiteral("Total Length: %2").arg(totalDistance, 0, 'f', 2, QLatin1Char('0'));
            dc.DrawTextLabel(vsegmentlDistStrPos - sz, 1.3f, segmentDistanceStr.toUtf8().data());

            // Yellow
            dc.SetLineWidth(2);
            dc.SetColor(QColor(184, 245, 0));

            if (i == startPosIndex)
            {
                Vec3 vStartStrPos = wtm.TransformPoint(m_points[startPosIndex].pos);
                vStartStrPos = Vec3(vStartStrPos.x, vStartStrPos.y, vStartStrPos.z + 0.7f);
                dc.DrawTextLabel(vStartStrPos, 1.6f, "Start");
            }

            if (endPosIndex != startPosIndex)
            {
                if (i == endPosIndex - 1)
                {
                    Vec3 vEndStrPos = wtm.TransformPoint(m_points[endPosIndex].pos);
                    vEndStrPos = Vec3(vEndStrPos.x, vEndStrPos.y, vEndStrPos.z + 0.7f);
                    dc.DrawTextLabel(vEndStrPos, 1.6f, "End");
                }
            }
        }
    }

    if ((IsSelected() || IsHighlighted()))
    {
        col = dc.GetSelectedColor();
        dc.SetColor(col);
    }

    float dummy = 0;

    if (startPosIndex != 0)
    {
        DrawSegmentBox(dc, 0, startPosIndex, col);
    }

    if (endPosIndex != m_points.size())
    {
        DrawSegmentBox(dc, endPosIndex, m_points.size(), col);
    }

    // Draw center line before start of segment selection
    if (startPosIndex != 0)
    {
        DrawCenterLine(0, startPosIndex, dummy, dummy, dc, false);
    }

    // Draw remaining line after end of last selected segment till last segment of object.
    DrawCenterLine(endPosIndex, m_points.size(), dummy, dummy, dc, true);

    // Draw other lines showing object width
    CRoadObject::DrawSectorLines(dc);

    CMeasurementSystem::GetMeasurementSystem().SetTotalDistance(totalDistance);

    CMeasurementSystemDialog* dlg = CMeasurementSystem::GetMeasurementSystem().GetMeasurementDialog();
    if (dlg)
    {
        dlg->UpdateSelectedLength(totalDistance);

        float objLength = 0.0;
        GetObjectLength(objLength);
        dlg->UpdateTotalLength(objLength);
    }
}


//////////////////////////////////////////////////////////////////////////
void CRoadObjectEnhanced::DrawCenterLine(const int& startPosIndex, const int& endPosIndex, float& segmentDist, float& totalDist, DisplayContext& dc, bool bDrawMeasurementInfo)
{
    //Note: texts are currently not translatable in here
    const Matrix34& wtm = GetWorldTM();
    float fPointSize = 0.5f;
    QString localDistStr;
    QString segmentNumberStr;

    for (int i = startPosIndex; i < endPosIndex; ++i)
    {
        if (i < m_points.size() - 1)
        {
            int kn = int((GetBezierSegmentLength(i) + 0.5f) / GetStepSize());
            if (kn == 0)
            {
                kn = 1;
            }

            for (int k = 0; k < kn; ++k)
            {
                Vec3 p = wtm.TransformPoint(GetBezierPos(i, float(k) / kn));
                Vec3 p1 = wtm.TransformPoint(GetBezierPos(i, float(k) / kn + 1.0f / kn));
                dc.DrawLine(p, p1);

                if (bDrawMeasurementInfo)
                {
                    if (kn > 1)
                    {
                        if (k == int(kn / 2))
                        {
                            segmentNumberStr = QStringLiteral("Segment %1").arg(i + 1);
                            dc.DrawTextLabel(p, 1.3f, segmentNumberStr.toUtf8().data());
                        }
                    }
                    else
                    if (kn == 1)
                    {
                        float distPos = p1.GetDistance(p);
                        if (distPos > 1)
                        {
                            Vec3 vDispPos = p1 - p;
                            vDispPos = p + vDispPos / 2;
                            segmentNumberStr = QStringLiteral("Segment %1").arg(i + 1);
                            dc.DrawTextLabel(vDispPos, 1.3f, segmentNumberStr.toUtf8().data());
                        }
                    }
                }

                segmentDist += p1.GetDistance(p);
            }
        }
    }

    totalDist += segmentDist;
}

//////////////////////////////////////////////////////////////////////////
void CRoadObjectEnhanced::DrawSegmentBox(DisplayContext& dc, const int& startPosIndex, const int& endPosIndex, const QColor& col)
{
    const Matrix34& wtm = GetWorldTM();
    float fPointSize = 0.5f;
    if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(col);
    }

    for (int i = startPosIndex; i < endPosIndex; ++i)
    {
        Vec3 p0 = wtm.TransformPoint(m_points[i].pos);

        float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0) * 0.01f;
        Vec3 sz(fScale, fScale, fScale);
        if (IsSelected())
        {
            dc.DepthTestOff();
        }
        dc.DrawWireBox(p0 - sz, p0 + sz);
        if (IsSelected())
        {
            dc.DepthTestOn();
        }
    }
}


void CRoadObjectEnhanced::GetSectorLength(const int& sectorIndex, const int& endIndex, float& sectorLength)
{
    if (sectorIndex > GetPointCount())
    {
        return;
    }

    const Matrix34& wtm = GetWorldTM();

    for (int i = sectorIndex; i < endIndex; ++i)
    {
        Vec3 p0 = wtm.TransformPoint(m_points[i].pos);

        if (i < m_points.size() - 1)
        {
            int kn = int((GetBezierSegmentLength(i) + 0.5f) / mv_step);
            if (kn == 0)
            {
                kn = 1;
            }
            for (int k = 0; k < kn; ++k)
            {
                Vec3 p = wtm.TransformPoint(GetBezierPos(i, float(k) / kn));
                Vec3 p1 = wtm.TransformPoint(GetBezierPos(i, float(k) / kn + 1.0f / kn));

                sectorLength += p1.GetDistance(p);
            }
        }
    }
}

void CRoadObjectEnhanced::GetObjectLength(float& objectLength)
{
    GetSectorLength(0, GetPointCount(), objectLength);
}
//////////////////////////////////////////////////////////////////////////

CMeasurementSystem::CMeasurementSystem()
{
    m_startPosIndex = 0;
    m_endPosIndex = 0;
    m_selectionLength = 0;
    m_firstMeasurePointClicked = false;
}

void CMeasurementSystem::ShutdownMeasurementSystem()
{
    CMeasurementSystem::GetMeasurementSystem().SetStartPos(0);
    CMeasurementSystem::GetMeasurementSystem().SetEndPos(0);

    ResetSelectedDistance();
}

bool CMeasurementSystem::ProcessedLButtonClick(int clickedSegmentPointIndex)
{
    if (CMeasurementSystem::GetMeasurementSystem().IsMeasurementToolActivated())
    {
        ResetSelectedDistance();

        if (CMeasurementSystem::GetMeasurementSystem().ShouldStartPointBecomeSelected())
        {
            CMeasurementSystem::GetMeasurementSystem().SetStartPos(clickedSegmentPointIndex);
        }
        else
        {
            CMeasurementSystem::GetMeasurementSystem().SetEndPos(clickedSegmentPointIndex);
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool CMeasurementSystem::ProcessedDblButtonClick(int endPoint)
{
    if (CMeasurementSystem::GetMeasurementSystem().IsMeasurementToolActivated())
    {
        ResetSelectedDistance();

        CMeasurementSystem::GetMeasurementSystem().SetStartPos(0);
        CMeasurementSystem::GetMeasurementSystem().SetEndPos(endPoint);

        return true;
    }
    else
    {
        return false;
    }
}

void CMeasurementSystem::ResetSelectedDistance()
{
    CMeasurementSystemDialog* dlg = CMeasurementSystem::GetMeasurementSystem().GetMeasurementDialog();
    if (dlg)
    {
        dlg->UpdateSelectedLength(0.00);
    }
}
//////////////////////////////////////////////////////////////////////////

void CMeasurementSystemDialog::Optimize()
{
    return;

    I3DEngine* p3DEngine = GetISystem()->GetI3DEngine();

    // iterate through all IStatObj
    int nObjCount = 0;

    p3DEngine->GetLoadedStatObjArray(0, nObjCount);

    if (nObjCount > 0)
    {
        std::vector<IStatObj*> pObjects;
        pObjects.resize(nObjCount);
        p3DEngine->GetLoadedStatObjArray(&pObjects[0], nObjCount);

        for (int nCurObj = 0; nCurObj < nObjCount; ++nCurObj)
        {
            IStatObj* statObj = pObjects[nCurObj];

            if (!statObj)
            {
                continue;
            }


            int nMinLod = 8;

            for (int i = 0; i < nMinLod; ++i)
            {
                IStatObj* pStatObjLOD = (IStatObj*)statObj->GetLodObject(i);

                if (!pStatObjLOD)
                {
                    continue;
                }

                IRenderMesh* renderMesh = pStatObjLOD->GetRenderMesh();

                if (!renderMesh)
                {
                    continue;
                }


                int nIndicesOld = renderMesh->GetIndicesCount();
                vtx_idx* idxPtrOld = renderMesh->GetIndexPtr(FSL_READ);

                if (!idxPtrOld)
                {
                    continue;
                }


                // get prev veritces
                int nVerticesOld = renderMesh->GetVerticesCount();
                int32 nStrideOld = 0;
                byte* posPtrOld = renderMesh->GetPosPtr(nStrideOld, FSL_READ);

                if (!posPtrOld)
                {
                    continue;
                }

                // get prev UVs
                int32 nUVStrideOld = 0;
                byte* pUVPtrOld = renderMesh->GetUVPtr(nUVStrideOld, FSL_READ);

                vtx_idx* pNewIndices = new vtx_idx [nIndicesOld];
                Vec3* pNewVerts = new Vec3[nVerticesOld];
                Vec2* pNewUV = new Vec2[nVerticesOld];

                int nNewVertexCount = nVerticesOld >> 1;
                int nNewIndices = nIndicesOld >> 1;

                for (int k = 0; k < nVerticesOld; k++)
                {
                    const Vec3& vOld = *(Vec3*)&posPtrOld[nStrideOld * k];
                    pNewVerts[k] = vOld;
                    const Vec2& vUVOld = *(Vec2*)&pUVPtrOld[nUVStrideOld * k];
                    pNewUV[k] = vUVOld;
                } //k

                for (int k = 0; k < nIndicesOld; k++)
                {
                    pNewIndices[k] = idxPtrOld[k];
                } //k

                // resize
                renderMesh->UpdateVertices(NULL, nNewVertexCount, 0, VSF_GENERAL, 0u);

                int nPosStride = 0;
                byte* pVertPos = renderMesh->GetPosPtr(nPosStride, FSL_SYSTEM_CREATE);
                if (!pVertPos)
                {
                    continue;
                }

                int nUVStride = 0;
                byte* pVertTexUV = renderMesh->GetUVPtr(nUVStride, FSL_SYSTEM_CREATE);

                if (!pVertTexUV)
                {
                    continue;
                }

                // set new verts
                for (int k = 0; k < nNewVertexCount; k++)
                {
                    *((Vec3*)pVertPos) = pNewVerts[k];
                    *((Vec2*)pVertTexUV) = pNewUV[k];

                    pVertPos += nPosStride;
                    pVertTexUV += nUVStride;
                }


                renderMesh->UnlockStream(VSF_GENERAL);
                //renderMesh->UnlockStream(VSF_TANGENTS);
                renderMesh->UpdateIndices(pNewIndices, nNewIndices, 0, 0u);

                SAFE_DELETE_ARRAY(pNewUV);
                SAFE_DELETE_ARRAY(pNewVerts);
                SAFE_DELETE_ARRAY(pNewIndices);

                // Update chunk params.
                CRenderChunk* pChunk = &renderMesh->GetChunks()[0];
                //if (m_pMaterial)
                //  pChunk->m_nMatFlags = m_pMaterial->GetFlags();
                pChunk->nNumIndices = nNewIndices;
                pChunk->nNumVerts = nNewVertexCount;
                renderMesh->SetChunk(0, *pChunk);
            }
        }
    }
}

#include <MeasurementSystem/MeasurementSystem.moc>