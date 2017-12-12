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

// set the matrix dimensions
MCORE_INLINE void NMatrix::SetDimensions(uint32 numRows, uint32 numColumns)
{
    const uint32 numBytes       = numRows * numColumns * sizeof(float);
    const uint32 numCurBytes    = mNumRows * mNumColumns * sizeof(float);

    // update the dimensions
    mNumRows    = numRows;
    mNumColumns = numColumns;

    // allocate the matrix data
    if (numBytes != numCurBytes)
    {
        mData = (float*)MCore::Realloc(mData, numBytes, MCORE_MEMCATEGORY_MATRIX, MCORE_DEFAULT_ALIGNMENT); // TODO: use special matrix cache instead, and use another category
    }
}


MCORE_INLINE float* NMatrix::GetData() const
{
    return mData;
}


MCORE_INLINE float NMatrix::Get(uint32 row, uint32 column) const
{
    MCORE_ASSERT(row < mNumRows && column < mNumColumns);
    return mData[column + row * mNumColumns];
}


MCORE_INLINE void NMatrix::Set(uint32 row, uint32 column, float value)
{
    MCORE_ASSERT(row < mNumRows && column < mNumColumns);
    mData[column + row * mNumColumns] = value;
}


MCORE_INLINE uint32 NMatrix::GetNumColumns() const
{
    return mNumColumns;
}


MCORE_INLINE uint32 NMatrix::GetNumRows() const
{
    return mNumRows;
}


MCORE_INLINE float& NMatrix::operator () (uint32 row, uint32 column)
{
    MCORE_ASSERT(row < mNumRows && column < mNumColumns);
    return mData[column + row * mNumColumns];
}


MCORE_INLINE float* NMatrix::operator [] (int32 row)
{
    MCORE_ASSERT((uint32)row < mNumRows);
    return &mData[row * mNumColumns];
}


MCORE_INLINE NMatrix& NMatrix::operator = (const NMatrix& other)
{
    SetDimensions(other.mNumRows, other.mNumColumns);
    MCore::MemCopy(mData, other.mData, sizeof(float) * mNumColumns * mNumRows);
    return *this;
}


MCORE_INLINE void NMatrix::Zero()
{
    MCore::MemSet(mData, 0, mNumRows * mNumColumns * sizeof(float));
}


MCORE_INLINE float NMatrix::Sign(const float& a, const float& b) const
{
    return b >= 0 ? (a >= 0 ? a : -a) : (a >= 0 ? -a : a);
}


// matrix multiply
MCORE_INLINE NMatrix& NMatrix::operator *= (const NMatrix& m)
{
    *this = *this * m;
    return *this;
}


// return the inversed version of this matrix using the LU method
MCORE_INLINE NMatrix NMatrix::LUInversed() const
{
    NMatrix mat(*this);
    mat.LUInverse();
    return mat;
}


// return the inversed version of this matrix
MCORE_INLINE NMatrix NMatrix::Inversed() const
{
    NMatrix mat(*this);
    mat.Inverse();
    return mat;
}


// return the inversed version of this matrix
MCORE_INLINE void NMatrix::Inverse()
{
    LUInverse();
}


// return the inversed version of this matrix using SVD method
MCORE_INLINE NMatrix NMatrix::SVDInversed() const
{
    NMatrix mat(*this);
    mat.SVDInverse();
    return mat;
}


// return the tranposed version of this matrix
MCORE_INLINE NMatrix NMatrix::Transposed() const
{
    NMatrix mat(*this);
    mat.Transpose();
    return mat;
}
