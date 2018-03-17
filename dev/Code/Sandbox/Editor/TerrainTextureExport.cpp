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
#include "TerrainTextureExport.h"
#include "Terrain/TerrainTexGen.h"
#include "WaitProgress.h"
#include "CryEditDoc.h"
#include "ResizeResolutionDialog.h"
#include "Viewport.h"
#include "Terrain/TerrainManager.h"
#include "TerrainLighting.h"
#include <ui_TerrainTextureExport.h>

#include <QtUtil.h>

#include <QMessageBox>
#include <QPainter>
#include <QMouseEvent>
#include <QPixmap>
#include <QApplication>
#include <QCursor>
#if defined(Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif

namespace {
    static int TERRAIN_PREVIEW_RESOLUTION = 256;
}

/////////////////////////////////////////////////////////////////////////////
// Utility method

static void PreviewToTile(uint32& outX, uint32& outY, uint32 x, uint32 y)
{
    uint32 dwTileCountY = GetIEditor()->GetTerrainManager()->GetRGBLayer()->GetTileCountY();

    // rotate 90 deg
    outX = dwTileCountY - 1 - y;
    outY = x;
}

/////////////////////////////////////////////////////////////////////////////
// TerrainTextureWidget

class TerrainTextureWidget
    : public QWidget
{
public:
    TerrainTextureWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_bSelectMode(false)
    {
        setFixedSize(TERRAIN_PREVIEW_RESOLUTION, TERRAIN_PREVIEW_RESOLUTION);

        m_pTexGen = new CTerrainTexGen;
        m_pTexGen->Init(TERRAIN_PREVIEW_RESOLUTION, true);
    }

    ~TerrainTextureWidget()
    {
        delete m_pTexGen;
    }

    QRect GetSelection() const{ return m_selection;  }

protected:
    void paintEvent(QPaintEvent* event)
    {
        CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();
        int dwTileCountX = pRGBLayer->GetTileCountX();
        int dwTileCountY = pRGBLayer->GetTileCountY();
        if (dwTileCountX == 0 || dwTileCountY == 0)
        {
            return;
        }

        QPainter p(this);

        // Generate a preview if we don't have one
        if (m_lightMap.isNull())
        {
            Generate();
        }

        // Draw the preview
        p.drawImage(1, 1, m_lightMap);

        // Draw the tiles
        p.setPen(qRgb(127, 127, 127));
        for (uint32 x = 0; x < dwTileCountX; x++)
        {
            for (uint32 y = 0; y < dwTileCountY; y++)
            {
                QRect rc = {
                    int(x) * TERRAIN_PREVIEW_RESOLUTION / dwTileCountX, int(y) * TERRAIN_PREVIEW_RESOLUTION / dwTileCountY,
                    TERRAIN_PREVIEW_RESOLUTION / dwTileCountX, TERRAIN_PREVIEW_RESOLUTION / dwTileCountY
                };

                uint32 dwTileX;
                uint32 dwTileY;
                PreviewToTile(dwTileX, dwTileY, x, y);
                uint32 dwLocalSize = pRGBLayer->GetTileResolution(dwTileX, dwTileY);
                p.setPen(qRgb(SATURATEB(dwLocalSize / 8), 0, 0));

                if (!m_selection.isEmpty() && m_selection.left() <= x && m_selection.right() >= x && m_selection.top() <= y && m_selection.bottom() >= y)
                {
                    rc.adjust(4, 4, -4, -4);
                    p.fillRect(rc, qRgb(128, 128, 128));
                }

                QString str;
                if (dwLocalSize == 1024)
                {
                    str = QLatin1String("1k");
                }
                else if (dwLocalSize == 2048)
                {
                    str = QLatin1String("2k");
                }
                else if (dwLocalSize == 4096)
                {
                    str = QLatin1String("4k");
                }
                else
                {
                    str = QString::number(dwLocalSize);
                }
                p.drawText(rc, Qt::AlignCenter, str);
            }
        }

        p.setPen(Qt::green);
        qreal dpos = 0;
        qreal dstep = (qreal) (TERRAIN_PREVIEW_RESOLUTION - 1) / (qreal)dwTileCountX;
        for (float x = 0; x <= dwTileCountX; x++, dpos += dstep)
        {
            p.drawLine(dpos, 0, dpos, TERRAIN_PREVIEW_RESOLUTION);
        }
        dpos = 0;
        dstep = (qreal)(TERRAIN_PREVIEW_RESOLUTION - 1) / (qreal)dwTileCountY;
        for (float y = 0; y <= dwTileCountY; y++, dpos += dstep)
        {
            p.drawLine(0, dpos, TERRAIN_PREVIEW_RESOLUTION, dpos);
        }

        if (!m_selection.isNull())
        {
            QRect rc;
            rc.setLeft(m_selection.left() * TERRAIN_PREVIEW_RESOLUTION / dwTileCountX + 1);
            rc.setTop(m_selection.top() * TERRAIN_PREVIEW_RESOLUTION / dwTileCountY + 1);
            rc.setRight((m_selection.right() + 1) * TERRAIN_PREVIEW_RESOLUTION / dwTileCountX - 2);
            rc.setBottom((m_selection.bottom() + 1) * TERRAIN_PREVIEW_RESOLUTION / dwTileCountY - 2);

            p.setPen(qRgb(255, 64, 64));
            p.drawRect(rc);
        }
    }

    void mousePressEvent(QMouseEvent* event)
    {
        QWidget::mousePressEvent(event);

        if (event->button() != Qt::LeftButton)
        {
            return;
        }

        CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();

        uint32 dwTileCountX = pRGBLayer->GetTileCountX();
        uint32 dwTileCountY = pRGBLayer->GetTileCountY();
        if (dwTileCountX == 0 || dwTileCountY == 0)
        {
            return;
        }

        QPoint point = event->pos();
        int x = point.x() * dwTileCountX / TERRAIN_PREVIEW_RESOLUTION;
        int y = point.y() * dwTileCountY / TERRAIN_PREVIEW_RESOLUTION;

        if (x >= 0 && y >= 0 && x < dwTileCountX && y < dwTileCountY)
        {
            if (m_selection.left() == x && m_selection.top() == y && m_selection.right() == x && m_selection.bottom() == y)
            {
                m_selection = QRect();
            }
            else
            {
                m_selection.setRect(x, y, 1, 1);
                m_bSelectMode = true;
            }
        }

        update();
    }

    void mouseReleaseEvent(QMouseEvent* event)
    {
        if (m_bSelectMode)
        {
            m_bSelectMode = false;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event)
    {
        QWidget::mouseMoveEvent(event);

        if (!m_bSelectMode)
        {
            return;
        }

        CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();

        uint32 dwTileCountX = pRGBLayer->GetTileCountX();
        uint32 dwTileCountY = pRGBLayer->GetTileCountY();

        QPoint point = event->pos();
        int x = point.x() * dwTileCountX / TERRAIN_PREVIEW_RESOLUTION;
        int y = point.y() * dwTileCountY / TERRAIN_PREVIEW_RESOLUTION;

        if (x >= 0 && y >= 0 && x >= m_selection.left() && y >= m_selection.top() && x < dwTileCountX && y < dwTileCountY
            && (x != m_selection.right() || y != m_selection.bottom()))
        {
            m_selection.setRight(x);
            m_selection.setBottom(y);
            update();
        }
    }

private:
    void Generate()
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        LightingSettings* ls = GetIEditor()->GetDocument()->GetLighting();

        // Calculate the lighting.
        m_terrain.Allocate(TERRAIN_PREVIEW_RESOLUTION, TERRAIN_PREVIEW_RESOLUTION);
        m_pTexGen->InvalidateLighting();
        m_pTexGen->GenerateSurfaceTexture(ETTG_LIGHTING | ETTG_QUIET | ETTG_BAKELIGHTING | ETTG_NOTEXTURE | ETTG_SHOW_WATER, m_terrain);

        // put terrain into m_lightMap. This does NOT copy the bits from m_terrain, so m_terrain must stay around
        m_lightMap = QImage((uchar*)m_terrain.GetData(), m_terrain.GetWidth(), m_terrain.GetHeight(), QImage::Format_RGBX8888);

        QApplication::restoreOverrideCursor();
    }
private:
    CTerrainTexGen* m_pTexGen;
    CImageEx m_terrain;

    QImage m_lightMap;
    QRect m_selection;
    bool m_bSelectMode;
};


/////////////////////////////////////////////////////////////////////////////
// CTerrainTextureExport dialog

//CDC CTerrainTextureExport::m_dcLightmap;


CTerrainTextureExport::CTerrainTextureExport(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , ui(new Ui::CTerrainTextureExport)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint | Qt::WindowSystemMenuHint));
    m_terrainTexture = new TerrainTextureWidget(ui->terrainTextureFrame);
    setFixedSize(sizeHint());

    connect(ui->exportButton, &QPushButton::clicked, this, &CTerrainTextureExport::OnExport);
    connect(ui->importButton, &QPushButton::clicked, this, &CTerrainTextureExport::OnImport);
    connect(ui->CHANGE, &QPushButton::clicked, this, &CTerrainTextureExport::OnChangeResolutionBtn);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &QDialog::accept);

    ui->FILE->setText(gSettings.terrainTextureExport);

    UpdateTotalResolution();
}

CTerrainTextureExport::~CTerrainTextureExport()
{
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureExport::ImportExport(bool bIsImport, bool bIsClipboard)
{
    if (!bIsClipboard)
    {
        //if(!strlen(gSettings.terrainTextureExport))
        if (!gSettings.BrowseTerrainTexture(!bIsImport))
        {
            return;
        }

        ui->FILE->setText(gSettings.terrainTextureExport);

        if (gSettings.terrainTextureExport.isEmpty())
        {
            QMessageBox::warning(this, tr("Terrain Texture"), tr("Error: Need to specify file name."));
            return;
        }
    }


    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();

    uint32 dwTileCountX = pRGBLayer->GetTileCountX();
    uint32 dwTileCountY = pRGBLayer->GetTileCountY();

    uint32 left;
    uint32 top;
    uint32 right;
    uint32 bottom;

    QRect sel = m_terrainTexture->GetSelection();
    PreviewToTile(right, top, sel.left(), sel.top());
    PreviewToTile(left, bottom, sel.right(), sel.bottom());

    AZStd::string filename;
    if (bIsClipboard == false)
    {
        filename = gSettings.terrainTextureExport.toUtf8().data();
    }

    if (bIsImport)
    {
        pRGBLayer->TryImportTileRect(filename, left, top, right + 1, bottom + 1);
    }
    else
    {
        pRGBLayer->ExportTileRect(filename, left, top, right + 1, bottom + 1);
    }

    if (bIsImport)
    {
        QRect rc(
            QPoint(left* pHeightmap->GetWidth() / dwTileCountX, top * pHeightmap->GetHeight() / dwTileCountY),
            QPoint((right + 1) * pHeightmap->GetWidth() / dwTileCountX, (bottom + 1) * pHeightmap->GetHeight() / dwTileCountY) - QPoint(1, 1)
        );
        pHeightmap->UpdateLayerTexture(rc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureExport::OnExport()
{
    if (m_terrainTexture->GetSelection().isNull())
    {
        return;
    }

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (!pHeightmap->IsAllocated())
    {
        return;
    }

    ImportExport(false /*bIsImport*/);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureExport::OnImport()
{
    if (m_terrainTexture->GetSelection().isNull())
    {
        return;
    }

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();
    if (!pHeightmap->IsAllocated())
    {
        return;
    }

    ImportExport(true /*bIsImport*/);

    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureExport::OnChangeResolutionBtn()
{
    if (m_terrainTexture->GetSelection().isNull())
    {
        QMessageBox::warning(this, tr("Terrain Texture"), tr("Error: You have to select a sector."));
        return;
    }

    uint32 dwTileX;
    uint32 dwTileY;
    QRect sel = m_terrainTexture->GetSelection();

    PreviewToTile(dwTileX, dwTileY, sel.left(), sel.top());

    CHeightmap* pHeightmap = GetIEditor()->GetHeightmap();

    CResizeResolutionDialog dlg;
    uint32 dwCurSize = pHeightmap->GetRGBLayer()->GetTileResolution(dwTileX, dwTileY);
    dlg.SetSize(dwCurSize);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    uint32 dwNewSize = dlg.GetSize();

    //TODO: make translatable
    CWaitProgress tileResolutionProgress("Changing tile resolution...");

    tileResolutionProgress.Start();

    // Updating the progress bar every sector can add a few seconds, update it once per tile
    int totalTiles = (sel.bottom() - sel.top() + 1) * (sel.right() - sel.left() + 1);
    int tilesProcessed = 0;
    for (uint32 y = sel.top(); y <= sel.bottom(); ++y)
    {
        for (uint32 x = sel.left(); x <= sel.right(); ++x)
        {
            PreviewToTile(dwTileX, dwTileY, x, y);

            uint32 dwOldSize = pHeightmap->GetRGBLayer()->GetTileResolution(dwTileX, dwTileY);
            if (dwOldSize == dwNewSize || dwNewSize < 64 || dwNewSize > 2048)
            {
                continue;
            }

            GetIEditor()->GetTerrainManager()->GetHeightmap()->GetRGBLayer()->ChangeTileResolution(dwTileX, dwTileY, dwNewSize);
            GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);
            GetIEditor()->SetModifiedModule(eModifiedTerrain);

            // update terrain preview image in the dialog
            m_terrainTexture->update();

            // update 3D Engine display
            I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();

            int nTerrainSize = p3DEngine->GetTerrainSize();
            int nTexSectorSize = p3DEngine->GetTerrainTextureNodeSizeMeters();
            uint32 dwCountX = pHeightmap->GetRGBLayer()->GetTileCountX();
            uint32 dwCountY = pHeightmap->GetRGBLayer()->GetTileCountY();

            if (!nTexSectorSize || !nTerrainSize || !dwCountX)
            {
                continue;
            }

            int numTexSectorsX = nTerrainSize / dwCountX / nTexSectorSize;
            int numTexSectorsY = nTerrainSize / dwCountY / nTexSectorSize;

            // Updating the progress bar every sector can add a few seconds, update it once per tile
            for (int iY = 0; iY < numTexSectorsY; ++iY)
            {
                for (int iX = 0; iX < numTexSectorsX; ++iX)
                {
                    pHeightmap->UpdateSectorTexture(QPoint(iX + dwTileX * numTexSectorsY, iY + dwTileY * numTexSectorsX), 0, 0, 1, 1);
                }
            }

            // update progress bar
            tileResolutionProgress.Step(++tilesProcessed * 100 / totalTiles);
            
        }
    }
    tileResolutionProgress.Stop();

    UpdateTotalResolution();

    GetIEditor()->GetActiveView()->Update();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureExport::UpdateTotalResolution()
{
    CRGBLayer* pRGBLayer = GetIEditor()->GetTerrainManager()->GetRGBLayer();
    uint32 minTexExtend = pRGBLayer->CalcMinRequiredTextureExtend();
    ui->INFO_TEXT->setText(QString("Total (%1x%1)").arg(minTexExtend));
}


#include <TerrainTextureExport.moc>
