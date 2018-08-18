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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTSERIALIZATION_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTSERIALIZATION_H
#pragma once

#include "Components/IComponentSerialization.h"

//! Component that can be bound to existing serialization (Serialize, SerializeXML) functions.
//! Components must register their serialization functions themselves, providing the ComponentTypeID,
//! the desired order in which serialization needs to occur and the serialization function point to
//! invoke.
class ComponentSerialization
    : public IComponentSerialization
{
public:
    //! Remove a component's registration.
    void Unregister(const ComponentType& componentType) override;

    //! Serialize all components registered.
    void Serialize(TSerialize s) override;

    //! Serialize only the specified components, if they are registered.
    void SerializeOnly(TSerialize s, const std::vector<ComponentType>& componentsToSerialize) override;

    //! Serialize components, with optional list of components to skip
    void SerializeXML(XmlNodeRef& node, bool loading, const std::vector<ComponentType>& componentsToSkip = {}) override;

    //! Serialize only the components specified
    void SerializeXMLOnly(XmlNodeRef& node, bool loading, const std::vector<ComponentType>& componentsToSerialize = {}) override;

    //! True if the component needs to be serialized.
    bool NeedSerialize() const override;

    //! Builds a signature to describe the dynamic hierarchy of the parent Entity container
    //! \param ser - the object to serialize with, forming the signature
    //! \return true - If the signature is thus far valid
    //! \remarks
    //! It's the responsibility of the component to identify its internal state which may complicate the hierarchy
    //! of the parent Entity
    bool GetSignature(TSerialize signature) override;

protected:

    void RegisterInternal(uint32 serializationOrder, const ComponentType& componentType, SerializeFunction serialize, SerializeXMLFunction serializeXML, NeedSerializeFunction needsSerialization, GetSignatureFunction getSignature) override;

    //! Index for each serialization function in SerializeFunctions
    struct SerializationFunctionIndex
    {
        enum
        {
            Serialize,
            SerializeXML,
            NeedsSerialize,
            GetSignature
        };
    };

    //! Available serialization functions.
    using SerializeFunctions = std::tuple < SerializeFunction, SerializeXMLFunction, NeedSerializeFunction, GetSignatureFunction>;

    //! Serialization functions are called using the serialization order defined in the SerializationOrder namespace.
    using OrderedMap = std::multimap < uint32, ComponentType >;
    OrderedMap m_order;

    // Registration of component types to serialization functions.
    using SerializationRegistration = std::map < ComponentType, SerializeFunctions >;
    SerializationRegistration m_serializationFunctionMap;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTSERIALIZATION_H