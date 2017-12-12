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

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "Matrix4.h"


namespace MCore
{
    // TODO: - Add Cholesky decomposition support
    //       - Add QR decomposition
    //       - Add Pseudo Inverse support
    //       - Add some helper functions, like IsDiagonal, IsSquare, etc.
    //       - Add more operators for multiplying with scalars, adding, subtracting, etc
    //       - Use a matrix cache for more efficient matrix data allocation

    /**
     * A generic (m x n) matrix class stored in row major order.
     * More advanced algorithms often use matrices that are bigger than just 4x4.
     * Often the dimensions of the required matrices depend on the input parameters of such algorithms.
     * This is why we need a powerful generic matrix class that can handle any size of matrix.
     * This class however is much slower than the specialized Matrix (4x4) class. So if you are dealing with 4x4 matrices
     * which is the case for most 3D transformations, use that class instead of this one.
     */
    class MCORE_API NMatrix
    {
    public:
        /**
         * The default constructor.
         * The matrix is not usable before you use the SetDimensions method to setup the matrix size.
         */
        NMatrix();

        /**
         * Extended constructor.
         * This initializes the matrix with the given dimensions.
         * The values of the matrix elements will be uninitialized though, so they can contain random values, unless
         * you have set the identity flag to true.
         * @param numRows The number of rows of the matrix.
         * @param numColumns The number of columns in the matrix.
         * @param identity Set this to true when you like to initialize the matrix as an identity matrix. In this case
         *                 the number of rows must equal the number of columns, so the matrix has to be square. If thats not the case
         *                 then no identity operation will be performed and the matrix element values will remain uninitialized.
         */
        NMatrix(uint32 numRows, uint32 numColumns, bool identity = false);

        /**
         * Constructor which converts from a 4x4 matrix.
         * This converts a 4x4 matrix into an NMatrix.
         * All elements are initialized as they are currently in the specified 4x4 matrix.
         * @param other The 4x4 matrix to convert into this NMatrix object.
         */
        NMatrix(const Matrix& other);

        /**
         * Copy constructor.
         * @param other The source matrix to copy from.
         */
        NMatrix(const NMatrix& other);

        /**
         * The destructor.
         * Automatically gets rid of allocated data.
         */
        ~NMatrix();

        /**
         * Set the matrix dimensions.
         * This performs the actual memory allocation for the matrix data.
         * It is possible to change the dimensions of the matrix at a later stage too.
         * @param numRows The number of rows to be used by the matrix.
         * @param numColumns The number of columns to be used by the matrix.
         */
        MCORE_INLINE void SetDimensions(uint32 numRows, uint32 numColumns);

        /**
         * Get a pointer to the matrix element data.
         * This points to the first element. The data is stored in row major order and the size (in bytes)
         * of the data block equals (GetNumRows() * GetNumColumns() * sizeof(float)).
         * @result A pointer to the matrix element data.
         */
        MCORE_INLINE float* GetData() const;

        /**
         * Get a matrix element value at a given row and column.
         * @param row The zero based row number index value.
         * @param column The zero based column number index value.
         * @result The value of the specified element.
         */
        MCORE_INLINE float Get(uint32 row, uint32 column) const;

        /**
         * Set the value of a given matrix element at a given row and column.
         * @param row The zero based row number index value.
         * @param column The zero based column number index value.
         * @param value The value to set the specified element to.
         */
        MCORE_INLINE void Set(uint32 row, uint32 column, float value);

        /**
         * Get the number of columns of this matrix.
         * @result The number of columns used by this matrix.
         */
        MCORE_INLINE uint32 GetNumColumns() const;

        /**
         * Get the number of rows of this matrix.
         * @result The number of rows used by this matrix.
         */
        MCORE_INLINE uint32 GetNumRows() const;

        /**
         * The operator to retrieve read and write access to a given matrix element.
         * For example you can use this operator like this:
         *
         * <pre>
         * NMatrix mat(3, 3);   // create a 3x3 matrix
         * mat(2, 1) = 10.0f;   // set the element at the third row and second column to a value of 10.
         * </pre>
         *
         * @param row The zero based row number index value.
         * @param column The zero based column number index value.
         * @result A reference (with write access) to the given element.
         */
        MCORE_INLINE float& operator () (uint32 row, uint32 column);

        /**
         * Operator to get access to a given row.
         * This can be used on a similar way than the operator ().
         *
         * <pre>
         * NMatrix mat(3, 3);   // create a 3x3 matrix
         * mat[2][1] = 10.0f;   // set the element at the third row and second column to a value of 10
         * </pre>
         *
         * @param row The zero based row number index value.
         * @result A pointer to the first element of the specified row.
         */
        MCORE_INLINE float* operator [] (int32 row);

        /**
         * Copy operator.
         * This copies a given matrix.
         * @param other The matrix to copy from.
         */
        MCORE_INLINE NMatrix& operator = (const NMatrix& other);

        /**
         * Post multiply this matrix with another matrix.
         * @param m The right hand side matrix to multiply with.
         * @result Returns this matrix, after multiplication with the specified matrix.
         */
        NMatrix& operator *= (const NMatrix& m);

        /**
         * Zero all the matrix elements.
         * This means that all elements inside the matrix will be set to a value of 0.
         */
        MCORE_INLINE void Zero();

        /**
         * Set the square matrix to an identity matrix.
         * Please keep in mind that the matrix has to be square, so its number of rows and columns must be equal!
         * An identity matrix contains all zero's, except for the diagonal elements, which contain a value of 1.
         */
        void Identity();

        /**
         * Debug print the matrix values using the log manager, using the LOG command.
         * This makes it very easy to dump the matrix values to a log file and/or debug output in Visual Studio.
         * @param description The description of this matrix, or nullptr when you don't want to name the matrix.
         */
        void Print(const char* description = nullptr);

        /**
         * Transpose the matrix.
         * This turns a (n x m) matrix into an (m x n) matrix, where all rows and columns will be swapped.
         */
        void Transpose();

        /**
         * Initialize this matrix to the transposed version of a specified matrix.
         * @param source The matrix which to initialize from. The current matrix will be the transposed version of this specified 'source' matrix.
         */
        void SetTransposed(const NMatrix& source);

        /**
         * Return the transposed version of this matrix.
         * This leaves the current matrix untouched.
         * @result The transposed version of this matrix.
         */
        MCORE_INLINE NMatrix Transposed() const;

        /**
         * Calculate the inverse of this matrix, using LU decomposition and backsubstitution.
         */
        void LUInverse();

        /**
         * Calculate the inverse of this matrix, using LU decomposition and backsubstitution.
         * This method leaves this matrix untouched.
         * @result The inversed version of this matrix.
         */
        MCORE_INLINE NMatrix LUInversed() const;

        /**
         * Calculate the inverse of this matrix. This currently is the same as LUInverse(), but might change to
         * another method in a later stage.
         */
        MCORE_INLINE void Inverse();

        /**
         * Calculate the inverse of this matrix, and leave this matrix untouched.
         * @result The inverse of this matrix.
         */
        MCORE_INLINE NMatrix Inversed() const;

        /**
         * Calculate the inverse of this matrix, using singular value decomposition (SVD).
         */
        void SVDInverse();

        /**
         * Calculate the inverse of this matrix, using singular value decomposition (SVD)
         * This method leaves this matrix untouched.
         * @result The inversed version of this matrix.
         */
        NMatrix SVDInversed() const;

        /**
         * Replace this square matrix with a LU decomposition matrix.
         * @param outIndices The array of index values that records the row permutation effected by partial pivoting.
         *                   The number of elements of this array will equal the number of rows/columns of this square matrix.
         * @param d The output value that contains +1 or -1, depending on whether the number of row interchanges was even or not.
         * @note This method is used in combination with LUBackSubstitution. Also keep in mind that the matrix has to be square!
         */
        void LUDecompose(Array<uint32>& outIndices, float& d);

        /**
         * Solves a set of linear equations Ax = b.
         * This matrix represents A, after LU decomposition (using the LUDecompose method).
         * @param indices The input array of indices that is created/returned by the LUDecompose method.
         * @param b The array that contains the augment matrix values. After executing this method the elements in this array
         *          will contain the solution for each unknown. So basically b will turn into x in the equation Ax = b.
         * @note If you are trying to solve a system of linear equations represented by your matrix, you can use the LUSolve method instead.
         */
        void LUBackSubstitution(const Array<uint32>& indices, Array<float>& b);

        /**
         * Calculate the determinant of the matrix.
         * @result The determinant of the matrix.
         */
        float CalcDeterminant() const;

        /**
         * Solve a system of linear equations stored in the current matrix, using LU decomposition and backsubstitution.
         * The system solved is: Ax = b. In this equation A represents this matrix, b represents the augment matrix, so the right hand side part of the equation.
         * The value x will be the solution.
         * @param b The input array of values of the augment matrix.
         * @param solution The output array that will contain the solution for each unknown.
         */
        void LUSolve(const Array<float>& b, Array<float>& solution) const;

        /**
         * Perform singular value decomposition (SVD).
         * In the equation Ax = b, this will decompose A into 3 matrices: U, W and V.
         * The matrix W is a diagonal matrix. The values of this diagonal are stored in the array w, which is one of the outputs of this method.
         * After calling this method, this matrix will respresnt the U matrix. So beware, this matrix will get modified by this operation.
         * @param w The array of floats that after execution of this method will contain the values of the diagonal of the W matrix.
         * @param v The output matrix that will contain the V matrix, after execution of this method.
         * @param rv1 A temp array. This one can be reused between different SVD calls, to prevent the SVD from doing an allocation internally each time.
         *            Instead, it will then reuse the temp array, so that only one alloc is done, and not one per call to this method.
         *            The array will be resized internally so it doesn't need to be of a given size.
         */
        void SVD(Array<float>& w, NMatrix& v, Array<float>& rv1);

        /**
         * Zero all tiny (near zero) values of the diagonal values of the W matrix created by SVD.
         * This will prevent unexpected behaviour due to floating point inaccuracy.
         * @param w The input values of the diagonal. These values will be modified when needed.
         */
        void SVDZeroTinyWValues(Array<float>& w);

        /**
         * Perform SVD back substitution to solve a set of linear equations (Ax = b).
         * This can be done after you called the method SVD.
         * @param w The diagonal values of the W matrix, as returned by the SVD method.
         * @param v The V matrix as returned by the SVD method.
         * @param b The input augment matrix, so the results of the equations.
         * @param x The solution, which contains the values for the unknowns.
         * @note If you are trying to solve a system of linear equations using SVD, you might want to use the SVDSolve method.
         */
        void SVDBackSubstitute(Array<float>& w, const NMatrix& v, const Array<float>& b, Array<float>& x);

        /**
         * Solve a system of linear equations using singular value decomposition (SVD).
         * The system looks like this: Ax = b.
         * @param b The augment matrix, so the result of the equation for each unknown.
         * @param solution The output array of floats, which will contain the solution (x in the equation).
         */
        void SVDSolve(const Array<float>& b, Array<float>& solution) const;

    private:
        float*  mData;          /**< The actual matrix data. */
        uint32  mNumRows;       /**< The number of rows. */
        uint32  mNumColumns;    /**< The number of columns. */

        /**
         * Function used by the SVD method.
         */
        MCORE_INLINE float Sign(const float& a, const float& b) const;

        /**
         * Function used by the SVD method.
         */
        float Pythag(float a, float b);
    };


    /**
     * Matrix multiplication operator.
     * The resulting matrix dimensions are: (n x m)(m x p) = (n x p).
     * So the dimensions of the resulting matrix are: (m1.GetNumRows() x m2.GetNumColumns()).
     * @param m1 The left hand side matrix.
     * @param m2 The right hand side matrix.
     * @result The result of the multiplication.
    */
    MCORE_API NMatrix operator * (const NMatrix& m1, const NMatrix& m2);
    MCORE_API NMatrix operator + (const NMatrix& m1, const NMatrix& m2);


    // include inline code
#include "NMatrix.inl"
} // namespace MCore
