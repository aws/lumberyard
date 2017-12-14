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
#ifndef AZCORE_MATH_MATRIX4X4_H
#define AZCORE_MATH_MATRIX4X4_H 1

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ
{
    /**
     * The general matrix class, stores all 4 rows and 4 columns, so can be used for all types of
     * transformations. If you don't need perspective transformations, consider using Transform instead,
     * if you don't need translation, consider using Matrix3x3 instead.
     *
     * When multiplying with a Vector3, it assumes the w-component of the Vector3 is 1.0. Use the
     * Multiply3x3 functions to multiply by the upper 3x3 submatrix only, e.g. for transforming
     * normals.
     */
    class Matrix4x4
    {
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Matrix4x4);
        AZ_TYPE_INFO(Matrix4x4, "{157193C7-B673-4A2B-8B43-5681DCC3DEC3}")

        //-------------------------------------
        // Constructors
        //-------------------------------------

        ///Default constructor does not initialize the matrix
        Matrix4x4()     { }

        //-----------------------------------------------------------
        // Static functions to initialize various types of matrices
        //-----------------------------------------------------------

        static const Matrix4x4 CreateIdentity();
        static const Matrix4x4 CreateZero();

        ///Constructs a matrix with all components set to the specified value
        static const Matrix4x4 CreateFromValue(const VectorFloat& value);

        ///Constructs from an array of 16 floats stored in row-major order
        static AZ_MATH_FORCE_INLINE const Matrix4x4 CreateFromRowMajorFloat16(const float* values) { return CreateFromRows(Vector4::CreateFromFloat4(values), Vector4::CreateFromFloat4(&values[4]), Vector4::CreateFromFloat4(&values[8]), Vector4::CreateFromFloat4(&values[12])); }
        static AZ_MATH_FORCE_INLINE const Matrix4x4 CreateFromRows(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3) { Matrix4x4 m; m.SetRows(row0, row1, row2, row3); return m; }

        ///Constructs from an array of 16 floats stored in column-major order
        static AZ_MATH_FORCE_INLINE const Matrix4x4 CreateFromColumnMajorFloat16(const float* values) { return CreateFromColumns(Vector4::CreateFromFloat4(values), Vector4::CreateFromFloat4(&values[4]), Vector4::CreateFromFloat4(&values[8]), Vector4::CreateFromFloat4(&values[12])); }
        static AZ_MATH_FORCE_INLINE const Matrix4x4 CreateFromColumns(const Vector4& col0, const Vector4& col1, const Vector4& col2, const Vector4& col3) { Matrix4x4 m; m.SetColumns(col0, col1, col2, col3); return m; }

        /**
         * Sets the matrix to be a rotation around a specified axis.
         */
        /*@{*/
        static const Matrix4x4 CreateRotationX(const VectorFloat& angle);
        static const Matrix4x4 CreateRotationY(const VectorFloat& angle);
        static const Matrix4x4 CreateRotationZ(const VectorFloat& angle);
        /*@}*/

        ///Sets the matrix from a quaternion, translation is set to zero
        static const Matrix4x4 CreateFromQuaternion(const Quaternion& q);

        ///Sets the matrix from a quaternion and a translation
        static const Matrix4x4 CreateFromQuaternionAndTranslation(const Quaternion& q, const Vector3& p);

        ///Sets the matrix to be a scale matrix
        static const Matrix4x4 CreateScale(const Vector3& scale);

        ///Sets the matrix to be a diagonal matrix
        static const Matrix4x4 CreateDiagonal(const Vector4& diagonal);

        ///Sets the matrix to be a translation matrix
        static const Matrix4x4 CreateTranslation(const Vector3& translation);

        /**
         * Creates a projection matrix using the vertical FOV and the aspect ratio, this is the preferred method which will avoid
         * unwanted stretching.
         */
        static const Matrix4x4 CreateProjection(float fovY, float aspectRatio, float nearDist, float farDist);

        /**
         * Creates a projection matrix using the vertical and horizontal FOV. Note that the relationship between FOV and aspect
         * ratio is not linear, so use the aspect ratio version unless you really know both desired FOVs.
         */
        static const Matrix4x4 CreateProjectionFov(float fovX, float fovY, float nearDist, float farDist);

        /**
         * Creates an off-center projection matrix
         */
        static const Matrix4x4 CreateProjectionOffset(float left, float right, float bottom, float top, float nearDist, float farDist);

        //===============================================================
        // Store
        //===============================================================

        ///Stores the matrix into to an array of 16 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        AZ_MATH_FORCE_INLINE void StoreToRowMajorFloat16(float* values) const
        {
            GetRow(0).StoreToFloat4(values);
            GetRow(1).StoreToFloat4(values + 4);
            GetRow(2).StoreToFloat4(values + 8);
            GetRow(3).StoreToFloat4(values + 12);
        }

        ///Stores the matrix into to an array of 16 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        AZ_MATH_FORCE_INLINE void StoreToColumnMajorFloat16(float* values) const
        {
            GetColumn(0).StoreToFloat4(values);
            GetColumn(1).StoreToFloat4(values + 4);
            GetColumn(2).StoreToFloat4(values + 8);
            GetColumn(3).StoreToFloat4(values + 12);
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
        const Vector4 GetRow(int row) const;
        const Vector3 GetRowAsVector3(int row) const;
        AZ_MATH_FORCE_INLINE void GetRows(Vector4* row0, Vector4* row1, Vector4* row2, Vector4* row3) const { *row0 = GetRow(0); *row1 = GetRow(1); *row2 = GetRow(2); *row3 = GetRow(3); }
        AZ_MATH_FORCE_INLINE void SetRow(int row, const VectorFloat& x,  const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)      { SetRow(row, Vector4(x, y, z, w)); }
        void SetRow(int row, const Vector3& v, const VectorFloat& w);
        void SetRow(int row, const Vector4& v);
        AZ_MATH_FORCE_INLINE void SetRows(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3) { SetRow(0, row0); SetRow(1, row1); SetRow(2, row2); SetRow(3, row3); }
        /*@}*/

        /**
         * Column access functions
         */
        /*@{*/
        const Vector4 GetColumn(int col) const;
        const Vector3 GetColumnAsVector3(int col) const;
        AZ_MATH_FORCE_INLINE void GetColumns(Vector4* col0, Vector4* col1, Vector4* col2, Vector4* col3) const { *col0 = GetColumn(0); *col1 = GetColumn(1); *col2 = GetColumn(2); *col3 = GetColumn(3); }
        AZ_MATH_FORCE_INLINE void SetColumn(int col, const VectorFloat& x,  const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)    { SetColumn(col, Vector4(x, y, z, w)); }
        void SetColumn(int col, const Vector3& v, const VectorFloat& w);
        void SetColumn(int col, const Vector4& v);
        AZ_MATH_FORCE_INLINE void SetColumns(const Vector4& col0, const Vector4& col1, const Vector4& col2, const Vector4& col3) { SetColumn(0, col0); SetColumn(1, col1); SetColumn(2, col2); SetColumn(3, col3); }
        /*@}*/

        ///Basis (column) access functions
        /*@{*/
        AZ_MATH_FORCE_INLINE const Vector4 GetBasisX() const            { return GetColumn(0); }
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisXAsVector3() const   { return GetColumnAsVector3(0); }
        AZ_MATH_FORCE_INLINE void SetBasisX(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w) { SetColumn(0, x, y, z, w); }
        AZ_MATH_FORCE_INLINE void SetBasisX(const Vector4& v)           { SetColumn(0, v); }
        AZ_MATH_FORCE_INLINE const Vector4 GetBasisY() const            { return GetColumn(1); }
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisYAsVector3() const   { return GetColumnAsVector3(1); }
        AZ_MATH_FORCE_INLINE void SetBasisY(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w) { SetColumn(1, x, y, z, w); }
        AZ_MATH_FORCE_INLINE void SetBasisY(const Vector4& v)           { SetColumn(1, v); }
        AZ_MATH_FORCE_INLINE const Vector4 GetBasisZ() const            { return GetColumn(2); }
        AZ_MATH_FORCE_INLINE const Vector3 GetBasisZAsVector3() const   { return GetColumnAsVector3(2); }
        AZ_MATH_FORCE_INLINE void SetBasisZ(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w) { SetColumn(2, x, y, z, w); }
        AZ_MATH_FORCE_INLINE void SetBasisZ(const Vector4& v)           { SetColumn(2, v); }
        AZ_MATH_FORCE_INLINE void GetBasisAndPosition(Vector4* basisX, Vector4* basisY, Vector4* basisZ, Vector4* pos) const { GetColumns(basisX, basisY, basisZ, pos); }
        AZ_MATH_FORCE_INLINE void SetBasisAndPosition(const Vector4& basisX, const Vector4& basisY, const Vector4& basisZ, const Vector4& pos) { SetColumns(basisX, basisY, basisZ, pos); }
        /*@}*/

        /**
         * Position (last column) access functions
         */
        /*@{*/
        const Vector3 GetPosition() const;
        AZ_MATH_FORCE_INLINE void SetPosition(const VectorFloat& x,  const VectorFloat& y, const VectorFloat& z)    { SetPosition(Vector3(x, y, z)); }
        void SetPosition(const Vector3& v);
        AZ_MATH_FORCE_INLINE const Vector3 GetTranslation() const           { return GetPosition(); }
        AZ_MATH_FORCE_INLINE void SetTranslation(const VectorFloat& x,  const VectorFloat& y, const VectorFloat& z) { SetPosition(x, y, z); }
        AZ_MATH_FORCE_INLINE void SetTranslation(const Vector3& v)          { SetPosition(v); }
        /*@}*/

        //---------------------------------------------
        // Multiplication operators and functions
        //---------------------------------------------

        const Matrix4x4 operator*(const Matrix4x4& rhs) const;

        AZ_MATH_FORCE_INLINE  Matrix4x4& operator*=(const Matrix4x4& rhs)  { *this = *this * rhs; return *this; }

        /**
         * Post-multiplies the matrix by a vector.  Assumes that the w-component of the Vector3 is 1.0.
         */
        const Vector3 operator*(const Vector3& rhs) const;

        /**
         * Post-multiplies the matrix by a vector.
         */
        const Vector4 operator*(const Vector4& rhs) const;

        /**
         * Pre-multiplies the matrix by a vector, using only the upper 3x3 submatrix. Note that this is NOT the
         * usual multiplication order for transformations.
         */
        const Vector3 TransposedMultiply3x3(const Vector3& v) const;

        /**
         * Post-multiplies the matrix by a vector, using only the upper 3x3 submatrix.
         */
        const Vector3 Multiply3x3(const Vector3& v) const;

        //-------------------------------------------------------
        // Transposing
        //-------------------------------------------------------

        /**
         * Transpose calculation, i.e. flipping the rows and columns.
         */
        /*@{*/
        const Matrix4x4 GetTranspose() const;
        AZ_MATH_FORCE_INLINE  void Transpose()          { *this = GetTranspose(); }
        /*@}*/

        //-------------------------------------------------------
        // Inverses
        //-------------------------------------------------------

        /**
         * Performs a full inversion for an arbitrary 4x4 matrix. Using GetInverse or GetFastInverse will often
         * be possible, use them in preference to this.
         */
        /*@{*/
        const Matrix4x4 GetInverseFull() const;
        AZ_MATH_FORCE_INLINE void  InvertFull()         { *this = GetInverseFull(); }
        /*@}*/

        /**
         * Gets the inverse of the matrix, assumes that the last row is (0,0,0,1), use GetInverseFull if this is
         * not true.
         */
        /*@{*/
        const Matrix4x4 GetInverseTransform() const;
        AZ_MATH_FORCE_INLINE void InvertTransform()     { *this = GetInverseTransform(); }
        /*@}*/

        /**
         * Fast inversion assumes the matrix consists of an upper 3x3 orthogonal matrix (i.e. a rotation)
         * and a translation in the last column
         */
        /*@{*/
        const Matrix4x4 GetInverseFast() const;
        AZ_MATH_FORCE_INLINE void InvertFast()          { *this = GetInverseFast(); }
        /*@}*/

        //-------------------------------------------------------
        // Scale access and decomposition
        //-------------------------------------------------------

        ///Gets the scale part of the transformation, i.e. the length of the scale components
        const Vector3 RetrieveScale() const;

        ///Gets the scale part of the transformation as in RetrieveScale, and also removes this scaling from the matrix
        const Vector3 ExtractScale();

        ///Quick multiplication by a scale matrix, equivalent to m*=Matrix4x4::CreateScale(scale)
        void MultiplyByScale(const Vector3& scale);

        //-------------------------------------------------------
        // Matrix interpolation
        //-------------------------------------------------------

        ///Interpolates between two matrices: linearly for scale/translation; spherically for rotation
        static const Matrix4x4 CreateInterpolated(const Matrix4x4& m1, const Matrix4x4& m2, float t);

        //-------------------------------------------------------
        // Compare
        //-------------------------------------------------------

        bool IsClose(const Matrix4x4& rhs, const VectorFloat& tolerance = g_simdTolerance) const;

        bool operator==(const Matrix4x4& rhs) const;
        AZ_MATH_FORCE_INLINE  bool operator!=(const Matrix4x4& rhs) const   { return !operator==(rhs); }

        //------------------------------------
        // Misc. stuff
        //------------------------------------

        ///sets the upper 3x3 rotation part of the matrix from a quaternion
        void SetRotationPartFromQuaternion(const Quaternion& q);

        const Vector4 GetDiagonal() const;

        AZ_MATH_FORCE_INLINE  bool IsFinite() const { return GetRow(0).IsFinite() && GetRow(1).IsFinite() && GetRow(2).IsFinite() && GetRow(3).IsFinite(); }

    private:
        friend class Matrix3x3;
        friend const Vector3 operator*(const Vector3& lhs, const Matrix4x4& rhs);
        friend const Vector4 operator*(const Vector4& lhs, const Matrix4x4& rhs);

        static const Matrix4x4 CreateProjectionInternal(float cotX, float cotY, float nearDist, float farDist);

        #if defined(AZ_SIMD)
        SimdVectorType m_rows[4];
        #else
        //matrix elements, stored in COLUMN-MAJOR format
        AZ_ALIGN(float m_values[4][4], 16);
        #endif
    };

    /**
     * Pre-multiplies the matrix by a vector. Assumes that the w-component of the Vector3 is 1.0, and returns the result as
     * a Vector3, without calculating the result w-component. Use the Vector4 version of this function to get the full result.
     * Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE const Vector3 operator*(const Vector3& lhs, const Matrix4x4& rhs);

    /**
     * Pre-multiplies the matrix by a vector in-place. Assumes that the w-component of the Vector3 is 1.0, and returns the result as
     * a Vector3, without calculating the result w-component. Use the Vector4 version of this function to get the full result.
     * Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE Vector3& operator*=(Vector3& lhs, const Matrix4x4& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }

    /**
     * Pre-multiplies the matrix by a vector. Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE const Vector4 operator*(const Vector4& lhs, const Matrix4x4& rhs);

    /**
     * Pre-multiplies the matrix by a vector in-place. Note that this is NOT the usual multiplication order for transformations.
     */
    AZ_MATH_FORCE_INLINE Vector4& operator*=(Vector4& lhs, const Matrix4x4& rhs)
    {
        lhs = lhs * rhs;
        return lhs;
    }
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Matrix4x4);
#endif

#if defined(AZ_SIMD_WINDOWS) || defined(AZ_SIMD_XBONE) || defined(AZ_SIMD_PS4) || defined(AZ_SIMD_LINUX) || defined(AZ_SIMD_APPLE_OSX)
    #include <AzCore/Math/Internal/Matrix4x4Win32.inl>
#else
    #include <AzCore/Math/Internal/Matrix4x4Fpu.inl>
#endif

#endif
#pragma once