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
#ifndef AZCORE_SERIALIZE_AZSTD_CONTAINERS_INL
#define AZCORE_SERIALIZE_AZSTD_CONTAINERS_INL

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/tuple.h>

#include <AzCore/IO/GenericStreams.h>

namespace AZStd
{
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class vector;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class list;
    template< class T, class Allocator/* = AZStd::allocator*/ >
    class forward_list;
    template< class T, size_t Capacity >
    class fixed_vector;
    template< class T, size_t N >
    class array;
    template<class Key, class MappedType, class Compare /*= AZStd::less<Key>*/, class Allocator /*= AZStd::allocator*/>
    class map;
    template<class Key, class MappedType, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_map;
    template<class Key, class MappedType, class Hasher /* = AZStd::hash<Key>*/, class EqualKey /* = AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/ >
    class unordered_multimap;
    template <class Key, class Compare, class Allocator>
    class set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_set;
    template<class Key, class Hasher /*= AZStd::hash<Key>*/, class EqualKey /*= AZStd::equal_to<Key>*/, class Allocator /*= AZStd::allocator*/>
    class unordered_multiset;
    template<AZStd::size_t NumBits>
    class bitset;

    template<class T>
    class intrusive_ptr;
    template<class T>
    class shared_ptr;
}

namespace AZ
{
    //template<class T>
    //class ScriptProperty;

    namespace Internal
    {
        class NullFactory
            : public SerializeContext::IObjectFactory
        {
            void* Create(const char* name) override
            {
                (void)name;
                AZ_Assert(false, "Containers and pointers (%s) should be stored by value. They are made to be small and have minimal overhead when empty!", name);
                return nullptr;
            }

            void Destroy(void*) override
            {
            }
        public:
            static NullFactory* GetInstance()
            {
                static NullFactory s_nullFactory;
                return &s_nullFactory;
            }
        };

        template<size_t Index, size_t... Digits>
        struct IndexToCStrHelper : IndexToCStrHelper<Index / 10, Index % 10, Digits...> {};

        template<size_t... Digits>
        struct IndexToCStrHelper<0, Digits...>
        {
            static const char value[];
        };

        template<size_t... Digits>
        const char IndexToCStrHelper<0, Digits...>::value[] = { 'V', 'a', 'l', 'u', 'e', ('0' + Digits)..., '\0' };

        // Converts an integral index into a string with value = "Value" + #Index, where #Index is the stringification of the Index value
        template<size_t Index>
        struct IndexToCStr : IndexToCStrHelper<Index> {};

        template<class T, bool IsStableIterators>
        class AZStdBasicContainer
            : public SerializeContext::IDataContainer
        {
        public:
            typedef typename T::value_type ValueType;
            typedef typename AZStd::remove_pointer<typename T::value_type>::type ValueClass;

            AZStdBasicContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType);
                m_classElement.m_offset = 0xbad0ffe0; // bad offset mark
                m_classElement.m_azRtti = GetRttiHelper<ValueClass>();
                m_classElement.m_flags = AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueClass>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueClass>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array.
            virtual void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);

                typename T::iterator it = arrayPtr->begin();
                typename T::iterator end = arrayPtr->end();
                for (; it != end; ++it)
                {
                    ValueType* valuePtr = &*it;
                    if (!cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement))
                    {
                        break;
                    }
                }
            }

            /// Return number of elements in the container.
            virtual size_t  Size(void* instance) const override
            {
                const T* arrayPtr = reinterpret_cast<const T*>(instance);
                return arrayPtr->size();
            }
            
            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 0;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const override           { return IsStableIterators; }

            /// Returns true if the container is fixed size, otherwise false.
            virtual bool    IsFixedSize() const override                { return false; }

            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const override            { return false; }

            /// Returns true if the container is a smart pointer.
            virtual bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            virtual bool    CanAccessElementsByIndex() const override   { return false; }

            /// Reserve element
            virtual void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* arrayPtr = reinterpret_cast<T*>(instance);
                arrayPtr->push_back();
                return &arrayPtr->back();
            }

            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)instance;
                (void)classElement;
                (void)index;
                return nullptr;
            }

            /// Store element
            virtual void    StoreElement(void* instance, void* element) override
            {
                (void)instance;
                (void)element;
                // do nothing as we have already pushed the element,
                // we can assert and check if the element belongs to the container
            }

            /// Remove element in the container.
            virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);
                for (typename T::iterator it = arrayPtr->begin(); it != arrayPtr->end(); ++it)
                {
                    void* arrayElement = &(*it);
                    if (arrayElement == element)
                    {
                        if (deletePointerDataContext)
                        {
                            DeletePointerData(deletePointerDataContext, &m_classElement, arrayElement);
                        }
                        arrayPtr->erase(it);
                        return true;
                    }
                }
                return false;
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                if (numElements == 0)
                {
                    return 0;
                }

                size_t numRemoved = 0;

                // here is a little tricky if the container doesn't have stable elements, we assume the only case if AZStd::vector.
                if (!IsStableIterators)
                {
                    // if elements are in order we can remove all of them from the container, otherwise we either need to resort locally (not done)
                    // or ask the user to pass elements in order and remove the first N we can (in order)
                    for (size_t i = 1; i < numElements; ++i)
                    {
                        if (elements[i - 1] >= elements[i])
                        {
                            AZ_TracePrintf("Serialization", "RemoveElements for AZStd::vector will perform optimally when the elements (addresses) are sorted in accending order!");
                            numElements = i;
                        }
                    }
                    // traverse the vector in reverse order, then addresses of elements should not change.
                    for (int i = static_cast<int>(numElements); i >= 0; --i)
                    {
                        if (RemoveElement(instance, elements[i], deletePointerDataContext))
                        {
                            ++numRemoved;
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < numElements; ++i)
                    {
                        if (RemoveElement(instance, elements[i], deletePointerDataContext))
                        {
                            ++numRemoved;
                        }
                    }
                }
                return numRemoved;
            }

            /// Clear elements in the instance.
            virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);
                if (deletePointerDataContext)
                {
                    for (typename T::iterator it = arrayPtr->begin(); it != arrayPtr->end(); ++it)
                    {
                        DeletePointerData(deletePointerDataContext, &m_classElement, &(*it));
                    }
                }
                arrayPtr->clear();
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        template<class T, bool IsStableIterators, size_t N>
        class AZStdFixedCapacityContainer
            : public SerializeContext::IDataContainer
        {
        public:
            typedef typename T::value_type ValueType;
            typedef typename AZStd::remove_pointer<typename T::value_type>::type ValueClass;

            AZStdFixedCapacityContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType);
                m_classElement.m_offset = 0xbad0ffe0; // bad offset mark
                m_classElement.m_azRtti = GetRttiHelper<ValueClass>();
                m_classElement.m_flags = AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueClass>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueClass>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array.
            virtual void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);

                typename T::iterator it = arrayPtr->begin();
                typename T::iterator end = arrayPtr->end();
                for (; it != end; ++it)
                {
                    ValueType* valuePtr = &*it;
                    if (!cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement))
                    {
                        break;
                    }
                }
            }

            /// Return number of elements in the container.
            virtual size_t  Size(void* instance) const override
            {
                const T* arrayPtr = reinterpret_cast<const T*>(instance);
                return arrayPtr->size();
            }
            
            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t Capacity(void* instance) const override
            {
                (void)instance;
                return N;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const override         { return IsStableIterators; }

            /// Returns true if the container is fixed size, otherwise false.
            virtual bool    IsFixedSize() const override              { return false; }

            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const override          { return true; }

            /// Returns true if the container is a smart pointer.
            virtual bool    IsSmartPointer() const override           { return false; }

            /// Returns true if elements can be retrieved by index.
            virtual bool    CanAccessElementsByIndex() const override { return false; }

            /// Reserve element
            virtual void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* arrayPtr = reinterpret_cast<T*>(instance);
                if (arrayPtr->size() < N)
                {
                    arrayPtr->push_back();
                    return &arrayPtr->back();
                }
                return nullptr;
            }

            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)instance;
                (void)classElement;
                (void)index;
                return nullptr;
            }

            /// Store element
            virtual void    StoreElement(void* instance, void* element) override
            {
                (void)instance;
                (void)element;
                // do nothing as we have already pushed the element,
                // we can assert and check if the element belongs to the container
            }

            /// Remove element in the container.
            virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);
                for (typename T::iterator it = arrayPtr->begin(); it != arrayPtr->end(); ++it)
                {
                    void* arrayElement = &(*it);
                    if (arrayElement == element)
                    {
                        if (deletePointerDataContext)
                        {
                            DeletePointerData(deletePointerDataContext, &m_classElement, arrayElement);
                        }
                        arrayPtr->erase(it);
                        return true;
                    }
                }
                return false;
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                if (numElements == 0)
                {
                    return 0;
                }

                size_t numRemoved = 0;

                // here is a little tricky if the container doesn't have stable elements, we assume the only case if AZStd::vector.
                if (!IsStableIterators)
                {
                    // if elements are in order we can remove all of them from the container, otherwise we either need to resort locally (not done)
                    // or ask the user to pass elements in order and remove the first N we can (in order)
                    for (size_t i = 1; i < numElements; ++i)
                    {
                        if (elements[i - 1] >= elements[i])
                        {
                            AZ_TracePrintf("Serialization", "RemoveElements for AZStd::vector will perform optimally when the elements (addresses) are sorted in accending order!");
                            numElements = i;
                        }
                    }
                    // traverse the vector in reverse order, then addresses of elements should not change.
                    for (int i = static_cast<int>(numElements); i >= 0; --i)
                    {
                        if (RemoveElement(instance, elements[i], deletePointerDataContext))
                        {
                            ++numRemoved;
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < numElements; ++i)
                    {
                        if (RemoveElement(instance, elements[i], deletePointerDataContext))
                        {
                            ++numRemoved;
                        }
                    }
                }
                return numRemoved;
            }

            /// Clear elements in the instance.
            virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                T* arrayPtr = reinterpret_cast<T*>(instance);
                if (deletePointerDataContext)
                {
                    for (typename T::iterator it = arrayPtr->begin(); it != arrayPtr->end(); ++it)
                    {
                        DeletePointerData(deletePointerDataContext, &m_classElement, &(*it));
                    }
                }
                arrayPtr->clear();
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        class AZStdArrayEvents : public SerializeContext::IEventHandler
        {
        public:
            using Stack = AZStd::stack<size_t, AZStd::vector<size_t, AZ::OSStdAllocator>>;

            void OnWriteBegin(void* classPtr) override
            {
                (void)classPtr;
                if (m_indices)
                {
                    if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
                    {
                        // Pointer is already in use to store an index so convert it to a stack
                        size_t previousIndex = reinterpret_cast<uintptr_t>(m_indices) >> 1;
                        Stack* stack = new Stack();
                        AZ_Assert((reinterpret_cast<uintptr_t>(stack) & 1) == 0, "Expected memory allocation to be at least 2 byte aligned.");
                        stack->push(previousIndex);
                        stack->push(0);
                        m_indices = stack;
                    }
                    else
                    {
                        Stack* stack = reinterpret_cast<Stack*>(m_indices);
                        stack->push(0);
                    }
                }
                else
                {
                    // Use the pointer to just store the one counter instead of allocating memory. Using 1 bit to identify this as a regular
                    // index and not a pointer.
                    m_indices = reinterpret_cast<void*>(1);
                }
            }

            void OnWriteEnd(void* classPtr) override
            {
                (void)classPtr;
                if (m_indices)
                {
                    if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
                    {
                        // There was only one entry so no stack. Clear out the final bit that indicated this was an index and not a pointer.
                        m_indices = nullptr;
                    }
                    else
                    {
                        Stack* stack = reinterpret_cast<Stack*>(m_indices);
                        stack->pop();
                        if (stack->empty())
                        {
                            delete stack;
                            m_indices = nullptr;
                        }
                    }
                }
                else
                {
                    AZ_Warning("Serialization", false, "Mismatch in the number of calls of AZStdArrayEvents::OnWriteEnd compared to AZStdArrayEvents::OnWriteBegin.");
                }

            }

            size_t GetIndex() const
            {
                if (m_indices)
                {
                    if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
                    {
                        // The first bit is used to indicate this is a regular index instead of a pointer so shift down one to get the actual index.
                        return reinterpret_cast<uintptr_t>(m_indices) >> 1;
                    }
                    else
                    {
                        const Stack* stack = reinterpret_cast<const Stack*>(m_indices);
                        return stack->top();
                    }
                }
                else
                {
                    AZ_Warning("Serialization", false, "AZStdArrayEvents is not in a valid state to return an index.");
                    return 0;
                }
            }

            void Increment()
            {
                if (m_indices)
                {
                    if ((reinterpret_cast<uintptr_t>(m_indices) & 1) == 1)
                    {
                        // Increment by 2 because the first bit is used to indicate whether or not a stack is used so the real
                        //      value starts one bit later.
                        size_t index = reinterpret_cast<uintptr_t>(m_indices) + (1 << 1);
                        m_indices = reinterpret_cast<void*>(index);
                    }
                    else
                    {
                        Stack* stack = reinterpret_cast<Stack*>(m_indices);
                        stack->top()++;
                    }
                }
                else
                {
                    AZ_Warning("Serialization", false, "AZStdArrayEvents is not in a valid state to increment.");
                }
            }

            bool IsEmpty() const
            {
                return m_indices == nullptr;
            }

        private:
            // To avoid allocating memory for the stack when there's only one AZStd::array being tracked, m_indices is used to both
            // store an integer value for the index, or when there's nested AZStd::arrays, an AZStd::stack. To tell the two apart
            // the least significant bit is set to 1 if an integer value is stored and 0 if m_indices points to an AZStd::stack.
            // Because the lsb is used for storing the indicator bit, the stored value needs to be shifted down to get the actual index.
            static AZ_THREAD_LOCAL void* m_indices;
        };
        
        template<typename T, size_t N>
        class AZStdArrayContainer
            : public SerializeContext::IDataContainer
        {
        public:
            typedef AZStd::array<T, N> ContainerType;
            typedef T ValueType;
            typedef typename AZStd::remove_pointer<T>::type ValueClass;

            AZStdArrayContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType);
                m_classElement.m_offset = 0xbad0ffe0; // bad offset mark
                m_classElement.m_azRtti = GetRttiHelper<ValueClass>();
                m_classElement.m_flags = AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueClass>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueClass>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array.
            virtual void EnumElements(void* instance, const ElementCB& cb) override
            {
                ContainerType* arrayPtr = reinterpret_cast<ContainerType*>(instance);

                typename ContainerType::iterator it = arrayPtr->begin();
                typename ContainerType::iterator end = arrayPtr->end();
                for (; it != end; ++it)
                {
                    ValueType* valuePtr = &*it;

                    if (!cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement))
                    {
                        break;
                    }
                }
            }

            /// Return number of elements in the container.
            virtual size_t  Size(void* instance) const override
            {
                (void)instance;
                return N;
            }
            
            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t Capacity(void* instance) const override
            {
                (void)instance;
                return N;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const override           { return true;  }

            /// Returns true if the container is fixed size, otherwise false.
            virtual bool    IsFixedSize() const override                { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            virtual bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            virtual bool    CanAccessElementsByIndex() const override   { return true; }

            /// Reserve element
            virtual void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                GenericClassInfo* containerClassInfo = SerializeGenericTypeInfo<ContainerType>::GetGenericInfo();
                const bool eventHandlerAvailable = containerClassInfo && containerClassInfo->GetClassData() && containerClassInfo->GetClassData()->m_eventHandler;
                if (!eventHandlerAvailable)
                {
                    return nullptr;
                }

                AZStdArrayEvents* eventHandler = reinterpret_cast<AZStdArrayEvents*>(containerClassInfo->GetClassData()->m_eventHandler);
                size_t index = eventHandler->GetIndex();
                ContainerType* arrayPtr = reinterpret_cast<ContainerType*>(instance);
                if (index < N)
                {
                    eventHandler->Increment();
                    return &(arrayPtr->at(index));
                }
                else
                {
                    AZ_Warning("Serialization", false, "Unable to reserve an element for AZStd::array because all %i slots are in use.", N);
                }
                return nullptr;
            }

            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                ContainerType* arrayPtr = reinterpret_cast<ContainerType*>(instance);
                if (index < arrayPtr->size())
                {
                    return &(*arrayPtr)[index];
                }
                return nullptr;
            }

            /// Store element
            virtual void    StoreElement(void* instance, void* element) override
            {
                (void)instance;
                (void)element;
                // do nothing as we have already pushed the element,
                // we can assert and check if the element belongs to the container
            }

            /// Remove element in the container.
            virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                (void)instance;
                (void)element;
                (void)deletePointerDataContext;
                return false;
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                (void)instance;
                (void)elements;
                (void)numElements;
                (void)deletePointerDataContext;
                return 0;
            }

            /// Clear elements in the instance.
            virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                (void)instance;
                (void)deletePointerDataContext;
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        template<class T>
        class AZStdAssociativeContainer
            : public SerializeContext::IDataContainer
        {
            typedef typename T::value_type ValueType;
            typedef typename AZStd::remove_pointer<typename T::value_type>::type ValueClass;
        public:
            AZStdAssociativeContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType);
                m_classElement.m_offset = 0xbad0ffe0; // bad offset mark
                m_classElement.m_azRtti = GetRttiHelper<ValueClass>();
                m_classElement.m_flags = AZStd::is_pointer<ValueType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                // Associative containers usually do a hash insert, default value will cause a collision.
                // If we want we can check for multi_set, multi_map, but that may be too much for now.
                m_classElement.m_flags |= SerializeContext::ClassElement::FLG_NO_DEFAULT_VALUE;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueClass>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueClass>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            virtual void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);

                typename T::iterator it = containerPtr->begin();
                typename T::iterator end = containerPtr->end();
                for (; it != end; ++it)
                {
                    ValueType* valuePtr = &*it;
                    if (!cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement))
                    {
                        break;
                    }
                }
            }

            /// Return number of elements in the container.
            virtual size_t  Size(void* instance) const override
            {
                const T* arrayPtr = reinterpret_cast<const T*>(instance);
                return arrayPtr->size();
            }
            
            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 0;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const override           { return true;  }

            /// Returns true if the container is fixed size, otherwise false.
            virtual bool    IsFixedSize() const override                { return false; }

            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const override            { return false; }

            /// Returns true if the container is a smart pointer.
            virtual bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            virtual bool    CanAccessElementsByIndex() const override   { return false; }

            /// Reserve element
            virtual void*   ReserveElement(void* instance, const  SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* containerPtr = reinterpret_cast<T*>(instance);
                return new(containerPtr->get_allocator().allocate(sizeof(ValueType), AZStd::alignment_of<ValueType>::value))ValueType;
            }

            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)instance;
                (void)classElement;
                (void)index;
                return nullptr;
            }

            /// Store element
            virtual void    StoreElement(void* instance, void* element) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);
                ValueType* valuePtr = reinterpret_cast<ValueType*>(element);
                containerPtr->insert(AZStd::move(*valuePtr));
                valuePtr->~ValueType();
                containerPtr->get_allocator().deallocate(valuePtr, sizeof(ValueType), AZStd::alignment_of<ValueType>::value);
            }

            /// Remove element in the container.
            virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);
                // this container can be a multi container so key is NOT enough, but a good start
                const auto& key = T::traits_type::key_from_value(*reinterpret_cast<const ValueType*>(element));
                typename T::iterator it = containerPtr->find(key);
                while (it != containerPtr->end()) // in a case of multi key support iterate over all elements with that key until we find the one
                {
                    void* containerElement = &(*it);
                    if (containerElement == element)
                    {
                        if (deletePointerDataContext)
                        {
                            DeletePointerData(deletePointerDataContext, &m_classElement, containerElement);
                        }
                        containerPtr->erase(it);
                        return true;
                    }
                    ++it;
                }
                return false;
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                size_t numRemoved = 0;
                for (size_t i = 0; i < numElements; ++i)
                {
                    if (RemoveElement(instance, elements[i], deletePointerDataContext))
                    {
                        ++numRemoved;
                    }
                }
                return numRemoved;
            }

            /// Clear elements in the instance.
            virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                T* containerPtr = reinterpret_cast<T*>(instance);
                if (deletePointerDataContext)
                {
                    for (typename T::iterator it = containerPtr->begin(); it != containerPtr->end(); ++it)
                    {
                        DeletePointerData(deletePointerDataContext, &m_classElement, &(*it));
                    }
                }
                containerPtr->clear();
            }

            void ElementsUpdated(void* instance) override
            {
                // reinsert all elements, technically for hash containers we can just call rehash
                // which might be faster, but we will not be able to cover other containers;
                T* containerPtr = reinterpret_cast<T*>(instance);
                T tempContainer(AZStd::make_move_iterator(containerPtr->begin()), AZStd::make_move_iterator(containerPtr->end()));
                containerPtr->swap(tempContainer);
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };


        template<class T1, class T2>
        class AZStdPairContainer
            : public SerializeContext::IDataContainer
        {
            typedef typename AZStd::remove_pointer<T1>::type Value1Class;
            typedef typename AZStd::remove_pointer<T2>::type Value2Class;
            typedef typename AZStd::pair<T1, T2> PairType;
        public:
            AZStdPairContainer()
            {
                // FIXME: We should properly fill in all the other fields as well.
                m_value1ClassElement.m_name = "value1";
                m_value1ClassElement.m_nameCrc = AZ_CRC("value1", 0xa2756c5a);
                m_value1ClassElement.m_offset = 0;
                m_value1ClassElement.m_dataSize = sizeof(T1);
                m_value1ClassElement.m_azRtti = GetRttiHelper<Value1Class>();
                m_value1ClassElement.m_flags = AZStd::is_pointer<T1>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                m_value1ClassElement.m_genericClassInfo = SerializeGenericTypeInfo<Value1Class>::GetGenericInfo();
                m_value1ClassElement.m_typeId = SerializeGenericTypeInfo<Value1Class>::GetClassTypeId();
                m_value1ClassElement.m_editData = nullptr;

                m_value2ClassElement.m_name = "value2";
                m_value2ClassElement.m_nameCrc = AZ_CRC("value2", 0x3b7c3de0);
                m_value2ClassElement.m_offset = sizeof(T1);
                m_value2ClassElement.m_dataSize = sizeof(T2);
                m_value2ClassElement.m_azRtti = GetRttiHelper<Value2Class>();
                m_value2ClassElement.m_flags = AZStd::is_pointer<T2>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                m_value2ClassElement.m_genericClassInfo = SerializeGenericTypeInfo<Value2Class>::GetGenericInfo();
                m_value2ClassElement.m_typeId = SerializeGenericTypeInfo<Value2Class>::GetClassTypeId();
                m_value2ClassElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_value1ClassElement.m_nameCrc)
                {
                    return &m_value1ClassElement;
                }
                if (elementNameCrc == m_value2ClassElement.m_nameCrc)
                {
                    return &m_value2ClassElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_value1ClassElement.m_nameCrc)
                {
                    classElement = m_value1ClassElement;
                    return true;
                }
                else if (dataElement.m_nameCrc == m_value2ClassElement.m_nameCrc)
                {
                    classElement = m_value2ClassElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            virtual void EnumElements(void* instance, const ElementCB& cb) override
            {
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);

                T1* value1Ptr = &pairPtr->first;
                T2* value2Ptr = &pairPtr->second;

                if (cb(value1Ptr, m_value1ClassElement.m_typeId, m_value1ClassElement.m_genericClassInfo ? m_value1ClassElement.m_genericClassInfo->GetClassData() : nullptr, &m_value1ClassElement))
                {
                    cb(value2Ptr, m_value2ClassElement.m_typeId, m_value2ClassElement.m_genericClassInfo ? m_value2ClassElement.m_genericClassInfo->GetClassData() : nullptr, &m_value2ClassElement);
                }
            }

            /// Return number of elements in the container.
            virtual size_t  Size(void* instance) const override
            {
                (void)instance;
                return 2;
            }
            
            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 2;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const override           { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            virtual bool    IsFixedSize() const override                { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            virtual bool    IsSmartPointer() const override             { return false; }

            /// Returns true if elements can be retrieved by index.
            virtual bool    CanAccessElementsByIndex() const override   { return true; }

            /// Reserve element
            virtual void*   ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                if (classElement->m_nameCrc == m_value1ClassElement.m_nameCrc)
                {
                    return &pairPtr->first;
                }
                if (classElement->m_nameCrc == m_value2ClassElement.m_nameCrc)
                {
                    return &pairPtr->second;
                }
                return nullptr; // you can't add any new elements no this container.
            }

            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                if (index == 0)
                {
                    return &pairPtr->first;
                }
                if (index == 1)
                {
                    return &pairPtr->second;
                }
                return nullptr; // you can't add any new elements no this container.
            }

            /// Store element
            virtual void    StoreElement(void* instance, void* element) override         { (void)instance; (void)element; }

            /// Remove element in the container.
            virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                if (&pairPtr->first == element && deletePointerDataContext)
                {
                    DeletePointerData(deletePointerDataContext, &m_value1ClassElement, &pairPtr->first);
                    pairPtr->first = typename PairType::first_type();
                }
                if (&pairPtr->second == element && deletePointerDataContext)
                {
                    DeletePointerData(deletePointerDataContext, &m_value2ClassElement, &pairPtr->second);
                    pairPtr->second = typename PairType::second_type();
                }
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    for (size_t i = 0; i < numElements; ++i)
                    {
                        RemoveElement(instance, elements[i], deletePointerDataContext);
                    }
                }
                return 0; // you can't remove elements from this container.
            }

            /// Clear elements in the instance.
            virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    PairType* pairPtr = reinterpret_cast<PairType*>(instance);
                    DeletePointerData(deletePointerDataContext, &m_value1ClassElement, &pairPtr->first);
                    DeletePointerData(deletePointerDataContext, &m_value2ClassElement, &pairPtr->second);
                    *pairPtr = PairType();
                }
            }

            SerializeContext::ClassElement m_value1ClassElement;
            SerializeContext::ClassElement m_value2ClassElement;
        };

        template<typename TupleType, size_t Index>
        struct CreateClassElementHelper
        {
            // First class element: Initializes a class element that starts at index 0
            static void Create(AZStd::array<SerializeContext::ClassElement, AZStd::tuple_size<TupleType>::value>& classElements)
            {
                using ElementType = AZStd::tuple_element_t<Index, TupleType>;
                classElements[Index].m_name = Internal::IndexToCStr<Index + 1>::value;
                classElements[Index].m_nameCrc = Crc32(classElements[Index].m_name);
                // Use the previous class element data to calculate the next class element offset
                classElements[Index].m_offset = classElements[Index - 1].m_offset + classElements[Index - 1].m_dataSize;
                classElements[Index].m_dataSize = sizeof(ElementType);
                classElements[Index].m_azRtti = GetRttiHelper<AZStd::remove_pointer_t<ElementType>>();
                classElements[Index].m_flags = AZStd::is_pointer<ElementType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                classElements[Index].m_genericClassInfo = SerializeGenericTypeInfo<AZStd::remove_pointer_t<ElementType>>::GetGenericInfo();
                classElements[Index].m_typeId = SerializeGenericTypeInfo<AZStd::remove_pointer_t<ElementType>>::GetClassTypeId();
                classElements[Index].m_editData = nullptr;
            }
        };

        template<typename TupleType>
        struct CreateClassElementHelper<TupleType, 0>
        {
            static void Create(AZStd::array<SerializeContext::ClassElement, AZStd::tuple_size<TupleType>::value>& classElements)
            {
                using ElementType = AZStd::tuple_element_t<0, TupleType>;
                classElements[0].m_name = "Value1";
                classElements[0].m_nameCrc = Crc32("Value1");
                classElements[0].m_offset = 0;
                classElements[0].m_dataSize = sizeof(ElementType);
                classElements[0].m_azRtti = GetRttiHelper<AZStd::remove_pointer_t<ElementType>>();
                classElements[0].m_flags = AZStd::is_pointer<ElementType>::value ? SerializeContext::ClassElement::FLG_POINTER : 0;
                classElements[0].m_genericClassInfo = SerializeGenericTypeInfo<AZStd::remove_pointer_t<ElementType>>::GetGenericInfo();
                classElements[0].m_typeId = SerializeGenericTypeInfo<AZStd::remove_pointer_t<ElementType>>::GetClassTypeId();
                classElements[0].m_editData = nullptr;
            }
        };

        template<typename... Types>
        class AZStdTupleContainer
            : public SerializeContext::IDataContainer
        {
            using TupleType = AZStd::tuple<Types...>;
        public:
            static const size_t s_tupleSize = AZStd::tuple_size<TupleType>::value;

            AZStdTupleContainer()
            {
                CreateClassElementArray(AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            const SerializeContext::ClassElement* GetElement(u32 elementNameCrc) const override
            {
                return GetElementAddressTuple(elementNameCrc, AZStd::make_index_sequence<s_tupleSize>{});
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                return GetElementTuple(classElement, dataElement.m_nameCrc, AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Enumerate elements in the array
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                EnumElementsTuple(*tuplePtr, cb, AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Return number of elements in the container.
            size_t  Size(void*) const override
            {
                return s_tupleSize;
            }

            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            size_t Capacity(void*) const override
            {
                return s_tupleSize;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            bool IsStableElements() const override { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            bool IsFixedSize() const override { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            bool IsFixedCapacity() const override { return true; }

            /// Returns true if the container is a smart pointer.
            bool IsSmartPointer() const override { return false; }

            /// Returns true if elements can be retrieved by index.
             bool CanAccessElementsByIndex() const override { return true; }

            /// Reserve element
             void* ReserveElement(void* instance, const SerializeContext::ClassElement* classElement) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                return ReserveElementTuple(*tuplePtr, classElement, AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Get an element's address by its index (called before the element is loaded).
            void* GetElementByIndex(void* instance, const SerializeContext::ClassElement*, size_t index) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                return GetElementByIndexTuple(*tuplePtr, index, AZStd::make_index_sequence<s_tupleSize>{});
            }

            /// Store element
            void StoreElement(void* instance, void* element) override { (void)instance; (void)element; }

            /// Remove element in the container.
            bool RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                RemoveElementTuple(*tuplePtr, element, deletePointerDataContext, AZStd::make_index_sequence<s_tupleSize>{});
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            size_t RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    for (size_t i = 0; i < numElements; ++i)
                    {
                        RemoveElement(instance, elements[i], deletePointerDataContext);
                    }
                }
                return 0; // you can't remove elements from this container.
            }

            /// Clear elements in the instance.
            void ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                if (deletePointerDataContext)
                {
                    auto tuplePtr = reinterpret_cast<TupleType*>(instance);
                    DeletePointerDataTuple(*tuplePtr, deletePointerDataContext, AZStd::make_index_sequence<s_tupleSize>{});
                    *tuplePtr = TupleType();
                }
            }

            AZStd::array<SerializeContext::ClassElement, s_tupleSize> m_valueClassElements;

            private:

                template<size_t... Indices>
                void CreateClassElementArray(AZStd::index_sequence<Indices...>)
                {
                    int dummy[]{ 0, (CreateClassElementHelper<TupleType, Indices>::Create(m_valueClassElements), 0)... };
                    (void)dummy;
                }

                template<size_t Index>
                bool GetElementAddressTuple(u32 elementNameCrc, const SerializeContext::ClassElement*& classElement) const
                {
                    if (!classElement && elementNameCrc == m_valueClassElements[Index].m_nameCrc)
                    {
                        classElement = &m_valueClassElements[Index];
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                const SerializeContext::ClassElement* GetElementAddressTuple(u32 elementNameCrc, AZStd::index_sequence<Indices...>) const
                {
                    (void)elementNameCrc;
                    const SerializeContext::ClassElement* classElement{};
                    bool dummy[]{ true, (GetElementAddressTuple<Indices>(elementNameCrc, classElement))... };
                    (void)dummy;
                    return classElement;
                }

                template<size_t Index>
                bool GetElementTuple(SerializeContext::ClassElement& classElement, u32 elementNameCrc, bool& elementFound) const
                {
                    if (!elementFound && elementNameCrc == m_valueClassElements[Index].m_nameCrc)
                    {
                        elementFound = true;
                        classElement = m_valueClassElements[Index];
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                bool GetElementTuple(SerializeContext::ClassElement& classElement, u32 elementNameCrc, AZStd::index_sequence<Indices...>) const
                {
                    (void)classElement;
                    (void)elementNameCrc;
                    bool elementFound = false;
                    bool dummy[]{ true, (GetElementTuple<Indices>(classElement, elementNameCrc, elementFound))... };
                    (void)dummy;
                    return elementFound;
                }

                template<size_t Index>
                bool EnumElementsTuple(TupleType& tupleRef, const ElementCB& cb, bool& aggregateCallbackResult)
                {
                    if (aggregateCallbackResult && cb(&AZStd::get<Index>(tupleRef), m_valueClassElements[Index].m_typeId,
                        m_valueClassElements[Index].m_genericClassInfo ? m_valueClassElements[Index].m_genericClassInfo->GetClassData() : nullptr, &m_valueClassElements[Index]))
                    {
                        return true;
                    }

                    aggregateCallbackResult = false;
                    return false;
                }
                template<size_t... Indices>
                void EnumElementsTuple(TupleType& tupleRef, const ElementCB& cb, AZStd::index_sequence<Indices...>)
                {
                    // When the parameter pack is empty the invoked Member functions are not called and therefore the function parameters are unused
                    (void)tupleRef;
                    (void)cb;
                    bool aggregateCBResult = true;
                    (void)aggregateCBResult;
                    bool dummy[]{ true, (EnumElementsTuple<Indices>(tupleRef, cb, aggregateCBResult))... };
                    (void)dummy;
                }

                template<size_t Index>
                bool ReserveElementTuple(TupleType& tupleRef, const SerializeContext::ClassElement* classElement, void*& reserveElement)
                {
                    if (!reserveElement && m_valueClassElements[Index].m_nameCrc == classElement->m_nameCrc)
                    {
                        reserveElement = &AZStd::get<Index>(tupleRef);
                        return true;
                    }
                    return false;
                }

                template<size_t... Indices>
                void* ReserveElementTuple(TupleType& tupleRef, const SerializeContext::ClassElement* classElement, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)classElement;
                    void* reserveElement{};
                    using DummyArray = bool[];
                    DummyArray{ true, (ReserveElementTuple<Indices>(tupleRef, classElement, reserveElement))... };
                    return reserveElement;
                }

                template<size_t Index>
                bool GetElementByIndexTuple(TupleType& tupleRef, size_t index, void*& resultElement)
                {
                    if (index == Index)
                    {
                        resultElement = &AZStd::get<Index>(tupleRef);
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                void* GetElementByIndexTuple(TupleType& tupleRef, size_t index, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)index;
                    void* resultElement{};
                    bool dummy[]{ true, (GetElementByIndexTuple<Indices>(tupleRef, index, resultElement))... };
                    (void)dummy;
                    return resultElement;
                }

                template<size_t Index>
                bool RemoveElementTuple(TupleType& tupleRef, const void* element, SerializeContext* deletePointerDataContext)
                {
                    if (&AZStd::get<Index>(tupleRef) == element && deletePointerDataContext)
                    {
                        DeletePointerData(deletePointerDataContext, &m_valueClassElements[Index], &AZStd::get<Index>(tupleRef));
                        AZStd::get<Index>(tupleRef) = AZStd::tuple_element_t<Index, TupleType>();
                        return true;
                    }
                    return false;
                }
                template<size_t... Indices>
                bool RemoveElementTuple(TupleType& tupleRef, const void* element, SerializeContext* deletePointerDataContext, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)element;
                    (void)deletePointerDataContext;
                    bool dummy[]{true, (RemoveElementTuple<Indices>(tupleRef, element, deletePointerDataContext))...};
                    (void)dummy;
                    return false; // Elements cannot be removed from tuple container
                }

                template<size_t... Indices>
                void DeletePointerDataTuple(TupleType& tupleRef, SerializeContext* deletePointerDataContext, AZStd::index_sequence<Indices...>)
                {
                    (void)tupleRef;
                    (void)deletePointerDataContext;
                    int dummy[]{0, (DeletePointerData(deletePointerDataContext, &m_valueClassElements[Indices], &AZStd::get<Indices>(tupleRef)), 0)...};
                    (void)dummy;
                }
        };

        template <class T>
        struct AZSmartPtrValueType
        {
            using value_type = typename T::value_type;
        };

        template <class E, class Deleter>
        struct AZSmartPtrValueType<AZStd::unique_ptr<E, Deleter>>
        {
            using value_type = typename AZStd::unique_ptr<E, Deleter>::element_type;
        };

        // Smart pointer generic handler
        template<class T>
        class AZStdSmartPtrContainer
            : public SerializeContext::IDataContainer
        {
        public:
            typedef typename AZSmartPtrValueType<T>::value_type ValueType;
            AZStdSmartPtrContainer()
            {
                m_classElement.m_name = GetDefaultElementName();
                m_classElement.m_nameCrc = GetDefaultElementNameCrc();
                m_classElement.m_dataSize = sizeof(ValueType*);
                m_classElement.m_offset = 0;
                m_classElement.m_azRtti = GetRttiHelper<ValueType>();
                m_classElement.m_flags = SerializeContext::ClassElement::FLG_POINTER;
                m_classElement.m_genericClassInfo = SerializeGenericTypeInfo<ValueType>::GetGenericInfo();
                m_classElement.m_typeId = SerializeGenericTypeInfo<ValueType>::GetClassTypeId();
                m_classElement.m_editData = nullptr;
            }

            /// Returns the element generic (offsets are mostly invalid 0xbad0ffe0, there are exceptions). Null if element with this name can't be found.
            virtual const SerializeContext::ClassElement* GetElement(AZ::u32 elementNameCrc) const override
            {
                if (elementNameCrc == m_classElement.m_nameCrc)
                {
                    return &m_classElement;
                }
                return nullptr;
            }

            bool GetElement(SerializeContext::ClassElement& classElement, const SerializeContext::DataElement& dataElement) const override
            {
                if (dataElement.m_nameCrc == m_classElement.m_nameCrc)
                {
                    classElement = m_classElement;
                    return true;
                }
                return false;
            }

            /// Enumerate elements in the array
            virtual void EnumElements(void* instance, const ElementCB& cb) override
            {
                T* smartPtr = reinterpret_cast<T*>(instance);
                // HACK HACK HACK!!!
                // We assume that the data pointer is at offset 0 inside the smart pointer!!!
                typename T::element_type * *valuePtr = reinterpret_cast<typename T::element_type**>(smartPtr);
                cb(valuePtr, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement);
            }

            /// Return number of elements in the container.
            virtual size_t  Size(void* instance) const override
            {
                (void)instance;
                return 1;
            }
            
            /// Returns the capacity of the container. Returns 0 for objects without fixed capacity.
            virtual size_t Capacity(void* instance) const override
            {
                (void)instance;
                return 1;
            }

            /// Returns true if elements pointers don't change on add/remove. If false you MUST enumerate all elements.
            virtual bool    IsStableElements() const override           { return true; }

            /// Returns true if the container is fixed size, otherwise false.
            virtual bool    IsFixedSize() const override                { return true; }

            /// Returns if the container is fixed capacity, otherwise false
            virtual bool    IsFixedCapacity() const override            { return true; }

            /// Returns true if the container is a smart pointer.
            virtual bool    IsSmartPointer() const override             { return true; }

            /// Returns true if the container elements can be addressesd by index, otherwise false.
            virtual bool    CanAccessElementsByIndex() const override   { return false; }

            /// Reserve element
            virtual void*   ReserveElement(void* instance, const  SerializeContext::ClassElement* classElement) override
            {
                (void)classElement;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
                return instance; // store the pointer in the first element of the smart pointer (this is hack, but it's true for our pointers and used in the serializer)
            }

            /// Get an element's address by its index (called before the element is loaded).
            virtual void*   GetElementByIndex(void* instance, const SerializeContext::ClassElement* classElement, size_t index) override
            {
                (void)classElement;
                (void)index;
                
                void* ptrToRawPtr = nullptr;
                auto captureValue = [&ptrToRawPtr](void* ptr, const AZ::Uuid&, const AZ::SerializeContext::ClassData*, const AZ::SerializeContext::ClassElement*) -> bool 
                {
                    ptrToRawPtr = ptr;
                    return false;
                };
                EnumElements(instance, captureValue);
                typename T::element_type *valuePtr = *reinterpret_cast<typename T::element_type**>(ptrToRawPtr);
                return valuePtr;
            }

            /// Store element
            virtual void    StoreElement(void* instance, void* element) override
            {
                (void)element;
                void* newElementAddress = *reinterpret_cast<void**>(instance);
                *reinterpret_cast<void**>(instance) = nullptr; // reset the smart pointer first element to the value before Reserve element
                T* smartPtr = reinterpret_cast<T*>(instance);
                *smartPtr = T(reinterpret_cast<ValueType*>(newElementAddress)); // store the new element properly, by assignment (so all counters are correct)
            }

            /// Remove element in the container.
            virtual bool    RemoveElement(void* instance, const void* element, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                (void)element;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
                return false; // you can't remove element from this container.
            }

            /// Remove elements (removed array of elements) regardless if the container is Stable or not (IsStableElements)
            virtual size_t  RemoveElements(void* instance, const void** elements, size_t numElements, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                (void)elements;
                (void)numElements;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
                return 0; // you can't remove elements from this container.
            }

            /// Clear elements in the instance.
            virtual void    ClearElements(void* instance, SerializeContext* deletePointerDataContext) override
            {
                (void)deletePointerDataContext;
                T* smartPtr = reinterpret_cast<T*>(instance);
                smartPtr->reset();
            }

            SerializeContext::ClassElement m_classElement;  ///< Generic class element covering as must as possible of the element (offset, and some other fields are invalid)
        };

        // basic string serialization
        template<class T>
        class AZStdString
            : public SerializeContext::IDataSerializer
        {
            /// Convert binary data to text
            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;

                size_t dataSize = static_cast<size_t>(in.GetLength());

                AZStd::string outText;
                outText.resize(dataSize);
                in.Read(dataSize, outText.data());

                return static_cast<size_t>(out.Write(outText.size(), outText.c_str()));
            }

            /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
            size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)textVersion;
                (void)isDataBigEndian;

                size_t bytesToWrite = strlen(text);

                return static_cast<size_t>(stream.Write(bytesToWrite, reinterpret_cast<const void*>(text)));
            }

            /// Store the class data into a stream.
            size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;

                const T* string = reinterpret_cast<const T*>(classPtr);
                size_t bytesToWrite = string->size();

                return static_cast<size_t>(stream.Write(bytesToWrite, string->c_str()));
            }

            /// Load the class data from a stream.
            bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;

                T* string = reinterpret_cast<T*>(classPtr);
                size_t textLen = static_cast<size_t>(stream.GetLength());

                string->resize(textLen);
                stream.Read(textLen, string->data());

                return true;
            }

            bool    CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<T>::CompareValues(lhs, rhs);
            }
        };

        class AZBinaryData
            : public SerializeContext::IDataSerializer
        {
            size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;
                const size_t dataSize = static_cast<size_t>(in.GetLength()); // each byte is encoded in 2 chars (hex)

                AZStd::string outStr;
                if (dataSize)
                {
                    AZ::u8* buffer = static_cast<AZ::u8*>(azmalloc(dataSize));

                    in.Read(dataSize, reinterpret_cast<void*>(buffer));

                    const AZ::u8* first = buffer;
                    const AZ::u8* last = first + dataSize;
                    if (first == last)
                    {
                        azfree(buffer);
                        outStr.clear();
                        return 1; // for no data (just terminate)
                    }

                    outStr.resize(dataSize * 2);

                    char* data = outStr.data();
                    for (; first != last; ++first, data += 2)
                    {
                        azsnprintf(data, 3, "%02X", *first);
                    }

                    azfree(buffer);
                }

                return static_cast<size_t>(out.Write(outStr.size(), outStr.data()));
            }

            size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) override
            {
                (void)textVersion;
                (void)isDataBigEndian;
                size_t textLenth = strlen(text);
                size_t minDataSize = textLenth / 2;

                const char* textEnd = text + textLenth;
                char workBuffer[3];
                workBuffer[2] = '\0';
                for (; text != textEnd; text += 2)
                {
                    workBuffer[0] = text[0];
                    workBuffer[1] = text[1];
                    AZ::u8 value = static_cast<AZ::u8>(strtoul(workBuffer, NULL, 16));
                    stream.Write(sizeof(AZ::u8), &value);
                }
                return minDataSize;
            }
        };

        template<class Allocator>
        class AZByteStream
            : public AZBinaryData
        {
            typedef AZStd::vector<AZ::u8, Allocator> ContainerType;

            /// Load the class data from a stream.
            bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;
                const AZ::u8* dataStart;
                const AZ::u8* dataEnd;
                ContainerType* container = reinterpret_cast<ContainerType*>(classPtr);

                size_t dataSize = static_cast<size_t>(stream.GetLength());
                if (dataSize)
                {
                    AZ::u8* data = static_cast<AZ::u8*>(azmalloc(dataSize));

                    size_t bytesRead = static_cast<size_t>(stream.Read(dataSize, data));

                    dataStart = reinterpret_cast<const AZ::u8*>(data);
                    dataEnd = dataStart + bytesRead;

                    *container = ContainerType(dataStart, dataEnd);
                    azfree(data);
                }
                else
                {
                    dataStart = nullptr;
                    dataEnd = nullptr;
                    *container = ContainerType(dataStart, dataEnd);
                }

                return true;
            }

            /// Store the class data into a stream.
            size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;
                const ContainerType* container = reinterpret_cast<const ContainerType*>(classPtr);
                size_t dataSize = container->size();
                return static_cast<size_t>(stream.Write(dataSize, container->data()));
            }

            bool    CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<ContainerType>::CompareValues(lhs, rhs);
            }
        };

        template<AZStd::size_t NumBits>
        class AZBitSet
            : public AZBinaryData
        {
            typedef typename AZStd::bitset<NumBits> ContainerType;

            /// Store the class data into a stream.
            size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
            {
                (void)isDataBigEndian;
                const ContainerType* container = reinterpret_cast<const ContainerType*>(classPtr);
                size_t dataSize = container->num_words() * sizeof(typename ContainerType::word_t);
                return static_cast<size_t>(stream.Write(dataSize, container->data()));
            }

            /// Load the class data from a stream.
            bool Load(void* classPtr, IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian = false) override
            {
                (void)isDataBigEndian;
                ContainerType* container = reinterpret_cast<ContainerType*>(classPtr);
                size_t dataSize = static_cast<size_t>(stream.GetLength());
                size_t bytes = container->num_words() * sizeof(typename ContainerType::word_t);
                (void)dataSize;
                AZ_Assert(bytes <= dataSize, "Not enough data provided expected %d bytes and got %d", bytes, dataSize);

                stream.Read(bytes, container->data());
                return true;
            }

            bool    CompareValueData(const void* lhs, const void* rhs) override
            {
                return SerializeContext::EqualityCompareHelper<ContainerType>::CompareValues(lhs, rhs);
            }
        };
    } // namespace Internal

    /// Generic specialization for AZStd::vector
    template<class T, class A>
    struct SerializeGenericTypeInfo< AZStd::vector<T, A> >
    {
        typedef typename AZStd::vector<T, A> ContainerType;

        class GenericClassInfoVector
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoVector, "{2BADE35A-6F1B-4698-B2BC-3373D010020C}");
            GenericClassInfoVector()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::vector", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassInfoVector* Instance()
            {
                static GenericClassInfoVector s_instance;
                return &s_instance;
            }

            Internal::AZStdBasicContainer<ContainerType, false> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassInfoVector::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassInfoVector::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::fixed_vector
    template<class T, size_t Capacity>
    struct SerializeGenericTypeInfo< AZStd::fixed_vector<T, Capacity> >
    {
        typedef typename AZStd::fixed_vector<T, Capacity>    ContainerType;

        class GenericClassInfoFixedVector
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoFixedVector, "{6C6751B0-392A-4E71-8BF8-179484D7D22F}");
            GenericClassInfoFixedVector()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::fixed_vector", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassInfoFixedVector* Instance()
            {
                static GenericClassInfoFixedVector s_instance;
                return &s_instance;
            }

            Internal::AZStdFixedCapacityContainer<ContainerType, true, Capacity> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassInfoFixedVector::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassInfoFixedVector::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::list
    template<class T, class A>
    struct SerializeGenericTypeInfo< AZStd::list<T, A> >
    {
        typedef typename AZStd::list<T, A>           ContainerType;

        class GenericClassInfoList
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoList, "{B845AD64-B5A0-4ccd-A86B-3477A36779BE}");
            GenericClassInfoList()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::list", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassInfoList* Instance()
            {
                static GenericClassInfoList s_instance;
                return &s_instance;
            }

            Internal::AZStdBasicContainer<ContainerType, true> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassInfoList::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassInfoList::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::forward_list
    template<class T, class A>
    struct SerializeGenericTypeInfo< AZStd::forward_list<T, A> >
    {
        typedef typename AZStd::forward_list<T, A> ContainerType;

        class GenericClassInfoForwardList
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoForwardList, "{D48E20E1-D3A3-46F7-A869-927F5EF50127}");
            GenericClassInfoForwardList()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::forward_list", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassInfoForwardList* Instance()
            {
                static GenericClassInfoForwardList s_instance;
                return &s_instance;
            }

            Internal::AZStdBasicContainer<ContainerType, true> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassInfoForwardList::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassInfoForwardList::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::array
    template<class T, size_t Size>
    struct SerializeGenericTypeInfo< AZStd::array<T, Size> >
    {
        typedef typename AZStd::array<T, Size>  ContainerType;

        class GenericClassInfoArray
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassInfoArray, "{286E1198-0867-4198-95D3-6CC569658E07}");
            GenericClassInfoArray()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::array", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
                m_classData.m_eventHandler = &m_eventHandler;
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassInfoArray* Instance()
            {
                static GenericClassInfoArray s_instance;
                return &s_instance;
            }

            Internal::AZStdArrayContainer<T, Size> m_containerStorage;
            SerializeContext::ClassData m_classData;
            Internal::AZStdArrayEvents m_eventHandler;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassInfoArray::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassInfoArray::Instance()->m_classData.m_typeId;
        }
    };

    // Generic specialization for AZStd::set
    template<class K, class C, class A>
    struct SerializeGenericTypeInfo< AZStd::set<K, C, A> >
    {
        typedef typename AZStd::set<K, C, A>        ContainerType;

        class GenericClassSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassSet, "{4A64D2A5-7265-4E3D-805C-BA2D0626F542}");
            GenericClassSet()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::set", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t /*element*/) override
            {
                return SerializeGenericTypeInfo<K>::GetClassTypeId();               
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassSet* Instance()
            {
                static GenericClassSet s_instance;
                return &s_instance;
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassSet::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassSet::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::unordered_set
    template<class K, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_set<K, H, E, A> >
    {
        typedef typename AZStd::unordered_set<K, H, E, A>      ContainerType;

        class GenericClassUnorderedSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedSet, "{B04E902E-C6F7-4212-A166-1B52F7437D3C}");
            GenericClassUnorderedSet()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::unordered_set", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<K>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassUnorderedSet* Instance()
            {
                static GenericClassUnorderedSet s_instance;
                return &s_instance;
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassUnorderedSet::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassUnorderedSet::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::unordered_multiset
    template<class K, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_multiset<K, H, E, A> >
    {
        typedef typename AZStd::unordered_multiset<K, H, E, A>      ContainerType;

        class GenericClassUnorderedMultiSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedMultiSet, "{B619DA0D-F050-41AA-B092-416CACB7C710}");
            GenericClassUnorderedMultiSet()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::unordered_multiset", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<K>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassUnorderedMultiSet* Instance()
            {
                static GenericClassUnorderedMultiSet s_instance;
                return &s_instance;
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassUnorderedMultiSet::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassUnorderedMultiSet::Instance()->m_classData.m_typeId;
        }

    };

    /// Generic specialization for AZStd::pair
    template<class T1, class T2>
    struct SerializeGenericTypeInfo< AZStd::pair<T1, T2> >
    {
        typedef typename  AZStd::pair<T1, T2> PairType;

        class GenericClassPair
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassPair, "{9F3F5302-3390-407a-A6F7-2E011E3BB686}");
            GenericClassPair()
            {
                m_classData = SerializeContext::ClassData::Create<PairType>("AZStd::pair", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_pairContainer);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<T1>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<T2>::GetClassTypeId();
                }
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<PairType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<PairType>::CreateAny);
                    if (GenericClassInfo* firstGenericClassInfo = m_pairContainer.m_value1ClassElement.m_genericClassInfo)
                    {
                        firstGenericClassInfo->Reflect(serializeContext);
                    }

                    if (GenericClassInfo* secondGenericClassInfo = m_pairContainer.m_value2ClassElement.m_genericClassInfo)
                    {
                        secondGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassPair* Instance()
            {
                static GenericClassPair s_instance;
                return &s_instance;
            }

            Internal::AZStdPairContainer<T1, T2> m_pairContainer;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassPair::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassPair::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::tuple
    template<typename... Types>
    struct SerializeGenericTypeInfo< AZStd::tuple<Types...> >
    {
        using TupleType = AZStd::tuple<Types...>;

        class GenericClassTuple
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassTuple, "{F98DF943-F870-4FE2-B6A9-3E8BC5861782}");
            GenericClassTuple()
            {
                m_classData = SerializeContext::ClassData::Create<TupleType>("AZStd::tuple", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_tupleContainer);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return Internal::AZStdTupleContainer<TupleType>::s_tupleSize;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                if (GenericClassInfo* valueGenericClassInfo = m_tupleContainer.m_valueClassElements[element].m_genericClassInfo)
                {
                    return valueGenericClassInfo->GetSpecializedTypeId();
                }
                return m_tupleContainer.m_valueClassElements[element].m_typeId;
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<TupleType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<TupleType>::CreateAny);
                    for (size_t index = 0; index < AZStd::tuple_size<TupleType>::value; ++index)
                    {
                        if (GenericClassInfo* valueGenericClassInfo = m_tupleContainer.m_valueClassElements[index].m_genericClassInfo)
                        {
                            valueGenericClassInfo->Reflect(serializeContext);
                        }

                    }
                }
            }

            static GenericClassTuple* Instance()
            {
                static GenericClassTuple s_instance;
                return &s_instance;
            }

            Internal::AZStdTupleContainer<Types...> m_tupleContainer;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassTuple::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassTuple::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::map
    template<class K, class M, class C, class A>
    struct SerializeGenericTypeInfo< AZStd::map<K, M, C, A> >
    {
        typedef typename AZStd::map<K, M, C, A>        ContainerType;

        class GenericClassMap
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassMap, "{DB825311-453D-45C8-B07F-B9CD9A32ACB4}");
            GenericClassMap()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::map", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<K>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<M>::GetClassTypeId();
                }
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassMap* Instance()
            {
                static GenericClassMap s_instance;
                return &s_instance;
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassMap::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassMap::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::unordered_map
    template<class K, class M, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_map<K, M, H, E, A> >
    {
        typedef typename AZStd::unordered_map<K, M, H, E, A>        ContainerType;

        class GenericClassUnorderedMap
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedMap, "{18456A80-63CC-40c5-BF16-6AF94F9A9ECC}");
            GenericClassUnorderedMap()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::unordered_map", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<K>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<M>::GetClassTypeId();
                }
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassUnorderedMap* Instance()
            {
                static GenericClassUnorderedMap s_instance;
                return &s_instance;
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassUnorderedMap::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassUnorderedMap::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::unordered_multimap
    template<class K, class M, class H, class E, class A>
    struct SerializeGenericTypeInfo< AZStd::unordered_multimap<K, M, H, E, A> >
    {
        typedef typename AZStd::unordered_multimap<K, M, H, E, A>        ContainerType;

        class GenericClassUnorderedMultiMap
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUnorderedMultiMap, "{119669B8-92BF-468F-97D9-9D45F298BCD4}");
            GenericClassUnorderedMultiMap()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::unordered_multimap", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 2;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                if (element == 0)
                {
                    return SerializeGenericTypeInfo<K>::GetClassTypeId();
                }
                else
                {
                    return SerializeGenericTypeInfo<M>::GetClassTypeId();
                }
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            static GenericClassUnorderedMultiMap* Instance()
            {
                static GenericClassUnorderedMultiMap s_instance;
                return &s_instance;
            }

            Internal::AZStdAssociativeContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassUnorderedMultiMap::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassUnorderedMultiMap::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::string
    template<class E, class T, class A>
    struct SerializeGenericTypeInfo< AZStd::basic_string<E, T, A> >
    {
        typedef typename AZStd::basic_string<E, T, A>     ContainerType;

        class GenericClassBasicString
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassBasicString, "{EF8FF807-DDEE-4eb0-B678-4CA3A2C490A4}");
            GenericClassBasicString()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::string", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), &m_stringSerializer, nullptr);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<E>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                }
            }

            static GenericClassBasicString* Instance()
            {
                static GenericClassBasicString s_instance;
                return &s_instance;
            }

            Internal::AZStdString<ContainerType> m_stringSerializer;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassBasicString::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassBasicString::Instance()->m_classData.m_typeId;
        }
    };

    /// Generic specialization for AZStd::vector<AZ::u8,Allocator> byte stream. If this conflics with a normal vector<unsigned char> create a new type.
    template<class A>
    struct SerializeGenericTypeInfo< AZStd::vector<AZ::u8, A> >
    {
        typedef typename AZStd::vector<AZ::u8, A>    ContainerType;

        class GenericClassByteStream
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassByteStream, "{6F949CC5-24A4-4229-AC8B-C5E6C70E145E}");
            GenericClassByteStream()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("ByteStream", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), &m_dataSerializer, nullptr);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<AZ::u8>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                }
            }

            static GenericClassByteStream* Instance()
            {
                static GenericClassByteStream s_instance;
                return &s_instance;
            }

            Internal::AZByteStream<A> m_dataSerializer;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassByteStream::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassByteStream::Instance()->m_classData.m_typeId;
        }
    };

    template<AZStd::size_t NumBits>
    struct SerializeGenericTypeInfo< AZStd::bitset<NumBits> >
    {
        typedef typename AZStd::bitset<NumBits> ContainerType;

        class GenericClassBitSet
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassBitSet, "{1C0270B7-F5E1-4bd6-B7BC-8D25A74B79B4}");
            GenericClassBitSet()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("BitSet", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), &m_dataSerializer, nullptr);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 0;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<AZ::u8>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                }
            }

            static GenericClassBitSet* Instance()
            {
                static GenericClassBitSet s_instance;
                return &s_instance;
            }

            Internal::AZBitSet<NumBits> m_dataSerializer;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassBitSet::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassBitSet::Instance()->m_classData.m_typeId;
        }
    };

    template<class T>
    struct SerializeGenericTypeInfo< AZStd::shared_ptr<T> >
    {
        typedef typename AZStd::shared_ptr<T>   ContainerType;

        class GenericClassSharedPtr
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassSharedPtr, "{D5B5ACA6-A81E-410E-8151-80C97B8CD2A0}");
            GenericClassSharedPtr()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::shared_ptr", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            // AZStdSmartPtrContainer uses the underlying smart_ptr container value_type typedef type id for serialization
            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            bool CanStoreType(const Uuid& typeId) const override
            {
                return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || m_containerStorage.m_classElement.m_typeId == typeId;
            }

            static GenericClassSharedPtr* Instance()
            {
                static GenericClassSharedPtr s_instance;
                return &s_instance;
            }

            Internal::AZStdSmartPtrContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassSharedPtr::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassSharedPtr::Instance()->m_classData.m_typeId;
        }
    };

    template<class T>
    struct SerializeGenericTypeInfo< AZStd::intrusive_ptr<T> >
    {
        typedef typename AZStd::intrusive_ptr<T>    ContainerType;

        class GenericClassIntrusivePtr
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassIntrusivePtr, "{C4B5400B-5CDC-4B14-932E-BFA30BC1DE35}");
            GenericClassIntrusivePtr()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::intrusive_ptr", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            // AZStdSmartPtrContainer uses the underlying smart_ptr container value_type typedef type id for serialization
            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            bool CanStoreType(const Uuid& typeId) const override
            {
                return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || m_containerStorage.m_classElement.m_typeId == typeId;
            }

            static GenericClassIntrusivePtr* Instance()
            {
                static GenericClassIntrusivePtr s_instance;
                return &s_instance;
            }

            Internal::AZStdSmartPtrContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassIntrusivePtr::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassIntrusivePtr::Instance()->m_classData.m_typeId;
        }
    };

    template<class T, class Deleter>
    struct SerializeGenericTypeInfo< AZStd::unique_ptr<T, Deleter> >
    {
        typedef typename AZStd::unique_ptr<T, Deleter>   ContainerType;

        class GenericClassUniquePtr
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassUniquePtr, "{D37DF835-5B18-4F9E-9C8F-03B967483080}");
            GenericClassUniquePtr()
            {
                m_classData = SerializeContext::ClassData::Create<ContainerType>("AZStd::unique_ptr", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, &m_containerStorage);
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 0;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            // AZStdSmartPtrContainer uses the smart_ptr container type id for serialization
            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<ContainerType>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext* serializeContext)
            {
                if (serializeContext)
                {
                    serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ContainerType>::CreateAny);
                    if (GenericClassInfo* containerGenericClassInfo = m_containerStorage.m_classElement.m_genericClassInfo)
                    {
                        containerGenericClassInfo->Reflect(serializeContext);
                    }
                }
            }

            bool CanStoreType(const Uuid& typeId) const override
            {
                return GetSpecializedTypeId() == typeId || GetGenericTypeId() == typeId || m_containerStorage.m_classElement.m_typeId == typeId;
            }

            static GenericClassUniquePtr* Instance()
            {
                static GenericClassUniquePtr s_instance;
                return &s_instance;
            }

            Internal::AZStdSmartPtrContainer<ContainerType> m_containerStorage;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassUniquePtr::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassUniquePtr::Instance()->m_classData.m_typeId;
        }
    };

    //// TODO: move to a separate file
    //template<class T>
    //struct SerializeGenericTypeInfo< AZ::ScriptProperty<T> >
    //{
    //  typedef typename AZ::ScriptProperty<T>  ContainerType;

    //  static void*    ClassCreator(void* /*userData*/, const char* /* DbgClassName */)
    //  {
    //      return aznew ContainerType;
    //  }
    //  static void     ClassDestructor(void* instance, void* /*userData */)
    //  {
    //      delete reinterpret_cast<ContainerType*>(instance);
    //  }
    //  static SerializeContext::ClassData* GetClassData()
    //  {
    //      static Internal::GenericSingleElementContainer<ContainerType> s_containerStorage;
    //      static SerializeContext::ClassData s_classData = SerializeContext::ClassData::Create<ContainerType>("AZ::ScriptProperty", GetClassTypeId(), &ClassCreator, &ClassDestructor, nullptr, nullptr, &s_containerStorage);
    //      return &s_classData;
    //  }
    //  static const Uuid& GetClassTypeId()
    //  {
    //      static Uuid s_typeId("{B53FD9F7-D9D1-404F-8B3D-0898AC578EC1}");
    //      return s_typeId;
    //  }
    //};
}

#endif // AZCORE_SERIALIZE_AZSTD_CONTAINERS_H
#pragma once
