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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/EBus/Results.h>

using namespace AZ;

namespace UnitTest
{
    class EBus
        : public AllocatorsFixture
    {
    public:
        EBus()
            : AllocatorsFixture(128, false)
        {
        }
    };
    /**
    * Test for Single Event group on a single Event bus.
    * This is the simplest and most memory efficient way to
    * trigger events. It's technically signal/slot way, while
    * still allowing for you to group function calls as you wish.
    */
    /**
    * This example will demonstrate how to create
    * an EvenGroup that will have only one bus and only
    * one event support.
    * Technically this is just a regular callback interface.
    */
    namespace SingleEventGroupTest
    {
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusInterface settings
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now let have some
        */
        class MyEventHandler
            : public MyEventGroupBus::Handler                  /* we will listen for MyEventGroupBus */
        {
        public:
            int actionCalls;
            int sumCalls;

            MyEventHandler()
                : actionCalls(0)
                , sumCalls(0)
            {
                // We will bind at construction time to the bus. Disconnect is automatic when the object is
                // destroyed or we can call BusDisconnect()
                EXPECT_FALSE(BusIsConnected());
                BusConnect();
                EXPECT_TRUE(BusIsConnected());
            }
            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) called\n", x, y); ++actionCalls;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y; AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) called\n", sum, x, y); ++sumCalls; return sum;
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }

    TEST_F(EBus, SingleEventGroupTest)
    {
        using namespace SingleEventGroupTest;
        MyEventHandler meh; // We bind on construction

        // Signal OnAction event
        MyEventGroupBus::Broadcast(&MyEventGroupBus::Events::OnAction, 1.0f, 2.0f);
        EXPECT_EQ(1, meh.actionCalls);
        EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
        EXPECT_EQ(2, meh.actionCalls);

        // Signal OnSum event
        EBUS_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
        EXPECT_EQ(1, meh.sumCalls);

        // How about we want to store the event result?
        // We can just store the last value (\note many assignments will happen)
        float lastResult = 0.0f;
        EBUS_EVENT_RESULT(lastResult, MyEventGroupBus, OnSum, 2.0f, 5.0f);
        EXPECT_EQ(2, meh.sumCalls);
        EXPECT_EQ(7.0f, lastResult);

        EXPECT_TRUE(meh.BusIsConnected());
        meh.BusDisconnect(); // we disconnect from receiving events.
        EXPECT_FALSE(meh.BusIsConnected());

        EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);  // this signal will NOT trigger any calls.
        EXPECT_EQ(2, meh.actionCalls);
    };

    /**
    * Test a case when you have single communication bus, but you can have multiple event handlers.
    */
    namespace SingleBusMultHand
    {
        /**
        * Single bus multiple event groups is the default setting for the event group.
        * Unless we want to modify the allocator, we don't need to change anything.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            virtual void    DestroyMe()
            {
                delete this;
            }
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::Handler                  /* we will listen for MyEventGroupBus */
        {
        public:
            int actionCalls;
            int sumCalls;

            AZ_CLASS_ALLOCATOR(MyEventHandler, AZ::SystemAllocator, 0);

            MyEventHandler()
                : actionCalls(0)
                , sumCalls(0)
            {
                // We will bind at construction time to the bus. Disconnect is automatic when the object is
                // destroyed or we can call BusDisconnect()
                BusConnect();
            }
            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) called\n", x, y); ++actionCalls;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y; AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) called\n", sum, x, y); ++sumCalls; return sum;
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }

    TEST_F(EBus, SingleBusMultHand)
    {
        using namespace SingleBusMultHand;
        {
            MyEventHandler meh;
            MyEventHandler meh1;

            // Signal OnAction event
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(1, meh.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);

            // Signal OnSum event
            EBUS_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(1, meh.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);

            // How about we want to store the event result?
            // We can just store the last value (\note many assignments will happen)
            float lastResult = 0.0f;
            EBUS_EVENT_RESULT(lastResult, MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(2, meh.sumCalls);
            EXPECT_EQ(2, meh1.sumCalls);
            EXPECT_EQ(7.0f, lastResult);

            // How about if we want to store all the results...
            EBusAggregateResults<float> allResults;
            EBUS_EVENT_RESULT(allResults, MyEventGroupBus, OnSum, 2.0f, 3.0f);
            EXPECT_EQ(3, meh.sumCalls);
            EXPECT_EQ(3, meh1.sumCalls);
            EXPECT_EQ(2, allResults.values.size()); // we should have 2 results
            EXPECT_EQ(5.0f, allResults.values[0]);
            EXPECT_EQ(5.0f, allResults.values[1]);

            meh.BusDisconnect(); // we disconnect from receiving events.

            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);  // this signal will NOT trigger any calls.
            EXPECT_EQ(1, meh.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls); // not disconnected
        }

        {
            // Test removing handlers while in messages - forward iterator
            MyEventHandler* handler1 = aznew MyEventHandler();
            MyEventHandler* handler2 = aznew MyEventHandler();
            (void)handler1; (void)handler2;

            EXPECT_EQ(2, MyEventGroupBus::GetTotalNumOfEventHandlers());

            EBUS_EVENT(MyEventGroupBus, DestroyMe);

            EXPECT_EQ(0, MyEventGroupBus::GetTotalNumOfEventHandlers());

            // Test removing handlers while in messages - reverse iterator
            handler1 = aznew MyEventHandler();
            handler2 = aznew MyEventHandler();

            EXPECT_EQ(2, MyEventGroupBus::GetTotalNumOfEventHandlers());

            EBUS_EVENT_REVERSE(MyEventGroupBus, DestroyMe);

            EXPECT_EQ(0, MyEventGroupBus::GetTotalNumOfEventHandlers());
        }
    }

    /**
    * Test a case when you have single communication bus, but you can have multiple ORDERED event handlers.
    */
    namespace SingleBusMultOrdHand
    {
        /**
        * Here we will define an EBus interface that will be sorted by a specific order.
        * This order will be int (stored in the interface - which we go in the user implementation (MyEventGroupBus::Handler)
        * and implement the Required "bool Compare(const Interface* rhs) const function.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        protected:
            int m_order;  ///< Some ordering value.
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusInterface settings
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered;
            //////////////////////////////////////////////////////////////////////////

            /// Required function because we use EBusHandlerPolicy::MultipleAndOrdered
            bool Compare(const MyEventGroup* rhs) const { return m_order < rhs->m_order; }

            virtual ~MyEventGroup() {}

            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::Handler
        {
        public:
            int actionCalls;
            int sumCalls;

            MyEventHandler(int order)  // we pass the handler order value at construction.
                : actionCalls(0)
                , sumCalls(0)
            {
                m_order = order; ///< Must be set before we connect, so we are inserted in the right place!
                // We will bind at construction time to the bus. Disconnect is automatic when the object is
                // destroyed or we can call BusDisconnect()
                MyEventGroupBus::Handler::BusConnect();
            }
            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) order %d called\n", x, y, m_order); ++actionCalls; s_eventOrder[s_eventOrderIndex++] = this;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y; AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) order %d called\n", sum, x, y, m_order); ++sumCalls; s_eventOrder[s_eventOrderIndex++] = this; return sum;
            }
            //////////////////////////////////////////////////////////////////////////

            static MyEventHandler* s_eventOrder[2];
            static int  s_eventOrderIndex;
        };

        MyEventHandler* MyEventHandler::s_eventOrder[2];
        int MyEventHandler::s_eventOrderIndex = 0;
    }

    TEST_F(EBus, SingleBusMultOrdHand)
    {
        using namespace SingleBusMultOrdHand;
        {
            MyEventHandler meh(100);      /// <-- Higher order value, it will be called second!
            MyEventHandler meh1(0);  /// <-- Lower order value, it will be called first!

            // Signal OnAction event
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(1, meh.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);

            // Signal OnAction event in reverse order
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            EBUS_EVENT_REVERSE(MyEventGroupBus, OnAction, 3.0f, 4.0f);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(2, meh.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls);

            // Signal OnSum event
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            EBUS_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(1, meh.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);

            // Signal OnSum event in reverse order
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            EBUS_EVENT_REVERSE(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(2, meh.sumCalls);
            EXPECT_EQ(2, meh1.sumCalls);

            // How about we want to store the event result? Let's try with the last result.
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            float lastResult = 0.0f;
            EBUS_EVENT_RESULT(lastResult, MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(3, meh.sumCalls);
            EXPECT_EQ(3, meh1.sumCalls);
            EXPECT_EQ(7.0f, lastResult);

            // How about we want to store the event result? Let's try with the last result. Execute in reverse order
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            lastResult = 0.0f;
            EBUS_EVENT_RESULT_REVERSE(lastResult, MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(4, meh.sumCalls);
            EXPECT_EQ(4, meh1.sumCalls);
            EXPECT_EQ(7.0f, lastResult);

            // How about if we want to store all the results...
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            EBusAggregateResults<float> allResults;
            EBUS_EVENT_RESULT(allResults, MyEventGroupBus, OnSum, 2.0f, 3.0f);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(5, meh.sumCalls);
            EXPECT_EQ(5, meh1.sumCalls);
            EXPECT_EQ(2, allResults.values.size()); // we should have 2 results
            EXPECT_EQ(5.0f, allResults.values[0]);
            EXPECT_EQ(5.0f, allResults.values[1]);

            // How about if we want to store all the results... in reverse order
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            allResults.values.clear();
            EBUS_EVENT_RESULT_REVERSE(allResults, MyEventGroupBus, OnSum, 2.0f, 3.0f);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(6, meh.sumCalls);
            EXPECT_EQ(6, meh1.sumCalls);
            EXPECT_EQ(2, allResults.values.size()); // we should have 2 results
            EXPECT_EQ(5.0f, allResults.values[0]);
            EXPECT_EQ(5.0f, allResults.values[1]);

            meh1.BusDisconnect(); // we disconnect from receiving events.

            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);  // this signal will NOT trigger any calls.
            EXPECT_EQ(2, meh1.actionCalls);
            EXPECT_EQ(3, meh.actionCalls); // not disconnected
        }
    }

    /**
    * Tests multiple communication buses but only
    * one event handler per bus. A great example where to use this
    * is if you user game components/entities and you have one for position
    * lets call it placement component! Normally you can have only position for
    * one entity in the world. So one bus for each object/entity and one handler per bus.
    */
    namespace MultBusSingleHndl
    {
        /**
        * Configure this event group to allow multiple buses but one handler per bus.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus Settings
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            // #TODO: Without this line, the test is testing the wrong case. With this line, the test crashes.
            //static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            typedef unsigned int BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            virtual void DestroyMe()
            {
                delete this;
            }
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(MyEventHandler, AZ::SystemAllocator, 0);

            int actionCalls;
            int sumCalls;

            MyEventHandler(const MyEventGroupBus::BusIdType busId)
                : actionCalls(0)
                , sumCalls(0)
            {
                BusConnect(busId); // connect to the specific bus
            }
            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) on busId: %d called\n", x, y, m_busPtr->m_busId);
                ++actionCalls;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y;
                AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) on busId: %d called\n", sum, x, y, m_busPtr->m_busId);
                ++sumCalls;
                return sum;
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }
    TEST_F(EBus, MultBusSingleHndl)
    {
        using namespace MultBusSingleHndl;
        {
            MyEventHandler meh0(0);   /// <-- Bind to bus 0
            MyEventHandler meh1(1);  /// <-- Bind to bus 1

            // Test handlers' enumeration functionality
            EXPECT_EQ(&meh0, MyEventGroupBus::FindFirstHandler(0));
            EXPECT_EQ(&meh1, MyEventGroupBus::FindFirstHandler(1));
            EXPECT_EQ(nullptr, MyEventGroupBus::FindFirstHandler(3));
            //EXPECT_TRUE(MyEventGroupBus::FindFirstHandler() == &meh0 || MyEventGroupBus::FindFirstHandler() == &meh1);

            // Signal OnAction event on all buses
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(1, meh0.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);

            // Signal OnSum event
            EBUS_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(1, meh0.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);

            // Signal OnAction event on bus 0
            EBUS_EVENT_ID(0, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);

            // Signal OnAction event on bus 1
            EBUS_EVENT_ID(1, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls);

            // When you call event very often... you can cache the the BUS pointer.
            // This is recommended only for performance critical systems, all you save is
            // a hash search.
            MyEventGroupBus::BusPtr busPtr;
            MyEventGroupBus::Bind(busPtr, 1);
            // Now signal this event on the specified bus.
            EBUS_EVENT_PTR(busPtr, MyEventGroupBus, OnAction, 2.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(3, meh1.actionCalls);

            //// How about we want to store the event result? Let's try with the last result.
            float lastResult = 0.0f;
            EBUS_EVENT_ID_RESULT(lastResult, 0, MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);
            EXPECT_EQ(7.0f, lastResult);

            EBUS_EVENT_ID_RESULT(lastResult, 1, MyEventGroupBus, OnSum, 6.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);
            EXPECT_EQ(2, meh1.sumCalls);
            EXPECT_EQ(11.0f, lastResult);

            // How about if we want to store all the results...
            EBusAggregateResults<float> allResults;
            EBUS_EVENT_RESULT(allResults, MyEventGroupBus, OnSum, 2.0f, 3.0f);
            EXPECT_EQ(3, meh0.sumCalls);
            EXPECT_EQ(3, meh1.sumCalls);
            EXPECT_EQ(2, allResults.values.size()); // we should have 2 results
            EXPECT_EQ(5.0f, allResults.values[0]);
            EXPECT_EQ(5.0f, allResults.values[1]);

            meh0.BusDisconnect(); // we disconnect from receiving events.

            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);  // this signal will NOT trigger any calls.
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(4, meh1.actionCalls); // not disconnected
        }

        {
            // Test removing handlers while in messages - forward iterator
            MyEventHandler* handler1 = aznew MyEventHandler(0);
            MyEventHandler* handler2 = aznew MyEventHandler(1);
            (void)handler1; (void)handler2;

            EXPECT_EQ(2, MyEventGroupBus::GetTotalNumOfEventHandlers());

            EBUS_EVENT(MyEventGroupBus, DestroyMe);

            EXPECT_EQ(0, MyEventGroupBus::GetTotalNumOfEventHandlers());

            // Test removing handlers while in messages - reverse iterator
            handler1 = aznew MyEventHandler(0);
            handler2 = aznew MyEventHandler(1);

            EXPECT_EQ(2, MyEventGroupBus::GetTotalNumOfEventHandlers());

            EBUS_EVENT_REVERSE(MyEventGroupBus, DestroyMe);

            EXPECT_EQ(0, MyEventGroupBus::GetTotalNumOfEventHandlers());
        }
    }

    /**
    * Tests communication buses with multiple event handlers.
    * This is the default configuration for components/data entities models.
    */
    namespace MultBusMultHand
    {
        /**
        * Create event that allows MULTI buses. By default we already allow multiple handlers per bus.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus interface settings
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef unsigned int BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::Handler
        {
        public:
            int actionCalls;
            int sumCalls;

            MyEventHandler(MyEventGroupBus::BusIdType busId)
                : actionCalls(0)
                , sumCalls(0)
            {
                BusConnect(busId); // connect to the specific bus
            }
            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) on busId: %d called\n", x, y, m_busPtr->m_busId);
                ++actionCalls;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y;
                AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) on busId: %d called\n", sum, x, y, m_busPtr->m_busId);
                ++sumCalls;
                return sum;
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }
    TEST_F(EBus, MultBusMultHand)
    {
        using namespace MultBusMultHand;
        {
            MyEventHandler meh0(0);   /// <-- Bind to bus 0
            MyEventHandler meh1(1);  /// <-- Bind to bus 1
            MyEventHandler meh2(1);  /// <-- Bind to bus 1

            // Signal OnAction event on all buses
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(1, meh0.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);
            EXPECT_EQ(1, meh2.actionCalls);

            // Signal OnSum event
            EBUS_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(1, meh0.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);
            EXPECT_EQ(1, meh2.sumCalls);

            // Signal OnAction event on bus 0
            EBUS_EVENT_ID(0, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);
            EXPECT_EQ(1, meh2.actionCalls);

            // Signal OnAction event on bus 1
            EBUS_EVENT_ID(1, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls);
            EXPECT_EQ(2, meh2.actionCalls);

            // When you call event very often... you can cache the the BUS pointer.
            // This is recommended only for performance critical systems, all you save is
            // a hash search.
            MyEventGroupBus::BusPtr busPtr;
            MyEventGroupBus::Bind(busPtr, 1);
            // Now signal this event on the specified bus.
            // \note EBusMyEventGroup has decorative role (we know that from the bus pointer, but it make
            // the code more readable.
            EBUS_EVENT_PTR(busPtr, MyEventGroupBus, OnAction, 2.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(3, meh1.actionCalls);
            EXPECT_EQ(3, meh1.actionCalls);

            // How about we want to store the event result? Let's try with the last result.
            float lastResult = 0.0f;
            EBUS_EVENT_ID_RESULT(lastResult, 0, MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);
            EXPECT_EQ(1, meh2.sumCalls);
            EXPECT_EQ(7.0f, lastResult);

            EBUS_EVENT_ID_RESULT(lastResult, 1, MyEventGroupBus, OnSum, 6.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);
            EXPECT_EQ(2, meh1.sumCalls);
            EXPECT_EQ(2, meh2.sumCalls);
            EXPECT_EQ(11.0f, lastResult);

            // How about if we want to store all the results...
            EBusAggregateResults<float> allResults;
            EBUS_EVENT_RESULT(allResults, MyEventGroupBus, OnSum, 2.0f, 3.0f);
            EXPECT_EQ(3, meh0.sumCalls);
            EXPECT_EQ(3, meh1.sumCalls);
            EXPECT_EQ(3, meh2.sumCalls);
            EXPECT_EQ(3, allResults.values.size());
            EXPECT_EQ(5.0f, allResults.values[0]);
            EXPECT_EQ(5.0f, allResults.values[1]);
            EXPECT_EQ(5.0f, allResults.values[2]);

            allResults.values.clear();

            EBUS_EVENT_ID_RESULT(allResults, 1, MyEventGroupBus, OnSum, 3.0f, 3.0f);
            EXPECT_EQ(3, meh0.sumCalls);
            EXPECT_EQ(4, meh1.sumCalls);
            EXPECT_EQ(4, meh2.sumCalls);
            EXPECT_EQ(2, allResults.values.size());
            EXPECT_EQ(6.0f, allResults.values[0]);
            EXPECT_EQ(6.0f, allResults.values[1]);

            meh2.BusDisconnect(); // we disconnect from receiving events.

            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);  // this signal will NOT trigger any calls.
            EXPECT_EQ(3, meh0.actionCalls);
            EXPECT_EQ(4, meh1.actionCalls);
            EXPECT_EQ(3, meh2.actionCalls);  // not connected
        }
    }

    /**
    * Tests communication with multiple ordered buses with multiple ordered event handlers.
    * This allows us to fully define the order of event broadcasting even across buses.
    * Example case for this in a engine tick/update event, when you want to control will object
    * gets ticked when and within each bus what is the order.
    */
    namespace MultBusMultOrdHnd
    {
        /**
        * Create event that allows multiple ordered buses.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        protected:
            int m_order; ///< Some order value.
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus interface settings
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ByIdAndOrdered;
            typedef int BusIdType;
            typedef AZStd::greater<BusIdType> BusIdOrderCompare;
            //////////////////////////////////////////////////////////////////////////

            /// Required function because we use EBusHandlerPolicy::MultipleAndOrdered
            bool Compare(const MyEventGroup* rhs) const { return m_order < rhs->m_order; }

            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            virtual void DestroyMe()
            {
                delete this;
            }
            //////////////////////////////////////////////////////////////////////////
        };

        // Declare a BUS where we have ordered bus and ordered handlers (this is great example for Update tick... since we want to update in order)...
        // We sort the handlers using "int m_order" in the interface and sort the Bus using AZStd::greater!
        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(MyEventHandler, AZ::SystemAllocator, 0);

            int actionCalls;
            int sumCalls;

            MyEventHandler(MyEventGroupBus::BusIdType busId, int order)
                : actionCalls(0)
                , sumCalls(0)
            {
                m_order = order; ///< Must be set before we connect, so we are inserted in the right place!
                BusConnect(busId); // connect to the specific bus
            }
            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) on busId: %d order %d called\n", x, y, m_busPtr->m_busId, m_order);
                ++actionCalls;
                s_eventOrder[s_eventOrderIndex++] = this;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y;
                AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) on busId: %d order %d called\n", sum, x, y, m_busPtr->m_busId, m_order);
                ++sumCalls;
                s_eventOrder[s_eventOrderIndex++] = this;
                return sum;
            }
            //////////////////////////////////////////////////////////////////////////

            static MyEventHandler* s_eventOrder[3];
            static int  s_eventOrderIndex;
        };

        MyEventHandler* MyEventHandler::s_eventOrder[3];
        int MyEventHandler::s_eventOrderIndex = 0;

    }
    TEST_F(EBus, MultBusMultOrdHnd)
    {
        using namespace MultBusMultOrdHnd;
        {
            MyEventHandler meh0(0, 0);     /// <-- Bind to bus 0 order 0 - since the bus 0 has lower id than 1 (\ref EBusMyEventGroup::BusIdOrderCompare) this will be called after all bus1 entries!
            MyEventHandler meh1(1, 100);  /// <-- Bind to bus 1 order 100 (will be called after meh2 with order 10)
            MyEventHandler meh2(1, 10);  /// <-- Bind to bus 1 order 10 (will be called first on the bus)

            // Signal OnAction event on all buses
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(&meh2, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(&meh0, MyEventHandler::s_eventOrder[2]);
            EXPECT_EQ(1, meh0.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);
            EXPECT_EQ(1, meh2.actionCalls);

            // Signal OnAction event on all buses in reverse order
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            EBUS_EVENT_REVERSE(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(&meh0, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(&meh2, MyEventHandler::s_eventOrder[2]);
            EXPECT_EQ(2, meh0.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls);
            EXPECT_EQ(2, meh2.actionCalls);

            // Signal OnSum event
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            EBUS_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(1, meh0.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);
            EXPECT_EQ(1, meh2.sumCalls);

            // Signal OnAction event on bus 0
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            EBUS_EVENT_ID(0, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(3, meh0.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls);
            EXPECT_EQ(2, meh2.actionCalls);

            // Signal OnAction event on bus 1
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            EBUS_EVENT_ID(1, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(3, meh0.actionCalls);
            EXPECT_EQ(3, meh1.actionCalls);
            EXPECT_EQ(3, meh2.actionCalls);

            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            // When you call event very often... you can cache the the BUS pointer.
            // This is recommended only for performance critical systems, all you save is
            // a hash search.
            MyEventGroupBus::BusPtr busPtr;
            MyEventGroupBus::Bind(busPtr, 1);
            // Now signal this event on the specified bus.
            // \note EBusMyEventGroup has decorative role (we know that from the bus pointer, but it make
            // the code more readable.
            EBUS_EVENT_PTR(busPtr, MyEventGroupBus, OnAction, 2.0f, 2.0f);
            EXPECT_EQ(3, meh0.actionCalls);
            EXPECT_EQ(4, meh1.actionCalls);
            EXPECT_EQ(4, meh1.actionCalls);

            // How about we want to store the event result? Let's try with the last result.
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            float lastResult = 0.0f;
            EBUS_EVENT_ID_RESULT(lastResult, 0, MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);
            EXPECT_EQ(1, meh2.sumCalls);
            EXPECT_EQ(7.0f, lastResult);

            EBUS_EVENT_ID_RESULT(lastResult, 1, MyEventGroupBus, OnSum, 6.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);
            EXPECT_EQ(2, meh1.sumCalls);
            EXPECT_EQ(2, meh2.sumCalls);
            EXPECT_EQ(11.0f, lastResult);

            // How about if we want to store all the results...
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            EBusAggregateResults<float> allResults;
            EBUS_EVENT_RESULT(allResults, MyEventGroupBus, OnSum, 2.0f, 3.0f);
            EXPECT_EQ(3, meh0.sumCalls);
            EXPECT_EQ(3, meh1.sumCalls);
            EXPECT_EQ(3, meh2.sumCalls);
            EXPECT_EQ(3, allResults.values.size());
            EXPECT_EQ(5.0f, allResults.values[0]);
            EXPECT_EQ(5.0f, allResults.values[1]);
            EXPECT_EQ(5.0f, allResults.values[2]);

            allResults.values.clear();
            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker

            EBUS_EVENT_ID_RESULT(allResults, 1, MyEventGroupBus, OnSum, 3.0f, 3.0f);
            EXPECT_EQ(3, meh0.sumCalls);
            EXPECT_EQ(4, meh1.sumCalls);
            EXPECT_EQ(4, meh2.sumCalls);
            EXPECT_EQ(2, allResults.values.size());
            EXPECT_EQ(6.0f, allResults.values[0]);
            EXPECT_EQ(6.0f, allResults.values[1]);

            meh2.BusDisconnect(); // we disconnect from receiving events.

            MyEventHandler::s_eventOrderIndex = 0; // reset the order checker
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);  // this signal will NOT trigger any calls.
            EXPECT_EQ(4, meh0.actionCalls);
            EXPECT_EQ(5, meh1.actionCalls);
            EXPECT_EQ(4, meh2.actionCalls);  // not connected
        }

        {
            // Test removing handlers while in messages - forward iterator
            MyEventHandler* handler1 = aznew MyEventHandler(0, 0);
            MyEventHandler* handler2 = aznew MyEventHandler(1, 10);
            (void)handler1; (void)handler2;

            EXPECT_EQ(2, MyEventGroupBus::GetTotalNumOfEventHandlers());

            EBUS_EVENT(MyEventGroupBus, DestroyMe);

            EXPECT_EQ(0, MyEventGroupBus::GetTotalNumOfEventHandlers());

            // Test removing handlers while in messages - reverse iterator
            handler1 = aznew MyEventHandler(0, 0);
            handler2 = aznew MyEventHandler(1, 10);

            EXPECT_EQ(2, MyEventGroupBus::GetTotalNumOfEventHandlers());

            EBUS_EVENT_REVERSE(MyEventGroupBus, DestroyMe);

            EXPECT_EQ(0, MyEventGroupBus::GetTotalNumOfEventHandlers());
        }
    }

    /**
    * Test for default case (single bus, multiple handlers) using queued (delayed) events.
    */
    namespace SingleBusMultHandQueued
    {
        /**
        * Single bus multiple event groups is the default setting for the event group.
        * Unless we want to modify the allocator, we don't need to change anything.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        protected:
            int m_order;  ///< Some ordering value.
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusInterface settings
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered;
            static const bool EnableEventQueue = true;
            struct BusHandlerOrderCompare
                : public AZStd::binary_function<MyEventGroup*, MyEventGroup*, bool>                           // implement custom Handler/Event ordering function
            {
                AZ_FORCE_INLINE bool operator()(const MyEventGroup* left, const MyEventGroup* right) const
                {
                    return left->m_order < right->m_order;
                }
            };
            //////////////////////////////////////////////////////////////////////////

            /// Required function because we use EBusHandlerPolicy::MultipleAndOrdered
            bool Compare(const MyEventGroup* rhs) const { return m_order < rhs->m_order; }

            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::Handler                 /* we will listen for MyEventGroupBus */
        {
        public:
            int actionCalls;
            int sumCalls;

            MyEventHandler(int order)
                : actionCalls(0)
                , sumCalls(0)
            {
                m_order = order; ///< Must be set before we connect, so we are inserted in the right place!
                // We will bind at construction time to the bus. Disconnect is automatic when the object is
                // destroyed or we can call BusDisconnect()
                MyEventGroupBus::Handler::BusConnect();
            }
            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) order %d called\n", x, y, m_order); ++actionCalls; s_eventOrder[s_eventOrderIndex++] = this;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y; AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) order %d called\n", sum, x, y, m_order); ++sumCalls; s_eventOrder[s_eventOrderIndex++] = this; return sum;
            }
            //////////////////////////////////////////////////////////////////////////

            static MyEventHandler* s_eventOrder[2];
            static int  s_eventOrderIndex;
        };

        MyEventHandler* MyEventHandler::s_eventOrder[2];
        int MyEventHandler::s_eventOrderIndex = 0;

    }
    TEST_F(EBus, SingleBusMultHandQueued)
    {
        using namespace SingleBusMultHandQueued;
        {
            MyEventHandler meh(100);
            MyEventHandler meh1(0);

            // Queue OnAction event
#ifdef AZ_HAS_VARIADIC_TEMPLATES
            MyEventGroupBus::QueueBroadcast(&MyEventGroupBus::Events::OnAction, 1.0f, 2.0f);
#else
            EBUS_QUEUE_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
#endif
            EXPECT_EQ(0, meh.actionCalls);
            EXPECT_EQ(0, meh1.actionCalls);
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            MyEventGroupBus::ExecuteQueuedEvents();
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(1, meh.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);

            // Queue OnAction event in reverse order
            EBUS_QUEUE_EVENT_REVERSE(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(1, meh.actionCalls);
            EXPECT_EQ(1, meh1.actionCalls);
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            MyEventGroupBus::ExecuteQueuedEvents();
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(2, meh.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls);

            // Signal OnSum event
            EBUS_QUEUE_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(0, meh.sumCalls);
            EXPECT_EQ(0, meh1.sumCalls);
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            MyEventGroupBus::ExecuteQueuedEvents();
            EXPECT_EQ(&meh1, MyEventHandler::s_eventOrder[0]);
            EXPECT_EQ(&meh, MyEventHandler::s_eventOrder[1]);
            EXPECT_EQ(1, meh.sumCalls);
            EXPECT_EQ(1, meh1.sumCalls);

            meh.BusDisconnect(); // we disconnect from receiving events.

            EBUS_QUEUE_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(2, meh.actionCalls);
            EXPECT_EQ(2, meh1.actionCalls);
            MyEventHandler::s_eventOrderIndex = 0; // reset the order check index
            MyEventGroupBus::ExecuteQueuedEvents();
            EXPECT_EQ(2, meh.actionCalls);
            EXPECT_EQ(3, meh1.actionCalls); // not disconnected
        }
    }

    /**
    * Tests multi-bus handler (a singe ebus instance that can connect to multiple buses)
    */
    namespace MultBusHandler
    {
        /**
        * Create event that allows MULTI buses. By default we already allow multiple handlers per bus.
        */
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus interface settings
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef int BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MyEventGroup() {}
            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void    OnAction(float x, float y) = 0;
            virtual float   OnSum(float x, float y) = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        /**
        * Now implement our event handler.
        */
        class MyEventHandler
            : public MyEventGroupBus::MultiHandler
        {
        public:
            int actionCalls;
            int sumCalls;

            MyEventHandler(MyEventGroupBus::BusIdType busId0, MyEventGroupBus::BusIdType busId1)
                : actionCalls(0)
                , sumCalls(0)
            {
                BusConnect(busId0); // connect to the specific bus
                BusConnect(busId1); // connect to the specific bus
            }

            //////////////////////////////////////////////////////////////////////////
            // Implement some action on the events...
            virtual void    OnAction(float x, float y)
            {
                AZ_Printf("UnitTest", "OnAction1(%.2f,%.2f) called\n", x, y); ++actionCalls;
            }

            virtual float   OnSum(float x, float y)
            {
                float sum = x + y; AZ_Printf("UnitTest", "%.2f OnAction1(%.2f,%.2f) on called\n", sum, x, y); ++sumCalls; return sum;
            }
            //////////////////////////////////////////////////////////////////////////
        };
    }

    TEST_F(EBus, MultBusHandler)
    {
        using namespace MultBusHandler;
        {
            MyEventHandler meh0(0, 1);     /// <-- Bind to bus 0 and 1

            // Signal OnAction event on all buses
            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(2, meh0.actionCalls);

            // Signal OnSum event
            EBUS_EVENT(MyEventGroupBus, OnSum, 2.0f, 5.0f);
            EXPECT_EQ(2, meh0.sumCalls);

            // Signal OnAction event on bus 0
            EBUS_EVENT_ID(0, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(3, meh0.actionCalls);

            // Signal OnAction event on bus 1
            EBUS_EVENT_ID(1, MyEventGroupBus, OnAction, 1.0f, 2.0f);
            EXPECT_EQ(4, meh0.actionCalls);

            meh0.BusDisconnect(1); // we disconnect from receiving events on bus 1

            EBUS_EVENT(MyEventGroupBus, OnAction, 1.0f, 2.0f);  // this signal will NOT trigger only one call
            EXPECT_EQ(5, meh0.actionCalls);
        }
    }

    /**
     *
     */
    namespace QueueMessageTest
    {
        class QueueTestEventsMultiBus
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZStd::mutex MutexType;

            typedef int            BusIdType;
            static const bool EnableEventQueue = true;
            //////////////////////////////////////////////////////////////////////////
            QueueTestEventsMultiBus()
                : m_callCount(0) {}
            virtual ~QueueTestEventsMultiBus() {}
            virtual void OnMessage() { m_callCount++; }

            int m_callCount;
        };
        typedef AZ::EBus<QueueTestEventsMultiBus> QueueTestMultiBus;

        class QueueTestEventsSingleBus
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            typedef AZStd::mutex MutexType;
            static const bool EnableEventQueue = true;
            //////////////////////////////////////////////////////////////////////////
            QueueTestEventsSingleBus()
                : m_callCount(0) {}
            virtual ~QueueTestEventsSingleBus() {}
            virtual void OnMessage() { m_callCount++; }

            int m_callCount;
        };
        typedef AZ::EBus<QueueTestEventsSingleBus> QueueTestSingleBus;

        JobManager*                    m_jobManager = nullptr;
        JobContext*                    m_jobContext = nullptr;
        QueueTestMultiBus::Handler     m_multiHandler;
        QueueTestSingleBus::Handler    m_singleHandler;
        QueueTestMultiBus::BusPtr      m_multiPtr = nullptr;

        void QueueMessage()
        {
            EBUS_QUEUE_EVENT_ID(0, QueueTestMultiBus, OnMessage);
            EBUS_QUEUE_EVENT(QueueTestSingleBus, OnMessage);
        }

        void QueueMessagePtr()
        {
            EBUS_QUEUE_EVENT_PTR(m_multiPtr, QueueTestMultiBus, OnMessage);
            EBUS_QUEUE_EVENT(QueueTestSingleBus, OnMessage);
        }
    }

    TEST_F(EBus, QueueMessage)
    {
        using namespace QueueMessageTest;

        // Setup
        AllocatorInstance<PoolAllocator>::Create();
        AllocatorInstance<ThreadPoolAllocator>::Create();
        JobManagerDesc jobDesc;
        JobManagerThreadDesc threadDesc;
        jobDesc.m_workerThreads.push_back(threadDesc);
        jobDesc.m_workerThreads.push_back(threadDesc);
        jobDesc.m_workerThreads.push_back(threadDesc);
        m_jobManager = aznew JobManager(jobDesc);
        m_jobContext = aznew JobContext(*m_jobManager);
        JobContext::SetGlobalContext(m_jobContext);

        m_singleHandler.m_callCount = 0;
        m_multiHandler.m_callCount = 0;
        const int NumCalls = 5000;
        QueueTestMultiBus::Bind(m_multiPtr, 0);
        m_multiHandler.BusConnect(0);
        m_singleHandler.BusConnect();
        for (int i = 0; i < NumCalls; ++i)
        {
            Job* job = CreateJobFunction(&QueueMessageTest::QueueMessage, true);
            job->Start();
            job = CreateJobFunction(&QueueMessageTest::QueueMessagePtr, true);
            job->Start();
        }
        while (m_singleHandler.m_callCount < NumCalls * 2 || m_multiHandler.m_callCount < NumCalls * 2)
        {
            QueueTestMultiBus::ExecuteQueuedEvents();
            QueueTestSingleBus::ExecuteQueuedEvents();
            AZStd::this_thread::yield();
        }

        // use queuing generic functions to disconnect from the bus

        // the same as m_singleHandler.BusDisconnect(); but delayed until QueueTestSingleBus::ExecuteQueuedEvents()
#ifdef AZ_HAS_VARIADIC_TEMPLATES
        QueueTestSingleBus::QueueFunction(&QueueTestSingleBus::Handler::BusDisconnect, &m_singleHandler);
#else

        EBUS_QUEUE_FUNCTION(QueueTestSingleBus, &QueueTestSingleBus::Handler::BusDisconnect, &m_singleHandler);
#endif
        // the same as m_multiHandler.BusDisconnect(); but dalayed until QueueTestMultiBus::ExecuteQueuedEvents();
        EBUS_QUEUE_FUNCTION(QueueTestMultiBus, static_cast<void(QueueTestMultiBus::Handler::*)()>(&QueueTestMultiBus::Handler::BusDisconnect), &m_multiHandler);

        EXPECT_EQ(1, QueueTestSingleBus::GetTotalNumOfEventHandlers());
        EXPECT_EQ(1, QueueTestMultiBus::GetTotalNumOfEventHandlers());
        QueueTestSingleBus::ExecuteQueuedEvents();
        QueueTestMultiBus::ExecuteQueuedEvents();
        EXPECT_EQ(0, QueueTestSingleBus::GetTotalNumOfEventHandlers());
        EXPECT_EQ(0, QueueTestMultiBus::GetTotalNumOfEventHandlers());

        // Cleanup
        m_multiPtr = nullptr;
        JobContext::SetGlobalContext(nullptr);
        delete m_jobContext;
        delete m_jobManager;
        AllocatorInstance<ThreadPoolAllocator>::Destroy();
        AllocatorInstance<PoolAllocator>::Destroy();
    }

    class ConnectDisconnectInterface
        : public EBusTraits
    {
    public:
        virtual ~ConnectDisconnectInterface() {}

        virtual void OnConnectChild() = 0;

        virtual void OnDisconnectMe() = 0;

        virtual void OnDisconnectAll() = 0;
    };
    typedef AZ::EBus<ConnectDisconnectInterface> ConnectDisconnectBus;

    class ConnectDisconnectHandler
        : public ConnectDisconnectBus::Handler
    {
        ConnectDisconnectHandler* m_child;
    public:
        ConnectDisconnectHandler(ConnectDisconnectHandler* child)
            : m_child(child)
        {
            s_handlers.push_back(this);

            if (child != nullptr)  // if we are the child don't connect yet
            {
                BusConnect();
            }
        }

        virtual ~ConnectDisconnectHandler()
        {
            s_handlers.erase(AZStd::find(s_handlers.begin(), s_handlers.end(), this));
        }

        virtual void OnConnectChild()
        {
            if (m_child)
            {
                m_child->BusConnect();
            }
        }
        virtual void OnDisconnectMe()
        {
            BusDisconnect();
        }

        virtual void OnDisconnectAll()
        {
            for (size_t i = 0; i < s_handlers.size(); ++i)
            {
                s_handlers[i]->BusDisconnect();
            }
        }

        static AZStd::fixed_vector<ConnectDisconnectHandler*, 5> s_handlers;
    };

    AZStd::fixed_vector<ConnectDisconnectHandler*, 5> ConnectDisconnectHandler::s_handlers;

    class ConnectDisconnectIdOrderedInterface
        : public EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBus interface settings
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef int BusIdType;
        //////////////////////////////////////////////////////////////////////////

        ConnectDisconnectIdOrderedInterface()
            : m_order(0) {}
        virtual ~ConnectDisconnectIdOrderedInterface() {}

        virtual void OnConnectChild() = 0;

        virtual void OnDisconnectMe() = 0;

        virtual void OnDisconnectAll(int busId) = 0;

        virtual bool Compare(const ConnectDisconnectIdOrderedInterface* rhs) const         { return m_order < rhs->m_order; }

        int m_order;
    };

    typedef AZ::EBus<ConnectDisconnectIdOrderedInterface> ConnectDisconnectIdOrderedBus;

    class ConnectDisconnectIdOrderedHandler
        : public ConnectDisconnectIdOrderedBus::Handler
    {
    public:
        ConnectDisconnectIdOrderedHandler(int id, int order, ConnectDisconnectIdOrderedHandler* child)
            : ConnectDisconnectIdOrderedBus::Handler()
            , m_child(child)
            , m_busId(id)
        {
            m_order = order;
            s_handlers.push_back(this);

            if (child != nullptr)  // if we are the child don't connect yet
            {
                BusConnect(m_busId);
            }
        }

        virtual ~ConnectDisconnectIdOrderedHandler()
        {
            s_handlers.erase(AZStd::find(s_handlers.begin(), s_handlers.end(), this));
        }

        virtual void OnConnectChild()
        {
            if (m_child)
            {
                m_child->BusConnect(m_busId);
            }
        }
        virtual void OnDisconnectMe()
        {
            BusDisconnect();
        }

        virtual void OnDisconnectAll(int busId)
        {
            for (size_t i = 0; i < s_handlers.size(); ++i)
            {
                if (busId == -1 || busId == s_handlers[i]->m_busId)
                {
                    s_handlers[i]->BusDisconnect();
                }
            }
        }

        static AZStd::fixed_vector<ConnectDisconnectIdOrderedHandler*, 5> s_handlers;

    protected:
        ConnectDisconnectIdOrderedHandler* m_child;
        int                                 m_busId;
    };

    AZStd::fixed_vector<ConnectDisconnectIdOrderedHandler*, 5> ConnectDisconnectIdOrderedHandler::s_handlers;

    /**
     * Tests a bus when we allow to disconnect while executing messages.
     */
    TEST_F(EBus, DisconnectInDispatch)
    {
        ConnectDisconnectHandler child(nullptr);
        EXPECT_EQ(0, ConnectDisconnectBus::GetTotalNumOfEventHandlers());
        ConnectDisconnectHandler l(&child);
        EXPECT_EQ(1, ConnectDisconnectBus::GetTotalNumOfEventHandlers());
        // Test connect in the during the message call
        EBUS_EVENT(ConnectDisconnectBus, OnConnectChild); // connect the child object

        EXPECT_EQ(2, ConnectDisconnectBus::GetTotalNumOfEventHandlers());
        EBUS_EVENT(ConnectDisconnectBus, OnDisconnectAll); // Disconnect all members during a message
        EXPECT_EQ(0, ConnectDisconnectBus::GetTotalNumOfEventHandlers());


        ConnectDisconnectIdOrderedHandler ch10(10, 1, nullptr);
        ConnectDisconnectIdOrderedHandler ch5(5, 20, nullptr);
        EXPECT_EQ(0, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
        ConnectDisconnectIdOrderedHandler pa10(10, 10, &ch10);
        ConnectDisconnectIdOrderedHandler pa20(20, 20, &ch5);
        EXPECT_EQ(2, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
        EBUS_EVENT(ConnectDisconnectIdOrderedBus, OnConnectChild); // connect the child object
        EXPECT_EQ(4, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());

        // Disconnect all members from bus 10 (it will be sorted first)
        // This we we can test a bus removal while traversing
        EBUS_EVENT(ConnectDisconnectIdOrderedBus, OnDisconnectAll, 10);
        EXPECT_EQ(2, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
        // Now disconnect all buses
        EBUS_EVENT(ConnectDisconnectIdOrderedBus, OnDisconnectAll, -1);
        EXPECT_EQ(0, ConnectDisconnectIdOrderedBus::GetTotalNumOfEventHandlers());
    }

    /**
     * Test multiple handler.
     */
    namespace MultiHandlerTest
    {
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBus Settings
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef unsigned int BusIdType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~MyEventGroup() {}

            //////////////////////////////////////////////////////////////////////////
            // Define the events in this event group!
            virtual void   OnAction() = 0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef AZ::EBus<MyEventGroup> MyEventBus;

        class MultiHandler
            : public MyEventBus::MultiHandler
        {
        public:
            MultiHandler()
                : m_expectedCurrentId(0)
                , m_numCalls(0)
            {}

            void OnAction() override
            {
                const unsigned int* currentIdPtr = MyEventBus::GetCurrentBusId();
                EXPECT_NE(nullptr, currentIdPtr);
                EXPECT_EQ(*currentIdPtr, m_expectedCurrentId);
                ++m_numCalls;
            }

            unsigned int m_expectedCurrentId;
            unsigned int m_numCalls;
        };
    }
    TEST_F(EBus, MultiHandler)
    {
        using namespace MultiHandlerTest;
        MultiHandler ml;
        ml.BusConnect(10);
        ml.BusConnect(12);
        ml.BusConnect(13);

        // test copy handlers and make sure they attached to the same bus
        MultiHandler mlCopy = ml;
        EXPECT_EQ(0, mlCopy.m_numCalls);

        // Called outside of an even it should always return nullptr
        EXPECT_EQ(nullptr, MyEventBus::GetCurrentBusId());

        EBUS_EVENT_ID(1, MyEventBus, OnAction);  // this should not trigger a call
        EXPECT_EQ(0, ml.m_numCalls);

        // Issues calls which we listen for
        ml.m_expectedCurrentId = 10;
        mlCopy.m_expectedCurrentId = 10;
        EBUS_EVENT_ID(10, MyEventBus, OnAction);
        EXPECT_EQ(1, ml.m_numCalls);
        EXPECT_EQ(1, mlCopy.m_numCalls);  // make sure the handler copy is connected
        mlCopy.BusDisconnect();

        ml.m_expectedCurrentId = 12;
        EBUS_EVENT_ID(12, MyEventBus, OnAction);
        EXPECT_EQ(2, ml.m_numCalls);

        ml.m_expectedCurrentId = 13;
        EBUS_EVENT_ID(13, MyEventBus, OnAction);
        EXPECT_EQ(3, ml.m_numCalls);
    }

    // Non intrusive EBusTraits
    struct MyCustomTraits
        : public AZ::EBusTraits
    {
        // ... custom traits here
    };

    /**
     *  Interface that we don't own and we can't inherit traits
     */
    class My3rdPartyInterface
    {
    public:
        virtual void SomeEvent(int a) = 0;
    };

    // 3rd party interface (which is compliant with EBus requirements)
    typedef AZ::EBus<My3rdPartyInterface, MyCustomTraits> My3rdPartyBus1;

    // 3rd party interface that we want to wrap
    class My3rdPartyInterfaceWrapped
        : public My3rdPartyInterface
        , public AZ::EBusTraits
    {
    };

    typedef AZ::EBus<My3rdPartyInterfaceWrapped> My3rdPartyBus2;

    // regular interface trough traits inheritance, please look at the all the samples above

    // combine an ebus and an interface, so you don't need any typedefs. You will need to specialize a template so the bus can get it's traits
    // Keep in mind that this type will not allow for interfaces to be extended, but it's ok for final interfaces
    class MyEBusInterface
        : public AZ::EBus<MyEBusInterface, MyCustomTraits>
    {
    public:
        virtual void Event(int a) const = 0;
    };

    /**
      * Test and demonstrate different EBus implementations
      */
    namespace ImplementationTest
    {
        class Handler1
            : public My3rdPartyBus1::Handler
        {
        public:
            Handler1()
                : m_calls(0)
            {
                My3rdPartyBus1::Handler::BusConnect();
            }

            int m_calls;

        private:
            void SomeEvent(int a) override
            {
                (void)a;
                ++m_calls;
            }
        };

        class Handler2
            : public My3rdPartyBus2::Handler
        {
        public:
            Handler2()
                : m_calls(0)
            {
                My3rdPartyBus2::Handler::BusConnect();
            }

            int m_calls;

        private:
            void SomeEvent(int a) override
            {
                (void)a;
                ++m_calls;
            }
        };

        class Handler3
            : public MyEBusInterface::Handler
        {
        public:
            Handler3()
                : m_calls(0)
            {
                MyEBusInterface::Handler::BusConnect();
            }

            mutable int m_calls;

        private:
            void Event(int a) const override
            {
                (void)a;
                ++m_calls;
            }
        };
    }
    TEST_F(EBus, ExternalInterface)
    {
        using namespace ImplementationTest;
        Handler1 h1;
        Handler2 h2;
        Handler3 h3;

        // test copy of handler
        Handler1 h1Copy = h1;
        EXPECT_EQ(0, h1Copy.m_calls);

        EBUS_EVENT(My3rdPartyBus1, SomeEvent, 1);
        EXPECT_EQ(1, h1.m_calls);
        EXPECT_EQ(1, h1Copy.m_calls);  // check that the copy works too
        EBUS_EVENT(My3rdPartyBus2, SomeEvent, 2);
        EXPECT_EQ(1, h2.m_calls);
        EBUS_EVENT(MyEBusInterface, Event, 3);
        EXPECT_EQ(1, h3.m_calls);
    }

    /**
    *
    */
    TEST_F(EBus, Results)
    {
        // Test the result logical aggregator for OR
        {
            AZ::EBusLogicalResult<bool, AZStd::logical_or<bool> > or_false_false(false);
            or_false_false = false;
            or_false_false = false;
            EXPECT_FALSE(or_false_false.value);
        }

        {
            AZ::EBusLogicalResult<bool, AZStd::logical_or<bool> > or_true_false(false);
            or_true_false = true;
            or_true_false = false;
            EXPECT_TRUE(or_true_false.value);
        }

        // Test the result logical aggregator for AND
        {
            AZ::EBusLogicalResult<bool, AZStd::logical_and<bool> > and_true_false(true);
            and_true_false = true;
            and_true_false = false;
            EXPECT_FALSE(and_true_false.value);
        }

        {
            AZ::EBusLogicalResult<bool, AZStd::logical_and<bool> > and_true_true(true);
            and_true_true = true;
            and_true_true = true;
            EXPECT_TRUE(and_true_true.value);
        }
    }

    // Routers, Bridging and Versioning

    /**
     * EBusInterfaceV1, since we want to keep binary compatibility (we don't need to recompile)
     * when we are implementing the version messaging we should not change the V1 EBus, all code
     * should be triggered from the new version that is not compiled is customer's code yet.
     */
    class EBusInterfaceV1 : public AZ::EBusTraits
    {
    public:
        virtual void OnEvent(int a)
        {
            (void)a;
        }
    };

    using EBusVersion1 = AZ::EBus<EBusInterfaceV1>;

    /**
     * Version 2 of the interface which communicates with Version 1 of the bus bidirectionally.
     */
    class EBusInterfaceV2 : public AZ::EBusTraits
    {
    public:
        /**
         * Router policy implementation that bridges two EBuses by default.
         * It this case we use it to implement versioning between V1 and V2
         * of a specific EBus version.
         */
        template<typename Bus>
        struct RouterPolicy : public EBusRouterPolicy<Bus>
        {
            struct V2toV1Router : public Bus::NestedVersionRouter
            {
                void OnEvent(int a, int b) override
                {
                    if (!m_policy->m_isOnEventRouting)
                    {
                        m_policy->m_isOnEventRouting = true;
                        this->template ForwardEvent<EBusVersion1>(&EBusVersion1::Events::OnEvent, a + b);
                        m_policy->m_isOnEventRouting = false;
                    }
                }

                typename Bus::RouterPolicy* m_policy = nullptr;
            };

            struct V1toV2Router : public EBusVersion1::Router
            {
                void OnEvent(int a) override
                {
                    if(!m_policy->m_isOnEventRouting)
                    {
                        m_policy->m_isOnEventRouting = true;
                        this->template ForwardEvent<Bus>(&Bus::Events::OnEvent, a, 0);
                        m_policy->m_isOnEventRouting = false;
                    }
                }

                typename Bus::RouterPolicy* m_policy = nullptr;
            };

            RouterPolicy()
            {
                m_v2toV1Router.m_policy = this;
                m_v1toV2Router.m_policy = this;
                m_v2toV1Router.BusRouterConnect(this->m_routers);
                m_v1toV2Router.BusRouterConnect();
            }

            ~RouterPolicy()
            {
                m_v2toV1Router.BusRouterDisconnect(this->m_routers);
                m_v1toV2Router.BusRouterDisconnect();
            }

            // State of current routed events to avoid loopbacks
            // this is NOT needed if we route only one way V2->V1 or V1->V2
            bool m_isOnEventRouting = false;

            // Possible optimization, When we are dealing with version we usually don't expect to have active use of the old version,
            // it's just for compatibility. Having routers trying to route to old version busses that rarely
            // have listeners will have it's overhead. To reduct that we can add m_onDemandRouters list that
            // have a pointer to a router and oder, so we can automatically connect that router only when
            // listeners are attached to the old version of the bus. We are talking only about NewVersion->OldVersion
            // bridge (the opposite can be always connected as the overhead will be on the OldVersion bus which we don't expect to use much anyway).
            V2toV1Router m_v2toV1Router;
            V1toV2Router m_v1toV2Router;
        };

        virtual void OnEvent(int a, int b) { (void)a; (void)b; }
    };

    using EBusVersion2 = AZ::EBus<EBusInterfaceV2>;

    namespace RoutingTest
    {
        class DrillerInterceptor : public EBusVersion1::Router
        {
        public:
            void OnEvent(int a) override
            {
                EXPECT_EQ(1020, a);
                m_numOnEvent++;
            }

            int m_numOnEvent = 0;
        };

        class V1EventRouter : public EBusVersion1::Router
        {
        public:
            void OnEvent(int a) override
            {
                (void)a;
                m_numOnEvent++;
                EBusVersion1::SetRouterProcessingState(m_processingState);
            }

            int m_numOnEvent = 0;
            EBusVersion1::RouterProcessingState m_processingState = EBusVersion1::RouterProcessingState::SkipListeners;
        };

        class EBusVersion1Handler : public EBusVersion1::Handler
        {
        public:
            void OnEvent(int a) override
            {
                (void)a;
                m_numOnEvent++;
            }

            int m_numOnEvent = 0;
        };

        class EBusVersion2Handler : public EBusVersion2::Handler
        {
        public:
            void OnEvent(int a, int b) override
            {
                (void)a; (void)b;
                m_numOnEvent++;
            }

            int m_numOnEvent = 0;
        };
    }

    TEST_F(EBus, Routing)
    {
        using namespace RoutingTest;
        DrillerInterceptor driller;
        EBusVersion1Handler v1Handler;

        v1Handler.BusConnect();
        driller.BusRouterConnect();

        EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
        EXPECT_EQ(1, driller.m_numOnEvent);
        EXPECT_EQ(1, v1Handler.m_numOnEvent);

        driller.BusRouterDisconnect();

        EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
        EXPECT_EQ(1, driller.m_numOnEvent);
        EXPECT_EQ(2, v1Handler.m_numOnEvent);

        // routing events
        {
            // reset counter
            v1Handler.m_numOnEvent = 0;

            V1EventRouter v1Router;
            v1Router.BusRouterConnect();

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
            EXPECT_EQ(1, v1Router.m_numOnEvent);
            EXPECT_EQ(0, v1Handler.m_numOnEvent);

            v1Router.BusRouterDisconnect();
        }

        // routing events and skipping further routing
        {
            // reset counter
            v1Handler.m_numOnEvent = 0;

            V1EventRouter v1RouterFirst, v1RouterSecond;
            v1RouterFirst.BusRouterConnect(-1);
            v1RouterSecond.BusRouterConnect();

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
            EXPECT_EQ(1, v1RouterFirst.m_numOnEvent);
            EXPECT_EQ(1, v1RouterSecond.m_numOnEvent);
            EXPECT_EQ(0, v1Handler.m_numOnEvent);

            // now instruct router 1 to block any further event processing
            v1RouterFirst.m_processingState = EBusVersion1::RouterProcessingState::SkipListenersAndRouters;

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 1020);
            EXPECT_EQ(2, v1RouterFirst.m_numOnEvent);
            EXPECT_EQ(1, v1RouterSecond.m_numOnEvent);
            EXPECT_EQ(0, v1Handler.m_numOnEvent);
        }

        // test bridging two EBus by using routers. This can be used to handle different bus versions.
        {
            EBusVersion2Handler v2Handler;
            v2Handler.BusConnect();

            // reset counter
            v1Handler.m_numOnEvent = 0;

            EBusVersion2::Broadcast(&EBusVersion2::Events::OnEvent, 10, 20);
            EXPECT_EQ(1, v1Handler.m_numOnEvent);
            EXPECT_EQ(1, v2Handler.m_numOnEvent);

            EBusVersion1::Broadcast(&EBusVersion1::Events::OnEvent, 30);
            EXPECT_EQ(2, v1Handler.m_numOnEvent);
            EXPECT_EQ(2, v2Handler.m_numOnEvent);
        }

        // We can test Queue and Event routing separately,
        // however they do use the same code path (as we don't queue routing and we just use the ID to differentiate between Broadcast and Event)

    }

    TEST_F(EBus, BusPtrExistsButNoHandlers_HasHandlersFalse)
    {
        MultBusMultHand::MyEventGroupBus::BusPtr busPtr;
        MultBusMultHand::MyEventGroupBus::Bind(busPtr, 1);
        EXPECT_NE(nullptr, busPtr);
        EXPECT_FALSE(MultBusMultHand::MyEventGroupBus::HasHandlers());
    }

    struct LocklessEvents
        : public AZ::EBusTraits
    {
        using MutexType = AZStd::mutex;
        static const bool LocklessDispatch = true;

        virtual ~LocklessEvents() = default;
        virtual void RemoveMe() = 0;
        virtual void DeleteMe() = 0;
        virtual void Calculate(int x, int y, int z) = 0;
    };

    using LocklessBus = AZ::EBus<LocklessEvents>;

    struct LocklessImpl
        : public LocklessBus::Handler
    {
        uint32_t m_val;
        uint32_t m_maxSleep;
        LocklessImpl(uint32_t maxSleep = 0)
            : m_val(0)
            , m_maxSleep(maxSleep)
        {
            BusConnect();
        }

        ~LocklessImpl()
        {
            BusDisconnect();
        }

        void RemoveMe() override
        {
            BusDisconnect();
        }
        void DeleteMe() override
        {
            delete this;
        }
        void Calculate(int x, int y, int z)
        {
            m_val = x + (y * z);
            if (m_maxSleep)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_val % m_maxSleep));
            }
        }
    };

    void ThrashLocklessDispatch(uint32_t maxSleep = 0)
    {
        const size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        AZStd::thread threads[threadCount];
        AZStd::vector<int> results[threadCount];

        LocklessImpl handler(maxSleep);

        auto work = [maxSleep]()
        {
            char sentinel[64] = { 0 };
            char* end = sentinel + AZ_ARRAY_SIZE(sentinel);
            for (int i = 1; i < cycleCount; ++i)
            {
                uint32_t ms = maxSleep ? rand() % maxSleep : 0;
                if (ms % 3)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(ms));
                }
                LocklessBus::Broadcast(&LocklessBus::Events::Calculate, i, i * 2, i << 4);
                bool failed = (AZStd::find_if(&sentinel[0], end, [](char s) { return s != 0; }) != end);
                EXPECT_FALSE(failed);
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }
    }

    TEST_F(EBus, ThrashLocklessDispatchYOLO)
    {
        ThrashLocklessDispatch();
    }

    TEST_F(EBus, ThrashLocklessDispatchSimulateWork)
    {
        ThrashLocklessDispatch(4);
    }

    TEST_F(EBus, DisconnectInLocklessDispatch)
    {
        LocklessImpl handler;
        AZ_TEST_START_ASSERTTEST;
        LocklessBus::Broadcast(&LocklessBus::Events::RemoveMe);
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    TEST_F(EBus, DeleteInLocklessDispatch)
    {
        LocklessImpl* handler = new LocklessImpl();
        AZ_UNUSED(handler);
        AZ_TEST_START_ASSERTTEST;
        LocklessBus::Broadcast(&LocklessBus::Events::DeleteMe);
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    namespace LocklessTest
    {
        struct LocklessConnectorEvents
            : public AZ::EBusTraits
        {
            using MutexType = AZStd::recursive_mutex;
            static const bool LocklessDispatch = true;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef uint32_t BusIdType;

            virtual ~LocklessConnectorEvents() = default;
            virtual void DoConnect() = 0;
            virtual void DoDisconnect() = 0;
        };

        using LocklessConnectorBus = AZ::EBus<LocklessConnectorEvents>;

        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            typedef uint32_t BusIdType;

            virtual void Calculate(int x, int y, int z) = 0;

            virtual ~MyEventGroup() {}
        };

        using MyEventGroupBus = AZ::EBus< MyEventGroup >;

        struct DoubleEbusImpl
            : public LocklessConnectorBus::Handler,
            MyEventGroupBus::Handler
        {
            uint32_t m_id;
            uint32_t m_val;
            uint32_t m_maxSleep;

            DoubleEbusImpl(uint32_t id, uint32_t maxSleep)
                : m_id(id)
                , m_val(0)
                , m_maxSleep(maxSleep)
            {
                LocklessConnectorBus::Handler::BusConnect(m_id);
            }

            ~DoubleEbusImpl()
            {
                MyEventGroupBus::Handler::BusDisconnect();
                LocklessConnectorBus::Handler::BusDisconnect();
            }

            void Calculate(int x, int y, int z) override
            {
                m_val = x + (y * z);
                if (m_maxSleep)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_val % m_maxSleep));
                }
            }
            
            virtual void DoConnect() override
            {
                MyEventGroupBus::Handler::BusConnect(m_id);
            }

            virtual void DoDisconnect() override
            {
                MyEventGroupBus::Handler::BusDisconnect();
            }
        };
    }

    TEST_F(EBus, MixedLocklessTest)
    {
        using namespace LocklessTest;

        const int maxSleep = 5;
        const size_t threadCount = 8;
        enum : size_t { cycleCount = 500 };
        AZStd::thread threads[threadCount];
        AZStd::vector<int> results[threadCount];

        AZStd::vector<DoubleEbusImpl> handlerList;

        for (int i = 0; i < threadCount; i++)
        {
            handlerList.emplace_back(i, maxSleep);
        }

        auto work = [maxSleep, threadCount]()
        {
            char sentinel[64] = { 0 };
            char* end = sentinel + AZ_ARRAY_SIZE(sentinel);
            for (int i = 1; i < cycleCount; ++i)
            {
                uint32_t id = rand() % threadCount;

                LocklessConnectorBus::Event(id, &LocklessConnectorBus::Events::DoConnect);

                uint32_t ms = maxSleep ? rand() % maxSleep : 0;
                if (ms % 3)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(ms));
                }

                MyEventGroupBus::Event(id, &MyEventGroupBus::Events::Calculate, i, i * 2, i << 4);
                
                LocklessConnectorBus::Event(id, &LocklessConnectorBus::Events::DoDisconnect);

                bool failed = (AZStd::find_if(&sentinel[0], end, [](char s) { return s != 0; }) != end);
                EXPECT_FALSE(failed);
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }
    }

    namespace MultithreadConnect
    {
        class MyEventGroup
            : public AZ::EBusTraits
        {
        public:
            using MutexType = AZStd::recursive_mutex;

            virtual ~MyEventGroup() {}
        };

        typedef AZ::EBus< MyEventGroup > MyEventGroupBus;

        struct MyEventGroupImpl :
            MyEventGroupBus::Handler
        {
            MyEventGroupImpl()
            {
                
            }

            ~MyEventGroupImpl()
            {
                MyEventGroupBus::Handler::BusDisconnect();
            }

            virtual void DoConnect()
            {
                MyEventGroupBus::Handler::BusConnect();
            }

            virtual void DoDisconnect()
            {
                MyEventGroupBus::Handler::BusDisconnect();
            }
        };
    }

    TEST_F(EBus, MultithreadConnectTest)
    {
        using namespace MultithreadConnect;

        const int maxSleep = 5;
        const size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        AZStd::thread threads[threadCount];
        AZStd::vector<int> results[threadCount];

        MyEventGroupImpl handler;

        auto work = [maxSleep, &handler]()
        {
            for (int i = 1; i < cycleCount; ++i)
            {
                handler.DoConnect();

                uint32_t ms = maxSleep ? rand() % maxSleep : 0;
                if (ms % 3)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(ms));
                }

                handler.DoDisconnect();
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }
    }

    struct LocklessNullMutexEvents
        : public AZ::EBusTraits
    {
        using MutexType = AZ::NullMutex;
        static const bool LocklessDispatch = true;

        virtual ~LocklessNullMutexEvents() = default;
        virtual void AtomicIncrement() = 0;
    };

    using LocklessNullMutexBus = AZ::EBus<LocklessNullMutexEvents>;

    struct LocklessNullMutexImpl
        : public LocklessNullMutexBus::Handler
    {
        AZStd::atomic<uint64_t> m_val{};
        LocklessNullMutexImpl()
        {
            BusConnect();
        }

        ~LocklessNullMutexImpl()
        {
            BusDisconnect();
        }

        void AtomicIncrement()
        {
            ++m_val;
        }
    };

    void ThrashLocklessDispatchNullMutex()
    {
        constexpr size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        constexpr uint64_t expectedAtomicCount = threadCount * cycleCount;
        AZStd::thread threads[threadCount];

        LocklessNullMutexImpl handler;

        auto work = []()
        {
            for (int i = 0; i < cycleCount; ++i)
            {
                constexpr int maxSleep = 3;
                uint32_t ms = rand() % maxSleep;
                if (ms != 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(ms));
                }
                LocklessNullMutexBus::Broadcast(&LocklessNullMutexBus::Events::AtomicIncrement);
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }

        EXPECT_EQ(expectedAtomicCount, static_cast<uint64_t>(handler.m_val));
    }

    TEST_F(EBus, LocklessDispatchWithNullMutex_Multithread_Thrash)
    {
        ThrashLocklessDispatchNullMutex();
    }
    namespace EBusResultsTest
    {
        class ResultClass
        {
        public:
            int m_value1 = 0;
            int m_value2 = 0;

            bool m_operator_called_const = false;
            bool m_operator_called_rvalue_ref = false;

            ResultClass() = default;
            ResultClass(const ResultClass&) = default;

            bool operator==(const ResultClass& b) const
            {
                return m_value1 == b.m_value1 && m_value2 == b.m_value2;
            }

            ResultClass& operator=(const ResultClass& b)
            {
                m_value1 = b.m_value1 + m_value1;
                m_value2 = b.m_value2 + m_value2;
                m_operator_called_const = true;
                m_operator_called_rvalue_ref = b.m_operator_called_rvalue_ref;
                return *this;
            }
            
            ResultClass& operator=(ResultClass&& b)
            {
                // combine together to prove its not just an assignment
                m_value1 = b.m_value1 + m_value1;
                m_value2 = b.m_value2 + m_value2;

                // but destroy the original value (emulating move op)
                b.m_value1 = 0;
                b.m_value2 = 0;

                m_operator_called_rvalue_ref = true;
                m_operator_called_const = b.m_operator_called_const;
                return *this;
            }
        };

        class ResultReducerClass
        {
        public:
            bool m_operator_called_const = false;
            bool m_operator_called_rvalue_ref = false;

            ResultClass operator()(const ResultClass& a, const ResultClass& b)
            {
                ResultClass newValue;
                newValue.m_value1 = a.m_value1 + b.m_value1;
                newValue.m_value2 = a.m_value2 + b.m_value2;
                m_operator_called_const = true;
                return newValue;
            }

            ResultClass operator()(const ResultClass& a, ResultClass&& b)
            {
                m_operator_called_rvalue_ref = true;
                ResultClass newValue;
                newValue.m_value1 = a.m_value1 + b.m_value1;
                newValue.m_value2 = a.m_value2 + b.m_value2;
                return newValue;
            }
        };

        class MyInterface
        {
        public:
            virtual ResultClass EventX() = 0;
            virtual const ResultClass& EventY() = 0;
        };

        using MyInterfaceBus = AZ::EBus<MyInterface, AZ::EBusTraits>;

        class MyListener : public MyInterfaceBus::Handler
        {
        public:
            MyListener(int value1, int value2)
            {
                m_result.m_value1 = value1;
                m_result.m_value2 = value2;
            }
            
            ~MyListener()
            {
            }

            ResultClass EventX() override
            {
                return m_result;
            }

            const ResultClass& EventY() override
            {
                return m_result;
            }

            ResultClass m_result;
        };

    } // EBusResultsTest

    TEST_F(EBus, ResultsTest)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            ResultClass results;
            MyInterfaceBus::BroadcastResult(results, &MyInterfaceBus::Events::EventX);

            // ensure that the RVALUE-REF op was called:
            EXPECT_FALSE(results.m_operator_called_const);
            EXPECT_TRUE(results.m_operator_called_rvalue_ref);
            EXPECT_EQ(results.m_value1, 4); // 1 + 3
            EXPECT_EQ(results.m_value2, 6); // 2 + 4
            // make sure originals are not destroyed
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            ResultClass results;
            MyInterfaceBus::BroadcastResult(results, &MyInterfaceBus::Events::EventY);

            // ensure that the const version of operator= was called.
            EXPECT_TRUE(results.m_operator_called_const);
            EXPECT_FALSE(results.m_operator_called_rvalue_ref);
            EXPECT_EQ(results.m_value1, 4); // 1 + 3
            EXPECT_EQ(results.m_value2, 6); // 2 + 4
            // make sure originals are not destroyed
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        
        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    // ensure RVALUE-REF move on RHS does not corrupt existing values.
    TEST_F(EBus, ResultsTest_ReducerCorruption)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            EBusReduceResult<ResultClass, ResultReducerClass> resultreducer;
            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventX);
            EXPECT_FALSE(resultreducer.unary.m_operator_called_const);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_rvalue_ref);

            // note that operator= is called TWICE here.  one on (val1+val2)
            // because the ebus results is defined as "value = unary(a, b)"
            // and in this case both operator = as well as the unary operate here.
            // meaning that the addition is actually run multiple times
            // once for (a+b) and then again, during value = unary(...) for a second time

            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            EBusReduceResult<ResultClass, ResultReducerClass> resultreducer;
            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventY);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_const); // we expect the const version to have been called this time
            EXPECT_FALSE(resultreducer.unary.m_operator_called_rvalue_ref);
            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }
        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    // ensure RVALUE-REF move on RHS does not corrupt existing values and operates correctly
    // even if the other form is used (where T is T&)
    TEST_F(EBus, ResultsTest_ReducerCorruption_Ref)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            ResultClass finalResult;
            EBusReduceResult<ResultClass&, ResultReducerClass> resultreducer(finalResult);

            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventX);
            EXPECT_FALSE(resultreducer.unary.m_operator_called_const);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_rvalue_ref);

            EXPECT_FALSE(finalResult.m_operator_called_const);
            EXPECT_TRUE(finalResult.m_operator_called_rvalue_ref);

            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            ResultClass finalResult;
            EBusReduceResult<ResultClass&, ResultReducerClass> resultreducer(finalResult);
            MyInterfaceBus::BroadcastResult(resultreducer, &MyInterfaceBus::Events::EventY);
            EXPECT_TRUE(resultreducer.unary.m_operator_called_const);  // EventY is const, so we expect this to have happened again
            EXPECT_FALSE(resultreducer.unary.m_operator_called_rvalue_ref);
            
            // we still expect the actual finalresult to have been populated via RVALUE REF MOVE
            EXPECT_FALSE(finalResult.m_operator_called_const);
            EXPECT_TRUE(finalResult.m_operator_called_rvalue_ref);

            EXPECT_EQ(resultreducer.value.m_value1, 7);  // (3 + 1) + 3
            EXPECT_EQ(resultreducer.value.m_value2, 10); // (4 + 2) + 4
            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }
        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    // ensure RVALUE-REF move on RHS does not corrupt existing values.
    TEST_F(EBus, ResultsTest_AggregatorCorruption)
    {
        using namespace EBusResultsTest;
        MyListener val1(1, 2);
        MyListener val2(3, 4);

        val1.BusConnect();
        val2.BusConnect();

        {
            EBusAggregateResults<ResultClass> resultarray;
            MyInterfaceBus::BroadcastResult(resultarray, &MyInterfaceBus::Events::EventX);
            EXPECT_EQ(resultarray.values.size(), 2);
            // bus connection is unordered, so we just have to find the two values on it, can't assume they're in same order.
            EXPECT_TRUE(resultarray.values[0] == val1.m_result || resultarray.values[1] == val1.m_result);
            EXPECT_TRUE(resultarray.values[0] == val2.m_result || resultarray.values[1] == val2.m_result);

            if (resultarray.values[0] == val1.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val2.m_result);
            }
            
            if (resultarray.values[0] == val2.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val1.m_result);
            }

            // make sure originals are not destroyed in the move
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }

        {
            EBusAggregateResults<ResultClass> resultarray;
            MyInterfaceBus::BroadcastResult(resultarray, &MyInterfaceBus::Events::EventY);
            // bus connection is unordered, so we just have to find the two values on it, can't assume they're in same order.
            EXPECT_TRUE(resultarray.values[0] == val1.m_result || resultarray.values[1] == val1.m_result);
            EXPECT_TRUE(resultarray.values[0] == val2.m_result || resultarray.values[1] == val2.m_result);

            if (resultarray.values[0] == val1.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val2.m_result);
            }

            if (resultarray.values[0] == val2.m_result)
            {
                EXPECT_EQ(resultarray.values[1], val1.m_result);
            }

            // make sure originals are not destroyed
            EXPECT_EQ(val1.m_result.m_value1, 1);
            EXPECT_EQ(val1.m_result.m_value2, 2);
            EXPECT_EQ(val2.m_result.m_value1, 3);
            EXPECT_EQ(val2.m_result.m_value2, 4);
        }


        val1.BusDisconnect();
        val2.BusDisconnect();
    }

    namespace EBusEnvironmentTest
    {
        class MyInterface
        {
        public:
            virtual void EventX() = 0;
        };

        using MyInterfaceBus = AZ::EBus<MyInterface, AZ::EBusTraits>;

        class MyInterfaceListener : public MyInterfaceBus::Handler
        {
        public:
            MyInterfaceListener(int environmentId = -1)
                : m_environmentId(environmentId)
                , m_numEventsX(0)
            {
            }

            void EventX() override
            {
                ++m_numEventsX;
            }

            int m_environmentId; ///< EBus environment id. -1 is global, otherwise index in the environment array.
            int m_numEventsX;
        };

        class ParallelSeparateEBusEnvironmentProcessor
        {
        public:

            using JobaToProcessArray = AZStd::vector<ParallelSeparateEBusEnvironmentProcessor, AZ::OSStdAllocator>;

            ParallelSeparateEBusEnvironmentProcessor()
            {
                m_busEvironment = AZ::EBusEnvironment::Create();
            }

            ~ParallelSeparateEBusEnvironmentProcessor()
            {
                AZ::EBusEnvironment::Destroy(m_busEvironment);
            }

            void ProcessSomethingInParallel(size_t jobId)
            {
                m_busEvironment->ActivateOnCurrentThread();

                EXPECT_EQ(0, MyInterfaceBus::GetTotalNumOfEventHandlers()); // If environments are properly separated we should have no listeners!"

                MyInterfaceListener uniqueListener((int)jobId);
                uniqueListener.BusConnect();

                const int numEventsToBroadcast = 100;

                for (int i = 0; i < numEventsToBroadcast; ++i)
                {
                    // from now on all EBus calls happen in unique environment
                    MyInterfaceBus::Broadcast(&MyInterfaceBus::Events::EventX);
                }

                uniqueListener.BusDisconnect();

                // Test that we have only X num events
                EXPECT_EQ(uniqueListener.m_numEventsX, numEventsToBroadcast); // If environments are properly separated we should get only the events from our environment!

                m_busEvironment->DeactivateOnCurrentThread();
            }

            static void ProcessJobsRange(JobaToProcessArray* jobs, size_t startIndex, size_t endIndex)
            {
                for (size_t i = startIndex; i <= endIndex; ++i)
                {
                    (*jobs)[i].ProcessSomethingInParallel(i);
                }
            }

            AZ::EBusEnvironment* m_busEvironment;
        };
    } // EBusEnvironmentTest

    TEST_F(EBus, EBusEnvironment)
    {
        using namespace EBusEnvironmentTest;
        ParallelSeparateEBusEnvironmentProcessor::JobaToProcessArray jobsToProcess;
        jobsToProcess.resize(10000);

        MyInterfaceListener globalListener;
        globalListener.BusConnect();

        // broadcast on global bus
        MyInterfaceBus::Broadcast(&MyInterfaceBus::Events::EventX);

        // spawn a few threads to process those jobs
        AZStd::thread thread1(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 0, 1999));
        AZStd::thread thread2(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 2000, 3999));
        AZStd::thread thread3(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 4000, 5999));
        AZStd::thread thread4(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 6000, 7999));
        AZStd::thread thread5(AZStd::bind(&ParallelSeparateEBusEnvironmentProcessor::ProcessJobsRange, &jobsToProcess, 8000, 9999));

        thread5.join();
        thread4.join();
        thread3.join();
        thread2.join();
        thread1.join();

        globalListener.BusDisconnect();

        EXPECT_EQ(1, globalListener.m_numEventsX); // If environments are properly separated we should get only the events the global/default Environment!
    }

    class BusWithConnectionPolicy 
    : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Crc32;
        virtual ~BusWithConnectionPolicy() = default;

        virtual void MessageWhichOccursDuringConnect() = 0;

        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, id);
                handler->MessageWhichOccursDuringConnect();
            }
        };
    };
    using BusWithConnectionPolicyBus = AZ::EBus<BusWithConnectionPolicy>;

    class HandlerWhichDisconnectsDuringDelivery : public BusWithConnectionPolicyBus::Handler
    {
        void MessageWhichOccursDuringConnect() override 
        {
            BusDisconnect();
        }
    };

    TEST_F(EBus, ConnectionPolicy_DisconnectDuringDelivery)
    {
        HandlerWhichDisconnectsDuringDelivery handlerTest;
        handlerTest.BusConnect(AZ_CRC("test"));
    }

    struct SimulatedTickBusInterface
        : public AZ::EBusTraits
    {
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef size_t BusIdType;
        typedef AZStd::mutex EventQueueMutexType;
        static const bool EnableEventQueue = true;

        virtual void OnTick() = 0;
        virtual void OnPayload(size_t payload) = 0;
    };

    using SimulatedTickBus = AZ::EBus<SimulatedTickBusInterface>;

    struct SimulatedTickBusHandler
        : public SimulatedTickBus::Handler
    {
        void OnTick() override 
        {
            // do something non trivial here so that it doesn't optimize out or something...
            AZStd::this_thread::yield();
            m_ticks++;
        }
        void OnPayload(size_t) override 
        {
            m_payloads++;
        }

        AZStd::atomic<size_t> m_ticks = { 0 };
        AZStd::atomic<size_t> m_payloads = { 0 };
    };

    TEST_F(EBus, QueueDuringDispatchOnSingleThreadedBus)
    {
        // make sure that its okay to queue events from any thread, on an EBUS with no mutex protecting its dispatch
        // (only its queue).
        // the general startegy here will be to start a 'main thread' which will be ticking as fast as it can
        // (with some thread yielding) and many child threads which are placing many messages into the queue as they can
        // (with some thread yielding also).
        // to make it extra likely that we find problems we will make sure the main thread is already ticking before we
        // create any child threads, and we will also have all the child threads 'line up' to start with, before they start
        // spamming to maximize the contention.
        const size_t numLoops = 10000;
        const int numThreads = 8;

        SimulatedTickBusHandler handler;
        handler.BusConnect(0);

        AZStd::atomic_int numThreadsWarmedUp = { false };
        AZStd::atomic_bool mainloopStop = { false };
        AZStd::atomic_int numFunctionsQueued = { 0 };
        auto mainLoop = [&mainloopStop, &handler]()
        {
            while (mainloopStop.load() == false)
            {
                SimulatedTickBus::ExecuteQueuedEvents();
                SimulatedTickBus::Event(0, &SimulatedTickBus::Events::OnTick);
            }
            
            // empty the queued events after we terminate just in case there are leftover events.
            SimulatedTickBus::ExecuteQueuedEvents();
        };

        auto incrementFunctionQueue = [&numFunctionsQueued]()
        {
            numFunctionsQueued++;
        };

        auto workerLoop = [numLoops, &numThreadsWarmedUp, numThreads, incrementFunctionQueue]()
        {
            numThreadsWarmedUp++;
            while (numThreadsWarmedUp.load() != numThreads)
            {
                AZStd::this_thread::yield(); // prepare to go!
            }

            size_t loops = 0;
            while (loops++ < numLoops)
            {
                SimulatedTickBus::QueueEvent(0, &SimulatedTickBus::Events::OnPayload, loops);
                SimulatedTickBus::QueueFunction(incrementFunctionQueue);
                AZStd::this_thread::yield();
            }
        };


        AZStd::thread mainThread(mainLoop);
        // wait for the main thread to start ticking:
        while (handler.m_ticks.load() == 0)
        {
            AZStd::this_thread::yield();
        }

        // start the other threads.  They'll actually go all at once:
        AZStd::vector<AZStd::thread> threadsToJoin;
        threadsToJoin.reserve(numThreads);

        // simulate a lot of threads spamming the queue rapidly:
        for (int threadIdx = 0; threadIdx < numThreads; ++threadIdx)
        {
            threadsToJoin.emplace_back(AZStd::thread(workerLoop));
        }

        for (AZStd::thread& thread : threadsToJoin)
        {
            thread.join();
        }

        mainloopStop = true;
        mainThread.join();

        EXPECT_EQ(handler.m_payloads, ((size_t)numThreads) * numLoops);
    }

    struct LastHandlerDisconnectInterface
        : public AZ::EBusTraits
    {
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef size_t BusIdType;

        virtual void OnEvent() = 0;
    };

    using LastHandlerDisconnectBus = AZ::EBus<LastHandlerDisconnectInterface>;

    struct LastHandlerDisconnectHandler
        : public LastHandlerDisconnectBus::Handler
    {
        void OnEvent() override 
        {
            ++m_numOnEvents;
            BusDisconnect();
        }

        unsigned int m_numOnEvents = 0;
    };

    TEST_F(EBus, LastHandlerDisconnectForward)
    {
        LastHandlerDisconnectHandler lastHandler;
        lastHandler.BusConnect(0);
        EBUS_EVENT_ID(0, LastHandlerDisconnectBus, OnEvent);
        EXPECT_FALSE(lastHandler.BusIsConnected());
        EXPECT_EQ(1, lastHandler.m_numOnEvents);
    }

    TEST_F(EBus, LastHandlerDisconnectReverse)
    {
        LastHandlerDisconnectHandler lastHandler;
        lastHandler.BusConnect(0);
        EBUS_EVENT_ID_REVERSE(0, LastHandlerDisconnectBus, OnEvent);
        EXPECT_FALSE(lastHandler.BusIsConnected());
        EXPECT_EQ(1, lastHandler.m_numOnEvents);
    }

}

#if defined(HAVE_BENCHMARK)
//-------------------------------------------------------------------------
// PERF TESTS
//-------------------------------------------------------------------------
namespace Benchmark
{
    // BenchmarkBus implementation details
    namespace BusImplementation
    {
        // Interface for the benchmark bus
        class Interface
        {
        public:
            virtual int OnEvent() = 0;
            virtual void OnWait() = 0;

            bool Compare(const Interface* other) const
            {
                return this < other;
            }
        };

        // Traits for the benchmark bus
        template <AZ::EBusAddressPolicy addressPolicy, AZ::EBusHandlerPolicy handlerPolicy, bool locklessDispatch = false>
        class Traits
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = addressPolicy;
            static const AZ::EBusHandlerPolicy HandlerPolicy = handlerPolicy;
            static const bool LocklessDispatch = locklessDispatch;

            // Allow queuing
            static const bool EnableEventQueue = true;

            // Force locking
            using MutexType = AZStd::recursive_mutex;

            // Only specialize BusIdType if not single address
            using BusIdType = AZStd::conditional_t<AddressPolicy == AZ::EBusAddressPolicy::Single, AZ::NullBusId, int>;
            // Only specialize BusIdOrderCompare if addresses are multiple and ordered
            using BusIdOrderCompare = AZStd::conditional_t<AddressPolicy == EBusAddressPolicy::ByIdAndOrdered, AZStd::less<int>, AZ::NullBusIdCompare>;
        };
    }

    // Definition of the benchmark bus, depending on supplied policies
    template <AZ::EBusAddressPolicy addressPolicy, AZ::EBusHandlerPolicy handlerPolicy, bool locklessDispatch = false>
    using BenchmarkBus = AZ::EBus<BusImplementation::Interface, BusImplementation::Traits<addressPolicy, handlerPolicy, locklessDispatch>>;

    // Predefined benchmark bus instantiations
    // Single
    using OneToOne = BenchmarkBus<AZ::EBusAddressPolicy::Single, AZ::EBusHandlerPolicy::Single>;
    using OneToMany = BenchmarkBus<AZ::EBusAddressPolicy::Single, AZ::EBusHandlerPolicy::Multiple>;
    using OneToManyOrdered = BenchmarkBus<AZ::EBusAddressPolicy::Single, AZ::EBusHandlerPolicy::MultipleAndOrdered>;
    // ById
    using ManyToOne = BenchmarkBus<AZ::EBusAddressPolicy::ById, AZ::EBusHandlerPolicy::Single>;
    using ManyToMany = BenchmarkBus<AZ::EBusAddressPolicy::ById, AZ::EBusHandlerPolicy::Multiple>;
    using ManyToManyOrdered = BenchmarkBus<AZ::EBusAddressPolicy::ById, AZ::EBusHandlerPolicy::MultipleAndOrdered>;
    // ByIdAndOrdered
    using ManyOrderedToOne = BenchmarkBus<AZ::EBusAddressPolicy::ByIdAndOrdered, AZ::EBusHandlerPolicy::Single>;
    using ManyOrderedToMany = BenchmarkBus<AZ::EBusAddressPolicy::ByIdAndOrdered, AZ::EBusHandlerPolicy::Multiple>;
    using ManyOrderedToManyOrdered = BenchmarkBus<AZ::EBusAddressPolicy::ByIdAndOrdered, AZ::EBusHandlerPolicy::MultipleAndOrdered>;

    // Handler for multi-address buses
    template <typename Bus, AZ::EBusAddressPolicy addressPolicy = Bus::Traits::AddressPolicy>
    class Handler
        : public Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Handler, AZ::SystemAllocator, 0);

        Handler(int id)
        {
            Bus::Handler::BusConnect(id);
        }

        ~Handler()
        {
            Bus::Handler::BusDisconnect();
        }

        int OnEvent() override
        {
            return 1;
        }

        void OnWait() override
        {
            AZ_Assert(false, "This shouldn't hit, only the single-address bus should receive OnWait.");
        }
    };

    // Special handler for single address buses
    template <typename Bus>
    class Handler<Bus, AZ::EBusAddressPolicy::Single>
        : public Bus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Handler, AZ::SystemAllocator, 0);

        Handler(int = 0)
        {
            Bus::Handler::BusConnect();
        }

        ~Handler()
        {
            Bus::Handler::BusDisconnect();
        }

        int OnEvent() override
        {
            return 2;
        }

        void OnWait() override
        {
            AZStd::this_thread::yield();
        }
    };

    // Fake fixture we use to connect to buses for us
    template <typename Bus>
    class Connector
    {
    public:
        AZ_CLASS_ALLOCATOR(Connector, AZ::SystemAllocator, 0);

        using HandlerT = Handler<Bus>;

        // Collection of handlers, will be cleared automagically at the end
        AZStd::vector<AZStd::unique_ptr<HandlerT>> m_handlers;

        explicit Connector(::benchmark::State& state)
        {
            int64_t numAddresses = state.range(0);
            int64_t numHandlers = state.range(1);

            // Connect handlers
            for (int64_t  address = 0; address < numAddresses; ++address)
            {
                for (int64_t  handler = 0; handler < numHandlers; ++handler)
                {
                    m_handlers.emplace_back(AZStd::make_unique<HandlerT>(static_cast<int>(address)));
                }
            }
        }
    };

    namespace BenchmarkSettings
    {
        namespace
        {
            // How many addresses/handlers count as "many"
            static const int Many = 1000;
        }

        void Common(::benchmark::internal::Benchmark* benchmark)
        {
            benchmark
                ->Unit(::benchmark::kNanosecond)
                ;
        }

        void OneToOne(::benchmark::internal::Benchmark* benchmark)
        {
            Common(benchmark);
            benchmark
                ->ArgNames({ { "Addresses" },{ "Handlers" } })
                ->Args({ 0, 0 })
                ->Args({ 1, 1 })
                ;
        }

        void OneToMany(::benchmark::internal::Benchmark* benchmark)
        {
            OneToOne(benchmark);
            benchmark
                ->Args({ 1, Many })
                ;
        }

        void ManyToOne(::benchmark::internal::Benchmark* benchmark)
        {
            OneToOne(benchmark);
            benchmark
                ->Args({ Many, 1 })
                ;
        }

        void ManyToMany(::benchmark::internal::Benchmark* benchmark)
        {
            OneToOne(benchmark);
            benchmark
                ->Args({    1, Many })
                ->Args({ Many, 1 })
                ->Args({ Many, Many })
                ;
        }

        // Expected that this will be called after one of the above, so Common not called
        void Multithreaded(::benchmark::internal::Benchmark* benchmark)
        {
            benchmark
                ->ThreadRange(1, 8)
                ->ThreadPerCpu();
                ;
        }
    }

// Internal macro callback for listing all buses requiring ids
#define BUS_BENCHMARK_PRIVATE_LIST_ID(cb, fn)    \
    cb(fn, ManyToOne, ManyToOne)                 \
    cb(fn, ManyToMany, ManyToMany)               \
    cb(fn, ManyToManyOrdered, ManyToMany)        \
    cb(fn, ManyOrderedToOne, ManyToOne)          \
    cb(fn, ManyOrderedToMany, ManyToMany)        \
    cb(fn, ManyOrderedToManyOrdered, ManyToMany)

// Internal macro callback for listing all buses
#define BUS_BENCHMARK_PRIVATE_LIST_ALL(cb, fn)  \
    cb(fn, OneToOne, OneToOne)                  \
    cb(fn, OneToMany, OneToMany)                \
    cb(fn, OneToManyOrdered, OneToMany)         \
    BUS_BENCHMARK_PRIVATE_LIST_ID(cb, fn)

// Internal macro callback for registering a benchmark
#define BUS_BENCHMARK_PRIVATE_REGISTER(fn, BusDef, SettingsFn) BENCHMARK_TEMPLATE(fn, BusDef)->Apply(&BenchmarkSettings::SettingsFn);

// Register a benchmark for all bus permutations requiring ids
#define BUS_BENCHMARK_REGISTER_ID(fn) BUS_BENCHMARK_PRIVATE_LIST_ID(BUS_BENCHMARK_PRIVATE_REGISTER, fn)

// Register a benchmark for all bus permutations
#define BUS_BENCHMARK_REGISTER_ALL(fn) BUS_BENCHMARK_PRIVATE_LIST_ALL(BUS_BENCHMARK_PRIVATE_REGISTER, fn)

    //////////////////////////////////////////////////////////////////////////
    // Single Threaded Events/Broadcasts
    //////////////////////////////////////////////////////////////////////////

    // Baseline benchmark for raw vtable call
    static void BM_EBus_RawCall(::benchmark::State& state)
    {
        AZStd::unique_ptr<Handler<OneToOne>> handler = AZStd::make_unique<Handler<OneToOne>>();

        while (state.KeepRunning())
        {
            handler->OnEvent();
        }
    }
    BENCHMARK(BM_EBus_RawCall)->Apply(&BenchmarkSettings::Common);

    template <typename Bus>
    static void BM_EBus_Broadcast(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        while (state.KeepRunning())
        {
            Bus::Broadcast(&Bus::Events::OnEvent);
        }
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_Broadcast);

    template <typename Bus>
    static void BM_EBus_BroadcastResult(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        while (state.KeepRunning())
        {
            int result = 0;
            Bus::BroadcastResult(result, &Bus::Events::OnEvent);
            ::benchmark::DoNotOptimize(result);
        }
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_BroadcastResult);

    template <typename Bus>
    static void BM_EBus_Event(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        while (state.KeepRunning())
        {
            Bus::Event(1, &Bus::Events::OnEvent);
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_Event);

    template <typename Bus>
    static void BM_EBus_EventResult(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        while (state.KeepRunning())
        {
            int result = 0;
            Bus::EventResult(result, 1, &Bus::Events::OnEvent);
            ::benchmark::DoNotOptimize(result);
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_EventResult);

    template <typename Bus>
    static void BM_EBus_EventCached(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        typename Bus::BusPtr cachedPtr;
        Bus::Bind(cachedPtr, 1);

        while (state.KeepRunning())
        {
            Bus::Event(cachedPtr, &Bus::Events::OnEvent);
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_EventCached);

    template <typename Bus>
    static void BM_EBus_EventCachedResult(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        typename Bus::BusPtr cachedPtr;
        Bus::Bind(cachedPtr, 1);

        while (state.KeepRunning())
        {
            int result = 0;
            Bus::EventResult(result, cachedPtr, &Bus::Events::OnEvent);
            ::benchmark::DoNotOptimize(result);
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_EventCachedResult);

    //////////////////////////////////////////////////////////////////////////
    // Broadcast/Event Queuing
    //////////////////////////////////////////////////////////////////////////

    // Broadcast
    template <typename Bus>
    static void BM_EBus_QueueBroadcast(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            Bus::QueueBroadcast(&Bus::Events::OnEvent);

            // Pause timing, and reset the queue
            state.PauseTiming();
            Bus::ClearQueuedEvents();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_QueueBroadcast);

    template <typename Bus>
    static void BM_EBus_ExecuteBroadcast(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        while (state.KeepRunning())
        {
            // Push an event to the queue to run
            state.PauseTiming();
            Bus::QueueBroadcast(&Bus::Events::OnEvent);
            state.ResumeTiming();

            Bus::ExecuteQueuedEvents();
        }
    }
    BUS_BENCHMARK_REGISTER_ALL(BM_EBus_ExecuteBroadcast);

    // Event
    template <typename Bus>
    static void BM_EBus_QueueEvent(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            Bus::QueueEvent(1, &Bus::Events::OnEvent);

            // Pause timing, and reset the queue
            state.PauseTiming();
            Bus::ClearQueuedEvents();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_QueueEvent);

    template <typename Bus>
    static void BM_EBus_ExecuteEvent(::benchmark::State& state)
    {
        Connector<Bus> f(state);

        while (state.KeepRunning())
        {
            // Push an event to the queue to run
            state.PauseTiming();
            Bus::QueueEvent(1, &Bus::Events::OnEvent);
            state.ResumeTiming();

            Bus::ExecuteQueuedEvents();
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_ExecuteEvent);

    // Event Cached
    template <typename Bus>
    static void BM_EBus_QueueEventCached(::benchmark::State& state)
    {
        typename Bus::BusPtr cachedPtr;
        Bus::Bind(cachedPtr, 1);

        while (state.KeepRunning())
        {
            Bus::QueueEvent(cachedPtr, &Bus::Events::OnEvent);

            // Pause timing, and reset the queue
            state.PauseTiming();
            Bus::ClearQueuedEvents();
            state.ResumeTiming();
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_QueueEventCached);

    template <typename Bus>
    static void BM_EBus_ExecuteQueueCached(::benchmark::State& state)
    {
        Connector<Bus> f(state);
        typename Bus::BusPtr cachedPtr;
        Bus::Bind(cachedPtr, 1);

        while (state.KeepRunning())
        {
            // Push an event to the queue to run
            state.PauseTiming();
            Bus::QueueEvent(cachedPtr, &Bus::Events::OnEvent);
            state.ResumeTiming();

            Bus::ExecuteQueuedEvents();
        }
    }
    BUS_BENCHMARK_REGISTER_ID(BM_EBus_ExecuteQueueCached);

    //////////////////////////////////////////////////////////////////////////
    // Multithreaded Broadcasts
    //////////////////////////////////////////////////////////////////////////

    static void BM_EBus_Multithreaded_Locks(::benchmark::State& state)
    {
        using Bus = BenchmarkBus<AZ::EBusAddressPolicy::Single, AZ::EBusHandlerPolicy::Multiple, false>;

        AZStd::unique_ptr<Connector<Bus>> job;
        if (state.thread_index == 0)
        {
            job = AZStd::make_unique<Connector<Bus>>(state);
        }

        while (state.KeepRunning())
        {
            Bus::Broadcast(&Bus::Events::OnWait);
        };
    }
    BENCHMARK(BM_EBus_Multithreaded_Locks)->Apply(&BenchmarkSettings::OneToMany)->Apply(&BenchmarkSettings::Multithreaded);

    static void BM_EBus_Multithreaded_Lockless(::benchmark::State& state)
    {
        using Bus = BenchmarkBus<AZ::EBusAddressPolicy::Single, AZ::EBusHandlerPolicy::Multiple, true>;

        AZStd::unique_ptr<Connector<Bus>> job;
        if (state.thread_index == 0)
        {
            job = AZStd::make_unique<Connector<Bus>>(state);
        }

        while (state.KeepRunning())
        {
            Bus::Broadcast(&Bus::Events::OnWait);
        };
    }
    BENCHMARK(BM_EBus_Multithreaded_Lockless)->Apply(&BenchmarkSettings::OneToMany)->Apply(&BenchmarkSettings::Multithreaded);
}

#endif // HAVE_BENCHMARK

