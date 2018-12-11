// Portions of this file Copyright (C) Amazon.com, Inc. or its affiliates.
#ifndef CRYINCLUDE_CRYCOMMON_ICOMPONENTFACTORY_H
#define CRYINCLUDE_CRYCOMMON_ICOMPONENTFACTORY_H
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <list>

class IComponent;
class ComponentType;

//! Base class for IComponentFactory<T>.
//! Derive factories from IComponentFactory, not IComponentFactoryBase.
class IComponentFactoryBase
{
public:
	virtual ~IComponentFactoryBase() {}

	virtual const ComponentType& GetComponentType() const = 0;
};

//! A factory is responsible for creating all components
//! whose type matches ComponentTypeT::Type()
//! \param ComponentTypeT  The class which defines the ComponentType used to
//!                        identify the component within an entity.
template<class ComponentTypeT>
class IComponentFactory : public IComponentFactoryBase
{
public:
	virtual AZStd::shared_ptr<ComponentTypeT> CreateComponent() = 0;

	const ComponentType& GetComponentType() const override { return ComponentTypeT::Type(); }
};


//! A component factory that simply uses new/delete.
//! \param ConcreteComponentT  The concrete class this factory creates.
//! \param ComponentTypeT  The class which defines the ComponentType used to
//!                        identify the component within an entity.
template<class ConcreteComponentT, class ComponentTypeT>
class DefaultComponentFactory : public IComponentFactory<ComponentTypeT>
{
public:
	AZStd::shared_ptr<ComponentTypeT> CreateComponent() override
	{
		return AZStd::shared_ptr<ConcreteComponentT>(
			new ConcreteComponentT(),
            AZStd::bind(&DefaultComponentFactory::Deleter, *this, AZStd::placeholders::_1));
	}
protected:

	//! Use a custom deleter to ensure that destruction logic runs in the
	//! module that created the component.
	void Deleter(ConcreteComponentT* component)
	{
		delete component;
	}
};


//! A ComponentFactoryCreationNode global variable should be declared
//! for each factory an engine module is responsible for.
//! <p>
//! Upon initilization each module should iterate its ComponentFactoryCreationNodes,
//! creating factories and registering them with the IComponentFactoryRegistry.
class ComponentFactoryCreationNode : public NoCopy
{
public:
	//! A function which creates an IComponentFactory
	using CreateFactoryFunction = AZStd::function<AZStd::unique_ptr<IComponentFactoryBase>()>;

	//! \param componentType  The factory creates components of this type.
	//! \param createFunction  A function to create the factory.
	ComponentFactoryCreationNode(CreateFactoryFunction createFunction);

	~ComponentFactoryCreationNode();

	//! \return this module's list of ComponentFactoryCreationNodes
	const CreateFactoryFunction GetCreateFactoryFunction() const { return m_createFactoryFunction; }

	//! \return this module's list of ComponentFactoryCreationNodes
	static const std::list<const ComponentFactoryCreationNode*>& GetGlobalList() { return GetMutableGlobalList(); }

protected:
	CreateFactoryFunction m_createFactoryFunction;

	static std::list<const ComponentFactoryCreationNode*>& GetMutableGlobalList();
	static ComponentFactoryCreationNode* sm_head;
    static size_t sm_size;
	ComponentFactoryCreationNode* m_next;
};


//! A ComponentFactoryCreationNode for a BasicComponentFactory
//! \param ConcreteComponentT  The concrete class this factory creates.
//! \param ComponentTypeT  The class which defines the ComponentType used to
//!                        identify the component within an entity.
template<class ConcreteComponentT, class ComponentTypeT>
class DefaultComponentFactoryCreationNode : public ComponentFactoryCreationNode
{
public:
	DefaultComponentFactoryCreationNode()
		: ComponentFactoryCreationNode([](){ return AZStd::make_unique<DefaultComponentFactory<ConcreteComponentT, ComponentTypeT>>(); })
	{}
};

//! Helper for declaring a DefaultComponentFactoryCreationNode global variable
//! \param ConcreteComponentT  The concrete class this factory creates.
//! \param ComponentTypeT  The class which defines the ComponentType used to
//!                        identify the component within an entity.
#define DECLARE_DEFAULT_COMPONENT_FACTORY(ConcreteComponentT, ComponentTypeT) \
	namespace FactoryNode_##ConcreteComponentT { \
		DefaultComponentFactoryCreationNode<ConcreteComponentT, ComponentTypeT> g##ConcreteComponentT##FactoryNode; \
	}

//
// Implementation
//

inline ComponentFactoryCreationNode::ComponentFactoryCreationNode(CreateFactoryFunction createFunction)
	: m_createFactoryFunction(createFunction)
    , m_next(sm_head)
{
	// insert self in this module's list of nodes
    sm_head = this;
    ++sm_size;
}

inline ComponentFactoryCreationNode::~ComponentFactoryCreationNode()
{
    ComponentFactoryCreationNode* node = sm_head;
    if (sm_head == this)
    {
        --sm_size;
        sm_head = sm_head->m_next;
        return;
    }
    node = sm_head->m_next;
    ComponentFactoryCreationNode* last = sm_head;
    while (node != nullptr)
    {
        if (node == this)
        {
            --sm_size;
            last->m_next = node->m_next;
            return;
        }
    }
}

inline std::list<const ComponentFactoryCreationNode*>& ComponentFactoryCreationNode::GetMutableGlobalList()
{
	static std::list<const ComponentFactoryCreationNode*> globalList;
    if (globalList.size() != sm_size)
    {
        globalList.clear();
        ComponentFactoryCreationNode* node = sm_head;
        while (node)
        {
            globalList.push_back(node);
            node = node->m_next;
        }
    }
	return globalList;
}

#endif // CRYINCLUDE_CRYCOMMON_ICOMPONENTFACTORY_H
