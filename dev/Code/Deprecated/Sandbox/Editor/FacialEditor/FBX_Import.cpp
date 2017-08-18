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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Implementation of FBX file import and convert to facial sequence


#include "StdAfx.h"
#include "FacialEditorDialog.h"


#include <IFacialAnimation.h>							// for handling the actual facial sequence and it's data
#include "GenericSelectItemDialog.h"			// access to the listbox dialog class

#include <fbxsdk.h>												// for importing FBX files
#include <fbxsdk/fileio/fbxiosettings.h>	// for importing FBX files
#include <fbxsdk/scene/geometry/fbxdeformer.h>  // to handle the blendshapes


//////////////////////////////////////////////////////////////////////////



FbxScene* LoadFBXFile(const char* filename)
{
    // Create the FBX SDK manager
    FbxManager* lSdkManager = FbxManager::Create();

    // Create an IOSettings object.
    FbxIOSettings * ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
    lSdkManager->SetIOSettings(ios);

    // Create a new scene so it can be populated by the imported file.
    FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");

    // Create an importer.
    int lFileFormat = -1;
    FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
    if (!lSdkManager->GetIOPluginRegistry()->DetectReaderFileFormat(filename, lFileFormat))
    {
        // Unrecognizable file format. Try to fall back to FbxImporter::eFBX_BINARY
        lFileFormat = lSdkManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");;
    }

    // Initialize the importer.
    bool lImportStatus = lImporter->Initialize(filename, lFileFormat/*, lSdkManager->GetIOSettings()*/);
    if (!lImportStatus)
    {
        AfxMessageBox(lImporter->GetStatus().GetErrorString(), MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        CryLog("[ERROR] Facial Editor: Could not import FBX file. Importer return error: '%s'", lImporter->GetStatus().GetErrorString());
        return NULL;
    }

    // Import the contents of the file into the scene.
    lImporter->Import(lScene);

    // The file has been imported; we can get rid of the importer.
    lImporter->Destroy();

    return lScene;
}

//////////////////////////////////////////////////////////////////////////

FbxAnimStack* GetAnimStackToUse(FbxScene* lScene, CWnd* parent)
{
    // Is there at least one animation in the FBX file?
    int numAnimStacks = lScene->GetSrcObjectCount(FbxAnimStack::ClassId);
    if (numAnimStacks <= 0)
    {
        AfxMessageBox("FBX file has no animation. Aborting import.", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        CryLog("[ERROR] Facial Editor: FBX file has no animation. Aborting import.");
        return NULL;
    }
    //	CryLogAlways("Found %i AnimStacks", numAnimStacks);

    FbxAnimStack* pAnimStack = FbxCast<FbxAnimStack>(lScene->GetSrcObject(FbxAnimStack::ClassId, 0));

    // Is there more than one animation in the FBX file? Let the user choose which one to use
    if (numAnimStacks > 1)
    {
        std::vector<CString> items;
        for (int n = 0; n < numAnimStacks; ++n)
        {
            FbxAnimStack* tempAnimStack = FbxCast<FbxAnimStack>(lScene->GetSrcObject(FbxAnimStack::ClassId, n));
            CString animName = tempAnimStack->GetName();
            items.push_back(animName);
            //		CryLogAlways("AnimStack #%i: %s", n, pAnimStack->GetName());
        }

        CGenericSelectItemDialog dlg(parent);
        dlg.SetTitle(CString(MAKEINTRESOURCE(IDS_FBXIMPORTER_SELECT_ANIMATION_TITLE)));
        dlg.SetItems(items);
        if (dlg.DoModal() != IDOK)
            return NULL;
        CString selectStack = dlg.GetSelectedItem();

        // Find the one with the right name (cannot access by index of selected item, since the listbox is auto-sorted)
        for (int n = 0; n < numAnimStacks; ++n)
        {
            pAnimStack = FbxCast<FbxAnimStack>(lScene->GetSrcObject(FbxAnimStack::ClassId, n));
            if (selectStack.Compare(pAnimStack->GetName()) == 0)
                break;
        }
    }


    // we can only import one animation layer
    if (pAnimStack->GetMemberCount(FbxCriteria::ObjectType(FbxAnimLayer::ClassId)))
    {
        AfxMessageBox("Found more than one layer in Animation Stack.\nOnly the first layer will be imported.", MB_OK | MB_ICONEXCLAMATION);
        CryLog("[Warning] Facial Editor: Found more than one animation layer in FBX file. Only first layer will be imported!");
    }

    //CryLogAlways("Using AnimStack %s", pAnimStack->GetName());

    // Get the animation to import
    return pAnimStack;
}

//////////////////////////////////////////////////////////////////////////

FbxNode* GetMeshToUse(FbxNode* lRootNode, CWnd* parent)
{
    // Traverse through all nodes and find the meshes
    // If more than one is found, let the user pick the one he wants
    std::vector<CString> items;
    int childCount = lRootNode->GetChildCount();
    for (int i = 0; i < childCount; ++i)
    {
        FbxNode* node = lRootNode->GetChild(i);
        if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            CString meshName = node->GetNameOnly();
            items.push_back(meshName);
        }
    }
    if (items.size() < 0)
    {
        AfxMessageBox("FBX file contains no meshes. Aborting import.", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        CryLog("[ERROR] Facial Editor: FBX file contains no meshes. Aborting import.");
        return NULL;
    }

    CString meshToUse = items[0];
    if (items.size() > 1)
    {
        CGenericSelectItemDialog dlg(parent);
        dlg.SetTitle(CString(MAKEINTRESOURCE(IDS_FBXIMPORTER_SELECT_MODEL_TITLE)));
        dlg.SetItems(items);
        if (dlg.DoModal() != IDOK)
            return NULL;
        meshToUse = dlg.GetSelectedItem();
    }

    for (int i = 0; i < childCount; ++i)
    {
        FbxNode* pMeshNode = lRootNode->GetChild(i);
        if (pMeshNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            if (meshToUse.Compare(pMeshNode->GetNameOnly()) == 0)
                return pMeshNode;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CFacialEditorDialog::LoadFBXToSequence(const char* filename)
{
    // Create a sequence and add a folder to it
    IFacialAnimSequence* newSequence = GetISystem()->GetIAnimationSystem()->GetIFacialAnimation()->CreateSequence();

    // Import FBX and convert to sequence
    if (!ImportFBXAsSequence(newSequence, filename))
    {
        SAFE_DELETE(newSequence);
        return;
    }

    // Prompt user whether he wants to merge with the current sequence or replace it
    if (m_pContext->GetSequence() == NULL || AfxMessageBox("Do you want to replace the current sequence?\n(Select No to merge with current sequence)", MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL) == IDYES)
    {
        // Replace
        m_pContext->SetSequence(newSequence);
    }
    else
    {
        // Merge
        m_pContext->ImportSequence(newSequence, this);
    }
}

//////////////////////////////////////////////////////////////////////////

bool CFacialEditorDialog::ImportFBXAsSequence(IFacialAnimSequence* newSequence, const char* filename)
{
    // Load the relevant data and put it into a temporary XML structure that the facial sequence can load from
    FbxScene* pScene = LoadFBXFile(filename);
    if (pScene == NULL)
        return false;
    // Get the root node of the FBX scene
    FbxNode* lRootNode = pScene->GetRootNode();

    // Retrieve the anim stack to use and display a listbox dialog if there is more than one
    FbxAnimStack* pAnimStack = GetAnimStackToUse(pScene, this);
    if (pAnimStack == NULL)
        return false;
    // only the first layer of animation will be imported
    FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>(0);

    // Get the mesh model to import the animation from, display a listbox if there is more than one
    FbxNode* pMeshNode = GetMeshToUse(lRootNode, this);
    if (pMeshNode == NULL)
        return false;

    // Safety check whether there actually ARE any morphs on this mesh
    FbxMesh* pMesh = pMeshNode->GetMesh();
    if (pMesh->GetDeformerCount(FbxDeformer::eBlendShape) <= 0 || pMesh->GetShapeCount() <= 0)
    {
        AfxMessageBox("Found no deformer/morpher and/or no blendshapes in selected model. Aborting import.", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        CryLog("[Error] Facial Editor FBX Importer: Found no deformer/morpher or no blendshapes in selected model. Aborting import");
        return false;
    }


    // Create an FSQ format XML structure to load the data into
    XmlNodeRef xmlRoot = XmlHelpers::CreateXmlNode("FacialSequence");
    // add all the nodes and attributes the Facial Editor requires as a minimum
    xmlRoot->setAttr("Flags", "0");

    // Get the animation start and end times
    double secondStartTime = pAnimStack->GetLocalTimeSpan().GetStart().GetSecondDouble();
    double secondEndTime = pAnimStack->GetLocalTimeSpan().GetStop().GetSecondDouble();
    xmlRoot->setAttr("StartTime", secondStartTime);
    xmlRoot->setAttr("EndTime", secondEndTime);
    double duration = secondEndTime - secondStartTime;
    //CryLogAlways("Found Anim Stack to contain animation from %.2f to %.2f seconds (duration %.4f seconds)", secondStartTime, secondEndTime, duration);

    // Create a folder, so that all imported data goes into a subfolder
    // This way things will stay organized even if the scene is merged with an existing one
    // Create the folder (=channel/group) into which this data is loaded
    XmlNodeRef channelNode = xmlRoot->createNode("Channel");
    channelNode->setAttr("Flags", "1"); // = IFacialAnimChannel::FLAG_GROUP
    xmlRoot->addChild(channelNode);
    // Name the folder 
    string channelName = "FBX_Import_";
    channelName.append(pMeshNode->GetNameOnly());
    channelName.append("_");
    channelName.append(pScene->GetName());
    channelNode->setAttr("Name", channelName);

    // Go through all the blendshape deformers, and through all their blendshapes, get their animation curves and put them into the XML structure
    bool warningAboutMultiTargetsShowed = false;
    int deformerCount = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
    for (int deformerIdx = 0; deformerIdx < deformerCount; ++deformerIdx)
    {
        FbxBlendShape* pDeformer = (FbxBlendShape*)pMesh->GetDeformer(deformerIdx, FbxDeformer::eBlendShape);
        int blendShapeChannelCount = pDeformer->GetBlendShapeChannelCount();
        for (int blendShapeChannelIdx = 0; blendShapeChannelIdx < blendShapeChannelCount; ++blendShapeChannelIdx)
        {
            FbxBlendShapeChannel* pChannel = pDeformer->GetBlendShapeChannel(blendShapeChannelIdx);
            if (!pChannel)
                continue;

            FbxAnimCurve* pAnimCurve = pMesh->GetShapeChannel(deformerIdx, blendShapeChannelIdx, pAnimLayer);
            if (!pAnimCurve)
                continue;

            // Only the first target shape in a blendchannel will be evaluated. The rare case of multiple target shapes in a channel is not supported.
            FbxShape* pShape = pMesh->GetShape(deformerIdx, blendShapeChannelIdx, 0);
            string shapeName = pShape->GetNameOnly();

            if (pChannel->GetTargetShapeCount() > 1 && !warningAboutMultiTargetsShowed)
            {
                AfxMessageBox("Found one or more blend channels that contain more that one morph target shape.\nThis is not supported, only the first shape will be evaluated.", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                CryLog("[WARNING] Facial Editor: Found one or more blend channels that contain more that one morph target shape.\nThis is not supported, only the first shape will be evaluated.");
                // only show this warning once per import, to avoid making the user click a hundred times
                warningAboutMultiTargetsShowed = true;
            }


            int keyCount = pAnimCurve->KeyGetCount();
            //CryLogAlways("Evaluating blendchannel %s. Found %i keys", pChannel->GetName(), keyCount);

            // Create a channel in the xml for this blendshape
            XmlNodeRef blendShapeChannel = channelNode->createNode("Channel");
            blendShapeChannel->setAttr("Flags", "0");
            blendShapeChannel->setAttr("Name", shapeName);

            // all keys are stored in one long string inside the xml
            CString animKeys = "";
            string curKey = "";
            double keyTime = 0;
            float keyValue = 0;

            // Optimization - don't create a channel if the morph has only 0 keys
            bool allZeroKeys = true;

            for (int keyIdx = 0; keyIdx < keyCount; ++keyIdx)
            {
                FbxAnimCurveKey animKey = pAnimCurve->KeyGet(keyIdx);
                FbxAnimCurveDef::EInterpolationType keyInterpolType = animKey.GetInterpolation();
                keyTime = animKey.GetTime().GetSecondDouble();
                keyValue = animKey.GetValue() / 100.0f;   // Morphs go from 0..100, but Facial Editor from 0..1

                // Convert the above data into an FSQ style key entry (still missing the tangent/interpolation type)
                // There is no easy way to convert the interpolation type, so we will use zeroed tangents for now (Flags 18) - which matches the default in Max closest
                curKey.Format("%f:%f:18,", keyTime, keyValue);
                animKeys.Append(curKey);

                if (keyValue != 0.0f)
                    allZeroKeys = false;
            }

            // If the last key is not at the end of the time, add another key there with the same values
            if (keyTime < secondEndTime)
            {
                curKey.Format("%f:%f:18,", secondEndTime, keyValue);
                animKeys.Append(curKey);
            }

            // Create a node for the actual keys on the timeline
            if (!allZeroKeys)
            {
                channelNode->addChild(blendShapeChannel);
                XmlNodeRef splineNode = blendShapeChannel->createNode("Spline");
                splineNode->setAttr("Keys", animKeys);
                blendShapeChannel->addChild(splineNode);
            }
        }
    }

    // Check whether any keys for anything have been loaded (if the channel group has any children at all)
    // and give a warning that no blendshape animation could be found in TAKE or LAYER
    if (channelNode->getChildCount() == 0)
    {
        AfxMessageBox("Animation Take contained no keyframes for any blendshapes (only layer 0 was evaluated). Aborting Import.", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        CryLog("[WARNING] Facial Editor: Animation Take contained no keyframes for any blendshapes (only layer 0 was evaluated). Aborting Import");
        return false;
    }

    // Now that the XML structure is prepared, use the internal serialize functionality to load the data
    newSequence->Serialize(xmlRoot, true);

    // Get the name of the scene and set it as the sequence name
    newSequence->SetName(pScene->GetNameOnly());

    // Save out the FSQ as a temp file. This helps with debugging later (if this is enabled in console)
    if (gEnv->pConsole->GetCVar("fe_fbx_savetempfile")->GetIVal() != 0)
        xmlRoot->saveToFile("Animations/temp/Converted_FBX_Temp.fsq");

    return true;
}

//////////////////////////////////////////////////////////////////////////



