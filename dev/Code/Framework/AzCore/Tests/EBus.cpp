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
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>

using namespace AZ;

namespace UnitTest
{
    class EBus
        : public AllocatorsFixture
    {};
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
                BusConnect();
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
                BusConnect();
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

            MyEventGroupBus::BusPtr& GetBusPtr() { return m_busPtr; }

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
        QueueTestSingleBus::BusPtr     m_singlePtr = nullptr;

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

    TEST_F(EBus, DISABLED_QueueMessage)
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

        const int NumCalls = 5000;
        QueueTestMultiBus::Bind(m_multiPtr);
        QueueTestSingleBus::Bind(m_singlePtr);
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
        m_singlePtr = nullptr;
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
        EXPECT_FALSE(MultBusMultHand::MyEventGroupBus::HasHandlers());
    }

    struct LocklessEvents
        : public AZ::EBusTraits
    {
        using MutexType = AZStd::mutex;
        static const bool LocklessDispatch = true;

        virtual ~LocklessEvents() = default;
        virtual void RemoveMe() = 0;
    };

    using LocklessBus = AZ::EBus<LocklessEvents>;

    struct LocklessImpl
        : public LocklessBus::Handler
    {
        LocklessImpl()
        {
            BusConnect();
        }
        void RemoveMe() override
        {
            BusDisconnect();
        }
    };

    TEST_F(EBus, DisconnectInLocklessDispatch)
    {
        LocklessImpl handler;
        AZ_TEST_START_ASSERTTEST;
        LocklessBus::Broadcast(&LocklessBus::Events::RemoveMe);
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    //-------------------------------------------------------------------------
    // PERF TESTS
    //-------------------------------------------------------------------------

    using EBusPerformance = EBus;

    namespace PerformanceTest
    {
        class BlahBlah
        {
        public:
            virtual ~BlahBlah() = default;
            virtual int  OnBlaBla() = 0;
        };

        class BlaEventGroup
            : public AZ::EBusTraits
            , public BlahBlah
        {
        public:
            static const bool EnableEventQueue = true;
        };

        struct ById : public AZ::EBusTraits
        {
            using BusIdType = int;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const bool EnableEventQueue = true;
        };

        class BlaEventGroupById
            : public ById
            , public BlahBlah
        {
        };

        typedef AZ::EBus<BlaEventGroup> BlaEventGroupBus;
        typedef AZ::EBus<BlaEventGroupById> BlaEventGroupBusById;

        class BlaEventHandler
            : public BlaEventGroupBus::Handler
        {
            int x;
        public:
            BlaEventHandler()
            {
                BusConnect();
                x = 0;
            }
            int OnBlaBla() override
            {
                x++;
                return x;
            }
        };

        class BlaEventHandler1
            : public BlaEventGroupBusById::Handler
        {
            int x;
        public:
            BlaEventHandler1()
            {
                BusConnect(1);
                x = 0;
            }
            int OnBlaBla() override
            {
                x++;
                return x;
            }
        };

        const int samples = 100000;
    }

    class Benchmarker
    {
    public:
        Benchmarker(int numSamples, const char* eventName)
            : m_numSamples(numSamples)
            , m_eventName(eventName)
        {
            m_timeStart = AZStd::chrono::system_clock::now();
        }

        // Disallow copying and moving
        Benchmarker(const Benchmarker&) = delete;
        Benchmarker(Benchmarker&&) = delete;
        Benchmarker& operator=(const Benchmarker&) = delete;
        Benchmarker& operator=(Benchmarker&&) = delete;

        ~Benchmarker()
        {
            AZStd::chrono::system_clock::time_point timeEnd = AZStd::chrono::system_clock::now();
            AZStd::chrono::microseconds elapsed = timeEnd - m_timeStart;

            float timePerSample = elapsed.count() / float(m_numSamples);
            printf("Time %s: %lld Ms, avg: %0.5f Ms/call\n", m_eventName, elapsed.count(), timePerSample);
        }

    private:
        const int m_numSamples = 0;
        const char* m_eventName = nullptr;
        AZStd::chrono::microseconds* m_outElapsed = nullptr;
        float* m_outTimePerSample = nullptr;
        AZStd::chrono::system_clock::time_point m_timeStart;
    };

    TEST_F(EBusPerformance, SingleThread)
    {
        using namespace PerformanceTest;
        BlaEventHandler sdr;
        BlaEventHandler1 handler1;
        {
            BlaEventGroup& ptr = sdr;
            Benchmarker bench(samples, "raw call");
            for (int i = 0; i < samples; ++i)
            {
                ptr.OnBlaBla();
            }
        }
        {
            Benchmarker bench(samples, "broadcast call");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBus::Broadcast(&BlaEventGroupBus::Events::OnBlaBla);
            }
        }
        {
            Benchmarker bench(samples, "broadcast result call");
            for (int i = 0; i < samples; ++i)
            {
                int result = 0;
                BlaEventGroupBus::BroadcastResult(result, &BlaEventGroupBus::Events::OnBlaBla);
            }
        }
        {
            Benchmarker bench(samples, "event id call");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBusById::Event(1, &BlaEventGroupBusById::Events::OnBlaBla);
            }
        }
        {
            Benchmarker bench(samples, "event id result call");
            for (int i = 0; i < samples; ++i)
            {
                int result = 0;
                BlaEventGroupBusById::EventResult(result, 1, &BlaEventGroupBusById::Events::OnBlaBla);
            }
        }
        {
            BlaEventGroupBusById::BusPtr cachedPtr;
            BlaEventGroupBusById::Bind(cachedPtr, 1);
            Benchmarker bench(samples, "cached event id call");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBusById::Event(cachedPtr, &BlaEventGroupBusById::Events::OnBlaBla);
            }
        }
        {
            BlaEventGroupBusById::BusPtr cachedPtr;
            BlaEventGroupBusById::Bind(cachedPtr, 1);
            Benchmarker bench(samples, "cached event id result call");
            for (int i = 0; i < samples; ++i)
            {
                int result = 0;
                BlaEventGroupBusById::EventResult(result, cachedPtr, &BlaEventGroupBusById::Events::OnBlaBla);
            }
        }
    }

    TEST_F(EBusPerformance, Queue)
    {
        using namespace PerformanceTest;
        BlaEventHandler sdr;
        BlaEventHandler1 handler1;
        const int samples = 10000; // Must be lower than the others so that the message queues don't exceed the SystemAllocator's capacity.

        {
            Benchmarker bench(samples, "broadcast queue");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBus::QueueBroadcast(&BlaEventGroupBus::Events::OnBlaBla);
            }
        }
        {
            Benchmarker bench(samples, "broadcast execute");
            BlaEventGroupBus::ExecuteQueuedEvents();
        }

        {
            Benchmarker bench(samples, "event id queue");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBusById::QueueEvent(1, &BlaEventGroupBusById::Events::OnBlaBla);
            }
        }
        {
            Benchmarker bench(samples, "event id execute");
            BlaEventGroupBusById::ExecuteQueuedEvents();
        }

        {
            BlaEventGroupBusById::BusPtr cachedPtr;
            BlaEventGroupBusById::Bind(cachedPtr, 1);
            Benchmarker bench(samples, "cached event id queue");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBusById::QueueEvent(cachedPtr, &BlaEventGroupBusById::Events::OnBlaBla);
            }
        }
        {
            Benchmarker bench(samples, "cached event id execute");
            BlaEventGroupBusById::ExecuteQueuedEvents();
        }
    }

    TEST_F(EBusPerformance, QueueExecute)
    {
        using namespace PerformanceTest;
        BlaEventHandler sdr;
        BlaEventHandler1 handler1;

        {
            Benchmarker bench(samples, "broadcast queue/execute");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBus::QueueBroadcast(&BlaEventGroupBus::Events::OnBlaBla);
                BlaEventGroupBus::ExecuteQueuedEvents();
            }
        }
        {
            Benchmarker bench(samples, "event id queue/execute");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBusById::QueueEvent(1, &BlaEventGroupBusById::Events::OnBlaBla);
                BlaEventGroupBusById::ExecuteQueuedEvents();
            }
        }
        {
            BlaEventGroupBusById::BusPtr cachedPtr;
            BlaEventGroupBusById::Bind(cachedPtr, 1);
            Benchmarker bench(samples, "cached event id queue/execute");
            for (int i = 0; i < samples; ++i)
            {
                BlaEventGroupBusById::QueueEvent(cachedPtr, &BlaEventGroupBusById::Events::OnBlaBla);
                BlaEventGroupBusById::ExecuteQueuedEvents();
            }
        }
    }

    namespace MultiThread
    {
        const int waitTimeMS = 100;
        struct FakeJobInterface
        {
            virtual ~FakeJobInterface() = default;
            virtual void DoTheThing() = 0;
        };
        
        struct FakeJobEvents
            : public AZ::EBusTraits
            , public FakeJobInterface
        {
            using MutexType = AZStd::mutex;  
        };

        struct FakeJobEventsLockless
            : public FakeJobEvents
        {
            static const bool LocklessDispatch = true;
        };

        using FakeJobBus = AZ::EBus<FakeJobEvents>;
        using FakeJobBusLockless = AZ::EBus<FakeJobEventsLockless>;

        template <class Bus>
        struct FakeJobImpl
            : public Bus::Handler
        {
            FakeJobImpl()
            {
                Bus::Handler::BusConnect();
            }
            ~FakeJobImpl()
            {
                Bus::Handler::BusDisconnect();
            }
            void DoTheThing() override
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(waitTimeMS));
            }
        };

        using FakeJobImplLocks = FakeJobImpl<FakeJobBus>;
        using FakeJobImplLockless = FakeJobImpl<FakeJobBusLockless>;
    }

    TEST_F(EBusPerformance, MultiThreadedWithLocks)
    {
        using namespace MultiThread;
        const int numSamples = 20;
        const int numThreads = 8;

        FakeJobImplLocks job;

        auto runJobs = [numSamples]()
        {
            for (int i = 0; i < numSamples; ++i)
            {
                FakeJobBus::Broadcast(&FakeJobBus::Events::DoTheThing);
            }
        };

        {
            AZStd::chrono::system_clock::time_point timeStart = AZStd::chrono::system_clock::now();
            runJobs();
            AZStd::chrono::milliseconds elapsed = AZStd::chrono::system_clock::now() - timeStart;
            printf("Time single-threaded broadcast: %lld ms, avg: %0.5f ms/call\n", elapsed.count(), elapsed.count() / float(numSamples));
        }

        {
            AZStd::chrono::system_clock::time_point timeStart = AZStd::chrono::system_clock::now();
            AZStd::thread threads[numThreads];
            for (int t = 0; t < numThreads; ++t)
            {
                threads[t] = AZStd::thread(runJobs);
            }
            for (auto& thread : threads)
            {
                thread.join();
            }
            AZStd::chrono::milliseconds elapsed = AZStd::chrono::system_clock::now() - timeStart;
            AZ::u64 ideal = numSamples * waitTimeMS;
            printf("Time multi-threaded broadcast: actual: %lld ms, ideal: %lld ms\n", elapsed.count(), ideal);
        }
    }

    TEST_F(EBusPerformance, MultiThreadedLocklessDispatch)
    {
        using namespace MultiThread;
        const int numSamples = 20;
        const int numThreads = 8;

        FakeJobImplLockless job;

        auto runJobs = [numSamples]()
        {
            for (int i = 0; i < numSamples; ++i)
            {
                FakeJobBusLockless::Broadcast(&FakeJobBusLockless::Events::DoTheThing);
            }
        };

        {
            AZStd::chrono::system_clock::time_point timeStart = AZStd::chrono::system_clock::now();
            runJobs();
            AZStd::chrono::milliseconds elapsed = AZStd::chrono::system_clock::now() - timeStart;
            printf("Time single-threaded broadcast: %lld ms, avg: %0.5f ms/call\n", elapsed.count(), elapsed.count() / float(numSamples));
        }

        {
            AZStd::chrono::system_clock::time_point timeStart = AZStd::chrono::system_clock::now();
            AZStd::thread threads[numThreads];
            for (int t = 0; t < numThreads; ++t)
            {
                threads[t] = AZStd::thread(runJobs);
            }
            for (auto& thread : threads)
            {
                thread.join();
            }
            AZStd::chrono::milliseconds elapsed = AZStd::chrono::system_clock::now() - timeStart;
            AZ::u64 ideal = numSamples * waitTimeMS;
            printf("Time multi-threaded broadcast: actual: %lld ms, ideal: %lld ms\n", elapsed.count(), ideal);
        }
    }
} // UnitTest
