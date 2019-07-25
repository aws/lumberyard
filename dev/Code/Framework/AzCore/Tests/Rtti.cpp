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

#include "TestTypes.h"

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

// Non intrusive typeinfo for external and intergral types
struct ExternalClass
{
};

// These 2 types must only EVER be used by the MultiThreadedTypeInfo test, or else
// that test is invalidated because the statics will have been initialized already
struct MTTI {};
struct MTTI2
{
    AZ_TYPE_INFO(MTTI2, "{CBC94693-5ECD-4CBF-A8DB-9B122E697E8D}");
};

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(ExternalClass, "{38380915-084B-4886-8D3D-B8439E9E987C}");
    AZ_TYPE_INFO_SPECIALIZE(MTTI, "{4876C017-0C26-4D0D-9A1F-2A738BAE6449}");
}

using namespace AZ;

namespace UnitTest
{
    class Rtti
        : public AllocatorsFixture
    {
    };

    // Intrusive TypeInfo
    struct MyClass
    {
        AZ_TYPE_INFO(MyClass, "{CADA6BA7-D479-4C20-B7F0-121A1DF4E9CC}");
    };

    template<class T1, class T2>
    struct MyClassTemplate
    {
        AZ_TYPE_INFO(MyClassTemplate, "{EBFE7ADF-1FCE-47F0-B417-14FE06BAF02D}", T1, T2)

    };

    template<class... Args>
    struct MyClassVariadicTemplate
    {
        AZ_TYPE_INFO(MyClassVariadicTemplate, "{60C1D809-09FA-48EB-A9B7-0BD8DBFF21C8}", Args...);
    };

    TEST_F(Rtti, TypeInfoTest)
    {
        AZ_TEST_ASSERT(AzTypeInfo<MyClass>::Uuid() == Uuid("{CADA6BA7-D479-4C20-B7F0-121A1DF4E9CC}"));
        AZ_TEST_ASSERT(strcmp(AzTypeInfo<MyClass>::Name(), "MyClass") == 0);

        AZ_TEST_ASSERT(AzTypeInfo<ExternalClass>::Uuid() == Uuid("{38380915-084B-4886-8D3D-B8439E9E987C}"));
        AZ_TEST_ASSERT(strcmp(AzTypeInfo<ExternalClass>::Name(), "ExternalClass") == 0);

        // templates
        {
            Uuid templateUuid = Uuid("{EBFE7ADF-1FCE-47F0-B417-14FE06BAF02D}") + AZ::Internal::AggregateTypes<MyClass, int>::Uuid();

            typedef MyClassTemplate<MyClass, int> MyClassTemplateType;

            AZ_TEST_ASSERT(AzTypeInfo<MyClassTemplateType>::Uuid() == templateUuid);
            const char* myClassTemplatename = AzTypeInfo<MyClassTemplateType>::Name();

            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClassTemplate"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClass"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "int"));
        }


        // variadic templates
        {
            Uuid templateUuid = Uuid("{60C1D809-09FA-48EB-A9B7-0BD8DBFF21C8}") + AZ::Internal::AggregateTypes<MyClass, int>::Uuid();

            typedef MyClassVariadicTemplate<MyClass, int> MyClassVariadicTemplateType;

            AZ_TEST_ASSERT(AzTypeInfo<MyClassVariadicTemplateType>::Uuid() == templateUuid);
            const char* myClassTemplatename = AzTypeInfo<MyClassVariadicTemplateType>::Name();

            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClassVariadicTemplate"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClass"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "int"));
        }
    }

    class MyBase
    {
    public:
        AZ_TYPE_INFO(MyBase, "{6A0855E5-6899-482B-B470-C3E5C13D13F5}");
        virtual ~MyBase() {}
        int dataMyBase;
    };

    class MyBase1
        : public MyBase
    {
    public:
        virtual ~MyBase1() {}
        // Event though MyBase doesn't have RTTI we do allow to be noted as a base class, of course it will NOT be
        // part of the RTTI chain. The goal is to allow AZ_RTTI to declare any base classes without worry if they have RTTI or not
        AZ_RTTI(MyBase1, "{F3F97A32-15D2-48FF-B741-B89EA2DD2280}", MyBase);
        int data1MyBase1;
        int data2MyBase1;
    };

    class MyDerived
        : public MyBase1
    {
    public:
        virtual ~MyDerived() {}
        AZ_RTTI(MyDerived, "{3BE0590A-F20F-4056-96AF-C2F0565C2EA5}", MyBase1);
        int dataMyDerived;
    };

    class MyDerived1
    {
    public:
        virtual ~MyDerived1() {}
        AZ_RTTI(MyDerived1, "{527B6166-1A4F-4782-8D06-F228860B1102}");
        int datatypename;
    };

    class MyDerived2
        : public MyDerived
    {
    public:
        virtual ~MyDerived2() {}
        AZ_RTTI(MyDerived2, "{8902C46B-61C5-4294-82A2-06CB61ACA314}", MyDerived);
        int dataMyDerived2;
    };

    class MyClassMix
        : public MyDerived2
        , public MyDerived1
    {
    public:
        virtual ~MyClassMix() {}
        AZ_RTTI(MyClassMix, "{F6CDCF25-3161-46AE-A46C-0F9B8A1027AF}", MyDerived2, MyDerived1);
        int dataMix;
    };

    class MyClassA
    {
    public:
        virtual ~MyClassA() {}
        AZ_RTTI(MyClassA, "{F2D44607-1BB6-4A6D-8D8B-4FDE27B488CF}");
        int dataClassA;
    };

    class MyClassB
    {
    public:
        virtual ~MyClassB() {}
        AZ_RTTI(MyClassB, "{E46477C8-4833-4F8C-A57A-02EAFA0C33D8}");
        int dataClassB;
    };

    class MyClassC
    {
    public:
        virtual ~MyClassC() {}
        AZ_RTTI(MyClassC, "{614F230F-1AD0-419D-8376-18891112F55D}");
        int dataClassC;
    };

    class MyClassD
        : public MyClassA
    {
    public:
        virtual ~MyClassD() {}
        AZ_RTTI(MyClassD, "{8E047831-1445-4D13-8F6F-DD36C871FD05}", MyClassA);
        int dataClassD;
    };

    class MyClassMaxMix
        : public MyDerived2
        , public MyDerived1
        , public MyClassB
        , public MyClassC
        , public MyClassD
    {
    public:
        virtual ~MyClassMaxMix() {}
        AZ_RTTI(MyClassMaxMix, "{49A7F45B-D039-44ED-A6BF-E500CB84E867}", MyDerived2, MyDerived1, MyClassB, MyClassC, MyClassD);
        int dataMaxMix;
    };

    TEST_F(Rtti, IsTypeOfTest)
    {
        typedef AZStd::vector<Uuid> TypeIdArray;
    
        auto EnumTypes = [](const Uuid& id, void* userData)
        {
            TypeIdArray* idArray = reinterpret_cast<TypeIdArray*>(userData);
            idArray->push_back(id);
        };

        MyBase1 mb1;
        MyDerived md;
        MyDerived2 md2;
        MyClassMix mcm;
        MyClassMaxMix mcmm;

        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(mb1) == false);// MyBase has not RTTI enabled, even though it's a base class
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(mb1) == false);
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(md) == false);
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived2>(md) == false);

        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(md2));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(md2));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(md2) == false);

        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived1>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived2>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(mcm) == false);

        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived1>(&mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived2>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassA>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassB>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassC>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassD>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(mcmm) == false);

        // type checks
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1&>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1&>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1&>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1&>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1*>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1*>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1*>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1*>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof(AzTypeInfo<const MyBase1>::Uuid(), &md));
        AZ_TEST_ASSERT(azrtti_istypeof(AzTypeInfo<MyBase1>::Uuid(), md));

        // check type enumeration
        TypeIdArray typeIds;
        // check a single type (no base types)
        MyDerived1::RTTI_EnumHierarchy(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 1);
        AZ_TEST_ASSERT(typeIds[0] == AzTypeInfo<MyDerived1>::Uuid());
        // check a simple inheritance
        typeIds.clear();
        MyDerived::RTTI_EnumHierarchy(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 2);
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyBase1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived>::Uuid()) != typeIds.end());
        // check a little more complicated one
        typeIds.clear();
        MyClassMix::RTTI_EnumHierarchy(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 5);
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyBase1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived2>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyClassMix>::Uuid()) != typeIds.end());

        // now check the virtual full time selection
        MyBase1* mb1Ptr = &mcm;
        typeIds.clear();
        mb1Ptr->RTTI_EnumTypes(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 5);
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyBase1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived2>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyClassMix>::Uuid()) != typeIds.end());
    }

    class ExampleAbstractClass
    {
    public: 
        AZ_RTTI(ExampleAbstractClass, "{F99EC269-3077-4984-A1B6-FA5656A65AC9}")
        virtual void AbstractFunction1() = 0;
        virtual void AbstractFunction2() = 0;
    };

    class ExampleFullImplementationClass : public ExampleAbstractClass
    {
    public:
        AZ_RTTI(ExampleFullImplementationClass, "{81B043ED-3770-414E-8B54-0F623C035926}", ExampleAbstractClass)
        void AbstractFunction1() override {} 
        void AbstractFunction2() override {}
    };

    class ExamplePartialImplementationClass1 
        : public ExampleAbstractClass
    {
    public:
        AZ_RTTI(ExamplePartialImplementationClass1, "{049B29D7-0414-4C5F-8FB2-589D0833121B}", ExampleAbstractClass)
        void AbstractFunction1() override {}
    };

    class ExampleCombined 
        : public ExamplePartialImplementationClass1
    {
    public:
        AZ_RTTI(ExampleCombined, "{0D03E811-F8F1-4AA5-8DA2-4CD6B7FB7080}", ExamplePartialImplementationClass1)
        void AbstractFunction2() override {}
    };

    TEST_F(Rtti, IsAbstract)
    {
        // compile time proof that the two non-abstract classes are not abstract at compile time:
        ExampleFullImplementationClass one;
        ExampleCombined two;

        ASSERT_NE(GetRttiHelper<ExampleAbstractClass>(), nullptr);
        ASSERT_NE(GetRttiHelper<ExampleFullImplementationClass>(), nullptr);
        ASSERT_NE(GetRttiHelper<ExamplePartialImplementationClass1>(), nullptr);
        ASSERT_NE(GetRttiHelper<ExampleCombined>(), nullptr);

        EXPECT_TRUE(GetRttiHelper<ExampleAbstractClass>()->IsAbstract());
        EXPECT_FALSE(GetRttiHelper<ExampleFullImplementationClass>()->IsAbstract());
        EXPECT_TRUE(GetRttiHelper<ExamplePartialImplementationClass1>()->IsAbstract());
        EXPECT_FALSE(GetRttiHelper<ExampleCombined>()->IsAbstract());
    }

    TEST_F(Rtti, DynamicCastTest)
    {
        MyBase1 i_mb1;
        MyDerived i_md;
        MyDerived2 i_md2;
        MyClassMix i_mcm;
        MyClassMaxMix i_mcmm;

        MyBase1* mb1 = &i_mb1;
        MyDerived* md = &i_md;
        MyDerived2* md2 = &i_md2;
        MyClassMix* mcm = &i_mcm;
        MyClassMaxMix* mcmm = &i_mcmm;

        // downcast
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(mb1) == nullptr);// MyBase has not RTTI enabled, even though it's a base class
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mb1) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(md));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(md) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(md) == nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(md2));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(md2));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(md2) == nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived1*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(mcm) == nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived1*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassA*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassB*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassC*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassD*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(mcmm) == nullptr);

        // up cast
        mb1 = mcmm;
        MyClassA* mca = mcmm;
        int i_i;
        int* pi = &i_i;

        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(nullptr) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(pi) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<int*>(pi) == pi);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mb1) != nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(mb1) != nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyClassMaxMix*>(mb1) != nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyClassD*>(mca) != nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyClassMaxMix*>(mca) != nullptr);

        // type checks
        const MyDerived* cmd = md;
        AZ_TEST_ASSERT(azdynamic_cast<const MyBase1*>(md));
        AZ_TEST_ASSERT(azdynamic_cast<const volatile MyBase1*>(md));
        AZ_TEST_ASSERT(azdynamic_cast<const MyBase1*>(cmd));
        AZ_TEST_ASSERT(azdynamic_cast<const volatile MyBase1*>(cmd));
        // unrelated cast not supported (we can, but why)
        //AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(mca));

        md = mcmm;

        // serialization helpers
        AZ_TEST_ASSERT(mca->RTTI_AddressOf(AzTypeInfo<MyClassMaxMix>::Uuid()) == mcmm);
        AZ_TEST_ASSERT(mb1->RTTI_AddressOf(AzTypeInfo<MyClassMaxMix>::Uuid()) == mcmm);
        AZ_TEST_ASSERT(mb1->RTTI_AddressOf(AzTypeInfo<MyClassA>::Uuid()) == mca);
        AZ_TEST_ASSERT(mb1->RTTI_AddressOf(AzTypeInfo<MyDerived>::Uuid()) == md);
        AZ_TEST_ASSERT(md2->RTTI_AddressOf(AzTypeInfo<MyClassA>::Uuid()) == nullptr);
        AZ_TEST_ASSERT(mcmm->RTTI_AddressOf(AzTypeInfo<MyClassA>::Uuid()) == mca);
        AZ_TEST_ASSERT(mcmm->RTTI_AddressOf(AzTypeInfo<MyBase1>::Uuid()) == mb1);

        // typeid
        AZ_TEST_ASSERT(azrtti_typeid<MyBase>() == AzTypeInfo<MyBase>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid(i_mb1) == AzTypeInfo<MyBase1>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid(md2) == AzTypeInfo<MyDerived2>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid(mca) == AzTypeInfo<MyClassMaxMix>::Uuid());
        MyClassA& mcar = i_mcmm;
        AZ_TEST_ASSERT(azrtti_typeid(mcar) == AzTypeInfo<MyClassMaxMix>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid<int>() == AzTypeInfo<int>::Uuid());
    }

    TEST_F(Rtti, MultiThreadedTypeInfo)
    {
        // These must be Uuids so that they don't engage the UuidHolder code
        const AZ::Uuid expectedMtti("{4876C017-0C26-4D0D-9A1F-2A738BAE6449}");
        const AZ::Uuid expectedMtti2("{CBC94693-5ECD-4CBF-A8DB-9B122E697E8D}");

        // Create 2x of each of these threads which are doing RTTI ops and
        // let the scheduler run them at random. This is attempting to crash
        // them into each other as best we can
        auto threadFunc1 = [&expectedMtti, &expectedMtti2]()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            const AZ::TypeId& mtti = azrtti_typeid<MTTI>();
            const AZ::TypeId& mtti2 = azrtti_typeid<MTTI2>();
            EXPECT_FALSE(mtti.IsNull());
            EXPECT_EQ(expectedMtti, mtti);
            EXPECT_FALSE(mtti2.IsNull());
            EXPECT_EQ(expectedMtti2, mtti2);
        };

        auto threadFunc2 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI* mtti = new MTTI();
            bool castSucceeded = (azrtti_cast<MTTI2*>(mtti) != nullptr);
            EXPECT_FALSE(castSucceeded);
            delete mtti;
        };

        auto threadFunc3 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI2* mtti2 = new MTTI2();
            bool castSucceeded = (azrtti_cast<MTTI*>(mtti2) != nullptr);
            EXPECT_FALSE(castSucceeded);
            delete mtti2;
        };

        auto threadFunc4 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI* mtti = new MTTI();
            bool castSucceeded = (azrtti_cast<MTTI*>(mtti) != nullptr);
            EXPECT_TRUE(castSucceeded);
            delete mtti;
        };

        auto threadFunc5 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI2* mtti2 = new MTTI2();
            bool castSucceeded = (azrtti_cast<MTTI2*>(mtti2) != nullptr);
            EXPECT_TRUE(castSucceeded);
            delete mtti2;
        };

        AZStd::fixed_vector<AZStd::function<void()>, 5> threadFuncs({ threadFunc1, threadFunc2, threadFunc3, threadFunc4, threadFunc5 });
        AZStd::thread threads[10];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            auto threadFunc = threadFuncs[threadIdx % threadFuncs.size()];
            threads[threadIdx] = AZStd::thread(threadFunc);
        }
        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    static void ExternalRttiEnumHeirarchyHelper(const AZ::TypeId&, void* userData)
    {
        auto totalClassesEnumerated = reinterpret_cast<size_t*>(userData);
        ++*totalClassesEnumerated;
    }

    class MyBaseExternal
    {
    public:
        AZ_TYPE_INFO(MyBaseExternal, "{F0F36BB2-14E6-4C44-B3D5-E0CBFD783C99}");

        int32_t m_intValue;
    };

    class MyDerivedExternal
        : public MyBaseExternal
    {
    public:
        AZ_TYPE_INFO(MyDerivedExternal, "{FFD1C3B7-7957-4270-BF10-700CE8BE2B53}");

        float m_floatValue;
    };

    class MyConvertibleExternal
    {
    public:
        AZ_TYPE_INFO(MyConvertibleExternal, "{3962F510-309B-4E32-8CE5-6DEE85F351A9}");

        MyConvertibleExternal() = default;
        MyConvertibleExternal(const MyBaseExternal& baseExternal)
            : m_baseExternal(baseExternal)
        {
        }

        operator MyBaseExternal() const
        {
            return m_baseExternal;
        }

        MyBaseExternal m_baseExternal;
    };

    class MyBaseIntrusive
    {
    public:
        AZ_RTTI(MyBaseIntrusive, "{06D41B30-CEDB-46C9-BD98-B8672A04F71F}");
        virtual ~MyBaseIntrusive() = default;
        uint64_t m_uintValue;
    };

    class MyDerivedIntrusive
        : public MyBaseIntrusive
    {
    public:
        AZ_RTTI(MyDerivedIntrusive, "{6F3FA2A5-CD05-424F-8E37-1DEDA7CE8816}", MyBaseIntrusive);
        virtual ~MyDerivedIntrusive() = default;
        double m_doubleValue;
    };

    class MyExternalDerivedFromExternalAndIntrusive
        : public MyDerivedExternal
        , public MyDerivedIntrusive
    {
    public:
        AZ_TYPE_INFO(MyExternalDerivedFromExternalAndIntrusive, "{79DC295D-98C5-4FEB-9DC0-0AC3D5A91855}");
    };
}

namespace AZ
{
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyBaseExternal);
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyDerivedExternal, UnitTest::MyBaseExternal);
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyExternalDerivedFromExternalAndIntrusive, UnitTest::MyDerivedExternal, UnitTest::MyDerivedIntrusive);
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyConvertibleExternal, UnitTest::MyBaseExternal);
}
namespace UnitTest
{
    class MyIntrusiveDerivedFromExternalAndIntrusive
        : public MyDerivedExternal
        , public MyDerivedIntrusive
    {
    public:
        AZ_RTTI(MyIntrusiveDerivedFromExternalAndIntrusive, "{3822CF8D-6AC7-4B71-B755-5C69B9DF5A3C}", MyDerivedExternal, MyDerivedIntrusive);
        virtual ~MyIntrusiveDerivedFromExternalAndIntrusive() = default;
    };


    TEST_F(Rtti, ExternalRtti)
    {
        MyBaseExternal baseInstance{ 7 };
        MyDerivedExternal derivedInstance;
        derivedInstance.m_intValue = 15;
        derivedInstance.m_floatValue = 0.0f;
        MyConvertibleExternal convertibleInstance(MyBaseExternal{ 24 });

        MyExternalDerivedFromExternalAndIntrusive externalDerivedFromExternalAndIntrusiveInstance;
        externalDerivedFromExternalAndIntrusiveInstance.m_intValue = -1;
        externalDerivedFromExternalAndIntrusiveInstance.m_uintValue = 2;
        externalDerivedFromExternalAndIntrusiveInstance.m_floatValue = 2.0f;
        externalDerivedFromExternalAndIntrusiveInstance.m_doubleValue = -32.0;

        MyIntrusiveDerivedFromExternalAndIntrusive intrusiveDerivedFromExternalAndIntrusiveInstance;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_intValue = -55;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_uintValue = 256;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_floatValue = -1023.0f;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_doubleValue = .0223;

        AZ::IRttiHelper* baseExternal = AZ::GetRttiHelper<MyBaseExternal>();
        AZ::IRttiHelper* derivedExternal = AZ::GetRttiHelper<MyDerivedExternal>();
        AZ::IRttiHelper* convertibleExternal = AZ::GetRttiHelper<MyConvertibleExternal>();
        AZ::IRttiHelper* externalDerivedFromExternalAndIntrusive = AZ::GetRttiHelper<MyExternalDerivedFromExternalAndIntrusive>();
        AZ::IRttiHelper* intrusiveDerivedFromExternalAndIntrusive = AZ::GetRttiHelper<MyIntrusiveDerivedFromExternalAndIntrusive>();

        ASSERT_NE(nullptr, baseExternal);
        ASSERT_NE(nullptr, derivedExternal);
        ASSERT_NE(nullptr, convertibleExternal);
        ASSERT_NE(nullptr, externalDerivedFromExternalAndIntrusive);
        ASSERT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive);

        // Base Class External RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyBaseExternal>::Uuid(), baseExternal->GetTypeId());
            EXPECT_TRUE(baseExternal->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            baseExternal->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyBaseExternal has no other base classes so this count should be 1
            EXPECT_EQ(1, enumHierarchyTotalClasses);
        }

        // Derived Class External RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyDerivedExternal>::Uuid(), derivedExternal->GetTypeId());
            EXPECT_TRUE(derivedExternal->IsTypeOf(AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            EXPECT_TRUE(derivedExternal->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            derivedExternal->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyDerivedExternal has MyBaseExternal as a base classes so this count should be 2
            EXPECT_EQ(2, enumHierarchyTotalClasses);

            // MyDerivedExternal -> MyDerivedExternal - succeeds
            EXPECT_NE(nullptr, derivedExternal->Cast(&derivedInstance, AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            // MyDerivedExternal -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, derivedExternal->Cast(&derivedInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyDerivedExternal - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyDerivedExternal>(&baseInstance));

            // MyBaseExternal -> MyBaseExternal(using derived class RttiHelper)- succeeds
            EXPECT_NE(nullptr, derivedExternal->Cast(&baseInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyBaseExternal(using RttiCast function which must lookup RTTI information from the derived instance)- fails
            // The reason why this fails is because the instance data does not have RTTI on it so it must lookup using
            // using the supplied template type id
            EXPECT_NE(nullptr, AZ::RttiCast<MyBaseExternal*>(&derivedInstance));
        }

        // Convertible Class External RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid(), convertibleExternal->GetTypeId());
            EXPECT_TRUE(convertibleExternal->IsTypeOf(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid()));
            EXPECT_TRUE(convertibleExternal->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            convertibleExternal->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyConvertibleExternal specifies "MyBaseExternal" as a base classes even though it really is not,
            // but EnumHierarchy should still enumerate for the "MyBaseExternal" typeid. Therefore the count should be 2
            EXPECT_EQ(2, enumHierarchyTotalClasses);

            // MyConvertibleExternal -> MyConvertibleExternal - succeeds
            EXPECT_NE(nullptr, convertibleExternal->Cast(&convertibleInstance, AZ::AzTypeInfo<MyConvertibleExternal>::Uuid()));
            // MyConvertibleExternal -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, convertibleExternal->Cast(&convertibleInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyConvertibleExternal - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyConvertibleExternal>(&baseInstance));

            // MyBaseExternal -> MyBaseExternal(using convertible class RttiHelper)- succeeds
            EXPECT_NE(nullptr, convertibleExternal->Cast(&baseInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyBaseExternal(using RttiCast function which must lookup RTTI information from the derived instance)- succeeds
            EXPECT_NE(nullptr, AZ::RttiCast<MyBaseExternal*>(&derivedInstance));
        }

        // Derived class with External RTTI which inherits from a class with external RTTI and intrusive RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid(), externalDerivedFromExternalAndIntrusive->GetTypeId());
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            externalDerivedFromExternalAndIntrusive->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyDerivedFromExternalAndIntrusive inherits from MyDerivedExternal which has one base class with external RTTI.
            // This adds 2 to the enumeration count.
            // MyDerivedFromExternalAndIntrusive also inherits from MyDerivedIntrusive which has one base with intrusive RTTI
            // This adds 2 more the enumeration count. Combining these counts with the one for this class the count value should be 5
            EXPECT_EQ(5, enumHierarchyTotalClasses);            

            // MyDerivedFromExternalAndIntrusive -> MyDerivedFromExternalAndIntrusive - succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid()));
            // MyDerivedFromExternalAndIntrusive -> MyDerivedExternal - succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));

            // MyDerivedFromExternalAndIntrusive -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyDerivedFromExternalAndIntrusive -> MyDerivedIntrusive- succeeds
            MyDerivedIntrusive* castedDerivedIntrusiveInstance = externalDerivedFromExternalAndIntrusive->Cast<MyDerivedIntrusive>(&externalDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedDerivedIntrusiveInstance);
            EXPECT_DOUBLE_EQ(-32.0, castedDerivedIntrusiveInstance->m_doubleValue);
            castedDerivedIntrusiveInstance->m_doubleValue = -64.0; // Verify that access doesn't crash due to invalid memory address
            // MyDerivedFromExternalAndIntrusive -> MyBaseIntrusive- succeeds
            MyBaseIntrusive* castedBaseIntrusiveInstance = externalDerivedFromExternalAndIntrusive->Cast<MyBaseIntrusive>(&externalDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedBaseIntrusiveInstance);
            EXPECT_EQ(2U, castedBaseIntrusiveInstance->m_uintValue);
            castedDerivedIntrusiveInstance->m_uintValue = 4U;

            // MyDerivedExternal -> MyDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, derivedExternal->Cast<MyExternalDerivedFromExternalAndIntrusive>(&derivedInstance));

            // MyBaseExternal -> MyDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyExternalDerivedFromExternalAndIntrusive>(&baseInstance));

            // MyBaseExternal -> MyBaseExternal(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&baseInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyDerivedExternal -> MyBaseExternal(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&derivedInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseIntrusive -> MyBaseIntrusive(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyBaseIntrusive baseIntrusiveInstance;
            baseIntrusiveInstance.m_uintValue = 3456893U;
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&baseIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // MyDerivedIntrusive-> MyBaseIntrusive(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyDerivedIntrusive derivedIntrusiveInstance;
            derivedIntrusiveInstance.m_uintValue = 1700U;
            derivedIntrusiveInstance.m_doubleValue = 24.0f;
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&derivedIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // Test Rtti Free functions for External class with external Rtti
            enumHierarchyTotalClasses = 0;
            AZ::RttiEnumHierarchy<MyExternalDerivedFromExternalAndIntrusive>(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            EXPECT_EQ(5, enumHierarchyTotalClasses);

            // This should fail
            EXPECT_EQ(nullptr, AZ::RttiCast<MyExternalDerivedFromExternalAndIntrusive*>(&derivedIntrusiveInstance));


            EXPECT_NE(nullptr, AZ::RttiCast<MyExternalDerivedFromExternalAndIntrusive*>(&externalDerivedFromExternalAndIntrusiveInstance));
            void* baseIntrusiveAddress = AZ::RttiAddressOf(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid());
            ASSERT_NE(nullptr, baseIntrusiveAddress);
            EXPECT_EQ(4U, static_cast<MyBaseIntrusive*>(baseIntrusiveAddress)->m_uintValue);

            EXPECT_FALSE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid(), externalDerivedFromExternalAndIntrusiveInstance));
            EXPECT_FALSE(AZ::RttiIsTypeOf<MyConvertibleExternal>(externalDerivedFromExternalAndIntrusiveInstance));

            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid(), externalDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyDerivedExternal>(externalDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiIsTypeOf
            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid(), &externalDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyBaseExternal>(&externalDerivedFromExternalAndIntrusiveInstance));

            EXPECT_EQ(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(externalDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiTypeId
            EXPECT_EQ(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(&externalDerivedFromExternalAndIntrusiveInstance));
        }

        // Derived class with Intrusive RTTI which inherits from a class with external RTTI and intrusive RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid(), intrusiveDerivedFromExternalAndIntrusive->GetTypeId());
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            intrusiveDerivedFromExternalAndIntrusive->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyIntrusiveDerivedFromExternalAndIntrusive inherits from MyDerivedExternal which has one base class with intrusive RTTI.
            // This adds 2 to the enumeration count.
            // MyIntrusiveDerivedFromExternalAndIntrusive also inherits from MyDerivedIntrusive which has one base with intrusive RTTI
            // This adds 2 more the enumeration count. Combining these counts with the one for this class the count value should be 5
            EXPECT_EQ(5, enumHierarchyTotalClasses);

            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyIntrusiveDerivedFromExternalAndIntrusive - succeeds
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid()));
            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyDerivedExternal - succeeds
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));

            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyDerivedIntrusive- succeeds
            MyDerivedIntrusive* castedDerivedIntrusiveInstance = intrusiveDerivedFromExternalAndIntrusive->Cast<MyDerivedIntrusive>(&intrusiveDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedDerivedIntrusiveInstance);
            EXPECT_DOUBLE_EQ(.0223, castedDerivedIntrusiveInstance->m_doubleValue);
            castedDerivedIntrusiveInstance->m_doubleValue = -64.0; // Verify that access doesn't crash due to invalid memory address
            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyBaseIntrusive- succeeds
            MyBaseIntrusive* castedBaseIntrusiveInstance = intrusiveDerivedFromExternalAndIntrusive->Cast<MyBaseIntrusive>(&intrusiveDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedBaseIntrusiveInstance);
            EXPECT_EQ(256U, castedBaseIntrusiveInstance->m_uintValue);
            castedDerivedIntrusiveInstance->m_uintValue = 71U;

            // MyDerivedExternal -> MyIntrusiveDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, derivedExternal->Cast<MyIntrusiveDerivedFromExternalAndIntrusive>(&derivedInstance));

            // MyBaseExternal -> MyIntrusiveDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyIntrusiveDerivedFromExternalAndIntrusive>(&baseInstance));

            // MyBaseIntrusive -> MyBaseIntrusive(using intrusiveDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyBaseIntrusive baseIntrusiveInstance;
            baseIntrusiveInstance.m_uintValue = 3456893U;
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&baseIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // MyDerivedIntrusive-> MyBaseIntrusive(using intrusiveDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyDerivedIntrusive derivedIntrusiveInstance;
            derivedIntrusiveInstance.m_uintValue = 1700U;
            derivedIntrusiveInstance.m_doubleValue = 24.0f;
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&derivedIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // Test Rtti Free functions for class with intrusive Rtti
            enumHierarchyTotalClasses = 0;
            AZ::RttiEnumHierarchy<MyIntrusiveDerivedFromExternalAndIntrusive>(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            EXPECT_EQ(5, enumHierarchyTotalClasses);

            // This should fail
            EXPECT_EQ(nullptr, AZ::RttiCast<MyIntrusiveDerivedFromExternalAndIntrusive*>(&derivedIntrusiveInstance));


            EXPECT_NE(nullptr, AZ::RttiCast<MyIntrusiveDerivedFromExternalAndIntrusive*>(&intrusiveDerivedFromExternalAndIntrusiveInstance));
            void* baseIntrusiveAddress = AZ::RttiAddressOf(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid());
            ASSERT_NE(nullptr, baseIntrusiveAddress);
            EXPECT_EQ(71U, static_cast<MyBaseIntrusive*>(baseIntrusiveAddress)->m_uintValue);

            EXPECT_FALSE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid(), intrusiveDerivedFromExternalAndIntrusiveInstance));
            EXPECT_FALSE(AZ::RttiIsTypeOf<MyConvertibleExternal>(intrusiveDerivedFromExternalAndIntrusiveInstance));

            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid(), intrusiveDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyDerivedExternal>(intrusiveDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiIsTypeOf
            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid(), &intrusiveDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyBaseExternal>(&intrusiveDerivedFromExternalAndIntrusiveInstance));

            EXPECT_EQ(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(intrusiveDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiTypeId
            EXPECT_EQ(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(&intrusiveDerivedFromExternalAndIntrusiveInstance));
        }
    }

    class ReflectionManagerTest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_reflection = AZStd::make_unique<ReflectionManager>();
        }

        void TearDown() override
        {
            m_reflection.reset();

            AllocatorsFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<ReflectionManager> m_reflection;
    };

    class TestReflectedClass
    {
    public:
        static bool s_isReflected;
        static void Reflect(ReflectContext* context)
        {
            s_isReflected = !context->IsRemovingReflection();
        }
    };
    bool TestReflectedClass::s_isReflected = false;

    TEST_F(ReflectionManagerTest, AddContext_AddClass)
    {
        m_reflection->AddReflectContext<SerializeContext>();

        m_reflection->Reflect(&TestReflectedClass::Reflect);
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->RemoveReflectContext<SerializeContext>();

        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }

    TEST_F(ReflectionManagerTest, AddClass_AddContext)
    {
        m_reflection->Reflect(&TestReflectedClass::Reflect);

        m_reflection->AddReflectContext<SerializeContext>();
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->Unreflect(&TestReflectedClass::Reflect);

        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }

    TEST_F(ReflectionManagerTest, UnreflectOnDestruct)
    {
        m_reflection->Reflect(&TestReflectedClass::Reflect);

        m_reflection->AddReflectContext<SerializeContext>();
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection.reset();

        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }

    TEST_F(ReflectionManagerTest, UnreflectReReflect)
    {
        m_reflection->AddReflectContext<SerializeContext>();

        m_reflection->Reflect(&TestReflectedClass::Reflect);
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->Unreflect(&TestReflectedClass::Reflect);
        EXPECT_FALSE(TestReflectedClass::s_isReflected);

        m_reflection->Reflect(&TestReflectedClass::Reflect);
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->RemoveReflectContext<SerializeContext>();
        EXPECT_FALSE(TestReflectedClass::s_isReflected);

        m_reflection->AddReflectContext<SerializeContext>();
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection.reset();
        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }
}
