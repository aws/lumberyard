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

// Description : Terrain Mini Map Tool implementation.


#include "StdAfx.h"

#include <I3DEngine.h>
#include <ITerrain.h>
#include <IEditorGame.h>
#include <IGameFramework.h>
#include <ILevelSystem.h>
#include "CryEditDoc.h"
#include "GameEngine.h"
#include "Util/ImageTIF.h"
#include "MainWindow.h"

#include "TerrainMiniMapTool.h"
#include <ui_TerrainMiniMapPanel.h>
#include <Viewport.h>
#include <QScopedPointer>
#include <QFileInfo>
#include <QMessageBox>

#define MAP_SCREENSHOT_SETTINGS "MapScreenshotSettings.xml"
#define MAX_RESOLUTION_SHIFT 11

//////////////////////////////////////////////////////////////////////////
// class CUndoTerrainMiniMapTool
// Undo object for storing Mini Map states
//////////////////////////////////////////////////////////////////////////
class CUndoTerrainMiniMapTool
    : public IUndoObject
{
public:
    CUndoTerrainMiniMapTool()
    {
        m_Undo = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "MiniMap Params"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_Redo = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
        }
        GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_Undo);
        if (bUndo)
        {
            GetIEditor()->Notify(eNotify_OnInvalidateControls);
        }
    }
    virtual void Redo()
    {
        GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_Redo);
        GetIEditor()->Notify(eNotify_OnInvalidateControls);
    }
private:
    SMinimapInfo m_Undo;
    SMinimapInfo m_Redo;
};


//////////////////////////////////////////////////////////////////////////
// Panel.
//////////////////////////////////////////////////////////////////////////

class CTerrainMiniMapPanel
    : public QWidget
{
public:
    CTerrainMiniMapTool* m_tool;

    QScopedPointer<Ui::CTerrainMiniMapPanel> ui;
    QPushButton* m_btnGenerate;
    QSpinBox* m_cameraHeight;
    QComboBox* m_resolutions;

    CTerrainMiniMapPanel(class CTerrainMiniMapTool* tool, QWidget* pParent = nullptr);

    void ReloadValues();
    void InitPanel();

protected:
    void OnGenerateArea();
    void OnHeightChange();
    void OnResolutionChange();
    void OnFileCommands();
    void OnFileSaveAs();
    void OnOrientationChanged();
};


CTerrainMiniMapPanel::CTerrainMiniMapPanel(class CTerrainMiniMapTool* tool, QWidget* pParent /*= nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CTerrainMiniMapPanel)
{
    m_tool = tool;
    ui->setupUi(this);

    m_btnGenerate = ui->GENERATEBTN;
    m_resolutions = ui->RESOLUTION;
    m_cameraHeight = ui->CAMHEIGHT;

    int nStart = 8;
    for (int i = 0; i < MAX_RESOLUTION_SHIFT; i++)
    {
        m_resolutions->addItem(QString::number(nStart << i));
    }
    ui->SCRIPT_NAME->setText(QtUtil::ToQString(MAP_SCREENSHOT_SETTINGS));
    m_tool->LoadSettingsXML();

    ReloadValues();
    InitPanel();

    connect(ui->FILE_COMMANDS, &QPushButton::clicked, this, &CTerrainMiniMapPanel::OnFileCommands);
    connect(ui->FILE_SAVE_AS, &QPushButton::clicked, this, &CTerrainMiniMapPanel::OnFileSaveAs);
    connect(ui->ALONG_XAXIS, &QCheckBox::toggled, this, &CTerrainMiniMapPanel::OnOrientationChanged);
    connect(m_btnGenerate, &QPushButton::clicked, this, &CTerrainMiniMapPanel::OnGenerateArea);
    connect(ui->RESOLUTION, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CTerrainMiniMapPanel::OnResolutionChange);
    connect(m_cameraHeight, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CTerrainMiniMapPanel::OnHeightChange);
}

void CTerrainMiniMapPanel::ReloadValues()
{
    SMinimapInfo minimap = m_tool->GetMinimap();
    m_cameraHeight->setValue(minimap.vExtends.x);
    int nRes = 0;
    int nStart = 8;
    for (int i = 0; i < MAX_RESOLUTION_SHIFT; i++)
    {
        if (minimap.textureWidth == (nStart << i))
        {
            m_resolutions->setCurrentIndex(i);
            break;
        }
    }
}

void CTerrainMiniMapPanel::InitPanel()
{
    bool isEnable = GetIEditor()->Get3DEngine()->GetITerrain() ? TRUE : FALSE;
    QList<QWidget*> nIDs = { ui->DDS_NAME, ui->TIF_NAME, ui->XML_NAME, ui->IS_DDS, ui->IS_TIF, ui->GENERATEBTN, ui->STATIC_OUTPUT, ui->STATIC_DDS, ui->STATIC_TIF, ui->FILE_SAVE_AS, ui->ALONG_XAXIS };
    for (int i = 0; i < nIDs.size(); i++)
    {
        nIDs[i]->setEnabled(isEnable);
    }

    if (!GetIEditor()->Get3DEngine()->GetITerrain())
    {
        return;
    }

    QString levelName = GetIEditor()->GetGameEngine()->GetLevelName();
    ui->DDS_NAME->setText(levelName);
    ui->TIF_NAME->setText(levelName);

    ui->XML_NAME->setText(m_tool->GetPath() + m_tool->GetFilename() + ".xml");

    ui->IS_DDS->setChecked(true);
    ui->IS_TIF->setChecked(true);
    ui->ALONG_XAXIS->setChecked(false);
    m_tool->SetOrientation(0);
}

void CTerrainMiniMapPanel::OnGenerateArea()
{
    m_tool->Generate();
}

void CTerrainMiniMapPanel::OnHeightChange()
{
    m_tool->SetCameraHeight(m_cameraHeight->value());
}

void CTerrainMiniMapPanel::OnResolutionChange()
{
    int nSel = m_resolutions->currentIndex();
    if (nSel >= 0)
    {
        int nRes = 0;
        int nStart = 8;
        for (int i = 0; i < 12; i++)
        {
            if (i == nSel)
            {
                m_tool->SetResolution(nStart << i);
                break;
            }
        }
    }
}

void CTerrainMiniMapPanel::OnFileCommands()
{
    CFileUtil::PopupQMenu(MAP_SCREENSHOT_SETTINGS, "Editor", this);
}

void CTerrainMiniMapPanel::OnFileSaveAs()
{
    QString path = m_tool->GetPath();
    QString filename = m_tool->GetFilename() + ".xml";

    if (CFileUtil::SelectSaveFile("XML Files (*.xml)", "xml", path, filename))
    {
        QFileInfo fi(filename);
        path = Path::GetPath(filename);

        m_tool->SetPath(path);
        m_tool->SetFilename(fi.baseName());
        ui->XML_NAME->setText(m_tool->GetPath() + m_tool->GetFilename() + ".xml");
    }
}

void CTerrainMiniMapPanel::OnOrientationChanged()
{
    m_tool->SetOrientation(ui->ALONG_XAXIS->isChecked() ? 1 : 0);
}

//////////////////////////////////////////////////////////////////////////


namespace
{
    int s_panelId = 0;
    CTerrainMiniMapPanel* s_panel = 0;
}





//////////////////////////////////////////////////////////////////////////
// CTerrainMiniMapTool
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTerrainMiniMapTool::CTerrainMiniMapTool()
{
    m_minimap = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
    m_bDragging = false;
    b_stateScreenShot = false;

    m_path = Path::GamePathToFullPath(GetIEditor()->GetGameEngine()->GetLevelPath());
    if (m_path.indexOf(":\\") == -1)
    {
        m_path = Path::GamePathToFullPath(m_path);
    }
    m_path = Path::AddPathSlash(m_path);
    m_filename = GetIEditor()->GetGameEngine()->GetLevelName();

    GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CTerrainMiniMapTool::~CTerrainMiniMapTool()
{
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }

    GetIEditor()->Get3DEngine()->SetScreenshotCallback(0);
    GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::BeginEditParams(IEditor* ie, int flags)
{
    if (!s_panelId)
    {
        s_panel = new CTerrainMiniMapPanel(this);
        s_panelId = GetIEditor()->AddRollUpPage(ROLLUP_TERRAIN, "Mini Map", s_panel);
        MainWindow::instance()->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::EndEditParams()
{
    if (s_panelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_TERRAIN, s_panelId);
        s_panelId = 0;
        s_panel = 0;
    }
}

void CTerrainMiniMapTool::SetResolution(int nResolution)
{
    GetIEditor()->BeginUndo();
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoTerrainMiniMapTool());
    }
    m_minimap.textureWidth = nResolution;
    m_minimap.textureHeight = nResolution;
    GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_minimap);
    GetIEditor()->AcceptUndo("Mini Map Resolution");
}

void CTerrainMiniMapTool::SetCameraHeight(float fHeight)
{
    GetIEditor()->BeginUndo();
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoTerrainMiniMapTool());
    }
    m_minimap.vExtends = Vec2(fHeight, fHeight);
    GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_minimap);
    GetIEditor()->AcceptUndo("Mini Map Camera Height");
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainMiniMapTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseLDown || (event == eMouseMove && (flags & MK_LBUTTON)))
    {
        Vec3 pos = view->ViewToWorld(point, 0, true);

        if (!m_bDragging)
        {
            GetIEditor()->BeginUndo();
            m_bDragging = true;
        }
        else
        {
            GetIEditor()->RestoreUndo();
        }

        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoTerrainMiniMapTool());
        }

        m_minimap.vCenter.x = pos.x;
        m_minimap.vCenter.y = pos.y;
        GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_minimap);

        return true;
    }
    else
    {
        // Stop.
        if (m_bDragging)
        {
            m_bDragging = false;
            GetIEditor()->AcceptUndo("Mini Map Position");
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::Display(DisplayContext& dc)
{
    dc.SetColor(0, 0, 1);
    dc.DrawTerrainRect(m_minimap.vCenter.x - 0.5f, m_minimap.vCenter.y - 0.5f,
        m_minimap.vCenter.x + 0.5f, m_minimap.vCenter.y + 0.5f,
        1.0f);
    dc.DrawTerrainRect(m_minimap.vCenter.x - 5.f, m_minimap.vCenter.y - 5.f,
        m_minimap.vCenter.x + 5.f, m_minimap.vCenter.y + 5.f,
        1.0f);
    dc.DrawTerrainRect(m_minimap.vCenter.x - 50.f, m_minimap.vCenter.y - 50.f,
        m_minimap.vCenter.x + 50.f, m_minimap.vCenter.y + 50.f,
        1.0f);

    dc.SetColor(0, 1, 0);
    dc.SetLineWidth(2);
    dc.DrawTerrainRect(m_minimap.vCenter.x - m_minimap.vExtends.x, m_minimap.vCenter.y - m_minimap.vExtends.y,
        m_minimap.vCenter.x + m_minimap.vExtends.x, m_minimap.vCenter.y + m_minimap.vExtends.y, 1.1f);
    dc.SetLineWidth(0);
}


void CTerrainMiniMapTool::SendParameters(void* data, uint32 width, uint32 height, f32 minx, f32 miny, f32 maxx, f32 maxy)
{
    QApplication::setOverrideCursor(Qt::BusyCursor);
    QString levelName = GetIEditor()->GetGameEngine()->GetLevelName();
    QString dataFile = m_path + m_filename + ".xml";
    QString imageTIFFile = m_path + levelName + ".tif";
    QString imageDDSFile = m_path + levelName + ".dds";
    QString imageDDSFileShort = levelName + ".dds";

    if (s_panel)
    {
        imageDDSFile = s_panel->ui->DDS_NAME->text();
        imageDDSFileShort = imageDDSFile + ".dds";
        imageDDSFile = m_path + imageDDSFile + ".dds";
        imageTIFFile = s_panel->ui->TIF_NAME->text();
        imageTIFFile = m_path + imageTIFFile + ".tif";
    }

    uint8* buf = (uint8*)data;

    CImageEx image;
    image.Allocate(width, height);

    if (s_panel && s_panel->ui->IS_DDS->isChecked())
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                image.ValueAt(x, height - 1 - y) = RGBA8(buf[x * 3 + y * width * 3], buf[x * 3 + 1 + y * width * 3], buf[x * 3 + 2 + y * width * 3], 255);
            }
        }

        bool success = GetIEditor()->GetRenderer()->WriteDDS((byte*)image.GetData(), width, height, 4, imageDDSFile.toUtf8().data(), eTF_BC2, 1);
        if (!success)
        {
            QMessageBox::warning(QApplication::activeWindow(), QObject::tr("Warning"), QObject::tr("Unable to save DDS to %1").arg(imageDDSFile));
        }
    }

    if (s_panel && s_panel->ui->IS_TIF->isChecked())
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                image.ValueAt(x, height - 1 - y) = RGBA8(buf[x * 3 + 2 + y * width * 3], buf[x * 3 + 1 + y * width * 3], buf[x * 3 + y * width * 3], 255);
            }
        }

        CImageTIF imageTIF;
        bool success = imageTIF.SaveRAW(imageTIFFile, image.GetData(), image.GetWidth(), image.GetHeight(), 1, 4, false, "Minimap");
        if (!success)
        {
            QMessageBox::warning(QApplication::activeWindow(), QObject::tr("Warning"), QObject::tr("Unable to save Tiff file to %1").arg(imageTIFFile));
        }
    }

    XmlNodeRef dataNode = GetISystem()->LoadXmlFromFile(dataFile.toUtf8().data());
    if (!dataNode)
    {
        dataNode = GetISystem()->CreateXmlNode("MetaData");
    }
    XmlNodeRef map = dataNode->findChild("MiniMap");
    if (!map)
    {
        map = GetISystem()->CreateXmlNode("MiniMap");
        dataNode->addChild(map);
    }
    map->setAttr("Filename", imageDDSFileShort.toUtf8().data());
    map->setAttr("startX", minx);
    map->setAttr("startY", miny);
    map->setAttr("endX", maxx);
    map->setAttr("endY", maxy);
    map->setAttr("width", width);
    map->setAttr("height", height);

    IEditorGame* pGame = GetIEditor()->GetGameEngine()->GetIEditorGame();
    if (pGame)
    {
        pGame->GetAdditionalMinimapData(dataNode);
    }

    bool success = dataNode->saveToFile(dataFile.toUtf8().data());
    if (!success)
    {
        QMessageBox::warning(QApplication::activeWindow(), QObject::tr("Warning"), QObject::tr("Unable to save minimap XML metadata file to %1").arg(dataFile));
    }

    ILevelSystem* pLevelSystem = GetISystem()->GetIGame()->GetIGameFramework()->GetILevelSystem();
    QString name = pLevelSystem->GetCurrentLevel() ? pLevelSystem->GetCurrentLevel()->GetLevelInfo()->GetName() : "";
    pLevelSystem->SetEditorLoadedLevel(name.toUtf8().data(), true);
    QApplication::restoreOverrideCursor();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::Generate(bool bHideProxy)
{
    m_ConstClearList.clear();

    if (bHideProxy)
    {
        gEnv->SetIsEditorGameMode(true); // To hide objects with collision_proxy_nomaterialset and editor materials
    }
    GetIEditor()->SetConsoleVar("e_ScreenShotWidth", m_minimap.textureWidth);
    GetIEditor()->SetConsoleVar("e_screenShotHeight", m_minimap.textureHeight);
    GetIEditor()->SetConsoleVar("e_ScreenShotMapOrientation", m_minimap.orientation);

    GetIEditor()->SetConsoleVar("e_ScreenShotMapCenterX", m_minimap.vCenter.x);
    GetIEditor()->SetConsoleVar("e_ScreenShotMapCenterY", m_minimap.vCenter.y);

    GetIEditor()->SetConsoleVar("e_ScreenShotMapSizeX", m_minimap.vExtends.x);
    GetIEditor()->SetConsoleVar("e_ScreenShotMapSizeY", m_minimap.vExtends.y);

    XmlNodeRef root = GetISystem()->LoadXmlFromFile((QString("Editor/") + MAP_SCREENSHOT_SETTINGS).toUtf8().data());
    if (root)
    {
        for (int i = 0, nChilds = root->getChildCount(); i < nChilds; ++i)
        {
            XmlNodeRef ChildNode = root->getChild(i);
            const char* pTagName = ChildNode->getTag();
            ICVar* pVar = gEnv->pConsole->GetCVar(pTagName);
            if (pVar)
            {
                m_ConstClearList[pTagName] = pVar->GetFVal();
                const char* pAttr = ChildNode->getAttr("value");
                string Value = pAttr;
                pVar->Set(Value.c_str());
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[MiniMap: %s] Console variable %s does not exist", MAP_SCREENSHOT_SETTINGS, pTagName);
            }
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[MiniMap: %s] Settings file does not exist. Using default values.", MAP_SCREENSHOT_SETTINGS);

        m_ConstClearList["r_PostProcessEffects"] = gEnv->pConsole->GetCVar("r_PostProcessEffects")->GetFVal();
        m_ConstClearList["e_Lods"] = gEnv->pConsole->GetCVar("e_Lods")->GetFVal();
        m_ConstClearList["e_ViewDistRatio"] = gEnv->pConsole->GetCVar("e_ViewDistRatio")->GetFVal();
        m_ConstClearList["e_ViewDistRatioVegetation"] = gEnv->pConsole->GetCVar("e_ViewDistRatioVegetation")->GetFVal();
        m_ConstClearList["e_TerrainLodRatio"] = gEnv->pConsole->GetCVar("e_TerrainLodRatio")->GetFVal();
        m_ConstClearList["e_ScreenShotQuality"] = gEnv->pConsole->GetCVar("e_ScreenShotQuality")->GetFVal();
        m_ConstClearList["e_Vegetation"] = gEnv->pConsole->GetCVar("e_Vegetation")->GetFVal();

        gEnv->pConsole->GetCVar("r_PostProcessEffects")->Set(1);
        gEnv->pConsole->GetCVar("e_ScreenShotQuality")->Set(0);
        gEnv->pConsole->GetCVar("e_ViewDistRatio")->Set(1000000.f);
        gEnv->pConsole->GetCVar("e_ViewDistRatioVegetation")->Set(100.f);
        gEnv->pConsole->GetCVar("e_Lods")->Set(0);
        gEnv->pConsole->GetCVar("e_TerrainLodRatio")->Set(0.f);
        gEnv->pConsole->GetCVar("e_Vegetation")->Set(0);
    }
    gEnv->pConsole->GetCVar("e_ScreenShotQuality")->Set(0);

    GetIEditor()->Get3DEngine()->SetScreenshotCallback(this);

    GetIEditor()->SetConsoleVar("e_ScreenShot", 3);

    b_stateScreenShot = true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnIdleUpdate:
    {
        ResetToDefault();
        break;
    }
    case eNotify_OnInvalidateControls:
        m_minimap = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
        if (s_panel)
        {
            s_panel->ReloadValues();
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::LoadSettingsXML()
{
    QString settingsXmlPath = m_path;
    settingsXmlPath += m_filename;
    settingsXmlPath += ".xml";
    XmlNodeRef dataNode = GetISystem()->LoadXmlFromFile(settingsXmlPath.toUtf8().data());
    if (!dataNode)
    {
        return;
    }
    XmlNodeRef mapNode = dataNode->findChild("MiniMap");
    if (!mapNode)
    {
        return;
    }
    float startX = 0, startY = 0, endX = 0, endY = 0;
    mapNode->getAttr("startX", startX);
    mapNode->getAttr("startY", startY);
    mapNode->getAttr("endX", endX);
    mapNode->getAttr("endY", endY);
    m_minimap.vCenter.x = 0.5f * (startX + endX);
    m_minimap.vCenter.y = 0.5f * (startY + endY);
    const float kMinExtend = 16.0f;
    m_minimap.vExtends.x = max(0.5f * (endX - startX), kMinExtend);
    m_minimap.vExtends.y = max(0.5f * (endY - startY), kMinExtend);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::ResetToDefault()
{
    if (b_stateScreenShot)
    {
        ICVar* pVar = gEnv->pConsole->GetCVar("e_ScreenShot");
        if (pVar && pVar->GetIVal() == 0)
        {
            for (std::map<string, float>::iterator it = m_ConstClearList.begin(); it != m_ConstClearList.end(); ++it)
            {
                ICVar* pVar = gEnv->pConsole->GetCVar(it->first.c_str());
                if (pVar)
                {
                    pVar->Set(it->second);
                }
            }
            m_ConstClearList.clear();

            b_stateScreenShot = false;
            GetIEditor()->Get3DEngine()->SetScreenshotCallback(0);
            gEnv->SetIsEditorGameMode(false);
        }
    }
}

#include <TerrainMiniMapTool.moc>