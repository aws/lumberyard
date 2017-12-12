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
#ifndef AZCORE_MATH_MATRIX3X3_H
#define AZCORE_MATH_MATRIX3X3_H 1

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    /**
     * Matrix with 3 rows and 3 columns. May use less memory than a Transform, although probably won't on vector processors.
     * See Matrix4x4 for general information about matrices.
     */
    class Matrix3x3
    {
    public:
        AZ_TYPE_INFO(Matrix3x3, "{15A4332F-7C3F-4A58-AC35-50E1CE53FB9C}")

        //-------------------------------------
        // Constructors
        //-------------------------------------

        ///Default constructor does not initialize the matrix
        Matrix3x3()     { }

        //-----------------------------------------------------------
        // Static functions to initialize various types of matrices
        //-----------------------------------------------------------

        static const Matrix3x3 CreateIdentity();
        static const Matrix3x3 CreateZero();

        ///Constructs a matrix with all components set to the specified value
        static const Matrix3x3 CreateFromValue(const VectorFloat& value);

        ///Constructs from an array of 9 floats stored in row-major order
        static AZ_MATH_FORCE_INLINE const Matrix3x3 CreateFromRowMajorFloat9(const float* values) { return CreateFromRows(Vector3::CreateFromFloat3(&values[0]), Vector3::CreateFromFloat3(&values[3]), Vector3::CreateFromFloat3(&values[6])); }
        static AZ_MATH_FORCE_INLINE const Matrix3x3 CreateFromRows(const Vector3& row0, const Vector3& row1, const Vector3& row2) { Matrix3x3 m; m.SetRows(row0, row1, row2); return m; }

        ///Constructs from an array of 9 floats stored in column-major order
        static AZ_MATH_FORCE_INLINE const Matrix3x3 CreateFromColumnMajorFloat9(const float* values) { return CreateFromColumns(Vector3::CreateFromFloat3(&values[0]), Vector3::CreateFromFloat3(&values[3]), Vector3::CreateFromFloat3(&values[6])); }
        static AZ_MATH_FORCE_INLINE const Matrix3x3 CreateFromColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2) { Matrix3x3 m; m.SetColumns(col0, col1, col2); return m; }

        /**
         * Sets the matrix to be a rotation around a specified axis.
         */
        /*@{*/
        static const Matrix3x3 CreateRotationX(const VectorFloat& angle);
        static const Matrix3x3 CreateRotationY(const VectorFloat& angle);
        static const Matrix3x3 CreateRotationZ(const VectorFloat& angle);
        /*@}*/

        ///Sets the matrix from the upper 3x3 sub-matrix of a Transform
        static const Matrix3x3 CreateFromTransform(const Transform& t);

        ///Sets the matrix from the upper 3x3 sub-matrix of a Matrix4x4
        static const Matrix3x3 CreateFromMatrix4x4(const Matrix4x4& m);

        ///Sets the matrix from a quaternion
        static const Matrix3x3 CreateFromQuaternion(const Quaternion& q);

        ///Sets the matrix to be a scale matrix
        static AZ_MATH_FORCE_INLINE const Matrix3x3 CreateScale(const Vector3& scale) { return CreateDiagonal(scale); }

        ///Sets the matrix to be a diagonal matrix
        static const Matrix3x3 CreateDiagonal(const Vector3& diagonal);

        ///Creates the skew-symmetric cross product matrix, i.e. M*a=p.Cross(a)
        static const Matrix3x3 CreateCrossProduct(const Vector3& p);

        //===============================================================
        // Store
        //===============================================================

        ///Stores the matrix into to an array of 9 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        AZ_MATH_FORCE_INLINE void StoreToRowMajorFloat9(float* values) const
        {
            GetRow(0).StoreToFloat4(values);
            GetRow(1).StoreToFloat4(values + 3);
            GetRow(2).StoreToFloat3(values + 6);
        }

        ///Stores the matrix into to an array of 9 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        AZ_MATH_FORCE_INLINE void StoreToColumnMajorFloat9(float* values) const
        {
            GetColumn(0).StoreToFloat4(values);
            GetColumn(1).StoreToFloat4(values + 3);
            GetColumn(2).StoreToFloat3(values + 6);
        }

        //-------------------------------------
        // Accessors
        //-------------------------------------

        /**
         * Indexed accessor functions
         */
        /*@{*/
        VectorFloat GetElement(int row, int col) const;
        void SetElement(int row, int col, const VectorFloat& value);
        /*@}*/

        /**
         * Indexed access using operator()
         */
        AZ_MATH_FORCE_INLINE VectorFloat operator()(int row, int col) const { return GetElement(row, col); }

        /**
         * Row access functions
         */
        /*@{*/
        const Vector3 GetRow(int row) const;
        AZ_MATH_FORCE_INLINE void SetRow(int row, const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetRow(row, Vector3(x, y, z)); }
        void SetRow(int row, const Vector3& v);
        AZ_MATH_FORCE_INLINE void GetRows(Vector3* row0, Vector3* row1, Vector3* row2) const { *row0 = GetRow(0); *row1 = GetRow(1); *row2 = GetRow(2); }
        AZ_MATH_FORCE_INLINE void SetRows(const Vector3& row0, const Vector3& row1, const Vector3& row2) { SetRow(0, row0); SetRow(1, row1); SetRow(2, row2); }
        /*@}*/

        /**
         * Column access functions
         */
        /*@{*/
        const Vector3 GetColumn(int col) const;
        AZ_MATH_FORCE_INLINE void SetColumn(int col, const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(col, Vector3(x, y, z)); }
        void SetColumn(int col, const Vector3& v);
        AZ_MATH_FORCE_INLINE void GetColumns(Vector3* col0, Vector3* col1, Vector3* col2) const  { *col0 = GetColumn(0); *col1 = GetColumn(1); *col2 = GetColumn(2); }
        AZ_MATH_FORCE_INLINE void SetColumns(const Vector3& col0, const Vector3& col1, const Vector3& col2) { SetColumn(0, col0); SetColumn(1, col1); SetColumn(2, col2); }
        /*@}*/

        ///Basis (column) access functions
        /*@{*/
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisX() const { return GetColumn(0); }
        AZ_MATH_FORCE_INLINE void SetBasisX(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(0, x, y, z); }
        AZ_MATH_FORCE_INLINE void SetBasisX(const Vector3& v) { SetColumn(0, v); }
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisY() const { return GetColumn(1); }
        AZ_MATH_FORCE_INLINE void SetBasisY(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(1, x, y, z); }
        AZ_MATH_FORCE_INLINE void SetBasisY(const Vector3& v) { SetColumn(1, v); }
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisZ() const { return GetColumn(2); }
        AZ_MATH_FORCE_INLINE void SetBasisZ(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { SetColumn(2, x, y, z); }
        AZ_MATH_FORCE_INLINE void SetBasisZ(const Vector3& v) { SetColumn(2, v); }
        AZ_MATH_FORCE_INLINE void GetBasis(Vector3* basisX, Vector3* basisY, Vector3* basisZ) const { GetColumns(basisX, basisY, basisZ); }
        AZ_MATH_FORCE_INLINE void SetBasis(const Vector3& basisX, const Vector3& basisY, const Vector3& basisZ) { SetColumns(basisX, basisY, basisZ); }
        /*@}*/

        //---------------------------------------------
        // Multiplication operators and functions
        //---------------------------------------------

        const Matrix3x3 operator*(const Matrix3x3& rhs) const;

        ///Calculates (this->GetTranspose() * m)
        const Matrix3x3 TransposedMultiply(const Matrix3x3& m) const;

        /**
         * Post-multiplies the matrix by a vector.
         */
        const Vector3 operator*(const Vector3& rhs) const;

        const Matrix3x3 operator+(const Matrix3x3& rhs) const;
        const Matrix3x3 operator-(const Matrix3x3& rhs) const;

        const Matrix3x3 operator*(const VectorFloat& multiplier) const;
        const Matrix3x3 operator/(const VectorFloat& divisor) const;

        const Matrix3x3 operator-() const;

        AZ_MATH_FORCE_INLINE Matrix3x3& operator*=(const Matrix3x3& rhs) { *this = *this * rhs; return *this; }
        AZ_MATH_FORCE_INLINE Matrix3x3& operator+=(const Matrix3x3& rhs)    { *this = *this + rhs; return *this; }
        AZ_MATH_FORCE_INLINE Matrix3x3& operator-=(const Matrix3x3& rhs)    { *this = *this - rhs; return *this; }
        AZ_MATH_FORCE_INLINE Matrix3x3& operator*=(const VectorFloat& multiplier) { *this = *this * multiplier; return *this; }
        AZ_MATH_FORCE_INLINE Matrix3x3& operator/=(const VectorFloat& divisor)    { *this = *this / divisor; return *this; }

        //-------------------------------------------------------
        // Transposing
        //-------------------------------------------------------

        /**
         * Transpose calculation, i.e. flipping the rows and columns.
         */
        /*@{*/
        const Matrix3x3 GetTranspose() const;
        AZ_MATH_FORCE_INLINE void Transpose()           { *this = GetTranspose(); }
        /*@}*/

        //-------------------------------------------------------
        // Inverses
        //-------------------------------------------------------

        /**
         * Gets the inverse of the matrix. Use GetInverseFast instead of this if the matrix is orthogonal
         */
        /*@{*/
        const Matrix3x3 GetInverseFull() const;
        AZ_MATH_FORCE_INLINE void InvertFull()      { *this = GetInverseFull(); }
        /*@}*/

        /**
         * Fast inversion assumes the matrix is orthogonal
         */
        /*@{*/
        AZ_MATH_FORCE_INLINE const Matrix3x3 GetInverseFast() const { return GetTranspose(); }
        AZ_MATH_FORCE_INLINE void InvertFast()                      { *this = GetInverseFast(); }
        /*@}*/

        //-------------------------------------------------------
        // Scale access and decomposition
        //-------------------------------------------------------

        ///Gets the scale part of the transformation, i.e. the length of the scale components
        const Vector3 RetrieveScale() const;

        ///Gets the scale part of the transformation as in RetrieveScale, and also removes this scaling from the matrix
        const Vector3 ExtractScale();

        ///Quick multiplication by a scale matrix, equivalent to m*=Matrix3x3::CreateScale(scale)
        void MultiplyByScale(const Vector3& scale);

        ///Polar decomposition, M=U*H, U is orthogonal (unitary) and H is symmetric (hermitian), this function
        ///returns the orthogonal part only
        const Matrix3x3 GetPolarDecomposition() const;

        ///Polar decomposition, M=U*H, U is orthogonal (unitary) and H is symmetric (hermitian)
        void GetPolarDecomposition(Matrix3x3* orthogonalOut, Matrix3x3* symmetricOut) const;

        //-------------------------------------------------------
        // Orthogonality
        //-------------------------------------------------------

        bool IsOrthogonal(const VectorFloat& tolerance = g_simdTolerance) const;

        ///Adjusts an almost orthogonal matrix to be orthogonal
        const Matrix3x3 GetOrthogonalized() const;

        ///Adjusts an almost orthogonal matrix to be orthogonal
        AZ_MATH_FORCE_INLINE void Orthogonalize() { *this = GetOrthogonalized(); }


        //-------------------------------------------------------
        // Compare
        //-------------------------------------------------------

        bool IsClose(const Matrix3x3& rhs, const VectorFloat& tolerance = g_simdTolerance) const;

        bool operator==(const Matrix3x3& rhs) const;
        AZ_MATH_FORCE_INLINE bool operator!=(const Matrix3x3& rhs) const    { return !operator==(rhs); }

        //------------------------------------
        // Misc. stuff
        //------------------------------------

        void SetRotationPartFromQuaternion(const Quaternion& q);

        const Vector3 GetDiagonal() const;

        VectorFloat GetDeterminant() const;

        ///Also known as the adjoint, adjugate is the modern name which avoids confusion with the adjoint conjugate
        ///transpose. This is the transpose of the matrix of cofactors.
        const Matrix3x3 GetAdjugate() const;

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return GetRow(0).IsFinite() && GetRow(1).IsFinite() && GetRow(2).IsFinite(); }

    private:
        friend const Vector3 operator*(const Vector3& lhs, const Matrix3x3& rhs);
        friend const Matrix3x3 operator*(const VectorFloat& lhs, const Matrix3x3& rhs);

        #if defined(AZ_SIMD)
        SimdVectorType m_rows[3];
        #else
        //matrix elements, stored in COLUMN-MAJOR format
        AZ_ALIGN(float m_values[3][3], 16);
        #endif
    };

    /**
     * Pre-multiplies the matrix by a vector. Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Matrix3x3& rhs);

    /**
     * Pre-multiplies the matrix by a vector in-place. Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE Vector3& operator*=(Vector3& lhs, const Matrix3x3& rhs) { lhs = lhs * rhs; return lhs; }

    AZ_MATH_FORCE_INLINE const Matrix3x3 operator*(const VectorFloat& lhs, const Matrix3x3& rhs);
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Matrix3x3);
#endif

#if defined(AZ_SIMD_WINDOWS) || defined(AZ_SIMD_XBONE) || defined(AZ_SIMD_PS4) || defined(AZ_SIMD_LINUX) || defined(AZ_SIMD_APPLE_OSX)
    #include <AzCore/Math/Internal/Matrix3x3Win32.inl>
#else
    #include <AzCore/Math/Internal/Matrix3x3Fpu.inl>
#endif

#endif
#pragma once