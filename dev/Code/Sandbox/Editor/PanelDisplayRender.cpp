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

// Description : implementation file


#include "StdAfx.h"
#include "PanelDisplayRender.h"
#include "DisplaySettings.h"
#include "VegetationMap.h"
#include <ui_PanelDisplayRender.h>

#define LEAVE_FLAGS (RENDER_FLAG_BBOX)

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayRender dialog


CPanelDisplayRender::CPanelDisplayRender(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CPanelDisplayRender)
    , m_initialized(false)
{
    ui->setupUi(this);

    m_roads = FALSE;
    m_decals = FALSE;
    m_detailTex = FALSE;
    m_fog = FALSE;
    m_shadowMaps = FALSE;
    m_staticShadowMaps = FALSE;
    m_skyBox = FALSE;
    m_staticObj = FALSE;
    m_terrain = FALSE;
    m_water = FALSE;
    m_detailObj = FALSE;
    m_particles = FALSE;
    m_clouds = FALSE;
    m_imposters = FALSE;
    m_geomCaches = FALSE;
    m_beams = FALSE;
    m_globalIllum = FALSE;
    m_alphablend = FALSE;

    m_dbg_frame_profile = FALSE;
    m_dbg_aidebugdraw = FALSE;
    m_dbg_aidebugdrawcover = FALSE;
    m_dbg_physicsDebugDraw = FALSE;
    m_dbg_mem_stats = FALSE;
    m_dbg_game_token_debugger = FALSE;
    m_dbg_renderer_profile_shaders = FALSE;
    m_dbg_renderer_overdraw = FALSE;
    m_dbg_renderer_resources = FALSE;
    m_dbg_budget_monitoring = FALSE;
    m_dbg_drawcalls = FALSE;

    m_boIsUpdatingValues = false;

    connect(ui->DISPLAY_FOG, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_ALL, &QCheckBox::clicked, this, &CPanelDisplayRender::OnDisplayAll);
    connect(ui->DISPLAY_NONE, &QCheckBox::clicked, this, &CPanelDisplayRender::OnDisplayNone);
    connect(ui->DISPLAY_INVERT, &QCheckBox::clicked, this, &CPanelDisplayRender::OnDisplayInvert);
    connect(ui->FILL_MODE, &QCheckBox::clicked, this, &CPanelDisplayRender::OnFillMode);
    connect(ui->WIREFRAME_MODE, &QCheckBox::clicked, this, &CPanelDisplayRender::OnWireframeMode);
    connect(ui->DISPLAY_SKYBOX, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_TERRAIN, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_WATER, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_SHADOWMAPS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_STATICSHADOWMAPS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_DETAILTEX, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_STATICOBJ, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_DECALS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_DETAILOBJ, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_PARTICLES, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_CLOUDS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_GEOMCACHES, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_IMPOSTERS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_BEAMS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_GLOBAL_ILLUMINATION, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);
    connect(ui->DISPLAY_HELPERS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnBnClickedToggleHelpers);
    connect(ui->DISPLAY_ROADS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeRenderFlag);

    // Debug flags.
    connect(ui->DISPLAY_PROFILE2, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_AIDEBUGDRAW, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_AIDEBUGDRAWCOVER, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->MEM_STATS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_PROFILESHADERS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_PHYSICSDEBUGDRAW, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->GAME_TOKEN_DEBUGGER, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_OVERDRAW, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_RENDERERRESOURCES, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_BUDGET_MONITOR, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->DISPLAY_DRAWCALLS, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->HIGHLIGHT_BREAKABLE, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);
    connect(ui->HIGHLIGHT_MISSING_SURFACE_TYPE, &QCheckBox::clicked, this, &CPanelDisplayRender::OnChangeDebugFlag);

    assert(GetCurrentDisplayRender() == NULL);
    GetCurrentDisplayRender() = this;

    SetupCallbacks();

    GetIEditor()->RegisterNotifyListener(this);
}

CPanelDisplayRender::~CPanelDisplayRender()
{
    GetIEditor()->UnregisterNotifyListener(this);
    GetCurrentDisplayRender() = NULL;
}


void CPanelDisplayRender::UpdateData(bool fromUi)
{
    if (fromUi)
    {
        // Render flags.
        m_roads = ui->DISPLAY_ROADS->isChecked();
        m_decals = ui->DISPLAY_DECALS->isChecked();
        m_detailTex = ui->DISPLAY_DETAILTEX->isChecked();
        m_fog = ui->DISPLAY_FOG->isChecked();
        m_shadowMaps = ui->DISPLAY_SHADOWMAPS->isChecked();
        m_staticShadowMaps = ui->DISPLAY_STATICSHADOWMAPS->isChecked();
        m_skyBox = ui->DISPLAY_SKYBOX->isChecked();
        m_staticObj = ui->DISPLAY_STATICOBJ->isChecked();
        m_terrain = ui->DISPLAY_TERRAIN->isChecked();
        m_water = ui->DISPLAY_WATER->isChecked();
        m_detailObj = ui->DISPLAY_DETAILOBJ->isChecked();
        m_particles = ui->DISPLAY_PARTICLES->isChecked();
        m_clouds = ui->DISPLAY_CLOUDS->isChecked();
        m_geomCaches = ui->DISPLAY_GEOMCACHES->isChecked();
        m_imposters = ui->DISPLAY_IMPOSTERS->isChecked();
        m_beams = ui->DISPLAY_BEAMS->isChecked();
        m_globalIllum = ui->DISPLAY_GLOBAL_ILLUMINATION->isChecked();

        // Debug flags.
        m_dbg_frame_profile = ui->DISPLAY_PROFILE2->isChecked();
        m_dbg_aidebugdraw = ui->DISPLAY_AIDEBUGDRAW->isChecked();
        m_dbg_aidebugdrawcover = ui->DISPLAY_AIDEBUGDRAWCOVER->isChecked();
        m_dbg_mem_stats = ui->MEM_STATS->isChecked();
        m_dbg_game_token_debugger = ui->GAME_TOKEN_DEBUGGER->isChecked();
        m_dbg_physicsDebugDraw = ui->DISPLAY_PHYSICSDEBUGDRAW->isChecked();
        m_dbg_renderer_profile_shaders = ui->DISPLAY_PROFILESHADERS->isChecked();
        m_dbg_renderer_overdraw = ui->DISPLAY_OVERDRAW->isChecked();
        m_dbg_renderer_resources = ui->DISPLAY_RENDERERRESOURCES->isChecked();
        m_dbg_budget_monitoring = ui->DISPLAY_BUDGET_MONITOR->isChecked();
        m_dbg_drawcalls = ui->DISPLAY_DRAWCALLS->isChecked();
        m_dbg_highlight_breakable = ui->HIGHLIGHT_BREAKABLE->isChecked();
        m_dbg_highlight_missing_surface_type = ui->HIGHLIGHT_MISSING_SURFACE_TYPE->isChecked();
    }
    else
    {
        // Render flags.
        ui->DISPLAY_ROADS->setChecked(m_roads);
        ui->DISPLAY_DECALS->setChecked(m_decals);
        ui->DISPLAY_DETAILTEX->setChecked(m_detailTex);
        ui->DISPLAY_FOG->setChecked(m_fog);
        ui->DISPLAY_SHADOWMAPS->setChecked(m_shadowMaps);
        ui->DISPLAY_STATICSHADOWMAPS->setChecked(m_staticShadowMaps);
        ui->DISPLAY_SKYBOX->setChecked(m_skyBox);
        ui->DISPLAY_STATICOBJ->setChecked(m_staticObj);
        ui->DISPLAY_TERRAIN->setChecked(m_terrain);
        ui->DISPLAY_WATER->setChecked(m_water);
        ui->DISPLAY_DETAILOBJ->setChecked(m_detailObj);
        ui->DISPLAY_PARTICLES->setChecked(m_particles);
        ui->DISPLAY_CLOUDS->setChecked(m_clouds);
        ui->DISPLAY_GEOMCACHES->setChecked(m_geomCaches);
        ui->DISPLAY_IMPOSTERS->setChecked(m_imposters);
        ui->DISPLAY_BEAMS->setChecked(m_beams);
        ui->DISPLAY_GLOBAL_ILLUMINATION->setChecked(m_globalIllum);

        // Debug flags.
        ui->DISPLAY_PROFILE2->setChecked(m_dbg_frame_profile);
        ui->DISPLAY_AIDEBUGDRAW->setChecked(m_dbg_aidebugdraw);
        ui->DISPLAY_AIDEBUGDRAWCOVER->setChecked(m_dbg_aidebugdrawcover);
        ui->MEM_STATS->setChecked(m_dbg_mem_stats);
        ui->GAME_TOKEN_DEBUGGER->setChecked(m_dbg_game_token_debugger);
        ui->DISPLAY_PHYSICSDEBUGDRAW->setChecked(m_dbg_physicsDebugDraw);
        ui->DISPLAY_PROFILESHADERS->setChecked(m_dbg_renderer_profile_shaders);
        ui->DISPLAY_OVERDRAW->setChecked(m_dbg_renderer_overdraw);
        ui->DISPLAY_RENDERERRESOURCES->setChecked(m_dbg_renderer_resources);
        ui->DISPLAY_BUDGET_MONITOR->setChecked(m_dbg_budget_monitoring);
        ui->DISPLAY_DRAWCALLS->setChecked(m_dbg_drawcalls);
        ui->HIGHLIGHT_BREAKABLE->setChecked(m_dbg_highlight_breakable);
        ui->HIGHLIGHT_MISSING_SURFACE_TYPE->setChecked(m_dbg_highlight_missing_surface_type);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayRender message handlers

void CPanelDisplayRender::showEvent(QShowEvent* event)
{
    if (!m_initialized)
    {
        m_initialized = true;
        SetControls();
    }
}

void CPanelDisplayRender::SetupCallbacks()
{
    RegisterChangeCallback("e_Roads", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_Decals", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_DetailTextures", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_TerrainDetailMaterials", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_Fog", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_Terrain", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_SkyBox", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_Shadows", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_ShadowsCache", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_Vegetation", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_WaterOcean", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_WaterVolumes", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_ProcVegetation", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_Particles", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_Clouds", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_GeomCaches", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_VegetationSprites", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("r_Beams", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("e_GI", &CPanelDisplayRender::OnDisplayOptionChanged);
    RegisterChangeCallback("r_wireframe", &CPanelDisplayRender::OnDisplayOptionChanged);


    // Debug variables.
    RegisterChangeCallback("ai_DebugDraw", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("r_TexLog", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("MemStats", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("p_draw_helpers", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("r_ProfileShaders", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("r_MeasureOverdraw", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("r_Stats", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("sys_enable_budgetmonitoring", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("gt_show", &CPanelDisplayRender::OnDebugOptionChanged);
    RegisterChangeCallback("Profile", &CPanelDisplayRender::OnDebugOptionChanged);
}

void CPanelDisplayRender::SetControls()
{
    ICVar* piVariable(NULL);

    piVariable = gEnv->pConsole->GetCVar("e_Roads");
    if (piVariable)
    {
        m_roads = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Decals");
    if (piVariable)
    {
        m_decals = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_DetailTextures");
    if (piVariable)
    {
        m_detailTex = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_TerrainDetailMaterials");
    if (piVariable)
    {
        m_detailTex |= (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Fog");
    if (piVariable)
    {
        m_fog = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Terrain");
    if (piVariable)
    {
        m_terrain = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_SkyBox");
    if (piVariable)
    {
        m_skyBox = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Shadows");
    if (piVariable)
    {
        m_shadowMaps = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Vegetation");
    if (piVariable)
    {
        m_staticObj = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_WaterOcean");
    if (piVariable)
    {
        m_water = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_WaterVolumes");
    if (piVariable)
    {
        m_water = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_ProcVegetation");
    if (piVariable)
    {
        m_detailObj = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Particles");
    if (piVariable)
    {
        m_particles = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Clouds");
    if (piVariable)
    {
        m_clouds = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_GeomCaches");
    if (piVariable)
    {
        m_geomCaches = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_VegetationSprites");
    if (piVariable)
    {
        m_imposters = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_Beams");
    if (piVariable)
    {
        m_beams = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_GI");
    if (piVariable)
    {
        m_globalIllum = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    // Debug
    piVariable = gEnv->pConsole->GetCVar("ai_DebugDraw");
    if (piVariable)
    {
        m_dbg_aidebugdraw = (piVariable->GetIVal() > 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("ai_DebugDrawCover");
    if (piVariable)
    {
        m_dbg_aidebugdrawcover = (piVariable->GetIVal() > 1) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("MemStats");
    if (piVariable)
    {
        m_dbg_mem_stats = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("p_draw_helpers");
    if (piVariable)
    {
        m_dbg_physicsDebugDraw = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_ProfileShaders");
    if (piVariable)
    {
        m_dbg_renderer_profile_shaders = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_MeasureOverdraw");
    if (piVariable)
    {
        m_dbg_renderer_overdraw = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("gt_show");
    if (piVariable)
    {
        m_dbg_game_token_debugger = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_Stats");
    if (piVariable)
    {
        m_dbg_renderer_resources = (piVariable->GetIVal() == 1) ? TRUE : FALSE;
        m_dbg_drawcalls = (piVariable->GetIVal() == 6) ? TRUE : FALSE;
        m_dbg_budget_monitoring = (piVariable->GetIVal() == 15) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("Profile");
    if (piVariable)
    {
        m_dbg_frame_profile = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
    m_dbg_highlight_breakable = (flags & DBG_HIGHLIGHT_BREAKABLE) ? TRUE : FALSE;
    m_dbg_highlight_missing_surface_type = (flags & DBG_HIGHLIGHT_MISSING_SURFACE_TYPE) ? TRUE : FALSE;

    UpdateData(FALSE);

    ui->DISPLAY_HELPERS->setChecked(GetIEditor()->GetDisplaySettings()->IsDisplayHelpers());

    OnDisplayOptionChanged();
    OnDebugOptionChanged();
}

void CPanelDisplayRender::OnChangeRenderFlag()
{
    UpdateData(TRUE);

    UpdateDisplayOptions();
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnChangeDebugFlag()
{
    UpdateData(TRUE);

    uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
    flags = 0;
    flags |= (m_dbg_highlight_breakable) ? DBG_HIGHLIGHT_BREAKABLE : 0;
    flags |= (m_dbg_highlight_missing_surface_type) ? DBG_HIGHLIGHT_MISSING_SURFACE_TYPE : 0;
    GetIEditor()->GetDisplaySettings()->SetDebugFlags(flags);

    UpdateDebugOptions();
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDisplayAll()
{
    uint32 flags = GetIEditor()->GetDisplaySettings()->GetRenderFlags();
    flags |= 0xFFFFFFFF & ~(LEAVE_FLAGS);
    GetIEditor()->GetDisplaySettings()->SetRenderFlags(flags);

    m_roads = TRUE;
    m_decals = TRUE;
    m_detailTex = TRUE;
    m_fog = TRUE;
    m_shadowMaps = TRUE;
    m_skyBox = TRUE;
    m_staticObj = TRUE;
    m_terrain = TRUE;
    m_water = TRUE;
    m_detailObj = TRUE;
    m_particles = TRUE;
    m_clouds = TRUE;
    m_imposters = TRUE;
    m_geomCaches = TRUE;
    m_beams = TRUE;
    m_globalIllum = TRUE;
    m_alphablend = TRUE;

    UpdateDisplayOptions();
    SetControls();
}

void CPanelDisplayRender::OnDisplayNone()
{
    uint32 flags = GetIEditor()->GetDisplaySettings()->GetRenderFlags();
    flags &= (LEAVE_FLAGS);
    GetIEditor()->GetDisplaySettings()->SetRenderFlags(flags);

    m_roads = FALSE;
    m_decals = FALSE;
    m_detailTex = FALSE;
    m_fog = FALSE;
    m_shadowMaps = FALSE;
    m_skyBox = FALSE;
    m_staticObj = FALSE;
    m_terrain = FALSE;
    m_water = FALSE;
    m_detailObj = FALSE;
    m_particles = FALSE;
    m_clouds = FALSE;
    m_imposters = FALSE;
    m_geomCaches = FALSE;
    m_beams = FALSE;
    m_globalIllum = FALSE;
    m_alphablend = FALSE;

    UpdateDisplayOptions();
    SetControls();
}

void CPanelDisplayRender::OnDisplayInvert()
{
    uint32 flags0 = GetIEditor()->GetDisplaySettings()->GetRenderFlags();
    uint32 flags = ~flags0;
    flags &= ~(LEAVE_FLAGS);
    flags |= flags0 & (LEAVE_FLAGS);
    GetIEditor()->GetDisplaySettings()->SetRenderFlags(flags);

    m_roads =                   m_roads             ? FALSE : TRUE;
    m_decals =              m_decals            ? FALSE : TRUE;
    m_detailTex =           m_detailTex     ? FALSE : TRUE;
    m_fog =                     m_fog                   ? FALSE : TRUE;
    m_shadowMaps =      m_shadowMaps    ? FALSE : TRUE;
    m_skyBox =              m_skyBox            ? FALSE : TRUE;
    m_staticObj =           m_staticObj     ? FALSE : TRUE;
    m_terrain =             m_terrain           ? FALSE : TRUE;
    m_water =                   m_water             ? FALSE : TRUE;
    m_detailObj =           m_detailObj     ? FALSE : TRUE;
    m_particles =           m_particles     ? FALSE : TRUE;
    m_clouds =              m_clouds            ? FALSE : TRUE;
    m_imposters =           m_imposters     ? FALSE : TRUE;
    m_geomCaches =        m_geomCaches    ? FALSE : TRUE;
    m_beams =                   m_beams             ? FALSE : TRUE;
    m_globalIllum =     m_globalIllum   ? FALSE : TRUE;
    m_alphablend =      m_alphablend    ? FALSE : TRUE;

    UpdateDisplayOptions();
    SetControls();
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnFillMode()
{
    if (ICVar* r_wireframe = gEnv->pConsole->GetCVar("r_wireframe"))
    {
        r_wireframe->Set(R_SOLID_MODE);
    }
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnWireframeMode()
{
    if (ICVar* r_wireframe = gEnv->pConsole->GetCVar("r_wireframe"))
    {
        r_wireframe->Set(R_WIREFRAME_MODE);
    }
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnBnClickedToggleHelpers()
{
    if (ui->DISPLAY_HELPERS->isChecked())
    {
        GetIEditor()->GetDisplaySettings()->DisplayHelpers(true);
    }
    else
    {
        GetIEditor()->GetDisplaySettings()->DisplayHelpers(false);
        GetIEditor()->GetObjectManager()->SendEvent(EVENT_HIDE_HELPER);
    }

    GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnDisplayRenderUpdate:
        SetControls();
        break;
    }
}
//////////////////////////////////////////////////////////////////////////
CPanelDisplayRender*& CPanelDisplayRender::GetCurrentDisplayRender()
{
    static CPanelDisplayRender* poCurrentDisplayRender(NULL);
    return poCurrentDisplayRender;
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDisplayOptionChanged(ICVar* piDisplayModeVariable)
{
    CPanelDisplayRender* poDisplayMode = CPanelDisplayRender::GetCurrentDisplayRender();
    if (!poDisplayMode)
    {
        return;
    }
    poDisplayMode->OnDisplayOptionChanged();

    TDVariableNameToConsoleFunction::iterator       itIterator;
    itIterator = poDisplayMode->m_cVariableNameToConsoleFunction.find(piDisplayModeVariable->GetName());
    if (itIterator != poDisplayMode->m_cVariableNameToConsoleFunction.end())
    {
        if (itIterator->second)
        {
            itIterator->second(piDisplayModeVariable);
        }
    }
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDebugOptionChanged(ICVar* piDisplayModeVariable)
{
    CPanelDisplayRender* poDisplayMode = CPanelDisplayRender::GetCurrentDisplayRender();
    if (!poDisplayMode)
    {
        return;
    }
    poDisplayMode->OnDebugOptionChanged();

    TDVariableNameToConsoleFunction::iterator       itIterator;
    itIterator = poDisplayMode->m_cVariableNameToConsoleFunction.find(piDisplayModeVariable->GetName());
    if (itIterator != poDisplayMode->m_cVariableNameToConsoleFunction.end())
    {
        if (itIterator->second)
        {
            itIterator->second(piDisplayModeVariable);
        }
    }
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDisplayOptionChanged()
{
    ICVar* piVariable(NULL);
    int         nDisplayMode(0);

    if (m_boIsUpdatingValues)
    {
        return;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Roads");
    if (piVariable)
    {
        m_roads = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Decals");
    if (piVariable)
    {
        m_decals = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_DetailTextures");
    if (piVariable)
    {
        m_detailTex = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_TerrainDetailMaterials");
    if (piVariable)
    {
        m_detailTex = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Fog");
    if (piVariable)
    {
        m_fog = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Terrain");
    if (piVariable)
    {
        m_terrain = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_SkyBox");
    if (piVariable)
    {
        m_skyBox = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Shadows");
    if (piVariable)
    {
        m_shadowMaps = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_ShadowsCache");
    if (piVariable)
    {
        m_staticShadowMaps = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_WaterOcean");
    if (piVariable)
    {
        m_water = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_WaterVolumes");
    if (piVariable)
    {
        m_water = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_ProcVegetation");
    if (piVariable)
    {
        m_detailObj = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Particles");
    if (piVariable)
    {
        m_particles = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Clouds");
    if (piVariable)
    {
        m_clouds = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_GeomCaches");
    if (piVariable)
    {
        m_geomCaches = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_VegetationSprites");
    if (piVariable)
    {
        m_imposters = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_Vegetation");
    if (piVariable)
    {
        m_staticObj = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_Beams");
    if (piVariable)
    {
        m_beams = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("e_GI");
    if (piVariable)
    {
        m_globalIllum = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    UpdateData(FALSE);

    piVariable = gEnv->pConsole->GetCVar("r_wireframe");
    if (piVariable)
    {
        nDisplayMode = piVariable->GetIVal();
    }

    ui->FILL_MODE->setChecked(nDisplayMode == R_SOLID_MODE);
    ui->WIREFRAME_MODE->setChecked(nDisplayMode == R_WIREFRAME_MODE);
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::UpdateDisplayOptions()
{
    ICVar* piVariable(NULL);

    m_boIsUpdatingValues = true;

    piVariable = gEnv->pConsole->GetCVar("e_Roads");
    if (piVariable)
    {
        piVariable->Set(m_roads);
    }

    piVariable = gEnv->pConsole->GetCVar("e_Decals");
    if (piVariable)
    {
        piVariable->Set(m_decals);
    }

    piVariable = gEnv->pConsole->GetCVar("r_DetailTextures");
    if (piVariable)
    {
        piVariable->Set(m_detailTex);
    }

    piVariable = gEnv->pConsole->GetCVar("e_TerrainDetailMaterials");
    if (piVariable)
    {
        piVariable->Set(m_detailTex);
    }

    piVariable = gEnv->pConsole->GetCVar("e_Fog");
    if (piVariable)
    {
        piVariable->Set(m_fog);
    }

    piVariable = gEnv->pConsole->GetCVar("e_Terrain");
    if (piVariable)
    {
        piVariable->Set(m_terrain);
    }

    piVariable = gEnv->pConsole->GetCVar("e_SkyBox");
    if (piVariable)
    {
        piVariable->Set(m_skyBox);
    }

    piVariable = gEnv->pConsole->GetCVar("e_Shadows");
    if (piVariable)
    {
        piVariable->Set(m_shadowMaps);
    }

    piVariable = gEnv->pConsole->GetCVar("e_ShadowsCache");
    if (piVariable)
    {
        piVariable->Set(m_staticShadowMaps);
    }

    piVariable = gEnv->pConsole->GetCVar("e_Vegetation");
    if (piVariable)
    {
        piVariable->Set(m_staticObj);
    }

    piVariable = gEnv->pConsole->GetCVar("e_WaterOcean");
    if (piVariable)
    {
        piVariable->Set(m_water);
    }

    piVariable = gEnv->pConsole->GetCVar("e_WaterVolumes");
    if (piVariable)
    {
        piVariable->Set(m_water);
    }

    piVariable = gEnv->pConsole->GetCVar("e_ProcVegetation");
    if (piVariable)
    {
        piVariable->Set(m_detailObj);
    }

    piVariable = gEnv->pConsole->GetCVar("e_Particles");
    if (piVariable)
    {
        piVariable->Set(m_particles);
    }

    piVariable = gEnv->pConsole->GetCVar("e_Clouds");
    if (piVariable)
    {
        piVariable->Set(m_clouds);
    }

    piVariable = gEnv->pConsole->GetCVar("e_GeomCaches");
    if (piVariable)
    {
        piVariable->Set(m_geomCaches);
    }

    piVariable = gEnv->pConsole->GetCVar("e_VegetationSprites");
    if (piVariable)
    {
        piVariable->Set(m_imposters);
    }

    piVariable = gEnv->pConsole->GetCVar("r_Beams");
    if (piVariable)
    {
        piVariable->Set(m_beams);
    }

    piVariable = gEnv->pConsole->GetCVar("e_GI");
    if (piVariable)
    {
        piVariable->Set(m_globalIllum);
    }

    piVariable = gEnv->pConsole->GetCVar("r_wireframe");
    if (piVariable)
    {
        if (ui->FILL_MODE->isChecked())
        {
            piVariable->Set(0);
        }
        else if (ui->WIREFRAME_MODE->isChecked())
        {
            piVariable->Set(1);
        }
    }
    m_boIsUpdatingValues = false;
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDebugOptionChanged()
{
    ICVar* piVariable(NULL);

    uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
    m_dbg_highlight_breakable = (flags & DBG_HIGHLIGHT_BREAKABLE) ? TRUE : FALSE;
    m_dbg_highlight_missing_surface_type = (flags & DBG_HIGHLIGHT_MISSING_SURFACE_TYPE) ? TRUE : FALSE;

    piVariable = gEnv->pConsole->GetCVar("ai_DebugDraw");
    if (piVariable)
    {
        m_dbg_aidebugdraw = (piVariable->GetIVal() > 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("p_draw_helpers");
    if (piVariable)
    {
        m_dbg_physicsDebugDraw = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_ProfileShaders");
    if (piVariable)
    {
        m_dbg_renderer_profile_shaders = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_MeasureOverdraw");
    if (piVariable)
    {
        m_dbg_renderer_overdraw = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("gt_show");
    if (piVariable)
    {
        m_dbg_game_token_debugger = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("r_Stats");
    if (piVariable)
    {
        m_dbg_renderer_resources = (piVariable->GetIVal() == 1) ? TRUE : FALSE;
        m_dbg_drawcalls = (piVariable->GetIVal() == 6) ? TRUE : FALSE;
        m_dbg_budget_monitoring = (piVariable->GetIVal() == 15) ? TRUE : FALSE;
    }

    piVariable = gEnv->pConsole->GetCVar("Profile");
    if (piVariable)
    {
        m_dbg_frame_profile = (piVariable->GetIVal() != 0) ? TRUE : FALSE;
    }

    UpdateData(FALSE);
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::UpdateDebugOptions()
{
    ICVar* piVariable(NULL);

    // It is commented here because it's not really necessary as this is not
    // a console variable.
    //uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
    //GetIEditor()->GetDisplaySettings()->SetDebugFlags(flags);

    piVariable = gEnv->pConsole->GetCVar("ai_DebugDraw");
    if (piVariable)
    {
        piVariable->Set(m_dbg_aidebugdraw ? 1 : -1);
    }

    piVariable = gEnv->pConsole->GetCVar("ai_DebugDrawCover");
    if (piVariable)
    {
        piVariable->Set(m_dbg_aidebugdrawcover ? 2 : 0);
    }

    piVariable = gEnv->pConsole->GetCVar("MemStats");
    if (piVariable)
    {
        piVariable->Set(m_dbg_mem_stats ? 1000 : 0);
    }

    piVariable = gEnv->pConsole->GetCVar("p_draw_helpers");
    if (piVariable)
    {
        piVariable->Set(m_dbg_physicsDebugDraw ? 1 : 0);
    }

    piVariable = gEnv->pConsole->GetCVar("r_ProfileShaders");
    if (piVariable)
    {
        piVariable->Set(m_dbg_renderer_profile_shaders);
    }

    piVariable = gEnv->pConsole->GetCVar("r_MeasureOverdraw");
    if (piVariable)
    {
        piVariable->Set(m_dbg_renderer_overdraw ? 1 : 0);
    }

    piVariable = gEnv->pConsole->GetCVar("gt_show");
    if (piVariable)
    {
        piVariable->Set(m_dbg_game_token_debugger ? 1 : 0);
    }

    piVariable = gEnv->pConsole->GetCVar("r_Stats");
    if (piVariable)
    {
        if (m_dbg_renderer_resources)
        {
            piVariable->Set(1);
        }
        else
        if (m_dbg_drawcalls)
        {
            piVariable->Set(6);
        }
        else
        if (m_dbg_budget_monitoring)
        {
            piVariable->Set(15);
        }
        else
        {
            piVariable->Set(0);
        }
    }

    piVariable = gEnv->pConsole->GetCVar("Profile");
    if (piVariable)
    {
        piVariable->Set(m_dbg_frame_profile ? 1 : 0);
    }
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::RegisterChangeCallback(const char* szVariableName, ConsoleVarFunc fnCallbackFunction)
{
    ICVar* piVariable(NULL);

    piVariable = gEnv->pConsole->GetCVar(szVariableName);
    if (!piVariable)
    {
        return;
    }

    m_cVariableNameToConsoleFunction[(string)szVariableName] = piVariable->GetOnChangeCallback();
    piVariable->SetOnChangeCallback(fnCallbackFunction);
}

#include <PanelDisplayRender.moc>
//////////////////////////////////////////////////////////////////////////
