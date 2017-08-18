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

#include <ISerialize.h>

INTEG_TEST(CrySerializationUnitTests, TestVectorSerialization)
{
    std::vector<int> writeInts;
    std::vector<std::pair<short, short> > writeShortPairs;
    for (int index = 0; index < 5; ++index)
    {
        writeInts.push_back(index);
        writeShortPairs.push_back(std::make_pair(index, 5 - index));
    }

    int writeInt = 42;

    // Test writing
    {
        XmlNodeRef root = GetISystem()->CreateXmlNode("Test");
        IXmlSerializer* pSerializer = GetISystem()->GetXmlUtils()->CreateXmlSerializer();
        ISerialize* pWriter = pSerializer->GetWriter(root);
        TSerialize ser = TSerialize(pWriter);

        ser.Value("singleInt", writeInt);
        ser.Value("ints", writeInts);
        ser.Value("pairs", writeShortPairs);

        bool saved = root->saveToFile("@cache@/CrySerializationUnitTests.xml");
        EXPECT_TRUE(saved);
    }

    std::vector<int> readInts;
    std::vector<std::pair<short, short> > readShortPairs;
    int readInt = 0;

    // Read back the written XML
    {
        XmlNodeRef root = GetISystem()->LoadXmlFromFile("@cache@/CrySerializationUnitTests.xml");
        EXPECT_TRUE(root != nullptr);

        IXmlSerializer* pSerializer = GetISystem()->GetXmlUtils()->CreateXmlSerializer();
        ISerialize* pReader = pSerializer->GetReader(root);

        TSerialize ser = TSerialize(pReader);

        ser.Value("singleInt", readInt);
        EXPECT_TRUE(readInt == writeInt);
        ser.Value("ints", readInts);
        EXPECT_TRUE(readInts == writeInts);
        ser.Value("pairs", readShortPairs);
        EXPECT_TRUE(readShortPairs == writeShortPairs);
    }
}
