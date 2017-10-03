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


// constructor from string
MCORE_INLINE UnicodeStringIterator::UnicodeStringIterator(const UnicodeString& str)
{
    InitFromString(str);
}


// constructor from data pointer
MCORE_INLINE UnicodeStringIterator::UnicodeStringIterator(const char* data, uint32 numCodeUnits)
{
    InitFromCharBuffer(data, numCodeUnits);
}

// constructor from data pointer
MCORE_INLINE UnicodeStringIterator::UnicodeStringIterator(const char* data)
{
    InitFromCharBuffer(data, (uint32)strlen(data));
}


// destructor
MCORE_INLINE UnicodeStringIterator::~UnicodeStringIterator()
{
}


// init from a string
MCORE_INLINE void UnicodeStringIterator::InitFromString(const UnicodeString& str)
{
    InitFromCharBuffer(str.AsChar(), str.GetLength());
}


// init from data pointer
MCORE_INLINE void UnicodeStringIterator::InitFromCharBuffer(const char* data, uint32 numCodeUnits)
{
    mData           = const_cast<char*>(data);
    mNumCodeUnits   = numCodeUnits;
    mCodeUnitIndex  = 0;
}


// get the number of code units
MCORE_INLINE uint32 UnicodeStringIterator::GetLength() const
{
    return mNumCodeUnits;
}


// have we reached the end of the string?
MCORE_INLINE bool UnicodeStringIterator::GetHasReachedEnd() const
{
    return (mCodeUnitIndex >= mNumCodeUnits);
}


// get the current code unit index
MCORE_INLINE uint32 UnicodeStringIterator::GetIndex() const
{
    return mCodeUnitIndex;
}
