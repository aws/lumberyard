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

// Description : PhysicalPlaceholder class declarations


#ifndef CRYINCLUDE_CRYPHYSICS_PHYSICALPLACEHOLDER_H
#define CRYINCLUDE_CRYPHYSICS_PHYSICALPLACEHOLDER_H
#pragma once

#pragma pack(push)
#pragma pack(1)
// Packed and aligned structure ensures bitfield packs properly across
// 4 byte boundaries but stops the compiler implicitly doing byte copies
_MS_ALIGN(4) struct pe_gridthunk
{
    uint64 inext : 20; // int64 for tighter packing
    uint64 iprev : 20;
    uint64 inextOwned : 20;
    uint64 iSimClass : 3;
    uint64 bFirstInCell : 1;
    unsigned char BBox[4];
    class CPhysicalPlaceholder* pent;
    int BBoxZ0 : 16;
    int BBoxZ1 : 16;
} _ALIGN(4);
#pragma pack(pop)

class CPhysicalEntity;
const int NO_GRID_REG = 0xFFFFFFFF << 14;
const int GRID_REG_PENDING = NO_GRID_REG + 1;

class CPhysicalPlaceholder
    : public IPhysicalEntity
{
public:
    CPhysicalPlaceholder()
    {
        m_lockUpdate = 0;
        m_iGThunk0 = 0;
        m_pEntBuddy = 0;
        m_bProcessed = 0;
        m_ig[0].x = m_ig[1].x = m_ig[0].y = m_ig[1].y = GRID_REG_PENDING;
        m_pForeignData = 0;
        m_iForeignData = m_iForeignFlags = 0;
        m_bOBBThunks = 0;
        m_iDeletionTime = 0;
        m_iSimClass = -1;
    }

    virtual CPhysicalEntity* GetEntity();
    virtual CPhysicalEntity* GetEntityFast() { return (CPhysicalEntity*)m_pEntBuddy; }

    virtual int AddRef() { return 0; }
    virtual int Release() { return 0; }

    virtual pe_type GetType() const;
    virtual int SetParams(const pe_params* params, int bThreadSafe = 1);
    virtual int GetParams(pe_params* params) const;
    virtual int GetStatus(pe_status* status) const;
    virtual int Action(const pe_action* action, int bThreadSafe = 1);

    virtual int AddGeometry(phys_geometry* pgeom, pe_geomparams* params, int id = -1, int bThreadSafe = 1);
    virtual void RemoveGeometry(int id, int bThreadSafe = 1);

    virtual PhysicsForeignData GetForeignData(int itype = 0) const { return m_iForeignData == itype ? m_pForeignData : PhysicsForeignData(0); }
    virtual int GetiForeignData() const { return m_iForeignData; }

    virtual int GetStateSnapshot(class CStream& stm, float time_back = 0, int flags = 0);
    virtual int GetStateSnapshot(TSerialize ser, float time_back = 0, int flags = 0);
    virtual int SetStateFromSnapshot(class CStream& stm, int flags = 0);
    virtual int SetStateFromSnapshot(TSerialize ser, int flags = 0);
    virtual int SetStateFromTypedSnapshot(TSerialize ser, int type, int flags = 0);
    virtual int PostSetStateFromSnapshot();
    virtual int GetStateSnapshotTxt(char* txtbuf, int szbuf, float time_back = 0);
    virtual void SetStateFromSnapshotTxt(const char* txtbuf, int szbuf);
    virtual unsigned int GetStateChecksum();
    virtual void SetNetworkAuthority(int authoritive, int paused);

    virtual void StartStep(float time_interval);
    virtual int Step(float time_interval);
    virtual int DoStep(float time_interval) { return DoStep(time_interval, MAX_PHYS_THREADS); }
    virtual int DoStep(float time_interval, int iCaller) { return 1; }
    virtual void StepBack(float time_interval);
    virtual IPhysicalWorld* GetWorld() const;

    virtual void GetMemoryStatistics(ICrySizer* pSizer) const {};

    Vec3 m_BBox[2];

    PhysicsForeignData m_pForeignData;
    int m_iForeignData  : 16;
    int m_iForeignFlags : 16;

    struct vec2dpacked
    {
        int x : 16;
        int y : 16;
    };
    vec2dpacked m_ig[2];
    int m_iGThunk0;

    CPhysicalPlaceholder* m_pEntBuddy;
    volatile unsigned int m_bProcessed;
    int m_id : 23;
    int m_bOBBThunks : 1;
    int m_iSimClass : 8;
    int m_iDeletionTime;
    mutable int m_lockUpdate;
};

#endif // CRYINCLUDE_CRYPHYSICS_PHYSICALPLACEHOLDER_H
