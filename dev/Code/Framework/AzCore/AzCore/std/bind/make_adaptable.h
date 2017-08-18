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
#ifndef AZSTD_BIND_MAKE_ADAPTABLE_HPP_INCLUDED
#define AZSTD_BIND_MAKE_ADAPTABLE_HPP_INCLUDED

// Based on boost 1.39.0
namespace boost
{
    namespace _bi
    {
        template<class R, class F>
        class af0
        {
        public:

            typedef R result_type;

            explicit af0(F f)
                : f_(f)
            {
            }

            result_type operator()()
            {
                return f_();
            }

            result_type operator()() const
            {
                return f_();
            }

        private:

            F f_;
        };

        template<class R, class A1, class F>
        class af1
        {
        public:

            typedef R result_type;
            typedef A1 argument_type;
            typedef A1 arg1_type;

            explicit af1(F f)
                : f_(f)
            {
            }

            result_type operator()(A1 a1)
            {
                return f_(a1);
            }

            result_type operator()(A1 a1) const
            {
                return f_(a1);
            }

        private:

            F f_;
        };

        template<class R, class A1, class A2, class F>
        class af2
        {
        public:

            typedef R result_type;
            typedef A1 first_argument_type;
            typedef A2 second_argument_type;
            typedef A1 arg1_type;
            typedef A2 arg2_type;

            explicit af2(F f)
                : f_(f)
            {
            }

            result_type operator()(A1 a1, A2 a2)
            {
                return f_(a1, a2);
            }

            result_type operator()(A1 a1, A2 a2) const
            {
                return f_(a1, a2);
            }

        private:

            F f_;
        };

        template<class R, class A1, class A2, class A3, class F>
        class af3
        {
        public:

            typedef R result_type;
            typedef A1 arg1_type;
            typedef A2 arg2_type;
            typedef A3 arg3_type;

            explicit af3(F f)
                : f_(f)
            {
            }

            result_type operator()(A1 a1, A2 a2, A3 a3)
            {
                return f_(a1, a2, a3);
            }

            result_type operator()(A1 a1, A2 a2, A3 a3) const
            {
                return f_(a1, a2, a3);
            }

        private:

            F f_;
        };

        template<class R, class A1, class A2, class A3, class A4, class F>
        class af4
        {
        public:

            typedef R result_type;
            typedef A1 arg1_type;
            typedef A2 arg2_type;
            typedef A3 arg3_type;
            typedef A4 arg4_type;

            explicit af4(F f)
                : f_(f)
            {
            }

            result_type operator()(A1 a1, A2 a2, A3 a3, A4 a4)
            {
                return f_(a1, a2, a3, a4);
            }

            result_type operator()(A1 a1, A2 a2, A3 a3, A4 a4) const
            {
                return f_(a1, a2, a3, a4);
            }

        private:

            F f_;
        };
    } // namespace _bi

    template<class R, class F>
    _bi::af0<R, F> make_adaptable(F f)
    {
        return _bi::af0<R, F>(f);
    }

    template<class R, class A1, class F>
    _bi::af1<R, A1, F> make_adaptable(F f)
    {
        return _bi::af1<R, A1, F>(f);
    }

    template<class R, class A1, class A2, class F>
    _bi::af2<R, A1, A2, F> make_adaptable(F f)
    {
        return _bi::af2<R, A1, A2, F>(f);
    }

    template<class R, class A1, class A2, class A3, class F>
    _bi::af3<R, A1, A2, A3, F> make_adaptable(F f)
    {
        return _bi::af3<R, A1, A2, A3, F>(f);
    }

    template<class R, class A1, class A2, class A3, class A4, class F>
    _bi::af4<R, A1, A2, A3, A4, F> make_adaptable(F f)
    {
        return _bi::af4<R, A1, A2, A3, A4, F>(f);
    }
} // namespace boost

#endif // #ifndef AZSTD_BIND_MAKE_ADAPTABLE_HPP_INCLUDED
