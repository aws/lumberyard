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

// Description : Defines the entry point for the DLL application.


#include "StdAfx.h"
//#include <float.h>
#include "IPhysics.h"
#include "geoman.h"
#include "bvtree.h"
#include "geometry.h"
#include "rigidbody.h"
#include "physicalplaceholder.h"
#include "physicalentity.h"
#include "physicalworld.h"
#include "trimesh.h"
#include "utils.h"

// Included only once per DLL module.
#include <platform_impl.h>

#ifndef STANDALONE_PHYSICS
#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>
#endif

#undef GetClassName

float g_sintab[SINCOSTABSZ + 1];
int g_szParams[ePE_Params_Count], g_szAction[ePE_Action_Count], g_szGeomParams[ePE_GeomParams_Count];
subref g_subrefBuf[19];
subref* g_subrefParams[ePE_Params_Count], * g_subrefAction[ePE_Action_Count], * g_subrefGeomParams[ePE_GeomParams_Count];

void TestbedPlaceholder() {}

/*
#ifdef _WIN32
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}
#endif
*/

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Physics
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_RANDOM_SEED:
            cry_random_seed(gEnv->bNoRandomSeed ? 0 : wparam);
            break;
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;
            break;
        }
        case ESYSTEM_EVENT_LEVEL_LOAD_END:
            CTriMesh::CleanupGlobalLoadState();
            g_pPhysWorlds[0]->SortThunks();
            break;
        case ESYSTEM_EVENT_3D_POST_RENDERING_END:
        {
            CTriMesh::CleanupGlobalLoadState();
            break;
        }
        }
    }
};
static CSystemEventListner_Physics g_system_event_listener_physics;

class InitPhysicsGlobals
{
public:
    InitPhysicsGlobals()
    {
        for (int i = 0; i <= SINCOSTABSZ; i++)
        {
            //g_costab[i] = cosf(i*(gf_PI*0.5f/SINCOSTABSZ));
            g_sintab[i] = sinf(i * (gf_PI * 0.5f / SINCOSTABSZ));
        }
        g_szParams[pe_params_pos::type_id] = sizeof(pe_params_pos);
        g_szParams[pe_params_bbox::type_id] = sizeof(pe_params_bbox);
        g_szParams[pe_params_outer_entity::type_id] = sizeof(pe_params_outer_entity);
        g_szParams[pe_params_part::type_id] = sizeof(pe_params_part);
        g_szParams[pe_params_sensors::type_id] = sizeof(pe_params_sensors);
        g_szParams[pe_simulation_params::type_id] = sizeof(pe_simulation_params);
        g_szParams[pe_params_foreign_data::type_id] = sizeof(pe_params_foreign_data);
        g_szParams[pe_params_buoyancy::type_id] = sizeof(pe_params_buoyancy);
        g_szParams[pe_params_flags::type_id] = sizeof(pe_params_flags);
        g_szParams[pe_params_joint::type_id] = sizeof(pe_params_joint);
        g_szParams[pe_params_articulated_body::type_id] = sizeof(pe_params_articulated_body);
        g_szParams[pe_player_dimensions::type_id] = sizeof(pe_player_dimensions);
        g_szParams[pe_player_dynamics::type_id] = sizeof(pe_player_dynamics);
        g_szParams[pe_params_particle::type_id] = sizeof(pe_params_particle);
        g_szParams[pe_params_car::type_id] = sizeof(pe_params_car);
        g_szParams[pe_params_wheel::type_id] = sizeof(pe_params_wheel);
        g_szParams[pe_params_rope::type_id] = sizeof(pe_params_rope);
        g_szParams[pe_params_softbody::type_id] = sizeof(pe_params_softbody);
        g_szParams[pe_tetrlattice_params::type_id] = sizeof(pe_tetrlattice_params);
        g_szParams[pe_params_area::type_id] = sizeof(pe_params_area);
        g_szParams[pe_params_ground_plane::type_id] = sizeof(pe_params_ground_plane);
        g_szParams[pe_params_structural_joint::type_id] = sizeof(pe_params_structural_joint);
        g_szParams[pe_params_structural_initial_velocity::type_id] = sizeof(pe_params_structural_initial_velocity);
        g_szParams[pe_params_waterman::type_id] = sizeof(pe_params_waterman);
        g_szParams[pe_params_timeout::type_id] = sizeof(pe_params_timeout);
        g_szParams[pe_params_skeleton::type_id] = sizeof(pe_params_skeleton);
        g_szParams[pe_params_collision_class::type_id] = sizeof(pe_params_collision_class);

        g_szAction[pe_action_impulse::type_id] = sizeof(pe_action_impulse);
        g_szAction[pe_action_reset::type_id] = sizeof(pe_action_reset);
        g_szAction[pe_action_add_constraint::type_id] = sizeof(pe_action_add_constraint);
        g_szAction[pe_action_update_constraint::type_id] = sizeof(pe_action_update_constraint);
        g_szAction[pe_action_register_coll_event::type_id] = sizeof(pe_action_register_coll_event);
        g_szAction[pe_action_awake::type_id] = sizeof(pe_action_awake);
        g_szAction[pe_action_remove_all_parts::type_id] = sizeof(pe_action_remove_all_parts);
        g_szAction[pe_action_set_velocity::type_id] = sizeof(pe_action_set_velocity);
        g_szAction[pe_action_move::type_id] = sizeof(pe_action_move);
        g_szAction[pe_action_drive::type_id] = sizeof(pe_action_drive);
        g_szAction[pe_action_attach_points::type_id] = sizeof(pe_action_attach_points);
        g_szAction[pe_action_target_vtx::type_id] = sizeof(pe_action_target_vtx);
        g_szAction[pe_action_reset_part_mtx::type_id] = sizeof(pe_action_reset_part_mtx);
        g_szAction[pe_action_auto_part_detachment::type_id] = sizeof(pe_action_auto_part_detachment);
        g_szAction[pe_action_move_parts::type_id] = sizeof(pe_action_move_parts);
        g_szAction[pe_action_batch_parts_update::type_id] = sizeof(pe_action_batch_parts_update);
        g_szAction[pe_action_slice::type_id] = sizeof(pe_action_slice);

        g_szGeomParams[pe_geomparams::type_id] = sizeof(pe_geomparams);
        g_szGeomParams[pe_articgeomparams::type_id] = sizeof(pe_articgeomparams);
        g_szGeomParams[pe_cargeomparams::type_id] = sizeof(pe_cargeomparams);

        memset(g_subrefParams, 0, sizeof(g_subrefParams));
        memset(g_subrefAction, 0, sizeof(g_subrefAction));
        memset(g_subrefGeomParams, 0, sizeof(g_subrefGeomParams));

        pe_params_pos pp;
        g_subrefParams[pe_params_pos::type_id] = g_subrefBuf + 0;
        g_subrefBuf[0].set((int)((INT_PTR)&pp.pMtx3x4 - (INT_PTR)&pp), 1, 0, sizeof(Matrix34), g_subrefBuf + 1);
        g_subrefBuf[1].set((int)((INT_PTR)&pp.pMtx3x3 - (INT_PTR)&pp), 1, 0, sizeof(Matrix33), 0);

        pe_params_part ppt;
        g_subrefParams[pe_params_part::type_id] = g_subrefBuf + 2;
        g_subrefBuf[2].set((int)((INT_PTR)&ppt.pMtx3x4 - (INT_PTR)&ppt), 1, 0, sizeof(Matrix34), g_subrefBuf + 3);
        g_subrefBuf[3].set((int)((INT_PTR)&ppt.pMtx3x3 - (INT_PTR)&ppt), 1, 0, sizeof(Matrix33), g_subrefBuf + 4);
        g_subrefBuf[4].set((int)((INT_PTR)&ppt.pMatMapping - (INT_PTR)&ppt), 0, (int)((INT_PTR)&ppt.nMats - (INT_PTR)&ppt), sizeof(int), 0);

        pe_params_sensors ps;
        g_subrefParams[pe_params_sensors::type_id] = g_subrefBuf + 4;
        g_subrefBuf[5].set((int)((INT_PTR)&ps.pOrigins - (INT_PTR)&ps), 0, (int)((INT_PTR)&ps.nSensors - (INT_PTR)&ps), sizeof(Vec3), g_subrefBuf + 6);
        g_subrefBuf[6].set((int)((INT_PTR)&ps.pDirections - (INT_PTR)&ps), 0, (int)((INT_PTR)&ps.nSensors - (INT_PTR)&ps), sizeof(Vec3), 0);

        pe_params_joint pj;
        g_subrefParams[pe_params_joint::type_id] = g_subrefBuf + 7;
        g_subrefBuf[7].set((int)((INT_PTR)&pj.pMtx0 - (INT_PTR)&pj), 1, 0, sizeof(Matrix33), g_subrefBuf + 8);
        g_subrefBuf[8].set((int)((INT_PTR)&pj.pSelfCollidingParts - (INT_PTR)&pj),
            0, (int)((INT_PTR)&pj.nSelfCollidingParts - (INT_PTR)&pj), sizeof(int), 0);

        pe_params_car pc;
        g_subrefParams[pe_params_car::type_id] = g_subrefBuf + 9;
        g_subrefBuf[9].set((int)((INT_PTR)&pc.gearRatios - (INT_PTR)&pc), 0, (int)((INT_PTR)&pc.nGears - (INT_PTR)&pc), sizeof(float), 0);

        pe_params_rope pr;
        g_subrefParams[pe_params_rope::type_id] = g_subrefBuf + 10;
        g_subrefBuf[10].set((int)((INT_PTR)&pr.pPoints - (INT_PTR)&pr), 1, (int)((INT_PTR)&pr.nSegments - (INT_PTR)&pr), sizeof(Vec3), g_subrefBuf + 11);
        g_subrefBuf[11].set((int)((INT_PTR)&pr.pVelocities - (INT_PTR)&pr), 1, (int)((INT_PTR)&pr.nSegments - (INT_PTR)&pr), sizeof(Vec3), 0);

        pe_action_attach_points aap;
        g_subrefAction[pe_action_attach_points::type_id] = g_subrefBuf + 12;
        g_subrefBuf[12].set((int)((INT_PTR)&aap.piVtx - (INT_PTR)&aap), 0, (int)((INT_PTR)&aap.nPoints - (INT_PTR)&aap), sizeof(int), g_subrefBuf + 13);
        g_subrefBuf[13].set((int)((INT_PTR)&aap.points - (INT_PTR)&aap), 0, (int)((INT_PTR)&aap.nPoints - (INT_PTR)&aap), sizeof(Vec3), 0);

        pe_action_target_vtx atv;
        g_subrefAction[pe_action_target_vtx::type_id] = g_subrefBuf + 14;
        g_subrefBuf[14].set((int)((INT_PTR)&atv.points - (INT_PTR)&atv), 0, (int)((INT_PTR)&atv.nPoints - (INT_PTR)&atv), sizeof(Vec3), 0);

        pe_geomparams gp;
        g_subrefGeomParams[pe_geomparams::type_id] = g_subrefGeomParams[pe_articgeomparams::type_id] =
                g_subrefGeomParams[pe_cargeomparams::type_id] = g_subrefBuf + 15;
        g_subrefBuf[15].set((int)((INT_PTR)&gp.pMtx3x4 - (INT_PTR)&gp), 1, 0, sizeof(Matrix34), g_subrefBuf + 16);
        g_subrefBuf[16].set((int)((INT_PTR)&gp.pMtx3x3 - (INT_PTR)&gp), 1, 0, sizeof(Matrix33), g_subrefBuf + 17);
        g_subrefBuf[17].set((int)((INT_PTR)&gp.pMatMapping - (INT_PTR)&gp), 0, (int)((INT_PTR)&gp.nMats - (INT_PTR)&gp), sizeof(int), 0);

        pe_action_slice as;
        g_subrefAction[pe_action_slice::type_id] = g_subrefBuf + 18;
        g_subrefBuf[18].set((int)((INT_PTR)&as.pt - (INT_PTR)&as), 0, (int)((INT_PTR)&as.npt - (INT_PTR)&as), sizeof(Vec3), 0);
    }
};

InitPhysicsGlobals Now;


CRYPHYSICS_API IPhysicalWorld* CreatePhysicalWorld(ISystem* pSystem)
{
    ModuleInitISystem(pSystem, "CryPhysics");
    g_bHasSSE = pSystem && (pSystem->GetCPUFlags() & CPUF_SSE) != 0;

    if (pSystem)
    {
        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_physics);
        return new CPhysicalWorld(pSystem->GetILog());
    }

    return new CPhysicalWorld(0);
}

#ifndef STANDALONE_PHYSICS
//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryPhysics
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryPhysics, "EngineModule_CryPhysics", 0x526cabf3d776407f, 0xaa2338545bb6ae7f)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryPhysics";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        g_bHasSSE = pSystem && (pSystem->GetCPUFlags() & CPUF_SSE) != 0;

        if (pSystem)
        {
            pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_physics);
        }

        env.pPhysicalWorld = new CPhysicalWorld(pSystem ? pSystem->GetILog() : 0);

        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryPhysics)

CEngineModule_CryPhysics::CEngineModule_CryPhysics()
{
};

CEngineModule_CryPhysics::~CEngineModule_CryPhysics()
{
};


// TypeInfo implementations for CryPhysics
#ifndef AZ_MONOLITHIC_BUILD
    #include "Common_TypeInfo.h"
#endif

#include "TypeInfo_impl.h"

TYPE_INFO_PLAIN(primitives::getHeightCallback)
TYPE_INFO_PLAIN(primitives::getSurfTypeCallback)

#include "primitives_info.h"
#include "aabbtree_info.h"
#include "obbtree_info.h"
#include "geoman_info.h"
#endif
