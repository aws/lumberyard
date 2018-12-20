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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/DataOverlayInstanceMsgs.h>
#include <AzCore/Serialization/DataOverlayProviderMsgs.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/IO/Streamer.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobManager.h>

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/error/en.h>

/**
 * Disable this to have more efficient packing for the json file (less readable without a tool)
 */
#define AZ_JSON_USE_PRETTY_WRITER

#ifdef AZ_JSON_USE_PRETTY_WRITER
#include <AzCore/JSON/prettywriter.h>
#else
#include <AzCore/JSON/writer.h>
#endif


#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    namespace ObjectStreamInternal
    {
        static const u32 s_objectStreamVersion = 3;
        static const u8 s_binaryStreamTag = 0;
        static const u8 s_xmlStreamTag = '<';
        static const u8 s_jsonStreamTag = '{';

        class ObjectStreamImpl;

        /**
         * ObjectStreamImpl
         */
        class ObjectStreamImpl
            : public ObjectStream
        {
        public:
            enum OptionFlags
            {
                OPF_SAVING      = 1 << 16,
            };

            // These are written with every data element. Note that the size can be
            // encoded in the flags if it is < 7 (this optimizes the common case 
            // of size=4). If new flags are required, they will need to be packed
            // into the extra size field
            enum BinaryFlags
            {
                ST_BINARYFLAG_MASK              = 0xF8, // upper-5 bits of a byte
                ST_BINARY_VALUE_SIZE_MASK       = 0x7,  // lower 3 bits of a byte. If ST_BINARYFLAG_EXTRA_SIZE_FIELD is set, contains the size of the size field, otherwise contains the size of the value chunk.
                ST_BINARYFLAG_ELEMENT_HEADER    = 1 << 3,
                ST_BINARYFLAG_HAS_VALUE         = 1 << 4,
                ST_BINARYFLAG_EXTRA_SIZE_FIELD  = 1 << 5,
                ST_BINARYFLAG_HAS_NAME          = 1 << 6,
                ST_BINARYFLAG_HAS_VERSION       = 1 << 7,
                ST_BINARYFLAG_ELEMENT_END       = 0
            };

            AZ_CLASS_ALLOCATOR(ObjectStreamImpl, SystemAllocator, 0);

            ObjectStreamImpl(IO::GenericStream* stream, SerializeContext* sc, const ClassReadyCB& readyCB, const CompletionCB& doneCB, const FilterDescriptor& filterDesc = FilterDescriptor(), int flags = 0, const InplaceLoadRootInfoCB& inplaceLoadInfoCB = InplaceLoadRootInfoCB())
                : ObjectStream(sc)
                , m_flags(flags)
                , m_stream(stream)
                , m_readyCB(readyCB)
                , m_doneCB(doneCB)
                , m_inplaceLoadInfoCB(inplaceLoadInfoCB)
                , m_version(s_objectStreamVersion)
                , m_pending(0)
                , m_inStream(&m_buffer1)
                , m_outStream(&m_buffer2)
            {
                // Assign default asset filter if none was provided by the user.
                m_scratchSpace.resize(m_scratchSpace.capacity());
                m_filterDesc = filterDesc;
            }

            /// Starts the operation
            bool Start();

            bool LoadClass(IO::GenericStream& stream, SerializeContext::DataElementNode& convertedClassElement, const SerializeContext::ClassData* parentClassInfo, void* parentClassPtr, int flags);

            // returns true if an element was found at the requested level
            bool ReadElement(SerializeContext& sc, const SerializeContext::ClassData*& cd, SerializeContext::DataElement& element, const SerializeContext::ClassData* parent, bool nextLevel, bool isTopElement);
            // used during load to skip the rest of the element including any subelements
            void SkipElement();

            bool WriteClass(const void* classPtr, const Uuid& classId, const SerializeContext::ClassData* classData);
            bool WriteElement(const void* elemPtr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement);
            bool CloseElement();

            /// finalizes the stream after the user is done submitting his writes
            virtual bool Finalize();

            /// Returns true if we will keep the element class, otherwise false
            bool ConvertOldVersion(SerializeContext& sc, SerializeContext::DataElementNode& elementNode, IO::GenericStream& stream, const SerializeContext::ClassData* elementClass);
            void PreparseOldVersion(SerializeContext& sc, SerializeContext::DataElementNode& elementNode, IO::GenericStream& stream, const SerializeContext::ClassData* elementClass);

            int                                 m_flags;
            FilterDescriptor                    m_filterDesc;
            IO::GenericStream*              m_stream;
            ClassReadyCB                        m_readyCB;
            CompletionCB                        m_doneCB;
            InplaceLoadRootInfoCB               m_inplaceLoadInfoCB;
            unsigned int                        m_version;
            SerializeContext::ErrorHandler      m_errorLogger;
            AZStd::atomic_int                   m_pending;

            // used for xml streams
            rapidxml::xml_document<char>*       m_xmlDoc;
            rapidxml::xml_node<char>*           m_xmlNode;

            AZStd::fixed_vector<char, 128>       m_scratchSpace; // prevent malloc thrash

            struct JSonReadNode
            {
                JSonReadNode()
                    : m_parentObjectArray(nullptr)
                    , m_currentObject(nullptr)
                {}
                JSonReadNode(rapidjson::Value* parentObjectArray, rapidjson::Value* currentObject)
                    : m_parentObjectArray(parentObjectArray)
                    , m_currentObject(currentObject)
                {}
                rapidjson::Value*   m_parentObjectArray;
                rapidjson::Value*   m_currentObject;
            };


            rapidjson::Document*                m_jsonDoc;


            AZStd::list<JSonReadNode>           m_jsonReadValues;
            AZStd::list<rapidjson::Value>       m_jsonWriteValues;


            AZStd::vector<char> m_buffer1;
            AZStd::vector<char> m_buffer2;
            IO::ByteContainerStream<AZStd::vector<char> > m_inStream;
            IO::ByteContainerStream<AZStd::vector<char> > m_outStream;

            // other state info
            bool                                m_isOverlaying;
        };

        //=========================================================================
        // PreparseOldVersion
        // [4/25/2012]
        //=========================================================================
        void ObjectStreamImpl::PreparseOldVersion(SerializeContext& sc, SerializeContext::DataElementNode& elementNode, IO::GenericStream& stream, const SerializeContext::ClassData* elementClass)
        {
            // whenever tracing is availalble we make error logging available.
#if defined(AZ_ENABLE_TRACING) 
            {
                SerializeContext::DbgStackEntry de;
                de.m_dataPtr = nullptr;
                de.m_uuidPtr = &elementClass->m_typeId;
                de.m_elementName = elementNode.GetNameString();
                de.m_classData = elementClass;
                de.m_classElement = nullptr;
                m_errorLogger.Push(de);
            }
#endif // AZ_ENABLE_TRACING
            SerializeContext::DataElement childElement;
            childElement.m_stream = &childElement.m_byteStream;
            childElement.m_stream->Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            childElement.m_dataSize = 0;

            const SerializeContext::ClassData* childClass = nullptr;
            bool nextLevel = true;
            while (ReadElement(sc, childClass, childElement, elementClass, nextLevel, false))
            {
                childElement.m_stream->Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                nextLevel = false;

                elementNode.m_subElements.push_back();
                SerializeContext::DataElementNode& childNode = elementNode.m_subElements.back();
                // we might need to copy the element name, if it's deleted after the read element
                // otherwise it will be left dangling
                childNode.m_element = AZStd::move(childElement);
                childNode.m_classData = childClass;

                if (childClass)
                {
                    AZ_Assert(childNode.m_element.m_version <= childClass->m_version, "Serialize was parsing old version class and found newer version element! This should be impossible!");

                    // Only proceed if:
                    // * the child node is out of date AND the class does not have a custom serializer
                    // * OR the class is deprecated
                    if ((childNode.m_element.m_version < childClass->m_version && !childClass->m_serializer)
                        || childClass->IsDeprecated())
                    {
                        if (!ConvertOldVersion(sc, childNode, *childNode.m_element.m_stream, childClass))
                        {
                            // conversion failed, remove the last element
                            elementNode.RemoveElement(static_cast<int>(elementNode.m_subElements.size()) - 1);
                        }
                        else if (childClass->IsDeprecated() && childClass->m_converter)
                        {
                            // Deprecated-class converters provide a replacement class.
                            childClass = childNode.m_classData;
                        }
                        continue;
                    }
                }
                else
                {
                    // output a warning
                    //AZ_Warning("Serializer",false,"Element '%s' with class ID '%s' found while converting '%s' is not registered with the serializer! You will have to parse this data yourself!",childElement.m_name,childElement.m_id.ToString<AZStd::string>().c_str(), parent->m_name);
                }

                if (childNode.m_element.m_dataSize > 0) // if we have values to convert
                {
                    // Now preparse this element's children
                    PreparseOldVersion(sc, childNode, *childNode.m_element.m_stream, childClass);
                }
                else
                {
                    // Now preparse this element's children
                    PreparseOldVersion(sc, childNode, stream, childClass);
                }
            }
#if defined(AZ_ENABLE_TRACING)
            m_errorLogger.Pop();
#endif // AZ_ENABLE_TRACING
        }


        //=========================================================================
        // ConvertOldVersion
        // [4/25/2012]
        //=========================================================================
        bool ObjectStreamImpl::ConvertOldVersion(SerializeContext& sc, SerializeContext::DataElementNode& elementNode, IO::GenericStream& stream, const SerializeContext::ClassData* elementClass)
        {
            AZ_Assert(elementNode.m_classData->IsDeprecated() || elementNode.m_element.m_version < elementNode.m_classData->m_version, "Don't call this function if the element is not an old version element!");

            PreparseOldVersion(sc, elementNode, stream, elementClass);

            if (elementNode.m_classData->m_converter)
            {
                if (!elementNode.m_classData->m_converter(sc, elementNode))
                {
                    return false;
                }
            }
            else
            {
                // If the user ups the version but does not provide a converter,
                // we will assume that any mismatching sub-element data is no longer needed:
                //  - Sub-elements whose name don't match any names of the class elements.
                //  - Sub-elements whose class data does not match the class data reflected.
                //  - Sub-elements whose class data is missing (a subcase of the above).
                for (int iDataElem = elementNode.GetNumSubElements() - 1; iDataElem >= 0; --iDataElem)
                {
                    SerializeContext::DataElement& subElem = elementNode.GetSubElement(iDataElem).m_element;
                    const SerializeContext::ClassData* classData = elementNode.GetSubElement(iDataElem).m_classData;
                    bool keep = false;
                    if (classData)
                    {
                        for (size_t iClassElem = 0; iClassElem < elementClass->m_elements.size(); ++iClassElem)
                        {
                            const SerializeContext::ClassElement& classElem = elementClass->m_elements[iClassElem];
                            if (classElem.m_nameCrc == subElem.m_nameCrc && m_sc->CanDowncast(subElem.m_id, classElem.m_typeId, classData->m_azRtti, classElem.m_azRtti))
                            {
                                keep = true;
                                break;
                            }
                        }
                    }
                    if (!keep)
                    {
                        elementNode.RemoveElement(iDataElem);
                    }
                }
            }
            elementNode.m_element.m_version = elementNode.m_classData->m_version;
            return true;
        }

        //=========================================================================
        // LoadClass
        // [4/25/2012]
        //=========================================================================
        bool ObjectStreamImpl::LoadClass(IO::GenericStream& stream, SerializeContext::DataElementNode& convertedClassElement, const SerializeContext::ClassData* parentClassInfo, void* parentClassPtr, int flags)
        {
            bool result = true;

            SerializeContext::DataElement element;

            if (element.m_stream == nullptr)
            {
                element.m_stream = &stream;
            }

            SerializeContext::DataElementNode* convertedNode = &convertedClassElement;
            size_t convertedClassElementIndex = 0;
            bool nextLevel = true;

            size_t currentContainerElementIndex = 0;    // used to load container elements

            while (true)
            {
                // reset the class info
                const SerializeContext::ClassData* classData = nullptr;

                bool isConvertedData = false;
                // read from the converted list (if we have something)
                if (convertedClassElement.m_classData != nullptr)
                {
                    if (convertedClassElementIndex < convertedClassElement.m_subElements.size())
                    {
                        convertedNode = &convertedClassElement.m_subElements[convertedClassElementIndex++];
                        classData = convertedNode->m_classData;
                        element = convertedNode->m_element;
                        isConvertedData = true;
                    }
                    else
                    {
                        convertedClassElement.m_classData = nullptr;
                        convertedClassElementIndex = 0;
                        while (!convertedClassElement.m_subElements.empty())
                        {
                            convertedClassElement.RemoveElement(static_cast<int>(convertedClassElement.m_subElements.size()) - 1);
                        }

                        // we have reached the end of our buffer, so exit the loop
                        break;
                    }
                }
                else // read from the stream
                {
                    if (!ReadElement(*m_sc, classData, element, parentClassInfo, nextLevel, parentClassInfo == nullptr))
                    {
                        // we have reached the end of this branch, so exit the loop
                        break;
                    }
                    nextLevel = false;
                }

                // Handle conversion of deprecated classes to non-deprecated ones.
                if (!convertedClassElement.m_classData && classData && classData->IsDeprecated())
                {
                    convertedClassElementIndex = 0;
                    convertedClassElement.m_element = element;
                    convertedClassElement.m_classData = classData;

                    bool converted = ConvertOldVersion(*m_sc, convertedClassElement, stream, classData);
                    if (!converted || convertedClassElement.m_classData == classData)
                    {
                        convertedClassElement.m_classData = nullptr;

                        if (classData->m_converter)
                        {
                            AZStd::string error = AZStd::string::format("Converter failed for element '%s'(0x%x) with deprecated class ID '%s'.",
                                element.m_name ? element.m_name : "NULL", element.m_nameCrc, element.m_id.ToString<AZStd::string>().c_str());
                            
                            m_errorLogger.ReportError(error.c_str());
                            
                            result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                        }

                        SkipElement();
                        element.m_dataSize = static_cast<size_t>(stream.GetCurPos());
                        continue;
                    }
                    
                    classData = convertedClassElement.m_classData;
                    element = convertedClassElement.m_element;
                    isConvertedData = true;
                }

                // If classData is NULL then we failed to find a registration for this element
                // It may be simply stale data that's safe to drop or someone forgot to register
                // this type. If the class is deprecated, but was not converted, then our only choice
                // is to toss the data.
                if (classData == nullptr || classData->IsDeprecated())
                {

                    if (classData == nullptr)
                    {
                        if ((m_filterDesc.m_flags & FILTERFLAG_IGNORE_UNKNOWN_CLASSES) == 0)
                        {
                            // we're not ignoring unknown classes.
                            AZStd::string error = AZStd::string::format(
                                "Element '%s'(0x%x) with class ID '%s' found in '%s' is not registered with the serializer!\n"
                                "If this class was removed, consider using serializeContext->ClassDeprecate(...) to register classes as having been deprecated to avoid this error\n",
                                element.m_name ? element.m_name : "NULL", element.m_nameCrc, element.m_id.ToString<AZStd::string>().c_str(), parentClassInfo ? parentClassInfo->m_name : "ROOT");
                            m_errorLogger.ReportError(error.c_str());

                            // its not just a deprecated class.  Its unregistered
                            result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                        }
                    }

                    if (!isConvertedData)
                    {
                        SkipElement();
                    }
                    element.m_dataSize = static_cast<size_t>(stream.GetCurPos());
                    continue;
                }

                // If the current node is been overlayed, get the data and patch into the current hierarchy
                // before continuing.
                // For now, we will simulate a data conversion to inject the data back into the data.
                if (element.m_id == SerializeTypeInfo<DataOverlayInfo>::GetUuid())
                {
                    DataOverlayInfo overlay;
                    result = LoadClass(stream, *convertedNode, classData, &overlay, flags) && result;
                    SerializeContext::DataElementNode overlaidNode;
                    overlaidNode.m_classData = parentClassInfo;
                    overlaidNode.m_element.m_stream = &stream;
                    DataOverlayTarget data(&overlaidNode, m_sc, &m_errorLogger);

                    EBUS_EVENT_ID(overlay.m_providerId, DataOverlayProviderBus, FillOverlayData, &data, overlay.m_dataToken);
                    if (overlaidNode.GetNumSubElements() > 0)
                    {
                        AZ_Assert(overlaidNode.GetNumSubElements() == 1, "Only one node should ever be returned by the overlay provider!");
                        overlaidNode.GetSubElement(0).m_element.m_name = element.m_name;
                        overlaidNode.GetSubElement(0).m_element.m_nameCrc = element.m_nameCrc;
                        overlaidNode.GetSubElement(0).m_element.m_byteStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        result = LoadClass(overlaidNode.GetSubElement(0).m_element.m_byteStream, overlaidNode, overlaidNode.m_classData, parentClassPtr, flags) && result;
                    }
                    continue;   // since we manually processed this node, there is no need to execute the rest of the code.
                }

                // If we are not a root object, find the correct ClassElement
                // that the new node represents.
                // The node needs to match the name and type of some ClassElement at the current level in the
                // reflected hierarchy.
                // If it is a pointer type, the node could be of a derived type!
                const SerializeContext::ClassElement* classElement = nullptr;
                SerializeContext::IDataContainer* classContainer = nullptr;
                SerializeContext::ClassElement dynamicElementMetadata;  // we'll point to this if we are loading a DynamicSerializableField
                if (parentClassInfo)
                {
                    if (parentClassInfo->m_container)
                    {
                        classContainer = parentClassInfo->m_container;
                        // Use the dynamicElementMetadata object to store the ClassElement into
                        bool classElementFound = classContainer->GetElement(dynamicElementMetadata, element);
                        AZ_Assert(classElementFound, "'%s'(0x%x) is not a valid element name for container type %s", element.m_name ? element.m_name : "NULL", element.m_nameCrc, parentClassInfo->m_name);
                        if (classElementFound)
                        {
                            classElement = &dynamicElementMetadata;
                            // if the container contains pointers, then the elements could be a derived type,
                            // otherwise we need the uuids to be exactly the same.
                            if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                            {
                                if (!m_sc->CanDowncast(element.m_id, classElement->m_typeId, classData->m_azRtti, classElement->m_azRtti))
                                {
                                    AZStd::string error = AZStd::string::format("Element of type %s cannot be added to container of pointers to type %s!"
                                        , element.m_id.ToString<AZStd::string>().c_str(), classElement->m_typeId.ToString<AZStd::string>().c_str());
                                    

                                    result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                                    m_errorLogger.ReportError(error.c_str());

                                    classElement = nullptr;
                                }
                            }
                            else
                            {
                                if (element.m_id != classElement->m_typeId)
                                {
                                    AZStd::string error = AZStd::string::format("Element of type %s cannot be added to container of type %s!"
                                        , element.m_id.ToString<AZStd::string>().c_str(), classElement->m_typeId.ToString<AZStd::string>().c_str());
                                    
                                    result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                                    m_errorLogger.ReportError(error.c_str());

                                    classElement = nullptr;
                                }
                            }
                        }
                    }
                    else if (parentClassInfo->m_typeId == SerializeTypeInfo<DynamicSerializableField>::GetUuid() && element.m_nameCrc == AZ_CRC("m_data", 0x335cc942))   // special case for dynamic-typed fields
                    {
                        DynamicSerializableField* fieldContainer = reinterpret_cast<DynamicSerializableField*>(parentClassPtr);
                        fieldContainer->m_typeId = classData->m_typeId;
                        dynamicElementMetadata.m_name = element.m_name;
                        dynamicElementMetadata.m_nameCrc = element.m_nameCrc;
                        dynamicElementMetadata.m_offset = reinterpret_cast<size_t>(&(reinterpret_cast<DynamicSerializableField const volatile*>(0)->m_data));
                        dynamicElementMetadata.m_dataSize = sizeof(void*);
                        dynamicElementMetadata.m_flags = SerializeContext::ClassElement::FLG_POINTER;   // we want to load the data as a pointer
                        dynamicElementMetadata.m_azRtti = classData->m_azRtti;
                        dynamicElementMetadata.m_genericClassInfo = m_sc->FindGenericClassInfo(element.m_id);
                        dynamicElementMetadata.m_typeId = fieldContainer->m_typeId;
                        classElement = &dynamicElementMetadata;
                    }
                    else
                    {
                        for (size_t i = 0; i < parentClassInfo->m_elements.size(); ++i)
                        {
                            const SerializeContext::ClassElement* childElement = &parentClassInfo->m_elements[i];
                            if (childElement->m_nameCrc == element.m_nameCrc)
                            {
                                // if the member is a pointer type, then the pointer could be a derived type,
                                // otherwise we need the uuids to be exactly the same.
                                if (childElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                                {
                                    if (m_sc->CanDowncast(element.m_id, childElement->m_typeId, classData->m_azRtti, childElement->m_azRtti))
                                    {
                                        classElement = childElement;
                                    }
                                    else
                                    {
                                        // Name matched but wrong type, this is an error when conversion function is not supplied.
                                        AZStd::string error = AZStd::string::format("Element '%s'(0x%x) in class '%s' is of type %s and cannot be downcasted to type %s.", 
                                                element.m_name ? element.m_name : "NULL", element.m_nameCrc, parentClassInfo->m_name,
                                                element.m_id.ToString<AZStd::string>().c_str(), childElement->m_typeId.ToString<AZStd::string>().c_str());
                                        
                                        result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                                        m_errorLogger.ReportError(error.c_str());
                                    }
                                }
                                else
                                {
                                    if (element.m_id == childElement->m_typeId)
                                    {
                                        classElement = childElement;
                                    }
                                    else
                                    {
                                        // Name matched but wrong type, this is an error when conversion function is not supplied.
                                        AZStd::string error = AZStd::string::format("Element '%s'(0x%x) in class '%s' is of type %s but needs to be type %s.", 
                                                element.m_name ? element.m_name : "NULL", element.m_nameCrc, parentClassInfo->m_name,
                                                element.m_id.ToString<AZStd::string>().c_str(), childElement->m_typeId.ToString<AZStd::string>().c_str());
                                        
                                        result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                                        m_errorLogger.ReportError(error.c_str());
                                    }
                                }
                                break;
                            }
                        }

                        // If we can't resolve classElement while looking into members of a containing class, issue a warning.
                        // We can continue safely, but this constitutes loss of old data that users should be aware of.
                        if (classElement == nullptr)
                        {
                            AZStd::string error = AZStd::string::format("Element '%s'(0x%x) of type %s is not registered as part of class '%s'. Data will be discarded.", 
                                element.m_name ? element.m_name : "NULL", element.m_nameCrc, element.m_id.ToString<AZStd::string>().c_str(), parentClassInfo->m_name);
                            
                            if (m_filterDesc.m_flags & FILTERFLAG_STRICT)
                            {
                                m_errorLogger.ReportError(error.c_str());
                                result = false;
                            }
                            else
                            {
                                m_errorLogger.ReportWarning(error.c_str());
                            }
                        }
                    }

                    if (classElement == nullptr)
                    {
                        if (!isConvertedData)
                        {
                            SkipElement();
                        }
                        element.m_dataSize = static_cast<size_t>(element.m_stream->GetCurPos());
                        continue;
                    }
                }

                void* dataAddress = nullptr;
                void* reserveAddress = nullptr; // Stores the dataAddress from IDataContainer::ReserveElement

                // Handle version conversions for non-custom serialized classes
                if (element.m_version < classData->m_version && !classData->m_serializer)
                {
                    AZ_Assert(convertedClassElement.m_classData == nullptr, "We can't convert a class inside a class!");
                    convertedClassElementIndex = 0;
                    convertedClassElement.m_element = element;
                    convertedClassElement.m_classData = classData;

                    bool converted = ConvertOldVersion(*m_sc, convertedClassElement, stream, classData);
                    bool illegalClassChange = (classData != convertedClassElement.m_classData);
                    if (!converted || illegalClassChange)
                    {
                        if (illegalClassChange)
                        {
                            AZStd::string error = AZStd::string::format("Converter changed element from type %s to type %s. Version converters cannot change the base type. "
                                "Instead use DeprecateClass for the old Uuid, provide a converter, and use that converter to designate a new type/uuid.",
                                convertedClassElement.m_classData->m_typeId.ToString<AZStd::string>().c_str(), classData->m_typeId.ToString<AZStd::string>().c_str());
                            
                            result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                            m_errorLogger.ReportError(error.c_str());
                        }

                        convertedClassElement.m_classData = nullptr;
                        while (!convertedClassElement.m_subElements.empty())
                        {
                            convertedClassElement.RemoveElement(convertedClassElement.GetNumSubElements() - 1);
                        }
                        continue; // go to next element
                    }
                }
                else if (element.m_version > classData->m_version)
                {
                    // This is a future version so we don't know how to read this. Skip it, but preserve the data!
                    AZ_Warning("Serialization", false, 
                        "Data element %s is version %u, but reflected field is version %u. "
                        "Serializer will attempt to load, but forward-compatibility is not guaranteed.",
                        element.m_name ? element.m_name : classData->m_name,
                        element.m_version, classData->m_version);
                }

                if (classElement)
                {
                    // ClassData can be influenced by converters. Verify the classData is compatible with the underlying class element.
                    if (classData->m_typeId != classElement->m_typeId)
                    {
                        if (!m_sc->CanDowncast(classData->m_typeId, classElement->m_typeId, classData->m_azRtti, classElement->m_azRtti))
                        {
                            AZStd::string error = AZStd::string::format("Converter switched to type %s, which cannot be casted to base type %s.",
                                classData->m_typeId.ToString<AZStd::string>().c_str(), classElement->m_typeId.ToString<AZStd::string>().c_str());
                            
                            result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                            m_errorLogger.ReportError(error.c_str());

                            continue; // go to next element
                        }
                    }

                    if (classContainer) // container elements
                    {
                        // add element to the array
                        if (classContainer->CanAccessElementsByIndex() && classContainer->Size(parentClassPtr) > currentContainerElementIndex)
                        {
                            dataAddress = classContainer->GetElementByIndex(parentClassPtr, classElement, currentContainerElementIndex);
                        }
                        else if(parentClassPtr)
                        {
                            dataAddress = classContainer->ReserveElement(parentClassPtr, classElement);
                        }

                        if (dataAddress == nullptr)
                        {
                            AZStd::string error = AZStd::string::format("Failed to reserve element in container. The container may be full. Element %u will not be added to container.", static_cast<unsigned int>(currentContainerElementIndex));
                            
                            result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                            m_errorLogger.ReportError(error.c_str());
                            continue; // go to next element
                        }

                        currentContainerElementIndex++;
                    }
                    else
                    {   // normal elements
                        dataAddress = reinterpret_cast<char*>(parentClassPtr) + classElement->m_offset;
                    }

                    reserveAddress = dataAddress;
                    // create a new instance if needed
                    if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                    {
                        // create a new instance if we are referencing it by pointer
                        AZ_Assert(classData->m_factory != nullptr, "We are attempting to create '%s', but no factory is provided! Either provide factory or change data member '%s' to value not pointer!", classData->m_name, classElement->m_name);

                        // If there is a value stored at the data address
                        // already, destroy it. This prevents leaks where the
                        // default constructor of object A allocates an object
                        // B and stores B in a field in A that is also
                        // serialized.
                        if (!classContainer && *reinterpret_cast<void**>(dataAddress))
                        {
                            classData->m_factory->Destroy(*reinterpret_cast<void**>(dataAddress));
                        }

                        void* newDataAddress = classData->m_factory->Create(classData->m_name);

                        // we need to account for additional offsets if we have a pointer to
                        // a base class.
                        void* basePtr = m_sc->DownCast(newDataAddress, element.m_id, classElement->m_typeId, classData->m_azRtti, classElement->m_azRtti);
                        AZ_Assert(basePtr != nullptr, classContainer
                            ? "Can't cast container element %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                            : "Can't cast %s(0x%x) to %s, make sure classes are registered in the system and not generics!"
                            , element.m_name ? element.m_name : "NULL"
                            , element.m_nameCrc
                            , classData->m_name);

                        *reinterpret_cast<void**>(dataAddress) = basePtr; // store the pointer in the class

                        // further construction of the members need to be based off the pointer to the
                        // actual type, not the base type!
                        dataAddress = newDataAddress;
                    }
                }
                else
                {
                    // If we ended up here then we are a root object
                    if (m_inplaceLoadInfoCB) // user can provide function for inplace loading.
                    {
                        dataAddress = nullptr;
                        m_inplaceLoadInfoCB(&dataAddress, nullptr, element.m_id, m_sc);
                    }

                    if (!dataAddress)
                    {
                        // Call the supplied creator to allocate the construct the object and push it into the process queue
                        AZ_Assert(classData->m_factory != nullptr, "We are attempting to create '%s', but no constructor is provided!", classData->m_name);
                        dataAddress = classData->m_factory->Create(classData->m_name);
                        if (dataAddress == nullptr)
                        {
                            AZ_Assert(false, "Creator for class %s(%s) returned a NULL pointer!", classData->m_name, element.m_id.ToString<AZStd::string>().c_str());
                        }
                    }

                    reserveAddress = dataAddress;
                }

#if defined(AZ_ENABLE_TRACING)
                {
                    SerializeContext::DbgStackEntry de;
                    de.m_dataPtr = dataAddress;
                    de.m_uuidPtr = &element.m_id;
                    de.m_elementName = element.m_name;
                    de.m_classData = classData;
                    de.m_classElement = classElement;
                    m_errorLogger.Push(de);
                }
#endif // AZ_ENABLE_TRACING

                if (classData->m_eventHandler)
                {
                    classData->m_eventHandler->OnWriteBegin(dataAddress);
                }

                if (element.m_id == GetAssetClassId())
                {
                    AZ_Assert(dataAddress, "Reference field address is invalid");
                    AZ_Assert(classData->m_serializer, "Asset references should always have a serializer defined");

                    // Intercept asset references so we can forward asset load filter information.
                    static_cast<AssetSerializer*>(classData->m_serializer)->LoadWithFilter(
                        dataAddress,
                        *element.m_stream,
                        element.m_version,
                        m_filterDesc.m_assetCB,
                        element.m_dataType == SerializeContext::DataElement::DT_BINARY_BE);

                }
                // Serializable leaf element.
                else if (classData->m_serializer)
                {
                    // Wrap the stream
                    IO::GenericStream* currentStream = &m_inStream;
                    IO::MemoryStream memStream(m_inStream.GetData()->data(), 0, static_cast<size_t>(m_inStream.GetCurPos()));
                    currentStream = &memStream;

                    if (element.m_byteStream.GetLength() > 0)
                    {
                        currentStream = &element.m_byteStream;
                    }

                    currentStream->Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                    // read the class value
                    if (dataAddress == nullptr || 
                        !classData->m_serializer->Load(dataAddress, *currentStream, element.m_version, element.m_dataType == SerializeContext::DataElement::DT_BINARY_BE))
                    {
                        result = result && ((m_filterDesc.m_flags & FILTERFLAG_STRICT) == 0);  // in strict mode, this is a complete failure.
                    }
                }

                // If it is a container, clear it before loading the child
                // nodes, otherwise we end up with more elements than the ones
                // we should have
                if (classData->m_container && dataAddress)
                {
                    classData->m_container->ClearElements(dataAddress, m_sc);
                }

                // Read child nodes
                result = LoadClass(stream, *convertedNode, classData, dataAddress, flags) && result;

                if (classContainer)
                {
                    classContainer->StoreElement(parentClassPtr, reserveAddress);
                }
                else if (!parentClassPtr)
                {
                    if (m_readyCB)
                    {
                        m_readyCB(dataAddress, element.m_id, m_sc);
                    }
                }

                if (classData->m_eventHandler)
                {
                    classData->m_eventHandler->OnWriteEnd(dataAddress);
                }

#if defined(AZ_ENABLE_TRACING)
                m_errorLogger.Pop();
#endif // AZ_ENABLE_TRACING
            }

            return result;
        }

        //=========================================================================
        // ReadElement
        // [4/19/2012]
        //=========================================================================
        bool
        ObjectStreamImpl::ReadElement(SerializeContext& sc, const SerializeContext::ClassData*& cd, SerializeContext::DataElement& element, const SerializeContext::ClassData* parent, bool nextLevel, bool isTopElement)
        {
            AZ_Assert(element.m_stream != nullptr, "You must provide a stream to store the values!");
            element.m_version = 0;
            element.m_name = nullptr;
            element.m_nameCrc = 0;
            element.m_dataSize = 0;
            element.m_id = AZ::Uuid::CreateNull();

            cd = nullptr;

            if (GetType() == ST_XML)
            {
                const char* values = nullptr;
                AZ_Assert(m_xmlNode, "There are no more nodes to read!");
                // The current node is the last node read, so
                // first advance to next node
                rapidxml::xml_node<char>* next;
                if (nextLevel)
                {
                    next = m_xmlNode->first_node();
                }
                else
                {
                    next = m_xmlNode->next_sibling();
                    if (!next)
                    {
                        // we are done at this level so back up
                        m_xmlNode = m_xmlNode->parent();
                        AZ_Assert(m_xmlNode, "We should always have a parent!");
                    }
                }
                if (!next)
                {
                    return false;
                }
                m_xmlNode = next;

                Uuid specializedId;
                // now parse the node
                rapidxml::xml_attribute<char>* attr = m_xmlNode->first_attribute();
                while (attr)
                {
                    // "name" used to store what's now in the "field" attribute while "name" is only used for readability
                    // to display the class name. Because at the time of the conversion of the XML the version (m_version) 
                    // was not incremented, it has become impossible to know which version is being loaded. To address this, 
                    // the "field" attribute is used by default, unless it's not found in which case the legacy "name" field is used.
                    bool isField = strcmp(attr->name(), "field") == 0;
                    if (strcmp(attr->name(), "name") == 0 || isField)
                    {
                        if (element.m_name == nullptr || isField)
                        {
                            element.m_name = attr->value();
                            element.m_nameCrc = Crc32(element.m_name);
                        }
                    }
                    else if (strcmp(attr->name(), "type") == 0)
                    {
                        element.m_id = Uuid(attr->value());
                    }
                    else if (strcmp(attr->name(), "value") == 0)
                    {
                        values = attr->value();
                    }
                    else if (strcmp(attr->name(), "version") == 0)
                    {
                        azsscanf(attr->value(), "%u", &element.m_version);
                    }
                    else if (m_version == 2 && strcmp(attr->name(), "specializationTypeId") == 0)
                    {
                        // Version 3 of the ObjectStream serializes the specialized type id directly in the data element id field.
                        specializedId = Uuid(attr->value());   
                    }
                    attr = attr->next_attribute();
                }

                // The Asset ClassId is handled directly within the LoadClass function so don't replace it
                if (m_version == 2 && element.m_id != GetAssetClassId())
                {
                    if (parent && parent->m_container)
                    {
                        const SerializeContext::ClassElement* classElement = parent->m_container->GetElement(element.m_nameCrc);
                        if (classElement && classElement->m_genericClassInfo)
                        {
                            if (classElement->m_genericClassInfo->CanStoreType(specializedId))
                            {
                                specializedId = classElement->m_genericClassInfo->GetSpecializedTypeId();
                            }
                        }

                    }
                    element.m_id = specializedId;
                }
 
                // find the registered class data
                cd = sc.FindClassData(element.m_id, parent, element.m_nameCrc);

                if (m_version <= 1 && cd)
                {
                    // For version 1 the stored typeId was statically associated with the GenericClassInfo, which is now the generic type id
                    // Therefore the GenericClassInfo is looked up using the generic typeId on the ClassData object
                    // Afterwards the element.m_id field is updated with the specialized type id
                    if (GenericClassInfo* genericClassInfo = sc.FindGenericClassInfo(cd->m_typeId))
                    {
                        element.m_id = genericClassInfo->GetSpecializedTypeId();
                    }
                }

                // Root elements may require classInfo to be provided by the in-place load callback.
                if (!cd && isTopElement && m_inplaceLoadInfoCB)
                {
                    m_inplaceLoadInfoCB(nullptr, &cd, element.m_id, &sc);
                }

                element.m_dataSize = 0;
                element.m_stream->Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                if (values)
                {
                    if (cd && !cd->IsDeprecated() && cd->m_serializer)
                    {
                        element.m_dataSize = cd->m_serializer->TextToData(values, element.m_version, *element.m_stream);
                        element.m_dataType = SerializeContext::DataElement::DT_BINARY;
                    }
                    else
                    {
                        // Can't convert the data because there is no ClassData*.
                        // Store the raw data and hope that someone knows what to do with it.
                        element.m_dataSize = strlen(values);

                        if (element.m_dataSize)
                        {
                            element.m_stream->Write(element.m_dataSize, values);
                        }

                        element.m_dataType = SerializeContext::DataElement::DT_TEXT;
                    }
                }
            }
            else if (GetType() == ST_JSON)
            {
                const char* values = nullptr;
                // The current node is the last node read, so
                // first advance to next node
                rapidjson::Value* currentElement = nullptr;
                JSonReadNode& currentNode = m_jsonReadValues.back();
                if (nextLevel)
                {
                    // Object array first element
                    auto memberIt = currentNode.m_currentObject->FindMember("Objects");
                    if (memberIt != currentNode.m_currentObject->MemberEnd())
                    {
                        currentElement = memberIt->value.Begin();
                        m_jsonReadValues.push_back(JSonReadNode(&memberIt->value, currentElement));
                    }
                    else
                    {
                        // no more levels to go
                        return false;
                    }
                }
                else
                {
                    currentElement = ++currentNode.m_currentObject;
                    if (currentElement == currentNode.m_parentObjectArray->End())
                    {
                        // we are done with this level
                        m_jsonReadValues.pop_back();
                        return false;
                    }
                }


                // now parse the node
                // element name
                auto valueIt = currentElement->FindMember("field");
                if (valueIt != currentElement->MemberEnd())
                {
                    element.m_name = valueIt->value.GetString();
                    element.m_nameCrc = Crc32(element.m_name);
                }
                valueIt = currentElement->FindMember("typeId");
                if (valueIt != currentElement->MemberEnd())
                {
                    element.m_id = Uuid(valueIt->value.GetString());
                }
                else
                {
                    AZ_Error("Serialization", valueIt != currentElement->MemberEnd(), "All objects should have a typeId, loading aborted!");
                    return false;
                }
                
                // Version 3 of the ObjectStream serializes the specialized type id directly in the data element id field. The data element old id field value is no longer needed
                if (m_version == 2)
                {
                    valueIt = currentElement->FindMember("specializationTypeId");
                    if (valueIt != currentElement->MemberEnd() && element.m_id != GetAssetClassId())
                    {
                        // The Asset ClassId is handled directly within the LoadClass function
                        Uuid specializedId(valueIt->value.GetString());
                        if (parent && parent->m_container)
                        {
                            const SerializeContext::ClassElement* classElement = parent->m_container->GetElement(element.m_nameCrc);
                            if (classElement && classElement->m_genericClassInfo)
                            {
                                if (classElement->m_genericClassInfo->CanStoreType(specializedId))
                                {
                                    specializedId = classElement->m_genericClassInfo->GetSpecializedTypeId();
                                }
                            }
                        }
                        element.m_id = specializedId;
                    }
                }
                valueIt = currentElement->FindMember("version");
                if (valueIt != currentElement->MemberEnd())
                {
                    element.m_version = valueIt->value.GetUint();
                }
                valueIt = currentElement->FindMember("value");
                if (valueIt != currentElement->MemberEnd())
                {
                    values = valueIt->value.GetString();
                }

                // find the registered class data
                cd = sc.FindClassData(element.m_id, parent, element.m_nameCrc);
                if (m_version <= 1 && cd)
                {
                    // For version 1 the stored typeId was statically associated with the GenericClassInfo, which is now the generic type id
                    // Therefore the GenericClassInfo is looked up using the generic typeId on the ClassData object
                    // Afterwards the element.m_id field is updated with the specialized type id
                    if (GenericClassInfo* genericClassInfo = sc.FindGenericClassInfo(cd->m_typeId))
                    {
                        element.m_id = genericClassInfo->GetSpecializedTypeId();
                    }
                }
                // Root elements may require classInfo to be provided by the in-place load callback.
                if (!cd && isTopElement && m_inplaceLoadInfoCB)
                {
                    m_inplaceLoadInfoCB(nullptr, &cd, element.m_id, &sc);
                }

                element.m_dataSize = 0;
                element.m_stream->Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                if (values)
                {
                    if (cd && !cd->IsDeprecated() && cd->m_serializer)
                    {
                        element.m_dataSize = cd->m_serializer->TextToData(values, element.m_version, *element.m_stream);
                        element.m_dataType = SerializeContext::DataElement::DT_BINARY;
                    }
                    else
                    {
                        // Can't convert the data because there is no ClassData*.
                        // Store the raw data and hope that someone knows what to do with it.
                        element.m_dataSize = strlen(values);

                        if (element.m_dataSize)
                        {
                            element.m_stream->Write(element.m_dataSize, values);
                        }
                        element.m_dataType = SerializeContext::DataElement::DT_TEXT;
                    }
                }
            }
            else /*ST_BINARY*/
            {
                if (m_stream->GetCurPos() == m_stream->GetLength())
                {
                    // Reached the end of the stream. We may reach this state if we just skipped the root element
                    return false;
                }

                // Read flags
                u8 flagsSize = 0;
                IO::SizeType nBytesRead = m_stream->Read(sizeof(u8), &flagsSize);
                AZ_Assert(nBytesRead == sizeof(u8), "Failed trying to read binary element tag!");
                (void)nBytesRead;
                if (flagsSize == ST_BINARYFLAG_ELEMENT_END)
                {
                    return false;
                }

                // Read name
                if (flagsSize & ST_BINARYFLAG_HAS_NAME)
                {
                    u32 nameCrc;
                    nBytesRead = m_stream->Read(sizeof(nameCrc), &nameCrc);
                    AZ_Assert(nBytesRead == sizeof(nameCrc), "Failed trying to read binary element nameCrc!");
#if !defined(AZ_BIG_ENDIAN)
                    AZStd::endian_swap(nameCrc);
#endif
                    element.m_nameCrc = nameCrc;
                }

                // Read version
                if (flagsSize & ST_BINARYFLAG_HAS_VERSION)
                {
                    u8 version;
                    nBytesRead = m_stream->Read(sizeof(version), &version);
                    AZ_Assert(nBytesRead == sizeof(version), "Failed trying to read binary element version!");
                    element.m_version = version;
                }

                // Read uuid
                nBytesRead = m_stream->Read(element.m_id.end() - element.m_id.begin(), element.m_id.begin());
                AZ_Assert(nBytesRead == static_cast<IO::SizeType>(element.m_id.end() - element.m_id.begin()), "Failed trying to read binary element uuid!");

                // Version 3 of the ObjectStream serializes the specialized type id directly in the data element id field. The data element old id field value is no longer needed
                if (m_version == 2)
                {
                    Uuid specializedId;
                    nBytesRead = m_stream->Read(specializedId.end() - specializedId.begin(), specializedId.begin());
                    AZ_Assert(nBytesRead == static_cast<IO::SizeType>(specializedId.end() - specializedId.begin()), "Failed trying to read binary class element uuid");

                    // The Asset ClassId is handled directly within the LoadClass function
                    if (element.m_id != GetAssetClassId())
                    {
                        if (parent && parent->m_container)
                        {
                            const SerializeContext::ClassElement* classElement = parent->m_container->GetElement(element.m_nameCrc);
                            if (classElement && classElement->m_genericClassInfo)
                            {
                                if (classElement->m_genericClassInfo->CanStoreType(specializedId))
                                {
                                    specializedId = classElement->m_genericClassInfo->GetSpecializedTypeId();
                                }
                            }
                        }
                        element.m_id = specializedId;
                    }
                }

                element.m_dataType = SerializeContext::DataElement::DT_BINARY_BE;


                // find the registered class data
                cd = sc.FindClassData(element.m_id, parent, element.m_nameCrc);
                if (m_version <= 1 && cd)
                {
                    // For version 1 the stored typeId was statically associated with the GenericClassInfo, which is now the generic type id
                    // Therefore the GenericClassInfo is looked up using the generic typeId on the ClassData object
                    // Afterwards the element.m_id field is updated with the specialized type id
                    if (GenericClassInfo* genericClassInfo = sc.FindGenericClassInfo(cd->m_typeId))
                    {
                        element.m_id = genericClassInfo->GetSpecializedTypeId();
                    }
                }

                // Root elements may require classInfo to be provided by the in-place load callback.
                if (!cd && isTopElement && m_inplaceLoadInfoCB)
                {
                    m_inplaceLoadInfoCB(nullptr, &cd, element.m_id, &sc);
                }

                // Read value
                if (flagsSize & ST_BINARYFLAG_HAS_VALUE)
                {
                    size_t valueBytes = static_cast<size_t>(flagsSize & ST_BINARY_VALUE_SIZE_MASK);
                    if (flagsSize & ST_BINARYFLAG_EXTRA_SIZE_FIELD)
                    {
                        switch (valueBytes)
                        {
                        case 1:
                        {
                            u8 size;
                            nBytesRead = m_stream->Read(sizeof(u8), &size);
                            AZ_Assert(nBytesRead == sizeof(u8), "Failed trying to read extra size field!");
                            valueBytes = size;
                            break;
                        }
                        case 2:
                        {
                            u16 size;
                            nBytesRead = m_stream->Read(sizeof(u16), &size);
                            AZ_Assert(nBytesRead == sizeof(u16), "Failed trying to read extra size field!");
#if !defined(AZ_BIG_ENDIAN)
                            AZStd::endian_swap(size);
#endif
                            valueBytes = size;
                            break;
                        }
                        case 4:
                        {
                            u32 size;
                            nBytesRead = m_stream->Read(sizeof(u32), &size);
                            AZ_Assert(nBytesRead == sizeof(u32), "Failed trying to read extra size field!");
#if !defined(AZ_BIG_ENDIAN)
                            AZStd::endian_swap(size);
#endif
                            valueBytes = size;
                            break;
                        }
                        default:
                            AZ_Assert(false, "Invalid number of bytes for value size field! (%llu)", (u64)valueBytes);
                        }
                    }

                    element.m_dataSize = valueBytes;
                    element.m_stream->Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    if (element.m_dataSize)
                    {
                        void* data = m_scratchSpace.data();
                        if (element.m_dataSize > m_scratchSpace.capacity())
                        {
                            data = azmalloc(valueBytes);
                        }
                        
                        nBytesRead = m_stream->Read(valueBytes, data);
                        element.m_stream->Write(valueBytes, data);

                        if (data != m_scratchSpace.data())
                        {
                            azfree(data);
                        }
                        AZ_Assert(nBytesRead == valueBytes, "Failed trying to read binary element value!");
                    }
                }
                else
                {
                    element.m_dataSize = 0;
                }
            }
            return true;
        }

        //=========================================================================
        // SkipElement
        // [1/19/2013]
        //=========================================================================
        void ObjectStreamImpl::SkipElement()
        {
            if (GetType() == ST_BINARY)
            {
                int endTagsNeeded = 1;
                while (endTagsNeeded > 0)
                {
                    // Read flags
                    u8 flagsSize = 0;
                    IO::SizeType nBytesRead = m_stream->Read(sizeof(u8), &flagsSize);
                    AZ_Assert(nBytesRead == sizeof(u8), "Failed trying to read binary element tag!");
                    (void)nBytesRead;
                    if (flagsSize == ST_BINARYFLAG_ELEMENT_END)
                    {
                        --endTagsNeeded;
                    }
                    else
                    {
                        ++endTagsNeeded;
                        size_t bytesToSkip = sizeof(Uuid);  // this field is guaranteed to be there
                        if (flagsSize & ST_BINARYFLAG_HAS_NAME)
                        {
                            bytesToSkip += sizeof(u32);
                        }
                        if (flagsSize & ST_BINARYFLAG_HAS_VERSION)
                        {
                            bytesToSkip += sizeof(u8);
                        }

                        if (m_version == 2) // need to account for the specialized uuid
                        {
                            bytesToSkip += sizeof(Uuid);
                        }

                        m_stream->Seek(bytesToSkip, IO::GenericStream::ST_SEEK_CUR);

                        if (flagsSize & ST_BINARYFLAG_HAS_VALUE)
                        {
                            bytesToSkip = static_cast<size_t>(flagsSize & ST_BINARY_VALUE_SIZE_MASK);
                            if (flagsSize & ST_BINARYFLAG_EXTRA_SIZE_FIELD)
                            {
                                switch (bytesToSkip)
                                {
                                case 1:
                                {
                                    u8 size;
                                    nBytesRead = m_stream->Read(sizeof(u8), &size);
                                    AZ_Assert(nBytesRead == sizeof(u8), "Failed trying to read extra size field!");
                                    bytesToSkip = size;
                                    break;
                                }
                                case 2:
                                {
                                    u16 size;
                                    nBytesRead = m_stream->Read(sizeof(u16), &size);
                                    AZ_Assert(nBytesRead == sizeof(u16), "Failed trying to read extra size field!");
#if !defined(AZ_BIG_ENDIAN)
                                    AZStd::endian_swap(size);
#endif
                                    bytesToSkip = size;
                                    break;
                                }
                                case 4:
                                {
                                    u32 size;
                                    nBytesRead = m_stream->Read(sizeof(u32), &size);
                                    AZ_Assert(nBytesRead == sizeof(u32), "Failed trying to read extra size field!");
#if !defined(AZ_BIG_ENDIAN)
                                    AZStd::endian_swap(size);
#endif
                                    bytesToSkip = size;
                                    break;
                                }
                                default:
                                    AZ_Assert(false, "Invalid number of bytes to skip! (%llu)", (u64)bytesToSkip);
                                }
                            }
                            m_stream->Seek(bytesToSkip, IO::GenericStream::ST_SEEK_CUR);
                        }
                    }
                }
            }
        }

        //=========================================================================
        // WriteClass
        // [6/22/2012]
        //=========================================================================
        bool ObjectStreamImpl::WriteClass(const void* classPtr, const Uuid& classId, const SerializeContext::ClassData* classData)
        {
            m_errorLogger.Reset();

            SerializeContext::EnumerateInstanceCallContext callContext(
                AZStd::bind(&ObjectStreamImpl::WriteElement, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3),
                AZStd::bind(&ObjectStreamImpl::CloseElement, this),
                m_sc,
                SerializeContext::ENUM_ACCESS_FOR_READ,
                &m_errorLogger
            );

            m_sc->EnumerateInstanceConst(
                  &callContext
                , classPtr
                , classId
                , classData
                , nullptr
                );

            return m_errorLogger.GetErrorCount() == 0;
        }

        //=========================================================================
        // WriteElement
        // [10/25/2012]
        //=========================================================================
        bool ObjectStreamImpl::WriteElement(const void* ptr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement)
        {
            AZ_Assert(classData, "We were handed a non-serializable object. This should never happen!");
            m_isOverlaying = false;
            const void* objectPtr = ptr;
            if (classElement)
            {
                // Data overlays are only supported for non-root elements, which means we should have a valid class element.
                DataOverlayInfo overlay;
                EBUS_EVENT_ID_RESULT(overlay, DataOverlayInstanceId(objectPtr, classElement->m_typeId), DataOverlayInstanceBus, GetOverlayInfo);
                if (overlay.m_providerId)
                {
                    const SerializeContext::ClassData* overlayClassMetadata = m_sc->FindClassData(SerializeTypeInfo<DataOverlayInfo>::GetUuid());
                    SerializeContext::ClassElement overlayElementMetadata;
                    overlayElementMetadata.m_name = classElement->m_name;
                    overlayElementMetadata.m_nameCrc = classElement->m_nameCrc;
                    overlayElementMetadata.m_flags = 0;
                    overlayElementMetadata.m_typeId = SerializeTypeInfo<DataOverlayInfo>::GetUuid();

                    SerializeContext::EnumerateInstanceCallContext callContext(
                        AZStd::bind(&ObjectStreamImpl::WriteElement, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3), 
                        AZStd::bind(&ObjectStreamImpl::CloseElement, this), 
                        m_sc,
                        SerializeContext::ENUM_ACCESS_FOR_READ,
                        &m_errorLogger
                    );

                    m_sc->EnumerateInstance(
                          &callContext
                        , &overlay
                        , overlayClassMetadata->m_typeId
                        , overlayClassMetadata
                        , &overlayElementMetadata
                        );
                    m_isOverlaying = true;
                    return false;
                }

                // if we are a pointer, then we may be pointing to a derived type.
                if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                {
                    // if dataAddress is a pointer in this case, cast it's value to a void* (or const void*) and dereference to get to the actual class.
                    objectPtr = *(void**)(ptr);
                    if (objectPtr && classElement->m_azRtti)
                    {
                        // if we are pointing to derived type, adjust pointer.
                        if (classData->m_typeId != classElement->m_typeId)
                        {
                            objectPtr = classElement->m_azRtti->Cast(objectPtr, classData->m_azRtti->GetTypeId());
                        }
                    }
                }
            }

            if (classData->m_doSave && !classData->m_doSave(objectPtr))
            {
                // the user chose to skip saving, we don't recommend this pattern as it can create asymetrical serialization
                return false;
            }

            SerializeContext::DataElement element;
            if (classElement)
            {
                element.m_name = classElement->m_name;
                element.m_nameCrc = classElement->m_nameCrc;
            }
            element.m_id = classData->m_typeId;
            element.m_version = classData->m_version;
            element.m_dataSize = 0;
            element.m_stream = &m_inStream;
            // Make sure we have serializer classes. Containers since they have 0..N child elements or not are not required to provide one.
            //AZ_Assert(!classData->m_elements.empty() || classData->m_serializer != nullptr || classData->m_container != nullptr,
            //    "Your leaf class '%s' as element '%s' must have a serialize interface or child elements. If you want to store empty class use 'SerializerForEmptyClass' in your class description !",classData->m_name,classElement ? classElement->m_name : "Root class");

            m_inStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

            if (classData->m_serializer)
            {
                element.m_dataSize = classData->m_serializer->Save(objectPtr, m_inStream, GetType() == ST_BINARY);
            }

            if (GetType() == ST_XML)
            {
                char buf[AZ_SERIALIZE_BINARY_STACK_BUFFER]; // used for temp string conversion
                rapidxml::xml_attribute<char>* attr;
                rapidxml::xml_node<char>* child = m_xmlDoc->allocate_node(rapidxml::node_element, m_xmlDoc->allocate_string("Class"));
                m_xmlNode->append_node(child);
                m_xmlNode = child;
                // class name (used for readability)
                attr = m_xmlDoc->allocate_attribute(m_xmlDoc->allocate_string("name"), m_xmlDoc->allocate_string(classData->m_name));
                m_xmlNode->append_attribute(attr);
                // element name
                if (element.m_name)
                {
                    attr = m_xmlDoc->allocate_attribute(m_xmlDoc->allocate_string("field"), m_xmlDoc->allocate_string(element.m_name));
                    m_xmlNode->append_attribute(attr);
                }
                // type
                element.m_id.ToString(buf, AZ_SERIALIZE_BINARY_STACK_BUFFER);
                attr = m_xmlDoc->allocate_attribute(m_xmlDoc->allocate_string("type"), m_xmlDoc->allocate_string(buf));
                m_xmlNode->append_attribute(attr);

                //value
                if (classData->m_serializer)
                {
                    auto currentsize = static_cast<size_t>(m_inStream.GetCurPos());
                    IO::MemoryStream memStream = IO::MemoryStream(m_inStream.GetData()->data(), 0, currentsize);
                    m_outStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    classData->m_serializer->DataToText(memStream, m_outStream, false);

                    char* rawText = static_cast<char*>(m_outStream.GetData()->data());
                    char* xmlString = m_xmlDoc->allocate_string(rawText, static_cast<size_t>(m_outStream.GetCurPos() + 1));
                    xmlString[m_outStream.GetCurPos()] = 0;

                    attr = m_xmlDoc->allocate_attribute(m_xmlDoc->allocate_string("value"), xmlString);
                    m_xmlNode->insert_attribute(m_xmlNode->last_attribute(), attr);
                }
                else
                {
                    AZ_Assert(element.m_dataSize == 0, "We can't serialize values for %s(0x%x), value=%s without a serializer to do DataToText()!", element.m_name ? element.m_name : "NULL", element.m_nameCrc, buf);
                }

                // version
                if (element.m_version)
                {
                    azsnprintf(buf, AZ_SERIALIZE_BINARY_STACK_BUFFER, "%u", element.m_version);
                    attr = m_xmlDoc->allocate_attribute(m_xmlDoc->allocate_string("version"), m_xmlDoc->allocate_string(buf));
                    m_xmlNode->insert_attribute(m_xmlNode->last_attribute(), attr);
                }
            }
            else if (GetType() == ST_JSON)
            {
                m_jsonWriteValues.push_back();
                rapidjson::Value& classObject = m_jsonWriteValues.back();
                classObject.SetObject();
                // element name
                if (element.m_name)
                {
                    classObject.AddMember("field", rapidjson::StringRef(element.m_name), m_jsonDoc->GetAllocator());
                }
                // typeName (just for readability)
                classObject.AddMember("typeName", rapidjson::StringRef(classData->m_name), m_jsonDoc->GetAllocator());
                // typeId
                char idBuffer[64];
                int idBufferLen = element.m_id.ToString(idBuffer, AZ_ARRAY_SIZE(idBuffer)) - 1;
                classObject.AddMember("typeId", rapidjson::Value(idBuffer, idBufferLen, m_jsonDoc->GetAllocator()), m_jsonDoc->GetAllocator());

                // version
                if (element.m_version)
                {
                    classObject.AddMember("version", element.m_version, m_jsonDoc->GetAllocator());
                }
                //value
                if (classData->m_serializer)
                {
                    auto currentsize = static_cast<size_t>(m_inStream.GetCurPos());
                    IO::MemoryStream memStream = IO::MemoryStream(m_inStream.GetData()->data(), 0, currentsize);
                    m_outStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                    classData->m_serializer->DataToText(memStream, m_outStream, false);

                    char* rawText = static_cast<char*>(m_outStream.GetData()->data());
                    classObject.AddMember("value", rapidjson::Value(rawText ? rawText : "", static_cast<rapidjson::SizeType>(m_outStream.GetCurPos()), m_jsonDoc->GetAllocator()), m_jsonDoc->GetAllocator());
                }
                else
                {
                    AZ_Assert(element.m_dataSize == 0, "We can't serialize values for %s(0x%x), value=%s without a serializer to do DataToText()!", element.m_name ? element.m_name : "NULL", element.m_nameCrc, idBuffer);
                }
                // Add child fields array
                m_jsonWriteValues.push_back();
                m_jsonWriteValues.back().SetArray();
            }
            else /*ST_BINARY*/
            {
                u8 flagsSize = ST_BINARYFLAG_ELEMENT_HEADER;
                if (element.m_nameCrc)
                {
                    flagsSize |= ST_BINARYFLAG_HAS_NAME;
                }
                if (classData->m_serializer)
                {
                    flagsSize |= ST_BINARYFLAG_HAS_VALUE;
                    if (element.m_dataSize < 8)
                    {
                        flagsSize |= static_cast<u8>(element.m_dataSize);
                    }
                    else
                    {
                        flagsSize |= ST_BINARYFLAG_EXTRA_SIZE_FIELD;
                        if (element.m_dataSize < 0x100)
                        {
                            flagsSize |= sizeof(u8);
                        }
                        else if (element.m_dataSize < 0x10000)
                        {
                            flagsSize |= sizeof(u16);
                        }
                        else if (element.m_dataSize < 0x100000000)
                        {
                            flagsSize |= sizeof(u32);
                        }
                        else
                        {
                            AZ_Assert(false, "We don't have enough bits to store a value size of %llu", (u64)element.m_dataSize);
                        }
                    }
                }
                if (element.m_version)
                {
                    flagsSize |= ST_BINARYFLAG_HAS_VERSION;
                }
                m_stream->Write(sizeof(flagsSize), &flagsSize);

                // Write name
                if (element.m_nameCrc)
                {
                    u32 nameCrc = element.m_nameCrc;
#if !defined(AZ_BIG_ENDIAN)
                    AZStd::endian_swap(nameCrc);
#endif
                    m_stream->Write(sizeof(nameCrc), &nameCrc);
                }

                // Write version
                if (element.m_version)
                {
                    AZ_Assert(element.m_version < 0x100, "element.version is too high for the current binary format!");
                    u8 version = static_cast<u8>(element.m_version);
                    m_stream->Write(sizeof(version), &version);
                }

                // Write Uuid
                m_stream->Write(element.m_id.end() - element.m_id.begin(), element.m_id.begin());

                // Write value
                if (classData->m_serializer)
                {
                    // Write extra size field if necessary
                    if (flagsSize & ST_BINARYFLAG_EXTRA_SIZE_FIELD)
                    {
                        size_t sizeBytes = flagsSize & ST_BINARY_VALUE_SIZE_MASK;
                        switch (sizeBytes)
                        {
                        case sizeof(u8):
                        {
                            u8 size = static_cast<u8>(element.m_dataSize);
                            m_stream->Write(sizeBytes, &size);
                            break;
                        }
                        case sizeof(u16):
                        {
                            u16 size = static_cast<u16>(element.m_dataSize);
#if !defined(AZ_BIG_ENDIAN)
                            AZStd::endian_swap(size);
#endif
                            m_stream->Write(sizeBytes, &size);
                            break;
                        }
                        case sizeof(u32):
                        {
                            u32 size = static_cast<u32>(element.m_dataSize);
#if !defined(AZ_BIG_ENDIAN)
                            AZStd::endian_swap(size);
#endif
                            m_stream->Write(sizeBytes, &size);
                            break;
                        }
                        }
                    }

                    if (element.m_dataSize)
                    {
                        void* buffer = m_scratchSpace.data();
                        if (element.m_dataSize > m_scratchSpace.capacity())
                        {
                            buffer = azmalloc(element.m_dataSize);
                        }
                        
                        element.m_stream->Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        element.m_stream->Read(element.m_dataSize, buffer);
                        m_stream->Write(element.m_dataSize, buffer);

                        if (buffer != m_scratchSpace.data())
                        {
                            azfree(buffer);
                        }
                    }

                    element.m_stream = nullptr;
                }
            }

            return true;
        }

        //=========================================================================
        // GotoParentNode
        // [10/25/2012]
        //=========================================================================
        bool ObjectStreamImpl::CloseElement()
        {
            if (!m_isOverlaying)
            {
                if (GetType() == ST_XML)
                {
                    m_xmlNode = m_xmlNode->parent();
                }
                else if (GetType() == ST_JSON)
                {
                    AZ_Assert(m_jsonWriteValues.back().IsArray(), "This value should be fields array");
                    rapidjson::Value fieldArray(AZStd::move(m_jsonWriteValues.back()));
                    m_jsonWriteValues.pop_back();
                    AZ_Assert(m_jsonWriteValues.back().IsObject(), "This value should be class object!");
                    rapidjson::Value classObject(AZStd::move(m_jsonWriteValues.back()));
                    m_jsonWriteValues.pop_back();
                    if (!fieldArray.Empty())
                    {
                        classObject.AddMember("Objects", AZStd::move(fieldArray), m_jsonDoc->GetAllocator());
                    }
                    AZ_Assert(m_jsonWriteValues.back().IsArray(), "This value should be the parent fields array!");
                    m_jsonWriteValues.back().PushBack(AZStd::move(classObject), m_jsonDoc->GetAllocator());
                }
                else /*ST_BINARY*/
                {
                    u8 endTag = ST_BINARYFLAG_ELEMENT_END;
                    m_stream->Write(sizeof(u8), &endTag);
                }
                m_isOverlaying = false;
            }
            return true;
        }

        //=========================================================================
        // Start
        // [6/12/2012]
        //=========================================================================
        bool ObjectStreamImpl::Start()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            ++m_pending;

            bool result = true;

            if (m_flags & OPF_SAVING)
            {
                if (m_type == ST_XML)
                {
                    AZStd::string versionStr = AZStd::string::format("%d", m_version);
                    m_xmlDoc = azcreate(rapidxml::xml_document<char>, (), SystemAllocator);
                    m_xmlNode = m_xmlDoc->allocate_node(rapidxml::node_element, m_xmlDoc->allocate_string("ObjectStream"));
                    rapidxml::xml_attribute<char>* attr = m_xmlDoc->allocate_attribute(m_xmlDoc->allocate_string("version"), m_xmlDoc->allocate_string(versionStr.c_str()));
                    m_xmlNode->append_attribute(attr);
                    m_xmlDoc->append_node(m_xmlNode);
                }
                else if (m_type == ST_JSON)
                {
                    m_jsonDoc = azcreate(rapidjson::Document, (), SystemAllocator);
                    m_jsonDoc->SetObject();
                    m_jsonDoc->AddMember("name", "ObjectStream", m_jsonDoc->GetAllocator());
                    m_jsonDoc->AddMember("version", m_version, m_jsonDoc->GetAllocator());
                    m_jsonWriteValues.push_back();
                    m_jsonWriteValues.back().SetArray();
                }
                else
                {
                    u8 binaryTag = s_binaryStreamTag;
                    u32 version = static_cast<u32>(m_version);
#if !defined(AZ_BIG_ENDIAN)
                    AZStd::endian_swap(binaryTag);
                    AZStd::endian_swap(version);
#endif
                    m_stream->Write(sizeof(binaryTag), &binaryTag);
                    m_stream->Write(sizeof(version), &version);
                }

                --m_pending;
            }
            else
            {   // Loading
                SerializeContext::DataElementNode convertedClassElement;

                IO::SizeType len = m_stream->GetLength();

                u8 streamTag = 0;
                if (m_stream->Read(sizeof(streamTag), &streamTag) == sizeof(streamTag))
                {
                    if (streamTag == s_binaryStreamTag)
                    {
                        SetType(ST_BINARY);

                        u32 version = 0;
                        m_stream->Read(sizeof(m_version), &version);
#if !defined(AZ_BIG_ENDIAN)
                        AZStd::endian_swap(version);
#endif
                        m_version = version;

                        if (m_version <= s_objectStreamVersion)
                        {
                            result = LoadClass(m_inStream, convertedClassElement, nullptr, nullptr, m_flags) && result;
                        }
                        else
                        {
                            AZStd::string newVersionError = AZStd::string::format("ObjectStream binary load error: Stream is a newer version than object stream supports. ObjectStream version: %u, load stream version: %u",
                                s_objectStreamVersion, m_version);
                            m_errorLogger.ReportError(newVersionError.c_str());

                            // this is considered a "fatal" error since the entire stream is unreadable.
                            result = false;
                        }
                    }
                    else if (streamTag == s_xmlStreamTag)
                    {
                        SetType(ST_XML);
                        AZStd::vector<char> memoryBuffer;
                        memoryBuffer.resize_no_construct(static_cast<AZStd::vector<char>::size_type>(static_cast<AZStd::vector<char>::size_type>(len) + 1));

                        // first byte wes already read to determine file type.
                        *reinterpret_cast<u8*>(memoryBuffer.data()) = s_xmlStreamTag;
                        m_stream->Read(len - sizeof(s_xmlStreamTag), memoryBuffer.data() + sizeof(s_xmlStreamTag));
                        memoryBuffer.back() = 0;

                        rapidxml::xml_document<char> xmlDoc;
                        xmlDoc.parse<rapidxml::parse_no_data_nodes>(memoryBuffer.data());
                        m_xmlNode = xmlDoc.first_node();

                        if (m_xmlNode)
                        {
                            rapidxml::xml_attribute<char>* attr = m_xmlNode->first_attribute("version");
                            if (attr)
                            {
                                azsscanf(attr->value(), "%u", &m_version);
                            }
                            if (m_version <= s_objectStreamVersion)
                            {
                                result = LoadClass(m_inStream, convertedClassElement, nullptr, nullptr, m_flags) && result;
                            }
                            else
                            {
                                AZStd::string newVersionError = AZStd::string::format("ObjectStream XML load error: Stream is a newer version than object stream supports. ObjectStream version: %u, load stream version: %u",
                                    s_objectStreamVersion, m_version);
                                m_errorLogger.ReportError(newVersionError.c_str());

                                // this is considered a "fatal" error since the entire stream is unreadable.
                                result = false;
                            }
                        }
                        else
                        {
                            if (xmlDoc.isError())
                            {
                                AZ_Error("Serialize", false, "XML parse error: %s", xmlDoc.getError());
                                m_errorLogger.ReportError("ObjectStream XML parse error.");
                            }
                            else
                            {
                                m_errorLogger.ReportError("ObjectStream XML is malformed.");
                            }

                            // this is considered a "fatal" error since the portions of the stream is unreadable and the file may be truncated
                            result = false;
                        }
                    }
                    else if (streamTag == s_jsonStreamTag)
                    {
                        SetType(ST_JSON);
                        AZStd::vector<char> memoryBuffer;
                        memoryBuffer.resize_no_construct(static_cast<AZStd::vector<char>::size_type>(static_cast<AZStd::vector<char>::size_type>(len) + 1));

                        // first byte wes already read to determine file type.
                        *reinterpret_cast<u8*>(memoryBuffer.data()) = s_jsonStreamTag;
                        m_stream->Read(len - sizeof(s_jsonStreamTag), memoryBuffer.data() + sizeof(s_jsonStreamTag));
                        memoryBuffer.back() = 0;

                        rapidjson::Document jsonDocument;
                        jsonDocument.ParseInsitu(memoryBuffer.data());
                        if (jsonDocument.HasParseError())
                        {
                            AZ_Error("Serialize", false, "JSON parse error: %s (%u)", rapidjson::GetParseError_En(jsonDocument.GetParseError()), jsonDocument.GetErrorOffset());
                            // this is considered a "fatal" error since the entire stream is unreadable.
                            result = false;
                        }
                        else
                        {
                            m_version = jsonDocument["version"].GetUint();
                            m_jsonDoc = &jsonDocument;

                            AZ_Assert(m_jsonDoc->IsObject(), "We should have one root object");

                            if (m_jsonDoc->HasMember("Objects"))
                            {
                                m_jsonReadValues.push_back(JSonReadNode(nullptr, m_jsonDoc));
                                if (m_version <= s_objectStreamVersion)
                                {
                                    result = LoadClass(m_inStream, convertedClassElement, nullptr, nullptr, m_flags) && result;
                                }
                                else
                                {
                                    AZStd::string newVersionError = AZStd::string::format("ObjectStream JSON load error: Stream is a newer version than object stream supports. ObjectStream version: %u, load stream version: %u",
                                        s_objectStreamVersion, m_version);
                                    m_errorLogger.ReportError(newVersionError.c_str());

                                    // this is considered a "fatal" error since the entire stream is unreadable.
                                    result = false;
                                }
                            }
                            m_jsonDoc = nullptr;
                        }
                    }
                    else
                    {
                        m_errorLogger.ReportError("Unknown stream tag (first byte): '\\0' binary, '<' xml or '{' json!");
                        // this is considered a "fatal" error since the entire stream is unreadable.
                        result = false;
                    }
                }
                else
                {
                    m_errorLogger.ReportError("Failed to read type tag from stream. Load aborted!");

                    // this is considered a "fatal" error since the stream is truncated or corrupted.
                    result = false;
                }
            }

            return result;
        }

        bool ObjectStreamImpl::Finalize()
        {
            bool success = true;
            if (m_flags & OPF_SAVING)
            {
                if (GetType() == ST_XML)
                {
                    IO::RapidXMLStreamWriter out(m_stream);
                    rapidxml::print(out.Iterator(), *m_xmlDoc);
                    out.FlushCache();
                    azdestroy(m_xmlDoc, SystemAllocator, rapidxml::xml_document<char>);
                    m_xmlDoc = nullptr;
                    success = out.m_errorCount == 0;
                }
                else if (GetType() == ST_JSON)
                {
                    // printf to stream
                    //
                    AZ_Assert(m_jsonWriteValues.size() == 1, "We should have only the root objects array value!");
                    if (!m_jsonWriteValues.back().Empty())
                    {
                        m_jsonDoc->AddMember("Objects", AZStd::move(m_jsonWriteValues.back()), m_jsonDoc->GetAllocator());
                    }
                    m_jsonWriteValues.clear();
                    IO::RapidJSONStreamWriter os(m_stream);
#ifdef AZ_JSON_USE_PRETTY_WRITER
                    rapidjson::PrettyWriter<IO::RapidJSONStreamWriter> writer(os);
#else
                    rapidjson::Writer<IO::RapidJSONStreamWriter> writer(os);
#endif
                    m_jsonDoc->Accept(writer);
                    azdestroy(m_jsonDoc, SystemAllocator, rapidjson::Document);
                    m_jsonDoc = nullptr;
                }
                else
                {   /* ST_BINARY */
                    u8 endTag = ST_BINARYFLAG_ELEMENT_END;
                    m_stream->Write(sizeof(u8), &endTag);
                }
            }
            delete this;
            return success;
        }
    }   // namespace ObjectStreamInternal

    //=========================================================================
    // LoadBlocking
    // [7/11/2012]
    //=========================================================================
    /*static*/ bool ObjectStream::LoadBlocking(IO::GenericStream* stream, SerializeContext& sc, const ClassReadyCB& readyCB, const FilterDescriptor& filterDesc, const InplaceLoadRootInfoCB& inplaceRootInfo)
    {
        AZ_Assert(stream != nullptr, "You are trying to serialize from a NULL stream!");
        ObjectStreamInternal::ObjectStreamImpl objectStream(stream, &sc, readyCB, CompletionCB(), filterDesc, 0, inplaceRootInfo);

        // This runs and completes (not asynchronous).
        return objectStream.Start();
    }

    //=========================================================================
    // Create
    // [12/21/2012]
    //=========================================================================
    /*static*/ ObjectStream* ObjectStream::Create(IO::GenericStream* stream, SerializeContext& sc, DataStream::StreamType fmt)
    {
        AZ_Assert(stream != nullptr, "You are trying to serialize to a NULL stream!");
        ObjectStreamInternal::ObjectStreamImpl* objStream = aznew ObjectStreamInternal::ObjectStreamImpl(stream, &sc, ClassReadyCB(), CompletionCB(), FilterDescriptor(), ObjectStreamInternal::ObjectStreamImpl::OPF_SAVING, InplaceLoadRootInfoCB());
        objStream->SetType(fmt);
        bool result = objStream->Start();
        if (result)
        {
            return objStream;
        }
        else
        {
            delete objStream;
            return nullptr;
        }
    }

    //=========================================================================
    // Cancel
    // [6/21/2012]
    //=========================================================================
    /*static*/ bool ObjectStream::Cancel(ObjectStream::Handle jobHandle)
    {
        (void)jobHandle;
        AZ_Assert(false, "TODO: Cancel ObjectStream");
        return false;
    }

} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
