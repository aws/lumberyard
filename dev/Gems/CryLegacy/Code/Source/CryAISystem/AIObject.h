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

// Description : interface for the CAIObject class.


#ifndef CRYINCLUDE_CRYAISYSTEM_AIOBJECT_H
#define CRYINCLUDE_CRYAISYSTEM_AIOBJECT_H
#pragma once

#include "AStarSolver.h"
#include "IPhysics.h"
#include "IAIObject.h"
#include "Reference.h"

#ifdef CRYAISYSTEM_DEBUG
    #include "AIRecorder.h"
#endif //CRYAISYSTEM_DEBUG

#include <VisionMap.h>

struct GraphNode;
class CFormation;
class CRecorderUnit;


/*! Basic ai object class. Defines a framework that all puppets and points of interest
later follow.
*/
class CAIObject
    : public IAIObject
#ifdef CRYAISYSTEM_DEBUG
    , public CRecordable
#endif //CRYAISYSTEM_DEBUG
{
    // only allow these classes to create AI objects.
    //  TODO: ideally only one place where objects are created
    friend class CAIObjectManager;
    friend struct SAIObjectCreationHelper;
protected:
    CAIObject();
    virtual ~CAIObject();

public:
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //IAIObject interfaces//////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////
    //Startup/shutdown//////////////////////////////////////////////////////////////////////
    virtual void Reset(EObjectResetType type);
    virtual void Release();

    // "true" if method Update(EObjectUpdate type) has been invoked AT LEAST once
    virtual bool IsUpdatedOnce() const;

    virtual bool IsEnabled() const;
    virtual void Event(unsigned short, SAIEVENT* pEvent);
    virtual void EntityEvent(const SEntityEvent& event);
    //Startup/shutdown//////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////
    //Basic properties//////////////////////////////////////////////////////////////////////
    virtual tAIObjectID GetAIObjectID() const;

    virtual const VisionID& GetVisionID() const;
    virtual void SetObservable(bool observable);
    virtual bool IsObservable() const;

    virtual void GetObservablePositions(ObservableParams& observableParams) const override;
    virtual uint32 GetObservableTypeMask() const;
    virtual void GetPhysicalSkipEntities(PhysSkipList& skipList) const;
    void UpdateObservableSkipList() const;

    virtual void SetName(const char* pName);
    virtual const char* GetName() const;

    virtual unsigned short GetAIType() const;
    virtual ESubType GetSubType() const;
    virtual void SetType(unsigned short type);

    virtual void SetPos(const Vec3& pos, const Vec3& dirForw = Vec3_OneX);
    virtual const Vec3& GetPos() const;
    virtual const Vec3 GetPosInNavigationMesh(const uint32 agentTypeID) const;

    virtual void SetRadius(float fRadius);
    virtual float GetRadius() const;

    //Body direction here should be animated character body direction, with a fall back to entity direction.
    virtual const Vec3& GetBodyDir() const;
    virtual void SetBodyDir(const Vec3& dir);

    virtual const Vec3& GetViewDir() const;
    virtual void SetViewDir(const Vec3& dir);
    virtual EFieldOfViewResult IsPointInFOV(const Vec3& pos, float distanceScale = 1.0f) const;

    virtual const Vec3& GetEntityDir() const;
    virtual void SetEntityDir(const Vec3& dir);

    virtual const Vec3& GetMoveDir() const;
    virtual void SetMoveDir(const Vec3& dir);
    virtual Vec3 GetVelocity() const;

    virtual size_t GetNavNodeIndex() const;
    //Basic properties//////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////



    ////////////////////////////////////////////////////////////////////////////////////////
    //Serialize/////////////////////////////////////////////////////////////////////////////
    //Tell AI what entity owns this so can sort out linking during save/load
    virtual void SetEntityID(unsigned ID);
    virtual unsigned GetEntityID() const;
    virtual IEntity* GetEntity() const;

    virtual void Serialize(TSerialize ser);
    virtual void PostSerialize() {};

    bool ShouldSerialize() const;
    void SetShouldSerialize(bool ser) { m_serialize = ser; }
    bool IsFromPool() const { return m_createdFromPool; }
    //Serialize/////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////



    ////////////////////////////////////////////////////////////////////////////////////////
    //Starting to assume WAY to many conflicting things about possible derived classes//////

    //Returns position weapon fires from
    virtual void SetFirePos(const Vec3& pos);
    virtual const Vec3& GetFirePos() const;
    virtual IBlackBoard* GetBlackBoard() { return NULL; }

    virtual uint8 GetFactionID() const;
    virtual void SetFactionID(uint8 factionID);

    virtual void SetGroupId(int id);
    virtual int GetGroupId() const;
    virtual bool IsHostile(const IAIObject* pOther, bool bUsingAIIgnorePlayer = true) const;
    virtual void SetThreateningForHostileFactions(const bool threatening);
    virtual bool IsThreateningForHostileFactions() const;

    virtual bool IsTargetable() const;

    // Returns the EntityId to be used by the perception manager when this AIObject is perceived by another.
    virtual EntityId GetPerceivedEntityID() const;

    virtual void SetProxy(IAIActorProxy* proxy);
    virtual IAIActorProxy* GetProxy() const;

    //Starting to assume WAY to many conflicting things about possible derived classes//////
    ////////////////////////////////////////////////////////////////////////////////////////



    ////////////////////////////////////////////////////////////////////////////////////////
    //Formations////////////////////////////////////////////////////////////////////////////
    virtual bool CreateFormation(const unsigned int nCrc32ForFormationName, Vec3 vTargetPos = ZERO);
    virtual bool CreateFormation(const char* szName, Vec3 vTargetPos = ZERO);
    virtual bool HasFormation();
    virtual bool ReleaseFormation();

    virtual void         CreateGroupFormation        (IAIObject* pLeader) {; }
    virtual void         SetFormationPos             (const Vec2& v2RelPos) {; }
    virtual void         SetFormationLookingPoint    (const Vec3& v3RelPos) {; }
    virtual void         SetFormationAngleThreshold  (float fThresholdDegrees) {; }
    virtual const Vec3&  GetFormationPos             () { return Vec3_Zero; }
    virtual const Vec3   GetFormationVelocity        ();
    virtual const Vec3&  GetFormationLookingPoint    () { return Vec3_Zero; }
    //Sets a randomly rotating range for the AIObject's formation sight directions.
    virtual void SetFormationUpdateSight(float range, float minTime, float maxTime);
    //Formations////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////





    //IAIObject interfaces//////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////













    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //IAIRecordable interfaces////////////////////////////////////////////////////////////////////////////////////////
    virtual void RecordEvent(IAIRecordable::e_AIDbgEvent event, const IAIRecordable::RecorderEventData* pEventData = NULL) {};
    virtual void RecordSnapshot() {};
    virtual IAIDebugRecord* GetAIDebugRecord();

#ifdef CRYAISYSTEM_DEBUG
    CRecorderUnit* CreateAIDebugRecord();
#endif //CRYAISYSTEM_DEBUG
       //IAIRecordable interfaces////////////////////////////////////////////////////////////////////////////////////////
       //////////////////////////////////////////////////////////////////////////////////////////////////////////////////












    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //CAIObject interfaces////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////
    //Original Virtuals/////////////////////////////////////////////////////////////////////
    virtual IPhysicalEntity* GetPhysics(bool wantCharacterPhysics = false) const;

    virtual void SetFireDir(const Vec3& dir);
    virtual const Vec3& GetFireDir() const;
    virtual const Vec3& GetShootAtPos() const { return m_vPosition; } ///< Returns the position to shoot (bbox middle for tank, etc)

    virtual CWeakRef<CAIObject> GetAssociation() const { return m_refAssociation; }
    virtual void SetAssociation(CWeakRef<CAIObject> refAssociation);

    virtual void OnObjectRemoved(CAIObject* pObject) {}
    //Original Virtuals/////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////
    //local level///////////////////////////////////////////////////////////////////////////
    const Vec3& GetLastPosition() { return m_vLastPosition; }
    Vec3 GetPhysicsPos() const;
    void SetExpectedPhysicsPos(const Vec3& pos);

    const char* GetEventName(unsigned short eType) const;

    void SetSubType(ESubType type);
    inline unsigned short GetType() const { return m_nObjectType; } // used internally to inline the function calls
    //local level///////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////



    ////////////////////////////////////////////////////////////////////////////////////////
    //ObjectContainer/WeakRef stuff/////////////////////////////////////////////////////////
    void SetSelfReference(CWeakRef<CAIObject> ref)
    {
        // Should only ever have to call once
        // Should never be invalid
        // Is serialized like any other reference
        assert(m_refThis.IsNil());
        m_refThis = ref;
    }

    CWeakRef<CAIObject> GetSelfReference() const
    {
        assert(!m_refThis.IsNil());
        return m_refThis;
    }

    bool HasSelfReference() const
    {
        return m_refThis.IsSet();
    }
    //ObjectContainer/WeakRef stuff/////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////

    //CAIObject interfaces////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

protected:
    void SetVisionID(const VisionID& visionID);

    bool m_bEnabled;
    uint16 m_nObjectType;       //  type would be Dummy
    ESubType m_objectSubType;   //  subType would be soundDummy, formationPoint, etc
    int m_groupId;
    float m_fRadius;

    EntityId m_entityID;

private:
    CWeakRef<CAIObject> m_refThis;

    Vec3 m_vPosition;
    Vec3 m_vEntityDir;
    Vec3 m_vBodyDir;    // direction of AI body, animated body direction if available
    Vec3 m_vMoveDir;        // last move direction of the entity
    Vec3 m_vView;       // view direction (where my head is looking at, tank turret turned, etc)

protected:
    mutable size_t m_lastNavNodeIndex;

public:
    bool m_bUpdatedOnce;
    mutable bool m_bTouched;            // this gets frequent write access.

    // please add a new variable below, the section above is optimized for memory caches.

    Vec3 m_vLastPosition;
    CFormation* m_pFormation;

private:

    Vec3 m_vFirePosition;
    Vec3 m_vFireDir;

    Vec3 m_vExpectedPhysicsPos;      // Where the AIObject is expected to be next frame (only valid when m_expectedPhysicsPosFrameId == current frameID)
    int m_expectedPhysicsPosFrameId; // Timestamp of m_vExpectedPhysicsPos

    VisionID m_visionID;
    uint8   m_factionID;
    bool m_isThreateningForHostileFactions;
    bool m_observable;

    // pooled objects are removed by the CAIObjectManager. NB by the time Release() is called
    // on this object, it will already have been removed from the manager's m_pooledObjects map,
    // hence this is stored here.
    bool m_createdFromPool;
    bool m_serialize;

    string m_name;
protected:
    CWeakRef<CAIObject>  m_refAssociation;
};

#endif // CRYINCLUDE_CRYAISYSTEM_AIOBJECT_H
