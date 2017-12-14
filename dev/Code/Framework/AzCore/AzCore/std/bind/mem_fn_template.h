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
// Based on boost 1.39.0

# define AZSTD_MEM_FN_ENABLE_CONST_OVERLOADS

// mf0

template<class R, class T AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf0)
{
public:

    typedef R result_type;
    typedef T* argument_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) ())
    F f_;

    template<class U>
    R call(U& u, T const*) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)();
    }

    template<class U>
    R call(U& u, void const*) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)();
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf0)(F f)
        : f_(f) {}

    R operator()(T* p) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)();
    }

    template<class U>
    R operator()(U& u) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u);
    }

    template<class U>
    R operator()(U const& u) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u);
    }

    R operator()(T& t) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)();
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf0) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf0) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf0)<R, T>> : function_traits_helper<R(T::*)()> { };

// cmf0

template<class R, class T AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf0)
{
public:

    typedef R result_type;
    typedef T const* argument_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) () const)
    F f_;

    template<class U>
    R call(U& u, T const*) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)();
    }

    template<class U>
    R call(U& u, void const*) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)();
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf0)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u);
    }

    R operator()(T const& t) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)();
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf0) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf0) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf0)<R, T>> : function_traits_helper<R(T::*)()> { };

// mf1

template<class R, class T, class A1 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf1)
{
public:

    typedef R result_type;
    typedef T* first_argument_type;
    typedef A1 second_argument_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1))
    F f_;

    template<class U, class B1>
    R call(U& u, T const*, B1& b1) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1);
    }

    template<class U, class B1>
    R call(U& u, void const*, B1& b1) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf1)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1);
    }

    template<class U>
    R operator()(U& u, A1 a1) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1);
    }

    template<class U>
    R operator()(U const& u, A1 a1) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1);
    }

    R operator()(T& t, A1 a1) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf1) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf1) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf1)<R, T, A1>> : function_traits_helper<R(T::*)(A1)> { };

// cmf1

template<class R, class T, class A1 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf1)
{
public:

    typedef R result_type;
    typedef T const* first_argument_type;
    typedef A1 second_argument_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1) const)
    F f_;

    template<class U, class B1>
    R call(U& u, T const*, B1& b1) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1);
    }

    template<class U, class B1>
    R call(U& u, void const*, B1& b1) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf1)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u, A1 a1) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1);
    }

    R operator()(T const& t, A1 a1) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf1) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf1) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf1)<R, T, A1>> : function_traits_helper<R(T::*)(A1)> { };

// mf2

template<class R, class T, class A1, class A2 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf2)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2))
    F f_;

    template<class U, class B1, class B2>
    R call(U& u, T const*, B1& b1, B2& b2) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2);
    }

    template<class U, class B1, class B2>
    R call(U& u, void const*, B1& b1, B2& b2) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf2)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1, A2 a2) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2);
    }

    template<class U>
    R operator()(U& u, A1 a1, A2 a2) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2);
    }

    R operator()(T& t, A1 a1, A2 a2) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf2) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf2) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf2)<R, T, A1, A2>> : function_traits_helper<R(T::*)(A1, A2)> { };

// cmf2

template<class R, class T, class A1, class A2 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf2)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2) const)
    F f_;

    template<class U, class B1, class B2>
    R call(U& u, T const*, B1& b1, B2& b2) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2);
    }

    template<class U, class B1, class B2>
    R call(U& u, void const*, B1& b1, B2& b2) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf2)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2);
    }

    R operator()(T const& t, A1 a1, A2 a2) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf2) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf2) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf2)<R, T, A1, A2>> : function_traits_helper<R(T::*)(A1, A2)> { };

// mf3

template<class R, class T, class A1, class A2, class A3 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf3)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3))
    F f_;

    template<class U, class B1, class B2, class B3>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3);
    }

    template<class U, class B1, class B2, class B3>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf3)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1, A2 a2, A3 a3) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2, a3);
    }

    template<class U>
    R operator()(U& u, A1 a1, A2 a2, A3 a3) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3);
    }

    R operator()(T& t, A1 a1, A2 a2, A3 a3) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf3) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf3) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf3)<R, T, A1, A2, A3>> : function_traits_helper<R(T::*)(A1, A2, A3)> { };

// cmf3

template<class R, class T, class A1, class A2, class A3 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf3)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3) const)
    F f_;

    template<class U, class B1, class B2, class B3>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3);
    }

    template<class U, class B1, class B2, class B3>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf3)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3);
    }

    R operator()(T const& t, A1 a1, A2 a2, A3 a3) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf3) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf3) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf3)<R, T, A1, A2, A3>> : function_traits_helper<R(T::*)(A1, A2, A3)> { };

// mf4

template<class R, class T, class A1, class A2, class A3, class A4 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf4)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4))
    F f_;

    template<class U, class B1, class B2, class B3, class B4>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4);
    }

    template<class U, class B1, class B2, class B3, class B4>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf4)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4);
    }

    template<class U>
    R operator()(U& u, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4);
    }

    R operator()(T& t, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf4) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf4) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf4)<R, T, A1, A2, A3, A4>> : function_traits_helper<R(T::*)(A1, A2, A3, A4)> { };

// cmf4

template<class R, class T, class A1, class A2, class A3, class A4 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf4)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4);
    }

    template<class U, class B1, class B2, class B3, class B4>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf4)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4);
    }

    R operator()(T const& t, A1 a1, A2 a2, A3 a3, A4 a4) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf4) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf4) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf4)<R, T, A1, A2, A3, A4>> : function_traits_helper<R(T::*)(A1, A2, A3, A4)> { };

// mf5

template<class R, class T, class A1, class A2, class A3, class A4, class A5 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf5)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf5)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5);
    }

    template<class U>
    R operator()(U& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5);
    }

    R operator()(T& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf5) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf5) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf5)<R, T, A1, A2, A3, A4, A5>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5)> { };

// cmf5

template<class R, class T, class A1, class A2, class A3, class A4, class A5 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf5)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf5)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5);
    }

    R operator()(T const& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf5) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf5) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf5)<R, T, A1, A2, A3, A4, A5>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5)> { };

// mf6

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf6)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5, A6))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf6)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6);
    }

    template<class U>
    R operator()(U& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6);
    }

    R operator()(T& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf6) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf6) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf6)<R, T, A1, A2, A3, A4, A5, A6>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5, A6)> { };

// cmf6

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf6)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5, A6) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf6)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6);
    }

    R operator()(T const& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf6) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf6) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf6)<R, T, A1, A2, A3, A4, A5, A6>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5, A6)> { };

// mf7

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf7)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5, A6, A7))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf7)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6, a7);
    }

    template<class U>
    R operator()(U& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6, a7);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6, a7);
    }

    R operator()(T& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf7) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf7) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf7)<R, T, A1, A2, A3, A4, A5, A6, A7>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5, A6, A7)> { };

// cmf7

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf7)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5, A6, A7) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf7)(F f)
        : f_(f) {}

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6, a7);
    }

    R operator()(T const& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf7) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf7) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf7)<R, T, A1, A2, A3, A4, A5, A6, A7>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5, A6, A7)> { };

// mf8

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(mf8)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5, A6, A7, A8))
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7, B8& b8) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7, B8& b8) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

public:

    explicit AZSTD_MEM_FN_NAME(mf8)(F f)
        : f_(f) {}

    R operator()(T* p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    template<class U>
    R operator()(U& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6, a7, a8);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6, a7, a8);
    }

    R operator()(T& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    bool operator==(AZSTD_MEM_FN_NAME(mf8) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(mf8) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct function_traits_helper<AZSTD_MEM_FN_NAME(mf8)<R, T, A1, A2, A3, A4, A5, A6, A7, A8>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5, A6, A7, A8)> { };

// cmf8

template<class R, class T, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8 AZSTD_MEM_FN_CLASS_F>
class AZSTD_MEM_FN_NAME(cmf8)
{
public:

    typedef R result_type;

private:

    AZSTD_MEM_FN_TYPEDEF(R (AZSTD_MEM_FN_CC T::* F) (A1, A2, A3, A4, A5, A6, A7, A8) const)
    F f_;

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8>
    R call(U& u, T const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7, B8& b8) const
    {
        AZSTD_MEM_FN_RETURN (u.*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

    template<class U, class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8>
    R call(U& u, void const*, B1& b1, B2& b2, B3& b3, B4& b4, B5& b5, B6& b6, B7& b7, B8& b8) const
    {
        AZSTD_MEM_FN_RETURN (get_pointer(u)->*f_)(b1, b2, b3, b4, b5, b6, b7, b8);
    }

public:

    explicit AZSTD_MEM_FN_NAME(cmf8)(F f)
        : f_(f) {}

    R operator()(T const* p, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        AZSTD_MEM_FN_RETURN (p->*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    template<class U>
    R operator()(U const& u, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        AZSTD_MEM_FN_RETURN call(u, &u, a1, a2, a3, a4, a5, a6, a7, a8);
    }

    R operator()(T const& t, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) const
    {
        AZSTD_MEM_FN_RETURN (t.*f_)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    bool operator==(AZSTD_MEM_FN_NAME(cmf8) const& rhs) const
    {
        return f_ == rhs.f_;
    }

    bool operator!=(AZSTD_MEM_FN_NAME(cmf8) const& rhs) const
    {
        return f_ != rhs.f_;
    }
};
template <typename R, typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct function_traits_helper<AZSTD_MEM_FN_NAME(cmf8)<R, T, A1, A2, A3, A4, A5, A6, A7, A8>> : function_traits_helper<R(T::*)(A1, A2, A3, A4, A5, A6, A7, A8)> { };

#undef AZSTD_MEM_FN_ENABLE_CONST_OVERLOADS
