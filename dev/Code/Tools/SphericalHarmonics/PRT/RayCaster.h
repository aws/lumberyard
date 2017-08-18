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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RAYCASTER_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RAYCASTER_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include <PRT/PRTTypes.h>
#include <limits>
#include "FullVisCache.h"
#include "RayCache.h"
#include <PRT/TransferParameters.h>
#include <algorithm>

namespace NSH
{
    //!< ray caster implementation for ray caster interface
    class CRayCaster
    {
    public:
        INSTALL_CLASS_NEW(CRayCaster)

    protected:

        //return data class
        struct SHitData
        {
            const RasterCubeUserT* pHitResult;          //!< closest hit storage
            Vec3                            baryCoord;      //!< barycentric coordinates of last recent hit, set by  ReturnElement
            float                           dist;                   //!< distance to ray source

            SHitData()
                : pHitResult(NULL)
                , baryCoord(0, 0, 0)
                , dist(std::numeric_limits<float>::max()){}
            void Reset(){ pHitResult = NULL; baryCoord.x = baryCoord.y = baryCoord.z = 0; dist = std::numeric_limits<float>::max(); }
            const bool operator<(const SHitData& crComp) const{return dist < crComp.dist; }//for std::set
            const SHitData& operator=(const SHitData& crFrom)
            {
                pHitResult = crFrom.pHitResult;
                baryCoord    = crFrom.baryCoord;
                dist             = crFrom.dist;
                return *this;
            }
        };

        struct SAscendingHitDataSort
        {
            const bool operator()(const SHitData& rStart, const SHitData& rEnd) const
            {
                return rStart < rEnd;
            }
        };

        typedef std::vector<SHitData, CSHAllocator<SHitData> > THitVec;

        /************************************************************************************************************************************************/

        //records the closest hit, if not broke after first hit, it gets the closest one with respect to bias
        class CAnyHit
            : public CEveryObjectOnlyOnce
        {
        public:

            void SetupRay(const Vec3& crRayDir, const Vec3& crRayOrigin, const float cRayLen, const float cSign, const Vec3& crNormal, const float cBias)
            {
                CEveryObjectOnlyOnce::SetupRay(crRayDir, crRayOrigin, cRayLen, cBias);
                m_PNormal = crNormal;
                m_D = cSign;
                m_HitData.Reset();
                m_fClosest = std::numeric_limits<float>::max();
            };
            bool IsIntersecting() const { return(m_HitData.pHitResult != 0); }

            Vec3                            m_PNormal;          //!< plane normal
            float                           m_D;                        //!< sign of plane normal

            const NSH::EReturnSinkValue ReturnElement(const RasterCubeUserT& crInObject, float& rInoutfRayMaxDist);

            const SHitData& GetHitData() const {return m_HitData; }

            void ResetHits()
            {
                m_HitData.Reset();
                m_fClosest = std::numeric_limits<float>::max();
                m_AlreadyTested = 0;
            }

        protected:
            float                           m_fClosest;         //!< closest hit distance recorded
            SHitData                    m_HitData;          //!< closest hit storage with   barycentric coordinates
        };

        /************************************************************************************************************************************************/

        //records all hits in a set sorted by distance to ray(closest first), if not broke after first hit, it gets the closest one with respect to bias
        class CAllHits
            : public CEveryObjectOnlyOnce
        {
        public:

            void SetupRay(const Vec3& crRayDir, const Vec3& crRayOrigin, const float cRayLen, const float cSign, const Vec3& crNormal, const float cBias)
            {
                CEveryObjectOnlyOnce::SetupRay(crRayDir, crRayOrigin, cRayLen, cBias);
                m_PNormal = crNormal;
                m_D = cSign;
                m_HitData.Reset();
                m_Hits.resize(0);
            };
            bool IsIntersecting() const { return(!m_Hits.empty()); }

            Vec3                            m_PNormal;          //!< plane normal
            float                           m_D;                        //!< sign of plane normal
            const NSH::EReturnSinkValue ReturnElement(const RasterCubeUserT& crInObject, float& rInoutfRayMaxDist);
            //pay attention that this is only valid til next setup ray call
            void GetHitData(THitVec& rHitSet)
            {
                //do a binary sort on the data
                std::sort(m_Hits.begin(), m_Hits.end(), SAscendingHitDataSort());
                rHitSet.resize(m_Hits.size());
                int i = 0;
                const THitVec::const_iterator cEnd = m_Hits.end();
                for (THitVec::const_iterator iter = m_Hits.begin(); iter != cEnd; ++iter)
                {
                    rHitSet[i++] = *iter;
                }
            }

            void ResetHits()
            {
                m_HitData.Reset();
                m_Hits.resize(0);
                m_AlreadyTested = 0;
            }

        protected:
            SHitData        m_HitData;          //!< closest hit storage with   barycentric coordinates
            THitVec         m_Hits;                 //!< all hits

            friend class CRayCaster;
        };

        /************************************************************************************************************************************************/

    public:
        CRayCaster();   //!< standard constructor
        ~CRayCaster();  //!< destructor, we need it here because of referencing
        //!< sets up world space geometry for ray tracer
        const bool SetupGeometry(const TGeomVec& crGeometries, const NTransfer::STransferParameters& crParameters);
        //!< casts a ray, returns true if at least one triangle intersects ray, rResult contains intersecting data if so, returns closest hit
        const bool CastRay(SRayResult& rResult, const TVec& crFrom, const TVec& crDir, const double cRayLen, const TVec& crSourcePlaneNormal, const float cBias = 0.05f) const;
        //!< casts a ray, returns true if at least one triangle intersects ray, all hits are recorded and sorted by distance in ascending order
        const bool CastRay(TRayResultVec& rResults, const TVec& crFrom, const TVec& crDir, const double cRayLen, const TVec& crSourcePlaneNormal, const float cBias = 0.05f) const;
        //!< logs the mesh stats for the ray casting
        void LogMeshStats();
        //!< resets the mesh stats for the ray cast logging
        void ResetMeshStats();
        //!< resets the cache, there two cache systems: one for the full visibility cache and one for the last recent hit caches between subsequent calls
        void ResetCache
        (
            const TVec& crFrom = TVec(0, 0, 0),
            const float cRayLen = 1.0f,
            const TVec& cNormal = TVec(0, 0, 0),
            const float cRayTracingBias = 0.05f,
            const bool cOnlyUpperHemisphere = false,
            const bool cUseFullVisCache = false
        );

        //special functions used for multi threaded ray casting
        const uint32 GetFullVisQueries() const;     //!< retrieves the number of full visibility queries
        const uint32 GetFullVisSuccesses() const;   //!< retrieves the number of full visibility successes

        NSH::CSmartPtr<NSH::CRayCaster, CSHAllocator<> > CreateClone() const;                   //!< creates a ray caster clone

    protected:
        TRasterCubeImpl*    m_pRasterCube;              //!< ref counted raster cube for this cluster, pointer therefore
        mutable uint32      m_CloneRefCount;            //!< reference count for clones
        mutable CRayCache   m_RayCache;                     //!< ray cache
        CFullVisCache           m_FullVisCache;             //!< full vis cache, used to discard any occlusion queries for objects being almost fully visible
        mutable CAllHits    m_AllHits;                      //!< all hits storage sink, reused to avoid reallocations
        mutable CAnyHit     m_AnyHits;                      //!< any hits storage sink, reused to avoid reallocations
        mutable THitVec     m_HitData;                      //!< hits retrieval storage for m_AllHits, reused to avoid reallocations

        union
        {
            uint32          m_IsClone;                              //!< indicates whether this is a clone or not, 0 if no
            CRayCaster* m_pOriginal;                            //!< pointer to original ray caster
        };

        NTransfer::STransferParameters m_Parameters;    //!< transfer parameters

        mutable uint32  m_FullVisQueries;               //!< counts the number of full vis queries
        mutable uint32  m_FullVisSuccesses;         //!< counts the number of full vis queries which succeeded

        TVec                        m_Source;                               //!< source pos to use for cache queries
        TVec                        m_Normal;                               //!< source direction to use for cache queries
        bool                        m_UseFullVisCache;          //!< indicates whether to use the full vis cache or not
        bool                        m_OnlyUpperHemisphere;  //!< indicates whether to use only the upper hemisphere for raycasting(-caching) or not
        float                       m_RayLen;                               //!< ray length for cache queries
        float                       m_RayTracingBias;               //!< ray tracing bias for cache queries

        void DecrementCloneRefCount() const;        //!< clone ref counting decrement for destructor
        void IncrementCloneRefCount() const;        //!< clone ref counting increment for destructor

        CRayCaster(const CRayCaster&);                  //!< disallow copy constructor
    };
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_RAYCASTER_H
