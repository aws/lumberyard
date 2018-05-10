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

// Description : CRiverObject implementation.


#include "StdAfx.h"
#include "RiverObject.h"
#include "Material/Material.h"
#include <I3DEngine.h>
#include "EntityObject.h"


//////////////////////////////////////////////////////////////////////////
// CRiverObject implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CRiverObject::CRiverObject()
{
    SetColor(Vec2Rgb(Vec3(0, 0, 1)));

    m_waterVolumeID = -1;
    m_fogPlane = Plane(Vec3(0, 0, 1), 0);
}

//////////////////////////////////////////////////////////////////////////
bool CRiverObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CRoadObject::Init(ie, prev, file);

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::InitVariables()
{
    //CRoadObject::InitVariables();
    CRoadObject::InitBaseVariables();

    mv_waterVolumeDepth = 10.0f;
    mv_waterStreamSpeed = 0.0f;
    mv_waterFogDensity = 0.5f;
    mv_waterFogColor = Vec3(0.005f, 0.01f, 0.02f);
    mv_waterFogColorMultiplier = 0.5f;
    mv_waterFogColorAffectedBySun = true;
    mv_waterFogShadowing = 0.5f;
    mv_waterSurfaceUScale = 1.0f;
    mv_waterSurfaceVScale = 1.0f;
    mv_waterCapFogAtVolumeDepth = false;
    mv_waterCaustics = true;
    mv_waterCausticIntensity = 1.0f;
    mv_waterCausticTiling = 1.0f;
    mv_waterCausticHeight = 0.5f;

    AddVariable(mv_waterVolumeDepth, "VolumeDepth", "Depth", functor(*this, &CRiverObject::OnRiverPhysicsParamChange));
    AddVariable(mv_waterStreamSpeed, "StreamSpeed", "Speed", functor(*this, &CRiverObject::OnRiverPhysicsParamChange));
    AddVariable(mv_waterFogDensity, "FogDensity", "FogDensity", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterFogColor, "FogColor", "FogColor", functor(*this, &CRiverObject::OnRiverParamChange), IVariable::DT_COLOR);
    AddVariable(mv_waterFogColorMultiplier, "FogColorMultiplier", "FogColorMultiplier", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterFogColorAffectedBySun, "FogColorAffectedBySun", "FogColorAffectedBySun", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterFogShadowing, "FogShadowing", "FogShadowing", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterCapFogAtVolumeDepth, "CapFogAtVolumeDepth", "CapFogAtVolumeDepth", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterSurfaceUScale, "UScale", "UScale", functor(*this, &CRiverObject::OnParamChange));
    AddVariable(mv_waterSurfaceVScale, "VScale", "VScale", functor(*this, &CRiverObject::OnParamChange));
    AddVariable(mv_waterCaustics, "Caustics", "Caustics", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterCausticIntensity, "CausticIntensity", "CausticIntensity", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterCausticTiling, "CausticTiling", "CausticTiling", functor(*this, &CRiverObject::OnRiverParamChange));
    AddVariable(mv_waterCausticHeight, "CausticHeight", "CausticHeight", functor(*this, &CRiverObject::OnRiverParamChange));
}

//////////////////////////////////////////////////////////////////////////
Vec3 CRiverObject::GetPremulFogColor() const
{
    return Vec3(mv_waterFogColor) * max(float( mv_waterFogColorMultiplier ), 0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::OnRiverParamChange(IVariable* var)
{
    for (size_t i(0); i < m_sectors.size(); ++i)
    {
        if (m_sectors[i].m_pRoadSector)
        {
            IWaterVolumeRenderNode* pRenderNode(static_cast< IWaterVolumeRenderNode* >(m_sectors[i].m_pRoadSector));
            pRenderNode->SetFogDensity(mv_waterFogDensity);
            pRenderNode->SetFogColor(GetPremulFogColor());
            pRenderNode->SetFogColorAffectedBySun(mv_waterFogColorAffectedBySun);
            pRenderNode->SetFogShadowing(mv_waterFogShadowing);
            pRenderNode->SetCapFogAtVolumeDepth(mv_waterCapFogAtVolumeDepth);
            pRenderNode->SetCaustics(mv_waterCaustics);
            pRenderNode->SetCausticIntensity(mv_waterCausticIntensity);
            pRenderNode->SetCausticTiling(mv_waterCausticTiling);
            pRenderNode->SetCausticHeight(mv_waterCausticHeight);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::Physicalize()
{
    for (size_t i(0); i < m_sectors.size(); ++i)
    {
        if (m_sectors[i].m_pRoadSector)
        {
            IWaterVolumeRenderNode* pRenderNode(static_cast<IWaterVolumeRenderNode*>(m_sectors[i].m_pRoadSector));
            pRenderNode->SetVolumeDepth(mv_waterVolumeDepth);
            pRenderNode->SetStreamSpeed(mv_waterStreamSpeed);
        }
    }

    if (!m_sectors.empty() && m_sectors[0].m_pRoadSector)
    {
        IWaterVolumeRenderNode* pRenderNode(static_cast< IWaterVolumeRenderNode* >(m_sectors[0].m_pRoadSector));

        pRenderNode->Dephysicalize();

        //////////////////////////////////////////////////////////////////////////
        size_t numRiverSegments(m_sectors.size());

        std::vector< Vec3 > outline;
        outline.reserve(2 * (numRiverSegments + 1));

        for (size_t i(0); i < numRiverSegments; ++i)
        {
            outline.push_back(m_sectors[ i ].points[1]);
        }

        outline.push_back(m_sectors[ numRiverSegments - 1 ].points[3]);
        outline.push_back(m_sectors[ numRiverSegments - 1 ].points[2]);

        for (size_t i(0); i < numRiverSegments; ++i)
        {
            outline.push_back(m_sectors[ numRiverSegments - 1 - i ].points[0]);
        }

        pRenderNode->SetRiverPhysicsArea(&outline[0], outline.size(), true);
        //////////////////////////////////////////////////////////////////////////

        pRenderNode->Physicalize();
    }
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::OnRiverPhysicsParamChange(IVariable* var)
{
    Physicalize();
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::SetMaterial(CMaterial* mtl)
{
    CBaseObject::SetMaterial(mtl);

    if (mtl)
    {
        const SShaderItem& si = mtl->GetMatInfo()->GetShaderItem();
        if (si.m_pShader && si.m_pShader->GetShaderType() != eST_Water)
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR, "Incorrect shader set for water / water fog volume \"%s\"!", GetName().toUtf8().constData());
        }
    }

    UpdateSectors();
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::UpdateSectors()
{
    IWaterVolumeRenderNode* pOriginatorProxy(0);
    if (!m_sectors.empty())
    {
        const Matrix34& wtm(GetWorldTM());
        m_fogPlane.SetPlane(wtm.GetColumn2().GetNormalized(), m_sectors[0].points[0]);
    }

    if (m_waterVolumeID == -1)
    {
        m_waterVolumeID = ToEntityGuid(GetId());
        assert(m_waterVolumeID);
    }

    //////////////////////////////////////////////////////////////////////////
    // base class impl no longer compatible, as a fix we put in previous
    // implementation of CRoadObject::UpdateSectors(...)

    //CRoadObject::UpdateSectors();

    for (int i = 0; i < m_sectors.size(); i++)
    {
        CRoadSector& sector = m_sectors[i];

        if (sector.points[0] != sector.points[1] &&
            sector.points[0] != sector.points[2] &&
            sector.points[1] != sector.points[2] &&
            sector.points[2] != sector.points[3] &&
            sector.points[1] != sector.points[3])
        {
            UpdateSector(sector);
        }
    }

    Physicalize();
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::UpdateSector(CRoadSector& sector)
{
    //if( !m_sectors.empty() && &sector != &m_sectors[0] )
    //  return;

    if (!sector.m_pRoadSector)
    {
        sector.m_pRoadSector = GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_WaterVolume);
    }

    bool isHidden = CheckFlags(OBJFLAG_INVISIBLE);

    IWaterVolumeRenderNode* pWaterVolumeRN(static_cast<IWaterVolumeRenderNode*>(sector.m_pRoadSector));
    if (pWaterVolumeRN)
    {
        int renderFlags = 0;
        if (isHidden || IsHiddenBySpec())
        {
            renderFlags = ERF_HIDABLE | ERF_HIDDEN;
        }
        pWaterVolumeRN->SetRndFlags(renderFlags);
        pWaterVolumeRN->SetViewDistanceMultiplier(mv_viewDistanceMultiplier);

        pWaterVolumeRN->SetFogDensity(mv_waterFogDensity);
        pWaterVolumeRN->SetFogColor(GetPremulFogColor());
        pWaterVolumeRN->SetFogColorAffectedBySun(mv_waterFogColorAffectedBySun);
        pWaterVolumeRN->SetFogShadowing(mv_waterFogShadowing);
        pWaterVolumeRN->SetCapFogAtVolumeDepth(mv_waterCapFogAtVolumeDepth);
        pWaterVolumeRN->SetCaustics(mv_waterCaustics);
        pWaterVolumeRN->SetCausticIntensity(mv_waterCausticIntensity);
        pWaterVolumeRN->SetCausticTiling(mv_waterCausticTiling);
        pWaterVolumeRN->SetCausticHeight(mv_waterCausticHeight);

        pWaterVolumeRN->CreateRiver(m_waterVolumeID, &sector.points[0], sector.points.size(),
            sector.t0, sector.t1, Vec2(mv_waterSurfaceUScale, mv_waterSurfaceVScale), m_fogPlane, true);

        CMaterial* pMat(GetMaterial());
        if (pMat)
        {
            pMat->AssignToEntity(pWaterVolumeRN);
        }
        else
        {
            sector.m_pRoadSector->SetMaterial(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRiverObject::Serialize(CObjectArchive& ar)
{
    XmlNodeRef xmlNode(ar.node);
    if (ar.bLoading)
    {
        float waterVolumeDepth(10.0f);
        xmlNode->getAttr("VolumeDepth", waterVolumeDepth);
        mv_waterVolumeDepth = waterVolumeDepth;

        float waterStreamSpeed(0.0f);
        xmlNode->getAttr("StreamSpeed", waterStreamSpeed);
        mv_waterStreamSpeed = waterStreamSpeed;

        float waterFogDensity(0.1f);
        xmlNode->getAttr("FogDensity", waterFogDensity);
        mv_waterFogDensity = waterFogDensity;

        Vec3 waterFogColor(0.2f, 0.5f, 0.7f);
        xmlNode->getAttr("FogColor", waterFogColor);
        mv_waterFogColor = waterFogColor;

        float waterFogColorMultiplier(1.0f);
        xmlNode->getAttr("FogColorMultiplier", waterFogColorMultiplier);
        mv_waterFogColorMultiplier = waterFogColorMultiplier;

        bool waterFogColorAffectedBySun(true);
        xmlNode->getAttr("FogColorAffectedBySun", waterFogColorAffectedBySun);
        mv_waterFogColorAffectedBySun = waterFogColorAffectedBySun;

        float waterFogShadowing(0.5f);
        xmlNode->getAttr("FogShadowing", waterFogShadowing);
        mv_waterFogShadowing = waterFogShadowing;

        float waterSurfaceUScale(1.0f);
        xmlNode->getAttr("UScale", waterSurfaceUScale);
        mv_waterSurfaceUScale = waterSurfaceUScale;

        float waterSurfaceVScale(1.0f);
        xmlNode->getAttr("VScale", waterSurfaceVScale);
        mv_waterSurfaceVScale = waterSurfaceVScale;

        bool waterCapFogAtVolumeDepth(false);
        xmlNode->getAttr("CapFogAtVolumeDepth", waterCapFogAtVolumeDepth);
        mv_waterCapFogAtVolumeDepth = waterCapFogAtVolumeDepth;

        bool waterCaustics(true);
        xmlNode->getAttr("Caustics", waterCaustics);
        mv_waterCaustics = waterCaustics;

        float waterCausticIntensity(1.0f);
        xmlNode->getAttr("CausticIntensity", waterCausticIntensity);
        mv_waterCausticIntensity = waterCausticIntensity;

        float waterCausticTiling(1.0f);
        xmlNode->getAttr("CausticTiling", waterCausticTiling);
        mv_waterCausticIntensity = waterCausticTiling;

        float waterCausticHeight(0.5f);
        xmlNode->getAttr("CausticHeight", waterCausticHeight);
        mv_waterCausticHeight = waterCausticHeight;
    }
    else
    {
        xmlNode->setAttr("VolumeDepth", mv_waterVolumeDepth);
        xmlNode->setAttr("StreamSpeed", mv_waterStreamSpeed);
        xmlNode->setAttr("FogDensity", mv_waterFogDensity);
        xmlNode->setAttr("FogColor", mv_waterFogColor);
        xmlNode->setAttr("FogColorMultiplier", mv_waterFogColorMultiplier);
        xmlNode->setAttr("FogColorAffectedBySun", mv_waterFogColorAffectedBySun);
        xmlNode->setAttr("FogShadowing", mv_waterFogShadowing);
        xmlNode->setAttr("UScale", mv_waterSurfaceUScale);
        xmlNode->setAttr("VScale", mv_waterSurfaceVScale);
        xmlNode->setAttr("CapFogAtVolumeDepth", mv_waterCapFogAtVolumeDepth);
        xmlNode->setAttr("Caustics", mv_waterCaustics);
        xmlNode->setAttr("CausticIntensity", mv_waterCausticIntensity);
        xmlNode->setAttr("CausticTiling", mv_waterCausticTiling);
        xmlNode->setAttr("CausticHeight", mv_waterCausticHeight);
    }

    CRoadObject::Serialize(ar);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CRiverObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    return 0;
}




/*!
 * Class Description of RiverObject.
 */
class CRiverObjectClassDesc
    : public CObjectClassDesc
{
public:
    REFGUID ClassID()
    {
        // {55EEF85E-56C6-44e2-AF0B-33D9E746D1F9}
        static const GUID guid = {
            0x55eef85e, 0x56c6, 0x44e2, { 0xaf, 0xb, 0x33, 0xd9, 0xe7, 0x46, 0xd1, 0xf9 }
        };
        return guid;
    }
    ObjectType GetObjectType() { return OBJTYPE_ROAD; };
    QString ClassName() { return "River"; };
    QString Category() { return "Misc"; };
    QObject* CreateQObject() const override { return new CRiverObject; }
    int GameCreationOrder() { return 50; };
};


REGISTER_CLASS_DESC(CRiverObjectClassDesc);

#include <Objects/RiverObject.moc>