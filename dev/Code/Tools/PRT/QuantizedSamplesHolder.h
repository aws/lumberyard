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


// Description : quantized sample holder (extra class necessary to be able to use 
//               vectors over interfaces)

#ifndef CRYINCLUDE_TOOLS_PRT_QUANTIZEDSAMPLESHOLDER_H
#define CRYINCLUDE_TOOLS_PRT_QUANTIZEDSAMPLESHOLDER_H
#pragma once

#include "PRTTypes.h"


namespace NSH
{
	class CQuantizedSamplesHolder;

	//!< helper class acting like a vector
	//!< to be able to cope with different STLs, vector has been replaced by this
	struct SDiscretizationElem
	{
	private:
		NSH::TSample *pElems;	//elements
		uint32 count;					//currently set elements
		uint32 allocCount;		//max allocatable elements

	public:
		SDiscretizationElem();
		~SDiscretizationElem();
		void Allocate(const uint32 cCount);
		void push_back(const NSH::TSample& crSample);
		const size_t size() const;
		NSH::TSample& operator[](const size_t cIndex);
		const NSH::TSample& operator[](const size_t cIndex) const;

		friend class CQuantizedSamplesHolder;
	};

	class CQuantizedSamplesHolder
	{
	public:
		//!< constructor with number of discretizations
		CQuantizedSamplesHolder(const uint32 cDiscretizations);
		~CQuantizedSamplesHolder();
		//!< access operators retrieving vector of samples for one particular discretization(step)
		SDiscretizationElem& operator[](const size_t cIndex);
		const SDiscretizationElem& operator[](const size_t cIndex) const;
		//!< returns number of discretizations
		const size_t Discretizations() const;
		//!< returns number of samples for a particular discretization
		const size_t GetDiscretizationSampleCount(const size_t cDiscretizationIndex) const;
		//!< returns sample for a particular discretization
		const NSH::TSample& GetDiscretizationSample(const size_t cSampleIndex, const size_t cDiscretizationIndex) const;

	private:
		SDiscretizationElem *m_pDiscretizations;
		uint32 m_Discretizations;
	};
}



#include "QuantizedSamplesHolder.inl"

#endif // CRYINCLUDE_TOOLS_PRT_QUANTIZEDSAMPLESHOLDER_H
