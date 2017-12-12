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

#include <AzCore/RTTI/ReflectContext.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/utils.h>

#include <AzCore/std/typetraits/decay.h>
#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/RTTI/OnDemandReflection.h>
#include <AzCore/RTTI/BehaviorObjectSignals.h>
#include <AzCore/Preprocessor/Sequences.h> // for AZ_EBUS_BEHAVIOR_BINDER
#include <AzCore/std/containers/array.h>
#include <AzCore/std/typetraits/function_traits.h>

#if defined(AZ_COMPILER_MSVC)
#   pragma warning(push)
#   pragma warning(disable: 4127) // conditional expression is constant
#endif

namespace AZ
{
    /// Typedef for class unwrapping callback (i.e. used for things like smart_ptr<T> to unwrap for T)
    using BehaviorClassUnwrapperFunction = void(*)(void* /*classPtr*/, void*& /*unwrappedClass*/, AZ::Uuid& /*unwrappedClassTypeId*/, void* /*userData*/);

    class BehaviorContext;
    class BehaviorClass;
    class BehaviorProperty;
    class BehaviorEBusHandler;

    struct BehaviorObject // same as DynamicSerializableField, make sure we merge them... so we can store the object easily 
    {
        AZ_TYPE_INFO(BehaviorObject, "{2813cdfb-0a4a-411c-9216-72a7b644d1dd}");

        BehaviorObject();
        BehaviorObject(void* address, const Uuid& typeId);
        bool IsValid() const;

        void* m_address;
        Uuid m_typeId;
    };

    /**
     * Stores information about a function parameter (no instance). During calls we use \ref BehaviorValueParameter which in addition
     * offers value storage and functions to interact with the data
     */
    struct BehaviorParameter
    {
        /// Temporary POD buffer when we convert parameters on the stack.
        typedef AZStd::static_buffer_allocator<32, 32> TempValueParameterAllocator;

        /**
         * Function parameters traits
         */
        enum Traits
        {
            TR_POINTER = (1 << 0),
            TR_CONST = (1 << 1),
            TR_REFERENCE = (1 << 2),
            TR_THIS_PTR = (1 << 3), // set if the parameter is a this pointer to a method
        };

        const char* m_name;
        Uuid m_typeId;
        IRttiHelper* m_azRtti;
        u8 m_traits;
    };

    /**
     * BehaviorValueParameter is used for calls on the stack. It should not be reused or stored as we might store temp data
     * during conversion in the class on the stack. For storing type info use \ref BehaviorParameter
     *
     * NOTE: If you get a (VS2013) error about adding AZ_RTTI info to BehaviorValueParameter in a situation using
     * operator=, use void BehaviorValueParameter::Set(const BehaviorValueParameter& param), instead.
     */
    struct BehaviorValueParameter : public BehaviorParameter
    {
        BehaviorValueParameter();

        template<class T>
        BehaviorValueParameter(T* value);

        /// Special handling for the generic object holder.
        BehaviorValueParameter(BehaviorObject* value);

        template<class T>
        void Set(T* value);
        void Set(BehaviorObject* value);
        void Set(const BehaviorParameter& param);
        void Set(const BehaviorValueParameter& param);

        void* GetValueAddress() const;

        /// Convert to BehaviorObject implicitly for passing generic parameters (usually not known at compile time)
        operator BehaviorObject() const;

        /// Converts internally the value to a specific type known at compile time. \returns true if conversion was successful. 
        template<class T>
        bool ConvertTo();

        /// Converts a value to a specific one by typeID (usually when the type is not known at compile time)
        bool ConvertTo(const AZ::Uuid& typeId);

        /// This function is Unsafe, because it assumes that you have called ConvertTo<T> prior to called it and it returned true (basically mean the BehaviorValueParameter is converted to T)
        template<typename T>
        AZStd::decay_t<T>*  GetAsUnsafe() const;

        template<typename T>
        BehaviorValueParameter& operator=(T&& result);

        /// Stores a value (usually return value of a function).
        template<typename T>
        bool StoreResult(T&& result);

        /// Used internally to store values in the temp data
        template<typename T>
        void StoreInTempData(T&& value);

        void* m_value;  ///< Pointer to value, keep it mind to check the traits as if the value is pointer, this will be pointer to pointer and use \ref GetValueAddress to get the actual value address
        AZStd::function<void()> m_onAssignedResult;
        BehaviorParameter::TempValueParameterAllocator m_tempData; ///< Temp data for conversion, etc. while preparing the parameter for a call (POD only)
    };

    /**
     * Base class that handles default values. Values types are verified to match exactly the function signature.
     * the oder of the default values is the same as in C++ (going in reverse the back).
     */
    class BehaviorValues
    {
    public:
        virtual ~BehaviorValues() {}

        virtual size_t GetNumValues() const = 0;
        virtual const BehaviorValueParameter& GetValue(size_t i) const = 0;
    };

    /**
     * Use behavior method to get type information and invoke reflected methods.
     */
    class BehaviorMethod
    {
    public:
        BehaviorMethod();
        virtual ~BehaviorMethod();

        template<class... Args>
        bool Invoke(Args&&... args);
        bool Invoke();

        template<class R, class... Args>
        bool InvokeResult(R& r, Args&&... args);
        template<class R>
        bool InvokeResult(R& r);

        virtual bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result = nullptr) = 0;
        virtual bool HasResult() const = 0;
        /// Returns true if the method is a class member method. If true the first argument should always be the "this"/ClassType pointer.
        virtual bool IsMember() const = 0;
        virtual size_t GetNumArguments() const = 0;
        virtual size_t GetNumArgumentNames() const = 0;
        /// Return the minimum number of arguments needed (considering default arguments)
        size_t GetMinNumberOfArguments() const;
        virtual const BehaviorParameter* GetArgument(size_t index) const = 0;
        virtual const AZStd::string* GetArgumentName(size_t index) const = 0;
        virtual void SetArgumentName(size_t index, const AZStd::string& name) = 0;
        virtual const BehaviorParameter* GetResult() const = 0;

        AZStd::string m_name;   ///< Debug friendly behavior method name
        const char* m_debugDescription;
        BehaviorValues* m_defaultArguments;
        bool m_isConst = false; ///< Is member function const (false if not a member function)
        AttributeArray m_attributes;    ///< Attributes for the method
    };

    namespace Internal
    {
        // Converts sourceAddress to targetType
        inline bool ConvertValueTo(void* sourceAddress, const IRttiHelper* sourceRtti, const AZ::Uuid& targetType, void*& targetAddress, BehaviorParameter::TempValueParameterAllocator& tempAllocator)
        {
            // convert
            void* convertedAddress = sourceRtti->Cast(sourceAddress, targetType);
            if (convertedAddress && convertedAddress != sourceAddress) // if we converted as we have a different address
        {
                // allocate temp storage and store it
                targetAddress = tempAllocator.allocate(sizeof(void*), AZStd::alignment_of<void*>::value, 0);
                *reinterpret_cast<void**>(targetAddress) = convertedAddress;
            }
            return convertedAddress != nullptr;
            }

        // Assumes parameters array is big enough to store all parameters
        template<class... Args>
        void SetParameters(BehaviorParameter* parameters, BehaviorContext* behaviorContext = nullptr);

        // call helper
        template<class R, class... Args>
        struct CallFunction
        {
            template<AZStd::size_t... Is, class Function>
            static inline void Global(Function functionPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>);
            template<AZStd::size_t... Is, class C, class Function>
            static inline void Member(Function functionPtr, C thisPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>);
        };

        template<class... Args>
        struct CallFunction<void, Args...>
        {
            template<AZStd::size_t... Is, class Function>
            static inline void Global(Function functionPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                functionPtr(*arguments[Is].GetAsUnsafe<Args>()...);
            };

            template<AZStd::size_t... Is, class C, class Function>
            static inline void Member(Function functionPtr, C thisPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                (thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...);
            };
        };

        template<class Function>
        class BehaviorMethodImpl;

        template<class R, class... Args>
        class BehaviorMethodImpl<R(Args...)> : public BehaviorMethod
        {
        public:
            using FunctionPointer = R(*)(Args...);
            typedef void ClassType;

            AZ_CLASS_ALLOCATOR(BehaviorMethodImpl<R(Args...)>, AZ::SystemAllocator, 0);

            static const int s_startNamedArgumentIndex = 1; // +1 for result type

            BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name = AZStd::string());

            bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) override;

            bool HasResult() const override;
            bool IsMember() const override;

            size_t GetNumArguments() const override;
            size_t GetNumArgumentNames() const override;

            const BehaviorParameter* GetArgument(size_t index) const override;
            const AZStd::string* GetArgumentName(size_t index) const override;
            void SetArgumentName(size_t index, const AZStd::string& name) override;

            const BehaviorParameter* GetResult() const override;

            FunctionPointer m_functionPtr;

            BehaviorParameter m_parameters[sizeof...(Args) + s_startNamedArgumentIndex]; 
            AZStd::string m_argumentNames[sizeof...(Args) +s_startNamedArgumentIndex];
        };

        template<class R, class C, class... Args>
        class BehaviorMethodImpl<R(C::*)(Args...)> : public BehaviorMethod
        {
        public:
            using FunctionPointer = R(C::*)(Args...);
            using FunctionPointerConst = R(C::*)(Args...) const;
            typedef C ClassType;

            AZ_CLASS_ALLOCATOR(BehaviorMethodImpl<R(C::*)(Args...)>, AZ::SystemAllocator, 0);

            static const int s_startNamedArgumentIndex = 2; // +1 for result type, +1 for class Type (this ptr)

            BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name = AZStd::string());
            BehaviorMethodImpl(FunctionPointerConst functionPointer, BehaviorContext* context, const AZStd::string& name = AZStd::string());

            bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) override;

            bool HasResult() const override;
            bool IsMember() const override;

            size_t GetNumArguments() const override;
            size_t GetNumArgumentNames() const override;
            const BehaviorParameter* GetArgument(size_t index) const override;
            const AZStd::string* GetArgumentName(size_t index) const override;
            void SetArgumentName(size_t index, const AZStd::string& name) override;
            const BehaviorParameter* GetResult() const override;

            FunctionPointer m_functionPtr;
            BehaviorParameter m_parameters[sizeof...(Args) +s_startNamedArgumentIndex];
            AZStd::string m_argumentNames[sizeof...(Args) +s_startNamedArgumentIndex];
        };

        enum BehaviorEventType
        {
            BE_BROADCAST,
            BE_EVENT_ID,
            BE_QUEUE_BROADCAST,
            BE_QUEUE_EVENT_ID,
        };

        template<BehaviorEventType EventType, class EBus, class R, class... Args>
        struct EBusCaller;

        template<class EBus, class... Args>
        struct EBusCaller<BE_BROADCAST, EBus, void, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                EBus::Broadcast(e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_BROADCAST, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)arguments;
                if (result)
                {
                EBus::BroadcastResult(*result, e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
                else
                {
                    EBus::Broadcast(e, *arguments[Is].GetAsUnsafe<Args>()...);
                }
            }
        };

        template<class EBus, class... Args>
        struct EBusCaller<BE_EVENT_ID, EBus, void, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result;
                BehaviorValueParameter& id = arguments[0];
                ++arguments;
                EBus::Event(*id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_EVENT_ID, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                BehaviorValueParameter& id = *arguments++;
                if (result)
                {
                EBus::EventResult(*result, *id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
                else
                {
                    EBus::Event(*id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
                }
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_QUEUE_BROADCAST, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result; (void)arguments;
                EBus::QueueBroadcast(e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, class R, class... Args>
        struct EBusCaller<BE_QUEUE_EVENT_ID, EBus, R, Args...>
        {
            template<AZStd::size_t... Is, class Event>
            static void Call(Event e, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
            {
                (void)result;
                BehaviorValueParameter& id = *arguments++;
                EBus::QueueEvent(*id.GetAsUnsafe<typename EBus::BusIdType>(), e, *arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        template<class EBus, BehaviorEventType EventType, class Function>
        class BehaviorEBusEvent;

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        class BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)> : public BehaviorMethod
        {
        public:
            typedef R(C::*FunctionPointer)(Args...);

            static const int s_isBusIdParameter = (EventType == BE_EVENT_ID || EventType == BE_QUEUE_EVENT_ID) ? 1 : 0;

            static const int s_startNamedArgumentIndex = 1 + s_isBusIdParameter; // +1 for result type, +1 (optional for busID)

            AZ_CLASS_ALLOCATOR(BehaviorEBusEvent, AZ::SystemAllocator, 0);

            BehaviorEBusEvent(FunctionPointer functionPointer, BehaviorContext* context);

            template<bool IsBusId>
            inline AZStd::enable_if_t<IsBusId> SetBusIdType();

            template<bool IsBusId>
            inline AZStd::enable_if_t<!IsBusId> SetBusIdType();

            bool Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result) override;
            bool HasResult() const override;
            bool IsMember() const override;

            size_t GetNumArguments() const override;
            size_t GetNumArgumentNames() const override;
            const BehaviorParameter* GetArgument(size_t index) const override;
            const AZStd::string* GetArgumentName(size_t index) const override;
            void SetArgumentName(size_t index, const AZStd::string& name) override;
            const BehaviorParameter* GetResult() const override;

            FunctionPointer m_functionPtr;
            BehaviorParameter m_parameters[sizeof...(Args) + s_startNamedArgumentIndex];
            AZStd::string m_argumentNames[sizeof...(Args) + s_startNamedArgumentIndex];

        };

        template<class F>
        struct SetFunctionParameters;

        template<class R, class... Args>
        struct SetFunctionParameters<R(Args...)>
        {
            static void Set(AZStd::vector<BehaviorParameter>& params);
            static bool Check(AZStd::vector<BehaviorParameter>& source);
        };

        template<class R, class C, class... Args>
        struct SetFunctionParameters<R(C::*)(Args...)>
        {
            static void Set(AZStd::vector<BehaviorParameter>& params);
            static bool Check(AZStd::vector<BehaviorParameter>& source);
        };

        template<class T>
        struct BahaviorDefaultFactory
        {
            static void* Create(void* inplaceAddress, void* userData);
            static void Destroy(void* objectPtr, bool isFreeMemory, void* userData);
            static void* Clone(void* targetAddress, void* sourceAddress, void* userData);
        };

        template<class Handler>
        struct BehaviorEBusHandlerFactory
        {
            static BehaviorEBusHandler* Create();
            static void Destroy(BehaviorEBusHandler* handler);
        };

        template<class... Values>
        class BehaviorValuesSpecialization : public BehaviorValues
        {
        public:
            AZ_CLASS_ALLOCATOR(BehaviorValuesSpecialization, AZ::SystemAllocator, 0);

            template<class LastValue>
            inline void SetValues(LastValue&& value)
            {
                m_values[sizeof...(Values)-1].StoreInTempData(AZStd::forward<LastValue>(value));
            }

            template<class CurrentValue, class... RestValues>
            inline void SetValues(CurrentValue&& value, RestValues&&... values)
            {
                m_values[sizeof...(Values)-sizeof...(RestValues)-1].StoreInTempData(AZStd::forward<CurrentValue>(value));
                SetValues(AZStd::forward<RestValues>(values)...);
            };

            BehaviorValuesSpecialization(Values&&... values)
            {
                SetValues(values...);
            }

            size_t GetNumValues() const override
            {
                return sizeof...(Values);
            }
            const BehaviorValueParameter& GetValue(size_t i) const override
            {
                AZ_Assert(i < sizeof...(Values), "Invalid value index!");
                return m_values[i];
            }

            BehaviorValueParameter m_values[sizeof...(Values)];
        };

        template<class Owner>
        class GenericAttributes
        {
        protected:
            template<class T>
            void SetAttributeContextData(T value, AZ::Attribute* attribute, const AZStd::false_type& /* is_function<remove_pointer<T>::type> && is_member_function_pointer<T>*/)
            {
                (void)value; (void)attribute;
            }

            template<class T>
            void SetAttributeContextData(T value, AZ::Attribute* attribute, const AZStd::true_type& /* is_function<remove_pointer<T>::type> && is_member_function_pointer<T>*/);

            GenericAttributes(BehaviorContext* context)
                : m_currentAttributes(nullptr)
                , m_context(context)
            {
            }
        public:

            /**
            * All T (attribute value) MUST be copy constructible as they are stored in internal
            * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
            * Attributes can be assigned to either classes or DataElements.
            */
            template<class T>
            Owner* Attribute(const char* id, T value)
            {
                return Attribute(Crc32(id), value);
            }

            /**
            * All T (attribute value) MUST be copy constructible as they are stored in internal
            * AttributeContainer<T>, which can be accessed by azrtti and AttributeData.
            * Attributes can be assigned to either classes or DataElements.
            */
            template<class T>
            Owner* Attribute(Crc32 idCrc, T value);

            AttributeArray* m_currentAttributes;
            BehaviorContext* m_context;
        };
    } // namespace Internal

    /**
     * Create a container of default values to be used with methods. Default values are stored by value
     * in a temp storage, so currently there is limit the \ref BehaviorValueParameter temp storage, we
     * can easily change that if it became a problem.
     */
    template<class... Values>
    inline BehaviorValues* BehaviorMakeDefaultValues(Values&&... values)
    {
        return aznew Internal::BehaviorValuesSpecialization<Values...>(AZStd::forward<Values>(values)...);
    }

    /**
     * Internal helpers to bind fields to properties.
     * See: \ref BehaviorValueGetter, \ref BehaviorValueSetter, and \ref BehaviorValueProperty
     */
    namespace Internal
    {
        template<class T>
        struct BehaviorValuePropertyHelper;

        template<class T>
        struct BehaviorValuePropertyHelper<T*>
        {
            template<T* Value>
            static T& Get()
            {
                return *Value;
            }

            template<T* Value>
            static void Set(T v)
            {
                *Value = v;
            }
        };

        template<class T, class C>
        struct BehaviorValuePropertyHelper<T C::*>
        {
            template<T C::* Member>
            static T& Get(C* thisPtr)
            {
                return thisPtr->*Member;
            }

            template<T C::* Member>
            static void Set(C* thisPtr, const T& v)
            {
                thisPtr->*Member = v;
            }
        };
    } // namespace Internal

    /**
     * Behavior representation of reflected class. 
     */
    class BehaviorClass
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorClass, SystemAllocator, 0);

        BehaviorClass();      
        ~BehaviorClass();

        /// Hooks to override default memory allocation for the class (AZ_CLASS_ALLOCATOR is used by default)
        using AllocateType = void*(*)(void* userData);
        using DeallocateType = void(*)(void* address, void* userData);
        /// Default constructor and destructor custom function
        using DefaultConstructorType = void(*)(void* address, void* userData);
        using DestructorType = void(*)(void* objectPtr, void* userData);
        /// Clone object over an existing address
        using CopyContructorType = void(*)(void* address, const void* sourceObjectPtr, void* userData);
        /// Move object over an existing address
        using MoveContructorType = void(*)(void* address, void* sourceObjectPtr, void* userData);

        // Create the object with default constructor if possible otherwise returns an invalid object
        BehaviorObject Create() const;

        // TODO: Should we do that or just ask users to Allocate and invoke the constructor manually ?
        //template<class... Args>
        //BehaviorObject Create();

        BehaviorObject Clone(const BehaviorObject& object) const;
        BehaviorObject Move(BehaviorObject&& object) const;

        void Destroy(const BehaviorObject& object) const;

        /// Allocate a class, NO CONSTRUCTOR is called, only memory is allocated, call \ref Construct or use Create to allocate and create the object
        void* Allocate() const;
        /// Deallocate a class, NO DESTRUCTOR is called, only memory is free, call \ref Destruct or use Destroy to destroy and free the object
        void  Deallocate(void* address) const;

        AllocateType m_allocate;
        DeallocateType m_deallocate;
        DefaultConstructorType m_defaultConstructor;
        AZStd::vector<BehaviorMethod*> m_constructors; // signatures are (address, Params...)
        DestructorType m_destructor;
        CopyContructorType m_cloner;
        MoveContructorType m_mover;

        void* m_userData;
        AZStd::string m_name;
        AZStd::vector<AZ::Uuid> m_baseClasses;  
        AZStd::unordered_map<AZStd::string, BehaviorMethod*> m_methods;
        AZStd::list<BehaviorMethod*> m_attributeMethods;
        AZStd::unordered_map<AZStd::string, BehaviorProperty*> m_properties;
        AttributeArray m_attributes;
        AZStd::unordered_set<OnDemandReflectionFunction> m_onDemandReflection; /// Store the on demand reflect function so we can un-reflect when needed.
        AZStd::unordered_set<AZStd::string> m_requestBuses;
        AZStd::unordered_set<AZStd::string> m_notificationBuses;
        AZ::Uuid m_typeId;
        IRttiHelper* m_azRtti;
        size_t m_alignment;
        size_t m_size;
        BehaviorClassUnwrapperFunction m_unwrapper;
        void* m_unwrapperUserData;
        AZ::Uuid m_wrappedTypeId;
        // Store all owned instances for unload verification?
    };
        
    // Helper macros to generate getter and setter function from a pointer to value or member value
    // Syntax BehaviorValueGetter(&globalValue) BehaviorValueGetter(&Class::MemberValue)
#   define BehaviorValueGetter(valueAddress) &AZ::Internal::BehaviorValuePropertyHelper<decltype(valueAddress)>::Get<valueAddress>
#   define BehaviorValueSetter(valueAddress) &AZ::Internal::BehaviorValuePropertyHelper<decltype(valueAddress)>::Set<valueAddress>
#   define BehaviorValueProperty(valueAddress) BehaviorValueGetter(valueAddress),BehaviorValueSetter(valueAddress)

    // Constant helper
#   define BehaviorConstant(value) []() { return value; }

    /**
     * Property representation, a property has getter and setter. A read only property will have a "nullptr" for a setter.
     * You can use lamdas, global of member function. If you want to just expose a variable (not write the function and handle changes)
     * you can use \ref BehaviorValueProperty macros (or BehaviorValueGetter/Setter to control read/write functionality)
     * Member constants are a property too, use \ref BehaviorConstant for it. Everything is either a property or a method, the main reason 
     * why we "push" people to use functions is that in most cases when we manipulate an object, you will need to do more than just set a value
     * to a new value.
     */
    class BehaviorProperty
    {
        template<class Getter>
        bool SetGetter(Getter, BehaviorClass* /*currentClass*/, BehaviorContext* context, const AZStd::true_type& /* is AZStd::is_same<Getter,nullptr_t>::type() */);

        template<class Getter>
        bool SetGetter(Getter getter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type& /* is AZStd::is_same<Getter,nullptr_t>::type( */);

        template<class Setter>
        bool SetSetter(Setter, BehaviorClass*, BehaviorContext*, const AZStd::true_type& /* is AZStd::is_same<Getter,nullptr_t>::type() */);

        template<class Setter>
        bool SetSetter(Setter setter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type& /* is AZStd::is_same<Getter,nullptr_t>::type( */);

    public:
        AZ_CLASS_ALLOCATOR(BehaviorProperty, AZ::SystemAllocator, 0);

        BehaviorProperty();
        virtual ~BehaviorProperty();


        template<class Getter, class Setter>
        bool Set(Getter getter, Setter setter, BehaviorClass* currentClass, BehaviorContext* context);

        const AZ::Uuid& GetTypeId() const;

        AZStd::string m_name;
        BehaviorMethod* m_getter;
        BehaviorMethod* m_setter;
        AttributeArray m_attributes;
    };

    struct BehaviorEBusEventSender
    {
        template<class EBus, class Event>
        void Set(Event e, BehaviorContext* context);

        template<class EBus, class Event>
        void SetEvent(Event e, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/);

        template<class EBus, class Event>
        void SetEvent(Event e, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/);

        template<class EBus, class Event>
        void SetQueueBroadcast(Event e, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/);

        template<class EBus, class Event>
        void SetQueueBroadcast(Event e, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/);

        template<class EBus, class Event>
        void SetQueueEvent(Event e, BehaviorContext* context, const AZStd::true_type& /* is Queue and is BusId valid*/);

        template<class EBus, class Event>
        void SetQueueEvent(Event e, BehaviorContext* context, const AZStd::false_type& /* is Queue and is BusId valid*/);

        BehaviorMethod* m_broadcast = nullptr;
        BehaviorMethod* m_event = nullptr;
        BehaviorMethod* m_queueBroadcast = nullptr;
        BehaviorMethod* m_queueEvent = nullptr;
        AttributeArray m_attributes;
    };

    /**
     * EBus behavior wrapper.
     */
    class BehaviorEBus
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorEBus, SystemAllocator, 0);

        typedef void(*QueueFunctionType)(void* /*userData1*/, void* /*userData2*/);
    
        struct VirtualProperty
        {
            VirtualProperty(BehaviorEBusEventSender* getter, BehaviorEBusEventSender* setter)
                : m_getter(getter)
                , m_setter(setter)
            {}

            BehaviorEBusEventSender* m_getter;
            BehaviorEBusEventSender* m_setter;
        };


        BehaviorEBus();
        ~BehaviorEBus();

        BehaviorMethod* m_createHandler;
        BehaviorMethod* m_destroyHandler;

        AZStd::string m_name;
        BehaviorMethod* m_queueFunction;
        BehaviorParameter m_idParam; /// Invalid if bus doesn't have ID (you can check the typeId for invalid)
        BehaviorMethod* m_getCurrentId; ///< Method that returns current ID of the message, null if this EBus has not ID.
        AZStd::unordered_map<AZStd::string, BehaviorEBusEventSender> m_events;
        AZStd::unordered_map<AZStd::string, VirtualProperty> m_virtualProperties;
        AZStd::unordered_set<OnDemandReflectionFunction> m_onDemandReflection; /// Store the on demand reflect function so we can un-reflect when needed.
        AttributeArray m_attributes;
    };

    enum eBehaviorBusForwarderEventIndices
    {
        Result,
        UserData,
        ParameterFirst,
        Count
    };

    class BehaviorEBusHandler
    {
    public:
        AZ_RTTI(BehaviorEBusHandler, "{10fbcb9d-8a0d-47e9-8a51-cbd9bfbbf60d}");

        // Since we can share hooks we should probably pass the event name
        typedef void(*GenericHookType)(void* /*userData*/, const char* /*eventName*/, int /*eventIndex*/, BehaviorValueParameter* /*result*/, int /*numParameters*/, BehaviorValueParameter* /*parameters*/);

        struct BusForwarderEvent
        {
            BusForwarderEvent()
                : m_name(nullptr)
                , m_function(nullptr)
                , m_isFunctionGeneric(false)
                , m_userData(nullptr)
            {
            }

            const char* m_name;
            void* m_function; ///< Pointer user handler R Function(userData, Args...)
            bool m_isFunctionGeneric;
            void* m_userData;

            /// \note, even if this function returns no result, the first parameter is STILL space for the result
            bool HasResult() const;

            AZStd::vector<BehaviorParameter> m_parameters; // result, userdata, arguments...
        };

        typedef AZStd::vector<BusForwarderEvent> EventArray;

        BehaviorEBusHandler() {}

        virtual ~BehaviorEBusHandler() {}

        virtual int GetFunctionIndex(const char* name) const = 0;

        template<class BusId>
        bool Connect(BusId id)
        {
            BehaviorValueParameter p(&id);
            return Connect(&p);
        }

        virtual bool Connect(BehaviorValueParameter* id = nullptr) = 0;

        virtual void Disconnect() = 0;

        //const AZ::Uuid* GetIdTypeId() const = 0;

        template<class Hook>
        bool InstallHook(int index, Hook h, void* userData = nullptr);

        template<class Hook>
        bool InstallHook(const char* name, Hook h, void* userData = nullptr);

        bool InstallGenericHook(int index, GenericHookType hook, void* userData = nullptr);

        bool InstallGenericHook(const char* name, GenericHookType hook, void* userData = nullptr);

        const EventArray& GetEvents() const;

    protected:
        template<class Event>
        void SetEvent(Event e, const char* name);

        template<class... Args>
        void Call(int index, Args&&... args);

        template<class R, class... Args>
        void CallResult(R& result, int index, Args&&... args);

        EventArray m_events;
    };

    // Behavior context events you can listen for
    class BehaviorContextEvents : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusInterface settings
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef BehaviorContext* BusIdType;
        //////////////////////////////////////////////////////////////////////////

        /// Called when a new global method is reflected in behavior context or removed from it
        virtual void OnAddGlobalMethod(const char* methodName, BehaviorMethod* method)      { (void)methodName; (void)method; }
        virtual void OnRemoveGlobalMethod(const char* methodName, BehaviorMethod* method)   { (void)methodName; (void)method; }

        /// Called when a new global property is reflected in behavior context or remove from it
        virtual void OnAddGlobalProperty(const char* propertyName, BehaviorProperty* prop)  { (void)propertyName; (void)prop; }
        virtual void OnRemoveGlobalProperty(const char* propertyName, BehaviorProperty* prop) { (void)propertyName; (void)prop; }

        /// Called when a class is added or removed 
        virtual void OnAddClass(const char* className, BehaviorClass* behaviorClass)    { (void)className; (void)behaviorClass; }
        virtual void OnRemoveClass(const char* className, BehaviorClass* behaviorClass) { (void)className; (void)behaviorClass; }

        /// Called when a ebus is added or removed
        virtual void OnAddEBus(const char* ebusName, BehaviorEBus* ebus)    { (void)ebusName; (void)ebus; }
        virtual void OnRemoveEBus(const char* ebusName, BehaviorEBus* ebus) { (void)ebusName; (void)ebus; }
    };

    using BehaviorContextBus = AZ::EBus<BehaviorContextEvents>;

    /**
     * BehaviorContext is used to reflect classes, methods and EBuses for runtime interaction. A typical consumer of this context and different 
     * scripting systems (i.e. Lua, Visual Script, etc.). Even though (as designed) there are overlaps between some context they have very different
     * purpose and set of rules. For example SerializeContext, doesn't reflect any methods, it just reflects data fields that will be stored for initial object
     * setup, it handles version conversion and so thing, this related to storing the object to a persistent storage. Behavior context, doesn't need to deal with versions as 
     * no data is stored, just methods for manipulating the object state.
     */
    class BehaviorContext : public ReflectContext
    {
        template<class Bus, typename AZStd::enable_if<!AZStd::is_same<typename Bus::BusIdType, AZ::NullBusId>::value>::type* = nullptr>
        void EBusSetIdFeatures(BehaviorEBus* ebus)
        {
            Internal::SetParameters<typename Bus::BusIdType>(&ebus->m_idParam);
            ebus->m_getCurrentId = aznew Internal::BehaviorMethodImpl<const typename Bus::BusIdType*()>(&Bus::GetCurrentBusId, this, AZStd::string(Bus::GetName()) + "::GetCurrentBusId");
        }

        template<class Bus, typename AZStd::enable_if<AZStd::is_same<typename Bus::BusIdType, AZ::NullBusId>::value>::type* = nullptr>
        void EBusSetIdFeatures(BehaviorEBus*)
        {
        }

        template<class Bus>
        static void QueueFunction(BehaviorEBus::QueueFunctionType f, void* userData1, void* userData2) 
        {
            Bus::QueueFunction(f, userData1, userData2);
        }

        template<class Bus, typename AZStd::enable_if<Bus::Traits::EnableEventQueue>::type* = nullptr>
        BehaviorMethod* QueueFunctionMethod()
        {
            return aznew Internal::BehaviorMethodImpl<void(BehaviorEBus::QueueFunctionType, void*, void*)>(&QueueFunction<Bus>, this, AZStd::string(Bus::GetName()) + "::QueueFunction");
        }

        template<class Bus, typename AZStd::enable_if<!Bus::Traits::EnableEventQueue>::type* = nullptr>
        BehaviorMethod* QueueFunctionMethod()
        {
            return nullptr;
        }

        /// Helper struct to call the default class allocator \ref AZ_CLASS_ALLOCATOR
        template<class T>
        struct DefaultAllocator
        {
            static void* Allocate(void* userData)
            {
                (void)userData;
                return T::AZ_CLASS_ALLOCATOR_Allocate();
            }

            static void DeAllocate(void* address, void* userData)
            {
                (void)userData;
                T::AZ_CLASS_ALLOCATOR_DeAllocate(address);
            }
        };

        /// Helper struct to pipe allocation to the system allocator
        template<class T>
        struct DefaultSystemAllocator
        {
            static void* Allocate(void* userData)
            {
                (void)userData;
                return azmalloc(sizeof(T), AZStd::alignment_of<T>::value, AZ::SystemAllocator, AZ::AzTypeInfo<T>::Name());
            }

            static void DeAllocate(void* address, void* userData)
            {
                (void)userData;
                azfree(address, AZ::SystemAllocator, sizeof(T), AZStd::alignment_of<T>::value);
            }
        };

        /// Helper function to call default constructor
        template<class T>
        static void DefaultConstruct(void* address, void* userData)
        {
            (void)userData;
            new(address) T();
        }

        /// Helper function to call generic constructor
        template<class T, class... Params>
        static void Construct(T* address, Params... params)
        {
            new(address) T(params...);
        }

        /// Helper function to destroy an object
        template<class T>
        static void DefaultDestruct(void* object, void* userData)
        {
            (void)userData; (void)object;
            reinterpret_cast<T*>(object)->~T();
        }

        /// Helper functor to default copy construct
        template<class T>
        static void DefaultCopyConstruct(void* address, const void* sourceObject, void* userData)
        {
            (void)userData;
            new(address) T(*reinterpret_cast<const T*>(sourceObject));
        }

        /// Helper functor to default move construct
        template<class T>
        static void DefaultMoveConstruct(void* address, void* sourceObject, void* userData)
        {
            (void)userData;
            new(address) T(AZStd::move(*reinterpret_cast<T*>(sourceObject)));
        }

        template<class T>
        static void SetClassDefaultAllocator(BehaviorClass* behaviorClass, const AZStd::false_type& /*HasAZClassAllocator<T>*/) 
        { 
            behaviorClass->m_allocate = &DefaultSystemAllocator<T>::Allocate;
            behaviorClass->m_deallocate = &DefaultSystemAllocator<T>::DeAllocate;
        }

        template<class T>
        static void SetClassDefaultConstructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_constructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultDestructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_destructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultCopyConstructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_copy_constructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultMoveConstructor(BehaviorClass* behaviorClass, const AZStd::false_type& /*AZStd::is_move_constructible<T>*/) { (void)behaviorClass; }

        template<class T>
        static void SetClassDefaultAllocator(BehaviorClass* behaviorClass, const AZStd::true_type&  /*HasAZClassAllocator<T>*/)
        {
            behaviorClass->m_allocate = &DefaultAllocator<T>::Allocate;
            behaviorClass->m_deallocate = &DefaultAllocator<T>::DeAllocate;
        }

        template<class T>
        static void SetClassDefaultConstructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_constructible<T>*/) 
        {
            behaviorClass->m_defaultConstructor = &DefaultConstruct<T>;
        }

        template<class T>
        static void SetClassDefaultDestructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_destructible<T>*/) 
            {
                behaviorClass->m_destructor = &DefaultDestruct<T>;
            }

        template<class T>
        static void SetClassDefaultCopyConstructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_copy_constructible<T>*/) 
        { 
            behaviorClass->m_cloner = &DefaultCopyConstruct<T>;
        }

        template<class T>
        static void SetClassDefaultMoveConstructor(BehaviorClass* behaviorClass, const AZStd::true_type& /*AZStd::is_move_constructible<T>*/)
        {
            behaviorClass->m_mover = &DefaultMoveConstruct<T>;
        }

        template<class ClassType, class WrappedType, class MemberFunction>
        struct WrappedClassCaller
        {
            static void Unwrap(void* classPtr, void*& unwrappedClassPtr, AZ::Uuid& unwrappedClassTypeId, void* userData)
            {
                union
                {
                    void* userData;
                    MemberFunction memberFunctionPtr;
                } u;
                u.memberFunctionPtr = nullptr;

                u.userData = userData;
                unwrappedClassPtr = (void*)(reinterpret_cast<ClassType*>(classPtr)->*u.memberFunctionPtr)();
                unwrappedClassTypeId = AzTypeInfo<WrappedType>::Uuid();
            }
        };

    public:
        AZ_CLASS_ALLOCATOR(BehaviorContext, SystemAllocator, 0);
        AZ_RTTI(BehaviorContext, "{ED75FE05-9196-4F69-A3E5-1BDF5FF034CF}", ReflectContext);

        struct GlobalMethodInfo : public Internal::GenericAttributes<GlobalMethodInfo>
        {
            typedef Internal::GenericAttributes<GlobalMethodInfo> Base;
            GlobalMethodInfo(BehaviorContext* context, const char* methodName, BehaviorMethod* method);
            ~GlobalMethodInfo();
            GlobalMethodInfo* operator->();

            const char* m_name;
            BehaviorMethod* m_method;
        };
         
        struct GlobalPropertyInfo : public Internal::GenericAttributes<GlobalPropertyInfo>
        {
            typedef Internal::GenericAttributes<GlobalPropertyInfo> Base;

            GlobalPropertyInfo(BehaviorContext* context, BehaviorProperty* prop);
            ~GlobalPropertyInfo();
            GlobalPropertyInfo* operator->();

            BehaviorProperty* m_prop;
        };

        /// Internal structure to maintain class information while we are describing a class.
        template<class C>
        struct ClassReflection : public Internal::GenericAttributes<ClassReflection<C>>
        {
            typedef Internal::GenericAttributes<ClassReflection<C>> Base;

            //////////////////////////////////////////////////////////////////////////
            /// Internal implementation
            ClassReflection(BehaviorContext* context, BehaviorClass* behaviorClass);
            ~ClassReflection();
            ClassReflection* operator->();
          
            /**
            * Sets custom allocator for a class, this function will error if this not inside a class.
            * This is only for very specific cases when you want to override AZ_CLASS_ALLOCATOR or you are dealing with 3rd party classes, otherwise
            * you should use AZ_CLASS_ALLOCATOR to control which allocator the class uses.
            */
            ClassReflection* Allocator(BehaviorClass::AllocateType allocate, BehaviorClass::DeallocateType deallocate);

            /// Attaches different constructor signatures to the class.
            template<class... Params>
            ClassReflection* Constructor();

            /// When your class is a wrapper, like smart pointers, you should use this to describe how to unwrap the class.
            template<class WrappedType>
            ClassReflection* Wrapping(BehaviorClassUnwrapperFunction unwrapper, void* userData);

            /// Provide a function to unwrap this class (use an underlaying class)
            template<class WrappedType, class MemberFunction>
            ClassReflection* WrappingMember(MemberFunction memberFunction);

            /// Sets userdata to a class.
            ClassReflection* UserData(void *userData);

            template<class Function>
            ClassReflection* Method(const char* name, Function, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

            template<class Function, class = AZStd::enable_if_t<AZStd::function_traits<Function>::num_args != 0>>
            ClassReflection* Method(const char* name, Function, AZStd::array<AZStd::string, AZStd::function_traits<Function>::num_args>, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

            template<class Getter, class Setter>
            ClassReflection* Property(const char* name, Getter getter, Setter setter);

            /// All enums are treated as ints
            template<int Value>
            ClassReflection* Enum(const char* name);

            template<class Getter>
            ClassReflection* Constant(const char* name, Getter getter);

            /**
             * You can describe buses that this class uses to communicate. Those buses will be used by tools when 
             * you need to give developers hints as to what buses this class interacts with.
             * You don't need to reflect all buses that your class uses, just the ones related to 
             * class behavior. Please refer to component documentation for more information on
             * the pattern of Request and Notification buses.
             * {@
             */
            ClassReflection* RequestBus(const char* busName);
            ClassReflection* NotificationBus(const char* busName);
            // @}


            BehaviorClass* m_class;
        };

        /// Internal structure to maintain EBus information while describing it.
        template<typename Bus>
        struct EBusReflection : public Internal::GenericAttributes<EBusReflection<Bus>>
        {
            typedef Internal::GenericAttributes<EBusReflection<Bus>> Base;

            EBusReflection(BehaviorContext* context, BehaviorEBus* behaviorEBus);
            ~EBusReflection();
            EBusReflection* operator->();

            /**
            * Reflects an EBus event, valid only when used with in the context of an EBus reflection.
            * We will automatically add all possible variations (Broadcast,Event,QueueBroadcast and QueueEvent)
            */
            template<class Function, class = AZStd::enable_if_t<AZStd::function_traits<Function>::num_args != 0>>
            EBusReflection* Event(const char* name, Function, AZStd::array<AZStd::string, AZStd::function_traits<Function>::num_args>);

            template<class Function>
            EBusReflection* Event(const char* name, Function);

            /**
            * Every \ref EBus has two components, sending and receiving. Sending is reflected via \ref BehaviorContext::Event calls and Handler/Received
            * use handled via the receiver class. This class will receive EBus events and forward them to the behavior context functions.
            * Since we can't write a class (without using a codegen) while reflecting, you will need to implement that class
            * with the help of \ref AZ_EBUS_BEHAVIOR_BINDER. This class in mandated because you can't hook to individual events at the moment.
            * In this version of Handler you can provide a custom function to create and destroy that handler (usually where aznew/delete is not
            * applicable or you have a better pooling schema that our memory allocators already have). For most cases use the function \ref
            * Handler()
            */
            template<typename HandlerCreator, typename HandlerDestructor >
            EBusReflection* Handler(HandlerCreator creator, HandlerDestructor destructor);

            /** Set the Handler/Receiver for ebus evens that will be forwarded to BehaviorFunctions. This is a helper implementation where aznew/delete
            * is ready to called on the handler.
            */
            template<class H>
            EBusReflection* Handler();

            /**
             * With request buses (please refer to component communication patterns documentation) we ofter have EBus events
             * that represent a getter and a setter for a value. To allow our tools to take advantage of it, you can reflect 
             * VirtualProperty to indicate which event is the getter and which is the setter.
             * This function validates that getter event has no argument and a result and setter function has no results and only
             * one argument which is the same type as the result of the getter. 
             * \note Make sure you call this function after you have reflected the getter and setter events as it will report an error
             * if we can't find the function
             */
            EBusReflection* VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent);

            BehaviorEBus* m_ebus;
        };

         BehaviorContext();
        ~BehaviorContext();
     
        template<class Function>
        GlobalMethodInfo Method(const char* name, Function f, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        template<class Function, class = AZStd::enable_if_t<AZStd::function_traits<Function>::num_args != 0>>
        GlobalMethodInfo Method(const char* name, Function f, AZStd::array<AZStd::string, AZStd::function_traits<Function>::num_args>, BehaviorValues* defaultValues = nullptr, const char* dbgDesc = nullptr);

        template<class Getter, class Setter>
        GlobalPropertyInfo Property(const char* name, Getter getter, Setter setter);

        /// All enums are treated as ints
        template<int Value>
        BehaviorContext* Enum(const char* name);

        template<class Getter>
        BehaviorContext* Constant(const char* name, Getter getter);

        template<class T>
        ClassReflection<T> Class(const char* name = nullptr);

        template<class T>
        EBusReflection<T> EBus(const char* name);

        // TODO: This is only for searching by string, do we even need that?
        //ClassReflection< OpenNamespace(const char* name);
        //ClassReflection<T> CloneNamespace();

        AZStd::unordered_map<AZStd::string, BehaviorMethod*> m_methods; // TODO: make it a set and use the name inside method
        AZStd::list<BehaviorMethod*> m_attributeMethods; // Methods that we generate for attribute generic calling
        AZStd::unordered_map<AZStd::string, BehaviorProperty*> m_properties; // TODO: make it a set and use the name inside property
        AZStd::unordered_map<AZStd::string, BehaviorClass*> m_classes; // TODO: make it a set and use the name inside class
        AZStd::unordered_map<AZ::Uuid, BehaviorClass*> m_typeToClassMap; // TODO: make it a set and use the UUID inside the class
        AZStd::unordered_map<AZStd::string, BehaviorEBus*> m_ebuses; // TODO: make it a set and use the name inside EBus
        AZStd::unordered_map<AZ::Uuid,OnDemandReflectionFunction> m_onDemandReflection; /// Store the on demand reflect function so we can un-reflect when needed.

        AZStd::unordered_map<AZ::Uuid, OnDemandReflectionFunction> m_toProcessOnDemandReflection;
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    namespace BehaviorContextHelper
    {
        template <typename T>
        AZ::BehaviorClass* GetClass(BehaviorContext* behaviorContext)
        {
            return GetClass(behaviorContext, AZ::AzTypeInfo<T>::Uuid());
        }

        AZ::BehaviorClass* GetClass(BehaviorContext* behaviorContext, const AZ::Uuid& id);
    }

    //////////////////////////////////////////////////////////////////////////

/**
 * Helper MACRO to help you write the EBus handler that you want to reflect to behavior. This is not required, but generally we recommend reflecting all useful
 * buses as this enable people to "script" complex behaviors. 
 * You don't have to use this macro to write a Handler, but some people find it useful
 * Here is an example how to use it:
 * class MyEBusBehaviorHandler : public MyEBus::Handler, public AZ::BehaviorEBusHandler
 * {
 * public:
 *      AZ_EBUS_BEHAVIOR_BINDER(MyEBusBehaviorHandler, "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXXXX}",Allocator, OnEvent1, OnEvent2 and so on);     
 *      // now you need implementations for those event
 *
 *
 *      int OnEvent1(int data) override
 *      {
 *          // do any conversion of caching of the "data" here and forward this to behavior (often the reason for this is that you can't pass everything to behavior
 *          // plus behavior can't really handle all constructs pointer to pointer, rvalues, etc. as they don't make sense for most script environments
 *          int result = 0; // set the default value for your result if the behavior if there is no implmentation
 *          // The AZ_EBUS_BEHAVIOR_BINDER defines FN_EventName for each index. You can also cache it yourself (but it's slower), static int cacheIndex = GetFunctionIndex("OnEvent1"); and use that .
 *          CallResult(result, FN_OnEvent1, data);  // forward to the binding (there can be none, this is why we need to always have properly set result, when there is one)
 *          return result; // return the result like you will in any normal EBus even with result
 *      } *      
 *      // handle the other events here
 * };
 *
 */
#define AZ_EBUS_BEHAVIOR_BINDER(_Handler,_Uuid,_Allocator,...)\
    AZ_CLASS_ALLOCATOR(_Handler,_Allocator,0)\
    AZ_RTTI(_Handler,_Uuid,AZ::BehaviorEBusHandler)\
    typedef _Handler ThisType;\
    enum {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_ENUM, AZ_EBUS_SEQ(__VA_ARGS__))\
        FN_MAX\
    };\
    int GetFunctionIndex(const char* functionName) const override {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_INDEX, AZ_EBUS_SEQ(__VA_ARGS__))\
        return -1;\
    }\
    void Disconnect() override {\
        BusDisconnect();\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_REG_EVENT, AZ_EBUS_SEQ(__VA_ARGS__))\
    }\
    bool Connect(AZ::BehaviorValueParameter* id = nullptr) override {\
        return AZ::Internal::EBusConnector<_Handler>::Connect(this, id);\
    }

#define AZ_EBUS_SEQ_1(_1) (_1)
#define AZ_EBUS_SEQ_2(_1,_2) AZ_EBUS_SEQ_1(_1) (_2)
#define AZ_EBUS_SEQ_3(_1,_2,_3) AZ_EBUS_SEQ_2(_1,_2) (_3)
#define AZ_EBUS_SEQ_4(_1,_2,_3,_4) AZ_EBUS_SEQ_3(_1,_2,_3) (_4)
#define AZ_EBUS_SEQ_5(_1,_2,_3,_4,_5) AZ_EBUS_SEQ_4(_1,_2,_3,_4) (_5)
#define AZ_EBUS_SEQ_6(_1,_2,_3,_4,_5,_6) AZ_EBUS_SEQ_5(_1,_2,_3,_4,_5) (_6)
#define AZ_EBUS_SEQ_7(_1,_2,_3,_4,_5,_6,_7) AZ_EBUS_SEQ_6(_1,_2,_3,_4,_5,_6) (_7)
#define AZ_EBUS_SEQ_8(_1,_2,_3,_4,_5,_6,_7,_8) AZ_EBUS_SEQ_7(_1,_2,_3,_4,_5,_6,_7) (_8)
#define AZ_EBUS_SEQ_9(_1,_2,_3,_4,_5,_6,_7,_8,_9) AZ_EBUS_SEQ_8(_1,_2,_3,_4,_5,_6,_7,_8) (_9)
#define AZ_EBUS_SEQ_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) AZ_EBUS_SEQ_9(_1,_2,_3,_4,_5,_6,_7,_8,_9) (_10)
#define AZ_EBUS_SEQ_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) AZ_EBUS_SEQ_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) (_11)
#define AZ_EBUS_SEQ_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) AZ_EBUS_SEQ_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) (_12)
#define AZ_EBUS_SEQ_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) AZ_EBUS_SEQ_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) (_13)
#define AZ_EBUS_SEQ_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) AZ_EBUS_SEQ_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) (_14)
#define AZ_EBUS_SEQ_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) AZ_EBUS_SEQ_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) (_15)
#define AZ_EBUS_SEQ_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) AZ_EBUS_SEQ_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) (_16)
#define AZ_EBUS_SEQ_17(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17) AZ_EBUS_SEQ_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) (_17)
#define AZ_EBUS_SEQ_18(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18) AZ_EBUS_SEQ_17(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17) (_18)
#define AZ_EBUS_SEQ_19(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19) AZ_EBUS_SEQ_18(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18) (_19)
#define AZ_EBUS_SEQ_20(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20) AZ_EBUS_SEQ_19(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19) (_20)
#define AZ_EBUS_SEQ_21(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21) AZ_EBUS_SEQ_20(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20) (_21)
#define AZ_EBUS_SEQ_22(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22) AZ_EBUS_SEQ_21(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21) (_22)
#define AZ_EBUS_SEQ_23(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23) AZ_EBUS_SEQ_22(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22) (_23)
#define AZ_EBUS_SEQ_24(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24) AZ_EBUS_SEQ_23(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23) (_24)
#define AZ_EBUS_SEQ_25(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25) AZ_EBUS_SEQ_24(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24) (_25)
#define AZ_EBUS_SEQ_26(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26) AZ_EBUS_SEQ_25(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25) (_26)
#define AZ_EBUS_SEQ_27(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27) AZ_EBUS_SEQ_26(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26) (_27)
#define AZ_EBUS_SEQ_28(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28) AZ_EBUS_SEQ_27(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27) (_28)
#define AZ_EBUS_SEQ_29(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29) AZ_EBUS_SEQ_28(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28) (_29)
#define AZ_EBUS_SEQ_30(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30) AZ_EBUS_SEQ_29(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29) (_30)
#define AZ_EBUS_SEQ_31(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31) AZ_EBUS_SEQ_30(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30) (_31)
#define AZ_EBUS_SEQ_32(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32) AZ_EBUS_SEQ_31(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31) (_32)
#define AZ_EBUS_SEQ_33(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33) AZ_EBUS_SEQ_32(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32) (_33)
#define AZ_EBUS_SEQ_34(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34) AZ_EBUS_SEQ_33(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33) (_34)
#define AZ_EBUS_SEQ_35(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35) AZ_EBUS_SEQ_34(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34) (_35)
#define AZ_EBUS_SEQ_36(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36) AZ_EBUS_SEQ_35(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35) (_36)
#define AZ_EBUS_SEQ_37(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37) AZ_EBUS_SEQ_36(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36) (_37)
#define AZ_EBUS_SEQ_38(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38) AZ_EBUS_SEQ_37(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37) (_38)
#define AZ_EBUS_SEQ_39(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39) AZ_EBUS_SEQ_38(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38) (_39)
#define AZ_EBUS_SEQ_40(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40) AZ_EBUS_SEQ_39(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_39) (_40)

#define AZ_EBUS_SEQ(...) AZ_MACRO_SPECIALIZE(AZ_EBUS_SEQ_,AZ_VA_NUM_ARGS(__VA_ARGS__),(__VA_ARGS__))

#define AZ_BEHAVIOR_EBUS_FUNC_ENUM(name)              AZ_BEHAVIOR_EBUS_FUNC_ENUM_I(name),
#define AZ_BEHAVIOR_EBUS_FUNC_ENUM_I(name)            FN_##name

#define AZ_BEHAVIOR_EBUS_FUNC_INDEX(name)             AZ_BEHAVIOR_EBUS_FUNC_INDEX_I(name);
#define AZ_BEHAVIOR_EBUS_FUNC_INDEX_I(name)           if(strcmp(functionName,#name)==0) return FN_##name

#define AZ_BEHAVIOR_EBUS_REG_EVENT(name)             AZ_BEHAVIOR_EBUS_REG_EVENT_I(name);
#define AZ_BEHAVIOR_EBUS_REG_EVENT_I(name)           SetEvent(&ThisType::name,#name);

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Template implementations
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////////////////////////////////////////////
    inline BehaviorObject::BehaviorObject()
        : m_address(nullptr)
        , m_typeId(AZ::Uuid::CreateNull())
    {
    }

    //////////////////////////////////////////////////////////////////////////
    inline BehaviorObject::BehaviorObject(void* address, const Uuid& typeId)
        : m_address(address)
        , m_typeId(typeId)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    inline bool BehaviorObject::IsValid() const
    {
        return m_address && !m_typeId.IsNull();
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    inline BehaviorValueParameter::BehaviorValueParameter()
        : m_value(nullptr)
    {
        m_name = nullptr;
        m_typeId = Uuid::CreateNull();
        m_azRtti = nullptr;
        m_traits = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    inline BehaviorValueParameter::BehaviorValueParameter(T* value)
    {
        Set<T>(value);
    }

    //////////////////////////////////////////////////////////////////////////
    inline BehaviorValueParameter::BehaviorValueParameter(BehaviorObject* value)
    {
        Set(value);
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void BehaviorValueParameter::StoreInTempData(T&& value)
    {
        Internal::SetParameters<T>(this);
        m_value = m_tempData.allocate(sizeof(T), AZStd::alignment_of<T>::value, 0);
        *reinterpret_cast<AZStd::decay_t<T>*>(m_value) = AZStd::move(value);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(T* value)
    {
        Internal::SetParameters<AZStd::decay_t<T>>(this);
        m_value = (void*)value;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(BehaviorObject* value)
    {
        m_value = &value->m_address;
        m_typeId = value->m_typeId;
        m_traits = BehaviorParameter::TR_POINTER;
        m_name = nullptr;
        m_azRtti = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(const BehaviorParameter& param)
    {
        *static_cast<BehaviorParameter*>(this) = param;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void BehaviorValueParameter::Set(const BehaviorValueParameter& param)
    {
        *static_cast<BehaviorParameter*>(this) = static_cast<const BehaviorParameter&>(param);
        m_value = param.m_value;
        m_onAssignedResult = param.m_onAssignedResult;
        m_tempData = param.m_tempData;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE void* BehaviorValueParameter::GetValueAddress() const
    {
        void* valueAddress = m_value;
        if (m_traits & BehaviorParameter::TR_POINTER)
        {
            valueAddress = *reinterpret_cast<void**>(valueAddress); // pointer to a pointer
        }
        return valueAddress;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE BehaviorValueParameter::operator BehaviorObject() const
    {
        return BehaviorObject(m_value, m_typeId);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    AZ_FORCE_INLINE bool BehaviorValueParameter::ConvertTo()
    {
        return ConvertTo(AzTypeInfo<AZStd::decay_t<T>>::Uuid());
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE bool BehaviorValueParameter::ConvertTo(const AZ::Uuid& typeId)
    {
        if (m_azRtti)
        {
            void* valueAddress = GetValueAddress();
            if (valueAddress) // should we make null value to convert to anything?
            {
                return Internal::ConvertValueTo(valueAddress, m_azRtti, typeId, m_value, m_tempData);
            }
        }
        return m_typeId == typeId;
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    AZ_FORCE_INLINE AZStd::decay_t<T>*  BehaviorValueParameter::GetAsUnsafe() const
    {
        return reinterpret_cast<AZStd::decay_t<T>*>(m_value);
    }

    //////////////////////////////////////////////////////////////////////////

    template<class T>
    struct SetResult
    {
        static bool Set(BehaviorValueParameter& param, T& result, bool IsValueCopy)
        {
            using Type = AZStd::decay_t<T>;
            if (param.m_traits & BehaviorParameter::TR_POINTER)
            {
                *reinterpret_cast<void**>(param.m_value) = (Type*)&result;
                return true;
            }
            else if (param.m_traits & BehaviorParameter::TR_REFERENCE)
            {
                param.m_value = (Type*)&result;
                return true;
            }
            else if (IsValueCopy)
            {
                // value copy
                new(param.m_value) Type((Type)result);
                return true;
            }
            return false;
        }
    };

    template<class T>
    struct SetResult<T*>
    {
        using Type = AZStd::decay_t<T>;

        static bool ValueCopy(BehaviorValueParameter& param, T* result, const AZStd::true_type& /*AZStd::is_copy_constructible */)
        {
            new(param.m_value) Type(*result);
            return true;
        }

        static bool ValueCopy(BehaviorValueParameter&,  T* , const AZStd::false_type& /*AZStd::is_copy_constructible */)
        {
            return false;
        }

        static bool Set(BehaviorValueParameter& param, T* result, bool IsValueCopy)
        {
            if (param.m_traits & BehaviorParameter::TR_POINTER)
            {
                *reinterpret_cast<void**>(param.m_value) = (Type*)result;
                return true;
            }
            else if (param.m_traits & BehaviorParameter::TR_REFERENCE)
            {
                param.m_value = (Type*)result;
                return true;
            }
            else if (IsValueCopy)
            {
                // we need AZStd::is_complete so we can work with incomplete types, then we can enable the code below
                return false;

                //return ValueCopy(param, result, typename AZStd::conditional<AZStd::is_copy_constructible<Type>::value && !AZStd::is_abstract<Type>::value, AZStd::true_type, AZStd::false_type>::type());
            }
            return false;
        }
    };

    template<class T>
    struct SetResult<T*&> : public SetResult<T*>
    {
    };

    template<class T>
    struct SetResult<T&>
    {
        static bool Set(BehaviorValueParameter& param, T& result, bool IsValueCopy)
        {
            using Type = AZStd::decay_t<T>;
            if (param.m_traits & BehaviorParameter::TR_POINTER)
            {
                *reinterpret_cast<void**>(param.m_value) = (Type*)&result;
                return true;
            }
            else if (param.m_traits & BehaviorParameter::TR_REFERENCE)
            {
                param.m_value = (Type*)&result;
                return true;
            }
            else if (IsValueCopy)
            {
                new(param.m_value) Type((Type)result);
                return true;
            }
            return false;
        }
    };

    template<typename T>
    AZ_FORCE_INLINE BehaviorValueParameter& BehaviorValueParameter::operator=(T&& result)
    {
        StoreResult(AZStd::forward<T>(result));
        return *this;
    }

    template<typename T>
    AZ_FORCE_INLINE bool BehaviorValueParameter::StoreResult(T&& result)
    {
        using Type = AZStd::RemoveEnumT<AZStd::decay_t<T>>;

        const AZ::Uuid& typeId = AzTypeInfo<Type>::Uuid();

        bool isResult = false;

        if (m_typeId == typeId)
        {
            isResult = SetResult<T>::Set(*this, result, true);
        }
        else if (GetRttiHelper<Type>())
        {
            // try casting
            void* valueAddress = (void*)&result;
            if (m_traits & BehaviorParameter::TR_POINTER)
            {
                valueAddress = *reinterpret_cast<void**>(valueAddress); // pointer to a pointer
            }
            isResult = Internal::ConvertValueTo(valueAddress, GetRttiHelper<Type>(), m_typeId, m_value, m_tempData);
        }
        else if (m_typeId.IsNull()) // if nullptr we can accept any type, by pointer or reference
        {
            m_typeId = typeId;
            isResult = SetResult<T>::Set(*this, result, false);
        }

        if (isResult && m_onAssignedResult)
        {
            m_onAssignedResult();
        }

        return isResult;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template<class... Args>
    bool BehaviorMethod::Invoke(Args&&... args)
    {
        BehaviorValueParameter arguments[] = { &args... };
        return Call(arguments, sizeof...(Args), nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    AZ_FORCE_INLINE bool BehaviorMethod::Invoke()
    {
        return Call(nullptr, 0, nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class R, class... Args>
    bool BehaviorMethod::InvokeResult(R& r, Args&&... args)
    {
        if (!HasResult())
        {
            return false;
        }
        BehaviorValueParameter arguments[sizeof...(args)] = { &args... };
        BehaviorValueParameter result(&r);
        return Call(arguments, sizeof...(Args), &result);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class R>
    bool BehaviorMethod::InvokeResult(R& r)
    {
        if (!HasResult())
        {
            return false;
        }
        BehaviorValueParameter result(&r);
        return Call(nullptr, 0, &result);
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    bool BehaviorProperty::SetGetter(Getter, BehaviorClass* /*currentClass*/, BehaviorContext* /*context*/, const AZStd::true_type&)
    {
        m_getter = nullptr;
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    bool BehaviorProperty::SetGetter(Getter getter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type&)
    {
        typedef Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Getter>::type>::type> GetterType;
        m_getter = aznew GetterType(getter, context, AZStd::string::format("%s::%s::Getter", currentClass ? currentClass->m_name.c_str() : "", m_name.c_str()));

        if (AZStd::is_class<typename GetterType::ClassType>::value)
        {
            AZ_Assert(currentClass, "We should declare class property with in the class!");

            // check getter to have only return value (and this pointer)
            if (m_getter->GetNumArguments() != 1 || m_getter->GetArgument(0)->m_typeId != currentClass->m_typeId)
            {
                AZ_Assert(false, "Member Getter can't have any argument but thisPointer and just return type!");
                delete m_getter;
                m_getter = nullptr;
                return false;
            }
        }
        else
        {
            // check getter to have only return value
            if (m_getter->GetNumArguments() > 0)
            {
                if (currentClass && m_getter->GetNumArguments() == 1 && m_getter->GetArgument(0)->m_typeId == currentClass->m_typeId)
                {
                    // it's ok as this is a different way to represent a member function
                }
                else
                {
                    AZ_Assert(false, "Getter can't have any argument just return type: %s!", currentClass->m_name.c_str());
                    delete m_getter;
                    m_getter = nullptr;
                    return false;
                }
            }
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Setter>
    bool BehaviorProperty::SetSetter(Setter, BehaviorClass*, BehaviorContext*, const AZStd::true_type&)
    {
        m_setter = nullptr;
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Setter>
    bool BehaviorProperty::SetSetter(Setter setter, BehaviorClass* currentClass, BehaviorContext* context, const AZStd::false_type&)
    {
        typedef Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Setter>::type>::type> SetterType;
        m_setter = aznew SetterType(setter, context, AZStd::string::format("%s::%s::Setter", currentClass ? currentClass->m_name.c_str() : "", m_name.c_str()));
        if (AZStd::is_class<typename SetterType::ClassType>::value)
        {
            AZ_Assert(currentClass, "We should declare class property with in the class!");

            // check setter have only 1 argument + 1 this pointer
            if (m_setter->GetNumArguments() != 2 || m_setter->GetArgument(0)->m_typeId != currentClass->m_typeId)
            {
                AZ_Assert(false, "Member Setter should have 2 arguments, thisPointer and dataValue to be set!");
                delete m_setter;
                m_setter = nullptr;
                return false;
            }
            // check getter result type is equal to setter input type
            if (m_getter && m_getter->GetResult()->m_typeId != m_setter->GetArgument(1)->m_typeId)
            {
                AZ_Assert(false, "Getter return type and Setter input arguement should be the same type!");
                delete m_setter;
                m_setter = nullptr;
                return false;
            }
        }
        else
        {
            size_t valueIndex = 0;
            // check setter have only 1 argument
            if (m_setter->GetNumArguments() != 1)
            {
                if (currentClass && m_setter->GetNumArguments() == 2 && m_setter->GetArgument(0)->m_typeId == currentClass->m_typeId)
                {
                    // it's ok as this is a different way to represent a member function
                    valueIndex = 1; // since this pointer is at 0
                }
                else
                {
                    AZ_Assert(false, "Setter should have 1 argument, data value to be set!");
                    delete m_setter;
                    m_setter = nullptr;
                    return false;
                }
            }

            // check getter result type is equal to setter input type
            if (m_getter && m_getter->GetResult()->m_typeId != m_setter->GetArgument(valueIndex)->m_typeId)
            {
                AZ_Assert(false, "Getter return type and Setter input arguement should be the same type!");
                delete m_setter;
                m_setter = nullptr;
                return false;
            }
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter, class Setter>
    bool BehaviorProperty::Set(Getter getter, Setter setter, BehaviorClass* currentClass, BehaviorContext* context)
    {
        if (!SetGetter(getter, currentClass, context, typename AZStd::is_same<Getter, AZStd::nullptr_t>::type()))
        {
            return false;
        }

        if (!SetSetter(setter, currentClass, context, typename AZStd::is_same<Setter, AZStd::nullptr_t>::type()))
        {
            return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::Set(Event e, BehaviorContext* context)
    {
        m_broadcast = aznew Internal::BehaviorEBusEvent<EBus, Internal::BE_BROADCAST, typename AZStd::remove_pointer<Event>::type>(e, context);
        SetEvent<EBus>(e, context, typename AZStd::is_same<typename EBus::BusIdType, NullBusId>::type());
        SetQueueBroadcast<EBus>(e, context, typename AZStd::is_same<typename EBus::MessageQueuePolicy::BusMessageCall, typename AZ::Internal::NullBusMessageCall>::type());
        SetQueueEvent<EBus>(e, context, typename AZStd::conditional<AZStd::is_same<typename EBus::BusIdType, NullBusId>::value || AZStd::is_same<typename EBus::MessageQueuePolicy::BusMessageCall, typename AZ::Internal::NullBusMessageCall>::value, AZStd::true_type, AZStd::false_type>::type());
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetEvent(Event e, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/)
    {
        (void)e; (void)context;
        m_event = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetEvent(Event e, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/)
    {
        m_event = aznew Internal::BehaviorEBusEvent<EBus, Internal::BE_EVENT_ID, typename AZStd::remove_pointer<Event>::type>(e, context);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueBroadcast(Event e, BehaviorContext* context, const AZStd::true_type& /*is NullBusId*/)
    {
        (void)e; (void)context;
        m_queueBroadcast = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueBroadcast(Event e, BehaviorContext* context, const AZStd::false_type& /*!is NullBusId*/)
    {
        m_queueBroadcast = aznew Internal::BehaviorEBusEvent<EBus, Internal::BE_QUEUE_BROADCAST, typename AZStd::remove_pointer<Event>::type>(e, context);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueEvent(Event e, BehaviorContext* context, const AZStd::true_type& /* is Queue and is BusId valid*/)
    {
        (void)e; (void)context;
        m_queueEvent = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, class Event>
    void BehaviorEBusEventSender::SetQueueEvent(Event e, BehaviorContext* context, const AZStd::false_type& /* is Queue and is BusId valid*/)
    {
        m_queueEvent = aznew Internal::BehaviorEBusEvent<EBus, Internal::BE_QUEUE_EVENT_ID, typename AZStd::remove_pointer<Event>::type>(e, context);
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    template<class Hook>
    bool BehaviorEBusHandler::InstallHook(int index, Hook h, void* userData)
    {
        if (index != -1)
        {
            // Check parameters
            if (!Internal::SetFunctionParameters<typename AZStd::remove_pointer<Hook>::type>::Check(m_events[index].m_parameters))
            {
                return false;
            }

            m_events[index].m_isFunctionGeneric = false;
            m_events[index].m_function = reinterpret_cast<void*>(h);
            m_events[index].m_userData = userData;
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Hook>
    bool BehaviorEBusHandler::InstallHook(const char* name, Hook h, void* userData)
    {
        return InstallHook(GetFunctionIndex(name), h, userData);
    };

    //////////////////////////////////////////////////////////////////////////
    template<class Event>
    void BehaviorEBusHandler::SetEvent(Event e, const char* name)
    {
        (void)e;
        int i = GetFunctionIndex(name);
        if (i != -1)
        {
            m_events.resize(i + 1);
            m_events[i].m_name = name;
            m_events[i].m_function = nullptr;
            Internal::SetFunctionParameters<typename AZStd::remove_pointer<Event>::type>::Set(m_events[i].m_parameters);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class... Args>
    void BehaviorEBusHandler::Call(int index, Args&&... args)
    {
        BusForwarderEvent& e = m_events[index];
        if (e.m_function)
        {
            if (e.m_isFunctionGeneric)
            {
                BehaviorValueParameter arguments[sizeof...(args)+1] = { &args... };
                reinterpret_cast<GenericHookType>(e.m_function)(e.m_userData, e.m_name, index, nullptr, AZ_ARRAY_SIZE(arguments)-1, arguments);
            }
            else
            {
                typedef void(*FunctionType)(void*, Args...);
                reinterpret_cast<FunctionType>(e.m_function)(e.m_userData, args...);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
        
    template<class R, class... Args>
    void BehaviorEBusHandler::CallResult(R& result, int index, Args&&... args)
    {
        BusForwarderEvent& e = m_events[index];
        if (e.m_function)
        {
            if (e.m_isFunctionGeneric)
            {
                BehaviorValueParameter arguments[sizeof...(args)+1] = { &args... };
                BehaviorValueParameter r(&result);
                reinterpret_cast<GenericHookType>(e.m_function)(e.m_userData, e.m_name, index, &r, AZ_ARRAY_SIZE(arguments) - 1, arguments);
                // Assign on top of the the value if the param isn't a pointer
                // (otherwise the pointer just gets overridden and no value is returned).
                if ((r.m_traits & BehaviorParameter::TR_POINTER) == 0 && r.GetAsUnsafe<R>())
                {
                    result = *r.GetAsUnsafe<R>();
                }
            }
            else
            {
                typedef R(*FunctionType)(void*, Args...);
                result = reinterpret_cast<FunctionType>(e.m_function)(e.m_userData, args...);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::ClassReflection<T> BehaviorContext::Class(const char* name)
    {
        if (name == nullptr)
        {
            name = AzTypeInfo<T>::Name();
        }

        auto classTypeIt = m_typeToClassMap.find(AzTypeInfo<T>::Uuid());
        if (IsRemovingReflection())
        {
            if (classTypeIt != m_typeToClassMap.end())
            {
                // find it in the name category
                auto nameIt = m_classes.find(name);
                while (nameIt != m_classes.end())
                {
                    if (nameIt->second == classTypeIt->second)
                    {
                        m_classes.erase(nameIt);
                        break;
                    }
                }
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveClass, name, classTypeIt->second);
                delete classTypeIt->second;
                m_typeToClassMap.erase(classTypeIt);
            }
            return ClassReflection<T>(this, static_cast<BehaviorClass*>(nullptr));
        }
        else
        {
            if (classTypeIt != m_typeToClassMap.end())
            {
                // class already reflected, display name and uuid
                char uuidName[AZ::Uuid::MaxStringBuffer];
                classTypeIt->first.ToString(uuidName, AZ::Uuid::MaxStringBuffer);
                
                AZ_Error("Reflection", false, "Class '%s' is already registered using Uuid: %s!", name, uuidName);
                return ClassReflection<T>(this, static_cast<BehaviorClass*>(nullptr));
            }

            // TODO: make it a set and use the name inside the class
            if (m_classes.find(name) != m_classes.end())
            {
                AZ_Error("Reflection", false, "A class with name '%s' is already registered!", name);
                return ClassReflection<T>(this, static_cast<BehaviorClass*>(nullptr));
            }

            BehaviorClass* behaviorClass = aznew BehaviorClass();
            behaviorClass->m_typeId = AzTypeInfo<T>::Uuid();
            behaviorClass->m_azRtti = GetRttiHelper<T>();
            behaviorClass->m_alignment = AZStd::alignment_of<T>::value;
            behaviorClass->m_size = sizeof(T);
            behaviorClass->m_name = name;

            // enumerate all base classes (RTTI), we store only the IDs to allow for our of order reflection
            // At runtime it will be more efficient to have the pointers to the classes. Analyze in practice and cache them if needed.
            AZ::RttiEnumHierarchy<T>(
                [](const AZ::Uuid& typeId, void* userData)
                {
                    BehaviorClass* bc = reinterpret_cast<BehaviorClass*>(userData);
                    if (typeId != bc->m_typeId)
                    {
                        bc->m_baseClasses.push_back(typeId);
                    }
                }
                , behaviorClass
                );

            SetClassDefaultAllocator<T>(behaviorClass, typename HasAZClassAllocator<T>::type());
            SetClassDefaultConstructor<T>(behaviorClass, typename AZStd::conditional< AZStd::is_constructible<T>::value && !AZStd::is_abstract<T>::value, AZStd::true_type, AZStd::false_type>::type());
            SetClassDefaultDestructor<T>(behaviorClass, typename AZStd::is_destructible<T>::type());
            SetClassDefaultCopyConstructor<T>(behaviorClass, typename AZStd::conditional< AZStd::is_copy_constructible<T>::value && !AZStd::is_abstract<T>::value, AZStd::true_type, AZStd::false_type>::type());
            SetClassDefaultMoveConstructor<T>(behaviorClass, typename AZStd::conditional< AZStd::is_move_constructible<T>::value && !AZStd::is_abstract<T>::value, AZStd::true_type, AZStd::false_type>::type());

            // Switch to Set (we store the name in the class)
            m_classes.insert(AZStd::make_pair(behaviorClass->m_name, behaviorClass));
            m_typeToClassMap.insert(AZStd::make_pair(behaviorClass->m_typeId, behaviorClass));
            return ClassReflection<T>(this, behaviorClass);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassReflection<C>::ClassReflection(BehaviorContext* context, BehaviorClass* behaviorClass)
        : Internal::GenericAttributes<ClassReflection<C>>(context)
        , m_class(behaviorClass)
    {
        Base::m_currentAttributes = &behaviorClass->m_attributes;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassReflection<C>::~ClassReflection()
    {
        // process all on demand queued reflections
        decltype (Base::m_context->m_toProcessOnDemandReflection) toProcess(AZStd::move(Base::m_context->m_toProcessOnDemandReflection));
        for (auto toReflectIt : toProcess)
        {
            toReflectIt.second(Base::m_context);
        }

        if (!Base::m_context->IsRemovingReflection())
        {
            BehaviorContextBus::Event(Base::m_context, &BehaviorContextBus::Events::OnAddClass, m_class->m_name.c_str(), m_class);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::operator->()
    {
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class... Params>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Constructor()
    {
        AZ_Error("BehaviorContext", m_class, "You can set constructors only on valid classes!");
        if (m_class)
        {
            BehaviorMethod* constructor = aznew Internal::BehaviorMethodImpl<void(C* address, Params...)>(&Construct<C, Params...>, Base::m_context, AZStd::string::format("%s::Constructor", m_class->m_name.c_str()));
            m_class->m_constructors.push_back(constructor);
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class WrappedType>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Wrapping(BehaviorClassUnwrapperFunction unwrapper, void* userData)
    {
        AZ_Error("BehaviorContext", m_class, "You can wrap only valid classes!");
        if (m_class)
        {
            m_class->m_wrappedTypeId = AzTypeInfo<WrappedType>::Uuid();
            m_class->m_unwrapper = unwrapper;
            m_class->m_unwrapperUserData = userData;
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class WrappedType, class MemberFunction>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::WrappingMember(MemberFunction memberFunction)
    {
        union
        {
            MemberFunction memberFunction;
            void* userData;
        } u;
        u.memberFunction = memberFunction;
        return Wrapping<WrappedType>(&WrappedClassCaller<C, WrappedType, MemberFunction>::Unwrap, u.userData);
    }

    template<class Function, class>
    BehaviorContext::GlobalMethodInfo BehaviorContext::Method(const char* name, Function f, AZStd::array<AZStd::string, AZStd::function_traits<Function>::num_args> args, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        BehaviorContext::GlobalMethodInfo info = Method(name, f, defaultValues, dbgDesc);

        if (!args.empty())
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                info->m_method->SetArgumentName(i, args[i]);
            }
        }

        return info;
    }

    template<class Function>
    BehaviorContext::GlobalMethodInfo BehaviorContext::Method(const char* name, Function f, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        if (IsRemovingReflection())
        {
            auto globalMethodIt = m_methods.find(name);
            if (globalMethodIt != m_methods.end())
            {
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveGlobalMethod, name, globalMethodIt->second);
            }
            return GlobalMethodInfo(this, nullptr, nullptr);
        }

        AZ_Assert(!AZStd::is_member_function_pointer<Function>::value, "This is a member %s method declared as global! use script.Class<Type>(Name)->Method()->Value()!\n", name);
        typedef Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Function>::type>::type> BehaviorMethodType;
        BehaviorMethod* method = aznew BehaviorMethodType(f, this, name);
        method->m_debugDescription = dbgDesc;
        method->m_defaultArguments = defaultValues;
        if (defaultValues)
        {
            AZ_Assert(defaultValues->GetNumValues() <= method->GetNumArguments(), "You can't have more default values than the number of function arguments");
            // validate all default value types
            size_t startMethodIndex = method->GetNumArguments() - defaultValues->GetNumValues();
            for (size_t i = 0; i < defaultValues->GetNumValues(); ++i)
            {
                if (defaultValues->GetValue(i).m_typeId != method->GetArgument(startMethodIndex + i)->m_typeId)
                {
                    AZ_Assert(false, "Argument %d default value type, doesn't match! Default value should be the same type! Current type %s!", defaultValues->GetValue(i).m_name);
                    delete defaultValues;
                    delete method;
                    return GlobalMethodInfo(this, nullptr, nullptr);
                }
            }
        }

        // global method
        if (!m_methods.insert(AZStd::make_pair(name, method)).second)
        {
            AZ_Error("Reflection", false, "Method '%s' is already registered in the global context!", name);
            delete method;
            return GlobalMethodInfo(this, nullptr, nullptr);
        }

        return GlobalMethodInfo(this, name, method);
    }

    template<class C>
    template<class Function>
    inline BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Method(const char* name, Function f, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        if (m_class)
        {
            typedef Internal::BehaviorMethodImpl<typename AZStd::RemoveFunctionConst<typename AZStd::remove_pointer<Function>::type>::type> BehaviorMethodType;
            BehaviorMethod* method = aznew BehaviorMethodType(f, Base::m_context, AZStd::string::format("%s::%s", m_class->m_name.c_str(), name));
            method->m_debugDescription = dbgDesc;
            method->m_defaultArguments = defaultValues;
            if (defaultValues)
            {
                AZ_Assert(defaultValues->GetNumValues() <= method->GetNumArguments(), "You can't have more default values than the number of function arguments");
                // validate all default value types
                size_t startMethodIndex = method->GetNumArguments() - defaultValues->GetNumValues();
                for (size_t i = 0; i < defaultValues->GetNumValues(); ++i)
                {
                    if (defaultValues->GetValue(i).m_typeId != method->GetArgument(startMethodIndex + i)->m_typeId)
                    {
                        AZ_Assert(false, "Argument %d default value type, doesn't match! Default value should be the same type! Current type %s!", defaultValues->GetValue(i).m_name);
                        delete defaultValues;
                        delete method;
                        return this;
                    }
                }
            }

            if (!m_class->m_methods.insert(AZStd::make_pair(name, method)).second)
            {
                // technically we can support multiple names with different signatures, however this will make binding to script
                // possible conversions of addressed trickier, so let's delay it till we have more data if we actually need it. Same it 
                // true for the global method insert above
                AZ_Error("Reflection", false, "Class '%s' already have Method '%s' reflected!", m_class->m_name.c_str(), name);
                delete method;
                return this;
            }

            // \note we can start returning a context so we can maintain the scope
            Base::m_currentAttributes = &method->m_attributes;
        }

        return this;
    }

    template<class C>
    template<class Function, class>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Method(const char* name, Function f, AZStd::array<AZStd::string, AZStd::function_traits<Function>::num_args> args, BehaviorValues* defaultValues, const char* dbgDesc)
    {
        BehaviorContext::ClassReflection<C>* classReflection = Method(name, f, defaultValues, dbgDesc);
        auto methodIterator = classReflection->m_class->m_methods.find(name);
        if (methodIterator != classReflection->m_class->m_methods.end())
        {
            if (!args.empty())
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    methodIterator->second->SetArgumentName(i, args[i]);
                }
            }
        }

        if (args.empty())
        {
            AZ_TracePrintf("BehaviorContext", "Method: %s does not have named attributes\n", name);
        }

        return classReflection;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class Getter, class Setter>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Property(const char* name, Getter getter, Setter setter)
    {
        if (m_class)
        {
            BehaviorProperty* prop = aznew BehaviorProperty;
            prop->m_name = name;
            if (!prop->Set(getter, setter, m_class, Base::m_context))
            {
                delete prop;
                return this;
            }

            m_class->m_properties.insert(AZStd::make_pair(name, prop));

            // \note we can start returning a context so we can maintain the scope
            Base::m_currentAttributes = &prop->m_attributes;
        }

        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<int Value>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Enum(const char* name)
    {
        Property(name, []() { return Value; }, nullptr);
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    template<class Getter>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Constant(const char* name, Getter getter)
    {
        Property(name, getter, nullptr);
        return this;
    };


    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::RequestBus(const char* name)
    {
        if (m_class)
        {
            m_class->m_requestBuses.insert(name);
        }
        return this;
    };


    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::NotificationBus(const char* name)
    {
        if (m_class)
        {
            m_class->m_notificationBuses.insert(name);
        }
        return this;
    };

    //////////////////////////////////////////////////////////////////////////
    template<class C>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::Allocator(BehaviorClass::AllocateType allocate, BehaviorClass::DeallocateType deallocate)
    {
        AZ_Error("BehaviorContext", m_class, "Allocator can be set on valid classes only!");
        if (m_class)
        {
            m_class->m_allocate = allocate;
            m_class->m_deallocate = deallocate;
        }
        return this;
    }

    template<class C>
    BehaviorContext::ClassReflection<C>* BehaviorContext::ClassReflection<C>::UserData(void *userData)
    {
        AZ_Error("BehaviorContext", m_class, "UserData can be set on valid classes only!");
        if (m_class)
        {
            m_class->m_userData = userData;
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter, class Setter>
    BehaviorContext::GlobalPropertyInfo BehaviorContext::Property(const char* name, Getter getter, Setter setter)
    {
        if (IsRemovingReflection())
        {
            auto globalPropIt = m_properties.find(name);
            if (globalPropIt != m_properties.end())
            {
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveGlobalProperty, name, globalPropIt->second);
            }
            return GlobalPropertyInfo(this, nullptr);
        }

        AZ_Assert(!AZStd::is_member_function_pointer<Getter>::value, "Getter for %s is a member method! script.Class<Type>(Name)->Property()!\n", name);
        AZ_Assert(!AZStd::is_member_function_pointer<Setter>::value, "Setter for %s is a member method! script.Class<Type>(Name)->Property()!\n", name);

        BehaviorProperty* prop = aznew BehaviorProperty;
        prop->m_name = name;
        if (!prop->Set(getter, setter, nullptr, this))
        {
            delete prop;
            return GlobalPropertyInfo(this, nullptr);
        }

        m_properties.insert(AZStd::make_pair(name, prop));

        return GlobalPropertyInfo(this, prop);
    }

    //////////////////////////////////////////////////////////////////////////
    template<int Value>
    BehaviorContext* BehaviorContext::Enum(const char* name)
    {
        Property(name, []() { return Value; }, nullptr);
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Getter>
    BehaviorContext* BehaviorContext::Constant(const char* name, Getter getter)
    {
        Property(name, getter, nullptr);
        return this;
    };

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusReflection<T>::EBusReflection(BehaviorContext* context, BehaviorEBus* ebus)
        : Internal::GenericAttributes<EBusReflection<T>>(context)
        , m_ebus(ebus)
    {
        Base::m_currentAttributes = &ebus->m_attributes;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusReflection<T>::~EBusReflection()
    {
        // process all on demand queued reflections
        decltype (Base::m_context->m_toProcessOnDemandReflection) toProcess(AZStd::move(Base::m_context->m_toProcessOnDemandReflection));
        for (auto toReflectIt : toProcess)
        {
            toReflectIt.second(Base::m_context);
        }

        if (!Base::m_context->IsRemovingReflection())
        {
            BehaviorContextBus::Event(Base::m_context, &BehaviorContextBus::Events::OnAddEBus, m_ebus->m_name.c_str(), m_ebus);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusReflection<T>* BehaviorContext::EBusReflection<T>::operator->()
    {
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class T>
    BehaviorContext::EBusReflection<T> BehaviorContext::EBus(const char* name)
    {
        // should we require AzTypeInfo for EBus, technically we should if we want to work around the compiler issue that made us to do it in first place
        if (IsRemovingReflection())
        {
            auto ebusIt = m_ebuses.find(name);
            if (ebusIt != m_ebuses.end())
            {
                BehaviorContextBus::Event(this, &BehaviorContextBus::Events::OnRemoveEBus, name, ebusIt->second);
                delete ebusIt->second;
                m_ebuses.erase(ebusIt);
            }
            return EBusReflection<T>(this, nullptr);
        }
        else
        {
            AZ_Error("BehaviorContext", m_ebuses.find(name) == m_ebuses.end(), "You shouldn't reflect an EBus multiple times (%s), subsequent reflections will not be registered!", name)
            BehaviorEBus* behaviorEBus = aznew BehaviorEBus();
            behaviorEBus->m_name = name;
            EBusSetIdFeatures<T>(behaviorEBus);
            behaviorEBus->m_queueFunction = QueueFunctionMethod<T>();

            // Switch to Set (we store the name in the class)
            m_ebuses.insert(AZStd::make_pair(behaviorEBus->m_name, behaviorEBus));
            return EBusReflection<T>(this, behaviorEBus);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    template<class Function, class>
    BehaviorContext::EBusReflection<Bus>* BehaviorContext::EBusReflection<Bus>::Event(const char* name, Function e, AZStd::array<AZStd::string, AZStd::function_traits<Function>::num_args> args)
    {
        BehaviorContext::EBusReflection<Bus>* ebusReflection = Event(name, e);
        auto eventIterator = ebusReflection->m_ebus->m_events.find(name);
        if (eventIterator != ebusReflection->m_ebus->m_events.end())
        {
            if (eventIterator->second.m_event)
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    eventIterator->second.m_event->SetArgumentName(i, args[i]);
                }
            }
        }

        if (args.empty())
        {
            AZ_TracePrintf("BehaviorContext", "Method: %s does not have named attributes\n", name);
        }

        return ebusReflection;
    }

    template<class Bus>
    template<class Function>
    BehaviorContext::EBusReflection<Bus>* BehaviorContext::EBusReflection<Bus>::Event(const char* name, Function e)
    {
        if (m_ebus)
        {
            BehaviorEBusEventSender ebusEvent;

            ebusEvent.Set<Bus>(e, Base::m_context);

            auto insertIt = m_ebus->m_events.insert(AZStd::make_pair(name, ebusEvent));
            AZ_Assert(insertIt.second, "Reflection inserted a duplicate event: '%s' for bus '%s' - please check that you are not reflecting the same event repeatedly.  This would cause memory leaks.", name, m_ebus->m_name.c_str());

            Base::m_currentAttributes = &insertIt.first->second.m_attributes;
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    template<typename HandlerCreator, typename HandlerDestructor >
    BehaviorContext::EBusReflection<Bus>* BehaviorContext::EBusReflection<Bus>::Handler(HandlerCreator creator, HandlerDestructor destructor)
    {
        if (m_ebus)
        {
            AZ_Assert(creator != nullptr && destructor != nullptr, "Both creator and destructor should be provided!");

            BehaviorMethod* createHandler = aznew Internal::BehaviorMethodImpl<typename AZStd::remove_pointer<HandlerCreator>::type>(creator, Base::m_context, m_ebus->m_name + "::CreateHandler");
            BehaviorMethod* destroyHandler = aznew Internal::BehaviorMethodImpl<typename AZStd::remove_pointer<HandlerDestructor>::type>(destructor, Base::m_context, m_ebus->m_name + "::DestroyHandler");

            // check than the handler returns the expected type
            if (createHandler->GetResult()->m_typeId != AzTypeInfo<BehaviorEBusHandler>::Uuid() || destroyHandler->GetArgument(0)->m_typeId != AzTypeInfo<BehaviorEBusHandler>::Uuid())
            {
                AZ_Assert(false, "HandlerCreator my return a BehaviorEBusHandler* object and HandlerDestrcutor should have an argument that can handle BehaviorEBusHandler!");
                delete createHandler;
                delete destroyHandler;
                createHandler = nullptr;
                destroyHandler = nullptr;
            }
            else
            {
                Base::m_currentAttributes = &createHandler->m_attributes;
            }
            m_ebus->m_createHandler = createHandler;
            m_ebus->m_destroyHandler = destroyHandler;
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    template<class H>
    BehaviorContext::EBusReflection<Bus>* BehaviorContext::EBusReflection<Bus>::Handler()
    {
        Handler(&AZ::Internal::BehaviorEBusHandlerFactory<H>::Create, &AZ::Internal::BehaviorEBusHandlerFactory<H>::Destroy);
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Bus>
    BehaviorContext::EBusReflection<Bus>* BehaviorContext::EBusReflection<Bus>::VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent)
    {
        if (m_ebus)
        {
            BehaviorEBusEventSender* getter = nullptr;
            BehaviorEBusEventSender* setter = nullptr;
            if (getterEvent)
            {
                auto getterIt = m_ebus->m_events.find(getterEvent);
                getter = &getterIt->second;
                if (getterIt == m_ebus->m_events.end())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter event %s is not reflected. Make sure VirtualProperty is reflected after the Event!", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }

                // we should always have the broadcast present, so use it for our checks
                if (!getter->m_broadcast->HasResult())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s should return result", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }
                if (getter->m_broadcast->GetNumArguments() != 0)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s can not have arguments only result", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }
            }
            if (setterEvent)
            {
                auto setterIt = m_ebus->m_events.find(setterEvent);
                if (setterIt == m_ebus->m_events.end())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter event %s is not reflected. Make sure VirtualProperty is reflected after the Event!", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
                setter = &setterIt->second;
                if (setter->m_broadcast->HasResult())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter %s should not return result", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
                if (setter->m_broadcast->GetNumArguments() != 1)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s can have only one argument", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
            }

            if (getter && setter)
            {
                if (getter->m_broadcast->GetResult()->m_typeId != setter->m_broadcast->GetArgument(0)->m_typeId)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s return [%s] and setter %s input argument [%s] types don't match ", m_ebus->m_name.c_str(), name, setterEvent, getter->m_broadcast->GetResult()->m_typeId.ToString<AZStd::string>().c_str(), setter->m_broadcast->GetArgument(0)->m_typeId.ToString<AZStd::string>().c_str());
                    return this;
                }
            }
            
            m_ebus->m_virtualProperties.insert(AZStd::make_pair(name, BehaviorEBus::VirtualProperty(getter, setter)));
        }        
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    namespace Internal
    {
        template<class T>
        struct SizeOfSafe
        {
            static size_t Get() { return sizeof(T); }
        };

        template<>
        struct SizeOfSafe<void>
        {
            static size_t Get() { return 0; }
        };

        // Assumes parameters array is big enough to store all parameters
        template<class... Args>
        inline void SetParametersStripped(BehaviorParameter* parameters, BehaviorContext* behaviorContext)
        {
            // +1 to avoid zero array size
            Uuid argumentTypes[sizeof...(Args)+1] = { AzTypeInfo<Args>::Uuid()... };
            const char* argumentNames[sizeof...(Args)+1] = { AzTypeInfo<Args>::Name()... };
            bool argumentIsPointer[sizeof...(Args)+1] = { AZStd::is_pointer<typename AZStd::remove_reference<Args>::type>::value... };
            bool argumentIsConst[sizeof...(Args)+1] = { AZStd::is_const<Args>::value... };
            bool argumentIsReference[sizeof...(Args)+1] = { AZStd::is_reference<Args>::value... };
            IRttiHelper* rttiHelper[sizeof...(Args)+1] = { GetRttiHelper<typename AZStd::remove_pointer<typename AZStd::decay<Args>::type>::type>()... };
            (void)argumentIsPointer; (void)argumentIsConst; (void)argumentIsReference;
            // function / member function pointer ?
            for (size_t i = 0; i < sizeof...(Args); ++i)
            {
                parameters[i].m_typeId = argumentTypes[i];
                parameters[i].m_name = argumentNames[i];
                parameters[i].m_azRtti = rttiHelper[i];
                parameters[i].m_traits = (argumentIsPointer[i] ? BehaviorParameter::TR_POINTER : 0) |
                    (argumentIsConst[i] ? BehaviorParameter::TR_CONST : 0) |
                    (argumentIsReference[i] ? BehaviorParameter::TR_REFERENCE : 0);
            }

            if (behaviorContext)
            {
                // deal with OnDemand reflection
                OnDemandReflectionFunction reflectHooks[sizeof...(Args)+1] = { OnDemandReflectHook<typename AZStd::remove_pointer<typename AZStd::decay<Args>::type>::type>::Get()... };
                for (size_t i = 0; i < sizeof...(Args); ++i)
                {
                    if (reflectHooks[i])
                    {
                        // One reflection per type 
                        bool isReflecting = behaviorContext->m_onDemandReflection.insert(AZStd::make_pair(argumentTypes[i],reflectHooks[i])).second;
                        if (isReflecting)
                        {
                            behaviorContext->m_toProcessOnDemandReflection.insert(AZStd::make_pair(argumentTypes[i], reflectHooks[i]));
                        }
                    }
                }
            }
        }
        template<class... Args>
        inline void SetParameters(BehaviorParameter* parameters, BehaviorContext* behaviorContext)
        {
            SetParametersStripped<AZStd::RemoveEnumT<Args>...>(parameters, behaviorContext);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        template<AZStd::size_t... Is, class Function>
        inline void CallFunction<R, Args...>::Global(Function functionPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
        {
            (void)arguments;
            if (result)
            {
                result->StoreResult<R>(functionPtr(*arguments[Is].GetAsUnsafe<Args>()...));
            }
            else
            {
                functionPtr(*arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        template<AZStd::size_t... Is, class C, class Function>
        inline void CallFunction<R, Args...>::Member(Function functionPtr, C thisPtr, BehaviorValueParameter* arguments, BehaviorValueParameter* result, AZStd::index_sequence<Is...>)
        {
            (void)arguments;
            if (result)
            {
                result->StoreResult<R>((thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...));
            }
            else
            {
                (thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...);
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        BehaviorMethodImpl<R(Args...)>::BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name)
            : m_functionPtr(functionPointer)
        {
            m_name = name;
            SetParameters<R>(m_parameters, context);
            SetParameters<Args...>(&m_parameters[1], context);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool BehaviorMethodImpl<R(Args...)>::Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result)
        {
            if (numArguments < AZ_ARRAY_SIZE(m_parameters) - 1)
            {
                if (m_defaultArguments && m_defaultArguments->GetNumValues() + numArguments >= AZ_ARRAY_SIZE(m_parameters) - 1)
                {
                    // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array 
                    // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
                    BehaviorValueParameter* newArguments = reinterpret_cast<BehaviorValueParameter*>(alloca(sizeof(BehaviorValueParameter)*(AZ_ARRAY_SIZE(m_parameters) - 1)));
                    // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
                    unsigned int iNewArgument = 0;
                    for (unsigned int i = 0; i < numArguments; ++i, ++iNewArgument)
                    {
                        new(&newArguments[iNewArgument]) BehaviorValueParameter(arguments[i]);
                    }
                    // clone from default list but
                    size_t defaultNumArguments = AZ_ARRAY_SIZE(m_parameters) - 1 - numArguments;
                    size_t defaultStartIndex = m_defaultArguments->GetNumValues() - defaultNumArguments;
                    for (size_t i = 0; i < defaultNumArguments; ++i, ++iNewArgument)
                    {
                        new(&newArguments[iNewArgument]) BehaviorValueParameter(m_defaultArguments->GetValue(i + defaultStartIndex));
                    }

                    arguments = newArguments;
                }
                else
                {
                    // not enough arguments
                    AZ_Warning("Behavior", false, "Not enough arguments to make a member call! %d needed %d", numArguments, AZ_ARRAY_SIZE(m_parameters) - 1);
                    return false;
                }
            }

            for (size_t i = 1; i < AZ_ARRAY_SIZE(m_parameters); ++i)
            {
                if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
                {
                    AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)", m_name.c_str(), i - 1, arguments[i - 1].m_name, arguments[i - 1].m_typeId.template ToString<AZStd::string>().c_str(), m_parameters[i].m_name, m_parameters[i].m_typeId.template ToString<AZStd::string>().c_str());
                    return false;
                }
            }

            CallFunction<R, Args...>::Global(m_functionPtr, arguments, result, AZStd::make_index_sequence<sizeof...(Args)>());

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool BehaviorMethodImpl<R(Args...)>::HasResult() const
        {
            return !AZStd::is_same<R, void>::value;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool BehaviorMethodImpl<R(Args...)>::IsMember() const
        {
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        size_t BehaviorMethodImpl<R(Args...)>::GetNumArguments() const
        {
            return AZ_ARRAY_SIZE(m_parameters) - 1;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(Args...)>::GetArgument(size_t index) const
        {
            if (index < AZ_ARRAY_SIZE(m_parameters) - 1)
            {
                return &m_parameters[index + 1];
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        size_t BehaviorMethodImpl<R(Args...)>::GetNumArgumentNames() const
        {
            return AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        const AZStd::string* BehaviorMethodImpl<R(Args...)>::GetArgumentName(size_t index) const
        {
            if (index < AZ_ARRAY_SIZE(m_argumentNames) - 1)
            {
                return &m_argumentNames[index + s_startNamedArgumentIndex];
            }
            return nullptr;
        }

        template<class R, class... Args>
        void BehaviorMethodImpl<R(Args...)>::SetArgumentName(size_t index, const AZStd::string& name)
        {
            if (index < AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex)
            {
                m_argumentNames[index + s_startNamedArgumentIndex] = name;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(Args...)>::GetResult() const
        {
            return &m_parameters[0];
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        BehaviorMethodImpl<R(C::*)(Args...)>::BehaviorMethodImpl(FunctionPointer functionPointer, BehaviorContext* context, const AZStd::string& name)
            : m_functionPtr(functionPointer)
        {
            m_name = name;
            SetParameters<R>(m_parameters, context);
            SetParameters<C*>(&m_parameters[1], context);
            m_parameters[1].m_traits |= BehaviorParameter::TR_THIS_PTR;
            SetParameters<Args...>(&m_parameters[2], context);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        BehaviorMethodImpl<R(C::*)(Args...)>::BehaviorMethodImpl(FunctionPointerConst functionPointer, BehaviorContext* context, const AZStd::string& name)
            : BehaviorMethodImpl((FunctionPointer)functionPointer, context, name)
        {
            m_isConst = true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool BehaviorMethodImpl<R(C::*)(Args...)>::Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result)
        {
            if (numArguments < AZ_ARRAY_SIZE(m_parameters) - 1)
            {
                if (m_defaultArguments && m_defaultArguments->GetNumValues() + numArguments >= AZ_ARRAY_SIZE(m_parameters) - 1)
                {
                    // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array 
                    // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
                    BehaviorValueParameter* newArguments = reinterpret_cast<BehaviorValueParameter*>(alloca(sizeof(BehaviorValueParameter)*(AZ_ARRAY_SIZE(m_parameters) - 1)));
                    // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
                    unsigned int iNewArgument = 0;
                    for (unsigned int i = 0; i < numArguments; ++i, ++iNewArgument)
                    {
                        new(&newArguments[iNewArgument]) BehaviorValueParameter(arguments[i]);
                    }
                    // clone from default list but
                    size_t defaultNumArguments = AZ_ARRAY_SIZE(m_parameters) - 1 - numArguments;
                    size_t defaultStartIndex = m_defaultArguments->GetNumValues() - defaultNumArguments;
                    for (size_t i = 0; i < defaultNumArguments; ++i, ++iNewArgument)
                    {
                        new(&newArguments[iNewArgument]) BehaviorValueParameter(m_defaultArguments->GetValue(i + defaultStartIndex));
                    }

                    arguments = newArguments;
                }
                else
                {
                    // not enough arguments
                    AZ_Warning("Behavior", false, "Not enough arguments to make a call! %d needed %d", numArguments, AZ_ARRAY_SIZE(m_parameters) - 1);
                    return false;
                }
            }

            if (!arguments[0].ConvertTo(AzTypeInfo<C>::Uuid()))
            {
                // this pointer is invalid
                AZ_Warning("Behavior", false, "First parameter should be the 'this' pointer for the member function!");
                return false;
            }

            for (size_t i = 2; i < AZ_ARRAY_SIZE(m_parameters); ++i)
            {
                if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
                {
                    AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)", m_name.c_str(), i - 1, arguments[i - 1].m_name, arguments[i - 1].m_typeId.template ToString<AZStd::string>().c_str(), m_parameters[i].m_name, m_parameters[i].m_typeId.template ToString<AZStd::string>().c_str());
                    return false;
                }
            }

            CallFunction<R, Args...>::Member(m_functionPtr, *arguments[0].GetAsUnsafe<C*>(), &arguments[1], result, AZStd::make_index_sequence<sizeof...(Args)>());

            EBUS_EVENT_ID(((void*)(*arguments[0].GetAsUnsafe<C*>())), BehaviorObjectSignals, OnMemberMethodCalled, this);

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool BehaviorMethodImpl<R(C::*)(Args...)>::HasResult() const
        {
            return !AZStd::is_same<R, void>::value;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool BehaviorMethodImpl<R(C::*)(Args...)>::IsMember() const
        {
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        size_t BehaviorMethodImpl<R(C::*)(Args...)>::GetNumArguments() const
        {
            return AZ_ARRAY_SIZE(m_parameters) - 1;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(C::*)(Args...)>::GetArgument(size_t index) const
        {
            if (index < AZ_ARRAY_SIZE(m_parameters) - 1)
            {
                return &m_parameters[index + 1];
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        size_t BehaviorMethodImpl<R(C::*)(Args...)>::GetNumArgumentNames() const
        {
            return AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        const AZStd::string* BehaviorMethodImpl<R(C::*)(Args...)>::GetArgumentName(size_t index) const
        {
            if (index < AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex)
            {
                return &m_argumentNames[index + s_startNamedArgumentIndex];
            }
            return nullptr;
        }

        template<class R, class C, class... Args>
        void BehaviorMethodImpl<R(C::*)(Args...)>::SetArgumentName(size_t index, const AZStd::string& name)
        {
            if (index < AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex)
            {
                m_argumentNames[index + s_startNamedArgumentIndex] = name;
            }
        }



        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        const BehaviorParameter* BehaviorMethodImpl<R(C::*)(Args...)>::GetResult() const
        {
            return &m_parameters[0];
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::BehaviorEBusEvent(FunctionPointer functionPointer, BehaviorContext* context)
            : m_functionPtr(functionPointer)
        {
            SetParameters<R>(m_parameters, context);
            SetParameters<Args...>(&m_parameters[1 + s_isBusIdParameter], context);
            // optional ID parameter
            SetBusIdType<s_isBusIdParameter == 1>();
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        template<bool IsBusId>
        inline AZStd::enable_if_t<IsBusId> BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetBusIdType()
        {
            SetParameters<typename EBus::BusIdType>(&m_parameters[1]);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        template<bool IsBusId>
        inline AZStd::enable_if_t<!IsBusId> BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetBusIdType()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        bool BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::Call(BehaviorValueParameter* arguments, unsigned int numArguments, BehaviorValueParameter* result)
        {
            if (numArguments < AZ_ARRAY_SIZE(m_parameters) - 1)
            {
                if (m_defaultArguments && m_defaultArguments->GetNumValues() + numArguments >= AZ_ARRAY_SIZE(m_parameters) - 1)
                {
                    // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array 
                    // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
                    BehaviorValueParameter* newArguments = reinterpret_cast<BehaviorValueParameter*>(alloca(sizeof(BehaviorValueParameter)*(AZ_ARRAY_SIZE(m_parameters) - 1)));
                    // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
                    unsigned int iNewArgument = 0;
                    for (unsigned int i = 0; i < numArguments; ++i, ++iNewArgument)
                    {
                        new(&newArguments[iNewArgument]) BehaviorValueParameter(arguments[i]);
                    }
                    // clone from default list but
                    size_t defaultNumArguments = AZ_ARRAY_SIZE(m_parameters) - 1 - numArguments;
                    size_t defaultStartIndex = m_defaultArguments->GetNumValues() - defaultNumArguments;
                    for (size_t i = 0; i < defaultNumArguments; ++i, ++iNewArgument)
                    {
                        new(&newArguments[iNewArgument]) BehaviorValueParameter(m_defaultArguments->GetValue(i + defaultStartIndex));
                    }

                    arguments = newArguments;
                }
                else
                {
                    // not enough arguments
                    AZ_Warning("Behavior", false, "Not enough arguments to make a call! %d needed %d", numArguments, AZ_ARRAY_SIZE(m_parameters) - 1);
                    return false;
                }
            }

            if (s_isBusIdParameter && !arguments[0].ConvertTo(m_parameters[1].m_typeId))
            {
                AZ_Warning("Behavior", false, "Invalid BusIdType type can't convert! %s -> %s", arguments[0].m_name, m_parameters[1].m_name);
                return false;
            }

            for (size_t i = 1 + s_isBusIdParameter; i < AZ_ARRAY_SIZE(m_parameters); ++i)
            {
                if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
                {
                    AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)", m_name.c_str(), i - 1, arguments[i - 1].m_name, arguments[i - 1].m_typeId.template ToString<AZStd::string>().c_str(), m_parameters[i].m_name, m_parameters[i].m_typeId.template ToString<AZStd::string>().c_str());
                    return false;
                }
            }

            Internal::EBusCaller<EventType, EBus, R, Args...>::Call(m_functionPtr, &arguments[0], result, AZStd::make_index_sequence<sizeof...(Args)>());

            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        bool BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::HasResult() const
        {
            return !AZStd::is_same<R, void>::value;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        bool BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::IsMember() const
        {
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        size_t BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetNumArguments() const
        {
            return AZ_ARRAY_SIZE(m_parameters) - 1;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetArgument(size_t index) const
        {
            if (index < AZ_ARRAY_SIZE(m_parameters) - 1)
            {
                return &m_parameters[index + 1];
            }
            return nullptr;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        size_t BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetNumArgumentNames() const
        {
            return AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const AZStd::string* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetArgumentName(size_t index) const
        {
            if (index < AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex)
            {
                return &m_argumentNames[index + s_startNamedArgumentIndex];
            }
            return nullptr;
        }

        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        void BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::SetArgumentName(size_t index, const AZStd::string& name)
        {
            if (index < AZ_ARRAY_SIZE(m_argumentNames) - s_startNamedArgumentIndex)
            {
                m_argumentNames[index + s_startNamedArgumentIndex] = name;
            }
        }


        //////////////////////////////////////////////////////////////////////////
        template<class EBus, BehaviorEventType EventType, class R, class C, class... Args>
        const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(C::*)(Args...)>::GetResult() const
        {
            return &m_parameters[0];
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        void SetFunctionParameters<R(Args...)>::Set(AZStd::vector<BehaviorParameter>& params)
        {
            // result, userdata, arguments  
            params.resize(sizeof...(Args) + eBehaviorBusForwarderEventIndices::ParameterFirst);
            SetParameters<R>(&params[eBehaviorBusForwarderEventIndices::Result], nullptr);
            SetParameters<void*>(&params[eBehaviorBusForwarderEventIndices::UserData], nullptr);
            if (sizeof...(Args) > 0)
            {
                SetParameters<Args...>(&params[eBehaviorBusForwarderEventIndices::ParameterFirst], nullptr);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class... Args>
        bool SetFunctionParameters<R(Args...)>::Check(AZStd::vector<BehaviorParameter>& source)
        {
            Uuid argumentTypes[sizeof...(Args)] = { AzTypeInfo<Args>::Uuid()... };
            // use for error control
            const char* argumentNames[sizeof...(Args)] = { AzTypeInfo<Args>::Name()... };
            (void)argumentNames;
            if (source.size() != sizeof...(Args)+1) // +1 for result
            {
                return false;
            }
            // check result type
            if (source[0].m_typeId != AzTypeInfo<R>::Uuid())
            {
                return false;
            }
            for (size_t i = 0; i < sizeof...(Args); ++i)
            {
                if (source[i + 1].m_typeId != argumentTypes[i])
                {
                    return false;
                }
            }
            return true;
        }


        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        void SetFunctionParameters<R(C::*)(Args...)>::Set(AZStd::vector<BehaviorParameter>& params)
        {
            SetFunctionParameters<R(Args...)>::Set(params);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class R, class C, class... Args>
        bool SetFunctionParameters<R(C::*)(Args...)>::Check(AZStd::vector<BehaviorParameter>& source)
        {
            Uuid argumentTypes[sizeof...(Args)] = { AzTypeInfo<Args>::Uuid()... };
            // use for error control
            const char* argumentNames[sizeof...(Args)] = { AzTypeInfo<Args>::Name()... };
            (void)argumentNames;
            if (source.size() != sizeof...(Args)+1) // +1 for result
            {
                return false;
            }
            // check result type
            if (source[0].m_typeId != AzTypeInfo<R>::Uuid())
            {
                return false;
            }
            for (size_t i = 1; i < sizeof...(Args); ++i)
            {
                if (source[i + 1].m_typeId != argumentTypes[i])
                {
                    return false;
                }
            }
            return true;
        }


        //////////////////////////////////////////////////////////////////////////
        template<class T>
        void* BahaviorDefaultFactory<T>::Create(void* inplaceAddress, void* userData)
        {
            (void)userData;
            if (inplaceAddress)
            {
                return new(inplaceAddress)T();
            }
            else
            {
                return aznew T();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class T>
        void BahaviorDefaultFactory<T>::Destroy(void* objectPtr, bool isFreeMemory, void* userData)
        {
            (void)userData;
            T* object = reinterpret_cast<T*>(objectPtr);
            if (isFreeMemory)
            {
                delete object;
            }
            else
            {
                object->~T();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class T>
        void* BahaviorDefaultFactory<T>::Clone(void* targetAddress, void* sourceAddress, void* userData)
        {
            (void)userData;
            if (targetAddress)
            {
                return new(targetAddress)T(*reinterpret_cast<T*>(sourceAddress));
            }
            else
            {
                return aznew T(*reinterpret_cast<T*>(sourceAddress));
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class Handler>
        BehaviorEBusHandler* BehaviorEBusHandlerFactory<Handler>::Create()
        {
            return aznew Handler();
        }

        //////////////////////////////////////////////////////////////////////////
        template<class Handler>
        void BehaviorEBusHandlerFactory<Handler>::Destroy(BehaviorEBusHandler* handler)
        {
            delete handler;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class BusHandler, bool IsBusId = !AZStd::is_same<typename BusHandler::BusType::BusIdType, AZ::NullBusId>::value>
        struct EBusConnector
        {
            static bool Connect(BusHandler* handler, BehaviorValueParameter* id)
            {
                if (id && id->ConvertTo<typename BusHandler::BusType::BusIdType>())
                {
                    handler->BusConnect(*id->GetAsUnsafe<typename BusHandler::BusType::BusIdType>());
                    return true;
                }
                return false;
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class BusHandler>
        struct EBusConnector<BusHandler, false>
        {
            static bool Connect(BusHandler* handler, BehaviorValueParameter* id)
            {
                (void)id;
                handler->BusConnect();
                return false;
            }
        };

        //////////////////////////////////////////////////////////////////////////
        template<class Owner>
        template<class T>
        void GenericAttributes<Owner>::SetAttributeContextData(T value, AZ::Attribute* attribute, const AZStd::true_type& /* is_function<remove_pointer<T>::type> && is_member_function_pointer<T>*/)
        {
            BehaviorMethod* method = aznew Internal::BehaviorMethodImpl<typename AZStd::remove_pointer<T>::type>(value, m_context, AZStd::string("Function-Attribute"));
            attribute->SetContextData(method);
            m_context->m_attributeMethods.push_back(method);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class Owner>
        template<class T>
        Owner* GenericAttributes<Owner>::Attribute(Crc32 idCrc, T value)
        {
            if (m_context->IsRemovingReflection())
            {
                return static_cast<Owner*>(this);
            }

            typedef typename AZStd::conditional<AZStd::is_member_pointer<T>::value,
                typename AZStd::conditional<AZStd::is_member_function_pointer<T>::value, AttributeMemberFunction<T>, AttributeMemberData<T> >::type,
                typename AZStd::conditional<AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value, AttributeFunction<typename AZStd::remove_pointer<T>::type>, AttributeData<T> >::type
            >::type ContainerType;

            //AZ_Assert(Internal::AttributeValueTypeClassChecker<T>::Check(m_classData->m_typeId), "Attribute (0x%08x) doesn't belong to '%s' class! You can't reference other classes!", idCrc, m_classData->m_name);
            AZ_Assert(m_currentAttributes, "You can attach attributes to Methods, Properties, Classes, EBuses and EBus Events!");
            if (m_currentAttributes)
            {
                AZ::Attribute* attribute = aznew ContainerType(value);
                SetAttributeContextData<T>(value, attribute, typename AZStd::conditional<AZStd::is_member_function_pointer<T>::value | AZStd::is_function<typename AZStd::remove_pointer<T>::type>::value, AZStd::true_type, AZStd::false_type>::type());
                m_currentAttributes->push_back(AttributePair(idCrc, attribute));
            }
            return static_cast<Owner*>(this);
        }
    } // namespace Internal
} // namespace AZ

#if defined(AZ_COMPILER_MSVC)
#   pragma warning(pop)
#endif

// pull AzStd on demand reflection
#include <AzCore/RTTI/AzStdOnDemandReflection.inl>

