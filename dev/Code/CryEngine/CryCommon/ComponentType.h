// Portions of this file Copyright (C) Amazon.com, Inc. or its affiliates.
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTTYPE_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTTYPE_H
#pragma once

#include <CryExtension/CryGUID.h>

using ComponentTypeId = CryGUID;

//! Contains type information for an IComponent.
//! <p>
//! A ComponentType does not exist for every subclass of IComponent.
//! A ComponentType should exist only for identifiable subclasses of IComponent
//! that a user would wish to fetch from an IEntity.
//! <p>
//! For example, IComponentRender is an abstract interface that expects a
//! single concrete implementation in some module. Many modules wish
//! to fetch an IComponentRender from an entity and interact with it,
//! but these modules know nothing about the actual concrete implementation.
//! Therefore, we assign a ComponentType to IComponentRender,
//! not to its concrete implementation class.
class ComponentType
{
public:
	//! A statically compiled string must be passed in.
	//! No local copy is made so the source must live forever.
	struct StaticString
	{
		explicit StaticString(const char* str) : value(str) {}
		const char* value;
	};

	ComponentType(const StaticString& name, const ComponentTypeId& id) : m_id(id), m_name(name.value) {}

	//! Each type must have a unique ID.
	const ComponentTypeId& Id() const { return m_id; }

	//! The component's name. For debugging purposes only.
	const char* Name() const { return m_name; }

	bool operator == (const ComponentType& rhs) const { return m_id == rhs.m_id; }
	bool operator != (const ComponentType& rhs) const { return m_id != rhs.m_id; }
	bool operator < (const ComponentType& rhs) const { return m_id < rhs.m_id; }

protected:
	ComponentTypeId m_id;
	const char* m_name;
};


//! Declare a ComponentType for an IComponent subclass,
//! accessible via static Type() function.
//! \param NAME_STATIC_STRING  Name, for debugging purposes only.
//!                            Pass a statically compiled string.
//! \param GUID_HI  High 64bits of a unique ID
//! \param GUID_LO  Low 64bits of a unique ID
#define DECLARE_COMPONENT_TYPE(NAME_STATIC_STRING, GUID_HI, GUID_LO) \
	static const ComponentType& Type() { \
		static const ComponentType type(ComponentType::StaticString(NAME_STATIC_STRING), {(GUID_HI), (GUID_LO)}); \
		return type; \
	} \
	/* This function is used to statically detect at compile time if a class has declared it's component type */ \
	const ComponentType &GetComponentType() const override { return Type(); }

//! Validate that a class or one of its parent classes has been properly decorated with DECLARE_COMPONENT_TYPE.
//! Usage: static_assert(HasTypeFunction<DD>::value,"Class was not decorated");
template<typename T>
struct HasTypeFunction
{
    // Match pointer to function: const ComponentType& Type()
    template<const ComponentType& (*)()>
    struct SFINAE;

    // Match classes with Type() member function
    template<typename U>
    static char Test(SFINAE<&U::Type>*);

    // Match everything else
    template<typename U> static int Test(...);

    // The return value of the static assert test
    static const bool value = sizeof(Test<T>(0)) == sizeof(char);
};

//! Allow ComponentType be used as key in std::unordered_map
//! by specializing std::hash<ComponentType>
namespace AZStd
{
template<>
struct hash<ComponentType>
{
	size_t operator()(const ComponentType& componentType) const
	{
		return hash<ComponentTypeId>()(componentType.Id());
	}
};
}

#endif //CRYINCLUDE_CRYCOMMON_COMPONENTTYPE_H
