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

// Description : PhysicalEntity class declarations


#ifndef CRYINCLUDE_CRYPHYSICS_PHYSICALENTITY_H
#define CRYINCLUDE_CRYPHYSICS_PHYSICALENTITY_H
#pragma once

#include "GeomQuery.h"

struct SRayTraceRes;

#include "ISerialize.h"

enum phentity_flags_int
{
    pef_use_geom_callbacks = 0x20000000,
    sef_skeleton = 0x04
};

struct coord_block
{
    Vec3 pos;
    quaternionf q;
};

struct coord_block_BBox
{
    Vec3 pos;
    quaternionf q;
    float scale;
    Vec3 BBox[2];
};

enum cbbx_flags
{
    part_added = 1, update_part_bboxes = 2
};                                                      // for ComputeBBox

enum geom_flags_aux
{
    geom_car_wheel = 0x40000000, geom_invalid = 0x20000000, geom_removed = 0x10000000, geom_will_be_destroyed = 0x8000000,
    geom_constraint_on_break = geom_proxy
};

class CTetrLattice;

struct geom
{
    Vec3 pos;
    quaternionf q;
    float scale;
    Vec3 BBox[2];
    phys_geometry* pPhysGeom, * pPhysGeomProxy;
    int id;
    float mass;
    unsigned int flags, flagsCollider;
    float maxdim;
    float minContactDist;
    int surface_idx : 10;
    int idmatBreakable : 14;
    unsigned int nMats : 7;
    int bChunkStart : 1;
    int* pMatMapping;
    union
    {
        CTetrLattice* pLattice;
        geom* next;
    };
    union
    {
        coord_block_BBox* pNewCoords;
        geom* prev;
    };
    CPhysicalPlaceholder* pPlaceholder;
};

struct SStructuralJoint
{
    int id;
    int ipart[2];
    Vec3 pt;
    Vec3 n, axisx;
    float maxForcePush, maxForcePull, maxForceShift;
    float maxTorqueBend, maxTorqueTwist;
    float damageAccum, damageAccumThresh;
    Vec3 limitConstr;
    float dampingConstr;
    int bBreakable, bBroken;
    float size;
    float tension;
    int itens;
    Vec3 Paccum, Laccum;
};

struct SSkinInfo
{
    int itri;
    float w[4];
};

struct SSkelInfo
{
    SSkinInfo* pSkinInfo;
    CPhysicalEntity* pSkelEnt;
    int idSkel;
    float timeStep;
    int nSteps;
    float maxImpulse;
    float lastUpdateTime;
    Vec3 lastUpdatePos;
    Quat lastUpdateq;
    float lastCollTime;
    float lastCollImpulse;
};

struct SPartInfo
{
    Vec3 Pext, Lext;
    Vec3 Fext, Text;
    Vec3 initialVel; // override for clients
    Vec3 initialAngVel;
    int iParent;
    int flags0, flagsCollider0;
};

struct SStructureInfo
{
    SStructuralJoint* pJoints;
    SPartInfo* pParts;
    int nJoints, nJointsAlloc;
    int nLastBrokenJoints;
    int bModified;
    int idpartBreakOrg;
    int nPartsAlloc;
    float timeLastUpdate;
    int nPrevJoints;
    int nLastUsedJoints;
    float prevdt;
    float minSnapshotTime;
    float autoDetachmentDist;
    Vec3 lastExplPos;
    Vec3* Pexpl, * Lexpl;
    SSkelInfo* defparts;
    int bTestRun;
    int bHasDirectBreaks;
};

struct SPartHelper
{
    int idx;
    float Minv;
    Matrix33 Iinv;
    Vec3 v, w;
    Vec3 org;
    int bProcessed;
    int ijoint0;
    int isle;
    CPhysicalEntity* pent;
};

struct SStructuralJointHelper
{
    int idx;
    Vec3 r0, r1;
    Matrix33 vKinv, wKinv;
    Vec3 P, L;
    int bBroken;
};
struct SStructuralJointDebugHelper
{
    int itens;
    quotientf tension;
};

struct SExplosionInfo
{
    Vec3 center;
    Vec3 dir;
    float rmin, kr;
};

class CPhysicalWorld;
class CRayGeom;
class CGeometry;

class CPhysicalEntity
    : public CPhysicalPlaceholder
{
public:
    static void CleanupGlobalState();

public:
    void* operator new (size_t sz, void* p) { return p; }
    void operator delete (void* p, void* q) {}

    // Not supported, use Create/Delete instead
    void* operator new (size_t sz) throw() { return NULL; }
    void operator delete (void* p) { __debugbreak(); }

    template <typename T>
    static T* Create(CPhysicalWorld* pWorld, IGeneralMemoryHeap* pHeap)
    {
        void* p = NULL;
        if (pHeap)
        {
            p = pHeap->Malloc(sizeof(T), "Physical Entity");
        }
        else
        {
            p = CryModuleMalloc(sizeof(T));
        }
        return p
               ? new (p) T(pWorld, pHeap)
               : NULL;
    }

public:
    explicit CPhysicalEntity(CPhysicalWorld* pworld, IGeneralMemoryHeap* pHeap = NULL);
    virtual ~CPhysicalEntity();
    virtual pe_type GetType() const { return PE_STATIC; }

    virtual void Delete();

    virtual int AddRef();
    virtual int Release();

    virtual int SetParams(const pe_params*, int bThreadSafe = 1);
    virtual int GetParams(pe_params*) const;
    virtual int GetStatus(pe_status*) const;
    virtual int Action(const pe_action*, int bThreadSafe = 1);
    virtual int AddGeometry(phys_geometry* pgeom, pe_geomparams* params, int id = -1, int bThreadSafe = 1);
    virtual void RemoveGeometry(int id, int bThreadSafe = 1);
    virtual float GetExtent(EGeomForm eForm) const;
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm) const;
    virtual IPhysicalWorld* GetWorld() const { return (IPhysicalWorld*)m_pWorld; }
    virtual CPhysicalEntity* GetEntity();
    virtual CPhysicalEntity* GetEntityFast() { return this; }

    virtual void StartStep(float time_interval) {}
    virtual float GetMaxTimeStep(float time_interval) { return time_interval; }
    virtual float GetLastTimeStep(float time_interval) { return time_interval; }
    virtual int Step(float time_interval) { return 1; }
    virtual int DoStep(float time_interval, int iCaller = 0) { return 1; }
    virtual void StepBack(float time_interval) {}
    virtual int GetContactCount(int nMaxPlaneContacts) { return 0; }
    virtual int RegisterContacts(float time_interval, int nMaxPlaneContacts) { return 0; }
    virtual int Update(float time_interval, float damping) { return 1; }
    virtual float CalcEnergy(float time_interval) { return 0; }
    virtual float GetDamping(float time_interval) { return 1.0f; }
    virtual float GetMaxFriction() { return 100.0f; }
    virtual bool IgnoreCollisionsWith(const CPhysicalEntity* pent, int bCheckConstraints = 0) const { return false; }
    virtual void GetSleepSpeedChange(int ipart, Vec3& v, Vec3& w) { v.zero(); w.zero(); }

    virtual int AddCollider(CPhysicalEntity* pCollider);
    virtual int AddColliderNoLock(CPhysicalEntity* pCollider) { return AddCollider(pCollider); }
    virtual int RemoveCollider(CPhysicalEntity* pCollider, bool bAlwaysRemove = true);
    virtual int RemoveColliderNoLock(CPhysicalEntity* pCollider, bool bAlwaysRemove = true) { return RemoveCollider(pCollider, bAlwaysRemove); }
    int RemoveColliderMono(CPhysicalEntity* pCollider, bool bAlwaysRemove = true) { WriteLock lock(m_lockColliders); return RemoveColliderNoLock(pCollider, bAlwaysRemove); }
    virtual int RemoveContactPoint(CPhysicalEntity* pCollider, const Vec3& pt, float mindist2) { return -1; }
    virtual int HasContactsWith(CPhysicalEntity* pent) { return 0; }
    virtual int HasPartContactsWith(CPhysicalEntity* pent, int ipart, int bGreaterOrEqual = 0) { return 1; }
    virtual int HasCollisionContactsWith(CPhysicalEntity* pent) { return 0; }
    virtual int HasConstraintContactsWith(const CPhysicalEntity* pent, int flagsIgnore = 0) const { return 0; }
    virtual void AlertNeighbourhoodND(int mode);
    virtual int Awake(int bAwake = 1, int iSource = 0) { return 0; }
    virtual int IsAwake(int ipart = -1) const { return 0; }
    int GetColliders(CPhysicalEntity**& pentlist) { pentlist = m_pColliders; return m_nColliders; }
    virtual int RayTrace(SRayTraceRes&);
    virtual void ApplyVolumetricPressure(const Vec3& epicenter, float kr, float rmin) {}
    virtual void OnContactResolved(struct entity_contact* pContact, int iop, int iGroupId);
    virtual void DelayedIntersect(geom_contact* pcontacts, int ncontacts, CPhysicalEntity** pColliders, int (*iCollParts)[2]) {}

    virtual void OnNeighbourSplit(CPhysicalEntity* pentOrig, CPhysicalEntity* pentNew) {}
    virtual void OnStructureUpdate() {}

    virtual class RigidBody* GetRigidBody(int ipart = -1, int bWillModify = 0);
    virtual class RigidBody* GetRigidBodyData(RigidBody* pbody, int ipart = -1) { return GetRigidBody(ipart); }
    virtual float GetMass(int ipart) { return m_parts[ipart].mass; }
    virtual void GetContactMatrix(const Vec3& pt, int ipart, Matrix33& K) {}
    virtual void GetSpatialContactMatrix(const Vec3& pt, int ipart, float Ibuf[][6]) {}
    virtual float GetMassInv() { return 0; }
    virtual int IsPointInside(Vec3 pt) const;
    virtual void GetLocTransform(int ipart, Vec3& offs, quaternionf& q, float& scale)
    {
        if ((unsigned int)ipart < (unsigned int)m_nParts)
        {
            q = m_qrot * m_parts[ipart].q;
            offs = m_qrot * m_parts[ipart].pos + m_pos;
            scale = m_parts[ipart].scale;
        }
        else
        {
            q.SetIdentity();
            offs.zero();
            scale = 1.0f;
        }
    }
    virtual void GetLocTransformLerped(int ipart, Vec3& offs, quaternionf& q, float& scale, float timeBack) {   GetLocTransform(ipart, offs, q, scale); }
    virtual void DetachPartContacts(int ipart, int iop0, CPhysicalEntity* pent, int iop1, int bCheckIfEmpty = 1) {}
    int TouchesSphere(const Vec3& center, float r);

    virtual void DrawHelperInformation(IPhysRenderer* pRenderer, int flags);
    virtual void GetMemoryStatistics(ICrySizer* pSizer) const;

    virtual int GetStateSnapshot(class CStream& stm, float time_back = 0, int flags = 0) { return 0; }
    virtual int GetStateSnapshot(TSerialize ser, float time_back = 0, int flags = 0);
    virtual int SetStateFromSnapshot(class CStream& stm, int flags = 0) { return 0; }
    virtual int SetStateFromSnapshot(TSerialize ser, int flags = 0);
    virtual int SetStateFromTypedSnapshot(TSerialize ser, int iSnapshotType, int flags = 0);
    virtual int PostSetStateFromSnapshot() { return 1; }
    virtual unsigned int GetStateChecksum() { return 0; }
    virtual int GetStateSnapshotTxt(char* txtbuf, int szbuf, float time_back = 0);
    virtual void SetStateFromSnapshotTxt(const char* txtbuf, int szbuf);
    virtual void SetNetworkAuthority(int authoritive, int paused) {}

    void AllocStructureInfo();
    int GenerateJoints();
    int UpdateStructure(float time_interval, pe_explosion* pexpl, int iCaller = 0, Vec3 gravity = Vec3(0));
    void RemoveBrokenParent(int i, int nIsles);
    int MapHitPointFromParent(int i, const Vec3& pt);
    virtual void RecomputeMassDistribution(int ipart = -1, int bMassChanged = 1) {}

    virtual void ComputeBBox(Vec3* BBox, int flags = update_part_bboxes);
    void WriteBBox(Vec3* BBox)
    {
        m_BBox[0] = BBox[0];
        m_BBox[1] = BBox[1];
        if (m_pEntBuddy)
        {
            m_pEntBuddy->m_BBox[0] = BBox[0];
            m_pEntBuddy->m_BBox[1] = BBox[1];
        }
    }

    bool OccupiesEntityGridSquare(const AABB& bbox);

    void UpdatePartIdmatBreakable(int ipart, int nParts = -1);
    int CapsulizePart(int ipart);

    int GetMatId(int id, int ipart)
    {
        if (ipart < m_nParts)
        {
            id += m_parts[ipart].surface_idx - id & id >> 31;
            intptr_t mask = iszero_mask(m_parts[ipart].pMatMapping);
            int idummy = 0, * pMatMapping = (int*)((intptr_t)m_parts[ipart].pMatMapping & ~mask | (intptr_t)&idummy & mask), nMats;
            nMats = m_parts[ipart].nMats + (65536 & (int)mask);
            return id & (int)mask | pMatMapping[id & ~(int)mask & (id & ~(int)mask) - nMats >> 31];
        }
        else
        {
            return id;
        }
    }

    bool MakeDeformable(int ipart, int iskel, float r = 0.0f);
    void UpdateDeformablePart(int ipart);

    static volatile int g_lockProcessedAux;
    int GetCheckPart(int ipart)
    {
        int i, j;
        if (m_nParts <= 24)
        {
            i = (m_bProcessed_aux & 0xFFFFFF) & (1 << ipart) - 1;
            return (m_bProcessed_aux >> 24) + bitcount(i) | (m_bProcessed_aux >> ipart & 1) - 1;
        }
        else
        {
            if (!(m_parts[ipart].flags & geom_removed))
            {
                return -1;
            }
            for (i = j = 0; i < ipart; i++)
            {
                j += iszero((int)m_parts[i].flags & geom_removed) ^ 1;
            }
            return (m_bProcessed_aux >> 24) + j;
        }
    }

    void RepositionParts();
    int GetUsedPartsCount(int iCaller) { int n = m_nUsedParts >> iCaller * 4 & 15; return n + (m_nParts - n & (14 - n | n - 1) >> 31); }
    int GetUsedPart(int iCaller, int i)
    {
        int n = m_nUsedParts >> iCaller * 4 & 15;
        return (14 - n | n - 1) < 0 ? i : m_pUsedParts[iCaller][i & 15];
    }
    CPhysicalPlaceholder* ReleasePartPlaceholder(int i);
    volatile int m_nRefCount;
    unsigned int m_flags;
    CPhysicalEntity* m_next, * m_prev;
    CPhysicalWorld* m_pWorld;
    IGeneralMemoryHeap* m_pHeap;
    int m_nRefCountPOD   : 16;
    int m_iLastPODUpdate : 16;

    int m_iPrevSimClass : 16;
    int m_bMoved        : 8;
    int m_bPermanent        : 4;
    int m_bPrevPermanent : 4;
    int m_iGroup;
    CPhysicalEntity* m_next_coll, * m_next_coll1, * m_next_coll2;

    Vec3 m_pos;
    quaternionf m_qrot;
    coord_block* m_pNewCoords;

    CPhysicalEntity** m_pColliders;
    int m_nColliders, m_nCollidersAlloc;
    mutable volatile int m_lockColliders;

    CPhysicalEntity* m_pOuterEntity;
    CGeometry* m_pBoundingGeometry;
    int m_bProcessed_aux;

    SCollisionClass m_collisionClass;

    float m_timeIdle, m_maxTimeIdle;

    float m_timeStructUpdate;
    int m_updStage, m_nUpdTicks;
    SExplosionInfo* m_pExpl;

    geom* m_parts;
    static geom m_defpart;
    int m_nParts, m_nPartsAlloc;
    int m_iLastIdx;
    volatile int m_lockPartIdx;
    mutable CGeomExtents m_Extents;

    plane* m_ground;
    int m_nGroundPlanes;

    int (*m_pUsedParts)[16];
    volatile unsigned int m_nUsedParts;

    SStructureInfo* m_pStructure;
    static SPartHelper* g_parts;
    static SStructuralJointHelper* g_joints;
    static SStructuralJointDebugHelper* g_jointsDbg;
    static int* g_jointidx;
    static int g_nPartsAlloc, g_nJointsAlloc;
};

extern RigidBody g_StaticRigidBodies[];
#define g_StaticRigidBody (g_StaticRigidBodies[0])
extern CPhysicalEntity g_StaticPhysicalEntity;

template<class T>
int GetStructSize(const T* pstruct);
template<class T>
subref* GetSubref(const T* pstruct);
template<class T>
int GetStructId(const T* pstruct);
template<class T>
bool StructUsesAuxData(const T* pstruct);
template<class T>
bool AllowChangesOnDeleted(const T* pstruct);

extern int g_szParams[], g_szAction[], g_szGeomParams[];
extern subref* g_subrefParams[], * g_subrefAction[], * g_subrefGeomParams[];

template<>
inline int GetStructSize(const pe_params* params)
{
    return g_szParams[params->type];
}

template<>
inline int GetStructSize(const pe_action* action)
{
    return g_szAction[action->type];
}

template<>
inline int GetStructSize(const pe_geomparams* geomparams)
{
    return g_szGeomParams[geomparams->type];
}

template<class T>
inline int GetStructSize(const T*)
{
    return 0;
}

template<>
inline subref* GetSubref(const pe_params* params)
{
    return g_subrefParams[params->type];
}

template<>
inline subref* GetSubref(const pe_action* action)
{
    return g_subrefAction[action->type];
}

template<>
inline subref* GetSubref(const pe_geomparams* geomparams)
{
    return g_subrefGeomParams[geomparams->type];
}

template<class T>
inline subref* GetSubref(const T*)
{
    return 0;
}

template<>
inline int GetStructId(const pe_params*)
{
    return 0;
}

template<>
inline int GetStructId(const pe_action*)
{
    return 1;
}

template<>
inline int GetStructId(const pe_geomparams*)
{
    return 2;
}

template<class T>
inline int GetStructId(const T*)
{
    return 3;
}

template<>
inline bool StructUsesAuxData(const pe_params*)
{
    return false;
}

template<>
inline bool StructUsesAuxData(const pe_action*)
{
    return false;
}

template<>
inline bool StructUsesAuxData(const pe_geomparams*)
{
    return true;
}

template<class T>
inline bool StructUsesAuxData(const T*)
{
    return true;
}

template<>
inline bool AllowChangesOnDeleted(const pe_params* params)
{
    return params->type == pe_params_foreign_data::type_id
           || params->type == pe_player_dynamics::type_id
           || params->type == pe_player_dimensions::type_id
           || params->type == pe_params_collision_class::type_id;
}

template<>
inline bool AllowChangesOnDeleted(const pe_geomparams* geomparams)
{
    return geomparams->type == pe_articgeomparams::type_id;
}

template<class T>
inline bool AllowChangesOnDeleted(const T* pstruct)
{
    return false;
}

#endif // CRYINCLUDE_CRYPHYSICS_PHYSICALENTITY_H
