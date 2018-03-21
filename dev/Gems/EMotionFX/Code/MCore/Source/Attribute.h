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

// include the required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "UnicodeString.h"
#include "Endian.h"
#include "Stream.h"

namespace MCore
{
    // forward declarations
    class AttributeSettings;


    // the attribute interface types
    enum : uint32
    {
        ATTRIBUTE_INTERFACETYPE_FLOATSPINNER    = 0,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_FLOATSLIDER     = 1,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_INTSPINNER      = 2,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_INTSLIDER       = 3,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_COMBOBOX        = 4,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_CHECKBOX        = 5,        // MCore::AttributeFloat
        ATTRIBUTE_INTERFACETYPE_VECTOR2         = 6,        // MCore::AttributeVector2
        ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO    = 7,        // MCore::AttributeVector3
        ATTRIBUTE_INTERFACETYPE_VECTOR4         = 8,        // MCore::AttributeVector4
        ATTRIBUTE_INTERFACETYPE_FILEBROWSE      = 9,        // MCore::AttributeString
        ATTRIBUTE_INTERFACETYPE_COLOR           = 10,       // MCore::AttributeVector4
        ATTRIBUTE_INTERFACETYPE_STRING          = 11,       // MCore::AttributeString
        ATTRIBUTE_INTERFACETYPE_VECTOR3         = 113212,   // MCore::AttributeVector3
        ATTRIBUTE_INTERFACETYPE_PROPERTYSET     = 113213,   // MCore::AttributeSet
        ATTRIBUTE_INTERFACETYPE_DEFAULT         = 0xFFFFFFFF// use the default attribute type that the specific attribute class defines as default
    };


    /**
     *
     *
     *
     */
    class MCORE_API Attribute
    {
        MCORE_MEMORYOBJECTCATEGORY(Attribute, MCORE_SIMD_ALIGNMENT, MCORE_MEMCATEGORY_ATTRIBUTES);

        friend class AttributePool;
        friend class AttributeFactory;
    public:
        virtual void Destroy(bool lock = true);

        virtual Attribute* Clone() const = 0;
        virtual Attribute* CreateInstance(void* destMemory) = 0;
        virtual const char* GetTypeString() const = 0;
        MCORE_INLINE uint32 GetType() const                                         { return mTypeID; }
        virtual bool InitFromString(const MCore::String& valueString) = 0;
        virtual bool ConvertToString(MCore::String& outString) const = 0;
        virtual bool InitFrom(const Attribute* other) = 0;
        virtual uint32 GetClassSize() const = 0;
        virtual uint8 GetStreamWriteVersion() const;
        virtual uint32 GetDefaultInterfaceType() const = 0;
        virtual void Scale(float scaleFactor)                                       { MCORE_UNUSED(scaleFactor); }  // scale data, for example a vector3 holding a position

        virtual bool GetCanHaveChildren() const                                     { return false; }
        virtual uint32 GetNumChildAttributes() const                                { return 0; }
        virtual Attribute* GetChildAttribute(uint32 index) const                    { MCORE_UNUSED(index); return nullptr; }
        virtual AttributeSettings* GetChildAttributeSettings(uint32 index) const    { MCORE_UNUSED(index); return nullptr; }

        virtual uint32 FindAttributeIndexByValuePointer(const Attribute* attribute) const;
        virtual uint32 FindAttributeIndexByName(const char* name) const;
        virtual uint32 FindAttributeIndexByNameID(uint32 nameID) const;
        virtual uint32 FindAttributeIndexByInternalName(const char* name) const;
        virtual uint32 FindAttributeIndexByInternalNameID(uint32 nameID) const;
        virtual uint32 FindAttributeIndexHierarchical(const char* namePath, Attribute** outContainer) const;
        virtual Attribute* FindAttributeHierarchical(const char* namePath, AttributeSettings** outSettings = nullptr, bool lookUpRef = true) const;

        AttributeSettings* FindAttributeSettings() const;

        template<typename AttributeClass>
        MCORE_INLINE AttributeClass* FindAttributeHierarchical(const char* namePath, AttributeSettings** outSettings = nullptr, bool lookUpRef = true) const
        {
            Attribute* result = FindAttributeHierarchical(namePath, outSettings, lookUpRef);
            if (result)
            {
                if (result->GetType() == AttributeClass::TYPE_ID)
                {
                    return static_cast<AttributeClass*>(result);
                }
                else
                {
                    LogWarning("MCore::Attribute::FindAttributeHierarchical<TYPE>() - The attribute for path '%s' is not of type %d but of type %d (%s), returning nullptr.", namePath, AttributeClass::TYPE_ID, result->GetType(), result->GetTypeString());
                }
            }

            return nullptr;
        }

        void BuildHierarchicalName(String& outString) const;
        String BuildHierarchicalName() const;

        MCORE_INLINE virtual uint8* GetRawDataPointer()             { return nullptr; }
        MCORE_INLINE virtual uint32 GetRawDataSize() const          { return 0; }
        virtual bool GetSupportsRawDataPointer() const              { return false; }

        bool Write(Stream* stream, MCore::Endian::EEndianType targetEndianType) const;
        bool Read(Stream* stream, MCore::Endian::EEndianType sourceEndianType);

        static bool WriteHeader(Stream* stream, Endian::EEndianType targetEndianType, const Attribute* attribute);  // write the header (type + size)
        static bool ReadHeader(Stream* stream, Endian::EEndianType endianType, Attribute** outAttribute); // read the header (type + size)
        static bool WriteFullAttribute(Stream* stream, Endian::EEndianType targetEndianType, const Attribute* attribute);   // writes header + data (data=versionnumber + data)
        static bool ReadFullAttribute(Stream* stream, Endian::EEndianType endianType, Attribute** outAttribute);    // read header + data (data=versionnumber + data)
        static uint32 GetFullAttributeSize(Attribute* attribute);

        uint32 GetStreamSize() const;                   // version + data
        virtual uint32 GetDataSize() const = 0;         // data only

        Attribute& operator=(const Attribute& other);

        void SetParent(Attribute* parent);
        Attribute* GetParent() const;
        void UpdateChildParentPointers(bool recursive = true);

    protected:
        uint32      mTypeID;    /**< The unique type ID of the attribute class. */
        Attribute*  mParent;    /**< The parent, which contains this attribute as child, or nullptr if none. */

        virtual ~Attribute();
        Attribute(uint32 typeID);

        /**
         * Write the attribute to a given stream.
         * @param stream The stream to write to.
         * @param endianType The target endian to write the data in.
         * @result Returns true when successful or false when failed.
         */
        virtual bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const = 0;

        /**
         * Read the attribute info and data from a given stream.
         * Please note that the endian information of the actual data is not being converted. You have to handle that yourself.
         * The data endian conversion could be done with for example the static Attribute::ConvertDataEndian method.
         * @param stream The stream to read the info and data from.
         * @param endianType The endian type in which the data is stored in the stream.
         * @param version The version of the attribute.
         * @result Returns true when successful, or false when reading failed.
         */
        virtual bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) = 0;
    };
} // namespace MCore
