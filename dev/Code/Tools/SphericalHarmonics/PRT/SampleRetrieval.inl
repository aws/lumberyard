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

/*
    sample retrieval class implementation
*/

static const double gs_cRandomFactorThreshold = 0.0001;
#undef max
#undef min

template <class SampleCoeffType>
NSH::NSampleGen::CSampleRetrieval_tpl<SampleCoeffType>::CSampleRetrieval_tpl(vectorpvector<CSample_tpl<SCoeffList_tpl<SampleCoeffType> > >& rVectorVector, const int32 cTotalSampleCount)
    : m_rSampleData(rVectorVector)
    , m_AlreadyRetrieved(0)
    , m_Iterator(rVectorVector.begin())
{
    //initialize everything
    const int32 sampleDataSize = (int32)m_rSampleData.size();
    if (cTotalSampleCount == -1 || cTotalSampleCount >= sampleDataSize)
    {
        m_SampleCountToRetrieve = sampleDataSize;
        m_RetrievePartially = false;
    }
    else
    {
        m_RetrievePartially = true;
        m_SampleCountToRetrieve = cTotalSampleCount;
    }
    if (!m_RetrievePartially)
    {
        //will use iterator to retrieve samples
        return;
    }

    //init random generator
    TUint32Vec vectorIterationTable;//index table for vectorvector iteration indices
    if (m_SampleCountToRetrieve < (int32)rVectorVector.vector_size())
    {
        GetSHLog().LogWarning("Sample Retrieval: sample count: %d is less than numbers of organized bins: %d, optimal distribution not guaranteed\n", m_SampleCountToRetrieve, (int32)rVectorVector.vector_size());
        //less samples to retrieve than vector sets containing the samples, if we would just iterate it, a non equal distribution would be achieved
        //access randomly
        //generate random table for vectorvector entries to not iterate it linearly
        TUint32Vec remainingIndicesVector;  //remaining indices
        for (size_t i = 0; i < rVectorVector.vector_size(); ++i)
        {
            remainingIndicesVector.push_back(i);//insert existing index
        }
        double factor = (double)(remainingIndicesVector.size()) - gs_cRandomFactorThreshold;
        while (vectorIterationTable.size() < rVectorVector.vector_size() - 1)
        {
            //pick a random number and access remainingIndicesVector to access rVectorVector.vector_size(), store result into vectorIterationTable
            const uint32 elem = (uint32)(factor * Random());
            vectorIterationTable.push_back(remainingIndicesVector[elem]);
            remainingIndicesVector.erase(remainingIndicesVector.begin() + elem);
            factor -= 1.0;
        }
        //now add last element
        if (remainingIndicesVector.size() == 1)//supposed value
        {
            vectorIterationTable.push_back(remainingIndicesVector[0]);//add last value
        }
        else
        {
            assert(true);
        }
    }
    else
    {
        //just fill it with the indices
        for (int i = 0; i < (int32)rVectorVector.vector_size(); ++i)
        {
            vectorIterationTable.push_back(i);
        }
    }
    uint32 elemsPerVector = (uint32)std::max((uint32)1, (uint32)(m_SampleCountToRetrieve / (uint32)rVectorVector.vector_size()));   //compute equal number of samples to retrieve per vector
    assert(vectorIterationTable.size() == rVectorVector.vector_size());
    //generate random table
    const TUint32Vec::const_iterator cEnd = vectorIterationTable.end();
    for (TUint32Vec::const_iterator iter = vectorIterationTable.begin(); iter != cEnd; ++iter)
    {
        assert(rVectorVector[*iter].size() > 0);
        //generate set of samples for each vector element to have some kind of equal distribution
        //numbers to generate
        const uint32 cVecSize = (uint32)(rVectorVector[*iter].size());
        uint32 n = (uint32)std::min(cVecSize, elemsPerVector);
        const double factor = (double)cVecSize - gs_cRandomFactorThreshold;//to never reach n
        while (n > 0)
        {
            //generate element number
            const uint32 elem = (uint32)(factor * Random());
            assert(elem < cVecSize);
            std::pair<TUint32PairSet::iterator, bool> result = m_RandomTable.insert(std::pair<uint32, uint32>(*iter, elem));
            if (result.second)
            {
                n--;//successfully inserted element
            }
        }
    }
    //now insert completely random remaining elements
    if ((uint32)m_RandomTable.size() < (uint32)m_SampleCountToRetrieve)
    {
        uint32 n = (uint32)(m_SampleCountToRetrieve - m_RandomTable.size());//remaining elements to retrieve
        const double factor = (double)(rVectorVector.vector_size()) - gs_cRandomFactorThreshold;
        while (n > 0)
        {
            //first generate vector element number
            const uint32 elem = (uint32)(factor * Random());
            assert(elem < rVectorVector.vector_size());
            const double factor0 = (double)(rVectorVector[elem].size()) - gs_cRandomFactorThreshold;
            const uint32 vectorElem = (uint32)(factor0 * Random());
            assert(vectorElem < rVectorVector[elem].size());
            std::pair<TUint32PairSet::iterator, bool> result = m_RandomTable.insert(std::pair<uint32, uint32>(elem, vectorElem));
            if (result.second)
            {
                n--;//successfully inserted element
            }
        }
    }
    assert((elemsPerVector == 1 && (uint32)m_RandomTable.size() >= (uint32)m_SampleCountToRetrieve) || ((uint32)m_RandomTable.size() == (uint32)m_SampleCountToRetrieve));
    m_RandomTableIter = m_RandomTable.begin();
}

template <class SampleCoeffType>
inline void NSH::NSampleGen::CSampleRetrieval_tpl<SampleCoeffType>::ResetForNextVertex()
{
    m_RandomTableIter = m_RandomTable.begin();
    m_AlreadyRetrieved = 0;
    m_Iterator = m_rSampleData.begin();
}

template <class SampleCoeffType>
inline const uint32 NSH::NSampleGen::CSampleRetrieval_tpl<SampleCoeffType>::SampleCount() const
{
    return m_SampleCountToRetrieve;
}

template <class SampleCoeffType>
inline const bool NSH::NSampleGen::CSampleRetrieval_tpl<SampleCoeffType>::End() const
{
    return m_AlreadyRetrieved >= m_SampleCountToRetrieve;
}

template <class SampleCoeffType>
NSH::CSample_tpl<NSH::SCoeffList_tpl<SampleCoeffType> >& NSH::NSampleGen::CSampleRetrieval_tpl<SampleCoeffType>::Next()
{
    assert(m_AlreadyRetrieved < m_SampleCountToRetrieve);
    if (!m_RetrievePartially)
    {
        //no special constraints here, retrieve all samples
        NSH::CSample_tpl<NSH::SCoeffList_tpl<SampleCoeffType> >& rResult = *m_Iterator;
        m_AlreadyRetrieved++;
        if (m_AlreadyRetrieved < m_SampleCountToRetrieve)
        {
            ++m_Iterator;
        }
        return rResult;
    }
    //ok retrieve via some random distribution
    assert(m_RandomTableIter->first < m_rSampleData.size());
    assert(m_RandomTableIter->second < m_rSampleData[m_RandomTableIter->first].size());
    assert(m_RandomTableIter != m_RandomTable.end());

    NSH::CSample_tpl<NSH::SCoeffList_tpl<SampleCoeffType> >& rResult = (m_rSampleData[m_RandomTableIter->first])[m_RandomTableIter->second];
    m_AlreadyRetrieved++;
    if (m_AlreadyRetrieved < m_SampleCountToRetrieve)
    {
        ++m_RandomTableIter;
    }
    return rResult;
}
