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
#include "GameEngine.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>
#include "AICoverSurface.h"
#include "AI/AIManager.h"
#include "AI/CoverSurfaceManager.h"
#include "AI/NavDataGeneration/Navigation.h"

#include "Viewport.h"
#include "Include/IIconManager.h"
//#include "SegmentedWorld/SegmentedWorldManager.h"


namespace
{
    int s_propertiesID = 0;
    ReflectedPropertiesPanel* s_properties = 0;

    struct PropertiesDeleter
    {
        ~PropertiesDeleter()
        {
            delete s_properties;
        }
    } __deleter;
};


CAICoverSurface::CAICoverSurface()
    : m_sampler(0)
    , m_surfaceID(0)
    , m_helperScale(1.0f)
    , m_aabb(AABB::RESET)
    , m_aabbLocal(AABB::RESET)
    , m_samplerResult(None)
{
    m_aabbLocal.Reset();
    SetColor(QColor(70, 130, 180));
}

CAICoverSurface::~CAICoverSurface()
{
}

bool CAICoverSurface::Init(IEditor* editor, CBaseObject* prev, const QString& file)
{
    if (CBaseObject::Init(editor, prev, file))
    {
        CreatePropertyVars();

        if (prev)
        {
            CAICoverSurface* original = static_cast<CAICoverSurface*>(prev);
            m_propertyValues = original->m_propertyValues;
        }
        else
        {
            SetPropertyVarsFromParams(ICoverSampler::Params()); // defaults
        }
        return true;
    }

    return false;
}

bool CAICoverSurface::CreateGameObject()
{
    if (CBaseObject::CreateGameObject())
    {
        GetIEditor()->GetAI()->GetCoverSurfaceManager()->AddSurfaceObject(this);

        return true;
    }

    return false;
}

void CAICoverSurface::Done()
{
    if (m_sampler)
    {
        m_sampler->Release();
        m_sampler = 0;
    }

    ClearSurface();

    IEditor*    pEditor(GetIEditor());
    CAIManager* pAIManager(pEditor->GetAI());
    CCoverSurfaceManager* pCoverSurfaceManager = pAIManager->GetCoverSurfaceManager();

    pCoverSurfaceManager->RemoveSurfaceObject(this);

    CBaseObject::Done();
}

void CAICoverSurface::DeleteThis()
{
    delete this;
}

void CAICoverSurface::Display(DisplayContext& disp)
{
    if (IsFrozen())
    {
        disp.SetFreezeColor();
    }
    else
    {
        disp.SetColor(GetColor());
    }

    Matrix34 scale(Matrix34::CreateScale(Vec3(gSettings.gizmo.helpersScale * GetHelperScale())));

    disp.RenderObject(eStatObject_Anchor, GetWorldTM() * scale);

    if (m_sampler)
    {
        switch (m_sampler->Update(0.025f, 2.0f))
        {
        case ICoverSampler::Finished:
        {
            CommitSurface();
            ReleaseSampler();
            m_samplerResult = Success;
        }
        break;
        case ICoverSampler::Error:
        {
            ClearSurface();
            ReleaseSampler();
            m_samplerResult = Error;
        }
        break;
        default:
            break;
        }
    }

    if (m_surfaceID && IsSelected() && (GetIEditor()->GetSelection()->GetCount() < 15))
    {
        gEnv->pAISystem->GetCoverSystem()->DrawSurface(m_surfaceID);
    }

    if (m_samplerResult == Error)
    {
        DisplayBadCoverSurfaceObject();
    }

    DrawDefault(disp);
}

int CAICoverSurface::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if ((event == eMouseMove) || (event == eMouseLDown))
    {
        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            SetPos(view->MapViewToCP(point));
        }
        else
        {
            bool terrain;
            Vec3 pos = view->ViewToWorld(point, &terrain);
            if (terrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y) + 0.25f;
            }

            SetPos(view->SnapToGrid(pos));
        }

        if (event == eMouseLDown)
        {
            //          SW_TEST_OBJ_PLACETO_MCB(GetPos(), GetLayer(), true);
            //          SW_ON_OBJ_NEW(this);
            return MOUSECREATE_OK;
        }

        return MOUSECREATE_CONTINUE;
    }

    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

bool CAICoverSurface::HitTest(HitContext& hitContext)
{
    Vec3 origin = GetWorldPos();
    float radius = GetHelperSize() * GetHelperScale();

    Vec3 w = origin - hitContext.raySrc;
    w = hitContext.rayDir.Cross(w);

    float d = w.GetLengthSquared();
    if (d < (radius * radius) + hitContext.distanceTolerance)
    {
        Vec3 i0;
        if (Intersect::Ray_SphereFirst(Ray(hitContext.raySrc, hitContext.rayDir), Sphere(origin, radius), i0))
        {
            hitContext.dist = hitContext.raySrc.GetDistance(i0);

            return true;
        }

        hitContext.dist = hitContext.raySrc.GetDistance(origin);

        return true;
    }

    return false;
}

void CAICoverSurface::GetBoundBox(AABB& aabb)
{
    aabb = m_aabb;

    float extent = GetHelperSize() * GetHelperScale();
    Vec3 world = GetWorldPos();

    aabb.Add(world - Vec3(extent));
    aabb.Add(world + Vec3(extent));
}

void CAICoverSurface::GetLocalBounds(AABB& aabb)
{
    aabb = m_aabbLocal;

    float extent = GetHelperSize() * GetHelperScale();
    aabb.Add(Vec3(-extent));
    aabb.Add(Vec3(extent));
}

void CAICoverSurface::SetHelperScale(float scale)
{
    if (m_helperScale != scale)
    {
        //      SW_ON_OBJ_MOD(this);
    }
    m_helperScale = scale;
}

float CAICoverSurface::GetHelperScale()
{
    return m_helperScale;
}

float CAICoverSurface::GetHelperSize() const
{
    return 0.5f * gSettings.gizmo.helpersScale;
}

const CoverSurfaceID& CAICoverSurface::GetSurfaceID() const
{
    return m_surfaceID;
}

void CAICoverSurface::SetSurfaceID(const CoverSurfaceID& coverSurfaceID)
{
    m_surfaceID = coverSurfaceID;
}

void CAICoverSurface::Generate()
{
    CreateSampler();
    StartSampling();

    while (m_sampler->Update(2.0f) == ICoverSampler::InProgress)
    {
        ;
    }

    if ((m_sampler->GetState() != ICoverSampler::Error) && (m_sampler->GetSampleCount() > 1))
    {
        CommitSurface();
    }

    ReleaseSampler();
}

void CAICoverSurface::ValidateGenerated()
{
    QString error;
    Vec3 pos = GetWorldPos();

    IErrorReport* errorReport = GetIEditor()->GetErrorReport();
    ICoverSystem::SurfaceInfo surfaceInfo;

    // Check surface is not empty
    {
        if (!m_surfaceID ||
            !gEnv->pAISystem->GetCoverSystem()->GetSurfaceInfo(m_surfaceID, &surfaceInfo) || (surfaceInfo.sampleCount < 2))
        {
            error = QStringLiteral("AI Cover Surface '%1' at (%2, %3, %4) is empty!").arg(GetName()).arg(pos.x, 0, 'f', 2).arg(pos.y, 0, 'f', 2).arg(pos.z, 0, 'f', 2);
            CErrorRecord err(this, CErrorRecord::ESEVERITY_WARNING, error);
            errorReport->ReportError(err);

            return;
        }
    }

    CNavigation* navigation = GetIEditor()->GetGameEngine()->GetNavigation();

    int buildingID = -1;
    IVisArea* visArea = 0;

    // Check surface doesn't cross a forbidden area
    bool inside;
    {
        const Vec3& firstSample = surfaceInfo.samples[0].position;
        inside = (navigation->CheckNavigationType(
                      firstSample, buildingID, visArea, IAISystem::NAV_WAYPOINT_HUMAN) != IAISystem::NAV_WAYPOINT_HUMAN) &&
            navigation->IsPointInForbiddenRegion(firstSample, 0, true) &&   navigation->IsPointInTriangulationAreas(firstSample);

        for (uint32 i = 1; i < surfaceInfo.sampleCount; ++i)
        {
            const Vec3& sample = surfaceInfo.samples[i].position;
            bool currInside = (navigation->CheckNavigationType(
                                   sample, buildingID, visArea, IAISystem::NAV_WAYPOINT_HUMAN) != IAISystem::NAV_WAYPOINT_HUMAN) &&
                navigation->IsPointInForbiddenRegion(sample, 0, true) &&    navigation->IsPointInTriangulationAreas(sample);

            if (inside != currInside)
            {
                error = QStringLiteral("AI Cover Surface '%1' at (%2, %3, %4) crosses a forbidden area near (%5, %6, %7)!")
                    .arg(GetName()).arg(pos.x, 0, 'f', 2).arg(pos.y, 0, 'f', 2).arg(pos.z, 0, 'f', 2).arg(sample.x, 0, 'f', 2).arg(sample.y, 0, 'f', 2).arg(sample.z, 0, 'f', 2);
                CErrorRecord err(this, CErrorRecord::ESEVERITY_WARNING, error);
                errorReport->ReportError(err);

                return;
            }
        }
    }

    // check if too far from forbidden area
    if (inside)
    {
        for (uint32 i = 0; i < surfaceInfo.sampleCount; ++i)
        {
            const float tolerance = 0.195f;
            const Vec3& sample = surfaceInfo.samples[i].position;

            bool pointInside = (navigation->CheckNavigationType(
                                    sample, buildingID, visArea, IAISystem::NAV_WAYPOINT_HUMAN) != IAISystem::NAV_WAYPOINT_HUMAN) &&
                navigation->IsPointInForbiddenRegion(sample, 0, true) &&    navigation->IsPointInTriangulationAreas(sample);

            if (pointInside && !navigation->IsPointOnForbiddenEdge(sample, tolerance))
            {
                error = QStringLiteral("AI Cover Surface '%1' at (%2, %3, %4) is too far away from enclosing forbidden area near (%5, %6, %7)!")
                    .arg(GetName()).arg(pos.x).arg(pos.y).arg(pos.z).arg(sample.x, 0, 'f', 2).arg(sample.y, 0, 'f', 2).arg(sample.z, 0, 'f', 2);
                CErrorRecord err(this, CErrorRecord::ESEVERITY_WARNING, error);
                errorReport->ReportError(err);

                return;
            }
        }
    }
}

void CAICoverSurface::CreateSampler()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    if (m_sampler)
    {
        m_sampler->Release();
    }

    m_sampler = gEnv->pAISystem->GetCoverSystem()->CreateCoverSampler();
}

void CAICoverSurface::ReleaseSampler()
{
    if (m_sampler)
    {
        m_sampler->Release();
        m_sampler = 0;
    }
}

void CAICoverSurface::StartSampling()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    ICoverSampler::Params params(GetParamsFromPropertyVars());

    Matrix34 worldTM = GetWorldTM();
    params.position = worldTM.GetTranslation();
    params.direction = worldTM.GetColumn1();

    m_sampler->StartSampling(params);
}

void CAICoverSurface::CommitSurface()
{
    uint32 sampleCount = m_sampler->GetSampleCount();
    if (sampleCount)
    {
        ICoverSystem::SurfaceInfo surfaceInfo;
        surfaceInfo.sampleCount = sampleCount;
        surfaceInfo.samples = m_sampler->GetSamples();
        surfaceInfo.flags = m_sampler->GetSurfaceFlags();

        if (!m_surfaceID)
        {
            m_surfaceID = gEnv->pAISystem->GetCoverSystem()->AddSurface(surfaceInfo);
        }
        else
        {
            gEnv->pAISystem->GetCoverSystem()->UpdateSurface(m_surfaceID, surfaceInfo);
        }

        Matrix34 invWorldTM = GetWorldTM().GetInverted();
        m_aabbLocal = AABB(AABB::RESET);

        for (uint32 i = 0; i < surfaceInfo.sampleCount; ++i)
        {
            const ICoverSampler::Sample& sample = surfaceInfo.samples[i];

            Vec3 position = invWorldTM.TransformPoint(sample.position);

            m_aabbLocal.Add(position);
            m_aabbLocal.Add(position + Vec3(0.0f, 0.0f, sample.GetHeight()));
        }

        m_aabb = m_sampler->GetAABB();
    }
    else
    {
        ClearSurface();
    }
}

void CAICoverSurface::ClearSurface()
{
    if (m_surfaceID)
    {
        if (gEnv->pAISystem)
        {
            gEnv->pAISystem->GetCoverSystem()->RemoveSurface(m_surfaceID);
        }
        m_surfaceID = CoverSurfaceID(0);
    }

    m_aabb = AABB(AABB::RESET);
    m_aabbLocal = AABB(AABB::RESET);
}

void CAICoverSurface::SetPropertyVarsFromParams(const ICoverSampler::Params& params)
{
    m_propertyValues.limitLeft = params.limitLeft;
    m_propertyValues.limitRight = params.limitRight;

    m_propertyValues.limitDepth = params.limitDepth;
    m_propertyValues.limitHeight = params.limitHeight;

    m_propertyValues.widthInterval = params.widthSamplerInterval;
    m_propertyValues.heightInterval = params.heightSamplerInterval;

    m_propertyValues.maxStartHeight = params.maxStartHeight;
    m_propertyValues.simplifyThreshold = params.simplifyThreshold;
}

ICoverSampler::Params CAICoverSurface::GetParamsFromPropertyVars()
{
    ICoverSampler::Params params;

    params.limitDepth = m_propertyValues.limitDepth;
    params.limitHeight = m_propertyValues.limitHeight;

    params.limitLeft = m_propertyValues.limitLeft;
    params.limitRight = m_propertyValues.limitRight;

    params.widthSamplerInterval = m_propertyValues.widthInterval;
    params.heightSamplerInterval = m_propertyValues.heightInterval;

    params.maxStartHeight = m_propertyValues.maxStartHeight;
    params.simplifyThreshold = m_propertyValues.simplifyThreshold;

    return params;
}

void CAICoverSurface::Serialize(CObjectArchive& archive)
{
    CBaseObject::Serialize(archive);

    if (!archive.bUndo)
    {
        SerializeValue(archive, "SurfaceID", m_surfaceID);
    }
    m_propertyValues.Serialize(*this, archive);
}

template<>
void CAICoverSurface::SerializeVarEnum<QString>(CObjectArchive& archive, const char* name, CSmartVariableEnum<QString>& value)
{
    if (archive.bLoading)
    {
        const char* saved;
        if (archive.node->getAttr(name, &saved))
        {
            value = saved;
        }
    }
    else
    {
        QString current;
        value->Get(current);

        archive.node->setAttr(name, current.toUtf8().data());
    }
};

template<>
void CAICoverSurface::SerializeValue(CObjectArchive& archive, const char* name, CoverSurfaceID& value)
{
    uint32 id = value;
    if (archive.bLoading)
    {
        archive.node->getAttr(name, id);
        value = CoverSurfaceID(id);
    }
    else
    {
        archive.node->setAttr(name, id);
    }
};

template<>
void CAICoverSurface::SerializeValue<QString>(CObjectArchive& archive, const char* name, QString& value)
{
    if (archive.bLoading)
    {
        const char* saved;
        if (archive.node->getAttr(name, &saved))
        {
            value = saved;
        }
    }
    else
    {
        archive.node->setAttr(name, value.toUtf8().data());
    }
};

void CAICoverSurface::PropertyValues::Serialize(CAICoverSurface& object, CObjectArchive& archive)
{
    object.SerializeVarEnum(archive, "Sampler", sampler);
    object.SerializeVar(archive, "LimitDepth", limitDepth);
    object.SerializeVar(archive, "LimitHeight", limitHeight);
    object.SerializeVar(archive, "LimitLeft", limitLeft);
    object.SerializeVar(archive, "LimitRight", limitRight);
    object.SerializeVar(archive, "SampleWidth", widthInterval);
    object.SerializeVar(archive, "SampleHeight", heightInterval);
    object.SerializeVar(archive, "MaxStartHeight", maxStartHeight);
    object.SerializeVar(archive, "SimplifyThreshold", simplifyThreshold);
}

XmlNodeRef CAICoverSurface::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    return 0;
}

void CAICoverSurface::InvalidateTM(int whyFlags)
{
    CBaseObject::InvalidateTM(whyFlags);

    if (!m_sampler)
    {
        CreateSampler();
    }

    StartSampling();
}

void CAICoverSurface::SetSelected(bool bSelected)
{
    CBaseObject::SetSelected(bSelected);

    if (bSelected)
    {
        ClearSurface();
        ReleaseSampler();

        CreateSampler();
        StartSampling();
    }
}

void CAICoverSurface::BeginEditParams(IEditor* editor, int flags)
{
    CBaseObject::BeginEditParams(editor, flags);

    if (!s_properties)
    {
        s_properties = new ReflectedPropertiesPanel;
        s_properties->Setup(true, 180);
    }
    else
    {
        s_properties->DeleteVars();
    }
    s_properties->AddVars(m_propertyVars.get());

    if (!s_propertiesID)
    {
        s_propertiesID = AddUIPage(QString(GetTypeName() + " Properties").toUtf8().data(), s_properties);
    }

    if (s_properties)
    {
        s_properties->SetUpdateCallback(functor(*this, &CBaseObject::OnPropertyChanged));
    }
}

void CAICoverSurface::EndEditParams(IEditor* editor)
{
    if (s_properties)
    {
        s_properties->ClearUpdateCallback();
    }

    if (s_propertiesID)
    {
        RemoveUIPage(s_propertiesID);
        s_propertiesID = 0;
        s_properties = 0;
    }

    CBaseObject::EndEditParams(editor);
}

void CAICoverSurface::BeginEditMultiSelParams(bool allSameType)
{
    CBaseObject::BeginEditMultiSelParams(allSameType);

    if (!allSameType)
    {
        return;
    }

    if (!s_properties)
    {
        s_properties = new ReflectedPropertiesPanel;
        s_properties->Setup(true, 180);
    }
    else
    {
        s_properties->DeleteVars();
    }

    s_propertiesID = AddUIPage(QString(GetTypeName() + " Properties").toUtf8().data(), s_properties);

    if (m_propertyVars.get())
    {
        CSelectionGroup* selectionGroup = GetIEditor()->GetSelection();
        for (int i = 0; i < selectionGroup->GetCount(); ++i)
        {
            CAICoverSurface* surfaceObject = (CAICoverSurface*)selectionGroup->GetObject(i);
            if (CVarBlockPtr surfaceObjectVars = surfaceObject->m_propertyVars.get())
            {
                s_properties->AddVars(surfaceObjectVars);
            }
        }

        if (!s_propertiesID)
        {
            s_propertiesID = AddUIPage(QString(GetTypeName() + " Properties").toUtf8().data(), s_properties);
        }
    }

    if (s_properties)
    {
        s_properties->SetUpdateCallback(functor(*this, &CBaseObject::OnMultiSelPropertyChanged));
    }
}

void CAICoverSurface::EndEditMultiSelParams()
{
    if (s_properties)
    {
        s_properties->ClearUpdateCallback();
    }

    if (s_propertiesID)
    {
        RemoveUIPage(s_propertiesID);
        s_propertiesID = 0;
        s_properties = 0;
    }

    CBaseObject::EndEditMultiSelParams();
}

void CAICoverSurface::OnPropertyVarChange(IVariable* var)
{
    ReleaseSampler();
    CreateSampler();

    StartSampling();
}

void CAICoverSurface::CreatePropertyVars()
{
    m_propertyVars.reset(new CVarBlock());

    m_propertyVars->AddVariable(m_propertyValues.sampler, "Sampler");
    m_propertyVars->AddVariable(m_propertyValues.limitLeft, "Limit Left");
    m_propertyVars->AddVariable(m_propertyValues.limitRight, "Limit Right");
    m_propertyVars->AddVariable(m_propertyValues.limitDepth, "Limit Depth");
    m_propertyVars->AddVariable(m_propertyValues.limitHeight, "Limit Height");

    m_propertyVars->AddVariable(m_propertyValues.widthInterval, "Sample Width");
    m_propertyVars->AddVariable(m_propertyValues.heightInterval, "Sample Height");

    m_propertyVars->AddVariable(m_propertyValues.maxStartHeight, "Max Start Height");

    m_propertyVars->AddVariable(m_propertyValues.simplifyThreshold, "Simplification Threshold");

    m_propertyValues.sampler->AddEnumItem(QString("Default"), QString("Default"));
    m_propertyValues.sampler->Set(QString("Default"));

    m_propertyValues.limitLeft->SetLimits(0.0f, 50.0f, 0.05f);
    m_propertyValues.limitRight->SetLimits(0.0f, 50.0f, 0.05f);
    m_propertyValues.limitDepth->SetLimits(0.0f, 10.0f, 0.05f);
    m_propertyValues.limitHeight->SetLimits(0.05f, 50.0f, 0.05f);
    m_propertyValues.widthInterval->SetLimits(0.05f, 1.0f, 0.05f);
    m_propertyValues.heightInterval->SetLimits(0.05f, 1.0f, 0.05f);
    m_propertyValues.maxStartHeight->SetLimits(0.0f, 2.0f, 0.05f);
    m_propertyValues.simplifyThreshold->SetLimits(0.0f, 1.0f, 0.005f);

    m_propertyVars->AddOnSetCallback(functor(*this, &CAICoverSurface::OnPropertyVarChange));
}

void CAICoverSurface::DisplayBadCoverSurfaceObject()
{
    IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();

    const SAuxGeomRenderFlags oldFlags = pRenderAux->GetRenderFlags();
    SAuxGeomRenderFlags renderFlags(oldFlags);

    renderFlags.SetAlphaBlendMode(e_AlphaBlended);

    pRenderAux->SetRenderFlags(renderFlags);

    const float time = gEnv->pTimer->GetAsyncCurTime();

    const float alpha = clamp_tpl((1.0f + sinf(time * 2.5f)) * 0.5f, 0.25f, 0.7f);
    const ColorB color (255, 0, 0, (uint8)(alpha * 255));

    const Matrix34& objectTM = GetWorldTM();

    AABB objectLocalBBox;
    GetLocalBounds(objectLocalBBox);
    objectLocalBBox.Expand(Vec3(0.15f, 0.15f, 0.15f));

    const float height = 1.5f;
    const float halfHeight = height * 0.5f;
    const float objectRadius = objectLocalBBox.GetRadius();
    const Vec3 objectCenter = objectTM.TransformPoint(objectLocalBBox.GetCenter());

    pRenderAux->DrawSphere(objectCenter, objectRadius, color, true);

    pRenderAux->DrawCylinder(objectCenter + (objectTM.GetColumn0() * (halfHeight + objectRadius)), objectTM.GetColumn0(), 0.25f, height, color, true);
    pRenderAux->DrawCylinder(objectCenter + (objectTM.GetColumn1() * (halfHeight + objectRadius)), objectTM.GetColumn1(), 0.25f, height, color, true);
    pRenderAux->DrawCylinder(objectCenter + (objectTM.GetColumn2() * (halfHeight + objectRadius)), objectTM.GetColumn2(), 0.25f, height, color, true);

    pRenderAux->SetRenderFlags(oldFlags);
}

#include <Objects/AICoverSurface.moc>