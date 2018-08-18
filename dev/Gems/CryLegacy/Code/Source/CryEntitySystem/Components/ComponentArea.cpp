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
#include "CryLegacy_precompiled.h"
#include "ComponentArea.h"
#include "Components/IComponentSerialization.h"

#include "Entity.h"
#include "ISerialize.h"

std::vector<Vec3> CComponentArea::s_tmpWorldPoints;

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentArea, IComponentArea)

void CComponentArea::ResetTempState()
{
    stl::free_container(s_tmpWorldPoints);
}

//////////////////////////////////////////////////////////////////////////
CComponentArea::CComponentArea()
    : m_pEntity(NULL)
    , m_pArea(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
CComponentArea::~CComponentArea()
{
    SAFE_RELEASE(m_pArea);
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;
    m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentArea>(SerializationOrder::Area, *this, &CComponentArea::Serialize, &CComponentArea::SerializeXML, &CComponentArea::NeedSerialize, &CComponentArea::GetSignature);

    m_pArea = static_cast<CAreaManager*>(GetIEntitySystem()->GetAreaManager())->CreateArea();
    m_pArea->SetEntityID(m_pEntity->GetId());

    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    m_pEntity = pEntity;

    assert(m_pArea);
    if (m_pArea)
    {
        m_pArea->SetEntityID(pEntity->GetId());
    }

    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::Reset()
{
    m_nFlags = 0;

    m_vCenter.Set(0, 0, 0);
    m_fRadius = 0;
    m_fGravity = 0;
    m_fFalloff = 0.8f;
    m_fDamping = 1.0f;
    m_bDontDisableInvisible = false;

    m_bIsEnable = true;
    m_bIsEnableInternal = true;
    m_lastFrameTime = 0.f;
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::OnMove()
{
    if (!(m_nFlags & FLAG_NOT_UPDATE_AREA))
    {
        EEntityAreaType type = m_pArea->GetAreaType();
        if (type == ENTITY_AREA_TYPE_SHAPE)
        {
            const Matrix34& tm = m_pEntity->GetWorldTM();
            s_tmpWorldPoints.resize(m_localPoints.size());
            //////////////////////////////////////////////////////////////////////////
            for (unsigned int i = 0; i < m_localPoints.size(); i++)
            {
                s_tmpWorldPoints[i] = tm.TransformPoint(m_localPoints[i]);
            }
            if (!s_tmpWorldPoints.empty())
            {
                unsigned int const nPointsCount = s_tmpWorldPoints.size();
                bool* const pbObstructSound = new bool[nPointsCount];
                for (unsigned int i = 0; i < nPointsCount; ++i)
                {
                    // Here we "un-pack" the data! (1 bit*nPointsCount to 1 byte*nPointsCount)
                    pbObstructSound[i] = m_abObstructSound[i];
                }

                m_pArea->SetPoints(&s_tmpWorldPoints[0], &pbObstructSound[0], nPointsCount);
                delete[] pbObstructSound;
            }
            else
            {
                m_pArea->SetPoints(0, NULL, 0);
            }
        }
        else if (type == ENTITY_AREA_TYPE_BOX)
        {
            m_pArea->SetMatrix(m_pEntity->GetWorldTM());

            assert(m_abObstructSound.size() == 6);
            size_t nIndex = 0;
            tSoundObstructionIterConst const ItEnd = m_abObstructSound.end();
            for (tSoundObstructionIterConst It = m_abObstructSound.begin(); It != ItEnd; ++It)
            {
                bool const bObstructed = (bool)(*It);
                m_pArea->SetSoundObstructionOnAreaFace(nIndex, bObstructed);
                ++nIndex;
            }
        }
        else if (type == ENTITY_AREA_TYPE_SPHERE)
        {
            m_pArea->SetSphere(m_pEntity->GetWorldTM().TransformPoint(m_vCenter), m_fRadius);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::OnEnable(bool bIsEnable, bool bIsCallScript)
{
    m_bIsEnable = bIsEnable;
    if (m_pArea->GetAreaType() == ENTITY_AREA_TYPE_GRAVITYVOLUME)
    {
        SEntityPhysicalizeParams physparams;
        if (bIsEnable && m_bIsEnableInternal)
        {
            physparams.pAreaDef = &m_areaDefinition;
            m_areaDefinition.areaType = SEntityPhysicalizeParams::AreaDefinition::AREA_SPLINE;
            m_bezierPointsTmp.resize(m_bezierPoints.size());
            memcpy(&m_bezierPointsTmp[0], &m_bezierPoints[0], m_bezierPoints.size() * sizeof(Vec3));
            m_areaDefinition.pPoints = &m_bezierPointsTmp[0];
            m_areaDefinition.nNumPoints = m_bezierPointsTmp.size();
            m_areaDefinition.fRadius = m_fRadius;
            m_gravityParams.gravity = Vec3(0, 0, m_fGravity);
            m_gravityParams.falloff0 = m_fFalloff;
            m_gravityParams.damping = m_fDamping;
            physparams.type = PE_AREA;
            m_areaDefinition.pGravityParams = &m_gravityParams;

            m_pEntity->SetTimer(0, 11000);
        }
        m_pEntity->Physicalize(physparams);

        if (bIsCallScript)
        {
            //call the OnEnable function in the script, to set game flags for this entity and such.
            IScriptTable* pScriptTable = m_pEntity->GetScriptTable();
            if (pScriptTable)
            {
                HSCRIPTFUNCTION scriptFunc(NULL);
                pScriptTable->GetValue("OnEnable", scriptFunc);

                if (scriptFunc)
                {
                    Script::Call(gEnv->pScriptSystem, scriptFunc, pScriptTable, bIsEnable);
                }

                gEnv->pScriptSystem->ReleaseFunc(scriptFunc);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CComponentArea::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_XFORM:
        OnMove();
        break;
    case ENTITY_EVENT_SCRIPT_EVENT:
    {
        const char* pName = (const char*)event.nParam[0];
        if (!_stricmp(pName, "Enable"))
        {
            OnEnable(true);
        }
        else if (!_stricmp(pName, "Disable"))
        {
            OnEnable(false);
        }
    }
    break;
    case ENTITY_EVENT_RENDER:
    {
        if (m_pArea->GetAreaType() == ENTITY_AREA_TYPE_GRAVITYVOLUME)
        {
            if (!m_bDontDisableInvisible)
            {
                m_lastFrameTime = gEnv->pTimer->GetCurrTime();
            }
            if (!m_bIsEnableInternal)
            {
                m_bIsEnableInternal = true;
                OnEnable(m_bIsEnable, false);
                m_pEntity->SetTimer(0, 11000);
            }
        }
    }
    break;
    case ENTITY_EVENT_TIMER:
    {
        if (m_pArea->GetAreaType() == ENTITY_AREA_TYPE_GRAVITYVOLUME)
        {
            if (!m_bDontDisableInvisible)
            {
                bool bOff = false;
                if (gEnv->pTimer->GetCurrTime() - m_lastFrameTime > 10.0f)
                {
                    bOff = true;
                    IComponentRenderPtr pEntPr = m_pEntity->GetComponent<IComponentRender>();
                    if (pEntPr)
                    {
                        IRenderNode* pRendNode = pEntPr->GetRenderNode();
                        if (pRendNode)
                        {
                            if (pEntPr->IsRenderComponentVisAreaVisible())
                            {
                                bOff = false;
                            }
                        }
                    }
                    if (bOff)
                    {
                        m_bIsEnableInternal = false;
                        OnEnable(m_bIsEnable, false);
                    }
                }
                if (!bOff)
                {
                    m_pEntity->SetTimer(0, 11000);
                }
            }
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::SerializeXML(XmlNodeRef& entityNode, bool bLoading)
{
    if (m_nFlags & FLAG_NOT_SERIALIZE)
    {
        return;
    }

    if (bLoading)
    {
        XmlNodeRef areaNode = entityNode->findChild("Area");
        if (!areaNode)
        {
            return;
        }

        int nId = 0, nGroup = 0, nPriority = 0;
        float fProximity = 0;
        float fHeight = 0;

        areaNode->getAttr("Id", nId);
        areaNode->getAttr("Group", nGroup);
        areaNode->getAttr("Proximity", fProximity);
        areaNode->getAttr("Priority", nPriority);
        m_pArea->SetID(nId);
        m_pArea->SetGroup(nGroup);
        m_pArea->SetProximity(fProximity);
        m_pArea->SetPriority(nPriority);
        const char* token(0);

        XmlNodeRef pointsNode = areaNode->findChild("Points");
        if (pointsNode)
        {
            for (int i = 0; i < pointsNode->getChildCount(); i++)
            {
                XmlNodeRef pntNode = pointsNode->getChild(i);
                Vec3 pos;
                if (pntNode->getAttr("Pos", pos))
                {
                    m_localPoints.push_back(pos);
                }

                // Get sound obstruction
                bool bObstructSound = 0;
                pntNode->getAttr("ObstructSound", bObstructSound);
                m_abObstructSound.push_back(bObstructSound);
            }
            m_pArea->SetAreaType(ENTITY_AREA_TYPE_SHAPE);

            areaNode->getAttr("Height", fHeight);
            m_pArea->SetHeight(fHeight);
            // Set points.
            OnMove();
        }
        else if (areaNode->getAttr("SphereRadius", m_fRadius))
        {
            // Sphere.
            areaNode->getAttr("SphereCenter", m_vCenter);
            m_pArea->SetSphere(m_pEntity->GetWorldTM().TransformPoint(m_vCenter), m_fRadius);
        }
        else if (areaNode->getAttr("VolumeRadius", m_fRadius))
        {
            areaNode->getAttr("Gravity", m_fGravity);
            areaNode->getAttr("DontDisableInvisible", m_bDontDisableInvisible);

            AABB box;
            box.Reset();

            // Bezier Volume.
            pointsNode = areaNode->findChild("BezierPoints");
            if (pointsNode)
            {
                for (int i = 0; i < pointsNode->getChildCount(); i++)
                {
                    XmlNodeRef pntNode = pointsNode->getChild(i);
                    Vec3 pt;
                    if (pntNode->getAttr("Pos", pt))
                    {
                        m_bezierPoints.push_back(pt);
                        box.Add(pt);
                    }
                }
            }
            m_pArea->SetAreaType(ENTITY_AREA_TYPE_GRAVITYVOLUME);
            if (!m_pEntity->GetComponent<IComponentRender>())
            {
                IComponentRenderPtr pRenderComponent = m_pEntity->GetOrCreateComponent<IComponentRender>();
                m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_SEND_RENDER_EVENT);

                if (box.min.x > box.max.x)
                {
                    box.min = box.max = Vec3(0, 0, 0);
                }
                box.min -= Vec3(m_fRadius, m_fRadius, m_fRadius);
                box.max += Vec3(m_fRadius, m_fRadius, m_fRadius);

                Matrix34 tm = m_pEntity->GetWorldTM();
                box.SetTransformedAABB(m_pEntity->GetWorldTM().GetInverted(), box);

                pRenderComponent->SetLocalBounds(box, true);
            }

            OnEnable(m_bIsEnable);
        }
        else if (areaNode->getAttr("AreaSolidFileName", &token))
        {
            CCryFile file;

            int nAliasLen = sizeof("%level%") - 1;
            const char* areaSolidFileName;
            if (strncmp(token, "%level%", nAliasLen) == 0)
            {
                areaSolidFileName = GetIEntitySystem()->GetSystem()->GetI3DEngine()->GetLevelFilePath(token + nAliasLen);
            }
            else
            {
                areaSolidFileName = token;
            }

            if (file.Open(areaSolidFileName, "rb"))
            {
                int numberOfClosedPolygon = 0;
                int numberOfOpenPolygon = 0;

                m_pArea->BeginSettingSolid(m_pEntity->GetWorldTM());

                file.ReadType(&numberOfClosedPolygon);
                file.ReadType(&numberOfOpenPolygon);

                ReadPolygonsForAreaSolid(file, numberOfClosedPolygon, true);
                ReadPolygonsForAreaSolid(file, numberOfOpenPolygon, false);

                m_pArea->EndSettingSolid();
            }
        }
        else
        {
            // Box.
            Vec3 bmin(0, 0, 0), bmax(0, 0, 0);
            areaNode->getAttr("BoxMin", bmin);
            areaNode->getAttr("BoxMax", bmax);
            m_pArea->SetBox(bmin, bmax, m_pEntity->GetWorldTM());

            // Get sound obstruction
            XmlNodeRef const pNodeSoundData = areaNode->findChild("SoundData");
            if (pNodeSoundData)
            {
                assert(m_abObstructSound.size() == 0);

                for (int i = 0; i < pNodeSoundData->getChildCount(); ++i)
                {
                    XmlNodeRef const pNodeSide = pNodeSoundData->getChild(i);

                    if (pNodeSide)
                    {
                        bool bObstructSound = false;
                        pNodeSide->getAttr("ObstructSound", bObstructSound);
                        m_abObstructSound.push_back(bObstructSound);
                    }
                }
            }

            OnMove();
        }

        m_pArea->ClearEntities();
        XmlNodeRef entitiesNode = areaNode->findChild("Entities");
        // Export Entities.
        if (entitiesNode)
        {
            for (int i = 0; i < entitiesNode->getChildCount(); i++)
            {
                XmlNodeRef entNode = entitiesNode->getChild(i);
                EntityId entityId;
                if (entNode->getAttr("Id", entityId) && (entityId != INVALID_ENTITYID))
                {
                    m_pArea->AddEntity(entityId);
                }
            }
        }
    }
    else
    {
        // Save points.
        XmlNodeRef areaNode = entityNode->newChild("Area");
        areaNode->setAttr("Id", m_pArea->GetID());
        areaNode->setAttr("Group", m_pArea->GetGroup());
        areaNode->setAttr("Proximity", m_pArea->GetProximity());
        areaNode->setAttr("Priority", m_pArea->GetPriority());
        EEntityAreaType type = m_pArea->GetAreaType();
        if (type == ENTITY_AREA_TYPE_SHAPE)
        {
            XmlNodeRef pointsNode = areaNode->newChild("Points");
            for (unsigned int i = 0; i < m_localPoints.size(); i++)
            {
                XmlNodeRef pntNode = pointsNode->newChild("Point");
                pntNode->setAttr("Pos", m_localPoints[i]);
                pntNode->setAttr("ObstructSound", m_abObstructSound[i]);
            }
            areaNode->setAttr("Height", m_pArea->GetHeight());
        }
        else if (type == ENTITY_AREA_TYPE_SPHERE)
        {
            // Box.
            areaNode->setAttr("SphereCenter", m_vCenter);
            areaNode->setAttr("SphereRadius", m_fRadius);
        }
        else if (type == ENTITY_AREA_TYPE_BOX)
        {
            // Box.
            Vec3 bmin, bmax;
            m_pArea->GetBox(bmin, bmax);
            areaNode->setAttr("BoxMin", bmin);
            areaNode->setAttr("BoxMax", bmax);

            // Set sound obstruction
            XmlNodeRef const pNodeSoundData = areaNode->newChild("SoundData");
            if (pNodeSoundData)
            {
                assert(m_abObstructSound.size() == 6);
                size_t nIndex = 0;
                tSoundObstructionIterConst const ItEnd = m_abObstructSound.end();
                for (tSoundObstructionIterConst It = m_abObstructSound.begin(); It != ItEnd; ++It)
                {
                    bool const bObstructed = (bool)(*It);
                    stack_string sTemp;
                    sTemp.Format("Side%d", ++nIndex);

                    XmlNodeRef const pNodeSide = pNodeSoundData->newChild(sTemp.c_str());
                    pNodeSide->setAttr("ObstructSound", bObstructed);
                }
            }
        }
        else if (type == ENTITY_AREA_TYPE_GRAVITYVOLUME)
        {
            areaNode->setAttr("VolumeRadius", m_fRadius);
            areaNode->setAttr("Gravity", m_fGravity);
            areaNode->setAttr("DontDisableInvisible", m_bDontDisableInvisible);
            XmlNodeRef pointsNode = areaNode->newChild("BezierPoints");
            for (unsigned int i = 0; i < m_bezierPoints.size(); i++)
            {
                XmlNodeRef pntNode = pointsNode->newChild("Point");
                pntNode->setAttr("Pos", m_bezierPoints[i]);
            }
        }

        const std::vector<EntityId>& entIDs = *m_pArea->GetEntities();
        // Export Entities.
        if (!entIDs.empty())
        {
            XmlNodeRef nodes = areaNode->newChild("Entities");
            for (uint32 i = 0; i < entIDs.size(); i++)
            {
                int entityId = entIDs[i];
                XmlNodeRef entNode = nodes->newChild("Entity");
                entNode->setAttr("Id", entityId);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::ReadPolygonsForAreaSolid(CCryFile& file, int numberOfPolygons, bool bObstruction)
{
    static const int numberOfStaticVertices(200);
    Vec3 pStaticVertices[numberOfStaticVertices];

    for (int i = 0; i < numberOfPolygons; ++i)
    {
        int numberOfVertices(0);
        file.ReadType(&numberOfVertices);
        Vec3* pVertices(NULL);
        if (numberOfVertices > numberOfStaticVertices)
        {
            pVertices = new Vec3[numberOfVertices];
        }
        else
        {
            pVertices = pStaticVertices;
        }
        for (int k = 0; k < numberOfVertices; ++k)
        {
            file.ReadType(&pVertices[k]);
        }
        m_pArea->AddConvexHullToSolid(pVertices, bObstruction, numberOfVertices);
        if (pVertices != pStaticVertices)
        {
            delete [] pVertices;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CComponentArea::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentArea");
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::Serialize(TSerialize ser)
{
    if (m_nFlags & FLAG_NOT_SERIALIZE)
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::SetPoints(const Vec3* const vPoints, const bool* const pabSoundObstructionSegments, int const nPointsCount, float const fHeight)
{
    m_localPoints.resize(nPointsCount);
    m_abObstructSound.resize(nPointsCount);
    if (nPointsCount > 0)
    {
        memcpy(&m_localPoints[0], vPoints, nPointsCount * sizeof(Vec3));

        // Here we pack the data again! (1 byte*nPointsCount to 1 bit*nPointsCount)
        for (int i = 0; i < nPointsCount; ++i)
        {
            m_abObstructSound[i] = pabSoundObstructionSegments[i];
        }
    }
    m_pArea->SetAreaType(ENTITY_AREA_TYPE_SHAPE);
    m_pArea->SetHeight(fHeight);
    OnMove();
}

//////////////////////////////////////////////////////////////////////////
const Vec3* CComponentArea::GetPoints()
{
    if (m_localPoints.empty())
    {
        return 0;
    }
    return &m_localPoints[0];
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::SetBox(const Vec3& min, const Vec3& max, const bool* const pabSoundObstructionSides, size_t const nSideCount)
{
    m_localPoints.clear();
    m_abObstructSound.clear();
    m_abObstructSound.reserve(6);

    // Here we pack the data again! (1 byte*nSideCount to 1 bit*nSideCount)
    for (size_t i = 0; i < nSideCount; ++i)
    {
        m_abObstructSound.push_back(pabSoundObstructionSides[i]);
    }

    m_pArea->SetBox(min, max, m_pEntity->GetWorldTM());
    OnMove();
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::SetSphere(const Vec3& vCenter, float fRadius)
{
    m_vCenter = vCenter;
    m_fRadius = fRadius;
    m_localPoints.clear();
    m_pArea->SetSphere(m_pEntity->GetWorldTM().TransformPoint(m_vCenter), fRadius);
    m_pArea->SetAreaType(ENTITY_AREA_TYPE_SPHERE);
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::BeginSettingSolid(const Matrix34& worldTM)
{
    m_pArea->BeginSettingSolid(worldTM);
    m_pArea->SetAreaType(ENTITY_AREA_TYPE_SOLID);
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::AddConvexHullToSolid(const Vec3* verticesOfConvexHull, bool bObstruction, int numberOfVertices)
{
    m_pArea->AddConvexHullToSolid(verticesOfConvexHull, bObstruction, numberOfVertices);
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::EndSettingSolid()
{
    m_pArea->EndSettingSolid();
}

//////////////////////////////////////////////////////////////////////////
void CComponentArea::SetGravityVolume(const Vec3* pPoints, int nNumPoints, float fRadius, float fGravity, bool bDontDisableInvisible, float fFalloff, float fDamping)
{
    m_bIsEnableInternal = true;
    m_fRadius = fRadius;
    m_fGravity = fGravity;
    m_fFalloff = fFalloff;
    m_fDamping = fDamping;
    m_bDontDisableInvisible = bDontDisableInvisible;

    m_bezierPoints.resize(nNumPoints);
    if (nNumPoints > 0)
    {
        memcpy(&m_bezierPoints[0], pPoints, nNumPoints * sizeof(Vec3));
    }

    if (!bDontDisableInvisible)
    {
        m_pEntity->SetTimer(0, 11000);
    }

    m_pArea->SetAreaType(ENTITY_AREA_TYPE_GRAVITYVOLUME);
}

//////////////////////////////////////////////////////////////////////////
size_t CComponentArea::GetNumberOfEntitiesInArea() const
{
    return m_pArea->GetEntityAmount();
}

//////////////////////////////////////////////////////////////////////////
EntityId CComponentArea::GetEntityInAreaByIdx(size_t index) const
{
    return m_pArea->GetEntityByIdx(index);
}
