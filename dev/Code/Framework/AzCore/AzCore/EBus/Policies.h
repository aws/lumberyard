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

/**
 * @file
 * Header file for EBus policies regarding addresses, handlers, connections, and storage.
 * These are internal policies. Do not include this file directly.
 */

// Includes for the event queue.
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/intrusive_set.h>

#include <AzCore/Module/Environment.h>

namespace AZ
{
    /**
     * Defines how many addresses exist on the EBus.
     */
    enum class EBusAddressPolicy
    {
        /**
         * (Default) The EBus has a single address.
         */
        Single,

        /**
         * The EBus has multiple addresses; the order in which addresses
         * receive events is undefined.
         * Events that are addressed to an ID are received by handlers
         * that are connected to that ID.
         * Events that are broadcast without an ID are received by
         * handlers at all addresses.
         */
        ById,

        /**
         * The EBus has multiple addresses; the order in which addresses
         * receive events is defined.
         * Events that are addressed to an ID are received by handlers
         * that are connected to that ID.
         * Events that are broadcast without an ID are received by
         * handlers at all addresses.
         * The order in which addresses receive events is defined by
         * AZ::EBusTraits::BusIdOrderCompare.
         */
        ByIdAndOrdered,
    };

    /**
     * Defines how many handlers can connect to an address on the EBus
     * and the order in which handlers at each address receive events.
     */
    enum class EBusHandlerPolicy
    {
        /**
         * The EBus supports one handler for each address.
         */
        Single,

        /**
         * (Default) Allows any number of handlers for each address;
         * handlers at an address receive events in the order
         * in which the handlers are connected to the EBus.
         */
        Multiple,

        /**
         * Allows any number of handlers for each address; the order
         * in which each address receives an event is defined
         * by AZ::EBusTraits::BusHandlerOrderCompare.
         */
        MultipleAndOrdered,
    };

    namespace Internal
    {
        struct NullBusMessageCall
        {
            template<class Function>
            NullBusMessageCall(Function) {}
            template<class Function, class Allocator>
            NullBusMessageCall(Function, const Allocator&) {}
        };
    } // namespace Internal

    /**
     * Defines the default connection policy that is used when 
     * AZ::EBusTraits::ConnectionPolicy is left unspecified.
     * Use this as a template for custom connection policies.
     * @tparam Bus A class that defines an EBus.
     */
    template <class Bus>
    struct EBusConnectionPolicy
    {
        /**
         * A pointer to a specific address on the EBus.
         */
        typedef typename Bus::BusPtr BusPtr;

        /**
         * The type of ID that is used to address the EBus.
         * This type is used when the address policy is EBusAddressPolicy::ById
         * or EBusAddressPolicy::ByIdAndOrdered only.
         */
        typedef typename Bus::BusIdType BusIdType;

        /**
         * A handler connected to the EBus.
         */
        typedef typename Bus::HandlerNode HandlerNode;

        /**
         * Global data for the EBus.
         * There is only one context for each EBus type.
         */
        typedef typename Bus::Context Context;

        /**
         * Binds a pointer to an EBus address.
         * @param ptr[out] A pointer that will be bound to the appropriate 
         * address on the EBus.
         * @param context Global data for the EBus.
         * @param id The ID of the EBus address that the pointer will be bound to.
         */
        static void Bind(BusPtr& ptr, Context& context, const BusIdType& id = 0);

        /**
         * Connects a handler to an EBus address.
         * @param ptr[out] A pointer that will be bound to the EBus address that 
         * the handler will be connected to.
         * @param context Global data for the EBus.
         * @param handler The handler to connect to the EBus address.
         * @param id The ID of the EBus address that the handler will be connected to.
         */
        static void Connect(BusPtr& ptr, Context& context, HandlerNode& handler, const BusIdType& id = 0);

        /**
         * Disconnects a handler from an EBus address.
         * @param context Global data for the EBus.
         * @param handler The handler to disconnect from the EBus address.
         * @param ptr A pointer to a specific address on the EBus.
         */
        static void Disconnect(Context& context, HandlerNode& handler, BusPtr& ptr);

        /**
         * Disconnects a handler from an EBus address, referencing the address by its ID.
         * @param context Global data for the EBus.
         * @param handler The handler to disconnect from the EBus address.
         * @param id The ID of the EBus address to disconnect the handler from.
         */
        static void DisconnectId(Context& context, HandlerNode& handler, const BusIdType& id);
    };

    /**
     * A choice of AZ::EBusTraits::StoragePolicy that specifies
     * that EBus data is stored in the AZ::Environment.
     * With this policy, a single EBus instance is shared across all
     * modules (DLLs) that attach to the AZ::Environment. 
     * @tparam Context A class that contains EBus data.
     */
    template <class Context>
    struct EBusEnvironmentStoragePolicy
    {
        /**
         * Returns the EBus data.
         * @return A reference to the EBus data.
         */
        static Context& Get()
        {
            if (!s_context)
            {
                // EBus traits should provide a valid unique name, so that handlers can 
                // connect to the EBus across modules.
                // This can fail on some compilers. If it fails, make sure that you give 
                // each bus a unique name.
                s_context = Environment::CreateVariable<Context>(AZ_FUNCTION_SIGNATURE);
            }
            return *s_context;
        }

        /**
         * Environment variable that contains a pointer to the EBus data.
         */
        static EnvironmentVariable<Context> s_context;
    };

    /**
     * Environment variable that contains a pointer to the EBus data.
     */
    template <class Context>
    EnvironmentVariable<Context> EBusEnvironmentStoragePolicy<Context>::s_context;

    /**
     * A choice of AZ::EBusTraits::StoragePolicy that specifies
     * that EBus data is stored in a global static variable.
     * With this policy, each module (DLL) has its own instance of the EBus.
     * @tparam Context A class that contains EBus data.
     */
    template <class Context>
    struct EBusGlobalStoragePolicy
    {
        /**
         * Returns the EBus data.
         * @return A reference to the EBus data.
         */
        static Context& Get()
        {
            static Context s_context;
            return s_context;
        }
    };
    
#if !defined(AZ_PLATFORM_APPLE) // thread_local storage is not supported for Apple platforms.
    /**
     * A choice of AZ::EBusTraits::StoragePolicy that specifies 
     * that EBus data is stored in a thread_local static variable.
     * With this policy, each thread has its own instance of the EBus.
     * @tparam Context A class that contains EBus data.
     */
    template <class Context>
    struct EBusThreadLocalStoragePolicy
    {
        /**
         * Returns the EBus data.
         * @return A reference to the EBus data.
         */
        static Context& Get()
        {
            thread_local static Context s_context;
            return s_context;
        }
    };
#endif


    /// @cond EXCLUDE_DOCS
    template <bool IsEnabled, class Bus, class MutexType>
    struct EBusMessageQueuePolicy
    {
        typedef int BusMessage;
        typedef Internal::NullBusMessageCall BusMessageCall;
        void Execute() {};
        void Clear() {};
    };

    template <class Bus, class MutexType>
    struct EBusMessageQueuePolicy<true, Bus, MutexType>
    {
        typedef AZStd::function<void(typename Bus::InterfaceType*)> BusMessageCall;
        struct BusMessage
        {
            typename Bus::BusIdType m_id;
            typename Bus::BusPtr m_ptr;
            BusMessageCall m_invoke;
            bool      m_isForward : 1; ///< True if the message should be sent forward. False if backwards.
            bool      m_isUseId : 1;   ///< True if the 'id' field should be used.
        };

        typedef AZStd::deque<BusMessage, typename Bus::AllocatorType> DequeType;
        typedef AZStd::queue<BusMessage, DequeType > MessageQueueType;

        MessageQueueType            m_messages;
        MutexType                   m_messagesMutex;        ///< Used to control access to the m_messages. Make sure you never interlock with the EBus mutex. Otherwise, a deadlock can occur.

        void Execute()
        {
            while (!m_messages.empty())
            {
                //////////////////////////////////////////////////////////////////////////
                // Pop element from the queue.
                m_messagesMutex.lock();
                size_t numMessages = m_messages.size();
                if (numMessages == 0)
                {
                    m_messagesMutex.unlock();
                    return;
                }
                BusMessage& srcMsg = m_messages.front();
                BusMessage msg;
                msg.m_isUseId = srcMsg.m_isUseId;
                if (srcMsg.m_isUseId)
                {
                    msg.m_id = srcMsg.m_id;
                }
                else
                {
                    msg.m_ptr = srcMsg.m_ptr;
                }
                msg.m_isForward = srcMsg.m_isForward;
                AZStd::swap(msg.m_invoke, srcMsg.m_invoke); // Don't copy the object.
                m_messages.pop();
                if (numMessages == 1)
                {
                    m_messages.get_container().clear(); // If it was the last message, free all memory.
                }
                m_messagesMutex.unlock();
                //////////////////////////////////////////////////////////////////////////

                auto& context = Bus::GetContext();
                if (msg.m_isUseId)
                {
                    context.m_mutex.lock();
                    typename Bus::BusesContainer::iterator iter = context.m_buses.find(msg.m_id);
                    if (iter != context.m_buses.end())
                    {
                        typename Bus::EBNode * bus = Bus::BusesContainer::toNodePtr(iter);
                        if (msg.m_isForward)
                        {
                            typename Bus::CallstackForwardIterator currentEvent(bus->begin(), &bus->m_busId);
                            typename Bus::EBNode::iterator lastEvent = bus->end();
                            for (; currentEvent.m_iterator != lastEvent; )
                            {
                                typename Bus::InterfaceType * handler = *currentEvent.m_iterator++;
                                msg.m_invoke(handler);
                            }
                        }
                        else
                        {
                            typename Bus::CallstackReverseIterator currentEvent(bus->rbegin(), &bus->m_busId);
                            typename Bus::EBNode::reverse_iterator lastEvent = bus->rend();
                            for (; currentEvent.m_iterator != lastEvent; )
                            {
                                typename Bus::InterfaceType * handler = *currentEvent.m_iterator++;
                                msg.m_invoke(handler);
                            }
                        }
                    }
                    context.m_mutex.unlock();
                }
                else if (msg.m_ptr)
                {
                    if (msg.m_ptr->size())
                    {
                        msg.m_ptr->lock();
                        if (msg.m_isForward)
                        {
                            typename Bus::CallstackForwardIterator currentEvent(msg.m_ptr->begin(), &msg.m_ptr.get()->m_busId);
                            typename Bus::EBNode::iterator lastEvent = msg.m_ptr->end();
                            for (; currentEvent.m_iterator != lastEvent; )
                            {
                                typename Bus::InterfaceType * handler = *currentEvent.m_iterator++;
                                msg.m_invoke(handler);
                            }
                        }
                        else
                        {
                            typename Bus::CallstackReverseIterator currentEvent(msg.m_ptr->rbegin(), &msg.m_ptr.get()->m_busId);
                            typename Bus::EBNode::reverse_iterator lastEvent = msg.m_ptr->rend();
                            for (; currentEvent.m_iterator != lastEvent; )
                            {
                                typename Bus::InterfaceType * handler = *currentEvent.m_iterator++;
                                msg.m_invoke(handler);
                            }
                        }
                        msg.m_ptr->unlock();
                    }
                }
                else
                {
                    if (context.m_buses.size())
                    {
                        context.m_mutex.lock();
                        if (msg.m_isForward)
                        {
                            typename Bus::BusesContainer::iterator firstBus = context.m_buses.begin();
                            typename Bus::BusesContainer::iterator lastBus = context.m_buses.end();
                            for (; firstBus != lastBus; )
                            {
                                typename Bus::EBNode&  bus = *firstBus;
                                typename Bus::CallstackForwardIterator currentEvent(bus.begin(), &bus.m_busId);
                                typename Bus::EBNode::iterator lastEvent = bus.end();
                                bus.add_ref(); // Lock the bus, so remove all handlers. We don't remove them until we get to the next one.
                                for (; currentEvent.m_iterator != lastEvent; )
                                {
                                    typename Bus::InterfaceType * handler = *currentEvent.m_iterator++;
                                    msg.m_invoke(handler);
                                }
                                ++firstBus; // Go to the next bus.
                                bus.release(); // If the bus is empty, we can remove it.
                            }
                        }
                        else
                        {
                            auto firstBus = context.m_buses.rbegin();
                            auto lastBus = context.m_buses.rend();
                            for (; firstBus != lastBus; )
                            {
                                typename Bus::EBNode&  bus = *firstBus;
                                typename Bus::CallstackReverseIterator currentEvent(bus.rbegin(), &bus.m_busId);
                                typename Bus::EBNode::reverse_iterator lastEvent = bus.rend();
                                bus.add_ref(); // Lock the bus, so remove all handlers. We don't remove them until we get to the next one.
                                for (; currentEvent.m_iterator != lastEvent; )
                                {
                                    typename Bus::InterfaceType * handler = *currentEvent.m_iterator++;
                                    msg.m_invoke(handler);
                                }
                                ++firstBus; // Go to the next bus.
                                bus.release(); // If the bus is empty, we can remove it.
                            }
                        }
                        context.m_mutex.unlock();
                    }
                }
            }
        }

        void Clear()
        {
            m_messagesMutex.lock();
            m_messages.get_container().clear();
            m_messagesMutex.unlock();
        }
    };

    template <bool IsEnabled, class Bus, class MutexType>
    struct EBusFunctionQueuePolicy
    {
        typedef Internal::NullBusMessageCall BusMessageCall;
        void Execute() {};
        void Clear() {};
        void SetActive(bool /*isActive*/) {};
        bool IsActive() { return false; }
    };

    template <class Bus, class MutexType>
    struct EBusFunctionQueuePolicy<true, Bus, MutexType>
    {
        typedef AZStd::function<void()> BusMessageCall;

        typedef AZStd::deque<BusMessageCall, typename Bus::AllocatorType> DequeType;
        typedef AZStd::queue<BusMessageCall, DequeType > MessageQueueType;

        EBusFunctionQueuePolicy()
            : m_isActive(true)  {}

        bool                        m_isActive;
        MessageQueueType            m_messages;
        MutexType                   m_messagesMutex;        ///< Used to control access to the m_messages. Make sure you never interlock with the EBus mutex. Otherwise, a deadlock can occur.

        void Execute()
        {
            AZ_Warning("System", m_isActive, "You are calling execute queued functions on a bus, which has not activate it's function queuing! Call YouBus::AllowFunctionQueuing(true)!");
            while (!m_messages.empty())
            {
                BusMessageCall invoke;
                //////////////////////////////////////////////////////////////////////////
                // Pop element from the queue.
                m_messagesMutex.lock();
                size_t numMessages = m_messages.size();
                if (numMessages == 0)
                {
                    m_messagesMutex.unlock();
                    return;
                }
                AZStd::swap(invoke, m_messages.front());
                m_messages.pop();
                if (numMessages == 1)
                {
                    m_messages.get_container().clear(); // If it was the last message, free all memory.
                }
                m_messagesMutex.unlock();
                //////////////////////////////////////////////////////////////////////////

                invoke();
            }
        }

        void Clear()
        {
            m_messagesMutex.lock();
            m_messages.get_container().clear();
            m_messagesMutex.unlock();
        }

        void SetActive(bool isActive)
        {
            m_messagesMutex.lock();
            m_isActive = isActive;
            if (!m_isActive)
            {
                m_messages.get_container().clear();
            }
            m_messagesMutex.unlock();
        };

        bool IsActive()
        {
            return m_isActive;
        }
    };

    /// @endcond

    ////////////////////////////////////////////////////////////
    // Implementations
    ////////////////////////////////////////////////////////////
    template <class Bus>
    void EBusConnectionPolicy<Bus>::Bind(BusPtr& ptr, Context& context, const BusIdType& id)
    {
        auto iter = context.m_buses.insert(id);
        ptr = Bus::BusesContainer::toNodePtr(iter);
    }

    template <class Bus>
    void EBusConnectionPolicy<Bus>::Connect(BusPtr& ptr, Context& context, HandlerNode& handler, const BusIdType& id)
    {
        auto iter = context.m_buses.insert(id);
        ptr = Bus::BusesContainer::toNodePtr(iter);
        ptr->insert(handler);
    }

    template <class Bus>
    void EBusConnectionPolicy<Bus>::Disconnect(Context& context, HandlerNode& handler, BusPtr& ptr)
    {
        (void)context;
        ptr->erase(handler);
    }

    template <class Bus>
    void EBusConnectionPolicy<Bus>::DisconnectId(Context& context, HandlerNode& handler, const BusIdType& id)
    {
        (void)context;
        auto iter = context.m_buses.find(id);
        if (iter != context.m_buses.end())
        {
            BusPtr ptr = Bus::BusesContainer::toNodePtr(iter);
            ptr->erase(handler);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Router Policy
    /// @cond EXCLUDE_DOCS
    template <class Interface>
    struct EBusRouterNode
        : public AZStd::intrusive_multiset_node<EBusRouterNode<Interface>>
    {
        Interface*  m_handler = nullptr;
        int m_order = 0;

        inline EBusRouterNode& operator=(Interface* handler)
        {
            m_handler = handler;
            return *this;
        }

        inline Interface* operator->() const
        {
            return m_handler;
        }

        inline operator Interface*() const
        {
            return m_handler;
        }

        bool operator<(const EBusRouterNode& rhs) const
        {
            return m_order < rhs.m_order;
        }
    };

    template <class Bus>
    struct EBusRouterPolicy
    {
        using RouterNode = EBusRouterNode<typename Bus::InterfaceType>;
        typedef AZStd::intrusive_multiset<RouterNode, AZStd::intrusive_multiset_base_hook<RouterNode>> Container;

        // We have to share this with a general processing event if we want to support stopping messages in progress.
        enum class EventProcessingState : int
        {
            ContinueProcess, ///< Continue with the event process as usual (default).
            SkipListeners, ///< Skip all listeners but notify all other routers.
            SkipListenersAndRouters, ///< Skip everybody. Nobody should receive the event after this.
        };

        template <class CallstackHandler, class Function, class... InputArgs>
        inline bool RouteEvent(const typename Bus::BusIdType* busIdPtr, bool isQueued, bool isReverse, Function func, InputArgs&&... args)
        {
            auto rtLast = m_routers.end();
            CallstackHandler rtCurrent(m_routers.begin(), busIdPtr, isQueued, isReverse);
            while (rtCurrent.m_iterator != rtLast)
            {
                ((*rtCurrent.m_iterator++)->*func)(args...);

                if (rtCurrent.m_processingState == EventProcessingState::SkipListenersAndRouters)
                {
                    return true;
                }
            }

            if (rtCurrent.m_processingState != EventProcessingState::ContinueProcess)
            {
                return true;
            }

            return false;
        }

        Container m_routers;
    };
    /// @endcond
    //////////////////////////////////////////////////////////////////////////
} // namespace AZ
