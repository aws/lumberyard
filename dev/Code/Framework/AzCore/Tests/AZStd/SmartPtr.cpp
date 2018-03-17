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

#include "UserTypes.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_array.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>
#include <AzCore/std/smart_ptr/scoped_array.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Memory/SystemAllocator.h>

using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    class SmartPtr
        : public AllocatorsFixture
    {
    public:
        void SetUp() override;

        void TearDown() override;

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Shared pointer
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        class SharedPtr
        {
        public:
            class n_element_type
            {
            public:
                static void f(int&)
                {
                }

                static void test()
                {
                    typedef AZStd::shared_ptr<int>::element_type T;
                    T t;
                    f(t);
                }
            }; // namespace n_element_type

            class incomplete;

            class test
            {
            public:
                struct A
                {
                    int dummy;
                };

                struct B
                {
                    int dummy2;
                };

                struct C
                    : public A
                    , public virtual B
                {
                };

                struct X
                {
                    X(test& instanceOwner)
                        : instanceRef(instanceOwner)
                    {
                        ++instanceRef.m_xInstances;
                    }

                    virtual ~X()
                    {
                        --instanceRef.m_xInstances;
                    }

                    bool operator==(const X&) const
                    {
                        return true;
                    }

                private:
                    test& instanceRef;
                    X(X const&) = delete;
                    X& operator= (X const&) = delete;
                };

                // virtual inheritance stresses the implementation

                struct Y
                    : public A
                    , public virtual X
                {

                    Y(test& instanceOwner)
                        : X(instanceOwner)
                        , instanceRef(instanceOwner)
                    {
                        ++instanceRef.m_yInstances;
                    }

                    ~Y() override
                    {
                        --instanceRef.m_yInstances;
                    }

                private:
                    test& instanceRef;
                    Y(Y const&) = delete;
                    Y& operator= (Y const&) = delete;
                };


                static void deleter(int* p)
                {
                    EXPECT_TRUE(p == 0);
                }

                struct deleter2
                {
                    deleter2(const deleter2&) = default;
                    deleter2& operator=(const deleter2&) = default;
                    deleter2(int* basePtr) : memberPtr(basePtr) {}
                    void operator()(int* p)
                    {
                        EXPECT_TRUE(p == memberPtr);
                        ++*p;
                    }

                    int* memberPtr = nullptr;
                };

                struct deleter3
                {
                    void operator()(incomplete* p)
                    {
                        EXPECT_TRUE(p == 0);
                    }
                };


                struct deleter_void
                {
                    deleter_void(const deleter_void&) = default;
                    deleter_void& operator=(const deleter_void& other) = default;
                    deleter_void(void*& deletedPtr) : d(&deletedPtr) {}

                    void operator()(void* p)
                    {
                        *d = p;
                    }
                    void** d;
                };

                int m = 0;
                void* deleted = nullptr;
                AZ::s32 m_xInstances = 0;
                AZ::s32 m_yInstances = 0;
            };

            template <class P, class T>
            void TestPtr(AZStd::shared_ptr<P>& pt, T* p)
            {
                if (p)
                {
                    EXPECT_TRUE(pt);
                    EXPECT_FALSE(!pt);
                }
                else
                {
                    EXPECT_FALSE(pt);
                    EXPECT_TRUE(!pt);
                }

                EXPECT_EQ(p, pt.get());
                EXPECT_EQ(1, pt.use_count());
                EXPECT_TRUE(pt.unique());
            }

            template <class P, class T>
            void TestAddr(T* p)
            {
                AZStd::shared_ptr<P> pt(p);
                TestPtr(pt, p);
            }

            template <class P, class T>
            void TestNull(T* p)
            {
                EXPECT_EQ(nullptr, p);
                TestAddr<P, T>(p);
            }

            template <class T>
            void TestType(T* p)
            {
                TestNull<T>(p);
            }

            template <class P, class T>
            void TestValue(T* p, const T& val)
            {
                AZStd::shared_ptr<P> pi(p);
                EXPECT_TRUE(pi);
                EXPECT_FALSE(!pi);
                EXPECT_EQ(p, pi.get());
                EXPECT_EQ(1, pi.use_count());
                EXPECT_TRUE(pi.unique());
                EXPECT_EQ(val, *reinterpret_cast<const T*>(pi.get()));
            }

            template <class P, class T>
            void TestPolymorphic(T* p)
            {
                AZStd::shared_ptr<P> pi(p);
                EXPECT_TRUE(pi);
                EXPECT_FALSE(!pi);
                EXPECT_EQ(p, pi.get());
                EXPECT_EQ(1, pi.use_count());
                EXPECT_TRUE(pi.unique());
                EXPECT_EQ(1, m_test.m_xInstances);
                EXPECT_EQ(1, m_test.m_yInstances);
            }

            test m_test;
        };

        SharedPtr* m_sharedPtr = nullptr;
    };

    void SmartPtr::SetUp()
    {
        AllocatorsFixture::SetUp();
        m_sharedPtr = new SharedPtr;
    }

    void SmartPtr::TearDown()
    {
        delete m_sharedPtr;
        AllocatorsFixture::TearDown();
    }

    TEST_F(SmartPtr, SharedPtrCtorInt)
    {
        AZStd::shared_ptr<int> pi;
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_EQ(nullptr, pi.get());
        EXPECT_EQ(0, pi.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorVoid)
    {
        AZStd::shared_ptr<void> pv;
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_EQ(0, pv.get());
        EXPECT_EQ(0, pv.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorIncomplete)
    {
        AZStd::shared_ptr<SharedPtr::incomplete> px;
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_EQ(nullptr, px.get());
        EXPECT_EQ(0, px.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorNullptr)
    {
        AZStd::shared_ptr<int> pi(nullptr);
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_EQ(nullptr, pi.get());
        EXPECT_EQ(0, pi.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorIntPtr)
    {
        m_sharedPtr->TestType(static_cast<int*>(0));
        m_sharedPtr->TestType(static_cast<int const*>(0));
        m_sharedPtr->TestType(static_cast<int volatile*>(0));
        m_sharedPtr->TestType(static_cast<int const volatile*>(0));
    }

    TEST_F(SmartPtr, SharedPtrCtorConstIntPtr)
    {
        m_sharedPtr->TestNull<int const>(static_cast<int*>(0));
        m_sharedPtr->TestNull<int volatile>(static_cast<int*>(0));
        m_sharedPtr->TestNull<void>(static_cast<int*>(0));
        m_sharedPtr->TestNull<void const>(static_cast<int*>(0));
    }

    TEST_F(SmartPtr, SharedPtrCtorX)
    {
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X*>(0));
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X const*>(0));
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X volatile*>(0));
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X const volatile*>(0));
    }

    TEST_F(SmartPtr, SharedPtrCtorXConvert)
    {
        m_sharedPtr->TestNull<SharedPtr::test::X const>(static_cast<SharedPtr::test::X*>(0));
        m_sharedPtr->TestNull<SharedPtr::test::X>(static_cast<SharedPtr::test::Y*>(0));
        m_sharedPtr->TestNull<SharedPtr::test::X const>(static_cast<SharedPtr::test::Y*>(0));
        m_sharedPtr->TestNull<void>(static_cast<SharedPtr::test::X*>(0));
        m_sharedPtr->TestNull<void const>(static_cast<SharedPtr::test::X*>(0));
    }

    TEST_F(SmartPtr, SharedPtrCtorIntValue)
    {
        m_sharedPtr->TestValue<int>(new int(7), 7);
        m_sharedPtr->TestValue<int const>(new int(7), 7);
        m_sharedPtr->TestValue<void>(new int(7), 7);
        m_sharedPtr->TestValue<void const>(new int(7), 7);
    }

    TEST_F(SmartPtr, SharedPtrTestXValue)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<X>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
        
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<X const>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
        
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<void>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
        
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<void const>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
    }

    TEST_F(SmartPtr, SharedPtrTestXValuePoly)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
        m_sharedPtr->TestPolymorphic< X, Y>(new Y(m_sharedPtr->m_test));

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
        m_sharedPtr->TestPolymorphic< X const, Y>(new Y(m_sharedPtr->m_test));
    }

    TEST_F(SmartPtr, SharedPtrFromUniquePtr)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        {
            X* p = new X(m_sharedPtr->m_test);
            AZStd::unique_ptr<X> pu(p);
            AZStd::shared_ptr<X> px(AZStd::move(pu));
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        {
            X* p = new X(m_sharedPtr->m_test);
            AZStd::unique_ptr<X> pu(p);
            AZStd::shared_ptr<X> px(nullptr);
            px = AZStd::move(pu);
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        {
            Y* p = new Y(m_sharedPtr->m_test);
            AZStd::unique_ptr<Y> pu(p);
            AZStd::shared_ptr<X> px(AZStd::move(pu));
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        {
            Y* p = new Y(m_sharedPtr->m_test);
            AZStd::unique_ptr<Y> pu(p);
            AZStd::shared_ptr<X> px(nullptr);
            px = AZStd::move(pu);
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, UniquePtrToUniquePtrPoly)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
        {
            // Test moving unique_ptr of child type to unique_ptr of base type
            Y * p = new Y(m_sharedPtr->m_test);
            AZStd::unique_ptr<Y> pu(p);
            AZStd::unique_ptr<X> px(AZStd::move(pu));
            EXPECT_TRUE(!!px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrCtorNullDeleter)
    {
        {
            AZStd::shared_ptr<int> pi(static_cast<int*>(0), &SharedPtr::test::deleter);
            m_sharedPtr->TestPtr<int, int>(pi, nullptr);
        }
        {
            AZStd::shared_ptr<void> pv(static_cast<int*>(0), &SharedPtr::test::deleter);
            m_sharedPtr->TestPtr<void, int>(pv, nullptr);
        }
        {
            AZStd::shared_ptr<void const> pv(static_cast<int*>(0), &SharedPtr::test::deleter);
            m_sharedPtr->TestPtr<void const, int>(pv, nullptr);
        }
    }

    TEST_F(SmartPtr, SharedPtrCtorIncompleteDeleter)
    {
        SharedPtr::incomplete* p0 = nullptr;
        {
            AZStd::shared_ptr<SharedPtr::incomplete> px(p0, SharedPtr::test::deleter3());
            m_sharedPtr->TestPtr(px, p0);
        }
        {
            AZStd::shared_ptr<void> pv(p0, SharedPtr::test::deleter3());
            m_sharedPtr->TestPtr(pv, p0);
        }
        {
            AZStd::shared_ptr<void const> pv(p0, SharedPtr::test::deleter3());
            m_sharedPtr->TestPtr(pv, p0);
        }
    }

    TEST_F(SmartPtr, SharedPtrCtorIntDeleter)
    {
        EXPECT_EQ(0, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<int> pi(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pi, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(1, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<int const> pi(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pi, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(2, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<void> pv(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pv, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(3, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<void const> pv(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pv, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(4, m_sharedPtr->m_test.m);
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorEmptyRhs)
    {
        AZStd::shared_ptr<int> pi;

        AZStd::shared_ptr<int> pi2(pi);
        EXPECT_EQ(pi2, pi);
        EXPECT_FALSE(pi2);
        EXPECT_TRUE(!pi2);
        EXPECT_EQ(0, pi2.get());
        EXPECT_EQ(pi2.use_count(), pi.use_count());

        AZStd::shared_ptr<void> pi3(pi);
        EXPECT_EQ(pi3, pi);
        EXPECT_FALSE(pi3);
        EXPECT_TRUE(!pi3);
        EXPECT_EQ(0, pi3.get());
        EXPECT_EQ(pi3.use_count(), pi.use_count());

        AZStd::shared_ptr<void> pi4(pi3);
        EXPECT_EQ(pi4, pi3);
        EXPECT_FALSE(pi4);
        EXPECT_TRUE(!pi4);
        EXPECT_EQ(0, pi4.get());
        EXPECT_EQ(pi4.use_count(), pi3.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorEmptyVoid)
    {
        AZStd::shared_ptr<void> pv;

        AZStd::shared_ptr<void> pv2(pv);
        EXPECT_EQ(pv2, pv);
        EXPECT_FALSE(pv2);
        EXPECT_TRUE(!pv2);
        EXPECT_EQ(0, pv2.get());
        EXPECT_EQ(pv2.use_count(), pv.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorVoidFromIncomplete)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<incomplete> px;

        AZStd::shared_ptr<incomplete> px2(px);
        EXPECT_EQ(px, px2);
        EXPECT_FALSE(px2);
        EXPECT_TRUE(!px2);
        EXPECT_EQ(0, px2.get());
        EXPECT_EQ(px.use_count(), px2.use_count());

        AZStd::shared_ptr<void> px3(px);
        EXPECT_EQ(px, px3);
        EXPECT_FALSE(px3);
        EXPECT_TRUE(!px3);
        EXPECT_EQ(0, px3.get());
        EXPECT_EQ(px.use_count(), px3.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorIntVoidSharedOwnershipTest)
    {
        AZStd::shared_ptr<int> pi(static_cast<int*>(0));

        AZStd::shared_ptr<int> pi2(pi);
        EXPECT_EQ(pi, pi2);
        EXPECT_FALSE(pi2);
        EXPECT_TRUE(!pi2);
        EXPECT_EQ(0, pi2.get());
        EXPECT_EQ(2, pi2.use_count());
        EXPECT_FALSE(pi2.unique());
        EXPECT_EQ(pi.use_count(), pi2.use_count());
        EXPECT_TRUE(!(pi < pi2 || pi2 < pi)); // shared ownership test

        AZStd::shared_ptr<void> pi3(pi);
        EXPECT_EQ(pi, pi3);
        EXPECT_FALSE(pi3);
        EXPECT_TRUE(!pi3);
        EXPECT_EQ(0, pi3.get());
        EXPECT_EQ(3, pi3.use_count());
        EXPECT_FALSE(pi3.unique());
        EXPECT_EQ(pi.use_count(), pi3.use_count());
        EXPECT_TRUE(!(pi < pi3 || pi3 < pi)); // shared ownership test

        AZStd::shared_ptr<void> pi4(pi2);
        EXPECT_EQ(pi2, pi4);
        EXPECT_FALSE(pi4);
        EXPECT_TRUE(!pi4);
        EXPECT_EQ(0, pi4.get());
        EXPECT_EQ(4, pi4.use_count());
        EXPECT_FALSE(pi4.unique());
        EXPECT_EQ(pi2.use_count(), pi4.use_count());
        EXPECT_TRUE(!(pi2 < pi4 || pi4 < pi2)); // shared ownership test

        EXPECT_EQ(pi4.use_count(), pi3.use_count());
        EXPECT_TRUE(!(pi3 < pi4 || pi4 < pi3)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorClassSharedOwnershipTest)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X> px(static_cast<X*>(0));

        AZStd::shared_ptr<X> px2(px);
        EXPECT_EQ(px, px2);
        EXPECT_FALSE(px2);
        EXPECT_TRUE(!px2);
        EXPECT_EQ(0, px2.get());
        EXPECT_EQ(2, px2.use_count());
        EXPECT_FALSE(px2.unique());
        EXPECT_EQ(px.use_count(), px2.use_count());
        EXPECT_TRUE(!(px < px2 || px2 < px)); // shared ownership test

        AZStd::shared_ptr<void> px3(px);
        EXPECT_EQ(px, px3);
        EXPECT_FALSE(px3);
        EXPECT_TRUE(!px3);
        EXPECT_EQ(0, px3.get());
        EXPECT_EQ(3, px3.use_count());
        EXPECT_FALSE(px3.unique());
        EXPECT_EQ(px.use_count(), px3.use_count());
        EXPECT_TRUE(!(px < px3 || px3 < px)); // shared ownership test

        AZStd::shared_ptr<void> px4(px2);
        EXPECT_EQ(px2, px4);
        EXPECT_FALSE(px4);
        EXPECT_TRUE(!px4);
        EXPECT_EQ(0, px4.get());
        EXPECT_EQ(4, px4.use_count());
        EXPECT_FALSE(px4.unique());
        EXPECT_EQ(px2.use_count(), px4.use_count());
        EXPECT_TRUE(!(px2 < px4 || px4 < px2)); // shared ownership test

        EXPECT_EQ(px4.use_count(), px3.use_count());
        EXPECT_TRUE(!(px3 < px4 || px4 < px3)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedIntValue)
    {
        int* p = new int(7);
        AZStd::shared_ptr<int> pi(p);

        AZStd::shared_ptr<int> pi2(pi);
        EXPECT_EQ(pi, pi2);
        EXPECT_TRUE(pi2 ? true : false);
        EXPECT_TRUE(!!pi2);
        EXPECT_EQ(p, pi2.get());
        EXPECT_EQ(2, pi2.use_count());
        EXPECT_TRUE(!pi2.unique());
        EXPECT_EQ(7, *pi2);
        EXPECT_EQ(pi.use_count(), pi2.use_count());
        EXPECT_TRUE(!(pi < pi2 || pi2 < pi)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedIntVoidValue)
    {
        int* p = new int(7);
        AZStd::shared_ptr<void> pv(p);
        EXPECT_EQ(p, pv.get());

        AZStd::shared_ptr<void> pv2(pv);
        EXPECT_EQ(pv, pv2);
        EXPECT_TRUE(pv2 ? true : false);
        EXPECT_TRUE(!!pv2);
        EXPECT_EQ(p, pv2.get());
        EXPECT_EQ(2, pv2.use_count());
        EXPECT_TRUE(!pv2.unique());
        EXPECT_EQ(pv.use_count(), pv2.use_count());
        EXPECT_TRUE(!(pv < pv2 || pv2 < pv)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedInstanceCount)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0 ,m_sharedPtr->m_test.m_xInstances);

        {
            X* p = new X(m_sharedPtr->m_test);
            AZStd::shared_ptr<X> px(p);
            EXPECT_EQ(p, px.get());

            AZStd::shared_ptr<X> px2(px);
            EXPECT_EQ(px, px2);
            EXPECT_TRUE(px2 ? true : false);
            EXPECT_TRUE(!!px2);
            EXPECT_EQ(p, px2.get());
            EXPECT_EQ(2, px2.use_count());
            EXPECT_TRUE(!px2.unique());

            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

            EXPECT_EQ(px.use_count(), px2.use_count());
            EXPECT_TRUE(!(px < px2 || px2 < px)); // shared ownership test

            AZStd::shared_ptr<void> px3(px);
            EXPECT_EQ(px, px3);
            EXPECT_TRUE(px3 ? true : false);
            EXPECT_TRUE(!!px3);
            EXPECT_EQ(p, px3.get());
            EXPECT_EQ(3, px3.use_count());
            EXPECT_TRUE(!px3.unique());
            EXPECT_EQ(px.use_count(), px3.use_count());
            EXPECT_TRUE(!(px < px3 || px3 < px)); // shared ownership test

            AZStd::shared_ptr<void> px4(px2);
            EXPECT_EQ(px2, px4);
            EXPECT_TRUE(px4 ? true : false);
            EXPECT_TRUE(!!px4);
            EXPECT_EQ(p, px4.get());
            EXPECT_EQ(4, px4.use_count());
            EXPECT_TRUE(!px4.unique());
            EXPECT_EQ(px2.use_count(), px4.use_count());
            EXPECT_TRUE(!(px2 < px4 || px4 < px2)); // shared ownership test

            EXPECT_EQ(px4.use_count(), px3.use_count());
            EXPECT_TRUE(!(px3 < px4 || px4 < px3)); // shared ownership test
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedPolyInstanceCount)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        {
            Y* p = new Y(m_sharedPtr->m_test);
            AZStd::shared_ptr<Y> py(p);
            EXPECT_EQ(p, py.get());

            AZStd::shared_ptr<X> px(py);
            EXPECT_EQ(py, px);
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(2, px.use_count());
            EXPECT_TRUE(!px.unique());
            EXPECT_EQ(py.use_count(), px.use_count());
            EXPECT_TRUE(!(px < py || py < px)); // shared ownership test

            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

            AZStd::shared_ptr<void const> pv(px);
            EXPECT_EQ(px, pv);
            EXPECT_TRUE(pv);
            EXPECT_FALSE(!pv);
            EXPECT_EQ(px.get(), pv.get());
            EXPECT_EQ(3, pv.use_count());
            EXPECT_TRUE(!pv.unique());
            EXPECT_EQ(px.use_count(), pv.use_count());
            EXPECT_TRUE(!(px < pv || pv < px)); // shared ownership test

            AZStd::shared_ptr<void const> pv2(py);
            EXPECT_EQ(py, pv2);
            EXPECT_TRUE(pv2);
            EXPECT_FALSE(!pv2);
            EXPECT_EQ(py.get(), pv2.get());
            EXPECT_EQ(4, pv2.use_count());
            EXPECT_TRUE(!pv2.unique());
            EXPECT_EQ(py.use_count(), pv2.use_count());
            EXPECT_TRUE(!(py < pv2 || pv2 < py)); // shared ownership test

            EXPECT_EQ(pv2.use_count(), pv.use_count());
            EXPECT_TRUE(!(pv < pv2 || pv2 < pv)); // shared ownership test
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrWeakPtrCtor)
    {
        using Y = SharedPtr::test::Y;
        AZStd::weak_ptr<Y> wp;
        EXPECT_EQ(0, wp.use_count());
    }

    TEST_F(SmartPtr, SharedPtrWeakPtrCtorSharedOwnership)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<Y> p;
        AZStd::weak_ptr<Y> wp(p);

        if (wp.use_count() != 0) // 0 allowed but not required
        {
            AZStd::shared_ptr<Y> p2(wp);
            EXPECT_EQ(wp.use_count(), p2.use_count());
            EXPECT_EQ(0, p2.get());

            AZStd::shared_ptr<X> p3(wp);
            EXPECT_EQ(wp.use_count(), p3.use_count());
            EXPECT_EQ(0, p3.get());
        }
    }


    TEST_F(SmartPtr, SharedPtrWeakPtrSharedOwnershipReset)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<Y> p(new Y(m_sharedPtr->m_test));
        AZStd::weak_ptr<Y> wp(p);

        {
            AZStd::shared_ptr<Y> p2(wp);
            EXPECT_TRUE(p2 ? true : false);
            EXPECT_TRUE(!!p2);
            EXPECT_EQ(p.get(), p2.get());
            EXPECT_EQ(2, p2.use_count());
            EXPECT_TRUE(!p2.unique());
            EXPECT_EQ(wp.use_count(), p2.use_count());

            EXPECT_EQ(p2.use_count(), p.use_count());
            EXPECT_TRUE(!(p < p2 || p2 < p)); // shared ownership test

            AZStd::shared_ptr<X> p3(wp);
            EXPECT_TRUE(p3 ? true : false);
            EXPECT_TRUE(!!p3);
            EXPECT_EQ(p.get(), p3.get());
            EXPECT_EQ(3, p3.use_count());
            EXPECT_TRUE(!p3.unique());
            EXPECT_EQ(wp.use_count(), p3.use_count());

            EXPECT_EQ(p3.use_count(), p.use_count());
        }

        p.reset();
        EXPECT_EQ(0, wp.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyAssignIncomplete)
    {
        using incomplete = SharedPtr::incomplete;

        AZStd::shared_ptr<incomplete> p1;

        p1 = p1;

        EXPECT_EQ(p1, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<incomplete> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<incomplete> p3(p1);

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());
    }

    TEST_F(SmartPtr, SharedPtrCopyAssignVoid)
    {
        AZStd::shared_ptr<void> p1;

        p1 = p1;

        EXPECT_EQ(p1, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<void> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<void> p3(p1);

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<void> p4(new int);
        EXPECT_EQ(1, p4.use_count());

        p1 = p4;

        EXPECT_EQ(p4, p1);
        EXPECT_TRUE(!(p1 < p4 || p4 < p1));
        EXPECT_EQ(2, p1.use_count());
        EXPECT_EQ(2, p4.use_count());

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_EQ(1, p4.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyAssignClass)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X> p1;

        p1 = p1;

        EXPECT_EQ(p1, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<X> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<X> p3(p1);

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        AZStd::shared_ptr<X> p4(new X(m_sharedPtr->m_test));

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        p1 = p4;

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        EXPECT_EQ(p4, p1);
        EXPECT_TRUE(!(p1 < p4 || p4 < p1));

        EXPECT_EQ(2, p1.use_count());

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        p4 = p3;

        EXPECT_EQ(p3, p4);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrConvertAssignIntVoidIncomplete)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<void> p1;

        AZStd::shared_ptr<incomplete> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        AZStd::shared_ptr<int> p4(new int);
        EXPECT_EQ(1, p4.use_count());

        AZStd::shared_ptr<void> p5(p4);
        EXPECT_EQ(2, p4.use_count());

        p1 = p4;

        EXPECT_EQ(p4, p1);
        EXPECT_TRUE(!(p1 < p5 || p5 < p1));
        EXPECT_EQ(3, p1.use_count());
        EXPECT_EQ(3, p4.use_count());

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_EQ(2, p4.use_count());
    }

    TEST_F(SmartPtr, SharedPtrConvertAssignPolymorphic)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<X> p1;
        AZStd::shared_ptr<Y> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(0, p1.get());

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        AZStd::shared_ptr<Y> p4(new Y(m_sharedPtr->m_test));

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(m_sharedPtr->m_test.m_yInstances, 1);
        EXPECT_TRUE(p4.use_count() == 1);

        AZStd::shared_ptr<X> p5(p4);
        EXPECT_TRUE(p4.use_count() == 2);

        p1 = p4;

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

        EXPECT_TRUE(p1 == p4);
        EXPECT_TRUE(!(p1 < p5 || p5 < p1));

        EXPECT_TRUE(p1.use_count() == 3);
        EXPECT_TRUE(p4.use_count() == 3);

        p1 = p2;

        EXPECT_TRUE(p1 == p2);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        EXPECT_TRUE(p4.use_count() == 2);

        p4 = p2;
        p5 = p2;

        EXPECT_TRUE(p4 == p2);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyInt)
    {
        AZStd::shared_ptr<int> pi;
        pi.reset();
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == 0);
        EXPECT_TRUE(pi.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNullInt)
    {
        AZStd::shared_ptr<int> pi(static_cast<int*>(0));
        pi.reset();
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == 0);
        EXPECT_TRUE(pi.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNewInt)
    {
        AZStd::shared_ptr<int> pi(new int);
        pi.reset();
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == 0);
        EXPECT_TRUE(pi.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetIncomplete)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<incomplete> px;
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetIncompleteWithDeleter)
    {
        using incomplete = SharedPtr::incomplete;
        incomplete* p0 = nullptr;
        AZStd::shared_ptr<incomplete> px(p0, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyClass)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X> px;
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNewClass)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        AZStd::shared_ptr<X> px(new X(m_sharedPtr->m_test));
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 0);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyVoid)
    {
        AZStd::shared_ptr<void> pv;
        pv.reset();
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNewVoid)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        AZStd::shared_ptr<void> pv(new X(m_sharedPtr->m_test));
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        pv.reset();
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 0);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyToNewInt)
    {
        AZStd::shared_ptr<int> pi;

        pi.reset(static_cast<int*>(0));
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == 0);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        int* p = new int;
        pi.reset(p);
        EXPECT_TRUE(pi ? true : false);
        EXPECT_TRUE(!!pi);
        EXPECT_TRUE(pi.get() == p);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        pi.reset(static_cast<int*>(0));
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == 0);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyClassToNewClass)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<X> px;

        px.reset(static_cast<X*>(0));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        X* p = new X(m_sharedPtr->m_test);
        px.reset(p);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        px.reset(static_cast<X*>(0));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        Y* q = new Y(m_sharedPtr->m_test);
        px.reset(q);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == q);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

        px.reset(static_cast<Y*>(0));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyVoidToNewClass)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<void> pv;

        pv.reset(static_cast<X*>(0));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        X* p = new X(m_sharedPtr->m_test);
        pv.reset(p);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == p);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        pv.reset(static_cast<X*>(0));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        Y* q = new Y(m_sharedPtr->m_test);
        pv.reset(q);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == q);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

        pv.reset(static_cast<Y*>(0));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetIntWithDeleter)
    {
        AZStd::shared_ptr<int> pi;

        pi.reset(static_cast<int*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == 0);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        m_sharedPtr->m_test.deleted = &pi;

        int m = 0;
        pi.reset(&m, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
        EXPECT_TRUE(pi ? true : false);
        EXPECT_TRUE(!!pi);
        EXPECT_TRUE(pi.get() == &m);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        pi.reset(static_cast<int*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &m);
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == 0);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        pi.reset();
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetClassWithDeleter)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<X> px;

        px.reset(static_cast<X*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        m_sharedPtr->m_test.deleted = &px;

        X x(m_sharedPtr->m_test);
        px.reset(&x, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == &x);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        px.reset(static_cast<X*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &x);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        Y y(m_sharedPtr->m_test);
        px.reset(&y, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == &y);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        px.reset(static_cast<Y*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &y);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        px.reset();
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetVoidClassWithDeleter)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<void> pv;

        pv.reset(static_cast<X*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        m_sharedPtr->m_test.deleted = &pv;

        X x(m_sharedPtr->m_test);
        pv.reset(&x, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == &x);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        pv.reset(static_cast<X*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &x);
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        Y y(m_sharedPtr->m_test);
        pv.reset(&y, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == &y);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        pv.reset(static_cast<Y*>(0), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &y);
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == 0);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        pv.reset();
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetIncompleteNullWithDeleter)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<incomplete> px;

        incomplete* p0 = nullptr;

        px.reset(p0, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        m_sharedPtr->m_test.deleted = &px;
        px.reset(p0, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == 0);
    }

    TEST_F(SmartPtr, SharedPtrGetPointerEmpty)
    {
        struct X {};
        AZStd::shared_ptr<X> px;
        EXPECT_TRUE(px.get() == 0);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerNull)
    {
        struct X {};
        AZStd::shared_ptr<X> px(static_cast<X*>(0));
        EXPECT_TRUE(px.get() == 0);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerCheckedDeleterNull)
    {
        struct X {};
        AZStd::shared_ptr<X> px(static_cast<X*>(0), AZStd::checked_deleter<X>());
        EXPECT_TRUE(px.get() == 0);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerNewClass)
    {
        struct X {};
        X* p = new X;
        AZStd::shared_ptr<X> px(p);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(&*px == px.get());
        EXPECT_TRUE(px.operator ->() == px.get());

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerCheckedDeleterNewClass)
    {
        struct X {};
        X* p = new X;
        AZStd::shared_ptr<X> px(p, AZStd::checked_deleter<X>());
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(&*px == px.get());
        EXPECT_TRUE(px.operator ->() == px.get());

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrUseCountNullClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px(static_cast<X*>(0));
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        AZStd::shared_ptr<X> px2(px);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(!px2.unique());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(!px.unique());
    }

    TEST_F(SmartPtr, SharedPtrUseCountNewClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px(new X);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        AZStd::shared_ptr<X> px2(px);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(!px2.unique());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(!px.unique());
    }

    TEST_F(SmartPtr, SharedPtrUseCountCheckedDeleterNewClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px(new X, AZStd::checked_deleter<X>());
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        AZStd::shared_ptr<X> px2(px);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(!px2.unique());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(!px.unique());
    }

    TEST_F(SmartPtr, SharedPtrSwapEmptyClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px;
        AZStd::shared_ptr<X> px2;

        px.swap(px2);

        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px2.get() == 0);

        using std::swap;
        swap(px, px2);

        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px2.get() == 0);
    }

    TEST_F(SmartPtr, SharedPtrSwapNewClass)
    {
        struct X {};
        X* p = new X;
        AZStd::shared_ptr<X> px;
        AZStd::shared_ptr<X> px2(p);
        AZStd::shared_ptr<X> px3(px2);

        px.swap(px2);

        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.get() == 0);
        EXPECT_TRUE(px3.get() == p);
        EXPECT_TRUE(px3.use_count() == 2);

        using std::swap;
        swap(px, px2);

        EXPECT_TRUE(px.get() == 0);
        EXPECT_TRUE(px2.get() == p);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(px3.get() == p);
        EXPECT_TRUE(px3.use_count() == 2);
    }

    TEST_F(SmartPtr, SharedPtrSwap2NewClasses)
    {
        struct X {};
        X* p1 = new X;
        X* p2 = new X;
        AZStd::shared_ptr<X> px(p1);
        AZStd::shared_ptr<X> px2(p2);
        AZStd::shared_ptr<X> px3(px2);

        px.swap(px2);

        EXPECT_TRUE(px.get() == p2);
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.get() == p1);
        EXPECT_TRUE(px2.use_count() == 1);
        EXPECT_TRUE(px3.get() == p2);
        EXPECT_TRUE(px3.use_count() == 2);

        using std::swap;
        swap(px, px2);

        EXPECT_TRUE(px.get() == p1);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px2.get() == p2);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(px3.get() == p2);
        EXPECT_TRUE(px3.use_count() == 2);
    }

    TEST_F(SmartPtr, SharedPtrCompareEmptyClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px;
        EXPECT_TRUE(px == px);
        EXPECT_TRUE(!(px != px));
        EXPECT_TRUE(!(px < px));

        AZStd::shared_ptr<A> px2;

        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(px == px2);
        EXPECT_TRUE(!(px != px2));
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareCopyOfEmptyClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px;
        AZStd::shared_ptr<A> px2(px);

        EXPECT_TRUE(px2 == px2);
        EXPECT_TRUE(!(px2 != px2));
        EXPECT_TRUE(!(px2 < px2));

        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(px == px2);
        EXPECT_TRUE(!(px != px2));
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareNewClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px;
        AZStd::shared_ptr<A> px2(new A);

        EXPECT_TRUE(px2 == px2);
        EXPECT_TRUE(!(px2 != px2));
        EXPECT_TRUE(!(px2 < px2));

        EXPECT_TRUE(px.get() != px2.get());
        EXPECT_TRUE(px != px2);
        EXPECT_TRUE(!(px == px2));
        EXPECT_TRUE(px < px2 || px2 < px);
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareNewClasses)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px(new A);
        AZStd::shared_ptr<A> px2(new A);

        EXPECT_TRUE(px.get() != px2.get());
        EXPECT_TRUE(px != px2);
        EXPECT_TRUE(!(px == px2));
        EXPECT_TRUE(px < px2 || px2 < px);
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareCopiedNewClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px(new A);
        AZStd::shared_ptr<A> px2(px);

        EXPECT_TRUE(px2 == px2);
        EXPECT_TRUE(!(px2 != px2));
        EXPECT_TRUE(!(px2 < px2));

        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(px == px2);
        EXPECT_TRUE(!(px != px2));
        EXPECT_TRUE(!(px < px2 || px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrComparePolymorphicClasses)
    {
        using A = SharedPtr::test::A;
        using B = SharedPtr::test::B;
        using C = SharedPtr::test::C;
        AZStd::shared_ptr<A> px(new A);
        AZStd::shared_ptr<B> py(new B);
        AZStd::shared_ptr<C> pz(new C);

        EXPECT_NE(px.get(), pz.get());
        EXPECT_TRUE(px != pz);
        EXPECT_TRUE(!(px == pz));

        EXPECT_TRUE(py.get() != pz.get());
        EXPECT_TRUE(py != pz);
        EXPECT_TRUE(!(py == pz));

        EXPECT_TRUE(px < py || py < px);
        EXPECT_TRUE(px < pz || pz < px);
        EXPECT_TRUE(py < pz || pz < py);

        EXPECT_TRUE(!(px < py && py < px));
        EXPECT_TRUE(!(px < pz && pz < px));
        EXPECT_TRUE(!(py < pz && pz < py));

        AZStd::shared_ptr<void> pvx(px);

        EXPECT_TRUE(pvx == pvx);
        EXPECT_TRUE(!(pvx != pvx));
        EXPECT_TRUE(!(pvx < pvx));

        AZStd::shared_ptr<void> pvy(py);
        AZStd::shared_ptr<void> pvz(pz);

        EXPECT_TRUE(pvx < pvy || pvy < pvx);
        EXPECT_TRUE(pvx < pvz || pvz < pvx);
        EXPECT_TRUE(pvy < pvz || pvz < pvy);

        EXPECT_TRUE(!(pvx < pvy && pvy < pvx));
        EXPECT_TRUE(!(pvx < pvz && pvz < pvx));
        EXPECT_TRUE(!(pvy < pvz && pvz < pvy));
    }

    TEST_F(SmartPtr, SharedPtrComparePolymorphicWithEmpty)
    {
        using A = SharedPtr::test::A;
        using B = SharedPtr::test::B;
        using C = SharedPtr::test::C;
        AZStd::shared_ptr<C> pz(new C);
        AZStd::shared_ptr<A> px(pz);

        EXPECT_TRUE(px == px);
        EXPECT_TRUE(!(px != px));
        EXPECT_TRUE(!(px < px));

        AZStd::shared_ptr<B> py(pz);

        EXPECT_EQ(px.get(), pz.get());
        EXPECT_TRUE(px == pz);
        EXPECT_TRUE(!(px != pz));

        EXPECT_TRUE(py.get() == pz.get());
        EXPECT_TRUE(py == pz);
        EXPECT_TRUE(!(py != pz));

        EXPECT_TRUE(!(px < py || py < px));
        EXPECT_TRUE(!(px < pz || pz < px));
        EXPECT_TRUE(!(py < pz || pz < py));

        AZStd::shared_ptr<void> pvx(px);
        AZStd::shared_ptr<void> pvy(py);
        AZStd::shared_ptr<void> pvz(pz);

        // pvx and pvy aren't equal...
        EXPECT_TRUE(pvx.get() != pvy.get());
        EXPECT_TRUE(pvx != pvy);
        EXPECT_TRUE(!(pvx == pvy));

        // ... but they share ownership ...
        EXPECT_TRUE(!(pvx < pvy || pvy < pvx));

        // ... with pvz
        EXPECT_TRUE(!(pvx < pvz || pvz < pvx));
        EXPECT_TRUE(!(pvy < pvz || pvz < pvy));
    }

    TEST_F(SmartPtr, SharedPtrStaticPointerCastNull)
    {
        struct X {};
        AZStd::shared_ptr<void> pv;

        AZStd::shared_ptr<int> pi = AZStd::static_pointer_cast<int>(pv);
        EXPECT_TRUE(pi.get() == 0);

        AZStd::shared_ptr<X> px = AZStd::static_pointer_cast<X>(pv);
        EXPECT_TRUE(px.get() == 0);
    }

    TEST_F(SmartPtr, SharedPtrStaticPointerCastNewIntToVoid)
    {
        AZStd::shared_ptr<int> pi(new int);
        AZStd::shared_ptr<void> pv(pi);

        AZStd::shared_ptr<int> pi2 = AZStd::static_pointer_cast<int>(pv);
        EXPECT_TRUE(pi.get() == pi2.get());
        EXPECT_TRUE(!(pi < pi2 || pi2 < pi));
        EXPECT_TRUE(pi.use_count() == 3);
        EXPECT_TRUE(pv.use_count() == 3);
        EXPECT_TRUE(pi2.use_count() == 3);
    }

    TEST_F(SmartPtr, SharedPtrStaticPointerCastPolymorphic)
    {
        struct X {};
        struct Y : public X {};
        AZStd::shared_ptr<X> px(new Y);

        AZStd::shared_ptr<Y> py = AZStd::static_pointer_cast<Y>(px);
        EXPECT_TRUE(px.get() == py.get());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(py.use_count() == 2);

        AZStd::shared_ptr<X> px2(py);
        EXPECT_TRUE(!(px < px2 || px2 < px));
    }

    // Test smart pointer interactions with EBus
    struct SmartPtrTestBusInterface : public AZ::EBusTraits
    {
        virtual void TakesSharedPtrByCopy(AZStd::shared_ptr<int> sharedPtrByCopy) = 0;
    };
    using SmartPtrTestBus = AZ::EBus<SmartPtrTestBusInterface>;

    class TakesSharedPtrHandler : public SmartPtrTestBus::Handler
    {
    public:
        TakesSharedPtrHandler()
        {
            SmartPtrTestBus::Handler::BusConnect();
        }

        void TakesSharedPtrByCopy(AZStd::shared_ptr<int> sharedPtrByCopy) override
        {
            EXPECT_NE(nullptr, sharedPtrByCopy.get());
        }
    };

    // Ensure that if a shared_ptr RValue is passed into an EBus function
    // the first handler doesn't steal the contents.
    TEST_F(SmartPtr, SharedPtrPassToBusByRValue_AllHandlersReceiveValue)
    {
        TakesSharedPtrHandler handler1;
        TakesSharedPtrHandler handler2;

        SmartPtrTestBus::Broadcast(&SmartPtrTestBus::Events::TakesSharedPtrByCopy, AZStd::make_shared<int>(9));
    }

    TEST_F(SmartPtr, SharedPtrVoidConstPointerCastVoid)
    {
        AZStd::shared_ptr<void const volatile> px;

        AZStd::shared_ptr<void> px2 = AZStd::const_pointer_cast<void>(px);
        EXPECT_TRUE(px2.get() == 0);
    }

    TEST_F(SmartPtr, SharedPtrIntConstPointerCastInt)
    {
        AZStd::shared_ptr<int const volatile> px;

        AZStd::shared_ptr<int> px2 = AZStd::const_pointer_cast<int>(px);
        EXPECT_TRUE(px2.get() == 0);
    }

    TEST_F(SmartPtr, SharedPtrClassConstPointerCastClass)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X const volatile> px;

        AZStd::shared_ptr<X> px2 = AZStd::const_pointer_cast<X>(px);
        EXPECT_TRUE(px2.get() == 0);
    }

    TEST_F(SmartPtr, SharedPtrVoidVolatileConstPointerCastVoid)
    {
        AZStd::shared_ptr<void const volatile> px(new int);

        AZStd::shared_ptr<void> px2 = AZStd::const_pointer_cast<void>(px);
        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(!(px < px2 || px2 < px));
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.use_count() == 2);
    }

    TEST_F(SmartPtr, SharedPtrIntVolatileConstPointerCastInt)
    {
        AZStd::shared_ptr<int const volatile> px(new int);

        AZStd::shared_ptr<int> px2 = AZStd::const_pointer_cast<int>(px);
        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(!(px < px2 || px2 < px));
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.use_count() == 2);
    }

#if !defined(AZ_NO_RTTI)
    TEST_F(SmartPtr, DynamicPointerUpCastNull)
    {
        using SharedPtr::test::X;
        using SharedPtr::test::Y;
        AZStd::shared_ptr<X> pv;
        AZStd::shared_ptr<Y> pw = AZStd::dynamic_pointer_cast<Y>(pv);
        EXPECT_TRUE(pw.get() == 0);
    }

    TEST_F(SmartPtr, DynamicPointerCastUpCastThenDowncastNull)
    {
        using SharedPtr::test::X;
        using SharedPtr::test::Y;
        AZStd::shared_ptr<X> pv(static_cast<X*>(0));

        AZStd::shared_ptr<Y> pw = AZStd::dynamic_pointer_cast<Y>(pv);
        EXPECT_TRUE(pw.get() == 0);

        AZStd::shared_ptr<X> pv2(pw);
        EXPECT_TRUE(pv < pv2 || pv2 < pv);
    }

    TEST_F(SmartPtr, DynamicPointerCastNullDerivedUpcastToDerived)
    {
        using SharedPtr::test::X;
        using SharedPtr::test::Y;
        AZStd::shared_ptr<X> pv(static_cast<Y*>(0));

        AZStd::shared_ptr<Y> pw = AZStd::dynamic_pointer_cast<Y>(pv);
        EXPECT_TRUE(pw.get() == 0);

        AZStd::shared_ptr<X> pv2(pw);
        EXPECT_TRUE(pv < pv2 || pv2 < pv);
    }
#endif
} // namespace UnitTest
#if 0
namespace SharedPtr
{
    namespace n_map
    {
        struct X
        {
        };

        void test()
        {
            AZStd::vector< AZStd::shared_ptr<int> > vi;

            {
                AZStd::shared_ptr<int> pi1(new int);
                AZStd::shared_ptr<int> pi2(new int);
                AZStd::shared_ptr<int> pi3(new int);

                vi.push_back(pi1);
                vi.push_back(pi1);
                vi.push_back(pi1);
                vi.push_back(pi2);
                vi.push_back(pi1);
                vi.push_back(pi2);
                vi.push_back(pi1);
                vi.push_back(pi3);
                vi.push_back(pi3);
                vi.push_back(pi2);
                vi.push_back(pi1);
            }

            AZStd::vector< AZStd::shared_ptr<X> > vx;

            {
                AZStd::shared_ptr<X> px1(new X);
                AZStd::shared_ptr<X> px2(new X);
                AZStd::shared_ptr<X> px3(new X);

                vx.push_back(px2);
                vx.push_back(px2);
                vx.push_back(px1);
                vx.push_back(px2);
                vx.push_back(px1);
                vx.push_back(px1);
                vx.push_back(px1);
                vx.push_back(px2);
                vx.push_back(px1);
                vx.push_back(px3);
                vx.push_back(px2);
            }

            AZStd::map< AZStd::shared_ptr<void>, long > m;

            {
                for (AZStd::vector< AZStd::shared_ptr<int> >::iterator i = vi.begin(); i != vi.end(); ++i)
                {
                    ++m[*i];
                }
            }

            {
                for (AZStd::vector< AZStd::shared_ptr<X> >::iterator i = vx.begin(); i != vx.end(); ++i)
                {
                    ++m[*i];
                }
            }

            {
                for (AZStd::map< AZStd::shared_ptr<void>, long >::iterator i = m.begin(); i != m.end(); ++i)
                {
                    EXPECT_TRUE(i->first.use_count() == i->second + 1);
                }
            }
        }
    } // namespace n_map

    namespace n_transitive
    {
        struct X
        {
            X()
                : next() {}
            AZStd::shared_ptr<X> next;
        };

        void test()
        {
            AZStd::shared_ptr<X> p(new X);
            p->next = AZStd::shared_ptr<X>(new X);
            EXPECT_TRUE(!p->next->next);
            p = p->next;
            EXPECT_TRUE(!p->next);
        }
    } // namespace n_transitive

    namespace n_report_1
    {
        class foo
        {
        public:

            foo()
                : m_self(this)
            {
            }

            void suicide()
            {
                m_self.reset();
            }

        private:

            AZStd::shared_ptr<foo> m_self;
        };

        void test()
        {
            foo* foo_ptr = new foo;
            foo_ptr->suicide();
        }
    } // namespace n_report_1

    // Test case by Per Kristensen
    namespace n_report_2
    {
        class foo
        {
        public:

            void setWeak(AZStd::shared_ptr<foo> s)
            {
                w = s;
            }

        private:

            AZStd::weak_ptr<foo> w;
        };

        class deleter
        {
        public:

            deleter()
                : lock(0)
            {
            }

            ~deleter()
            {
                EXPECT_TRUE(lock == 0);
            }

            void operator() (foo* p)
            {
                ++lock;
                delete p;
                --lock;
            }

        private:

            int lock;
        };

        void test()
        {
            AZStd::shared_ptr<foo> s(new foo, deleter());
            s->setWeak(s);
            s.reset();
        }
    } // namespace n_report_2

    namespace n_spt_incomplete
    {
        class file;

        AZStd::shared_ptr<file> fopen(char const* name, char const* mode);
        void fread(AZStd::shared_ptr<file> f, void* data, long size);

        int file_instances = 0;

        void test()
        {
            EXPECT_TRUE(file_instances == 0);

            {
                AZStd::shared_ptr<file> pf = fopen("name", "mode");
                EXPECT_TRUE(file_instances == 1);
                fread(pf, 0, 17041);
            }

            EXPECT_TRUE(file_instances == 0);
        }
    } // namespace n_spt_incomplete

    namespace n_spt_pimpl
    {
        class file
        {
        private:

            class impl;
            AZStd::shared_ptr<impl> pimpl_;

        public:

            file(char const* name, char const* mode);

            // compiler generated members are fine and useful

            void read(void* data, long size);

            long total_size() const;
        };

        int file_instances = 0;

        void test()
        {
            EXPECT_TRUE(file_instances == 0);

            {
                file f("name", "mode");
                EXPECT_TRUE(file_instances == 1);
                f.read(0, 152);

                file f2(f);
                EXPECT_TRUE(file_instances == 1);
                f2.read(0, 894);

                EXPECT_TRUE(f.total_size() == 152 + 894);

                {
                    file f3("name2", "mode2");
                    EXPECT_TRUE(file_instances == 2);
                }

                EXPECT_TRUE(file_instances == 1);
            }

            EXPECT_TRUE(file_instances == 0);
        }
    } // namespace n_spt_pimpl

    namespace n_spt_abstract
    {
        class X
        {
        public:
            virtual void f(int) = 0;
            virtual int g() = 0;

        protected:
            virtual ~X() {}
        };

        AZStd::shared_ptr<X> createX();

        int X_instances = 0;

        void test()
        {
            EXPECT_TRUE(X_instances == 0);

            {
                AZStd::shared_ptr<X> px = createX();

                EXPECT_TRUE(X_instances == 1);

                px->f(18);
                px->f(152);

                EXPECT_TRUE(px->g() == 170);
            }

            EXPECT_TRUE(X_instances == 0);
        }
    } // namespace n_spt_abstract

    namespace n_spt_preventing_delete
    {
        int X_instances = 0;

        class X
        {
        private:

            X()
            {
                ++X_instances;
            }

            ~X()
            {
                --X_instances;
            }

            class deleter;
            friend class deleter;

            class deleter
            {
            public:

                void operator()(X* p) { delete p; }
            };

        public:

            static AZStd::shared_ptr<X> create()
            {
                AZStd::shared_ptr<X> px(new X, X::deleter());
                return px;
            }
        };

        void test()
        {
            EXPECT_TRUE(X_instances == 0);

            {
                AZStd::shared_ptr<X> px = X::create();
                EXPECT_TRUE(X_instances == 1);
            }

            EXPECT_TRUE(X_instances == 0);
        }
    } // namespace n_spt_preventing_delete

    // WE DON'T support arrays, this will break the alignment on certain platforms.
    //namespace n_spt_array
    //{

    //  int X_instances = 0;

    //  struct X
    //  {
    //      X()
    //      {
    //          ++X_instances;
    //      }

    //      ~X()
    //      {
    //          --X_instances;
    //      }
    //  };

    //  void test()
    //  {
    //      EXPECT_TRUE(X_instances == 0);
    //      {
    //          AZStd::shared_ptr<X> px(new X[4], AZStd::checked_array_deleter<X>());
    //          EXPECT_TRUE(X_instances == 4);
    //      }

    //      EXPECT_TRUE(X_instances == 0);
    //  }

    //} // namespace n_spt_array

    namespace n_spt_static
    {
        class X
        {
        public:

            X()
            {
            }

        private:

            void operator delete(void*)
            {
            }
        };

        struct null_deleter
        {
            void operator()(void const*) const
            {
            }
        };

        static X x;

        void test()
        {
            AZStd::shared_ptr<X> px(&x, null_deleter());
        }
    } // namespace n_spt_static

    namespace n_spt_intrusive
    {
        int X_instances = 0;

        struct X
        {
            long count;

            X()
                : count(0)
            {
                ++X_instances;
            }

            ~X()
            {
                --X_instances;
            }
        };

        void intrusive_ptr_add_ref(X* p)
        {
            ++p->count;
        }

        void intrusive_ptr_release(X* p)
        {
            if (--p->count == 0)
            {
                delete p;
            }
        }

        template<class T>
        struct intrusive_deleter
        {
            void operator()(T* p)
            {
                if (p != 0)
                {
                    intrusive_ptr_release(p);
                }
            }
        };

        AZStd::shared_ptr<X> make_shared_from_intrusive(X* p)
        {
            if (p != 0)
            {
                intrusive_ptr_add_ref(p);
            }
            AZStd::shared_ptr<X> px(p, intrusive_deleter<X>());
            return px;
        }

        void test()
        {
            EXPECT_TRUE(X_instances == 0);

            {
                X* p = new X;
                EXPECT_TRUE(X_instances == 1);
                EXPECT_TRUE(p->count == 0);
                AZStd::shared_ptr<X> px = make_shared_from_intrusive(p);
                EXPECT_TRUE(px.get() == p);
                EXPECT_TRUE(p->count == 1);
                AZStd::shared_ptr<X> px2(px);
                EXPECT_TRUE(px2.get() == p);
                EXPECT_TRUE(p->count == 1);
            }

            EXPECT_TRUE(X_instances == 0);
        }
    } // namespace n_spt_intrusive

    namespace n_spt_another_sp
    {
        template<class T>
        class another_ptr
            : private AZStd::shared_ptr<T>
        {
        private:

            typedef AZStd::shared_ptr<T> base_type;

        public:

            explicit another_ptr(T* p = 0)
                : base_type(p)
            {
            }

            void reset()
            {
                base_type::reset();
            }

            T* get() const
            {
                return base_type::get();
            }
        };

        class event_handler
        {
        public:

            virtual ~event_handler() {}
            virtual void begin() = 0;
            virtual void handle(int event) = 0;
            virtual void end() = 0;
        };

        int begin_called = 0;
        int handle_called = 0;
        int end_called = 0;

        class event_handler_impl
            : public event_handler
        {
        public:

            virtual void begin()
            {
                ++begin_called;
            }

            virtual void handle(int event)
            {
                handle_called = event;
            }

            virtual void end()
            {
                ++end_called;
            }
        };

        another_ptr<event_handler> get_event_handler()
        {
            another_ptr<event_handler> p(new event_handler_impl);
            return p;
        }

        AZStd::shared_ptr<event_handler> current_handler;

        void install_event_handler(AZStd::shared_ptr<event_handler> p)
        {
            p->begin();
            current_handler = p;
        }

        void handle_event(int event)
        {
            current_handler->handle(event);
        }

        void remove_event_handler()
        {
            current_handler->end();
            current_handler.reset();
        }

        template<class P>
        class smart_pointer_deleter
        {
        private:

            P p_;

        public:

            smart_pointer_deleter(P const& p)
                : p_(p)
            {
            }

            void operator()(void const*)
            {
                p_.reset();
            }
        };

        void test()
        {
            another_ptr<event_handler> p = get_event_handler();

            AZStd::shared_ptr<event_handler> q(p.get(), smart_pointer_deleter< another_ptr<event_handler> >(p));

            p.reset();

            EXPECT_TRUE(begin_called == 0);

            install_event_handler(q);

            EXPECT_TRUE(begin_called == 1);

            EXPECT_TRUE(handle_called == 0);

            handle_event(17041);

            EXPECT_TRUE(handle_called == 17041);

            EXPECT_TRUE(end_called == 0);

            remove_event_handler();

            EXPECT_TRUE(end_called == 1);
        }
    } // namespace n_spt_another_sp

    namespace n_spt_shared_from_this
    {
        class X
        {
        public:
            virtual void f() = 0;

        protected:

            virtual ~X() {}
        };

        class Y
        {
        public:
            virtual AZStd::shared_ptr<X> getX() = 0;

        protected:

            virtual ~Y() {}
        };

        class impl
            : public X
            , public Y
        {
        private:

            AZStd::weak_ptr<impl> weak_this;

            impl(impl const&);
            impl& operator=(impl const&);

            impl() {}
        public:
            virtual ~impl() {}
            static AZStd::shared_ptr<impl> create()
            {
                AZStd::shared_ptr<impl> pi(new impl);
                pi->weak_this = pi;
                return pi;
            }

            virtual void f() {}

            virtual AZStd::shared_ptr<X> getX()
            {
                AZStd::shared_ptr<X> px = weak_this.lock();
                return px;
            }
        };

        void test()
        {
            AZStd::shared_ptr<Y> py = impl::create();
            EXPECT_TRUE(py.get() != 0);
            EXPECT_TRUE(py.use_count() == 1);

            AZStd::shared_ptr<X> px = py->getX();
            EXPECT_TRUE(px.get() != 0);
            EXPECT_TRUE(py.use_count() == 2);

#if !defined(AZ_NO_RTTI)
            AZStd::shared_ptr<Y> py2 = AZStd::dynamic_pointer_cast<Y>(px);
            EXPECT_TRUE(py.get() == py2.get());
            EXPECT_TRUE(!(py < py2 || py2 < py));
            EXPECT_TRUE(py.use_count() == 3);
#endif
        }
    } // namespace n_spt_shared_from_this

    namespace n_spt_wrap
    {
        void test()
        {
        }
    } // namespace n_spt_wrap


    struct SharedPointerTest
    {
        void* m_memBlock;
    public:
        SharedPointerTest()
        {
            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_numMemoryBlocks = 1;
            desc.m_heap.m_memoryBlocksByteSize[0] = 15 * 1024 * 1024;
            m_memBlock = UnitTest::DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_memoryBlocks[0] = m_memBlock;

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        ~SharedPointerTest()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            UnitTest::DebugAlignFree(m_memBlock);
        }
        void run()
        {
            n_element_type::test();
            test::test();
            n_assignment::test();
            n_reset::test();
            n_access::test();
            n_use_count::test();
            n_swap::test();
            n_comparison::test();
            n_static_cast::test();
            n_const_cast::test();
    #if !defined(AZ_NO_RTTI)
            n_dynamic_cast::test();
    #endif

            n_map::test();

            n_transitive::test();
            n_report_1::test();
            n_report_2::test();

            n_spt_incomplete::test();
            n_spt_pimpl::test();
            n_spt_abstract::test();
            n_spt_preventing_delete::test();
            //n_spt_array::test();
            n_spt_static::test();
            n_spt_intrusive::test();
            n_spt_another_sp::test();
            n_spt_shared_from_this::test();
            n_spt_wrap::test();
        }
    };

    namespace n_spt_incomplete
    {
        class file
        {
        public:

            file()
                : fread_called(false)
            {
                ++file_instances;
            }

            ~file()
            {
                EXPECT_TRUE(fread_called);
                --file_instances;
            }

            bool fread_called;
        };

        AZStd::shared_ptr<file> fopen(char const*, char const*)
        {
            AZStd::shared_ptr<file> pf(new file);
            return pf;
        }

        void fread(AZStd::shared_ptr<file> pf, void*, long)
        {
            pf->fread_called = true;
        }
    } // namespace n_spt_incomplete

    namespace n_spt_pimpl
    {
        class file::impl
        {
        private:

            impl(impl const&);
            impl& operator=(impl const&);

            long total_size_;

        public:

            impl(char const*, char const*)
                : total_size_(0)
            {
                ++file_instances;
            }

            ~impl()
            {
                --file_instances;
            }

            void read(void*, long size)
            {
                total_size_ += size;
            }

            long total_size() const
            {
                return total_size_;
            }
        };

        file::file(char const* name, char const* mode)
            : pimpl_(new impl(name, mode))
        {
        }

        void file::read(void* data, long size)
        {
            pimpl_->read(data, size);
        }

        long file::total_size() const
        {
            return pimpl_->total_size();
        }
    } // namespace n_spt_pimpl

    namespace n_spt_abstract
    {
        class X_impl
            : public X
        {
        private:

            X_impl(X_impl const&);
            X_impl& operator=(X_impl const&);
            int n_;

        public:

            X_impl()
                : n_(0)
            {
                ++X_instances;
            }

            virtual ~X_impl()
            {
                --X_instances;
            }

            virtual void f(int n)
            {
                n_ += n;
            }

            virtual int g()
            {
                return n_;
            }
        };

        AZStd::shared_ptr<X> createX()
        {
            AZStd::shared_ptr<X> px(new X_impl);
            return px;
        }
    } // namespace n_spt_abstract

    //////////////////////////////////////////////////////////////////////////
    // Shared ptr multithread test
    class SharedPointerMultiThreadTest
    {
#if defined(AZ_PLATFORM_ANDROID)
        static int const n = 256 * 1024;
        static const int numThreads = 4;
#elif defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
        static int const n = 1024 * 1024;
        static const int numThreads = 16;
#endif
        void test(AZStd::shared_ptr<int> const& pi)
        {
            AZStd::vector< AZStd::shared_ptr<int> > v;

            for (int i = 0; i < n; ++i)
            {
                v.push_back(pi);
            }
        }

        void* m_memBlock;
    public:
        SharedPointerMultiThreadTest()
        {
            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_numMemoryBlocks = 1;
#if   defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
            desc.m_heap.m_memoryBlocksByteSize[0] = 800 * 1024 * 1024;
#elif defined(AZ_PLATFORM_ANDROID)
            desc.m_heap.m_memoryBlocksByteSize[0] = 400 * 1024 * 1024;
#endif
            m_memBlock = UnitTest::DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_memoryBlocks[0] = m_memBlock;

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        ~SharedPointerMultiThreadTest()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            UnitTest::DebugAlignFree(m_memBlock);
        }

        void run()
        {
            //using namespace std; // printf, clock_t, clock

            AZStd::shared_ptr<int> pi(new int(42));

            //clock_t t = clock();

            AZStd::thread a[ numThreads ];

            for (int i = 0; i < numThreads; ++i)
            {
                a[i] = AZStd::thread(AZStd::bind(&SharedPointerMultiThreadTest::test, this, pi));
            }

            for (int j = 0; j < numThreads; ++j)
            {
                a[j].join();
            }

            //t = clock() - t;
            //printf( "\n\n%.3f seconds.\n", static_cast<double>(t) / CLOCKS_PER_SEC );
        }
    };

    //////////////////////////////////////////////////////////////////////////

    class X
    {
    private:
        X(X const&);
        X& operator=(X const&);
        void* operator new(std::size_t n)
        {
            return ::operator new(n);
        }
        void operator delete(void* p)
        {
            ::operator delete(p);
        }
    public:
        static int instances;
        int v;
        explicit X(int a1 = 0, int a2 = 0, int a3 = 0, int a4 = 0, int a5 = 0, int a6 = 0, int a7 = 0, int a8 = 0, int a9 = 0)
            : v(a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9)
        {
            ++instances;
        }
        ~X()
        {
            --instances;
        }
    };

    int X::instances = 0;

    class MakeSharedPtrTest
    {
        void* m_memBlock;
    public:
        MakeSharedPtrTest()
        {
            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_numMemoryBlocks = 1;
            desc.m_heap.m_memoryBlocksByteSize[0] = 15 * 1024 * 1024;
            m_memBlock = UnitTest::DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_memoryBlocks[0] = m_memBlock;

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        ~MakeSharedPtrTest()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            UnitTest::DebugAlignFree(m_memBlock);
        }
        void run()
        {
            {
                AZStd::shared_ptr< int > pi = AZStd::make_shared< int >();

                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(*pi == 0);
            }

            {
                AZStd::shared_ptr< int > pi = AZStd::make_shared< int >(5);

                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(*pi == 5);
            }

            EXPECT_TRUE(X::instances == 0);

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >();
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 0);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >(1);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >(1, 2);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >(1, 2, 3);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2 + 3);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >(1, 2, 3, 4);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >(1, 2, 3, 4, 5);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4 + 5);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            /*{
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >( 1, 2, 3, 4, 5, 6 );
                AZStd::weak_ptr<X> wp( pi );

                EXPECT_TRUE( X::instances == 1 );
                EXPECT_TRUE( pi.get() != 0 );
                EXPECT_TRUE( pi->v == 1+2+3+4+5+6 );

                pi.reset();

                EXPECT_TRUE( X::instances == 0 );
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >( 1, 2, 3, 4, 5, 6, 7 );
                AZStd::weak_ptr<X> wp( pi );

                EXPECT_TRUE( X::instances == 1 );
                EXPECT_TRUE( pi.get() != 0 );
                EXPECT_TRUE( pi->v == 1+2+3+4+5+6+7 );

                pi.reset();

                EXPECT_TRUE( X::instances == 0 );
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >( 1, 2, 3, 4, 5, 6, 7, 8 );
                AZStd::weak_ptr<X> wp( pi );

                EXPECT_TRUE( X::instances == 1 );
                EXPECT_TRUE( pi.get() != 0 );
                EXPECT_TRUE( pi->v == 1+2+3+4+5+6+7+8 );

                pi.reset();

                EXPECT_TRUE( X::instances == 0 );
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::make_shared< X >( 1, 2, 3, 4, 5, 6, 7, 8, 9 );
                AZStd::weak_ptr<X> wp( pi );

                EXPECT_TRUE( X::instances == 1 );
                EXPECT_TRUE( pi.get() != 0 );
                EXPECT_TRUE( pi->v == 1+2+3+4+5+6+7+8+9 );

                pi.reset();

                EXPECT_TRUE( X::instances == 0 );
            }*/

            //
            AZStd::allocator myalloc("My test allocator");

            {
                AZStd::shared_ptr< int > pi = AZStd::allocate_shared< int >(myalloc);

                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(*pi == 0);
            }

            {
                AZStd::shared_ptr< int > pi = AZStd::allocate_shared< int >(myalloc, 5);

                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(*pi == 5);
            }

            EXPECT_TRUE(X::instances == 0);

            {
                AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 0);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2, 3);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2 + 3);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2, 3, 4);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            {
                AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2, 3, 4, 5);
                AZStd::weak_ptr<X> wp(pi);

                EXPECT_TRUE(X::instances == 1);
                EXPECT_TRUE(pi.get() != 0);
                EXPECT_TRUE(pi->v == 1 + 2 + 3 + 4 + 5);

                pi.reset();

                EXPECT_TRUE(X::instances == 0);
            }

            /*{
            AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2, 3, 4, 5, 6 );
            AZStd::weak_ptr<X> wp( pi );

            EXPECT_TRUE( X::instances == 1 );
            EXPECT_TRUE( pi.get() != 0 );
            EXPECT_TRUE( pi->v == 1+2+3+4+5+6 );

            pi.reset();

            EXPECT_TRUE( X::instances == 0 );
            }

            {
            AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2, 3, 4, 5, 6, 7 );
            AZStd::weak_ptr<X> wp( pi );

            EXPECT_TRUE( X::instances == 1 );
            EXPECT_TRUE( pi.get() != 0 );
            EXPECT_TRUE( pi->v == 1+2+3+4+5+6+7 );

            pi.reset();

            EXPECT_TRUE( X::instances == 0 );
            }

            {
            AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2, 3, 4, 5, 6, 7, 8 );
            AZStd::weak_ptr<X> wp( pi );

            EXPECT_TRUE( X::instances == 1 );
            EXPECT_TRUE( pi.get() != 0 );
            EXPECT_TRUE( pi->v == 1+2+3+4+5+6+7+8 );

            pi.reset();

            EXPECT_TRUE( X::instances == 0 );
            }

            {
            AZStd::shared_ptr< X > pi = AZStd::allocate_shared< X >(myalloc, 1, 2, 3, 4, 5, 6, 7, 8, 9 );
            AZStd::weak_ptr<X> wp( pi );

            EXPECT_TRUE( X::instances == 1 );
            EXPECT_TRUE( pi.get() != 0 );
            EXPECT_TRUE( pi->v == 1+2+3+4+5+6+7+8+9 );

            pi.reset();

            EXPECT_TRUE( X::instances == 0 );
            }*/

            // test alignment and in-place deleters
            {
                AZStd::shared_ptr<uint64_t> p = AZStd::make_shared<uint64_t>(8);
                *p = 100;
                EXPECT_TRUE(*p == 100);
            }
        }
    };
}
}

namespace WeakPtr
{
    namespace n_element_type
    {
        void f(int&)
        {
        }

        void test()
        {
            typedef AZStd::weak_ptr<int>::element_type T;
            T t;
            f(t);
        }
    } // namespace n_element_type

    class incomplete;

    AZStd::shared_ptr<incomplete> create_incomplete();

    struct X
    {
        int dummy;
    };

    struct Y
    {
        int dummy2;
    };

    struct Z
        : public X
        , public virtual Y
    {
    };

    namespace n_constructors
    {
        void default_constructor()
        {
            {
                AZStd::weak_ptr<int> wp;
                EXPECT_TRUE(wp.use_count() == 0);
            }

            {
                AZStd::weak_ptr<void> wp;
                EXPECT_TRUE(wp.use_count() == 0);
            }

            {
                AZStd::weak_ptr<incomplete> wp;
                EXPECT_TRUE(wp.use_count() == 0);
            }
        }

        void shared_ptr_constructor()
        {
            {
                AZStd::shared_ptr<int> sp;

                AZStd::weak_ptr<int> wp(sp);
                EXPECT_TRUE(wp.use_count() == sp.use_count());

                AZStd::weak_ptr<void> wp2(sp);
                EXPECT_TRUE(wp2.use_count() == sp.use_count());
            }

            {
                AZStd::shared_ptr<int> sp(static_cast<int*>(0));

                {
                    AZStd::weak_ptr<int> wp(sp);
                    EXPECT_TRUE(wp.use_count() == sp.use_count());
                    EXPECT_TRUE(wp.use_count() == 1);
                    AZStd::shared_ptr<int> sp2(wp);
                    EXPECT_TRUE(wp.use_count() == 2);
                    EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
                }

                {
                    AZStd::weak_ptr<void> wp(sp);
                    EXPECT_TRUE(wp.use_count() == sp.use_count());
                    EXPECT_TRUE(wp.use_count() == 1);
                    AZStd::shared_ptr<void> sp2(wp);
                    EXPECT_TRUE(wp.use_count() == 2);
                    EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
                }
            }

            {
                AZStd::shared_ptr<int> sp(new int);

                {
                    AZStd::weak_ptr<int> wp(sp);
                    EXPECT_TRUE(wp.use_count() == sp.use_count());
                    EXPECT_TRUE(wp.use_count() == 1);
                    AZStd::shared_ptr<int> sp2(wp);
                    EXPECT_TRUE(wp.use_count() == 2);
                    EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
                }

                {
                    AZStd::weak_ptr<void> wp(sp);
                    EXPECT_TRUE(wp.use_count() == sp.use_count());
                    EXPECT_TRUE(wp.use_count() == 1);
                    AZStd::shared_ptr<void> sp2(wp);
                    EXPECT_TRUE(wp.use_count() == 2);
                    EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
                }
            }

            {
                AZStd::shared_ptr<void> sp;

                AZStd::weak_ptr<void> wp(sp);
                EXPECT_TRUE(wp.use_count() == sp.use_count());
            }

            {
                AZStd::shared_ptr<void> sp(static_cast<int*>(0));

                AZStd::weak_ptr<void> wp(sp);
                EXPECT_TRUE(wp.use_count() == sp.use_count());
                EXPECT_TRUE(wp.use_count() == 1);
                AZStd::shared_ptr<void> sp2(wp);
                EXPECT_TRUE(wp.use_count() == 2);
                EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
            }

            {
                AZStd::shared_ptr<void> sp(new int);

                AZStd::weak_ptr<void> wp(sp);
                EXPECT_TRUE(wp.use_count() == sp.use_count());
                EXPECT_TRUE(wp.use_count() == 1);
                AZStd::shared_ptr<void> sp2(wp);
                EXPECT_TRUE(wp.use_count() == 2);
                EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
            }

            {
                AZStd::shared_ptr<incomplete> sp;

                AZStd::weak_ptr<incomplete> wp(sp);
                EXPECT_TRUE(wp.use_count() == sp.use_count());

                AZStd::weak_ptr<void> wp2(sp);
                EXPECT_TRUE(wp2.use_count() == sp.use_count());
            }

            {
                AZStd::shared_ptr<incomplete> sp = create_incomplete();

                {
                    AZStd::weak_ptr<incomplete> wp(sp);
                    EXPECT_TRUE(wp.use_count() == sp.use_count());
                    EXPECT_TRUE(wp.use_count() == 1);
                    AZStd::shared_ptr<incomplete> sp2(wp);
                    EXPECT_TRUE(wp.use_count() == 2);
                    EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
                }

                {
                    AZStd::weak_ptr<void> wp(sp);
                    EXPECT_TRUE(wp.use_count() == sp.use_count());
                    EXPECT_TRUE(wp.use_count() == 1);
                    AZStd::shared_ptr<void> sp2(wp);
                    EXPECT_TRUE(wp.use_count() == 2);
                    EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
                }
            }

            {
                AZStd::shared_ptr<void> sp = create_incomplete();

                AZStd::weak_ptr<void> wp(sp);
                EXPECT_TRUE(wp.use_count() == sp.use_count());
                EXPECT_TRUE(wp.use_count() == 1);
                AZStd::shared_ptr<void> sp2(wp);
                EXPECT_TRUE(wp.use_count() == 2);
                EXPECT_TRUE(!(sp < sp2 || sp2 < sp));
            }
        }

        void copy_constructor()
        {
            {
                AZStd::weak_ptr<int> wp;
                AZStd::weak_ptr<int> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 0);
            }

            {
                AZStd::weak_ptr<void> wp;
                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 0);
            }

            {
                AZStd::weak_ptr<incomplete> wp;
                AZStd::weak_ptr<incomplete> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 0);
            }

            {
                AZStd::shared_ptr<int> sp(static_cast<int*>(0));
                AZStd::weak_ptr<int> wp(sp);

                AZStd::weak_ptr<int> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<int> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<int> sp(new int);
                AZStd::weak_ptr<int> wp(sp);

                AZStd::weak_ptr<int> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<int> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<void> sp(static_cast<int*>(0));
                AZStd::weak_ptr<void> wp(sp);

                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<void> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<void> sp(new int);
                AZStd::weak_ptr<void> wp(sp);

                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<void> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<incomplete> sp = create_incomplete();
                AZStd::weak_ptr<incomplete> wp(sp);

                AZStd::weak_ptr<incomplete> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<incomplete> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }
        }

        void conversion_constructor()
        {
            {
                AZStd::weak_ptr<int> wp;
                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 0);
            }

            {
                AZStd::weak_ptr<incomplete> wp;
                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 0);
            }

            {
                AZStd::weak_ptr<Z> wp;

                AZStd::weak_ptr<X> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 0);

                AZStd::weak_ptr<Y> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
            }

            {
                AZStd::shared_ptr<int> sp(static_cast<int*>(0));
                AZStd::weak_ptr<int> wp(sp);

                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<void> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<int> sp(new int);
                AZStd::weak_ptr<int> wp(sp);

                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<void> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<incomplete> sp = create_incomplete();
                AZStd::weak_ptr<incomplete> wp(sp);

                AZStd::weak_ptr<void> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<void> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<Z> sp(static_cast<Z*>(0));
                AZStd::weak_ptr<Z> wp(sp);

                AZStd::weak_ptr<X> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<X> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<Z> sp(static_cast<Z*>(0));
                AZStd::weak_ptr<Z> wp(sp);

                AZStd::weak_ptr<Y> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<Y> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<Z> sp(new Z);
                AZStd::weak_ptr<Z> wp(sp);

                AZStd::weak_ptr<X> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<X> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }

            {
                AZStd::shared_ptr<Z> sp(new Z);
                AZStd::weak_ptr<Z> wp(sp);

                AZStd::weak_ptr<Y> wp2(wp);
                EXPECT_TRUE(wp2.use_count() == wp.use_count());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                sp.reset();
                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));

                AZStd::weak_ptr<Y> wp3(wp);
                EXPECT_TRUE(wp3.use_count() == wp.use_count());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
            }
        }

        void test()
        {
            default_constructor();
            shared_ptr_constructor();
            copy_constructor();
            conversion_constructor();
        }
    } // namespace n_constructors

    namespace n_assignment
    {
        template<class T>
        void copy_assignment(AZStd::shared_ptr<T>& sp)
        {
            EXPECT_TRUE(sp.unique());

            AZStd::weak_ptr<T> p1;

            p1 = p1;
            EXPECT_TRUE(p1.use_count() == 0);

            AZStd::weak_ptr<T> p2;

            p1 = p2;
            EXPECT_TRUE(p1.use_count() == 0);

            AZStd::weak_ptr<T> p3(p1);

            p1 = p3;
            EXPECT_TRUE(p1.use_count() == 0);

            AZStd::weak_ptr<T> p4(sp);

            p4 = p4;
            EXPECT_TRUE(p4.use_count() == 1);

            p1 = p4;
            EXPECT_TRUE(p1.use_count() == 1);

            p4 = p2;
            EXPECT_TRUE(p4.use_count() == 0);

            sp.reset();

            p1 = p1;
            EXPECT_TRUE(p1.use_count() == 0);

            p4 = p1;
            EXPECT_TRUE(p4.use_count() == 0);
        }

        void conversion_assignment()
        {
            {
                AZStd::weak_ptr<void> p1;

                AZStd::weak_ptr<incomplete> p2;

                p1 = p2;
                EXPECT_TRUE(p1.use_count() == 0);

                AZStd::shared_ptr<incomplete> sp = create_incomplete();
                AZStd::weak_ptr<incomplete> p3(sp);

                p1 = p3;
                EXPECT_TRUE(p1.use_count() == 1);

                sp.reset();

                p1 = p3;
                EXPECT_TRUE(p1.use_count() == 0);

                p1 = p2;
                EXPECT_TRUE(p1.use_count() == 0);
            }

            {
                AZStd::weak_ptr<X> p1;

                AZStd::weak_ptr<Z> p2;

                p1 = p2;
                EXPECT_TRUE(p1.use_count() == 0);

                AZStd::shared_ptr<Z> sp(new Z);
                AZStd::weak_ptr<Z> p3(sp);

                p1 = p3;
                EXPECT_TRUE(p1.use_count() == 1);

                sp.reset();

                p1 = p3;
                EXPECT_TRUE(p1.use_count() == 0);

                p1 = p2;
                EXPECT_TRUE(p1.use_count() == 0);
            }

            {
                AZStd::weak_ptr<Y> p1;

                AZStd::weak_ptr<Z> p2;

                p1 = p2;
                EXPECT_TRUE(p1.use_count() == 0);

                AZStd::shared_ptr<Z> sp(new Z);
                AZStd::weak_ptr<Z> p3(sp);

                p1 = p3;
                EXPECT_TRUE(p1.use_count() == 1);

                sp.reset();

                p1 = p3;
                EXPECT_TRUE(p1.use_count() == 0);

                p1 = p2;
                EXPECT_TRUE(p1.use_count() == 0);
            }
        }

        template<class T, class U>
        void shared_ptr_assignment(AZStd::shared_ptr<U>& sp, T* = 0)
        {
            EXPECT_TRUE(sp.unique());

            AZStd::weak_ptr<T> p1;
            AZStd::weak_ptr<T> p2(p1);
            AZStd::weak_ptr<T> p3(sp);
            AZStd::weak_ptr<T> p4(p3);

            p1 = sp;
            EXPECT_TRUE(p1.use_count() == 1);

            p2 = sp;
            EXPECT_TRUE(p2.use_count() == 1);

            p3 = sp;
            EXPECT_TRUE(p3.use_count() == 1);

            p4 = sp;
            EXPECT_TRUE(p4.use_count() == 1);

            sp.reset();

            EXPECT_TRUE(p1.use_count() == 0);
            EXPECT_TRUE(p2.use_count() == 0);
            EXPECT_TRUE(p3.use_count() == 0);
            EXPECT_TRUE(p4.use_count() == 0);

            p1 = sp;
        }

        void test()
        {
            {
                AZStd::shared_ptr<int> p(new int);
                copy_assignment(p);
            }

            {
                AZStd::shared_ptr<X> p(new X);
                copy_assignment(p);
            }

            {
                AZStd::shared_ptr<void> p(new int);
                copy_assignment(p);
            }

            {
                AZStd::shared_ptr<incomplete> p = create_incomplete();
                copy_assignment(p);
            }

            conversion_assignment();

            {
                AZStd::shared_ptr<int> p(new int);
                shared_ptr_assignment<int>(p);
            }

            {
                AZStd::shared_ptr<int> p(new int);
                shared_ptr_assignment<void>(p);
            }

            {
                AZStd::shared_ptr<X> p(new X);
                shared_ptr_assignment<X>(p);
            }

            {
                AZStd::shared_ptr<X> p(new X);
                shared_ptr_assignment<void>(p);
            }

            {
                AZStd::shared_ptr<void> p(new int);
                shared_ptr_assignment<void>(p);
            }

            {
                AZStd::shared_ptr<incomplete> p = create_incomplete();
                shared_ptr_assignment<incomplete>(p);
            }

            {
                AZStd::shared_ptr<incomplete> p = create_incomplete();
                shared_ptr_assignment<void>(p);
            }
        }
    } // namespace n_assignment

    namespace n_reset
    {
        template<class T, class U>
        void test2(AZStd::shared_ptr<U>& sp, T* = 0)
        {
            EXPECT_TRUE(sp.unique());

            AZStd::weak_ptr<T> p1;
            AZStd::weak_ptr<T> p2(p1);
            AZStd::weak_ptr<T> p3(sp);
            AZStd::weak_ptr<T> p4(p3);
            AZStd::weak_ptr<T> p5(sp);
            AZStd::weak_ptr<T> p6(p5);

            p1.reset();
            EXPECT_TRUE(p1.use_count() == 0);

            p2.reset();
            EXPECT_TRUE(p2.use_count() == 0);

            p3.reset();
            EXPECT_TRUE(p3.use_count() == 0);

            p4.reset();
            EXPECT_TRUE(p4.use_count() == 0);

            sp.reset();

            p5.reset();
            EXPECT_TRUE(p5.use_count() == 0);

            p6.reset();
            EXPECT_TRUE(p6.use_count() == 0);
        }

        void test()
        {
            {
                AZStd::shared_ptr<int> p(new int);
                test2<int>(p);
            }

            {
                AZStd::shared_ptr<int> p(new int);
                test2<void>(p);
            }

            {
                AZStd::shared_ptr<X> p(new X);
                test2<X>(p);
            }

            {
                AZStd::shared_ptr<X> p(new X);
                test2<void>(p);
            }

            {
                AZStd::shared_ptr<void> p(new int);
                test2<void>(p);
            }

            {
                AZStd::shared_ptr<incomplete> p = create_incomplete();
                test2<incomplete>(p);
            }

            {
                AZStd::shared_ptr<incomplete> p = create_incomplete();
                test2<void>(p);
            }
        }
    } // namespace n_reset

    namespace n_use_count
    {
        void test()
        {
            {
                AZStd::weak_ptr<X> wp;
                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp.expired());

                AZStd::weak_ptr<X> wp2;
                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp.expired());

                AZStd::weak_ptr<X> wp3(wp);
                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp.expired());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(wp3.expired());
            }

            {
                AZStd::shared_ptr<X> sp(static_cast<X*>(0));

                AZStd::weak_ptr<X> wp(sp);
                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(!wp.expired());

                AZStd::weak_ptr<X> wp2(sp);
                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(!wp.expired());

                AZStd::weak_ptr<X> wp3(wp);
                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(!wp.expired());
                EXPECT_TRUE(wp3.use_count() == 1);
                EXPECT_TRUE(!wp3.expired());

                AZStd::shared_ptr<X> sp2(sp);

                EXPECT_TRUE(wp.use_count() == 2);
                EXPECT_TRUE(!wp.expired());
                EXPECT_TRUE(wp2.use_count() == 2);
                EXPECT_TRUE(!wp2.expired());
                EXPECT_TRUE(wp3.use_count() == 2);
                EXPECT_TRUE(!wp3.expired());

                AZStd::shared_ptr<void> sp3(sp);

                EXPECT_TRUE(wp.use_count() == 3);
                EXPECT_TRUE(!wp.expired());
                EXPECT_TRUE(wp2.use_count() == 3);
                EXPECT_TRUE(!wp2.expired());
                EXPECT_TRUE(wp3.use_count() == 3);
                EXPECT_TRUE(!wp3.expired());

                sp.reset();

                EXPECT_TRUE(wp.use_count() == 2);
                EXPECT_TRUE(!wp.expired());
                EXPECT_TRUE(wp2.use_count() == 2);
                EXPECT_TRUE(!wp2.expired());
                EXPECT_TRUE(wp3.use_count() == 2);
                EXPECT_TRUE(!wp3.expired());

                sp2.reset();

                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(!wp.expired());
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!wp2.expired());
                EXPECT_TRUE(wp3.use_count() == 1);
                EXPECT_TRUE(!wp3.expired());

                sp3.reset();

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp.expired());
                EXPECT_TRUE(wp2.use_count() == 0);
                EXPECT_TRUE(wp2.expired());
                EXPECT_TRUE(wp3.use_count() == 0);
                EXPECT_TRUE(wp3.expired());
            }
        }
    } // namespace n_use_count

    namespace n_swap
    {
        void test()
        {
            {
                AZStd::weak_ptr<X> wp;
                AZStd::weak_ptr<X> wp2;

                wp.swap(wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 0);

                using std::swap;
                swap(wp, wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 0);
            }

            {
                AZStd::shared_ptr<X> sp(new X);
                AZStd::weak_ptr<X> wp;
                AZStd::weak_ptr<X> wp2(sp);
                AZStd::weak_ptr<X> wp3(sp);

                wp.swap(wp2);

                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(wp2.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

                using std::swap;
                swap(wp, wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));

                sp.reset();

                wp.swap(wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

                swap(wp, wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 0);
                EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));
            }

            {
                AZStd::shared_ptr<X> sp(new X);
                AZStd::shared_ptr<X> sp2(new X);
                AZStd::weak_ptr<X> wp(sp);
                AZStd::weak_ptr<X> wp2(sp2);
                AZStd::weak_ptr<X> wp3(sp2);

                wp.swap(wp2);

                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

                using std::swap;
                swap(wp, wp2);

                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));

                sp.reset();

                wp.swap(wp2);

                EXPECT_TRUE(wp.use_count() == 1);
                EXPECT_TRUE(wp2.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

                swap(wp, wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 1);
                EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));

                sp2.reset();

                wp.swap(wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 0);
                EXPECT_TRUE(!(wp < wp3 || wp3 < wp));

                swap(wp, wp2);

                EXPECT_TRUE(wp.use_count() == 0);
                EXPECT_TRUE(wp2.use_count() == 0);
                EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));
            }
        }
    } // namespace n_swap

    namespace n_comparison
    {
        void test()
        {
            {
                AZStd::weak_ptr<X> wp;
                EXPECT_TRUE(!(wp < wp));

                AZStd::weak_ptr<X> wp2;
                EXPECT_TRUE(!(wp < wp2 && wp2 < wp));

                AZStd::weak_ptr<X> wp3(wp);
                EXPECT_TRUE(!(wp3 < wp3));
                EXPECT_TRUE(!(wp < wp3 && wp3 < wp));
            }

            {
                AZStd::shared_ptr<X> sp(new X);

                AZStd::weak_ptr<X> wp(sp);
                EXPECT_TRUE(!(wp < wp));

                AZStd::weak_ptr<X> wp2;
                EXPECT_TRUE(wp < wp2 || wp2 < wp);
                EXPECT_TRUE(!(wp < wp2 && wp2 < wp));

                bool b1 = wp < wp2;
                bool b2 = wp2 < wp;

                {
                    AZStd::weak_ptr<X> wp3(wp);

                    EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
                    EXPECT_TRUE(!(wp < wp3 && wp3 < wp));

                    EXPECT_TRUE(wp2 < wp3 || wp3 < wp2);
                    EXPECT_TRUE(!(wp2 < wp3 && wp3 < wp2));

                    AZStd::weak_ptr<X> wp4(wp2);

                    EXPECT_TRUE(wp4 < wp3 || wp3 < wp4);
                    EXPECT_TRUE(!(wp4 < wp3 && wp3 < wp4));
                }

                sp.reset();

                EXPECT_TRUE(b1 == (wp < wp2));
                EXPECT_TRUE(b2 == (wp2 < wp));

                {
                    AZStd::weak_ptr<X> wp3(wp);

                    EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
                    EXPECT_TRUE(!(wp < wp3 && wp3 < wp));

                    EXPECT_TRUE(wp2 < wp3 || wp3 < wp2);
                    EXPECT_TRUE(!(wp2 < wp3 && wp3 < wp2));

                    AZStd::weak_ptr<X> wp4(wp2);

                    EXPECT_TRUE(wp4 < wp3 || wp3 < wp4);
                    EXPECT_TRUE(!(wp4 < wp3 && wp3 < wp4));
                }
            }

            {
                AZStd::shared_ptr<X> sp(new X);
                AZStd::shared_ptr<X> sp2(new X);

                AZStd::weak_ptr<X> wp(sp);
                AZStd::weak_ptr<X> wp2(sp2);

                EXPECT_TRUE(wp < wp2 || wp2 < wp);
                EXPECT_TRUE(!(wp < wp2 && wp2 < wp));

                bool b1 = wp < wp2;
                bool b2 = wp2 < wp;

                {
                    AZStd::weak_ptr<X> wp3(wp);

                    EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
                    EXPECT_TRUE(!(wp < wp3 && wp3 < wp));

                    EXPECT_TRUE(wp2 < wp3 || wp3 < wp2);
                    EXPECT_TRUE(!(wp2 < wp3 && wp3 < wp2));

                    AZStd::weak_ptr<X> wp4(wp2);

                    EXPECT_TRUE(wp4 < wp3 || wp3 < wp4);
                    EXPECT_TRUE(!(wp4 < wp3 && wp3 < wp4));
                }

                sp.reset();

                EXPECT_TRUE(b1 == (wp < wp2));
                EXPECT_TRUE(b2 == (wp2 < wp));

                {
                    AZStd::weak_ptr<X> wp3(wp);

                    EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
                    EXPECT_TRUE(!(wp < wp3 && wp3 < wp));

                    EXPECT_TRUE(wp2 < wp3 || wp3 < wp2);
                    EXPECT_TRUE(!(wp2 < wp3 && wp3 < wp2));

                    AZStd::weak_ptr<X> wp4(wp2);

                    EXPECT_TRUE(wp4 < wp3 || wp3 < wp4);
                    EXPECT_TRUE(!(wp4 < wp3 && wp3 < wp4));
                }

                sp2.reset();

                EXPECT_TRUE(b1 == (wp < wp2));
                EXPECT_TRUE(b2 == (wp2 < wp));

                {
                    AZStd::weak_ptr<X> wp3(wp);

                    EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
                    EXPECT_TRUE(!(wp < wp3 && wp3 < wp));

                    EXPECT_TRUE(wp2 < wp3 || wp3 < wp2);
                    EXPECT_TRUE(!(wp2 < wp3 && wp3 < wp2));

                    AZStd::weak_ptr<X> wp4(wp2);

                    EXPECT_TRUE(wp4 < wp3 || wp3 < wp4);
                    EXPECT_TRUE(!(wp4 < wp3 && wp3 < wp4));
                }
            }

            {
                AZStd::shared_ptr<X> sp(new X);
                AZStd::shared_ptr<X> sp2(sp);

                AZStd::weak_ptr<X> wp(sp);
                AZStd::weak_ptr<X> wp2(sp2);

                EXPECT_TRUE(!(wp < wp2 || wp2 < wp));
                EXPECT_TRUE(!(wp < wp2 && wp2 < wp));

                bool b1 = wp < wp2;
                bool b2 = wp2 < wp;

                {
                    AZStd::weak_ptr<X> wp3(wp);

                    EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
                    EXPECT_TRUE(!(wp < wp3 && wp3 < wp));

                    EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));
                    EXPECT_TRUE(!(wp2 < wp3 && wp3 < wp2));

                    AZStd::weak_ptr<X> wp4(wp2);

                    EXPECT_TRUE(!(wp4 < wp3 || wp3 < wp4));
                    EXPECT_TRUE(!(wp4 < wp3 && wp3 < wp4));
                }

                sp.reset();
                sp2.reset();

                EXPECT_TRUE(b1 == (wp < wp2));
                EXPECT_TRUE(b2 == (wp2 < wp));

                {
                    AZStd::weak_ptr<X> wp3(wp);

                    EXPECT_TRUE(!(wp < wp3 || wp3 < wp));
                    EXPECT_TRUE(!(wp < wp3 && wp3 < wp));

                    EXPECT_TRUE(!(wp2 < wp3 || wp3 < wp2));
                    EXPECT_TRUE(!(wp2 < wp3 && wp3 < wp2));

                    AZStd::weak_ptr<X> wp4(wp2);

                    EXPECT_TRUE(!(wp4 < wp3 || wp3 < wp4));
                    EXPECT_TRUE(!(wp4 < wp3 && wp3 < wp4));
                }
            }

            {
                AZStd::shared_ptr<X> spx(new X);
                AZStd::shared_ptr<Y> spy(new Y);
                AZStd::shared_ptr<Z> spz(new Z);

                AZStd::weak_ptr<X> px(spx);
                AZStd::weak_ptr<Y> py(spy);
                AZStd::weak_ptr<Z> pz(spz);

                EXPECT_TRUE(px < py || py < px);
                EXPECT_TRUE(px < pz || pz < px);
                EXPECT_TRUE(py < pz || pz < py);

                EXPECT_TRUE(!(px < py && py < px));
                EXPECT_TRUE(!(px < pz && pz < px));
                EXPECT_TRUE(!(py < pz && pz < py));

                AZStd::weak_ptr<void> pvx(px);
                EXPECT_TRUE(!(pvx < pvx));

                AZStd::weak_ptr<void> pvy(py);
                EXPECT_TRUE(!(pvy < pvy));

                AZStd::weak_ptr<void> pvz(pz);
                EXPECT_TRUE(!(pvz < pvz));

                EXPECT_TRUE(pvx < pvy || pvy < pvx);
                EXPECT_TRUE(pvx < pvz || pvz < pvx);
                EXPECT_TRUE(pvy < pvz || pvz < pvy);

                EXPECT_TRUE(!(pvx < pvy && pvy < pvx));
                EXPECT_TRUE(!(pvx < pvz && pvz < pvx));
                EXPECT_TRUE(!(pvy < pvz && pvz < pvy));

                spx.reset();
                spy.reset();
                spz.reset();

                EXPECT_TRUE(px < py || py < px);
                EXPECT_TRUE(px < pz || pz < px);
                EXPECT_TRUE(py < pz || pz < py);

                EXPECT_TRUE(!(px < py && py < px));
                EXPECT_TRUE(!(px < pz && pz < px));
                EXPECT_TRUE(!(py < pz && pz < py));

                EXPECT_TRUE(!(pvx < pvx));
                EXPECT_TRUE(!(pvy < pvy));
                EXPECT_TRUE(!(pvz < pvz));

                EXPECT_TRUE(pvx < pvy || pvy < pvx);
                EXPECT_TRUE(pvx < pvz || pvz < pvx);
                EXPECT_TRUE(pvy < pvz || pvz < pvy);

                EXPECT_TRUE(!(pvx < pvy && pvy < pvx));
                EXPECT_TRUE(!(pvx < pvz && pvz < pvx));
                EXPECT_TRUE(!(pvy < pvz && pvz < pvy));
            }

            {
                AZStd::shared_ptr<Z> spz(new Z);
                AZStd::shared_ptr<X> spx(spz);

                AZStd::weak_ptr<Z> pz(spz);
                AZStd::weak_ptr<X> px(spx);
                AZStd::weak_ptr<Y> py(spz);

                EXPECT_TRUE(!(px < px));
                EXPECT_TRUE(!(py < py));

                EXPECT_TRUE(!(px < py || py < px));
                EXPECT_TRUE(!(px < pz || pz < px));
                EXPECT_TRUE(!(py < pz || pz < py));

                AZStd::weak_ptr<void> pvx(px);
                AZStd::weak_ptr<void> pvy(py);
                AZStd::weak_ptr<void> pvz(pz);

                EXPECT_TRUE(!(pvx < pvy || pvy < pvx));
                EXPECT_TRUE(!(pvx < pvz || pvz < pvx));
                EXPECT_TRUE(!(pvy < pvz || pvz < pvy));

                spx.reset();
                spz.reset();

                EXPECT_TRUE(!(px < px));
                EXPECT_TRUE(!(py < py));

                EXPECT_TRUE(!(px < py || py < px));
                EXPECT_TRUE(!(px < pz || pz < px));
                EXPECT_TRUE(!(py < pz || pz < py));

                EXPECT_TRUE(!(pvx < pvy || pvy < pvx));
                EXPECT_TRUE(!(pvx < pvz || pvz < pvx));
                EXPECT_TRUE(!(pvy < pvz || pvz < pvy));
            }
        }
    } // namespace n_comparison

    namespace n_lock
    {
        void test()
        {
        }
    } // namespace n_lock

    namespace n_map
    {
        void test()
        {
            AZStd::vector< AZStd::shared_ptr<int> > vi;

            {
                AZStd::shared_ptr<int> pi1(new int);
                AZStd::shared_ptr<int> pi2(new int);
                AZStd::shared_ptr<int> pi3(new int);

                vi.push_back(pi1);
                vi.push_back(pi1);
                vi.push_back(pi1);
                vi.push_back(pi2);
                vi.push_back(pi1);
                vi.push_back(pi2);
                vi.push_back(pi1);
                vi.push_back(pi3);
                vi.push_back(pi3);
                vi.push_back(pi2);
                vi.push_back(pi1);
            }

            AZStd::vector< AZStd::shared_ptr<X> > vx;

            {
                AZStd::shared_ptr<X> px1(new X);
                AZStd::shared_ptr<X> px2(new X);
                AZStd::shared_ptr<X> px3(new X);

                vx.push_back(px2);
                vx.push_back(px2);
                vx.push_back(px1);
                vx.push_back(px2);
                vx.push_back(px1);
                vx.push_back(px1);
                vx.push_back(px1);
                vx.push_back(px2);
                vx.push_back(px1);
                vx.push_back(px3);
                vx.push_back(px2);
            }

            AZStd::map< AZStd::weak_ptr<void>, long > m;

            {
                for (AZStd::vector< AZStd::shared_ptr<int> >::iterator i = vi.begin(); i != vi.end(); ++i)
                {
                    ++m[*i];
                }
            }

            {
                for (AZStd::vector< AZStd::shared_ptr<X> >::iterator i = vx.begin(); i != vx.end(); ++i)
                {
                    ++m[*i];
                }
            }

            {
                for (AZStd::map< AZStd::weak_ptr<void>, long >::iterator i = m.begin(); i != m.end(); ++i)
                {
                    EXPECT_TRUE(i->first.use_count() == i->second);
                }
            }
        }
    } // namespace n_map

    class WeakPointerTest
    {
        void* m_memBlock;
    public:
        WeakPointerTest()
        {
            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_numMemoryBlocks = 1;
            desc.m_heap.m_memoryBlocksByteSize[0] = 15 * 1024 * 1024;
            m_memBlock = UnitTest::DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_memoryBlocks[0] = m_memBlock;

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        ~WeakPointerTest()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            UnitTest::DebugAlignFree(m_memBlock);
        }

        void run()
        {
            n_element_type::test();
            n_constructors::test();
            n_assignment::test();
            n_reset::test();
            n_use_count::test();
            n_swap::test();
            n_comparison::test();
            n_lock::test();

            n_map::test();
        }
    };

    class incomplete
    {
    };

    AZStd::shared_ptr<incomplete> create_incomplete()
    {
        AZStd::shared_ptr<incomplete> px(new incomplete);
        return px;
    }
}

namespace IntrusivePointer
{
    namespace N
    {
        class base
        {
        private:
            AZStd::atomic<long> use_count_;
            base(base const&);
            base& operator=(base const&);
        protected:

            base()
                : use_count_(0)
            {
            }
            virtual ~base()
            {
            }

        public:
            long use_count() const
            {
                return use_count_.load();
            }

            inline friend void intrusive_ptr_add_ref(base* p)
            {
                ++p->use_count_;
            }

            inline friend void intrusive_ptr_release(base* p)
            {
                if (--p->use_count_ == 0)
                {
                    delete p;
                }
            }

            void add_ref()
            {
                ++use_count_;
            }

            void release()
            {
                if (--use_count_ == 0)
                {
                    delete this;
                }
            }
        };
    } // namespace N

    struct X
        : public virtual N::base
    {
    };

    struct Y
        : public X
    {
    };

    //

    namespace n_element_type
    {
        void f(X&)
        {
        }

        void test()
        {
            typedef AZStd::intrusive_ptr<X>::element_type T;
            T t;
            f(t);
        }
    } // namespace n_element_type

    namespace n_constructors
    {
        void default_constructor()
        {
            AZStd::intrusive_ptr<X> px;
            EXPECT_TRUE(px.get() == 0);
        }

        void pointer_constructor()
        {
            {
                AZStd::intrusive_ptr<X> px(0);
                EXPECT_TRUE(px.get() == 0);
            }

            {
                AZStd::intrusive_ptr<X> px(0, false);
                EXPECT_TRUE(px.get() == 0);
            }

            {
                X* p = new X;
                EXPECT_TRUE(p->use_count() == 0);

                AZStd::intrusive_ptr<X> px(p);
                EXPECT_TRUE(px.get() == p);
                EXPECT_TRUE(px->use_count() == 1);
            }

            {
                X* p = new X;
                EXPECT_TRUE(p->use_count() == 0);

                intrusive_ptr_add_ref(p);
                EXPECT_TRUE(p->use_count() == 1);

                AZStd::intrusive_ptr<X> px(p, false);
                EXPECT_TRUE(px.get() == p);
                EXPECT_TRUE(px->use_count() == 1);
            }
        }

        void copy_constructor()
        {
            {
                AZStd::intrusive_ptr<X> px;
                AZStd::intrusive_ptr<X> px2(px);
                EXPECT_TRUE(px2.get() == px.get());
            }

            {
                AZStd::intrusive_ptr<Y> py;
                AZStd::intrusive_ptr<X> px(py);
                EXPECT_TRUE(px.get() == py.get());
            }

            {
                AZStd::intrusive_ptr<X> px(0);
                AZStd::intrusive_ptr<X> px2(px);
                EXPECT_TRUE(px2.get() == px.get());
            }

            {
                AZStd::intrusive_ptr<Y> py(0);
                AZStd::intrusive_ptr<X> px(py);
                EXPECT_TRUE(px.get() == py.get());
            }

            {
                AZStd::intrusive_ptr<X> px(0);
                AZStd::intrusive_ptr<X> px2(px);
                EXPECT_TRUE(px2.get() == px.get());
            }

            {
                AZStd::intrusive_ptr<Y> py(0);
                AZStd::intrusive_ptr<X> px(py);
                EXPECT_TRUE(px.get() == py.get());
            }

            {
                AZStd::intrusive_ptr<X> px(new X);
                AZStd::intrusive_ptr<X> px2(px);
                EXPECT_TRUE(px2.get() == px.get());
            }

            {
                AZStd::intrusive_ptr<Y> py(new Y);
                AZStd::intrusive_ptr<X> px(py);
                EXPECT_TRUE(px.get() == py.get());
            }
        }

        void test()
        {
            default_constructor();
            pointer_constructor();
            copy_constructor();
        }
    } // namespace n_constructors

    namespace n_destructor
    {
        void test()
        {
            AZStd::intrusive_ptr<X> px(new X);
            EXPECT_TRUE(px->use_count() == 1);

            {
                AZStd::intrusive_ptr<X> px2(px);
                EXPECT_TRUE(px->use_count() == 2);
            }

            EXPECT_TRUE(px->use_count() == 1);
        }
    } // namespace n_destructor

    namespace n_assignment
    {
        void copy_assignment()
        {
        }

        void conversion_assignment()
        {
        }

        void pointer_assignment()
        {
        }

        void test()
        {
            copy_assignment();
            conversion_assignment();
            pointer_assignment();
        }
    } // namespace n_assignment

    namespace n_access
    {
        void test()
        {
            {
                AZStd::intrusive_ptr<X> px;
                EXPECT_FALSE(px);
                EXPECT_TRUE(!px);

                EXPECT_TRUE(get_pointer(px) == px.get());
            }

            {
                AZStd::intrusive_ptr<X> px(0);
                EXPECT_FALSE(px);
                EXPECT_TRUE(!px);

                EXPECT_TRUE(get_pointer(px) == px.get());
            }

            {
                AZStd::intrusive_ptr<X> px(new X);
                EXPECT_TRUE(px ? true : false);
                EXPECT_TRUE(!!px);
                EXPECT_TRUE(&*px == px.get());
                EXPECT_TRUE(px.operator ->() == px.get());

                EXPECT_TRUE(get_pointer(px) == px.get());
            }
        }
    } // namespace n_access

    namespace n_swap
    {
        void test()
        {
            {
                AZStd::intrusive_ptr<X> px;
                AZStd::intrusive_ptr<X> px2;

                px.swap(px2);

                EXPECT_TRUE(px.get() == 0);
                EXPECT_TRUE(px2.get() == 0);

                using std::swap;
                swap(px, px2);

                EXPECT_TRUE(px.get() == 0);
                EXPECT_TRUE(px2.get() == 0);
            }

            {
                X* p = new X;
                AZStd::intrusive_ptr<X> px;
                AZStd::intrusive_ptr<X> px2(p);
                AZStd::intrusive_ptr<X> px3(px2);

                px.swap(px2);

                EXPECT_TRUE(px.get() == p);
                EXPECT_TRUE(px->use_count() == 2);
                EXPECT_TRUE(px2.get() == 0);
                EXPECT_TRUE(px3.get() == p);
                EXPECT_TRUE(px3->use_count() == 2);

                using std::swap;
                swap(px, px2);

                EXPECT_TRUE(px.get() == 0);
                EXPECT_TRUE(px2.get() == p);
                EXPECT_TRUE(px2->use_count() == 2);
                EXPECT_TRUE(px3.get() == p);
                EXPECT_TRUE(px3->use_count() == 2);
            }

            {
                X* p1 = new X;
                X* p2 = new X;
                AZStd::intrusive_ptr<X> px(p1);
                AZStd::intrusive_ptr<X> px2(p2);
                AZStd::intrusive_ptr<X> px3(px2);

                px.swap(px2);

                EXPECT_TRUE(px.get() == p2);
                EXPECT_TRUE(px->use_count() == 2);
                EXPECT_TRUE(px2.get() == p1);
                EXPECT_TRUE(px2->use_count() == 1);
                EXPECT_TRUE(px3.get() == p2);
                EXPECT_TRUE(px3->use_count() == 2);

                using std::swap;
                swap(px, px2);

                EXPECT_TRUE(px.get() == p1);
                EXPECT_TRUE(px->use_count() == 1);
                EXPECT_TRUE(px2.get() == p2);
                EXPECT_TRUE(px2->use_count() == 2);
                EXPECT_TRUE(px3.get() == p2);
                EXPECT_TRUE(px3->use_count() == 2);
            }
        }
    } // namespace n_swap

    namespace n_comparison
    {
        template<class T, class U>
        void test2(AZStd::intrusive_ptr<T> const& p, AZStd::intrusive_ptr<U> const& q)
        {
            EXPECT_TRUE((p == q) == (p.get() == q.get()));
            EXPECT_TRUE((p != q) == (p.get() != q.get()));
        }

        template<class T>
        void test3(AZStd::intrusive_ptr<T> const& p, AZStd::intrusive_ptr<T> const& q)
        {
            EXPECT_TRUE((p == q) == (p.get() == q.get()));
            EXPECT_TRUE((p.get() == q) == (p.get() == q.get()));
            EXPECT_TRUE((p == q.get()) == (p.get() == q.get()));
            EXPECT_TRUE((p != q) == (p.get() != q.get()));
            EXPECT_TRUE((p.get() != q) == (p.get() != q.get()));
            EXPECT_TRUE((p != q.get()) == (p.get() != q.get()));

            // 'less' moved here as a g++ 2.9x parse error workaround
            AZStd::less<T*> less;
            EXPECT_TRUE((p < q) == less(p.get(), q.get()));
        }

        void test()
        {
            {
                AZStd::intrusive_ptr<X> px;
                test3(px, px);

                AZStd::intrusive_ptr<X> px2;
                test3(px, px2);

                AZStd::intrusive_ptr<X> px3(px);
                test3(px3, px3);
                test3(px, px3);
            }

            {
                AZStd::intrusive_ptr<X> px;

                AZStd::intrusive_ptr<X> px2(new X);
                test3(px, px2);
                test3(px2, px2);

                AZStd::intrusive_ptr<X> px3(new X);
                test3(px2, px3);

                AZStd::intrusive_ptr<X> px4(px2);
                test3(px2, px4);
                test3(px4, px4);
            }

            {
                AZStd::intrusive_ptr<X> px(new X);

                AZStd::intrusive_ptr<Y> py(new Y);
                test2(px, py);

                AZStd::intrusive_ptr<X> px2(py);
                test2(px2, py);
                test3(px, px2);
                test3(px2, px2);
            }
        }
    } // namespace n_comparison

    namespace n_static_cast
    {
        void test()
        {
        }
    } // namespace n_static_cast

    namespace n_dynamic_cast
    {
        void test()
        {
        }
    } // namespace n_dynamic_cast

    namespace n_transitive
    {
        struct X
            : public N::base
        {
            AZStd::intrusive_ptr<X> next;
        };

        void test()
        {
            AZStd::intrusive_ptr<X> p(new X);
            p->next = AZStd::intrusive_ptr<X>(new X);
            EXPECT_TRUE(!p->next->next);
            p = p->next;
            EXPECT_TRUE(!p->next);
        }
    } // namespace n_transitive

    namespace n_report_1
    {
        class foo
            : public N::base
        {
        public:

            foo()
                : m_self(this)
            {
            }

            void suicide()
            {
                m_self = 0;
            }

        private:

            AZStd::intrusive_ptr<foo> m_self;
        };

        void test()
        {
            foo* foo_ptr = new foo;
            foo_ptr->suicide();
        }
    } // namespace n_report_1

    class IntrusivePointerTest
    {
        void* m_memBlock;
    public:
        IntrusivePointerTest()
        {
            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_numMemoryBlocks = 1;
            desc.m_heap.m_memoryBlocksByteSize[0] = 15 * 1024 * 1024;
            m_memBlock = UnitTest::DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_memoryBlocks[0] = m_memBlock;

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
        }

        ~IntrusivePointerTest()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            UnitTest::DebugAlignFree(m_memBlock);
        }
        void run()
        {
            n_element_type::test();
            n_constructors::test();
            n_destructor::test();
            n_assignment::test();
            n_access::test();
            n_swap::test();
            n_comparison::test();
            n_static_cast::test();
            n_dynamic_cast::test();

            n_transitive::test();
            n_report_1::test();
        }
    };
}

namespace UniquePtr
{
    class MyClass
    {
    public:
        AZ_CLASS_ALLOCATOR(MyClass, AZ::SystemAllocator, 0);
        MyClass(int data = 100)
            : m_data(data)
        {}

        int m_data;
    };

    class UniquePointerTest
        : public UnitTest::AllocatorsFixture
    {
    public:
        void run()
        {
            typedef AZStd::unique_ptr<MyClass> my_unique_ptr_class;
            typedef AZStd::set<my_unique_ptr_class, AZStd::less<my_unique_ptr_class> > MySet;
            typedef AZStd::list<my_unique_ptr_class> MyList;
            typedef AZStd::vector <my_unique_ptr_class> MyVector;

            //Create unique_ptr using dynamic allocation
            my_unique_ptr_class my_ptr(aznew MyClass);
            my_unique_ptr_class my_ptr2(aznew MyClass);

            MyClass* ptr1 = my_ptr.get();
            MyClass* ptr2 = my_ptr2.get();

            //Test some copy constructors
            my_unique_ptr_class my_ptr3(nullptr);
            my_unique_ptr_class my_ptr4(AZStd::move(my_ptr3));

            // Construct a list and fill
            MyList list;

            //Insert from my_unique_ptr_class
            list.push_front(AZStd::move(my_ptr));
            list.push_back(AZStd::move(my_ptr2));

            //Check pointers
            EXPECT_TRUE(my_ptr.get() == 0);
            EXPECT_TRUE(my_ptr2.get() == 0);
            EXPECT_TRUE(list.begin()->get() == ptr1);
            EXPECT_TRUE(list.rbegin()->get() == ptr2);

            //Construct a set and fill
            MySet set;

            //Insert in set from list passing ownership
            set.insert(AZStd::move(*list.begin()));
            set.insert(AZStd::move(*list.rbegin()));

            //Check pointers
            EXPECT_TRUE(list.begin()->get() == 0);
            EXPECT_TRUE(list.rbegin()->get() == 0);

            //A set is ordered by std::less<my_unique_ptr_class> so
            //be careful when comparing pointers
            if (ptr1 < ptr2)
            {
                EXPECT_TRUE(set.begin()->get() == ptr1);
                EXPECT_TRUE(set.rbegin()->get() == ptr2);
            }
            else
            {
                EXPECT_TRUE(set.rbegin()->get() == ptr1);
                EXPECT_TRUE(set.begin()->get() == ptr2);
            }

            //Now with vector
            MyVector vector;

            //Insert from my_unique_ptr_class
            if (ptr1 < ptr2)
            {
                vector.insert(vector.begin(), AZStd::move(*set.begin()));
                vector.insert(vector.end(), AZStd::move(*set.rbegin()));
            }
            else
            {
                vector.insert(vector.begin(), AZStd::move(*set.rbegin()));
                vector.insert(vector.end(), AZStd::move(*set.begin()));
            }

            //Check pointers
            EXPECT_TRUE(my_ptr.get() == 0);
            EXPECT_TRUE(my_ptr2.get() == 0);
            EXPECT_TRUE(vector.begin()->get() == ptr1);
            EXPECT_TRUE(vector.rbegin()->get() == ptr2);

            MyVector vector2(AZStd::move(vector));
            vector2.swap(vector);

            EXPECT_TRUE(vector.begin()->get() == ptr1);
            EXPECT_TRUE(vector.rbegin()->get() == ptr2);

            // check that we can resize the container
            vector.resize(10);
            EXPECT_TRUE(vector.size() == 10);
            // 2 elements originally and 8 new ones
            for (size_t i = 2; i < vector.size(); ++i)
            {
                EXPECT_TRUE(vector[i].get() == nullptr);
            }

            my_unique_ptr_class a(nullptr), b(nullptr);
            a = AZStd::move(b);
        }
    };
} //UniquePtrTest

#endif // 0

