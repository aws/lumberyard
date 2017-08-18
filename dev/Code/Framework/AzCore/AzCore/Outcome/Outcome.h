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

#include "AzCore/Outcome/Internal/OutcomeStorage.h"

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    // Outcome

    /**
     * Outcome is intended for use as the return type of a function that may fail.
     * A successful outcome contains the desired value, while a failed outcome contains an error.
     * Either the value exists or the error exists, the other type is never even constructed.
     *
     * Check the outcome's IsSuccess() before accessing its contents via GetValue() or GetError().
     *
     * Outcome performs no dynamic allocations and holds the
     * value or error type within its own memory footprint.
     *
     * \param ValueT    The value type contained in a successful outcome.
     *                  void is an acceptable value type.
     * \param ErrorT    The error type contained in a failed outcome.
     *                  void is an acceptable error type.
     *
     * An outcome is constructed with the aid of helper functions
     * Success<ValueT>(ValueT&& value) and Failure<ErrorT>(ErrorT&& error):
     * \code
     * AZ::Outcome<int, const char*> successfulOutcome = AZ::Success(9);
     * AZ::Outcome<int, const char*> failedOutcome     = AZ::Failure("can't do that");
     * \endcode
     *
     * Example Usage:
     * \code
     *
     * // Returning AZ::Outcome from a function...
     * // If successful, outcome contains valid FILE*.
     * // If failed, outcome contains an int corresponding to the system-specific error.
     * AZ::Outcome<FILE*, int> FileOpen(const char* filepath, const char* mode)
     * {
     *     FILE* file = fopen(filepath, mode);
     *     if (file)
     *     {
     *         // Returning successful outcome
     *         return AZ::Success(file);
     *     }
     *     else
     *     {
     *         // Returning failed outcome
     *         return AZ::Failure(errno);
     *     }
     * }
     *
     * // Receiving AZ::Outcome from a function...
     * void ContrivedExample()
     * {
     *     AZ::Outcome<FILE*, int> fileOutcome = FileOpen("contrived.txt", "r");
     *     if (fileOutcome.IsSuccess())
     *     {
     *         // when IsSuccess() == true, outcome contains value
     *         DoSomethingWithFile(fileOutcome.GetValue());
     *     }
     *     else
     *     {
     *         // when IsSuccess() == false, outcome contains error
     *         Log("FileOpen failed (error %d): %s", fileOutcome.GetError(), filepath);
     *     }
     * }
     *
     * \endcode
     */
    template <class ValueT, class ErrorT = void>
    class Outcome
    {
    public:
        using ValueType = ValueT;
        using ErrorType = ErrorT;

    private:
        using SuccessType = SuccessValue<ValueType>;
        using FailureType = FailureValue<ErrorType>;

    public:
        //! Default construction is not allowed; Outcome must be either in success state or failure state
        Outcome() = delete;

        //! Constructs successful outcome, where value is copy-constructed.
        AZ_FORCE_INLINE Outcome(const SuccessType& success);

#ifdef AZ_HAS_RVALUE_REFS
        //! Constructs successful outcome, where value is move-constructed.
        AZ_FORCE_INLINE Outcome(SuccessType&& success);
#endif // AZ_HAS_RVALUE_REFS

        //! Constructs failed outcome, where error is copy-constructed.
        AZ_FORCE_INLINE Outcome(const FailureType& failure);

#ifdef AZ_HAS_RVALUE_REFS
        //! Constructs failed outcome, where error is move-constructed.
        AZ_FORCE_INLINE Outcome(FailureType&& failure);
#endif // AZ_HAS_RVALUE_REFS

        //! Copy constructor.
        AZ_FORCE_INLINE Outcome(const Outcome& other);

#ifdef AZ_HAS_RVALUE_REFS
        //! Move constructor.
        AZ_FORCE_INLINE Outcome(Outcome&& other);
#endif // AZ_HAS_RVALUE_REFS

        AZ_FORCE_INLINE ~Outcome();

        //! Copy-assignment from other outcome.
        AZ_FORCE_INLINE Outcome& operator=(const Outcome& other);

        //! Copy value into outcome. Outcome is now successful.
        AZ_FORCE_INLINE Outcome& operator=(const SuccessType& success);

        //! Copy error into outcome. Outcome is now failed.
        AZ_FORCE_INLINE Outcome& operator=(const FailureType& failure);

#ifdef AZ_HAS_RVALUE_REFS
        //! Move-assignment from other outcome.
        AZ_FORCE_INLINE Outcome& operator=(Outcome&& other);

        //! Move value into outcome. Outcome is now successful.
        AZ_FORCE_INLINE Outcome& operator=(SuccessType&& success);

        //! Move error into outcome. Outcome is now failed.
        AZ_FORCE_INLINE Outcome& operator=(FailureType&& failure);
#endif // AZ_HAS_RVALUE_REFS

        //! Returns whether outcome was a success, containing a valid value.
        AZ_FORCE_INLINE bool IsSuccess() const;
        AZ_FORCE_INLINE explicit operator bool() const;

        //! Returns value from successful outcome.
        //! Behavior is undefined if outcome was a failure.
        template <class Value_Type = ValueType, class = Internal::enable_if_not_void<Value_Type> >
        AZ_FORCE_INLINE Value_Type& GetValue();

        template <class Value_Type = ValueType, class = Internal::enable_if_not_void<Value_Type> >
        AZ_FORCE_INLINE const Value_Type& GetValue() const;

        //! Returns value from successful outcome as rvalue reference.
        //! Note that outcome's value may have its contents stolen,
        //! rendering it invalid for further access.
        //! Behavior is undefined if outcome was a failure.
#ifdef AZ_HAS_RVALUE_REFS
        template <class Value_Type = ValueType, class = Internal::enable_if_not_void<Value_Type> >
        AZ_FORCE_INLINE Value_Type && TakeValue();
#endif // AZ_HAS_RVALUE_REFS

        //! Returns value from successful outcome.
        //! defaultValue is returned if outcome was a failure.
#ifdef AZ_HAS_RVALUE_REFS
        template <class U, class Value_Type = ValueType, class = Internal::enable_if_not_void<Value_Type> >
        AZ_FORCE_INLINE Value_Type GetValueOr(U&& defaultValue) const;
#else
        template <class Value_Type = ValueType, class = Internal::enable_if_not_void<Value_Type> >
        AZ_FORCE_INLINE Value_Type GetValueOr(const ValueType& defaultValue) const;
#endif // AZ_HAS_RVALUE_REFS

        //! Returns error for failed outcome.
        //! Behavior is undefined if outcome was a success.
        template <class Error_Type = ErrorType, class = Internal::enable_if_not_void<Error_Type> >
        AZ_FORCE_INLINE Error_Type& GetError();

        template <class Error_Type = ErrorType, class = Internal::enable_if_not_void<Error_Type> >
        AZ_FORCE_INLINE const Error_Type& GetError() const;

        //! Returns error for failed outcome as rvalue reference.
        //! Note that outcome's error may have its contents stolen,
        //! rendering it invalid for further access.
        //! Behavior is undefined if outcome was a success.
#ifdef AZ_HAS_RVALUE_REFS
        template <class Error_Type = ErrorType, class = Internal::enable_if_not_void<Error_Type> >
        AZ_FORCE_INLINE Error_Type && TakeError();
#endif // AZ_HAS_RVALUE_REFS

    private:

        //! Return m_success  as a SuccessType.
        //! Behavior is undefined if outcome was a failure.
        SuccessType& GetSuccess();
        const SuccessType& GetSuccess() const;

        //! Return m_failure as a FailureType.
        //! Behavior is undefined if outcome was a success.
        FailureType& GetFailure();
        const FailureType& GetFailure() const;

        //! Run the appropriate SuccessType constructor on m_success.
        template<class ... ArgsT>
        void ConstructSuccess(ArgsT&& ... args);

        //! Run the appropriate FailureType constructor on m_failure.
        template<class ... ArgsT>
        void ConstructFailure(ArgsT&& ... args);

        // Memory for SuccessType and FailureType are stored within a union.
        // This reduces the size of Outcome and lets us prevent instantiation
        // of the unused type.
        union
        {
            typename AZStd::aligned_storage<
                sizeof(SuccessType),
                AZStd::alignment_of<SuccessType>::value
                >::type m_success;

            typename AZStd::aligned_storage<
                sizeof(FailureType),
                AZStd::alignment_of<FailureType>::value
                >::type m_failure;
        };

        bool m_isSuccess;
    };

    //////////////////////////////////////////////////////////////////////////
    // Success

#ifdef AZ_HAS_RVALUE_REFS
    /**
     * Used to return a success case in a function returning an AZ::Outcome<ValueT, ...>.
     * rhs is a universal reference: can either copy or move
     */
    template <class ValueT, class = Internal::enable_if_not_void<ValueT> >
    inline SuccessValue<ValueT> Success(ValueT&& rhs);
#else // AZ_HAS_RVALUE_REFS
      /**
       * Used to return a success case in a function returning an AZ::Outcome<ValueT, ...>.
       * Moving is disallowed, so we must copy rhs.
       */
    template <class ValueT, class = Internal::enable_if_not_void<ValueT> >
    inline SuccessValue<ValueT> Success(const ValueT& rhs);
#endif // AZ_HAS_RVALUE_REFS

    /**
     * Used to return a success case in a function returning an AZ::Outcome<void, ...>.
     */
    inline SuccessValue<void> Success();

    //////////////////////////////////////////////////////////////////////////
    // Failure

#ifdef AZ_HAS_RVALUE_REFS
    /**
     * Used to return a failure case in a function returning an AZ::Outcome<..., ValueT>.
     * rhs is a universal reference: can either copy or move
     */
    template <class ValueT, class = Internal::enable_if_not_void<ValueT> >
    inline FailureValue<ValueT> Failure(ValueT&& rhs);
#else // AZ_HAS_RVALUE_REFS
      /**
       * Used to return a failure case in a function returning an AZ::Outcome<..., ValueT>.
       * Moving is disallowed, so we must copy rhs.
       */
    template <class ValueT, class = Internal::enable_if_not_void<ValueT> >
    inline FailureValue<ValueT> Failure(const ValueT& rhs);
#endif // AZ_HAS_RVALUE_REFS

    /**
     * Used to return a failure case in a function returning an AZ::Outcome<..., void>.
     */
    inline FailureValue<void> Failure();
} // namespace AZ

#include "AzCore/Outcome/Internal/OutcomeImpl.h"
