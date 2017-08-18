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
#ifndef AZSTD_CHRONO_TYPES_H
#define AZSTD_CHRONO_TYPES_H

#include <AzCore/std/base.h>
#include <AzCore/std/ratio.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_floating_point.h>

#include <limits>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
// Needed for FLT_MAX
    #include <float.h>
#endif

// Fix for windows without NO_MIN_MAX define, or any other abuse of such basic keywords.
#if defined(AZ_COMPILER_MSVC)
#   ifdef min
#       pragma push_macro("min")
#       undef min
#       define AZ_WINDOWS_MIN_PUSHED
#   endif
#   ifdef max
#       pragma push_macro("max")
#       undef max
#       define AZ_WINDOWS_MAX_PUSHED
#   endif
#endif


namespace AZStd
{
    /**
     * IMPORTANT: This is not the full standard implementation, it is the same except for supported Rep types.
     * At the moment you can use only types "AZStd::sys_time_t" and "float" as duration Rep parameter. Any other type
     * will lead to either a compiler error or bad results.
     * Everything else should be up to standard. The reason for that is the use of common_type which requires compiler support or rtti.
     * We can't rely on any of those for the variety of compilers we use.
     */
    namespace chrono
    {
        class system_clock;
        /*
        * Based on 20.9.3.
        * A duration type measures time between two points in time (time_points). A duration has a representation
        * which holds a count of ticks and a tick period. The tick period is the amount of time which occurs from one
        * tick to the next, in units of seconds. It is expressed as a rational constant using the template ratio.
        *
        * Examples: duration<AZStd::time_t, ratio<60>> d0; // holds a count of minutes using a time_t
        * duration<AZStd::time_t, milli> d1; // holds a count of milliseconds using a time_t
        * duration<float, ratio<1, 30>> d2; // holds a count with a tick period of 1/30 of a second (30 Hz) using a float
        */
        template <class Rep, class Period = ratio<1> >
        class duration;

        template <class Clock = system_clock, class Duration = typename Clock::duration>
        class time_point;

        namespace Internal
        {
            //////////////////////////////////////////////////////////////////////////
            // Common type hack
            /**
             * We need compiler features to implement proper common type,
             * so we will just cover the types we use for chrono
             */
            template <class T, class U = void, class V = void>
            struct common_type
            {
            public:
                typedef typename common_type<typename common_type<T, U>::type, V>::type type;
            };

            template <class T>
            struct common_type<T, void, void>
            {
            public:
                typedef T type;
            };

            template <>
            struct common_type<AZStd::sys_time_t, AZStd::sys_time_t, void>
            {
                typedef AZStd::sys_time_t type;
            };
            template <class T>
            struct common_type<float, T, void>
            {
                typedef float type;
            };
            template <class T>
            struct common_type<T, float, void>
            {
                typedef float type;
            };
            template <class T>
            struct common_type<double, T, void>
            {
                typedef double type;
            };
            template <class T>
            struct common_type<T, double, void>
            {
                typedef double type;
            };

            template <class Rep1, class Period1, class Rep2, class Period2>
            struct common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >;

            template <class Clock, class Duration1, class Duration2>
            struct common_type<time_point<Clock, Duration1>, time_point<Clock, Duration2> >;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //
            template<class Rep>
            struct duration_value {};

            template<>
            struct duration_value<AZStd::sys_time_t>
            {
                static AZStd::sys_time_t zero() { return 0; }
                static AZStd::sys_time_t min()  { return 0; }
                static AZStd::sys_time_t max()
                {
                    return std::numeric_limits<AZStd::sys_time_t>::max();
                }
            };

            template<>
            struct duration_value<float>
            {
                static float zero() { return 0.0f; }
                static float min()  { return FLT_MAX; }
                static float max()  { return -FLT_MAX; }
            };
            //////////////////////////////////////////////////////////////////////////

            template <class T>
            struct is_duration
                : AZStd::false_type {};

            template <class Rep, class Period>
            struct is_duration<duration<Rep, Period> >
                : AZStd::true_type  {};

            // duration_cast

            // duration_cast is the heart of this whole prototype.  It can convert any
            //   duration to any other.  It is also (implicitly) used in converting
            //   time_points.  The conversion is always exact if possible.  And it is
            //   always as efficient as hand written code.  If different representations
            //   are involved, care is taken to never require implicit conversions.
            //   Instead static_cast is used explicitly for every required conversion.
            //   If there are a mixture of integral and floating point representations,
            //   the use of common_type ensures that the most logical "intermediate"
            //   representation is used.
            template <class FromDuration, class ToDuration,
                class Period = typename ratio_divide<typename FromDuration::period, typename ToDuration::period>::type, bool = (Period::num == 1), bool = (Period::den == 1)>
            struct duration_cast;

            // When the two periods are the same, all that is left to do is static_cast from
            //   the source representation to the target representation (which may be a no-op).
            //   This conversion is always exact as long as the static_cast from the source
            //   representation to the destination representation is exact.
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, true, true>
            {
                static ToDuration cast(const FromDuration& fd)
                {
                    return ToDuration(static_cast<typename ToDuration::rep>(fd.count()));
                }
            };

            // When the numerator of FromPeriod / ToPeriod is 1, then all we need to do is
            //   divide by the denominator of FromPeriod / ToPeriod.  The common_type of
            //   the two representations is used for the intermediate computation before
            //   static_cast'ing to the destination.
            //   This conversion is generally not exact because of the division (but could be
            //   if you get lucky on the run time value of fd.count()).
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, true, false>
            {
                static ToDuration cast(const FromDuration& fd)
                {
                    typedef typename common_type<typename ToDuration::rep, typename FromDuration::rep, AZStd::sys_time_t>::type C;
                    return ToDuration(static_cast<typename ToDuration::rep>(static_cast<C>(fd.count()) / static_cast<C>(Period::den)));
                }
            };

            // When the denomenator of FromPeriod / ToPeriod is 1, then all we need to do is
            //   multiply by the numerator of FromPeriod / ToPeriod.  The common_type of
            //   the two representations is used for the intermediate computation before
            //   static_cast'ing to the destination.
            //   This conversion is always exact as long as the static_cast's involved are exact.
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, false, true>
            {
                static ToDuration cast(const FromDuration& fd)
                {
                    typedef typename common_type<typename ToDuration::rep, typename FromDuration::rep, AZStd::sys_time_t>::type C;
                    return ToDuration(static_cast<typename ToDuration::rep>(static_cast<C>(fd.count()) * static_cast<C>(Period::num)));
                }
            };

            // When neither the numerator or denominator of FromPeriod / ToPeriod is 1, then we need to
            //   multiply by the numerator and divide by the denominator of FromPeriod / ToPeriod.  The
            //   common_type of the two representations is used for the intermediate computation before
            //   static_cast'ing to the destination.
            //   This conversion is generally not exact because of the division (but could be
            //   if you get lucky on the run time value of fd.count()).
            template <class FromDuration, class ToDuration, class Period>
            struct duration_cast<FromDuration, ToDuration, Period, false, false>
            {
                static ToDuration cast(const FromDuration& fd)
                {
                    typedef typename common_type<typename ToDuration::rep, typename FromDuration::rep, AZStd::sys_time_t>::type C;
                    return ToDuration(static_cast<typename ToDuration::rep>(static_cast<C>(fd.count()) * static_cast<C>(Period::num) / static_cast<C>(Period::den)));
                }
            };
        }

        //----------------------------------------------------------------------------//
        //      20.9.2.1 is_floating_point [time.traits.is_fp]                        //
        //      Probably should have been treat_as_floating_point. Editor notifed.    //
        //----------------------------------------------------------------------------//
        // Support bidirectional (non-exact) conversions for floating point rep types
        //   (or user defined rep types which specialize treat_as_floating_point).
        template <class Rep>
        struct treat_as_floating_point
            : AZStd::is_floating_point<Rep> {};

        //----------------------------------------------------------------------------//
        //      20.9.3.7 duration_cast [time.duration.cast]                           //
        //----------------------------------------------------------------------------//
        // Compile-time select the most efficient algorithm for the conversion...
        template <class ToDuration, class Rep, class Period>
        inline ToDuration duration_cast(const duration<Rep, Period>& fd)
        {
            return Internal::duration_cast<duration<Rep, Period>, ToDuration>::cast(fd);
        }

        //////////////////////////////////////////////////////////////////////////
        // Duration
        template <class Rep, class Period >
        class duration
        {
        public:
            typedef Rep     rep;
            typedef Period  period;

        public:
            // 20.9.3.1, construct/copy/destroy:
            AZ_FORCE_INLINE duration() {}
            template <class Rep2>
            explicit AZ_FORCE_INLINE duration(const Rep2& r)
                : m_rep(r) {}
            template <class Rep2, class Period2>
            duration(const duration<Rep2, Period2>& d)
                : m_rep(duration_cast<duration>(d).count())
            {}

            // 20.9.3.2, observer:
            rep count() const           { return m_rep; }
            // 20.9.3.3, arithmetic:
            duration operator+() const  { *this; }
            duration operator-() const  { return duration(-m_rep); }
            duration& operator++()      { ++m_rep; return *this; }
            duration operator++(int)    { return duration(m_rep++); }
            duration& operator--()      { --m_rep; return *this; }
            duration operator--(int)    { return duration(m_rep--); }
            duration& operator+=(const duration& d) { m_rep += d.count(); return *this; }
            duration& operator-=(const duration& d) { m_rep -= d.count(); return *this; }
            duration& operator*=(const rep& rhs)    { m_rep *= rhs; return *this; }
            duration  operator/(const rep& rhs)     { return duration(m_rep / rhs); }
            duration& operator/=(const rep& rhs)    { m_rep /= rhs; return *this; }
            // 20.9.3.4, special values:
            static const duration zero()            { return duration(Internal::duration_value<rep>::zero()); }
            static const duration min()             { return duration(Internal::duration_value<rep>::min()); }
            static const duration max()             { return duration(Internal::duration_value<rep>::max()); }
        private:
            rep         m_rep; // exposition only
        };
        //////////////////////////////////////////////////////////////////////////

        // convenience typedefs
        typedef duration<AZStd::sys_time_t, nano> nanoseconds;    // at least 64 bits needed
        typedef duration<AZStd::sys_time_t, micro> microseconds;  // at least 55 bits needed
        typedef duration<AZStd::sys_time_t, milli> milliseconds;  // at least 45 bits needed
        typedef duration<AZStd::sys_time_t> seconds;              // at least 35 bits needed
        typedef duration<AZStd::sys_time_t, ratio< 60> > minutes; // at least 29 bits needed
        typedef duration<AZStd::sys_time_t, ratio<3600> > hours;  // at least 23 bits needed

        //----------------------------------------------------------------------------//
        //      20.9.3.5 duration non-member arithmetic [time.duration.nonmember]     //
        //----------------------------------------------------------------------------//
        // Duration +
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE typename Internal::common_type< duration<Rep1, Period1>, duration<Rep2, Period2> >::type
        operator+(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
        {
            typename Internal::common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type result = lhs;
            result += rhs;
            return result;
        }

        // Duration -
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE typename Internal::common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type
        operator-(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
        {
            typename Internal::common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type result = lhs;
            result -= rhs;
            return result;
        }

        // Duration *
        template <class Rep1, class Period, class Rep2>
        AZ_FORCE_INLINE typename AZStd::Utils::enable_if_c
        <
            AZStd::is_convertible<Rep1, typename Internal::common_type<Rep1, Rep2>::type>::value
            && AZStd::is_convertible<Rep2, typename Internal::common_type<Rep1, Rep2>::type>::value,
            duration<typename Internal::common_type<Rep1, Rep2>::type, Period>
        >::type
        operator*(const duration<Rep1, Period>& d, const Rep2& s)
        {
            typedef typename Internal::common_type<Rep1, Rep2>::type CR;
            duration<CR, Period> r = d;
            r *= static_cast<CR>(s);
            return r;
        }

        template <class Rep1, class Period, class Rep2>
        AZ_FORCE_INLINE
        typename AZStd::Utils::enable_if_c
        <
            AZStd::is_convertible<Rep1, typename Internal::common_type<Rep1, Rep2>::type>::value
            && AZStd::is_convertible<Rep2, typename Internal::common_type<Rep1, Rep2>::type>::value,
            duration<typename Internal::common_type<Rep1, Rep2>::type, Period>
        >::type
        operator*(const Rep1& s, const duration<Rep2, Period>& d)
        {
            return d * s;
        }

        // Duration /
        namespace Internal
        {
            template <class Duration, class Rep, bool = is_duration<Rep>::value>
            struct duration_divide_result
            {};

            template <class Duration, class Rep2,
                bool = (AZStd::is_convertible<typename Duration::rep, typename common_type<typename Duration::rep, Rep2>::type>::value
                        && AZStd::is_convertible<Rep2, typename common_type<typename Duration::rep, Rep2>::type>::value) >
            struct duration_divide_imp
            {};

            template <class Rep1, class Period, class Rep2>
            struct duration_divide_imp<duration<Rep1, Period>, Rep2, true>
            {
                typedef duration<typename common_type<Rep1, Rep2>::type, Period> type;
            };

            template <class Rep1, class Period, class Rep2>
            struct duration_divide_result<duration<Rep1, Period>, Rep2, false>
                : duration_divide_imp<duration<Rep1, Period>, Rep2>
            {};
        }

        template <class Rep1, class Period, class Rep2>
        AZ_FORCE_INLINE typename Internal::duration_divide_result<duration<Rep1, Period>, Rep2>::type
        operator/(const duration<Rep1, Period>& d, const Rep2& s)
        {
            typedef typename Internal::common_type<Rep1, Rep2>::type CR;
            duration<CR, Period> r = d;
            r /= static_cast<CR>(s);
            return r;
        }

        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE typename Internal::common_type<Rep1, Rep2>::type
        operator/(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
        {
            typedef typename Internal::common_type<duration<Rep1, Period1>, duration<Rep2, Period2> >::type CD;
            return CD(lhs).count() / CD(rhs).count();
        }

        //----------------------------------------------------------------------------//
        //      20.9.3.6 duration comparisons [time.duration.comparisons]             //
        //----------------------------------------------------------------------------//
        namespace Internal
        {
            template <class LhsDuration, class RhsDuration>
            struct duration_eq
            {
                bool operator()(const LhsDuration& lhs, const RhsDuration& rhs)
                {
                    typedef typename common_type<LhsDuration, RhsDuration>::type CD;
                    return CD(lhs).count() == CD(rhs).count();
                }
            };

            template <class LhsDuration>
            struct duration_eq<LhsDuration, LhsDuration>
            {
                bool operator()(const LhsDuration& lhs, const LhsDuration& rhs)
                {return lhs.count() == rhs.count(); }
            };

            template <class LhsDuration, class RhsDuration>
            struct duration_lt
            {
                bool operator()(const LhsDuration& lhs, const RhsDuration& rhs)
                {
                    typedef typename common_type<LhsDuration, RhsDuration>::type CD;
                    return CD(lhs).count() < CD(rhs).count();
                }
            };

            template <class LhsDuration>
            struct duration_lt<LhsDuration, LhsDuration>
            {
                bool operator()(const LhsDuration& lhs, const LhsDuration& rhs)
                {return lhs.count() < rhs.count(); }
            };
        }

        // Duration ==
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE bool
        operator==(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
        {
            return Internal::duration_eq<duration<Rep1, Period1>, duration<Rep2, Period2> >()(lhs, rhs);
        }

        // Duration !=
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE bool
        operator!=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)      { return !(lhs == rhs); }

        // Duration <
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE bool
        operator< (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)
        {
            return Internal::duration_lt<duration<Rep1, Period1>, duration<Rep2, Period2> >()(lhs, rhs);
        }

        // Duration >
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE bool
        operator> (const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)      { return rhs < lhs; }

        // Duration <=
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE bool
        operator<=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)      { return !(rhs < lhs); }

        // Duration >=
        template <class Rep1, class Period1, class Rep2, class Period2>
        AZ_FORCE_INLINE bool
        operator>=(const duration<Rep1, Period1>& lhs, const duration<Rep2, Period2>& rhs)      {   return !(lhs < rhs); }

        //////////////////////////////////////////////////////////////////////////
        // Time point

        /**
         *  20.9.4 Class template time_point [time.point]
         */
        template<class Clock, class Duration >
        class time_point
        {
        public:
            typedef Clock clock;
            typedef Duration duration;
            typedef typename duration::rep rep;
            typedef typename duration::period period;

        public:
            // 20.9.4.1, construct
            time_point()
                : m_d(duration::zero()) {}          // has value epoch
            explicit time_point(const duration& d)
                : m_d(d) {}
            template <class Duration2>
            time_point(const time_point<clock, Duration2>& t)
                : m_d(t.time_since_epoch()) {}
            // 20.9.4.2, observer:
            duration time_since_epoch() const   { return m_d; }
            // 20.9.4.3, arithmetic:
            time_point& operator+=(const duration& d) { m_d += d; return *this; }
            time_point& operator-=(const duration& d) { m_d -= d; return *this; }
            // 20.9.4.4, special values:
            static const time_point min()         { return time_point(duration::min()); }
            static const time_point max()         { return time_point(duration::max()); }
        private:
            duration m_d;
        };
        //////////////////////////////////////////////////////////////////////////

        //----------------------------------------------------------------------------//
        //      20.9.2.3 Specializations of common_type [time.traits.specializations] //
        //----------------------------------------------------------------------------//
        namespace Internal
        {
            template <class Rep1, class Period1, class Rep2, class Period2>
            struct common_type< duration<Rep1, Period1>, duration<Rep2, Period2> >
            {
                typedef duration<typename common_type<Rep1, Rep2>::type, typename AZStd::Internal::ratio_gcd<Period1, Period2>::type> type;
            };

            template <class Clock, class Duration1, class Duration2>
            struct common_type<time_point<Clock, Duration1>, time_point<Clock, Duration2> >
            {
                typedef time_point<Clock, typename common_type<Duration1, Duration2>::type> type;
            };
        }

        //----------------------------------------------------------------------------//
        //      20.9.4.5 time_point non-member arithmetic [time.point.nonmember]      //
        //----------------------------------------------------------------------------//
        // time_point operator+(time_point x, duration y);

        template <class Clock, class Duration1, class Rep2, class Period2>
        AZ_FORCE_INLINE time_point<Clock, typename Internal::common_type<Duration1, duration<Rep2, Period2> >::type>
        operator+(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs)
        {
            typedef time_point<Clock, typename Internal::common_type<Duration1, duration<Rep2, Period2> >::type> TimeResult;
            TimeResult r(lhs);
            r += rhs;
            return r;
        }

        // time_point operator+(duration x, time_point y);

        template <class Rep1, class Period1, class Clock, class Duration2>
        AZ_FORCE_INLINE time_point<Clock, typename Internal::common_type<duration<Rep1, Period1>, Duration2>::type>
        operator+(const duration<Rep1, Period1>& lhs, const time_point<Clock, Duration2>& rhs)  { return rhs + lhs; }

        // time_point operator-(time_point x, duration y);
        template <class Clock, class Duration1, class Rep2, class Period2>
        AZ_FORCE_INLINE time_point<Clock, typename Internal::common_type<Duration1, duration<Rep2, Period2> >::type>
        operator-(const time_point<Clock, Duration1>& lhs, const duration<Rep2, Period2>& rhs)  { return lhs + (-rhs); }

        // duration operator-(time_point x, time_point y);
        template <class Clock, class Duration1, class Duration2>
        AZ_FORCE_INLINE typename Internal::common_type<Duration1, Duration2>::type
        operator-(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs) { return lhs.time_since_epoch() - rhs.time_since_epoch(); }

        //----------------------------------------------------------------------------//
        //      20.9.4.6 time_point comparisons [time.point.comparisons]              //
        //----------------------------------------------------------------------------//
        // time_point ==
        template <class Clock, class Duration1, class Duration2>
        AZ_FORCE_INLINE bool
        operator==(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)
        {
            return lhs.time_since_epoch() == rhs.time_since_epoch();
        }

        // time_point !=
        template <class Clock, class Duration1, class Duration2>
        AZ_FORCE_INLINE bool
        operator!=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)    { return !(lhs == rhs); }

        // time_point <
        template <class Clock, class Duration1, class Duration2>
        AZ_FORCE_INLINE bool
        operator<(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)
        {
            return lhs.time_since_epoch() < rhs.time_since_epoch();
        }

        // time_point >
        template <class Clock, class Duration1, class Duration2>
        AZ_FORCE_INLINE bool
        operator>(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)     { return rhs < lhs; }

        // time_point <=
        template <class Clock, class Duration1, class Duration2>
        AZ_FORCE_INLINE bool
        operator<=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)    { return !(rhs < lhs);  }

        // time_point >=
        template <class Clock, class Duration1, class Duration2>
        AZ_FORCE_INLINE bool
        operator>=(const time_point<Clock, Duration1>& lhs, const time_point<Clock, Duration2>& rhs)    { return !(lhs < rhs);  }

        //----------------------------------------------------------------------------//
        //      20.9.4.7 time_point_cast [time.point.cast]                            //
        //----------------------------------------------------------------------------//
        template <class ToDuration, class Clock, class Duration>
        inline time_point<Clock, ToDuration> time_point_cast(const time_point<Clock, Duration>& t)
        {
            return time_point<Clock, ToDuration>(duration_cast<ToDuration>(t.time_since_epoch()));
        }
    }
}

#ifdef AZ_WINDOWS_MIN_PUSHED
#   pragma pop_macro("min")
#   undef AZ_WINDOWS_MIN_PUSHED
#endif
#ifdef AZ_WINDOWS_MAX_PUSHED
#   pragma pop_macro("max")
#   undef AZ_WINDOWS_MAX_PUSHED
#endif

#endif // AZSTD_CHRONO_TYPES_H
#pragma once