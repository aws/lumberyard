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

// Description : Terrain Modification Tool implementation.


#include "StdAfx.h"
#include "TerrainMoveTool.h"
#include "TerrainMoveToolPanel.h"
#include "Viewport.h"
#include "VegetationMap.h"
#include "Terrain/TerrainManager.h"
#include "CryEditDoc.h"
#include "MainWindow.h"

#include "ITransformManipulator.h"

#include "TrackView/TrackViewSequenceManager.h"

#include <QPushButton>
#include <QMessageBox>

#include "QtUtil.h"

namespace {
    QRect GetRectMargin(const QRectF& rc, const QRectF& rcCrop, ImageRotationDegrees rotation)
    {
        QRectF rcMar(QPointF(rcCrop.left() - rc.left(), rcCrop.top() - rc.top()), QPointF(rc.right() - rcCrop.right(), rc.bottom() - rcCrop.bottom()));
        if (rotation == ImageRotationDegrees::Rotate90)
        {
            return QRectF(QPointF(rcMar.top(), rcMar.right()), QPointF(rcMar.bottom(), rcMar.left())).toRect();
        }
        if (rotation == ImageRotationDegrees::Rotate180)
        {
            return QRectF(QPointF(rcMar.right(), rcMar.bottom()), QPointF(rcMar.left(), rcMar.top())).toRect();
        }
        if (rotation == ImageRotationDegrees::Rotate270)
        {
            return QRectF(QPointF(rcMar.bottom(), rcMar.left()), QPointF(rcMar.top(), rcMar.right())).toRect();
        }
        return rcMar.toRect();
    }

    QRect CropRect(const QRect& rc, const QRect& rcMar)
    {
        return QRect(QPoint(rc.left() + rcMar.left(), rc.top() + rcMar.top()), QPoint(rc.right() - rcMar.right() -1, rc.bottom() - rcMar.bottom() -1));
    }
}



//////////////////////////////////////////////////////////////////////////
//class CUndoTerrainMoveTool

class CUndoTerrainMoveTool
    : public IUndoObject
{
public:
    CUndoTerrainMoveTool(CTerrainMoveTool* pMoveTool)
    {
        m_posSourceUndo = pMoveTool->m_source.pos;
        m_posTargetUndo = pMoveTool->m_target.pos;
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return ""; };

    virtual void Undo(bool bUndo)
    {
        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (!pTool || !qobject_cast<CTerrainMoveTool*>(pTool))
        {
            return;
        }
        CTerrainMoveTool* pMoveTool = (CTerrainMoveTool*)pTool;
        if (bUndo)
        {
            m_posSourceRedo = pMoveTool->m_source.pos;
            m_posTargetRedo = pMoveTool->m_target.pos;
        }
        pMoveTool->m_source.pos = m_posSourceUndo;
        pMoveTool->m_target.pos = m_posTargetUndo;
        if (pMoveTool->m_source.isSelected)
        {
            pMoveTool->Select(1);
        }
        if (pMoveTool->m_target.isSelected)
        {
            pMoveTool->Select(2);
        }
    }
    virtual void Redo()
    {
        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (!pTool || !qobject_cast<CTerrainMoveTool*>(pTool))
        {
            return;
        }
        CTerrainMoveTool* pMoveTool = (CTerrainMoveTool*)pTool;
        pMoveTool->m_source.pos = m_posSourceRedo;
        pMoveTool->m_target.pos = m_posTargetRedo;
        if (pMoveTool->m_source.isSelected)
        {
            pMoveTool->Select(1);
        }
        if (pMoveTool->m_target.isSelected)
        {
            pMoveTool->Select(2);
        }
    }
private:
    Vec3 m_posSourceRedo;
    Vec3 m_posTargetRedo;
    Vec3 m_posSourceUndo;
    Vec3 m_posTargetUndo;
};




//////////////////////////////////////////////////////////////////////////
//class CTerrainMoveTool

Vec3 CTerrainMoveTool::m_dym(512, 512, 1024);
ImageRotationDegrees CTerrainMoveTool::m_targetRot = ImageRotationDegrees::Rotate0;
//SMTBox CTerrainMoveTool::m_source;
//SMTBox CTerrainMoveTool::m_target;

//////////////////////////////////////////////////////////////////////////
CTerrainMoveTool::CTerrainMoveTool()
    : m_archive(0)
    , m_isSyncHeight(false)
    , m_panelId(0)
    , m_panel(0)
{
}

//////////////////////////////////////////////////////////////////////////
CTerrainMoveTool::~CTerrainMoveTool()
{
    if (m_archive)
    {
        delete m_archive;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::BeginEditParams(IEditor* ie, int flags)
{
    m_ie = ie;

    CUndo undo("Unselect All");
    GetIEditor()->ClearSelection();

    if (!m_panelId)
    {
        m_panel = new CTerrainMoveToolPanel(this);
        m_panelId = m_ie->AddRollUpPage(ROLLUP_TERRAIN, "Terrain Move Tool", m_panel);
        MainWindow::instance()->setFocus();
    }

    if (m_source.isCreated)
    {
        m_source.isShow = true;
    }
    if (m_target.isCreated)
    {
        m_target.isShow = true;
    }

    //CPoint hmapSrcMin,hmapSrcMax;
    //hmapSrcMin = GetIEditor()->GetHeightmap()->WorldToHmap(srcBox.min);
    //hmapSrcMax = GetIEditor()->GetHeightmap()->WorldToHmap(srcBox.max);
    //m_srcRect.SetRect( hmapSrcMin,hmapSrcMax );

    //GetIEditor()->GetHeightmap()->ExportBlock( m_srcRect,*m_archive );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::EndEditParams()
{
    if (m_panelId)
    {
        m_ie->RemoveRollUpPage(ROLLUP_TERRAIN, m_panelId);
        m_panelId = 0;
        m_panel = 0;
    }
    Select(0);
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainMoveTool::MouseCallback(CViewport* pView, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove)
    {
        if (m_source.isSelected && !m_source.isCreated || m_target.isSelected && !m_target.isCreated)
        {
            Vec3 pos = pView->SnapToGrid(pView->ViewToWorld(point));
            if (pos == Vec3(0, 0, 0))
            {
                // collide with Z=0 plane
                Vec3 src, dir;
                pView->ViewToWorldRay(point, src, dir);
                if (fabs(dir.z) > 0.00001f)
                {
                    pos = pView->SnapToGrid(src - (src.z / dir.z) * dir);
                }
            }

            if (m_source.isSelected)
            {
                m_source.pos = pos;
                UpdateOffsetDisplay();
                Select(1);
            }
            if (m_target.isSelected)
            {
                m_target.pos = pos;
                UpdateOffsetDisplay();
                Select(2);
                if (m_isSyncHeight)
                {
                    m_target.pos.z = m_source.pos.z;
                }
            }
        }
    }
    else if (event == eMouseLDown)
    {
        // Close tool.
        if (!m_source.isSelected && !m_target.isSelected)
        {
            GetIEditor()->SetEditTool(0);
        }
    }
    else if (event == eMouseLUp)
    {
        if (m_source.isSelected && !m_source.isCreated)
        {
            m_source.isCreated = true;
            m_panel->UpdateButtons();
        }
        else if (m_target.isSelected && !m_target.isCreated)
        {
            m_target.isCreated = true;
            m_panel->UpdateButtons();
        }
        else
        {
            GetIEditor()->AcceptUndo("Move Box");
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::Display(DisplayContext& dc)
{
    if (m_source.isShow)
    {
        dc.SetColor(QColor(255, 128, 128), 1);
        if (m_source.isSelected)
        {
            dc.SetColor(dc.GetSelectedColor());
        }
        dc.DrawWireBox(m_source.pos - m_dym / 2, m_source.pos + m_dym / 2);
    }

    if (m_target.isShow)
    {
        dc.SetColor(QColor(128, 128, 255), 1);
        if (m_target.isSelected)
        {
            dc.SetColor(dc.GetSelectedColor());
        }

        Matrix34 tm;
        tm.SetIdentity();
        switch (m_targetRot)
        {
            case ImageRotationDegrees::Rotate90:
                tm.SetRotationZ(gf_halfPI);
                break;
            case ImageRotationDegrees::Rotate180:
                tm.SetRotationZ(gf_PI);
                break;
            case ImageRotationDegrees::Rotate270:
                tm.SetRotationZ(gf_PI + gf_halfPI);
                break;
            default:
                break;
        }
        tm.SetTranslation(m_target.pos);
        dc.PushMatrix(tm);
        dc.DrawWireBox(-m_dym / 2, m_dym / 2);
        dc.PopMatrix();
    }

    /*
    BBox box;
    GetIEditor()->GetSelectedRegion(box);

    Vec3 p1 = GetIEditor()->GetHeightmap()->HmapToWorld( CPoint(m_srcRect.left,m_srcRect.top) );
    Vec3 p2 = GetIEditor()->GetHeightmap()->HmapToWorld( CPoint(m_srcRect.right,m_srcRect.bottom) );

    Vec3 ofs = m_pointerPos - p1;
    p1 += ofs;
    p2 += ofs;

    dc.SetColor( QColor(0,0,255) );
    dc.DrawTerrainRect( p1.x,p1.y,p2.x,p2.y,0.2f );
    */
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainMoveTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    bool bProcessed = false;
    return bProcessed;
}


//////////////////////////////////////////////////////////////////////////
bool CTerrainMoveTool::OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::Move(bool isCopy, bool bOnlyVegetation, bool bOnlyTerrain)
{
    enum
    {
        eInit = 0,
        eCancel = 1,
        eIgnore = 2,
        eSave = 3,
        eHold = 4,
    };

    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Warning);
    mbox.setWindowTitle(QObject::tr("Terrain Move Area"));
    mbox.setText(QObject::tr("Can't store complete undo for this operation. If Ignore is chosen, only the object movement will be undoable."));
    const auto cancelBtn = mbox.addButton(QMessageBox::Cancel);
    const auto ignoreBtn = mbox.addButton(QMessageBox::Ignore);
    const auto saveBtn = mbox.addButton(QMessageBox::Save);
    const auto holdBtn = mbox.addButton(QObject::tr("Hold"), QMessageBox::ActionRole);
    mbox.setEscapeButton(QMessageBox::Cancel);
    mbox.exec();

    if (mbox.clickedButton() == cancelBtn)
    {
        return;
    }
    else if (mbox.clickedButton() == saveBtn)
    {
        GetIEditor()->GetDocument()->Save();
    }
    else if (mbox.clickedButton() == holdBtn)
    {
        GetIEditor()->GetDocument()->Hold(HOLD_FETCH_FILE);
    }
    else if (mbox.clickedButton() == ignoreBtn)
    {
        // Just do moving without holding or saving
    }

    // Move terrain area.
    CUndo undo("Copy Area");
    QWaitCursor wait;

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (!pHeightmap || !pHeightmap->GetHeight() || !pHeightmap->GetWidth())
    {
        return;
    }

    AABB srcBox(m_source.pos - m_dym / 2, m_source.pos + m_dym / 2);

    LONG bx1 = pHeightmap->WorldToHmap(srcBox.min).x();
    LONG by1 = pHeightmap->WorldToHmap(srcBox.min).y();
    LONG bx2 = pHeightmap->WorldToHmap(srcBox.max).x();
    LONG by2 = pHeightmap->WorldToHmap(srcBox.max).y();

    LONG xc = (bx1 + bx2) / 2;
    LONG yc = (by1 + by2) / 2;

    LONG cx1 = bx1;
    LONG cy1 = by1;

    QPoint hmapPosStart = pHeightmap->WorldToHmap(m_target.pos - m_dym / 2) - QPoint(cx1, cy1);

    if (m_targetRot == ImageRotationDegrees::Rotate90 || m_targetRot == ImageRotationDegrees::Rotate270)
    {
        cx1 = xc - (by2 - by1) / 2;
        cy1 = yc - (bx2 - bx1) / 2;
        hmapPosStart = pHeightmap->WorldToHmap(m_target.pos - Vec3(m_dym.y / 2, m_dym.x / 2, m_dym.z)) - QPoint(cx1, cy1);
    }

#ifdef WIN64
    LONG blocksize = 4096;
#else
    LONG blocksize = 512;
#endif

    for (LONG by = by1; by < by2; by += blocksize)
    {
        for (LONG bx = bx1; bx < bx2; bx += blocksize)
        {
            LONG cx = bx;
            LONG cy = by;

            if (m_targetRot == ImageRotationDegrees::Rotate90)
            {
                cx = xc - (yc - by);
                cy = yc + (xc - bx) - blocksize;
                if (cy < cy1)
                {
                    cy = cy1;
                }
            }
            else if (m_targetRot == ImageRotationDegrees::Rotate270)
            {
                cx = xc + (yc - by) - blocksize;
                cy = yc - (xc - bx);
                if (cx < cx1)
                {
                    cx = cx1;
                }
            }
            else if (m_targetRot == ImageRotationDegrees::Rotate180)
            {
                cx = bx2 - 1 + bx1 - bx - blocksize;
                if (cx < bx1)
                {
                    cx = bx1;
                }

                cy = by2 - 1 + by1 - by - blocksize;
                if (cy < by1)
                {
                    cy = by1;
                }
            }

            // Move terrain heightmap block.

            QRect rcHeightmap(0, 0, pHeightmap->GetWidth(), pHeightmap->GetHeight());

            // left-top position of destination rectangle in Heightmap space
            QPoint posDst = hmapPosStart + QPoint(cx, cy);

            // source rectangle in Heightmap space
            QRect rcSrc(QPoint(bx, by), QPoint(min(bx + blocksize + 1, bx2), min(by + blocksize + 1, by2)) - QPoint(1, 1));
            // destination rectangle in Heightmap space
            QRect rcDst(posDst, rcSrc.size());
            if (m_targetRot == ImageRotationDegrees::Rotate90 || m_targetRot == ImageRotationDegrees::Rotate270)
            {
                rcDst.setRight(posDst.x() + rcSrc.height() - 1);
                rcDst.setBottom(posDst.y() + rcSrc.width() - 1);
            }

            // Crop destination region by terrain outside
            ImageRotationDegrees negativeRotation;
            switch (m_targetRot)
            {
                case ImageRotationDegrees::Rotate90:
                    negativeRotation = ImageRotationDegrees::Rotate270;
                    break;
                case ImageRotationDegrees::Rotate270:
                    negativeRotation = ImageRotationDegrees::Rotate90;
                    break;
                default:
                    // If we're rotating 0 or 180 degrees, negative rotation is still 0 or 180 degrees.
                    negativeRotation = m_targetRot;
                    break;
            }

            QRect rcCropDst(rcDst & rcHeightmap);
            QRect rcMar = GetRectMargin(rcDst, rcCropDst, negativeRotation);
            QRect rcCropSrc = CropRect(rcSrc, rcMar); // sync src with dst

            if (!rcCropSrc.isEmpty())
            {
                if (m_archive)
                {
                    delete m_archive;
                }
                m_archive = new CXmlArchive("Root");

                // don't need crop by source outside for Heightmap moving (only destination crop)
                // data (height values) from source outside will be filled by 0
                m_archive->bLoading = false;
                pHeightmap->ExportBlock(rcCropSrc, *m_archive, false);

                m_archive->bLoading = true;
                pHeightmap->ImportBlock(*m_archive, rcCropDst.topLeft(), true, (m_target.pos - m_source.pos).z, bOnlyVegetation && !bOnlyTerrain, m_targetRot);
            }

            // Crop source region by terrain outside
            rcCropSrc &= rcHeightmap;
            rcMar = GetRectMargin(rcSrc, rcCropSrc, m_targetRot);
            rcCropDst = CropRect(rcDst, rcMar); // sync dst with src

            if (!rcCropSrc.isEmpty() && (!bOnlyVegetation || bOnlyTerrain))
            {
                CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();
                uint32 nRes = pRGBLayer->CalcMaxLocalResolution((float)rcCropSrc.left() / pHeightmap->GetWidth(), (float)rcCropSrc.top() / pHeightmap->GetHeight(), (float)(rcCropSrc.right() + 1) / pHeightmap->GetWidth(), (float)(rcCropSrc.bottom() + 1) / pHeightmap->GetHeight());

                {
                    CImageEx image;
                    CImageEx tmpImage;
                    image.Allocate((rcCropSrc.width()) * nRes / pHeightmap->GetWidth(), (rcCropSrc.height()) * nRes / pHeightmap->GetHeight());
                    pRGBLayer->GetSubImageStretched(
                        (float)rcCropSrc.left() / pHeightmap->GetWidth(), (float)rcCropSrc.top() / pHeightmap->GetHeight(),
                        (float)(rcCropSrc.right() + 1) / pHeightmap->GetWidth(), (float)(rcCropSrc.bottom() + 1) / pHeightmap->GetHeight(), image);

                    if (m_targetRot != ImageRotationDegrees::Rotate0)
                    {
                        tmpImage.RotateOrt(image, m_targetRot);
                    }

                    pRGBLayer->SetSubImageStretched(
                        ((float)rcCropDst.left()) / pHeightmap->GetWidth(), ((float)rcCropDst.top()) / pHeightmap->GetHeight(),
                        ((float)rcCropDst.right() + 1) / pHeightmap->GetWidth(), ((float)(rcCropDst.bottom() + 1)) / pHeightmap->GetHeight(), 
                        (m_targetRot != ImageRotationDegrees::Rotate0) ? tmpImage : image);
                }

                for (int x = 0; x < rcCropSrc.width(); x++)
                {
                    for (int y = 0; y < rcCropSrc.height(); y++)
                    {
                        LayerWeight weight = pHeightmap->GetLayerWeightAt(x + rcCropSrc.left(), y + rcCropSrc.top());
                        if (m_targetRot == ImageRotationDegrees::Rotate90)
                        {
                            pHeightmap->SetLayerWeightAt(rcCropDst.right() + 1 - y, x, weight);
                        }
                        else if (m_targetRot == ImageRotationDegrees::Rotate180)
                        {
                            pHeightmap->SetLayerWeightAt(rcCropDst.right() + 1 - x, rcCropDst.bottom() + 1 - y, weight);
                        }
                        else if (m_targetRot == ImageRotationDegrees::Rotate270)
                        {
                            pHeightmap->SetLayerWeightAt(y, rcCropDst.bottom() + 1 - x, weight);
                        }
                        else
                        {
                            pHeightmap->SetLayerWeightAt(x + rcCropDst.left(), y + rcCropDst.top(), weight);
                        }
                    }
                }

                pHeightmap->UpdateLayerTexture(rcCropDst);
            }
            if (m_archive)
            {
                delete m_archive;
                m_archive = 0;
            }
        }
    }


    if (bOnlyVegetation || (!bOnlyVegetation && !bOnlyTerrain))
    {
        GetIEditor()->GetVegetationMap()->RepositionArea(srcBox, m_target.pos - m_source.pos, m_targetRot, isCopy);
    }

    if (!isCopy && (!bOnlyVegetation && !bOnlyTerrain))
    {
        GetIEditor()->GetObjectManager()->MoveObjects(srcBox, m_target.pos - m_source.pos, m_targetRot, isCopy);
        OffsetTrackViewPositionKeys(m_target.pos - m_source.pos);
    }

    /*
    // Load selection from archive.
    XmlNodeRef objRoot = m_archive->root->findChild("Objects");
    if (objRoot)
    {
        GetIEditor()->ClearSelection();
        CObjectArchive ar( GetIEditor()->GetObjectManager(),objRoot,true );
        GetIEditor()->GetObjectManager()->LoadObjects( ar,false );
    }
    */
    GetIEditor()->ClearSelection();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::SetArchive(CXmlArchive* ar)
{
    if (m_archive)
    {
        delete m_archive;
    }
    m_archive = ar;

    int x1, y1, x2, y2;
    // Load rect size our of archive.
    m_archive->root->getAttr("X1", x1);
    m_archive->root->getAttr("Y1", y1);
    m_archive->root->getAttr("X2", x2);
    m_archive->root->getAttr("Y2", y2);

    m_srcRect = QRect(QPoint(x1, y1), QPoint(x2, y2));
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::Select(int nBox)
{
    m_source.isSelected = false;
    m_target.isSelected = false;

    if (nBox == 0)
    {
        GetIEditor()->ShowTransformManipulator(false);
        m_source.isShow = false;
        m_target.isShow = false;
    }

    if (nBox == 1)
    {
        m_source.isSelected = true;
        m_source.isShow = true;
        ITransformManipulator* pManipulator = GetIEditor()->ShowTransformManipulator(true);
        Matrix34 tm;
        tm.SetIdentity();
        tm.SetTranslation(m_source.pos);
        pManipulator->SetTransformation(COORDS_LOCAL, tm);
        pManipulator->SetTransformation(COORDS_PARENT, tm);
        pManipulator->SetTransformation(COORDS_USERDEFINED, tm);
    }

    if (nBox == 2)
    {
        m_target.isSelected = true;
        m_target.isShow = true;
        ITransformManipulator* pManipulator = GetIEditor()->ShowTransformManipulator(true);
        Matrix34 tm;
        tm.SetIdentity();
        switch (m_targetRot)
        {
            case ImageRotationDegrees::Rotate90:
                tm.SetRotationZ(gf_halfPI);
                break;
            case ImageRotationDegrees::Rotate180:
                tm.SetRotationZ(gf_PI);
                break;
            case ImageRotationDegrees::Rotate270:
                tm.SetRotationZ(gf_PI + gf_halfPI);
                break;
            default:
                break;
        }
        tm.SetTranslation(m_target.pos);
        pManipulator->SetTransformation(COORDS_LOCAL, tm);
        pManipulator->SetTransformation(COORDS_PARENT, tm);
        pManipulator->SetTransformation(COORDS_USERDEFINED, tm);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const Vec3& value)
{
    int editMode = GetIEditor()->GetEditMode();
    if (editMode == eEditModeMove)
    {
        CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
        GetIEditor()->RestoreUndo();

        Vec3 pos = m_source.pos;
        Vec3 val = value;

        Vec3 max = pHeightmap->HmapToWorld(QPoint(pHeightmap->GetWidth(), pHeightmap->GetHeight()));

        uint32 wid = max.x;
        uint32 hey = max.y;

        if (m_target.isSelected)
        {
            pos = m_target.pos;
        }

        pManipulator = GetIEditor()->ShowTransformManipulator(true);
        Matrix34 tm = pManipulator->GetTransformation(COORDS_LOCAL);

        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoTerrainMoveTool(this));
        }

        if (m_source.isSelected)
        {
            m_source.pos += val;
            UpdateOffsetDisplay();
            if (m_isSyncHeight)
            {
                m_target.pos.z = m_source.pos.z;
            }
        }

        if (m_target.isSelected)
        {
            m_target.pos += val;
            UpdateOffsetDisplay();
            if (m_isSyncHeight)
            {
                m_source.pos.z = m_target.pos.z;
            }
        }

        tm.SetTranslation(pos + val);
        pManipulator->SetTransformation(COORDS_LOCAL, tm);
        pManipulator->SetTransformation(COORDS_PARENT, tm);
        pManipulator->SetTransformation(COORDS_USERDEFINED, tm);
    }
}


//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::SetDym(Vec3 dym)
{
    m_dym = dym;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::SetTargetRot(ImageRotationDegrees targetRot)
{
    m_targetRot = targetRot;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::OffsetTrackViewPositionKeys(const Vec3& offset)
{
    const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
    const unsigned int numSequences = pSequenceManager->GetCount();

    for (unsigned int sequenceIndex = 0; sequenceIndex < numSequences; ++sequenceIndex)
    {
        CTrackViewSequence* pSequence = pSequenceManager->GetSequenceByIndex(sequenceIndex);
        CTrackViewTrackBundle trackBundle = pSequence->GetAllTracks();

        const unsigned int numTracks = trackBundle.GetCount();
        for (unsigned int trackIndex = 0; trackIndex < numTracks; ++trackIndex)
        {
            CTrackViewTrack* pTrack = trackBundle.GetTrack(trackIndex);
            pTrack->OffsetKeyPosition(offset);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::SetSyncHeight(bool isSyncHeight)
{
    m_isSyncHeight = isSyncHeight;
    if (m_isSyncHeight)
    {
        m_target.pos.z = m_source.pos.z;
        if (m_target.isSelected)
        {
            Select(2);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::UpdateOffsetDisplay()
{
    const bool bCanDraw = m_source.isShow && m_target.isShow;
    m_panel->UpdateOffsetText(m_target.pos - m_source.pos, !bCanDraw);
}

#include <TerrainMoveTool.moc>