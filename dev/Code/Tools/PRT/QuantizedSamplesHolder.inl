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
	quantized sample holder inline implementations
*/

inline NSH::SDiscretizationElem::SDiscretizationElem() : pElems(NULL), count(0), allocCount(0)
{}

inline void NSH::SDiscretizationElem::push_back(const NSH::TSample& crSample)
{
	assert(pElems);
	assert(count < allocCount);
	pElems[count++] = crSample;
}

inline void NSH::SDiscretizationElem::Allocate(const uint32 cCount)
{
	allocCount = cCount;
	assert(!pElems);
	pElems = new NSH::TSample[cCount];
}

inline NSH::SDiscretizationElem::~SDiscretizationElem()
{
	delete [] pElems;
	pElems = NULL;
	count = allocCount = 0;
}

inline const size_t NSH::SDiscretizationElem::size() const
{
	return (size_t)count;
}

inline NSH::TSample& NSH::SDiscretizationElem::operator[](const size_t cIndex)
{
	assert(cIndex < count);
	return pElems[cIndex];
}

inline const NSH::TSample& NSH::SDiscretizationElem::operator[](const size_t cIndex) const
{
	assert(cIndex < count);
	return pElems[cIndex];
}

inline NSH::CQuantizedSamplesHolder::CQuantizedSamplesHolder(const uint32 cDiscretizations) : m_Discretizations(cDiscretizations)
{
	m_pDiscretizations = new NSH::SDiscretizationElem[m_Discretizations];
	for(int i=0; i<m_Discretizations; ++i)
		m_pDiscretizations[i].Allocate(m_Discretizations);
}

inline NSH::SDiscretizationElem& NSH::CQuantizedSamplesHolder::operator[](const size_t cIndex)
{
	assert(cIndex < m_Discretizations);
	return m_pDiscretizations[cIndex];
}

inline const NSH::SDiscretizationElem& NSH::CQuantizedSamplesHolder::operator[](const size_t cIndex) const
{
	assert(cIndex < m_Discretizations);
	return m_pDiscretizations[cIndex];
}

inline const size_t NSH::CQuantizedSamplesHolder::Discretizations() const
{
	return m_Discretizations;
}

inline const size_t NSH::CQuantizedSamplesHolder::GetDiscretizationSampleCount(const size_t cDiscretizationIndex) const
{
	assert(cDiscretizationIndex < m_Discretizations);
	return m_pDiscretizations[cDiscretizationIndex].size();
}

inline const NSH::TSample& NSH::CQuantizedSamplesHolder::GetDiscretizationSample(const size_t cSampleIndex, const size_t cDiscretizationIndex) const
{
	assert(cDiscretizationIndex < m_Discretizations);
	assert(cSampleIndex < m_pDiscretizations[cDiscretizationIndex].size());	
  return m_pDiscretizations[cDiscretizationIndex][cSampleIndex];
}

inline NSH::CQuantizedSamplesHolder::~CQuantizedSamplesHolder()
{
	delete [] m_pDiscretizations;
}