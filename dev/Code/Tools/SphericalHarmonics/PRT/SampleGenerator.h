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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SAMPLEGENERATOR_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SAMPLEGENERATOR_H
#pragma once

#include "ISampleGenerator.h"
#include "SubTriManager.h"
#include "Icosahedron.h"
#include <float.h>

namespace NSH
{
    //!< configuration enum for sample generator
    namespace NSampleGen
    {
        /*
            TSampleHandle is just a simple index
        */
        class CLinearSampleOrganizer
            : public ISampleOrganizer
        {
        public:
            //need both constructors here
            CLinearSampleOrganizer(const uint32, const int32){}
            CLinearSampleOrganizer(){}

            virtual const uint32 ConvertIntoSHAndReOrganize
            (
                const TVec2D* cpRandomSamples,
                const uint32 cRandomSampleCount,
                const SDescriptor& rSHDescriptor
            );      //!< converts NOT NORMALIZED random samples into SH sample and reorganizes all samples

            virtual void GetSamples(TScalarVecVec& rSamples) const
            {
                rSamples.clear();
                rSamples.push_back(&m_RandomSamples);
            }

            //!< resets organizer
            virtual void Reset()
            {
                m_RandomSamples.clear();
            }

            virtual const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& GetSample(const TSampleHandle& crHandle) const
            {
                assert(crHandle < (TSampleHandle)m_RandomSamples.size());
                return m_RandomSamples[crHandle];
            }

            virtual const uint32 Capacity()
            {
                return g_cMaxHandleTypeValue;
            }

            virtual const uint32 MinCapacity()
            {
                return 1;
            }

        private:
            TSampleVec  m_RandomSamples;        //!< SH sample storage
        };

        /************************************************************************************************************************************************/

        /*
            TSampleHandle is encoded as follows:
                first 5 bits isocahedron index (0..20)
                next 2*m_SubdivisionLevels bits for subtriangle index
                last 16-5-2*m_SubdivisionLevels bits for triangle id within subtriangle
        */
        static const bool HandleEncodeFunction(TSampleHandle* pOutputHandle, const uint32 cIsocahedronLevels, const uint32 cIsocahedronIndex, const uint32 cSubtriangleIndex, const uint32 cTriangleIndex)
        {
            if
            (
                cIsocahedronIndex >= (uint32)(2 << (5 - 1)) ||
                cSubtriangleIndex >= (uint32)(2 << (cIsocahedronLevels * 2 - 1)) ||
                cTriangleIndex >= (uint32)(2 << (sizeof(TSampleHandle) * 8 - 5 - 2 * cIsocahedronLevels - 1))
            )
            {
                return false;
            }
            *pOutputHandle = TSampleHandle(cIsocahedronIndex + (cSubtriangleIndex << 5) + (cTriangleIndex << (5 + cIsocahedronLevels * 2)));
            return true;
        }

        class CIsocahedronSampleOrganizer
            : public ISampleOrganizer
        {
        public:
            CIsocahedronSampleOrganizer()
            {
                CIsocahedronSampleOrganizer(gs_cDefaultSampleCount);                //!< default constructor passing the default sample count
            }

            CIsocahedronSampleOrganizer(const uint32 cSamplesToHold, const int32 cMinSampleCountToRetrieve = -1);           //!< constructor with expected samples to hold and min samples to retrieve

            virtual const uint32 ConvertIntoSHAndReOrganize
            (
                const TVec2D* cpRandomSamples,
                const uint32 cRandomSampleCount,
                const SDescriptor& rSHDescriptor
            );      //!< converts NOT NORMALIZED random samples into SH sample and reorganizes all samples

            virtual void GetSamples(TScalarVecVec& rSamples) const;

            virtual const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& GetSample(const TSampleHandle& crHandle) const
            {
                const uint32 cFaceIndex                 = ((uint32)crHandle & 0x0000001F);//mask last 5 bits
                const uint32 cSubTriangleIndex  = (((uint32)crHandle >> 5) & ((2 << (m_SubdivisionLevels * 2 - 1)) - 1));//mask sub triangle bits
                const uint32 cTriangleIndex         = (uint32)crHandle >> (5 + m_SubdivisionLevels * 2);
                assert(cFaceIndex < NSH::NIcosahedron::g_cIsocahedronFaceCount);
                assert(cSubTriangleIndex < (uint32)pow(4.0, m_SubdivisionLevels));
#if defined(_DEBUG)
                const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSample = m_SubTriManagers[cFaceIndex].GetSample(cSubTriangleIndex, cTriangleIndex);
                assert(crSample.GetHandle() == crHandle);
                return crSample;
#else
                return m_SubTriManagers[cFaceIndex].GetSample(cSubTriangleIndex, cTriangleIndex);//enable return value optimization
#endif
            }

            //!< resets organizer
            virtual void Reset()
            {
                //reset subtri manager contents
                for (uint8 i = 0; i < NSH::NIcosahedron::g_cIsocahedronFaceCount; ++i)
                {
                    m_SubTriManagers[i].Reset();
                }
            }

            virtual const uint32 Capacity()
            {
                const uint32 cRemainingBits = sizeof(TSampleHandle) * 8 /*8 bits per byte*/ - 5 /*bits for face encoding*/ - 2 * m_SubdivisionLevels /*log(4^m_SubdivisionLevels)*/;
                assert(cRemainingBits > 1);
                return (uint32)(20 * (int)pow(4.0, m_SubdivisionLevels) * (2 << (cRemainingBits - 1)));
            }

            virtual const uint32 MinCapacity()
            {
                return 20 * 4;//we wanna have at least one subdivision level
            }

        private:
            uint8       m_SubdivisionLevels;    //!< subdivision levels for triangle manager

            typedef CSample_tpl<SCoeffList_tpl<TScalarCoeff> > TSampleType;

            NSH::NTriSub::CSampleSubTriManager_tpl<TSampleType> m_SubTriManagers[NSH::NIcosahedron::g_cIsocahedronFaceCount];   //!< the sub triangle managers for each isocahedron face
            NSH::NIcosahedron::CIsocahedronManager m_IsoManager;        //!< the isocahedron manager
        };

        /************************************************************************************************************************************************/

#pragma warning (disable : 4512)
        //!< unit sphere sample generator implementation
        template< class CSampleOrganizer, class CSampleGenPolicy >
        class CSampleGenerator
            : public ISampleGenerator
        {
            typedef CSampleGenerator<CSampleOrganizer, CSampleGenPolicy> TSampleGenType;
        public:

            INSTALL_CLASS_NEW(TSampleGenType)

            CSampleGenerator(const SDescriptor& rSHDescriptor, const uint32 cSampleCountToConfigureWith, const int32 cMinSampleCountToRetrieve = -1)
                : m_SampleOrganizer(cSampleCountToConfigureWith, cMinSampleCountToRetrieve)
                , m_SHDescriptor(rSHDescriptor)
                , m_Size(0)
                , m_OrderedSize(0){}

            virtual ~CSampleGenerator(){}

            //!< generates cSampleCount SH samples according to policies, sets a new descriptor prior
            virtual void GenerateSamples(const uint32 cSampleCount, const SDescriptor& crNewDescriptor)
            {
                m_SHDescriptor = crNewDescriptor;
                GenerateSamples(cSampleCount);
            }

            //!< generates cSampleCount SH samples according to policies
            virtual void GenerateSamples(const uint32 cSampleCount)
            {
                uint32 sampleCount = cSampleCount;
                if (cSampleCount >= m_SampleOrganizer.Capacity())
                {
                    GetSHLog().LogWarning("Chosen Sample Generator configuration has maximum capacity of %d samples, clamping to this value\n", m_SampleOrganizer.Capacity());
                    sampleCount = m_SampleOrganizer.Capacity();
                }
                if (cSampleCount < m_SampleOrganizer.MinCapacity())
                {
                    GetSHLog().LogWarning("Chosen Sample Generator configuration has minimum capacity of %d samples, clamping to this value\n", m_SampleOrganizer.MinCapacity());
                    sampleCount = m_SampleOrganizer.MinCapacity();
                }
                if (m_OrderedSize != 0 && m_Size != 0)
                {
                    //perform a restart, it has had already been initialized
                    Restart(sampleCount, m_SHDescriptor);
                    return;
                }

                TRandomSampleVec randomSamples;
                //srand( (unsigned)time( NULL ) );
                m_SampleGenerator.GenerateSamples(randomSamples, sampleCount);
                assert(!randomSamples.empty());
                m_Size = m_SampleOrganizer.ConvertIntoSHAndReOrganize(&randomSamples[0], (uint32)randomSamples.size(), m_SHDescriptor);
                m_OrderedSize = sampleCount;
            }

            virtual const uint32 Size() const                               //!< retrieves the number of constructed samples
            {
                return m_Size;
            }

            virtual const uint32 OrderedSize()  const                               //!< retrieves the number of ordered constructed samples(to avoid restarts)
            {
                return m_OrderedSize;
            }

            //!< retrieves all samples
            virtual void GetSamples(TScalarVecVec& rSampleVector) const
            {
                m_SampleOrganizer.GetSamples(rSampleVector);
            }

            virtual const SDescriptor& SHDescriptor() const{return m_SHDescriptor; }

            //!< resets and restarts sample ,generator
            virtual void Restart(const uint32 cSampleCount, const SDescriptor& crDescriptor)
            {
                if (cSampleCount != m_OrderedSize || crDescriptor.Bands != m_SHDescriptor.Bands)
                {
                    m_SHDescriptor = crDescriptor;
                    m_SampleGenerator.Reset();
                    m_SampleOrganizer.Reset();
                    m_OrderedSize = m_Size = 0;
                    GenerateSamples(cSampleCount);
                }
            }

            virtual const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& ReturnSampleFromHandle(const TSampleHandle& crHandle) const
            {
                return m_SampleOrganizer.GetSample(crHandle);
            }

        private:
            CSampleOrganizer                m_SampleOrganizer;                          //!< the sample organizer
            CSampleGenPolicy                m_SampleGenerator;                          //!< the sample generator
            SDescriptor                         m_SHDescriptor;                                 //!< the SH descriptor
            uint32                                  m_Size;                                                 //!< number of generated samples
            uint32                                  m_OrderedSize;                                  //!< number of ordered generated samples, to avoid double restarts
        };
    }
}
#pragma warning (default : 4512)








#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SAMPLEGENERATOR_H
