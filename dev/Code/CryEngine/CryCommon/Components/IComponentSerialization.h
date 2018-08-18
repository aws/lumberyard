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
#ifndef CRYINCLUDE_CRYCOMMON_ICOMPONENTSERIALIZATION_H
#define CRYINCLUDE_CRYCOMMON_ICOMPONENTSERIALIZATION_H
#pragma once

#include <SerializeFwd.h>

//! Central location to specify serialization order of components.
namespace SerializationOrder
{
    static const uint32 Render          = 1;
    static const uint32 Script          = 2;
    static const uint32 Audio           = 3;
    static const uint32 AI              = 4;
    static const uint32 Area            = 5;
    static const uint32 Boids           = 6;
    static const uint32 BoidObject      = 7;
    static const uint32 Camera          = 8;
    static const uint32 FlowGraph       = 9;
    static const uint32 Substitution    = 10;
    static const uint32 Trigger         = 11;
    static const uint32 User            = 12;
    static const uint32 Physics         = 13;
    static const uint32 Rope            = 14;
    static const uint32 EntityNode      = 15;
    static const uint32 Attributes      = 16;
    static const uint32 ClipVolume      = 17;
}

//! Interface for an entity's serialization component.
//! This component serves to coordinate serialization for all
//! components on the entity. Other components must register themselves
//! or their serialization functions will not be called.
class IComponentSerialization
    : public IComponent
{
public:

    DECLARE_COMPONENT_TYPE("ComponentSerialization", 0x034C99C6F8AC4023, 0x89E25A034A9EBFA0)
    IComponentSerialization() {}

    //! Register a component's serialization functions.
    //! \tparam T                 the component class being registered
    //! \param serializationOrder lower numbers are serialized earlier
    //! \param objectRef          reference to component being registered
    //! \param serialize          member function for handling network serialization
    //! \param serializeXML       member function for handling XML serialization
    //! \param needsSerialize     member function that tells whether component needs to be serialized
    //! \param getSignature       member function for writing the component's signature
    template <typename T>
    void Register(uint32 serializationOrder, T& objectRef, void(T::* serialize)(TSerialize), void(T::* serializeXML)(XmlNodeRef&, bool), bool(T::* needsSerialize)(), bool(T::* getSignature)(TSerialize))
    {
        static_assert(std::is_base_of<IComponent, T>::value, "Only classes that derive from IComponent can be registered with this component.");
        using namespace AZStd::placeholders;
        RegisterInternal(serializationOrder, T::Type(),
            serialize ? AZStd::bind(serialize, &objectRef, _1) : SerializeFunction(),
            serializeXML ? AZStd::bind(serializeXML, &objectRef, _1, _2) : SerializeXMLFunction(),
            needsSerialize ? AZStd::bind(needsSerialize, &objectRef) : NeedSerializeFunction(),
            getSignature ? AZStd::bind(getSignature, &objectRef, _1) : GetSignatureFunction());
    }

    //! Remove a component's registration.
    virtual void Unregister(const ComponentType& componentType) = 0;

    //! Serialize all components registered.
    virtual void Serialize(TSerialize s) = 0;

    //! Serialize only the specified components, if they are registered.
    virtual void SerializeOnly(TSerialize s, const std::vector<ComponentType>& componentsToSerialize) = 0;

    //! Serialize components, with optional list of components to skip
    virtual void SerializeXML(XmlNodeRef& node, bool loading, const std::vector<ComponentType>& componentsToSkip = {}) = 0;

    //! Serialize only the components specified
    virtual void SerializeXMLOnly(XmlNodeRef& node, bool loading, const std::vector<ComponentType>& componentsToSerialize = {}) = 0;

    //! True if the component needs to be serialized.
    virtual bool NeedSerialize() const = 0;

    //! Builds a signature to describe the dynamic hierarchy of the parent Entity container
    //! \param ser - the object to serialize with, forming the signature
    //! \return true - If the signature is thus far valid
    //! \remarks
    //! It's the responsibility of the component to identify its internal state which may complicate the hierarchy
    //! of the parent Entity
    virtual bool GetSignature(TSerialize signature) = 0;

protected:

    //! Serialization function signatures.
    using SerializeXMLFunction = AZStd::function < void(XmlNodeRef&, bool) >;
    using SerializeFunction = AZStd::function < void(TSerialize) >;
    using NeedSerializeFunction = AZStd::function < bool() >;
    using GetSignatureFunction = AZStd::function < bool (TSerialize signature) >;

    //! Concrete implementation must implement the registration.
    virtual void RegisterInternal(uint32, const ComponentType&, SerializeFunction, SerializeXMLFunction, NeedSerializeFunction, GetSignatureFunction) = 0;
};

DECLARE_COMPONENT_POINTERS(IComponentSerialization)

#endif // CRYINCLUDE_CRYCOMMON_ICOMPONENTSERIALIZATION_H