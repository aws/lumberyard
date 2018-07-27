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
#include "ComponentSubstitution.h"
#include "Entity.h"
#include "ISerialize.h"
#include "EntitySystem.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentSubstitution, IComponentSubstitution)

void CComponentSubstitution::Done()
{
    // Substitution component does not need to be restored if entity system is being rested.
    if (m_pSubstitute && !g_pIEntitySystem->m_bReseting)
    {
        //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::Done: Ptr=%d", (int)m_pSubstitute);
        //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::Done: %s", m_pSubstitute->GetName());
        //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::Done: Pos=(%.2f,%.2f,%.2f)", m_pSubstitute->GetPos().x, m_pSubstitute->GetPos().y, m_pSubstitute->GetPos().z);
        gEnv->p3DEngine->RegisterEntity(m_pSubstitute);
        m_pSubstitute->Physicalize(true);
        AABB WSBBox = m_pSubstitute->GetBBox();
        static ICVar* e_on_demand_physics(gEnv->pConsole->GetCVar("e_OnDemandPhysics")),
        *e_on_demand_maxsize(gEnv->pConsole->GetCVar("e_OnDemandMaxSize"));
        if (m_pSubstitute->GetPhysics() && e_on_demand_physics && e_on_demand_physics->GetIVal() &&
            e_on_demand_maxsize && max(WSBBox.max.x - WSBBox.min.x, WSBBox.max.y - WSBBox.min.y) <= e_on_demand_maxsize->GetFVal())
        {
            gEnv->pPhysicalWorld->AddRefEntInPODGrid(m_pSubstitute->GetPhysics(), &WSBBox.min);
        }
        m_pSubstitute = 0;
    }
}

void CComponentSubstitution::SetSubstitute(IRenderNode* pSubstitute)
{
    m_pSubstitute = pSubstitute;
    //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::SetSubstitute: Ptr=%d", (int)m_pSubstitute);
    //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::SetSubstitute: %s", m_pSubstitute->GetName());
    //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::SetSubstitute: Pos=(%.2f,%.2f,%.2f)", m_pSubstitute->GetPos().x, m_pSubstitute->GetPos().y, m_pSubstitute->GetPos().z);
}

//////////////////////////////////////////////////////////////////////////
void CComponentSubstitution::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    m_pSubstitute = 0;
}

//////////////////////////////////////////////////////////////////////////
bool CComponentSubstitution::NeedSerialize()
{
    return m_pSubstitute != 0;
};

//////////////////////////////////////////////////////////////////////////
bool CComponentSubstitution::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentSubstitution");
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentSubstitution::Serialize(TSerialize ser)
{
    Vec3 center, pos;
    if (ser.IsReading())
    {
        if (!m_pSubstitute)
        {
            ser.Value("SubstCenter", center);
            ser.Value("SubstPos", pos);
            IPhysicalEntity** pents;
            m_pSubstitute = 0;
            int i = gEnv->pPhysicalWorld->GetEntitiesInBox(center - Vec3(0.05f), center + Vec3(0.05f), pents, ent_static);
            for (--i; i >= 0 && !((m_pSubstitute = (IRenderNode*)pents[i]->GetForeignData(PHYS_FOREIGN_ID_STATIC)) &&
                                  (m_pSubstitute->GetPos() - pos).len2() < sqr(0.03f) &&
                                  (m_pSubstitute->GetBBox().GetCenter() - center).len2() < sqr(0.03f)); i--)
            {
                ;
            }
            if (i < 0)
            {
                m_pSubstitute = 0;
            }
            else if (m_pSubstitute)
            {
                //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::Serialize: Ptr=%d", (int)m_pSubstitute);
                //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::Serialize: %s", m_pSubstitute->GetName());
                //gEnv->pLog->Log("CRYSIS-3502: CComponentSubstitution::Serialize: Pos=(%.2f,%.2f,%.2f)", m_pSubstitute->GetPos().x, m_pSubstitute->GetPos().y, m_pSubstitute->GetPos().z);

                m_pSubstitute->Dephysicalize();
                gEnv->p3DEngine->UnRegisterEntityAsJob(m_pSubstitute);
            }
        }
    }
    else
    {
        if (m_pSubstitute)
        {
            ser.Value("SubstCenter", center = m_pSubstitute->GetBBox().GetCenter());
            ser.Value("SubstPos", pos = m_pSubstitute->GetPos());
        }
    }
}

