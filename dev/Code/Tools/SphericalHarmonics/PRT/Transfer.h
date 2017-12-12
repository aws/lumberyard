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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TRANSFER_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TRANSFER_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include "ITransfer.h"
#include "RayCache.h"
#ifdef _WIN32
#include <windows.h>//needed for threading
#endif

//IMPORTANT NOTE: put all meshes for a mesh vector together into the 3d transport sampler/raycaster to take inter object shadowing/interreflection into account
//IMPORTANT NOTE: all meshes need to be in world coordinates
/* explanation of how the macro/meso scale system works(switched on in preprocess by setting bumpGranularity in transfer parameters to true)

    - per vertex: the visibility coefficients resulting from integration (Vi) are convolved with the bump normal to retrieve a scalar value
      representing the visibility at this vertex
    - at bump level the normal is used to access the environment irradiance colour(lighting SH coefficients aligned to object coord system(Li)),
      multiplied by the visibility scalar results in shadowed transfer at bump granularity
    - separate RGB coefficients for interreflections are computed at vertex level and added at vertex level
    - for non bump granularity the coefficients represent the surface response
    - with bump granularity the visibility is encoded as a scalar tangent space SH signal, encoded without Y2-2 and as colors
    - with bump interreflection granularity, the cos is not applied to the interreflection, a SH encodes the directional dependent
      cos term which is looked up by the bump normal and multiplied with the interreflection colour
*/

namespace NSH
{
    struct SRayResult;

    class CRayCaster;

    template< class T >
    class vectorpvector;

    namespace NSampleGen
    {
        template <class SampleCoeffType>
        class CSampleRetrieval_tpl;
    }

    namespace NMaterial
    {
        struct ISHMaterial;
    }

    namespace NTransfer
    {
        /************************************************************************************************************************************************/

        class CInterreflectionTransfer
            : public IObservableTransfer
        {
        private:
            //!< ray casting cached data for intersection info
            struct SRayCache
            {
                TSampleHandle hSampleHandle;        //!< handle to sample
                uint16 faceIndex;                               //!< face index of hit face
                uint16 meshIndex;                               //!< mesh index
            };

            typedef std::vector<SRayCache, CSHAllocator<SRayCache> > TRayCacheVec;
            typedef std::vector<TRayCacheVec, CSHAllocator<TRayCacheVec> > TRayCacheVecVec;
            typedef std::vector<NSH::CSmartPtr<CRayCaster, CSHAllocator<> >, CSHAllocator<NSH::CSmartPtr<CRayCaster, CSHAllocator<> > > > TRayCasterVec;

            static const uint32 scBorderColours = 256;//blurry anyway, amount of samples at one theta angle
            static const uint8 scLowerHSDiscretization      = 10;//adding scLowerHSDiscretization x scBorderColours

            static const float scLowerHSPercentage;

            //!< look up vectors for visibility determination for bump granularity (to apply cos lobe rather than lookup to the normals)
            struct SVISCache
            {
                SCartesianCoord_tpl<float> tangentSpaceCartCoord;                       //!< tangent space lookup vector
            };
            std::vector<SVISCache, CSHAllocator<SVISCache> >    m_VISCache;                                                         //!< VIS cache with look up vectors

            void InitializeVISCache(const SDescriptor& crSHDescriptor);         //!< init VIS cache

            //!< adjusts the coefficients: scale and rotate into tangent space if bump mapped granularity
            //!< it does add some useful visibility values for the lower hemisphere
            void AdjustDirectCoefficients
            (
                const TSampleVec& crSamples,
                const uint32 cSampleCount,
                const CSimpleIndexedMesh& crMesh,
                const Vec2& crTexCoord,
                NSH::CRayCaster& rRayCaster,
                const float cRayLength,
                const NSH::NTransfer::STransferParameters& crParameters,
                const uint32 cNormalIndex,
                NSH::SCoeffList_tpl<TScalarCoeff>& rCoeffListDirect,
                const double cBaseSampleScale,
                const NSH::NMaterial::ISHMaterial& crMat,
                TRayResultVec& rRayResults,
                const SDescriptor& crSHDescriptor,
                const ITransferConfigurator& crConfigurator,
                const bool cHasTransparentMats = false,
                const bool cApplyValuesToLowerHS = true
            );

        public:
            CInterreflectionTransfer()      //!< standard constructor
            {
                m_Results.reserve(50);  //reserve memory for ray cast results
            }

            virtual ~CInterreflectionTransfer(){}

            //!< transfer specific process function
            virtual const bool Process
            (
                const NSampleGen::ISampleGenerator& crSampleGen,        //!< samples generator where to get samples from
                const TGeomVec& crMeshes,                                                       //!< vector of meshes to work on
                const SDescriptor& crSHDescriptor,                                  //!< descriptor for spherical harmonic setup
                const STransferParameters& crParameters,                        //!< parameters to control the computation
                const ITransferConfigurator& crConfigurator                 //!< configurator for transfer process
            );

            inline TScalarCoeffVecVec& GetDirectCoefficients()
            {
                return m_DirectCoefficients;
            }

            //!< retrieves the current and total processing state
            virtual void GetProcessingState(STransferStatus& rState)
            {
                rState = m_State;
            }

        private:
            typedef std::vector<TRayCacheVecVec, CSHAllocator<TRayCacheVecVec> > TIntersectionsInfo;    //!< intersection info caches the handles to the hit triangles
            typedef TRayCacheVec::const_iterator    TIntersectionsInfoIter;                                     //!< intersection info iterator
            typedef std::vector<uint16, CSHAllocator<uint16> > THemispherePerMeshCountInfo;                                             //!< counter for each vertex within one mesh
            typedef std::vector<THemispherePerMeshCountInfo, CSHAllocator<THemispherePerMeshCountInfo> > THemisphereCountInfo;                  //!< counter for each mesh

            TRayResultVec m_Results;        //!< results of ray casting for multiple results, used here because it allocates memory too often otherwise

            TScalarCoeffVecVec m_DirectCoefficients;        //!< direct coefficients

            TIntersectionsInfo  m_IntersectionInfo;     //!< store for each vertex a triangle id and barycentric coordinates if the ray intersects, this will be used for the subsequent interreflection passes
            THemisphereCountInfo m_HSVertexCounter;     //!< counter for each vertex how many hemisphere samples were taken, needed for interreflection scale
            STransferStatus m_State;                                    //!< processing state
            TUint32Vec m_VerticesToSHProcessPerMesh;    //!< vertices to process for SH for each mesh

            //!< result of multiple thread ray casting calls
            typedef struct SThreadRayResult
            {
                TRGBCoeffType incidentIntensity;//!< incident intensity converted to exitance intensity for coefficients(determined by materials and ray path)
                bool continueLoop;                          //!< indicates whether to continue loop or not
                bool applyTransparency;                 //!< indicates whether to apply transparency for visibility or not
                bool returnValue;                               //!< return value for ray caster
            }SThreadRayResult;

            //!< query for multiple thread ray casting calls
            typedef struct SThreadRayVertexQuery
            {
                TVec pos;                                                                       //!< vertex pos
                TVec normal;                                                                //!< vertex normal
                Vec2 texCoord;                                                          //!< tex coord for vertex
                const NSH::NMaterial::ISHMaterial* cpMat;       //!< associated vertex material
                uint32 vertexIndex;                                                 //!< index of vertex
                uint32 normalIndex;                                                 //!< index of normal
                uint32 visQueries;                                                  //!< vertex queries for visibility (for log statistics)
                float vis;                                                                  //!< counts the resulted visibility (for log statistics)
                SCoeffList_tpl<TScalarCoeff>* pCoeffListDirect;     //!< direct coefficients for this vertex
                int hemisphereSampleCount;                                  //!< number of samples processed on the upper hemisphere, remember that hemispherical retrieval only covers 2 g_cPi of the sphere

                SThreadRayVertexQuery()
                    : hemisphereSampleCount(0)
                    , vis(0.f)
                    , visQueries(0){}
            }SThreadRayVertexQuery;

            typedef std::vector<SThreadRayVertexQuery, CSHAllocator<SThreadRayVertexQuery> > TThreadRayVertexQueryVec;

            //!< thread params passed to ray casting thread procedure
#pragma warning (disable : 4512)
            typedef struct SThreadParams
            {
                CRayCaster& rRayCaster;                                             //!< individual copy of ray casting interface
                const TSampleVec& crSamples;                                    //!< samples to iterate(the same for each vertex)
                const STransferParameters& crParameters;            //!< transfer params
                TThreadRayVertexQueryVec& rQueries;                     //!< ray casting queries(result is stored in there too)
                TRayCacheVecVec& rIntersectionInfo;                     //!< intersection info to fill
                const TGeomVec& crMeshes;                                           //!< vector of meshes to work on
                const float cRayLen;                                                    //!< length of ray
                const bool cHasTransparentMats;                             //!< determines whether there are any transparent materials or not(in that case ray casting needs to be handled in another way)
                TRayResultVec& rRayResultVec;                                   //!< results for ray casting (save mem allocs)
                const ITransferConfigurator& crConfigurator;    //!< configurator to use

                SThreadParams
                (
                    CRayCaster& rRayCasterParam,
                    const TSampleVec& crSamplesParam,
                    const STransferParameters& crParametersParam,
                    TThreadRayVertexQueryVec& rQueriesParam,
                    TRayCacheVecVec& rIntersectionInfoParam,
                    const TGeomVec& crMeshesParam,
                    const float cRayLenParam,
                    const bool cHasTransparentMatsParam,
                    TRayResultVec& rRayResultVecParam,
                    const ITransferConfigurator& crConfiguratorParam
                )
                    : rRayCaster(rRayCasterParam)
                    , crSamples(crSamplesParam)
                    , crParameters(crParametersParam)
                    , rQueries(rQueriesParam)
                    , rIntersectionInfo(rIntersectionInfoParam)
                    , crMeshes(crMeshesParam)
                    , cRayLen(cRayLenParam)
                    , cHasTransparentMats(cHasTransparentMatsParam)
                    , rRayResultVec(rRayResultVecParam)
                    , crConfigurator(crConfiguratorParam)
                {}
            }SThreadParams;
#pragma warning (default : 4512)

            //!< core ray casting thread, casts ray and does everything else what PerformRayCasting does (but for threads)
            static DWORD WINAPI RayCastingThread(LPVOID pThreadParam);

            //!< determine ray length from bounding box extensions of all participating meshes
            static const float DetermineRayLength(const TGeomVec& crMeshes);
            //!< determine whether there are any transparent materials or not(in that case ray casting needs to be handled in another way)
            static const bool HasTransparentMaterials(const TGeomVec& crMeshes);

            //!< core ray casting call for each sample and each vertex
            //!< returns true if the ray has hit something
            const bool PerformRayCasting
            (
                CRayCaster& rRayCaster,                 //!< interface to ray cast engine
                const bool cHasTransparentMats, //!< determines whether there are any transparent materials or not(in that case ray casting needs to be handled in another way)
                const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSample, //!< sample to shoot ray for
                const float cRayLen,                        //!< length of ray
                const TVec cPos,                                //!< vertex pos
                const TVec cNormal,                         //!< vertex normal
                const STransferParameters& crParameters,    //!< transfer params
                TRGBCoeffType& rIncidentIntensity,  //!< incident intensity for coefficients(determined by materials and ray path)
                bool& rContinueLoop,                        //!< indicates whether to continue loop or not
                bool& rApplyTransparency,               //!< indicates whether to apply transparency for visibility or not
                bool& rCacheRecorded,                       //!< indicates whether a hit was reorded or not
                SRayCache& rRayInterreflectionCache,    //!< records all intersections for interreflections
                const TGeomVec& crMeshes,               //!< vector of meshes to work on
                const bool cIsOnUpperHemisphere,//!< indicates where sample lies with respect to vertex
                const ITransferConfigurator& crConfigurator //!< configurator for ray casting result processing
            );

            void FreeMemory();  //!< resets internal vectors

            void PrepareVectors                                                                                 //!< helper functions for Process, prepares all coeff(also temp used ones) vectors
            (
                const TGeomVec& crMeshes,
                const STransferParameters& crParameters,
                const SDescriptor& crSHDescriptor
            );

            void ComputeDirectPassMultipleThreading                                             //!< helper functions for Process, computes the direct illumination values using two ray cast threads
            (
                const NSampleGen::ISampleGenerator& crSampleGen,
                const TGeomVec& crMeshes,
                const SDescriptor& crSHDescriptor,
                CRayCaster& rRayCaster,
                const STransferParameters& crParameters,
                const ITransferConfigurator& crConfigurator
            );

            void ComputeDirectPass                                                                      //!< helper functions for Process, computes the direct illumination values
            (
                const NSampleGen::ISampleGenerator& crSampleGen,
                const TGeomVec& crMeshes,
                const SDescriptor& crSHDescriptor,
                CRayCaster& rRayCaster,
                const STransferParameters& crParameters,
                const ITransferConfigurator& crConfigurator
            );
        };
    }
}

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TRANSFER_H
