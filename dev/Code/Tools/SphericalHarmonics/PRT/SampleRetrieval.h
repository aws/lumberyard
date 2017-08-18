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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SAMPLERETRIEVAL_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SAMPLERETRIEVAL_H
#pragma once

#include <PRT/PRTTypes.h>
#include <PRT/SHMath.h>
#include "BasicUtils.h"


namespace NSH
{
    template< class T >
    class vectorpvector;

    namespace NSampleGen
    {
        //!< sample retrieval class to just retrieve certain samples from a whole sample vector (to be able to process with less than all samples)
        template <class SampleCoeffType>
        class CSampleRetrieval_tpl
        {
        public:
            CSampleRetrieval_tpl(vectorpvector<CSample_tpl<SCoeffList_tpl<SampleCoeffType> > >& rVectorVector, const int32 cTotalSampleCount);
            CSample_tpl<SCoeffList_tpl<SampleCoeffType> >& Next();  //!< retrieves next sample
            void ResetForNextVertex();//!< resets for next sample retrieval
            const bool End() const;     //!< true if all samples have been retrieved
            const uint32 SampleCount() const;   //!< returns the number of samples to retrieve
        private:
            typename vectorpvector<CSample_tpl<SCoeffList_tpl<SampleCoeffType> > >::iterator m_Iterator;    //!< iterator used in case of full sample retrieval
            typedef std::set<TUint32Pair, std::less<TUint32Pair>, CSHAllocator<TUint32Pair> > TUint32PairSet;


            bool m_RetrievePartially;                   //!< indicates whether any partially retrieval should be performed
            int32 m_SampleCountToRetrieve;      //!< numbers to retrieve
            int32 m_AlreadyRetrieved;                   //!< counts the samples already been retrieved
            //specific members for partial retrieval
            TUint32PairSet  m_RandomTable;      //!< table generated in constructor containing each vector index and vector element member index
            TUint32PairSet::iterator    m_RandomTableIter;//!< iterator working on the random table

            vectorpvector<CSample_tpl<SCoeffList_tpl<SampleCoeffType> > >& m_rSampleData;

            CSampleRetrieval_tpl();                             //!< no default constructor available
            const CSampleRetrieval_tpl& operator=(const CSampleRetrieval_tpl&);//!< no assignment operator possible
        };
    }
}

#include "SampleRetrieval.inl"
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_SAMPLERETRIEVAL_H
