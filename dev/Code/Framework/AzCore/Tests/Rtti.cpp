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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/thread.h>

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
        AZ_TYPE_INFO(MyBase, "{6A0855E5-6899-482B-B470-C3E5C13D13F5}")
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
        AZ_RTTI(MyBase1, "{F3F97A32-15D2-48FF-B741-B89EA2DD2280}", MyBase)
        int data1MyBase1;
        int data2MyBase1;
    };

    class MyDerived
        : public MyBase1
    {
    public:
        virtual ~MyDerived() {}
        AZ_RTTI(MyDerived, "{3BE0590A-F20F-4056-96AF-C2F0565C2EA5}", MyBase1)
        int dataMyDerived;
    };

    class MyDerived1
    {
    public:
        virtual ~MyDerived1() {}
        AZ_RTTI(MyDerived1, "{527B6166-1A4F-4782-8D06-F228860B1102}")
        int datatypename;
    };

    class MyDerived2
        : public MyDerived
    {
    public:
        virtual ~MyDerived2() {}
        AZ_RTTI(MyDerived2, "{8902C46B-61C5-4294-82A2-06CB61ACA314}", MyDerived)
        int dataMyDerived2;
    };

    class MyClassMix
        : public MyDerived2
        , public MyDerived1
    {
    public:
        virtual ~MyClassMix() {}
        AZ_RTTI(MyClassMix, "{F6CDCF25-3161-46AE-A46C-0F9B8A1027AF}", MyDerived2, MyDerived1)
        int dataMix;
    };

    class MyClassA
    {
    public:
        virtual ~MyClassA() {}
        AZ_RTTI(MyClassA, "{F2D44607-1BB6-4A6D-8D8B-4FDE27B488CF}")
        int dataClassA;
    };

    class MyClassB
    {
    public:
        virtual ~MyClassB() {}
        AZ_RTTI(MyClassB, "{E46477C8-4833-4F8C-A57A-02EAFA0C33D8}")
        int dataClassB;
    };

    class MyClassC
    {
    public:
        virtual ~MyClassC() {}
        AZ_RTTI(MyClassC, "{614F230F-1AD0-419D-8376-18891112F55D}")
        int dataClassC;
    };

    class MyClassD
        : public MyClassA
    {
    public:
        virtual ~MyClassD() {}
        AZ_RTTI(MyClassD, "{8E047831-1445-4D13-8F6F-DD36C871FD05}", MyClassA)
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
        AZ_RTTI(MyClassMaxMix, "{49A7F45B-D039-44ED-A6BF-E500CB84E867}", MyDerived2, MyDerived1, MyClassB, MyClassC, MyClassD)
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
        const AZ::Uuid expectedMtti("{4876C017-0C26-4D0D-9A1F-2A738BAE6449}");
        const AZ::Uuid expectedMtti2("{CBC94693-5ECD-4CBF-A8DB-9B122E697E8D}");
        auto threadFunc = [&expectedMtti, &expectedMtti2]()
        {
            const AZ::Uuid& mtti = azrtti_typeid<MTTI>();
            const AZ::Uuid& mtti2 = azrtti_typeid<MTTI2>();
            EXPECT_FALSE(mtti.IsNull());
            EXPECT_EQ(expectedMtti, mtti);
            EXPECT_FALSE(mtti2.IsNull());
            EXPECT_EQ(expectedMtti2, mtti2);
        };

        AZStd::thread threads[64];
        for (int threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            threads[threadIdx] = AZStd::thread(threadFunc);
        }
        for (auto& thread : threads)
        {
            thread.join();
        }
    }
}
