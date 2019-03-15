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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/array.h>
namespace AZ
{
    namespace Vertex
    {
        const uint32_t VERTEX_BUFFER_ALIGNMENT = 4;
        enum class AttributeUsage : uint32_t
        {
            StreamDelimiter = 0,
            Unknown,

            Position,
            Color,
            Normal,
            TexCoord,
            Weights,
            Indices,
            Tangent,
            BiTangent,
            BufferAlignment,

            NumTypes
        };      

        struct AttributeUsageData
        {
            AZStd::string friendlyName;
            AZStd::string semanticName;
        };

        static AttributeUsageData AttributeUsageDataTable[(unsigned int)AZ::Vertex::AttributeUsage::NumTypes] =
        {
            // {friendlyName, semanticName}
            { "StreamDelimiter", "UNKNOWN" },
            { "Unknown", "UNKNOWN" },

            { "Position", "POSITION" },
            { "Color", "COLOR" },
            { "Normal", "NORMAL" },
            { "TexCoord", "TEXCOORD" },
            { "Weights", "BLENDWEIGHT" },
            { "Indices", "BLENDINDICES" },
            { "Tangent", "TEXCOORD" },
            { "BiTangent", "TEXCOORD" },
            { "BufferAlignment", "UNKNOWN" },
        };

        enum class AttributeType : uint32_t
        {
            Float16_1 = 0,
            Float16_2,
            Float16_4,

            Float32_1,
            Float32_2,
            Float32_3,
            Float32_4,

            Byte_1,
            Byte_2,
            Byte_4,

            Short_1,
            Short_2,
            Short_4,

            UInt16_1,
            UInt16_2,
            UInt16_4,

            UInt32_1,
            UInt32_2,
            UInt32_3,
            UInt32_4,

            NumTypes
        };    

        struct AttributeTypeData
        {
            AZStd::string friendlyName;
            uint8 byteSize;
        };

        static AttributeTypeData AttributeTypeDataTable[(unsigned int)AZ::Vertex::AttributeType::NumTypes] =
        {
            { "Float16_1", 2 },
            { "Float16_2", 4 },
            { "Float16_4", 8 },

            { "Float32_1", 4 },
            { "Float32_2", 8 },
            { "Float32_3", 12 },
            { "Float32_4", 16 },

            { "Byte_1", 1 },
            { "Byte_2", 2 },
            { "Byte_4", 4 },

            { "Short_1", 2 },
            { "Short_2", 4 },
            { "Short_4", 8 },

            { "UInt16_1", 2 },
            { "UInt16_2", 4 },
            { "UInt16_4", 8 },

            { "UInt32_1", 4 },
            { "UInt32_2", 8 },
            { "UInt32_3", 12 },
            { "UInt32_4", 16 },
        };

        //! Stores the usage, type, and byte length of an individual vertex attribute
        class Attribute
        {
        public:
            Attribute(AttributeUsage usage, AttributeType type)
            {
                m_usage = usage;
                m_type = type;
                m_byteLength = AttributeTypeDataTable[(unsigned int)m_type].byteSize;
            }
            AttributeUsage GetUsage() const { return m_usage; }
            AttributeType GetType() const { return m_type; }
            uint8 GetByteLength() const { return m_byteLength; }
            AZStd::string ToString() const
            {
                return AttributeUsageDataTable[(unsigned int)m_usage].friendlyName + "." + AttributeTypeDataTable[(unsigned int)m_type].friendlyName;
            }

            bool operator==(const Attribute& other) const
            {
                if (m_usage == other.GetUsage() && m_type == other.GetType())
                {
                    return true;
                }

                return false;
            }

        private:
            AttributeUsage m_usage;
            AttributeType m_type;
            uint32_t m_byteLength;
        };

        //! Flexible vertex format class
        class Format
        {
        public:
            Format(){};
            Format(const AZStd::vector<Attribute>& attributes)
            {
                m_vertexAttributes = attributes;
                m_enum = eVF_Unknown;
                UpdateStride();
                PadFormatForBufferAlignment();           
                GenerateName();
                GenerateCrc();
            }
            //! Conversion from old hard-coded EVertexFormat enum to new, flexible vertex class
            Format(EVertexFormat format)
            {
                switch (format)
                {
                case eVF_Unknown:
                    m_enum = eVF_Unknown;
                    break;
                case eVF_P3F_C4B_T2F:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_2)
                    };
                    m_enum = eVF_P3F_C4B_T2F;
                    break;
                case eVF_P3S_C4B_T2S:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float16_4),// vec3f16 is backed by a CryHalf4
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float16_2)
                    };
                    m_enum = eVF_P3S_C4B_T2S;
                    break;
                case eVF_P3S_N4B_C4B_T2S:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float16_4),// vec3f16 is backed by a CryHalf4
                        Attribute(AttributeUsage::Normal, AttributeType::Byte_4),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float16_2)
                    };
                    m_enum = eVF_P3S_N4B_C4B_T2S;
                    break;
                case eVF_P3F_C4B_T4B_N3F2:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::Tangent, AttributeType::Float32_3),//x-axis
                        Attribute(AttributeUsage::BiTangent, AttributeType::Float32_3)//y-axis
#ifdef PARTICLE_MOTION_BLUR
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),//prevPos
                        Attribute(AttributeUsage::Tangent, AttributeType::Float32_3),//prevXTan
                        Attribute(AttributeUsage::BiTangent, AttributeType::Float32_3)//prevYTan
#endif
                    };
                    m_enum = eVF_P3F_C4B_T4B_N3F2;// Particles.
                    break;
                case eVF_TP3F_C4B_T2F:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_4),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_2)
                    };
                    m_enum = eVF_TP3F_C4B_T2F;// Fonts (28 bytes).
                    break;
                case eVF_TP3F_T2F_T3F:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_2),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_3)
                    };
                    m_enum = eVF_TP3F_T2F_T3F;
                    break;
                case eVF_P3F_T3F:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_3)
                    };
                    m_enum = eVF_P3F_T3F; // Miscellaneus.
                    break;
                case eVF_P3F_T2F_T3F:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_2),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_3)
                    };
                    m_enum = eVF_P3F_T2F_T3F;
                    break;
                // Additional streams
                case eVF_T2F:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_2)
                    };
                    m_enum = eVF_T2F; // Light maps TC (8 bytes).
                    break;
                case eVF_W4B_I4S:// Skinned weights/indices stream.
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Weights, AttributeType::Byte_4),
                        Attribute(AttributeUsage::Indices, AttributeType::UInt16_4)
                    };
                    m_enum = eVF_W4B_I4S;
                    break;
                case eVF_C4B_C4B:// SH coefficients.
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Unknown, AttributeType::Byte_4),    //coef0
                        Attribute(AttributeUsage::Unknown, AttributeType::Byte_4)    //coef1
                    };
                    m_enum = eVF_C4B_C4B;
                    break;
                case eVF_P3F_P3F_I4B:// Shape deformation stream.
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),    //thin
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),    //fat
                        Attribute(AttributeUsage::Indices, AttributeType::Byte_4)
                    };
                    m_enum = eVF_P3F_P3F_I4B;
                    break;
                case eVF_P3F:// Velocity stream.
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3)
                    };
                    m_enum = eVF_P3F;
                    break;
                case eVF_C4B_T2S:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float16_2),
                    };
                    m_enum = eVF_C4B_T2S;// General (Position is merged with Tangent stream)
                    break;
                case eVF_P2F_T4F_C4F: // Lens effects simulation
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_2),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_4),
                        Attribute(AttributeUsage::Color, AttributeType::Float32_4)
                    };
                    m_enum = eVF_P2F_T4F_C4F;
                    break;
                case eVF_P2F_T4F_T4F_C4F:
                    m_enum = eVF_P2F_T4F_T4F_C4F;
                    break;
                case eVF_P2S_N4B_C4B_T1F:// terrain
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float16_2),// xy-coordinates in terrain
                        Attribute(AttributeUsage::Normal, AttributeType::Byte_4),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_1)// z-coordinate in terrain
                    };
                    m_enum = eVF_P2S_N4B_C4B_T1F;
                    break;
                case eVF_P3F_C4B_T2S:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_3),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float16_2)
                    };
                    m_enum = eVF_P3F_C4B_T2S;
                    break;
                case eVF_P2F_C4B_T2F_F4B:
                    m_vertexAttributes =
                    {
                        Attribute(AttributeUsage::Position, AttributeType::Float32_2),
                        Attribute(AttributeUsage::Color, AttributeType::Byte_4),
                        Attribute(AttributeUsage::TexCoord, AttributeType::Float32_2),
                        Attribute(AttributeUsage::Indices, AttributeType::UInt16_2)
                    };
                    m_enum = eVF_P2F_C4B_T2F_F4B;
                    break;
                case eVF_Max:
                default:
                    AZ_Assert(false, "Invalid vertex format");
                    m_enum = eVF_Unknown;
                }
                UpdateStride();
                PadFormatForBufferAlignment();               
                GenerateName();
                GenerateCrc();
            }

            //! Get the equivalent old-style EVertexFormat enum
            uint GetEnum() const { return m_enum; }

            /*! The first 3 entries to the EVertexFormat enum (below) are labeled as Base Stream in the comments
            CLodGeneratorInteractionManager::VerifyNoOverlap checks for these enums, then gets the uvs. I assume it is looking for
            the base streams since it checks these 3, hence this function is called IsBaseStream
            Need a more generic way to determine this...if possible
            */
            bool IsBaseStream() const
            {
                return m_enum == eVF_P3F_C4B_T2F || m_enum == eVF_P3S_C4B_T2S || m_enum == eVF_P3S_N4B_C4B_T2S;
            }
            //! Helper function to check to see if the vertex format has a position attribute that uses 16 bit floats for the underlying type
            bool Has16BitFloatPosition() const
            {
                for (Attribute attribute : m_vertexAttributes)
                {
                    if (attribute.GetUsage() == AttributeUsage::Position && attribute.GetType() == AttributeType::Float16_4)
                    {
                        return true;
                    }
                }
                return false;
            }
            
            //! Helper function to check to see if the vertex format has a texture coordinate attribute that uses 16 bit floats for the underlying type
            bool Has16BitFloatTextureCoordinates() const
            {
                for (Attribute attribute : m_vertexAttributes)
                {
                    if (attribute.GetUsage() == AttributeUsage::TexCoord && attribute.GetType() == AttributeType::Float16_2)
                    {
                        return true;
                    }
                }
                return false;
            }

            //! Helper function to check to see if the vertex format has a texture coordinate attribute that uses 32 bit floats for the underlying type
            bool Has32BitFloatTextureCoordinates() const
            {
                for (Attribute attribute : m_vertexAttributes)
                {
                    if (attribute.GetUsage() == AttributeUsage::TexCoord && attribute.GetType() == AttributeType::Float32_2)
                    {
                        return true;
                    }
                }
                return false;
            }

            //! Return any attributes that match the usage. // waltont TODO It's possible that getting the number of attributes or the index of those attributes would be preferable
            AZStd::vector<Attribute> GetAttributesByUsage(AttributeUsage usage) const
            {
                AZStd::vector<Attribute> outputAttributes;
                for (Attribute attribute : m_vertexAttributes)
                {
                    if (attribute.GetUsage() == usage)
                    {
                        outputAttributes.push_back(attribute);
                    }
                }
                return outputAttributes;
            }

            const AZStd::vector<Attribute>& GetAttributes() const
            {
                return m_vertexAttributes;
            }

            //! Return true if the vertex format is a superset of the input
            bool IsSupersetOf(const AZ::Vertex::Format& input) const
            {
                AZStd::vector<Attribute> attributes = input.GetAttributes();
                for (Attribute a : attributes)
                {
                    if (this->GetAttributesByUsage(a.GetUsage()).size() < input.GetAttributesByUsage(a.GetUsage()).size())
                    {
                        return false;
                    }
                }
                return true;
            }

            uint GetStride(void) const { return m_stride; }
            const char* GetName(void) const { return m_Name.data(); }
            AZ::u32 GetCRC(void) const { return m_CRC; }
            
            //! Returns the true if an attribute with the given usage is found, false otherwise
            bool TryCalculateOffset(uint& offset, AttributeUsage usage, uint index = 0) const
            {
                offset = 0;
                for (Attribute attribute : m_vertexAttributes)
                {
                    if (attribute.GetUsage() == usage)
                    {
                        if (index == 0)
                        {
                            return true;
                        }
                        else
                        {
                            --index;
                        }
                    }
                    offset += attribute.GetByteLength();
                }
                return false;
            }

            // Quick comparison operators. These will need to be replaced when we remove the EVertexFormat enum entirely
            bool operator==(const Format& other) const
            {
                return m_CRC == other.GetCRC();
            }
            bool operator!=(const Format& other) const
            {
                return !(*this == other);
            }
            bool operator==(const EVertexFormat& other) const
            {
                return m_enum == other;
            }
            // Used in RendermeshMerger.cpp
            // waltont TODO In order to replace this, we need to determine if actual order of EVertexFormat enums have any significance other than base formats coming first, or if these are just needed to have an arbitrary way to sort by vertex format
            // CHWShader_D3D::mfUpdateFXVertexFormat and CHWShader_D3D::mfVertexFormat want the max between two vertex formats...why? It seems like a bad guess of which vertex format to use since there's no particular order to EVertexFormats...other than the more specialized EVertexFormats come after the base EVertexFormats
            bool operator<(const Format& other) const
            {
                if (other.IsSupersetOf(*this))
                {
                    return true;
                }
                if (*this == other || this->IsSupersetOf(other))
                {
                    return false;
                }
                if (m_enum != eVF_Unknown && other.GetEnum() != eVF_Unknown)
                {
                    return m_enum < other.GetEnum();
                }
                return m_CRC < other.GetCRC();
            }
            bool operator<=(const Format& other) const
            {
                return (*this == other || *this < other);
            }
            bool operator>(const Format& other) const
            {
                return !(*this <= other);
            }
            bool operator>=(const Format& other) const
            {
                return (*this == other || *this > other);
            }
        private:
            //! Calculates the sum of the size in bytes of all attributes that make up this format
            void UpdateStride()
            {
                m_stride = 0;
                for (Attribute attribute : m_vertexAttributes)
                {
                    m_stride += attribute.GetByteLength();
                }
            }

            //! Add extra padding to the end of the vertex format for alignment
            void PadFormatForBufferAlignment(void)
            {
                uint formatSize = GetStride();

                // Add padding to the stream to make it 4-byte aligned
                switch (formatSize % VERTEX_BUFFER_ALIGNMENT)
                {
                case (3):
                {
                    m_vertexAttributes.push_back(Attribute(AttributeUsage::BufferAlignment, AttributeType::Byte_1));
                    break;
                }
                case (2):
                {
                    m_vertexAttributes.push_back(Attribute(AttributeUsage::BufferAlignment, AttributeType::Byte_2));
                    break;
                }
                case (1):
                {
                    m_vertexAttributes.push_back(Attribute(AttributeUsage::BufferAlignment, AttributeType::Byte_2));
                    m_vertexAttributes.push_back(Attribute(AttributeUsage::BufferAlignment, AttributeType::Byte_1));
                    break;
                }
                case (0):
                default:
                    break;
                }

                //re-calculate and update the stride
                UpdateStride();
            }

            void GenerateName(void)
            {
                auto itor = m_Name.begin();
                for (Attribute attribute : m_vertexAttributes)
                {
                    const AZStd::string& usageStr = AttributeUsageDataTable[static_cast<unsigned int>(attribute.GetUsage())].friendlyName;
                    itor = AZStd::copy(usageStr.begin(), usageStr.end() , itor);

                    *itor++ = '.';

                    const AZStd::string& typeStr = AttributeTypeDataTable[static_cast<unsigned int>(attribute.GetType())].friendlyName;
                    itor = AZStd::copy(typeStr.begin(), typeStr.end(), itor);
                }

                if(itor < m_Name.end())
                {
                    *itor = '\0';
                }
                else
                {
                    m_Name.back() = '\0';
                }
            }

            void GenerateCrc(void)
            {
                m_CRC = AZ::Crc32(m_vertexAttributes.data(), m_vertexAttributes.size() * sizeof(m_vertexAttributes[0]));
            }

            AZ::u32 m_CRC = 0;
            static const uint32_t kMaxVertexNameLen = 128;
            AZStd::array<char, kMaxVertexNameLen> m_Name;

            AZStd::vector<Attribute> m_vertexAttributes;
            uint m_enum = eVF_Unknown;
            uint m_stride = 0;
        };


        // bNeedNormals=1 - float normals; bNeedNormals=2 - byte normals //waltont TODO (this was copied as is from vertexformats.h) this comment is out of date and the function does not even use all the parameters. This should be replaceable with the new vertex class, and should be replaced when refactoring CHWShader_D3D::mfVertexFormat which handles the shader parsing/serialization
        _inline Format VertFormatForComponents(bool bNeedCol, bool bHasTC, bool bHasTC2, bool bHasPS, bool bHasNormal)
        {
            AZ::Vertex::Format RequestedVertFormat;

            if (bHasPS)
            {
                RequestedVertFormat = AZ::Vertex::Format(eVF_P3F_C4B_T4B_N3F2);
            }
            else
            if (bHasNormal)
            {
                RequestedVertFormat = AZ::Vertex::Format(eVF_P3S_N4B_C4B_T2S);
            }
            else
            {
                if (!bHasTC2)
                {
                    RequestedVertFormat = AZ::Vertex::Format(eVF_P3S_C4B_T2S);
                }
                else
                {
                    RequestedVertFormat = AZ::Vertex::Format({
                        AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::Position, AZ::Vertex::AttributeType::Float32_3),
                        AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::Color, AZ::Vertex::AttributeType::Byte_4),
                        AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::TexCoord, AZ::Vertex::AttributeType::Float32_2),
                        AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::TexCoord, AZ::Vertex::AttributeType::Float32_2)
                    }
                    );
                }
            }

            return RequestedVertFormat;
        }
    }
}