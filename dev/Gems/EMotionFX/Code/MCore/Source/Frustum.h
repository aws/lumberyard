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

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "PlaneEq.h"
#include "Vector.h"
#include "Array.h"


namespace MCore
{
    // forward declarations
    class AABB;
    class Matrix;

    /**
     * The frustum base class.
     * A frustum is a convex volume which we can use for several
     * visibility related things, like occlusion culling.
     * The frustum is constructed out of planes and the class provides
     * methods to quickly determine if several bounding volume objects are
     * intersection this volume or not. Also it's possible to detect if
     * these bounding volume objects are completely inside the volume or not.
     * This is especially usefull for occlusion culling. If the bounding box
     * of an object is completely inside the occluders frustums volume, it means
     * that this object will be invisible.
     * The same can be used for view frustum culling. If an objects bounding volume
     * is partially or completely inside the volume, it means it's visible to the viewer.
     * Don't forget that the frustum has to be CONVEX in order to work correctly.
     */
    class MCORE_API Frustum
    {
    public:
        /**
         * The constructor.
         */
        MCORE_INLINE Frustum() { mPlanes.SetMemoryCategory(MCORE_MEMCATEGORY_FRUSTUM); }

        /**
         * The virtual destructor.
         * Will get rid of all the planes automatically.
         */
        MCORE_INLINE virtual ~Frustum()                                                             { RemoveAllPlanes(); }

        /**
         * Adds a plane to the frustum volume.
         * @param plane The plane to add to the volume.
         */
        MCORE_INLINE void AddPlane(const PlaneEq& plane)                                            { this->mPlanes.Add(plane); }

        /**
         * Set the plane at a given index.
         * @param index The plane number, which must be in range of [0..GetNumPlanes()-1]
         * @param plane The plane to set at the given index.
         */
        MCORE_INLINE void SetPlane(uint32 index, const PlaneEq& plane)                              { MCORE_ASSERT(index < this->mPlanes.GetLength()); this->mPlanes[index] = plane; }

        /**
         * Returns the number of planes in the frustum volume.
         * @result The number of planes of the frustum volume.
         */
        MCORE_INLINE uint32 GetNumPlanes() const                                                    { return this->mPlanes.GetLength(); }

        /**
         * Returns a given plane from the frustum volume.
         * @param index The plane number, which must be in range of [0..GetNumPlanes()-1]
         * @result The requested plane.
         */
        MCORE_INLINE const PlaneEq& GetPlane(uint32 index)                                          { MCORE_ASSERT(index < this->mPlanes.GetLength()); return this->mPlanes[index]; }

        /**
         * Removes all planes inside the current frustum volume.
         * After calling this method, the method GetNumPlanes() will return 0.
         * The destructor of this class will automatically remove all planes on destruction.
         */
        MCORE_INLINE void RemoveAllPlanes()                                                         { this->mPlanes.Clear(); }

        /**
         * Removes the plane at a given index from the frustum volume.
         * @param index The plane number to remove. Must be in range of [0..GetNumPlanes()-1]
         */
        MCORE_INLINE void RemovePlane(uint32 index)                                                 { MCORE_ASSERT(index < this->mPlanes.GetLength()); this->mPlanes.Remove(index); }

        /**
         * Checks if a given axis aligned bounding box intersects (or is completely inside) the frustum volume.
         * @param box The axis aligned bounding box to use for the check.
         * @result True will be returned in case of an intersection with the volume (so also when completely inside), otherwise false will be returned.
         */
        bool PartiallyContains(const AABB& box) const;

        /**
         * Checks if a given axis aligned bounding box is completely inside the frustum volume.
         * @param box The axis aligned bounding box to use for the check.
         * @result True will be returned in case the box is completely inside the volume. False is returned when the box is not completely inside the volume (but partially or completely outside).
         */
        bool CompletelyContains(const AABB& box) const;

    protected:
        Array< PlaneEq > mPlanes;       /**< The collection of planes which form the frustum volume */
    };



    /**
     * The view frustum class, which is inherited from the Frustum class.
     * A view frustum is a frustum which is constructed out of exactly six planes and is used for
     * frustum culling, so to detect what objects are potentially in the view volume (on screen) and which ones are not.
     * The class contains a special method (Init) to initialize the frustum planes for given camera settings.
     * @see Init
     */
    class MCORE_API ViewFrustum
        : public Frustum
    {
    public:
        /**
         * The viewfrustum plane indices.
         */
        enum EFrustumPlane
        {
            FRUSTUMPLANE_LEFT   = 0,    /**< The left frustum plane. */
            FRUSTUMPLANE_RIGHT  = 1,    /**< The right frustum plane. */
            FRUSTUMPLANE_TOP    = 2,    /**< The top frustum plane. */
            FRUSTUMPLANE_BOTTOM = 3,    /**< The bottom frustum plane. */
            FRUSTUMPLANE_NEAR   = 4,    /**< The near frustum plane. */
            FRUSTUMPLANE_FAR    = 5     /**< The far frustum plane. */
        };

        /**
         * The constructor, which automatically creates six planes.
         */
        MCORE_INLINE ViewFrustum()
            : Frustum() { mPlanes.SetMemoryCategory(MCORE_MEMCATEGORY_FRUSTUM); mPlanes.Resize(6); }

        /**
         * The destructor.
         */
        MCORE_INLINE ~ViewFrustum()     { }

        /**
         * Initialize the frustum planes from the given matrix.
         * @param viewProjMatrix The view projection matrix from which we will determine the view frustum planes. The view projection matrix is the following pre-multiplied matrix: viewMatrix*projectionMatrix.
         */
        void InitFromMatrix(const Matrix& viewProjMatrix);

        /**
         * Prints the view frustum into the logfile or debug output, using MCore::LogDetailedInfo().
         */
        void Log();
    };
}   // namespace MCore
