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

using System;
using System.Runtime.InteropServices;

namespace AssetTaggingImport
{
	public static class Win32
	{
		public const string LIBRARY_KERNEL = "kernel32.dll";

		[DllImport(LIBRARY_KERNEL, SetLastError = true, CharSet = CharSet.Auto)]
		public static extern IntPtr LoadLibrary(string lpFileName);

		[DllImport(LIBRARY_KERNEL, SetLastError = true, ExactSpelling = true)]
		public static extern bool FreeLibrary(IntPtr hModule);
	}

	public static class AssetTagging
	{
		#region DLL Paths
		public const string LIBRARY_ASSETTAGGING = "AssetTaggingTools.dll";
		#endregion

		#region Helper Functions
		private static IntPtr m_assettaggingapi = IntPtr.Zero;

		public static void LoadAssetTaggingLibraries()
		{
			m_assettaggingapi = Win32.LoadLibrary(LIBRARY_ASSETTAGGING);
		}

		public static void FreeAssetTaggingLibraries()
		{
			Win32.FreeLibrary(m_assettaggingapi);
			m_assettaggingapi = IntPtr.Zero;
		}
		#endregion
		
		#region DLL Import Functions
		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern Boolean AssetTagging_Initialize([In, MarshalAs(UnmanagedType.LPStr)] string localpath);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_MaxStringLen();

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_CloseConnection();


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_CreateTag([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_CreateAsset([In, MarshalAs(UnmanagedType.LPStr)] string asset, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_CreateProject([In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_CreateCategory([In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_AddAssetsToTag([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project, IntPtr assets, int nAssets);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_RemoveAssetsFromTag([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project, IntPtr assets, int nAssets);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_RemoveTagFromAsset([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project, [In, MarshalAs(UnmanagedType.LPStr)] string asset);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_DestroyTag([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_DestroyCategory([In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetNumTagsForAsset([In, MarshalAs(UnmanagedType.LPStr)] string asset, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetTagsForAsset(IntPtr tags, int nTags, [In, MarshalAs(UnmanagedType.LPStr)] string asset, [In, MarshalAs(UnmanagedType.LPStr)] string project);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetTagForAssetInCategory([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string asset, [In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetNumAssetsForTag([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetAssetsForTag(IntPtr assets, int nAssets, [In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string project);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetNumAssetsWithDescription([In, MarshalAs(UnmanagedType.LPStr)] string description);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetAssetsWithDescription([MarshalAs(UnmanagedType.LPArray)] IntPtr[] assets , int nAssets, [In, MarshalAs(UnmanagedType.LPStr)] string description);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetNumTags([In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetAllTags([MarshalAs(UnmanagedType.LPArray)] IntPtr[] tags, int nTags, [In, MarshalAs(UnmanagedType.LPStr)] string project);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetNumCategories([In, MarshalAs(UnmanagedType.LPStr)] string project);
		
		[DllImport(LIBRARY_ASSETTAGGING, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetAllCategories([In, MarshalAs(UnmanagedType.LPStr)] string project, [MarshalAs(UnmanagedType.LPArray)] IntPtr[] categories, int nCategories);
		
		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetNumTagsForCategory([In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetTagsForCategory([In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project, [MarshalAs(UnmanagedType.LPArray)] IntPtr[] tags, int nTags);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetNumProjects();

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetProjects([MarshalAs(UnmanagedType.LPArray)] IntPtr[] projects, int nProjects);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_TagExists([In, MarshalAs(UnmanagedType.LPStr)] string tag, [In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_AssetExists([In, MarshalAs(UnmanagedType.LPStr)] string relpath, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_ProjectExists([In, MarshalAs(UnmanagedType.LPStr)] string project);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_CategoryExists([In, MarshalAs(UnmanagedType.LPStr)] string category, [In, MarshalAs(UnmanagedType.LPStr)] string project);


		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern int AssetTagging_GetErrorString([MarshalAs(UnmanagedType.LPArray)] char[] errorString, int nLen);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern bool AssetTagging_GetAssetDescription([In, MarshalAs(UnmanagedType.LPStr)] string relpath, [In, MarshalAs(UnmanagedType.LPStr)] string project, [In, MarshalAs(UnmanagedType.LPStr)] char[] description, int nChars);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_SetAssetDescription([In, MarshalAs(UnmanagedType.LPStr)] string relpath, [In, MarshalAs(UnmanagedType.LPStr)] string project, [In, MarshalAs(UnmanagedType.LPStr)] string description);

		[DllImport(LIBRARY_ASSETTAGGING, CallingConvention = CallingConvention.Cdecl)]
		public static extern void AssetTagging_UpdateCategoryOrderId([In, MarshalAs(UnmanagedType.LPStr)] string category, int idx, [In, MarshalAs(UnmanagedType.LPStr)] string project);

		public static IntPtr[] Marshal2DToArray(int d1, int d2)
		{
			IntPtr[] buffer = new IntPtr[d1];

			for (int i = 0; i < d1; i++)
				buffer[i] = Marshal.AllocHGlobal(d2);

			return buffer;
		}
	
		public static string[] MarshalBufferToStringArray(IntPtr [] buffer, int size)
		{
			string[] output = new string[size];
			for (int i = 0; i < size; i++)
			{
				output[i] = Marshal.PtrToStringAnsi(buffer[i]);
				Marshal.FreeHGlobal(buffer[i]);
			}
			return output;
		}
		#endregion
	}
}