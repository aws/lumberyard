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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTAREA_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTAREA_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

// Description:
//      Type of an area managed by IComponentArea.
enum EEntityAreaType
{
    ENTITY_AREA_TYPE_SHAPE,     // Area type is a closed set of points forming shape.
    ENTITY_AREA_TYPE_BOX,       // Area type is a oriented bounding box.
    ENTITY_AREA_TYPE_SPHERE,    // Area type is a sphere.
    ENTITY_AREA_TYPE_GRAVITYVOLUME,    // Area type is a volume around a bezier curve.
    ENTITY_AREA_TYPE_SOLID          // Area type is a solid which can have any geometry figure.
};


//! Interface for the entity's area component.
//! Area components allow for an entity to host an area trigger.
//! The area is one of several kinds of shapes (box, sphere, etc).
//! When marked entities cross this area's border, it will send
//! events to the target entities. Ex:
//! ENTITY_EVENT_ENTERAREA, ENTITY_EVENT_LEAVEAREA
struct IComponentArea
    : public IComponent
{
    enum EAreaFlags
    {
        FLAG_NOT_UPDATE_AREA = BIT(1), // When set points in the area will not be updated.
        FLAG_NOT_SERIALIZE = BIT(2)  // Areas with this flag will not be serialized
    };

    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentArea", 0x271FAFA6B75F404C, 0x98E4864AD657ECE0)

    IComponentArea() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Area; }

    // Area flags.
    virtual void SetFlags(int nAreaFlags) = 0;
    // Area flags.
    virtual int  GetFlags() = 0;

    // Description:
    //    Retrieve area type.
    // Return:
    //    One of EEntityAreaType enumerated types.
    virtual EEntityAreaType GetAreaType() const = 0;

    // Description:
    //    Sets area to be a shape, and assign points to this shape.
    //    Points are specified in local entity space, shape will always be constructed in XY plane,
    //    lowest Z of specified points will be used as a base Z plane.
    //    If fHeight parameter is 0, shape will be considered 2D shape, and during intersection Z will be ignored
    //    If fHeight is not zero shape will be considered 3D and will accept intersection within vertical range from baseZ to baseZ+fHeight.
    // Arguments:
    //    vPoints                   - Array of 3D vectors defining shape vertices.
    //      vObstructSound  - Array of corresponding booleans that indicate sound obstruction
    //    nPoints                   - Number of vertices in vPoints array.
    //    fHeight                   - Height of the shape .
    // See Also: SetSphere,SetBox,GetSphere,GetBox,GetPoints
    virtual void    SetPoints(const Vec3* const vPoints, const bool* const pabSoundObstructionSegments, int const nPointsCount, float const fHeight) = 0;

    // Description:
    //    Sets area to be a Box, min and max must be in local entity space.
    //    Host entity orientation will define the actual world position and orientation of area box.
    // See Also: SetSphere,SetPoints,GetSphere,GetBox,GetPoints
    virtual void    SetBox(const Vec3& min, const Vec3& max, const bool* const pabSoundObstructionSides, size_t const nSideCount) = 0;

    // Description:
    //    Sets area to be a Sphere, center and radius must be specified in local entity space.
    //    Host entity world position will define the actual world position of the area sphere.
    // See Also: SetBox,SetPoints,GetSphere,GetBox,GetPoints
    virtual void    SetSphere(const Vec3& vCenter, float fRadius) = 0;

    // Description:
    //      This function need to be called before setting convexhulls for a AreaSolid,
    //      And then AddConvexHullSolid() function is called as the number of convexhulls consisting of a geometry.
    //  See Also : AddConvexHullToSolid,EndSettingSolid
    virtual void    BeginSettingSolid(const Matrix34& worldTM) = 0;

    // Description:
    //      Add a convexhull to a solid. This function have to be called after calling BeginSettingSolid()
    //  See Also : BeginSettingSolid,EndSettingSolid
    virtual void    AddConvexHullToSolid(const Vec3* verticesOfConvexHull, bool bObstruction, int numberOfVertices) = 0;

    // Description:
    //      Finish setting a solid geometry. Generally the BSPTree based on convexhulls which is set before is created in this function.
    //  See Also : BeginSettingSolid,AddConvexHullToSolid
    virtual void    EndSettingSolid() = 0;

    // Description:
    //    Retrieve number of points for shape area, return 0 if not area type is not shape.
    // See Also: SetPoints,GetPoints
    virtual int   GetPointsCount() = 0;
    // Description:
    //    Retrieve array of points for shape area, will return NULL for all other area types.
    // See Also: SetSphere,SetBox,SetPoints,GetSphere,GetBox
    virtual const Vec3* GetPoints() = 0;
    // Description:
    //    Retrieve shape area height, if height is 0 area is 2D.
    // See Also: SetPoints
    virtual float GetHeight() = 0;
    // Description:
    //    Retrieve min and max in local space of area box.
    // See Also: SetSphere,SetBox,SetPoints,GetSphere,GetBox,GetPoints
    virtual void    GetBox(Vec3& min, Vec3& max) = 0;
    // Description:
    //    Retrieve center and radius of the sphere area in local space.
    // See Also: SetSphere,SetBox,SetPoints,GetSphere,GetBox,GetPoints
    virtual void    GetSphere(Vec3& vCenter, float& fRadius) = 0;

    virtual void SetGravityVolume(const Vec3* pPoints, int nNumPoints, float fRadius, float fGravity, bool bDontDisableInvisible, float fFalloff, float fDamping) = 0;

    // Description:
    //    Set area ID, this id will be provided to the script callback OnEnterArea , OnLeaveArea.
    // See Also: GetID
    virtual void    SetID(const int id) = 0;
    // Description:
    //    Retrieve area ID.
    // See Also: SetID
    virtual int     GetID() const = 0;

    // Description:
    //    Set area group id, areas with same group id act as an exclusive areas,
    //    If 2 areas with same group id overlap, entity will be considered in the most internal area (closest to entity).
    // See Also: GetGroup
    virtual void    SetGroup(const int id) = 0;

    // Description:
    //    Retrieve area group id.
    // See Also: SetGroup
    virtual int     GetGroup() const = 0;

    // Description:
    //    Set priority defines the individual priority of an area,
    //    Area with same group ideas will depend on which has the higher priority
    // See Also: GetPriority
    virtual void    SetPriority(const int nPriority) = 0;

    // Description:
    //    Retrieve area priority.
    // See Also: SetPriority
    virtual int     GetPriority() const = 0;

    // Description:
    //    Sets sound obstruction depending on area type
    virtual void    SetSoundObstructionOnAreaFace(int unsigned const nFaceIndex, bool const bObstructs) = 0;

    // Description:
    //    Add target entity to the area.
    //    When someone enters/leaves an area, it will send ENTERAREA,LEAVEAREA,AREAFADE, events to these target entities.
    // See Also: ClearEntities
    virtual void    AddEntity(EntityId id) = 0;

    // Description:
    //    Add target entity to the area.
    //    When someone enters/leaves an area, it will send ENTERAREA,LEAVEAREA,AREAFADE, events to these target entities.
    // See Also: ClearEntities
    virtual void    AddEntity(EntityGUID guid) = 0;

    // Description:
    //    Removes all added target entities.
    // See Also: AddEntity
    virtual void    ClearEntities() = 0;

    // Description:
    //    Set area proximity region near the border.
    //    When someone is moving within this proximity region from the area outside border
    //    Area will generate ENTITY_EVENT_AREAFADE event to the target entity, with a fade ratio from 0, to 1.
    //    Where 0 will be at he area outside border, and 1 inside the area in distance fProximity from the outside area border.
    // See Also: GetProximity
    virtual void    SetProximity(float fProximity) = 0;

    // Description:
    //    Retrieves area proximity.
    // See Also: SetProximity
    virtual float   GetProximity() = 0;

    // Description:
    //    computes and returned squared distance to a point which is outside
    //    OnHull3d is the closest point on the hull of the area
    virtual float CalcPointNearDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d) = 0;

    // Description:
    //    computes and returned squared distance from a point to the hull of the area
    //    OnHull3d is the closest point on the hull of the area
    //      This function is not sensitive of if the point is inside or outside the area
    virtual float ClosestPointOnHullDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d) = 0;

    // Description:
    //    checks if a given point is inside the area
    //    ignoring the height speeds up the check
    virtual bool    CalcPointWithin(EntityId const nEntityID, Vec3 const& Point3d, bool const bIgnoreHeight = false) const = 0;

    // Description:
    //    get number of entities in area
    virtual size_t GetNumberOfEntitiesInArea() const = 0;

    // Description:
    //    get entity in area by index
    virtual EntityId GetEntityInAreaByIdx(size_t index) const = 0;

    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentArea);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTAREA_H