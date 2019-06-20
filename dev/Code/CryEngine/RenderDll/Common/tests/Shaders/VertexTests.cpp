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
#include "StdAfx.h"
#include <AzTest/AzTest.h>
#include <Tests/TestTypes.h>
#include "Common/Shaders/Vertex.h"
#include "DriverD3D.h"
// General vertex stream stride
int32 m_cSizeVF[eVF_Max] =
{
    0,
    sizeof(SVF_P3F_C4B_T2F),
    sizeof(SVF_P3S_C4B_T2S),
    sizeof(SVF_P3S_N4B_C4B_T2S),
    sizeof(SVF_P3F_C4B_T4B_N3F2),
    sizeof(SVF_TP3F_C4B_T2F),
    sizeof(SVF_TP3F_T2F_T3F),
    sizeof(SVF_P3F_T3F),
    sizeof(SVF_P3F_T2F_T3F),
    sizeof(SVF_T2F),
    sizeof(SVF_W4B_I4S),
    sizeof(SVF_C4B_C4B),
    sizeof(SVF_P3F_P3F_I4B),
    sizeof(SVF_P3F),
    sizeof(SVF_C4B_T2S),

    sizeof(SVF_P2F_T4F_C4F),
    0, //sizeof(SVF_P2F_T4F_T4F_C4F)

    sizeof(SVF_P2S_N4B_C4B_T1F),
    sizeof(SVF_P3F_C4B_T2S),
    sizeof(SVF_P2F_C4B_T2F_F4B)
};

// Legacy table copied from RenderMesh.cpp
#define OOFS(t, x) ((int)offsetof(t, x))

SBufInfoTable m_cBufInfoTable[eVF_Max] =
{
    // { Texcoord, Color, Normal }
    {
        -1, -1, -1
    },
    {      //eVF_P3F_C4B_T2F
        OOFS(SVF_P3F_C4B_T2F, st),
        OOFS(SVF_P3F_C4B_T2F, color.dcolor),
        -1
    },
    {      //eVF_P3S_C4B_T2S
        OOFS(SVF_P3S_C4B_T2S, st),
        OOFS(SVF_P3S_C4B_T2S, color.dcolor),
        -1
    },
    {      //eVF_P3S_N4B_C4B_T2S
        OOFS(SVF_P3S_N4B_C4B_T2S, st),
        OOFS(SVF_P3S_N4B_C4B_T2S, color.dcolor),
        OOFS(SVF_P3S_N4B_C4B_T2S, normal)
    },
    {     // eVF_P3F_C4B_T4B_N3F2
        -1,
        OOFS(SVF_P3F_C4B_T4B_N3F2, color.dcolor),
        -1
    },
    {     // eVF_TP3F_C4B_T2F
        OOFS(SVF_TP3F_C4B_T2F, st),
        OOFS(SVF_TP3F_C4B_T2F, color.dcolor),
        -1
    },
    {     // eVF_TP3F_T2F_T3F
        OOFS(SVF_TP3F_T2F_T3F, st0),
        -1,
        -1
    },
    {     // eVF_P3F_T3F
        OOFS(SVF_P3F_T3F, st),
        -1,
        -1
    },
    {     // eVF_P3F_T2F_T3F
        OOFS(SVF_P3F_T2F_T3F, st0),
        -1,
        -1
    },
    {// eVF_T2F
        OOFS(SVF_T2F, st),
        -1,
        -1
    },
    { -1, -1, -1 },// eVF_W4B_I4S
    { -1, -1, -1 },// eVF_C4B_C4B
    { -1, -1, -1 },// eVF_P3F_P3F_I4B

    {
        -1,
        -1,
        -1
    },
    {     // eVF_C4B_T2S
        OOFS(SVF_C4B_T2S, st),
        OOFS(SVF_C4B_T2S, color.dcolor),
        -1
    },
    {    // eVF_P2F_T4F_C4F
        OOFS(SVF_P2F_T4F_C4F, st),
        OOFS(SVF_P2F_T4F_C4F, color),
        -1
    },
    { -1, -1, -1 }, // eVF_P2F_T4F_T4F_C4F
    {     // eVF_P2S_N4B_C4B_T1F
        OOFS(SVF_P2S_N4B_C4B_T1F, z),
        OOFS(SVF_P2S_N4B_C4B_T1F, color.dcolor),
        OOFS(SVF_P2S_N4B_C4B_T1F, normal)
    },
    {     // eVF_P3F_C4B_T2S
        OOFS(SVF_P3F_C4B_T2S, st),
        OOFS(SVF_P3F_C4B_T2S, color.dcolor),
        -1
    },
    {     // SVF_P2F_C4B_T2F_F4B
        OOFS(SVF_P2F_C4B_T2F_F4B, st),
        OOFS(SVF_P2F_C4B_T2F_F4B, color.dcolor),
        -1
    },
};
#undef OOFS

AZStd::vector<D3D11_INPUT_ELEMENT_DESC> Legacy_InitBaseStreamD3DVertexDeclaration(EVertexFormat nFormat)
{
    //========================================================================================
    // base stream declarations (stream 0)
    D3D11_INPUT_ELEMENT_DESC elemPosHalf = { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemTCHalf = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

    D3D11_INPUT_ELEMENT_DESC elemPos = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemPos2 = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemPosTR = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };  // position
    D3D11_INPUT_ELEMENT_DESC elemPos2Half = { "POSITION", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemPos1 = { "POSITION", 1, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

    D3D11_INPUT_ELEMENT_DESC elemNormalB = { "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    D3D11_INPUT_ELEMENT_DESC elemTan = { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };       // axis/size
    D3D11_INPUT_ELEMENT_DESC elemBitan = { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };    // axis/size
    D3D11_INPUT_ELEMENT_DESC elemColor = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };        // diffuse
    D3D11_INPUT_ELEMENT_DESC elemColorF = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };   // general color
    D3D11_INPUT_ELEMENT_DESC elemTC0 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC1 = { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC1_3 = { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC0_4 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture
    D3D11_INPUT_ELEMENT_DESC elemTC0_1 = { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };      // texture

    AZ::Vertex::Format vertexFormat = AZ::Vertex::Format((EVertexFormat)nFormat);
    AZStd::vector<D3D11_INPUT_ELEMENT_DESC> decl;

    uint texCoordOffset = 0;
    bool hasTexCoord = vertexFormat.TryCalculateOffset(texCoordOffset, AZ::Vertex::AttributeUsage::TexCoord);
    uint colorOffset = 0;
    bool hasColor = vertexFormat.TryCalculateOffset(colorOffset, AZ::Vertex::AttributeUsage::Color);
    uint normalOffset = 0;
    bool hasNormal = vertexFormat.TryCalculateOffset(normalOffset, AZ::Vertex::AttributeUsage::Normal);

    // Position

    switch (nFormat)
    {
    case eVF_TP3F_C4B_T2F:
    case eVF_TP3F_T2F_T3F:
        decl.push_back(elemPosTR);
        break;
    case eVF_P3S_C4B_T2S:
    case eVF_P3S_N4B_C4B_T2S:
        decl.push_back(elemPosHalf);
        break;
    case eVF_P2S_N4B_C4B_T1F:
        decl.push_back(elemPos2Half);
        break;
    case eVF_P2F_T4F_C4F:
        decl.push_back(elemPos2);
        break;
    case eVF_T2F:
    case eVF_C4B_T2S:
    case eVF_Unknown:
        break;
    default:
        decl.push_back(elemPos);
    }

    // Normal
    if (hasNormal)
    {
        elemNormalB.AlignedByteOffset = normalOffset;
        decl.push_back(elemNormalB);
    }

#ifdef PARTICLE_MOTION_BLUR
    if (nFormat == eVF_P3F_C4B_T4B_N3F2)
    {
        elemTC0_4.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, prevXaxis);
        elemTC0_4.SemanticIndex = 0;
        decl.AddElem(elemTC0_4);
    }
#endif
    // eVF_P2F_T4F_C4F has special case logic below, so ignore it here
    if (hasColor && nFormat != eVF_P2F_T4F_C4F)
    {
        elemColor.AlignedByteOffset = colorOffset;
        elemColor.SemanticIndex = 0;
        decl.push_back(elemColor);
    }
    if (nFormat == eVF_P3F_C4B_T4B_N3F2)
    {
#ifdef PARTICLE_MOTION_BLUR
        elemTC1_3.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, prevPos);
        elemTC1_3.SemanticIndex = 1;
        decl.push_back(elemTC1_3);
#endif
        elemColor.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, st);
        elemColor.SemanticIndex = 1;
        decl.push_back(elemColor);

        elemTan.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, xaxis);
        decl.push_back(elemTan);

        elemBitan.AlignedByteOffset = (int)offsetof(SVF_P3F_C4B_T4B_N3F2, yaxis);
        decl.push_back(elemBitan);
    }

    if (nFormat == eVF_P2F_T4F_C4F)
    {
        elemTC0_4.AlignedByteOffset = (int)offsetof(SVF_P2F_T4F_C4F, st);
        elemTC0_4.SemanticIndex = 0;
        decl.push_back(elemTC0_4);

        elemColorF.AlignedByteOffset = (int)offsetof(SVF_P2F_T4F_C4F, color);
        elemColorF.SemanticIndex = 0;
        decl.push_back(elemColorF);
    }
    if (hasTexCoord)
    {
        elemTC0.AlignedByteOffset = texCoordOffset;
        elemTC0.SemanticIndex = 0;
        switch (nFormat)
        {
        case eVF_P2F_T4F_C4F:
            // eVF_P2F_T4F_C4F has special case logic above, so ignore it here
            break;
        case eVF_P3S_C4B_T2S:
        case eVF_P3S_N4B_C4B_T2S:
        case eVF_C4B_T2S:
        case eVF_P3F_C4B_T2S:
            elemTCHalf.AlignedByteOffset = texCoordOffset;
            elemTCHalf.SemanticIndex = 0;
            decl.push_back(elemTCHalf);
            break;
        case eVF_P3F_T3F:
            elemTC1_3.AlignedByteOffset = texCoordOffset;
            elemTC1_3.SemanticIndex = 0;
            decl.push_back(elemTC1_3);
            break;
        case eVF_P2S_N4B_C4B_T1F:
            elemTC0_1.AlignedByteOffset = texCoordOffset;
            elemTC0_1.SemanticIndex = 0;
            decl.push_back(elemTC0_1);
            break;
        case eVF_TP3F_T2F_T3F:
        case eVF_P3F_T2F_T3F:
            decl.push_back(elemTC0);

            //This case needs two TexCoord elements
            elemTC1_3.AlignedByteOffset = texCoordOffset + 8;
            elemTC1_3.SemanticIndex = 1;
            decl.push_back(elemTC1_3);
            break;
        default:
            decl.push_back(elemTC0);
        }
    }
    if (nFormat == eVF_P2S_N4B_C4B_T1F)
    {
        elemPos1.AlignedByteOffset = (int)offsetof(SVF_P2S_N4B_C4B_T1F, z);
        decl.push_back(elemPos1);
    }
    decl.shrink_to_fit();

    return decl;
}

bool DeclarationsAreEqual(AZStd::vector<D3D11_INPUT_ELEMENT_DESC>& declarationA, AZStd::vector<D3D11_INPUT_ELEMENT_DESC>& declarationB)
{
    if (declarationA.size() != declarationB.size())
    {
        return false;
    }

    for (uint i = 0; i < declarationA.size(); ++i)
    {
        D3D11_INPUT_ELEMENT_DESC elementDescriptionA = declarationA[i];
        D3D11_INPUT_ELEMENT_DESC elementDescriptionB = declarationB[i];
        if (elementDescriptionA.SemanticIndex != elementDescriptionB.SemanticIndex ||
            elementDescriptionA.Format != elementDescriptionB.Format ||
            elementDescriptionA.InputSlot != elementDescriptionB.InputSlot ||
            elementDescriptionA.AlignedByteOffset != elementDescriptionB.AlignedByteOffset ||
            elementDescriptionA.InputSlotClass != elementDescriptionB.InputSlotClass ||
            elementDescriptionA.InstanceDataStepRate != elementDescriptionB.InstanceDataStepRate ||
            strcmp(elementDescriptionA.SemanticName, elementDescriptionB.SemanticName) != 0)
        {
            return false;
        }
    }
    return true;
}

class VertexFormatTest
    : public ::testing::TestWithParam < int >
    , public UnitTest::AllocatorsBase
{
public:
    void SetUp() override
    {
        UnitTest::AllocatorsBase::SetupAllocator();
    }

    void TearDown() override
    {
        UnitTest::AllocatorsBase::TeardownAllocator();
    }
};

TEST_P(VertexFormatTest, GetStride_MatchesExpected)
{
    EVertexFormat eVF = (EVertexFormat)GetParam();

    AZ::Vertex::Format format(eVF);
    uint actualSize = format.GetStride();
    uint expectedSize = m_cSizeVF[eVF];
    ASSERT_EQ(actualSize, expectedSize);
}

TEST_P(VertexFormatTest, CalculateOffset_MatchesExpected)
{
    EVertexFormat eVF = (EVertexFormat)GetParam();

    AZ::Vertex::Format format(eVF);
    // TexCoord
    uint actualOffset = 0;
    bool hasOffset = format.TryCalculateOffset(actualOffset, AZ::Vertex::AttributeUsage::TexCoord);
    int expectedOffset = m_cBufInfoTable[eVF].OffsTC;
    if (expectedOffset >= 0)
    {
        ASSERT_TRUE(hasOffset);
        ASSERT_EQ(actualOffset, expectedOffset);
    }
    else
    {
        ASSERT_FALSE(hasOffset);
    }

    // Color
    hasOffset = format.TryCalculateOffset(actualOffset, AZ::Vertex::AttributeUsage::Color);
    expectedOffset = m_cBufInfoTable[eVF].OffsColor;
    if (expectedOffset >= 0)
    {
        ASSERT_TRUE(hasOffset);
        ASSERT_EQ(actualOffset, expectedOffset);
    }
    else
    {
        ASSERT_FALSE(hasOffset);
    }

    // Normal
    hasOffset = format.TryCalculateOffset(actualOffset, AZ::Vertex::AttributeUsage::Normal);
    expectedOffset = m_cBufInfoTable[eVF].OffsNorm;
    if (expectedOffset >= 0)
    {
        ASSERT_TRUE(hasOffset);
        ASSERT_EQ(actualOffset, expectedOffset);
    }
    else
    {
        ASSERT_FALSE(hasOffset);
    }
}

TEST_F(VertexFormatTest, CalculateOffsetMultipleUVs_MatchesExpected)
{
    AZ::Vertex::Format vertexFormat(eVF_P3F_T2F_T3F);
    uint offset = 0;
    // The first uv set exists
    ASSERT_TRUE(vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::TexCoord, 0));
    // First Texture coordinate comes after the position, which has 3 32 bit floats
    ASSERT_EQ(offset, AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Float32_3].byteSize);

    // The second uv set exists
    ASSERT_TRUE(vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::TexCoord, 1));
    // Second Texture coordinate comes after the position + the first texture coordinate
    ASSERT_EQ(offset, AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Float32_3].byteSize + AZ::Vertex::AttributeTypeDataTable[(int)AZ::Vertex::AttributeType::Float32_2].byteSize);

    // The third uv set does not exist
    ASSERT_FALSE(vertexFormat.TryCalculateOffset(offset, AZ::Vertex::AttributeUsage::TexCoord, 2));
}

TEST_P(VertexFormatTest, D3DVertexDeclarations_MatchesLegacy)
{
    EVertexFormat eVF = (EVertexFormat)GetParam();
    AZStd::vector<D3D11_INPUT_ELEMENT_DESC> expected = Legacy_InitBaseStreamD3DVertexDeclaration(eVF);
    bool matchesLegacy = true;
    // The legacy result of EF_InitD3DVertexDeclarations for the following formats are flat out wrong (it defaults to one D3D11_INPUT_ELEMENT_DESC that is a position, which is clearly not the case for any of these)
    // eVF_W4B_I4S is really blendweights + indices
    // eVF_C4B_C4B is really two spherical harmonic coefficients
    // eVF_P3F_P3F_I4B is really two positions and an index
    // eVF_P2F_T4F_T4F_C4F doesn't actually exist in the engine anymore
    // ignore these cases
    // Also ignore eVF_P2S_N4B_C4B_T1F: the T1F attribute has a POSITION semantic name in the legacy declaration, even though both the engine and shader treat it as a TEXCOORD (despite the fact that it is eventually used for a position)
    if (eVF != eVF_W4B_I4S && eVF != eVF_C4B_C4B && eVF != eVF_P3F_P3F_I4B && eVF != eVF_P2F_T4F_T4F_C4F && eVF != eVF_P2S_N4B_C4B_T1F && eVF != eVF_P2F_C4B_T2F_F4B)
    {
        AZStd::vector<D3D11_INPUT_ELEMENT_DESC> actual = GetD3D11Declaration(AZ::Vertex::Format(eVF));
        matchesLegacy = DeclarationsAreEqual(actual, expected);
    }
    ASSERT_TRUE(matchesLegacy);
}

// Instantiate tests
// Start with 1 to skip eVF_Unknown
INSTANTIATE_TEST_CASE_P(EVertexFormatValues, VertexFormatTest, ::testing::Range<int>(1, eVF_Max));


//Tests to check 4 byte alignment padding
class VertexFormat4ByteAlignedTest
    : public ::testing::TestWithParam < int >
    , public UnitTest::AllocatorsBase
{
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsBase::SetupAllocator();
        }

        void TearDown() override
        {
            UnitTest::AllocatorsBase::TeardownAllocator();
        }
};

TEST_P(VertexFormat4ByteAlignedTest, GetStride_4ByteAligned)
{
    const AZ::Vertex::Format g_PaddingTestCaseFormats[] =
    {
        { AZ::Vertex::Format({ AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::Position, AZ::Vertex::AttributeType::Byte_1) }) },
        { AZ::Vertex::Format({ AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::Position, AZ::Vertex::AttributeType::Byte_2) }) },
        { AZ::Vertex::Format({ AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::Position, AZ::Vertex::AttributeType::Byte_1), AZ::Vertex::Attribute(AZ::Vertex::AttributeUsage::Position, AZ::Vertex::AttributeType::Byte_2) }) }
    };

    AZ::Vertex::Format format = g_PaddingTestCaseFormats[GetParam()];
    ASSERT_EQ(format.GetStride() % AZ::Vertex::VERTEX_BUFFER_ALIGNMENT, 0);
}

INSTANTIATE_TEST_CASE_P(EVertexFormatStride4ByteAligned, VertexFormat4ByteAlignedTest, ::testing::Range<int>(0, 3));
