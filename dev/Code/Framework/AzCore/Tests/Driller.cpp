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
#include <time.h>

#include "TestTypes.h"

#include <AzCore/IO/FileIOEventBus.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Driller/DrillerBus.h>
#include <AzCore/Driller/DrillerRootHandler.h>
#include <AzCore/Driller/DefaultStringPool.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/IO/StreamerComponent.h>

//#define AZ_CORE_DRILLER_COMPARE_TEST
#if defined(AZ_CORE_DRILLER_COMPARE_TEST)
#   include <AzCore/IO/GenericStreams.h>
#   include <AzCore/Serialization/ObjectStream.h>
#   include <AzCore/Serialization/SerializeContext.h>
#endif

using namespace AZ;
using namespace AZ::Debug;

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/Driller_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/Driller_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE_OSX)
#   define AZ_ROOT_TEST_FOLDER "./"
#elif defined(AZ_PLATFORM_ANDROID)
#   define AZ_ROOT_TEST_FOLDER  "/sdcard/"
#elif defined(AZ_PLATFORM_APPLE_IOS)
#   define AZ_ROOT_TEST_FOLDER "/Documents/"
#elif defined(AZ_PLATFORM_APPLE_TV)
#   define AZ_ROOT_TEST_FOLDER "/Library/Caches/"
#else
#   define AZ_ROOT_TEST_FOLDER ""
#endif

namespace // anonymous
{
#if defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
    AZStd::string GetTestFolderPath()
    {
        return AZStd::string(getenv("HOME")) + AZ_ROOT_TEST_FOLDER;
    }
    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName)
    {
        azsnprintf(buffer, bufferLen, "%s%s%s", getenv("HOME"), AZ_ROOT_TEST_FOLDER, fileName);
    }
#else
    AZStd::string GetTestFolderPath()
    {
        return AZ_ROOT_TEST_FOLDER;
    }
    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName)
    {
        azsnprintf(buffer, bufferLen, "%s%s", AZ_ROOT_TEST_FOLDER, fileName);
    }
#endif
} // anonymous namespace

namespace UnitTest
{
    /**
     * MyDriller event bus...
     */
    class MyDrillerInterface
        : public AZ::Debug::DrillerEBusTraits
    {
    public:
        virtual ~MyDrillerInterface() {}
        // define one event X
        virtual void OnEventX(int data) = 0;
        // define a string event
        virtual void OnStringEvent() = 0;
    };

    class MyDrillerCommandInterface
        : public AZ::Debug::DrillerEBusTraits
    {
    public:
        virtual ~MyDrillerCommandInterface() {}

        virtual class MyDrilledObject* RequestDrilledObject() = 0;
    };

    typedef AZ::EBus<MyDrillerInterface> MyDrillerBus;
    typedef AZ::EBus<MyDrillerCommandInterface> MyDrillerCommandBus;

    class MyDrilledObject
        : public MyDrillerCommandBus::Handler
    {
        int i;
    public:
        MyDrilledObject()
            : i(0)
        {
            BusConnect();
        }

        ~MyDrilledObject()
        {
            BusDisconnect();
        }
        //////////////////////////////////////////////////////////////////////////
        // MyDrillerCommandBus
        virtual MyDrilledObject* RequestDrilledObject()
        {
            return this;
        }
        //////////////////////////////////////////////////////////////////////////
        void OnEventX()
        {
            EBUS_EVENT(MyDrillerBus, OnEventX, i);
            ++i;
        }

        void OnStringEvent()
        {
            EBUS_DBG_EVENT(MyDrillerBus, OnStringEvent);
        }
    };


    /**
     * My driller implements the driller interface and an handles the MyDrillerBus events...
     */
    class MyDriller
        : public Driller
        , public MyDrillerBus::Handler
    {
        bool m_isDetailedCapture;
        class MyDrilledObject* drilledObject;
        typedef vector<Param>::type ParamArrayType;
        ParamArrayType m_params;
    public:
        AZ_CLASS_ALLOCATOR(MyDriller, OSAllocator, 0);

        virtual const char*  GroupName() const          { return "TestDrillers"; }
        virtual const char*  GetName() const            { return "MyTestDriller"; }
        virtual const char*  GetDescription() const     { return "MyTestDriller description...."; }
        virtual int          GetNumParams() const       { return static_cast<int>(m_params.size()); }
        virtual const Param* GetParam(int index) const  { return &m_params[index]; }

        MyDriller()
            : m_isDetailedCapture(false)
            , drilledObject(NULL)
        {
            Param isDetailed;
            isDetailed.desc = "IsDetailedDrill";
            isDetailed.name = AZ_CRC("IsDetailedDrill", 0x2155cef2);
            isDetailed.type = Param::PT_BOOL;
            isDetailed.value = 0;
            m_params.push_back(isDetailed);
        }

        virtual void Start(const Param* params = NULL, int numParams = 0)
        {
            m_isDetailedCapture = m_params[0].value != 0;
            if (params)
            {
                for (int i = 0; i < numParams; i++)
                {
                    if (params[i].name == m_params[0].name)
                    {
                        m_isDetailedCapture = params[i].value != 0;
                    }
                }
            }

            EBUS_EVENT_RESULT(drilledObject, MyDrillerCommandBus, RequestDrilledObject);
            AZ_TEST_ASSERT(drilledObject != NULL); /// Make sure we have our object by the time we started the driller

            m_output->BeginTag(AZ_CRC("MyDriller", 0xc3b7dceb));
            m_output->Write(AZ_CRC("OnStart", 0x8b372fca), m_isDetailedCapture);
            // write drilled object initial state
            m_output->EndTag(AZ_CRC("MyDriller", 0xc3b7dceb));

            BusConnect();
        }
        virtual void Stop()
        {
            drilledObject = NULL;
            m_output->BeginTag(AZ_CRC("MyDriller", 0xc3b7dceb));
            m_output->Write(AZ_CRC("OnStop", 0xf6701caa), m_isDetailedCapture);
            m_output->EndTag(AZ_CRC("MyDriller", 0xc3b7dceb));
            BusDisconnect();
        }

        virtual void OnEventX(int data)
        {
            void* ptr = AZ_INVALID_POINTER;
            float f = 3.2f;
            m_output->BeginTag(AZ_CRC("MyDriller", 0xc3b7dceb));
            m_output->Write(AZ_CRC("EventX", 0xc4558ec2), data);
            m_output->Write(AZ_CRC("Pointer", 0x320468a8), ptr);
            m_output->Write(AZ_CRC("Float", 0xc9a55e95), f);
            m_output->EndTag(AZ_CRC("MyDriller", 0xc3b7dceb));
        }

        virtual void OnStringEvent()
        {
            m_output->BeginTag(AZ_CRC("MyDriller", 0xc3b7dceb));
            m_output->BeginTag(AZ_CRC("StringEvent", 0xd1e005df));
            m_output->Write(AZ_CRC("StringOne", 0x56efb231), "This is copied string");
            m_output->Write(AZ_CRC("StringTwo", 0x3d49bea6), "This is referenced string", false);       // don't copy the string if we use string pool, this will be faster as we don't delete the string
            m_output->EndTag(AZ_CRC("StringEvent", 0xd1e005df));
            m_output->EndTag(AZ_CRC("MyDriller", 0xc3b7dceb));
        }
    };

    /**
     *
     */
    class FileStreamDrillerTest
        : public AllocatorsFixture
    {
        DrillerManager* m_drillerManager = nullptr;
        MyDriller* m_driller = nullptr;
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_drillerManager = DrillerManager::Create();
            m_driller = aznew MyDriller;
            // Register driller descriptor
            m_drillerManager->Register(m_driller);
            // check that our driller descriptor is registered
            AZ_TEST_ASSERT(m_drillerManager->GetNumDrillers() == 1);
        }

        void TearDown() override
        {
            // remove our driller descriptor
            m_drillerManager->Unregister(m_driller);
            AZ_TEST_ASSERT(m_drillerManager->GetNumDrillers() == 0);
            DrillerManager::Destroy(m_drillerManager);

            AllocatorsFixture::TearDown();
        }

        /**
         * My Driller data handler.
         */
        class MyDrillerHandler
            : public DrillerHandlerParser
        {
        public:
            static const bool s_isWarnOnMissingDrillers = true;
            int m_lastData;

            MyDrillerHandler()
                : m_lastData(-1) {}

            // From the template query
            DrillerHandlerParser*           FindDrillerHandler(u32 drillerId)
            {
                if (drillerId == AZ_CRC("MyDriller", 0xc3b7dceb))
                {
                    return this;
                }
                return NULL;
            }

            virtual DrillerHandlerParser*   OnEnterTag(u32 tagName)
            {
                (void)tagName;
                return NULL;
            }
            virtual void                    OnData(const DrillerSAXParser::Data& dataNode)
            {
                if (dataNode.m_name == AZ_CRC("OnStart", 0x8b372fca) || dataNode.m_name == AZ_CRC("OnStop", 0xf6701caa))
                {
                    bool isDetailedCapture;
                    dataNode.Read(isDetailedCapture);
                    AZ_TEST_ASSERT(isDetailedCapture == true);
                }
                else if (dataNode.m_name == AZ_CRC("EventX", 0xc4558ec2))
                {
                    int data;
                    dataNode.Read(data);
                    AZ_TEST_ASSERT(data > m_lastData);
                    m_lastData = data;
                }
                else if (dataNode.m_name == AZ_CRC("Pointer", 0x320468a8))
                {
                    AZ::u64 pointer = 0; //< read pointers in u64 to cover all platforms
                    dataNode.Read(pointer);
                    AZ_TEST_ASSERT(pointer == 0x0badf00dul);
                }
                else if (dataNode.m_name == AZ_CRC("Float", 0xc9a55e95))
                {
                    float f;
                    dataNode.Read(f);
                    AZ_TEST_ASSERT(f == 3.2f);
                }
            }
        };

        //////////////////////////////////////////////////////////////////////////

        void run()
        {
            // get our driller descriptor
            Driller* driller = m_drillerManager->GetDriller(0);
            AZ_TEST_ASSERT(driller != NULL);
            AZ_TEST_ASSERT(strcmp(driller->GetName(), "MyTestDriller") == 0);
            AZ_TEST_ASSERT(driller->GetNumParams() == 1);

            // read the default params and make a copy...
            Driller::Param param = *driller->GetParam(0);
            AZ_TEST_ASSERT(strcmp(param.desc, "IsDetailedDrill") == 0);
            AZ_TEST_ASSERT(param.name == AZ_CRC("IsDetailedDrill", 0x2155cef2));
            AZ_TEST_ASSERT(param.type == Driller::Param::PT_BOOL);
            // tweak the default params by enabling detailed drilling
            param.value = 1;

            // create a list of driller we what to drill
            DrillerManager::DrillerListType dillersToDrill;
            DrillerManager::DrillerInfo di;
            di.id = driller->GetId();       // set driller id
            di.params.push_back(param);     // set driller custom params
            dillersToDrill.push_back(di);

            // open a driller output file stream
            // open a driller output file stream
            AZStd::string testFileName  = GetTestFolderPath() + "drilltest.dat";
            DrillerOutputFileStream drillerOutputStream;
            drillerOutputStream.Open(testFileName.c_str(), IO::SystemFile::SF_OPEN_CREATE | IO::SystemFile::SF_OPEN_WRITE_ONLY);

            //////////////////////////////////////////////////////////////////////////
            // Drill an object
            MyDrilledObject myDrilledObject;
            clock_t st = clock();

            // start a driller session with the file stream and the list of drillers
            DrillerSession* drillerSession = m_drillerManager->Start(drillerOutputStream, dillersToDrill);
            // update for N frames
            for (int i = 0; i < 100000; ++i)
            {
                // trigger event X that we want to drill...
                myDrilledObject.OnEventX();
                m_drillerManager->FrameUpdate();
            }
            // stop the drillers
            m_drillerManager->Stop(drillerSession);
            // Stop writing and flush all data
            drillerOutputStream.Close();
            AZ_Printf("Driller", "Compression time %.09f seconds\n", (double)(clock() - st) / CLOCKS_PER_SEC);
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // try to load the drill data
            DrillerInputFileStream drillerInputStream;
            drillerInputStream.Open(testFileName.c_str(), IO::SystemFile::SF_OPEN_READ_ONLY);
            DrillerDOMParser dp;
            AZ_TEST_ASSERT(dp.CanParse() == true);
            dp.ProcessStream(drillerInputStream);
            AZ_TEST_ASSERT(dp.CanParse() == true);
            drillerInputStream.Close();

            u32 startDataId = AZ_CRC("StartData", 0xecf3f53f);
            u32 frameId = AZ_CRC("Frame", 0xb5f83ccd);

            //////////////////////////////////////////////////////////////////////////
            // read all data
            const DrillerDOMParser::Node* root = dp.GetRootNode();
            int lastFrame = -1;
            int lastData = -1;
            for (DrillerDOMParser::Node::NodeListType::const_iterator iter = root->m_tags.begin(); iter != root->m_tags.end(); ++iter)
            {
                const DrillerDOMParser::Node* node = &*iter;
                u32 name = node->m_name;
                AZ_TEST_ASSERT(name == startDataId || name == frameId);
                if (name == startDataId)
                {
                    unsigned int currentPlatform;
                    node->GetDataRequired(AZ_CRC("Platform", 0x3952d0cb))->Read(currentPlatform);
                    AZ_TEST_ASSERT(currentPlatform == AZ::g_currentPlatform);
                    const DrillerDOMParser::Node* drillerNode = node->GetTag(AZ_CRC("Driller", 0xa6e1fb73));
                    AZ_TEST_ASSERT(drillerNode != NULL);
                    AZ::u32  drillerName;
                    drillerNode->GetDataRequired(AZ_CRC("Name", 0x5e237e06))->Read(drillerName);
                    AZ_TEST_ASSERT(drillerName == m_driller->GetId());
                    const DrillerDOMParser::Node* paramNode = drillerNode->GetTag(AZ_CRC("Param", 0xa4fa7c89));
                    AZ_TEST_ASSERT(paramNode != NULL);
                    u32 paramName;
                    char paramDesc[128];
                    int paramType;
                    int paramValue;
                    paramNode->GetDataRequired(AZ_CRC("Name", 0x5e237e06))->Read(paramName);
                    AZ_TEST_ASSERT(paramName == param.name);
                    paramNode->GetDataRequired(AZ_CRC("Description", 0x6de44026))->Read(paramDesc, AZ_ARRAY_SIZE(paramDesc));
                    AZ_TEST_ASSERT(strcmp(paramDesc, param.desc) == 0);
                    paramNode->GetDataRequired(AZ_CRC("Type", 0x8cde5729))->Read(paramType);
                    AZ_TEST_ASSERT(paramType == param.type);
                    paramNode->GetDataRequired(AZ_CRC("Value", 0x1d775834))->Read(paramValue);
                    AZ_TEST_ASSERT(paramValue == param.value);
                }
                else
                {
                    int curFrame;
                    node->GetDataRequired(AZ_CRC("FrameNum", 0x85a1a919))->Read(curFrame);
                    AZ_TEST_ASSERT(curFrame > lastFrame); // check order
                    lastFrame = curFrame;
                    const DrillerDOMParser::Node* myDrillerNode = node->GetTag(AZ_CRC("MyDriller", 0xc3b7dceb));
                    AZ_TEST_ASSERT(myDrillerNode != NULL);
                    const DrillerDOMParser::Data* dataEntry;
                    dataEntry = myDrillerNode->GetData(AZ_CRC("EventX", 0xc4558ec2));
                    if (dataEntry)
                    {
                        int data;
                        dataEntry->Read(data);
                        AZ_TEST_ASSERT(data > lastData);
                        lastData = data;
                        dataEntry = myDrillerNode->GetData(AZ_CRC("Pointer", 0x320468a8));
                        AZ_TEST_ASSERT(dataEntry);
                        unsigned int ptr;
                        dataEntry->Read(ptr);
                        AZ_TEST_ASSERT(static_cast<size_t>(ptr) == reinterpret_cast<size_t>(AZ_INVALID_POINTER));
                        float f;
                        dataEntry = myDrillerNode->GetData(AZ_CRC("Float", 0xc9a55e95));
                        AZ_TEST_ASSERT(dataEntry);
                        dataEntry->Read(f);
                        AZ_TEST_ASSERT(f == 3.2f);
                    }
                    else
                    {
                        bool isDetailedCapture;
                        dataEntry = myDrillerNode->GetData(AZ_CRC("OnStart", 0x8b372fca));
                        if (dataEntry)
                        {
                            dataEntry->Read(isDetailedCapture);
                        }
                        else
                        {
                            myDrillerNode->GetDataRequired(AZ_CRC("OnStop", 0xf6701caa))->Read(isDetailedCapture);
                        }
                        AZ_TEST_ASSERT(isDetailedCapture == true);
                    }
                }
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Read that with Tag Handlers
            drillerInputStream.Open(testFileName.c_str(), IO::SystemFile::SF_OPEN_READ_ONLY);

            DrillerRootHandler<MyDrillerHandler> rootHandler;
            DrillerSAXParserHandler dhp(&rootHandler);
            dhp.ProcessStream(drillerInputStream);
            // Verify that Default templates forked fine...
            AZ_TEST_ASSERT(rootHandler.m_drillerSessionInfo.m_platform == AZ::g_currentPlatform);
            AZ_TEST_ASSERT(rootHandler.m_drillerSessionInfo.m_drillers.size() == 1);
            {
                const DrillerManager::DrillerInfo& dinfo = rootHandler.m_drillerSessionInfo.m_drillers.front();
                AZ_TEST_ASSERT(dinfo.id == AZ_CRC("MyTestDriller", 0x5cc4edf5));
                AZ_TEST_ASSERT(dinfo.params.size() == 1);
                AZ_TEST_ASSERT(strcmp(param.desc, "IsDetailedDrill") == 0);
                AZ_TEST_ASSERT(param.name == AZ_CRC("IsDetailedDrill", 0x2155cef2));
                AZ_TEST_ASSERT(param.type == Driller::Param::PT_BOOL);
                // tweak the default params by enabling detailed drilling
                param.value = 1;
                AZ_TEST_ASSERT(dinfo.params[0].name == AZ_CRC("IsDetailedDrill", 0x2155cef2));
                AZ_TEST_ASSERT(dinfo.params[0].desc == NULL); // ignored for now
                AZ_TEST_ASSERT(dinfo.params[0].type == Driller::Param::PT_BOOL);
                AZ_TEST_ASSERT(dinfo.params[0].value == 1);
            }
            drillerInputStream.Close();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //

            //////////////////////////////////////////////////////////////////////////
        }
    };

    TEST_F(FileStreamDrillerTest, Test)
    {
        run();
    }

    /**
     *
     */
    class StringPoolDrillerTest
        : public AllocatorsFixture
    {
        DrillerManager* m_drillerManager = nullptr;
        MyDriller* m_driller = nullptr;
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_drillerManager = DrillerManager::Create();
            m_driller = aznew MyDriller;
            // Register driller descriptor
            m_drillerManager->Register(m_driller);
            // check that our driller descriptor is registered
            AZ_TEST_ASSERT(m_drillerManager->GetNumDrillers() == 1);
        }

        void TearDown() override
        {
            // remove our driller descriptor
            m_drillerManager->Unregister(m_driller);
            AZ_TEST_ASSERT(m_drillerManager->GetNumDrillers() == 0);
            DrillerManager::Destroy(m_drillerManager);

            AllocatorsFixture::TearDown();
        }

        /**
 * My Driller data handler.
 */
        class MyDrillerHandler
            : public DrillerHandlerParser
        {
        public:
            static const bool s_isWarnOnMissingDrillers = true;

            MyDrillerHandler() {}

            // From the template query
            DrillerHandlerParser*           FindDrillerHandler(u32 drillerId)
            {
                if (drillerId == AZ_CRC("MyDriller", 0xc3b7dceb))
                {
                    return this;
                }
                return NULL;
            }

            virtual DrillerHandlerParser*   OnEnterTag(u32 tagName)
            {
                if (tagName == AZ_CRC("StringEvent", 0xd1e005df))
                {
                    return this;
                }
                return NULL;
            }
            virtual void                    OnData(const DrillerSAXParser::Data& dataNode)
            {
                if (dataNode.m_name == AZ_CRC("OnStart", 0x8b372fca) || dataNode.m_name == AZ_CRC("OnStop", 0xf6701caa))
                {
                    bool isDetailedCapture;
                    dataNode.Read(isDetailedCapture);
                    AZ_TEST_ASSERT(isDetailedCapture == true);
                }
                else if (dataNode.m_name == AZ_CRC("StringOne", 0x56efb231))
                {
                    // read string as a copy
                    char stringCopy[256];
                    dataNode.Read(stringCopy, AZ_ARRAY_SIZE(stringCopy));
                    AZ_TEST_ASSERT(strcmp(stringCopy, "This is copied string") == 0);
                }
                else if (dataNode.m_name == AZ_CRC("StringTwo", 0x3d49bea6))
                {
                    // read string as reference if possible, otherwise read it as a copy
                    const char* stringRef = dataNode.ReadPooledString();
                    AZ_TEST_ASSERT(strcmp(stringRef, "This is referenced string") == 0);
                }
            }
        };

        void run()
        {
            // get our driller descriptor
            Driller* driller = m_drillerManager->GetDriller(0);
            Driller::Param param = *driller->GetParam(0);
            param.value = 1;
            // create a list of driller we what to drill
            DrillerManager::DrillerListType dillersToDrill;
            DrillerManager::DrillerInfo di;
            di.id = driller->GetId();       // set driller id
            di.params.push_back(param);     // set driller custom params
            dillersToDrill.push_back(di);

            MyDrilledObject myDrilledObject;

            // open a driller output file stream
            AZStd::string testFileName  = GetTestFolderPath() + "stringpooldrilltest.dat";
            DrillerOutputFileStream drillerOutputStream;
            DrillerInputFileStream drillerInputStream;
            DrillerDefaultStringPool stringPool;
            DrillerSession* drillerSession;
            DrillerRootHandler<MyDrillerHandler> rootHandler;
            DrillerSAXParserHandler dhp(&rootHandler);

            //////////////////////////////////////////////////////////////////////////
            // Drill an object without string pools
            drillerOutputStream.Open(testFileName.c_str(), IO::SystemFile::SF_OPEN_CREATE | IO::SystemFile::SF_OPEN_WRITE_ONLY);

            // start a driller session with the file stream and the list of drillers
            drillerSession = m_drillerManager->Start(drillerOutputStream, dillersToDrill);
            // update for N frames
            for (int i = 0; i < 100000; ++i)
            {
                myDrilledObject.OnStringEvent();
                m_drillerManager->FrameUpdate();
            }
            // stop the drillers
            m_drillerManager->Stop(drillerSession);
            // Stop writing and flush all data
            drillerOutputStream.Close();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Read all data that was written without string pool, in a stream that uses one.
            drillerInputStream.Open(testFileName.c_str(), IO::SystemFile::SF_OPEN_READ_ONLY);
            drillerInputStream.SetStringPool(&stringPool);

            dhp.ProcessStream(drillerInputStream);
            drillerInputStream.Close();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Drill an object without string pools
            drillerOutputStream.Open(testFileName.c_str(), IO::SystemFile::SF_OPEN_CREATE | IO::SystemFile::SF_OPEN_WRITE_ONLY);
            stringPool.Reset();
            drillerOutputStream.SetStringPool(&stringPool); // set the string pool on save

            // start a driller session with the file stream and the list of drillers
            drillerSession = m_drillerManager->Start(drillerOutputStream, dillersToDrill);
            // update for N frames
            for (int i = 0; i < 100000; ++i)
            {
                myDrilledObject.OnStringEvent();
                m_drillerManager->FrameUpdate();
            }
            // stop the drillers
            m_drillerManager->Stop(drillerSession);
            // Stop writing and flush all data
            drillerOutputStream.Close();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Read all data that was written without string pool, in a stream that uses one.
            stringPool.Reset();
            drillerInputStream.Open(testFileName.c_str(), IO::SystemFile::SF_OPEN_READ_ONLY);
            drillerInputStream.SetStringPool(&stringPool);

            dhp.ProcessStream(drillerInputStream);
            drillerInputStream.Close();
            //////////////////////////////////////////////////////////////////////////
        }
    };

    TEST_F(StringPoolDrillerTest, Test)
    {
        run();
    }

    /**
     *
     */
    class DrillFileStreamCheck
    {
    public:
        void run()
        {
            // open and read drilled file
        }
    };
#if defined(AZ_CORE_DRILLER_COMPARE_TEST)
    /**
     * Driller compare test
     */
    class DrillerCompareTest
        : public AllocatorsFixture
    {
    public:
        struct DrillEventA
        {
            AZ_CLASS_ALLOCATOR(DrillEventA, AZ::SystemAllocator, 0);

            float a;
            float b;
            float c;

            static void Reflect(SerializeContext& context)
            {
                context.Class<DrillEventA>("DrillEventA", "{F80D00B2-B4B8-4DA7-A0F2-7C7CBC567125}")->
                    Field("a", &DrillEventA::a)->
                    Field("b", &DrillEventA::b)->
                    Field("c", &DrillEventA::c);
            }
        };

        struct DrillEventB
        {
            AZ_CLASS_ALLOCATOR(DrillEventB, AZ::SystemAllocator, 0);

            int index;
            bool isTrue;

            static void Reflect(SerializeContext& context)
            {
                context.Class<DrillEventB>("DrillEventB", "{F70045C1-27FA-4AEE-A0A2-6655A3165320}")->
                    Field("index", &DrillEventB::index)->
                    Field("isTrue", &DrillEventB::isTrue);
            }
        };

        struct DrillEventC
        {
            AZ_CLASS_ALLOCATOR(DrillEventC, AZ::SystemAllocator, 0);

            AZStd::string text;

            static void Reflect(SerializeContext& context)
            {
                context.Class<DrillEventC>("DrillEventC", "{4F5ECA68-C470-4EFA-8925-7F0A52D0D2C6}")->
                    Field("text", &DrillEventC::text);
            }
        };

        void OnDrillEventA(ObjectStream* output, float a, float b, float c)
        {
            DrillEventA data;
            data.a = a;
            data.b = b;
            data.c = c;
            output->WriteClass(&data);
        }

        void OnDrillEventB(ObjectStream* output, int index, bool isTrue)
        {
            DrillEventB data;
            data.index = index;
            data.isTrue = isTrue;
            output->WriteClass(&data);
        }

        void OnDrillEventC(ObjectStream* output, const AZStd::string& text)
        {
            DrillEventC data;
            data.text = text;
            output->WriteClass(&data);
        }

        void OnDrillEventA(DrillerOutputStream& output, float a, float b, float c)
        {
            output.BeginTag(AZ_CRC("TestDriller", 0xc80b1bde));
            output.BeginTag(AZ_CRC("EventA", 0xa03e2602));
            output.Write(AZ_CRC("a", 0xe8b7be43), a);
            output.Write(AZ_CRC("b", 0x71beeff9), b);
            output.Write(AZ_CRC("c", 0x06b9df6f), c);
            output.EndTag(AZ_CRC("EventA", 0xa03e2602));
            output.EndTag(AZ_CRC("TestDriller", 0xc80b1bde));
        }

        void OnDrillEventB(DrillerOutputStream& output, int index, bool isTrue)
        {
            output.BeginTag(AZ_CRC("TestDriller", 0xc80b1bde));
            output.BeginTag(AZ_CRC("EventB", 0x393777b8));
            output.Write(AZ_CRC("index", 0x80736701), index);
            output.Write(AZ_CRC("isTrue", 0xfcf3f6bf), isTrue);
            output.EndTag(AZ_CRC("EventB", 0x393777b8));
            output.EndTag(AZ_CRC("TestDriller", 0xc80b1bde));
        }

        void OnDrillEventC(DrillerOutputStream& output, const AZStd::string& text)
        {
            output.BeginTag(AZ_CRC("TestDriller", 0xc80b1bde));
            output.BeginTag(AZ_CRC("EventC", 0x4e30472e));
            output.Write(AZ_CRC("text", 0x3b8ba7c7), text);
            output.EndTag(AZ_CRC("EventC", 0x4e30472e));
            output.EndTag(AZ_CRC("TestDriller", 0xc80b1bde));
        }


        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            IO::Streamer::Descriptor streamerDesc;
            AZStd::string testFolder  = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                streamerDesc.m_fileMountPoint = testFolder.c_str();
            }
            IO::Streamer::Create(streamerDesc);

            // enable compression on the device (we can find device by name and/or by drive name, etc.)
            IO::Device* device = IO::Streamer::Instance().FindDevice(testFolder.c_str());
            AZ_TEST_ASSERT(device); // we must have a valid device
            bool result = device->RegisterCompressor(aznew IO::CompressorZLib());
            AZ_TEST_ASSERT(result); // we should be able to register

            //ObjectStream::Descriptor objstreamDesc;
            //ObjectStream::InitAsyncMode(objstreamDesc);
        }


        void TearDown() override
        {
            //ObjectStream::ShutdownAsyncMode();
            IO::Streamer::Destroy();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void run()
        {
            const int memoryBufferSize = 10 * 1024 * 1024;
            const int numIterations = 100000;

            AZ_Printf("Driller", "\nDriller test compare:");

            AZStd::string eventCText("EventC text");

            {
                DrillerOutputMemoryStream memoryStream(memoryBufferSize);

                // Drill events
                clock_t start = clock();

                // update for N frames
                for (int i = 0; i < numIterations; ++i)
                {
                    int eventId = i % 3;
                    if (eventId == 0)
                    {
                        OnDrillEventA(memoryStream, (float)i * 3.14f, (float)i * 0.11f, (float)i * (float)i);
                    }
                    else if (eventId == 1)
                    {
                        OnDrillEventB(memoryStream, i, (i % 2 == 0) ? true : false);
                    }
                    else
                    {
                        OnDrillEventC(memoryStream, /*AZStd::string::format("Current index %d",i)*/ eventCText);
                    }
                }

                clock_t end = clock();

                // write data to a file (with compression)
                //////////////////////////////////////////////////////////////////////////
                // Compressed file with no extra seek points  write a compressed file (no extra sync points)
                IO::Stream* stream = IO::Streamer::Instance().RegisterFileStream("drillerOldData.dat", IO::Streamer::FS_WRITE_CREATE);
                AZ_TEST_ASSERT(stream);
                // setup compression parameters
                stream->WriteCompressed(IO::CompressorZLib::TypeId(), 0, 2);
                IO::Streamer::Instance().Write(stream, memoryStream.GetData(), memoryStream.GetDataSize());

                AZ_Printf("Driller", "Drill old framework time %.09f seconds dataSize: %u compressed: %u ratio:%.2f\n", (double)(end - start) / CLOCKS_PER_SEC, memoryStream.GetDataSize(), stream->CompressedSize(), (float)memoryStream.GetDataSize() / (float)stream->CompressedSize());

                IO::Streamer::Instance().UnRegisterStream(stream);
            }

            {
                SerializeContext context;
                DrillEventA::Reflect(context);
                DrillEventB::Reflect(context);
                DrillEventC::Reflect(context);

                void* memoryBuffer = azmalloc(memoryBufferSize);
                IO::MemoryStream memoryStream(memoryBuffer, memoryBufferSize);
                ObjectStream* objStream = ObjectStream::Create(&memoryStream, context, ObjectStream::ST_BINARY);

                clock_t start = clock();

                // update for N frames
                for (int i = 0; i < numIterations; ++i)
                {
                    int eventId = i % 3;
                    if (eventId == 0)
                    {
                        OnDrillEventA(objStream, (float)i * 3.14f, (float)i * 0.11f, (float)i * (float)i);
                    }
                    else if (eventId == 1)
                    {
                        OnDrillEventB(objStream, i, (i % 2 == 0) ? true : false);
                    }
                    else
                    {
                        OnDrillEventC(objStream, /*AZStd::string::format("Current index %d",i)*/ eventCText);
                    }
                }

                objStream->Finalize();

                clock_t end = clock();

                // write data to a file (with compression)
                //////////////////////////////////////////////////////////////////////////
                // Compressed file with no extra seek points  write a compressed file (no extra sync points)
                IO::Stream* stream = IO::Streamer::Instance().RegisterFileStream("drillerNewData.dat", IO::Streamer::FS_WRITE_CREATE);
                AZ_TEST_ASSERT(stream);
                // setup compression parameters
                stream->WriteCompressed(IO::CompressorZLib::TypeId(), 0, 2);
                IO::Streamer::Instance().Write(stream, memoryStream.GetData(), memoryStream.GetLength());

                AZ_Printf("Driller", "Drill new framework time %.09f seconds dataSize: %u compressed: %u ratio:%.2f\n", (double)(end - start) / CLOCKS_PER_SEC, memoryStream.GetLength(), stream->CompressedSize(), (float)memoryStream.GetLength() / (float)stream->CompressedSize());

                IO::Streamer::Instance().UnRegisterStream(stream);

                azfree(memoryBuffer);
            }
        }
    };

    TEST_F(DrillerCompareTest, Test)
    {
        run();
    }
#endif // AZ_CORE_DRILLER_COMPARE_TEST


    /**
     * Driller application test
     */
    TEST(DrillerApplication, Test)
    {
        ComponentApplication app;

        //////////////////////////////////////////////////////////////////////////
        // Create application environment code driven
        ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        Entity* systemEntity = app.Create(appDesc);

        systemEntity->CreateComponent<MemoryComponent>();
        systemEntity->CreateComponent<StreamerComponent>(); // note that this component is what registers the streamer driller

        systemEntity->Init();
        systemEntity->Activate();

        {
            // open a driller output file stream
            char testFileName[AZ_MAX_PATH_LEN];
            MakePathFromTestFolder(testFileName, AZ_MAX_PATH_LEN, "drillapptest.dat");
            DrillerOutputFileStream fs;
            fs.Open(testFileName, IO::SystemFile::SF_OPEN_CREATE | IO::SystemFile::SF_OPEN_WRITE_ONLY);

            // create a list of driller we what to drill
            DrillerManager::DrillerListType drillersToDrill;
            DrillerManager::DrillerInfo di;
            di.id = AZ_CRC("TraceMessagesDriller", 0xa61d1b00);
            drillersToDrill.push_back(di);
            di.id = AZ_CRC("StreamerDriller", 0x2f27ed88);
            drillersToDrill.push_back(di);
            di.id = AZ_CRC("MemoryDriller", 0x1b31269d);
            drillersToDrill.push_back(di);

            ASSERT_NE(nullptr, app.GetDrillerManager());
            DrillerSession* drillerSession = app.GetDrillerManager()->Start(fs, drillersToDrill);
            ASSERT_NE(nullptr, drillerSession);

            const int numOfFrames = 10000;
            void* memory = NULL;
            for (int i = 0; i < numOfFrames; ++i)
            {
                memory = azmalloc(rand() % 2048 + 1);
                azfree(memory);
                app.Tick();
            }

            app.GetDrillerManager()->Stop(drillerSession); // stop session manually
            fs.Close(); // close the file with driller info
        }

        app.Destroy();
        //////////////////////////////////////////////////////////////////////////
    }
}
