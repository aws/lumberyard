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

// Auto-generated AZCore release notes
///
/// \page ReleaseNotes Release Notes
///
/// \section Build275 Build 275 Tue 01/29/2013 15:39:04.37
/// - Added binary version of object streams.
///
/// \section Build237 Build 237 Fri 11/02/2012 17:22:34.74
/// - Added support for user setting overrides through UserSettingsComponent.
/// - Smart pointer serialization (shared_ptr,intrusive_ptr)
/// - EditContext support.
/// - Add a generic EnumerateInstance() function to SerializeContext that can be used for anything that requires traversing an instance hierarchy (save/populate UI, etc).
/// - Add IsOpen() function to GenericStream.
/// - Added plane line-ray intersections.
///
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(ReleaseNotes_h, AZ_RESTRICTED_PLATFORM)
#endif
/// \section Build177 Build 177 Thu 06/21/2012 10:00:03.17
/// - Added AssetDatabase
/// - Added generic stream wrappers.
///
/// \section Build173 Build 173 Mon 06/18/2012 13:07:37.31
/// - Added Sha1 support. Uuid can be generated based on a string.
///
/// \section Build170 Build 170 Wed 06/13/2012 17:36:48.15
/// - Add support for serialization of containers with polymorphic types and add a series of unit tests.
/// - VS11 project files.
/// - Class serialization to XML format.
///
/// \section Build169 Build 169 Wed 05/09/2012 17:24:45.81
/// - Added basic, manual, fast custom RTTI support
/// - SCARY iterators, renamed slist to forward_list (C++11), added VS visualizer support for rbtree (set/map/etc).
///
/// \section Build164 Build 164 Thu 04/12/2012 11:01:01.41
/// - SFMT fast marsenne twister pseudorandom number generator(SIMD optimized). A 128 bit Uuid compatible with the standart (MS,RFC 4122,etc).
///
/// \section Build160 Build 160 Sat 04/07/2012 18:07:24.60
/// - Script support, with LUA integration.
///
/// \section Build154 Build 154 Wed 01/25/2012 10:51:11.44
/// - RValues basic support (still have some containers, thread, atomics to update). Changed the AZ_STATIC_ASSERT format to include a string.
///
/// \section Build109 Build 109 Fri 01/06/2012 11:11:56.65
/// - SNC compiler support. For VS2010 SNC is the default PPU compiler.
/// - Component framework.
///
/// \section Build106 Build 106 Mon 12/12/2011 18:35:32.45
/// - EBus supports queued events.
///
/// \section Build104 Build 104 Fri 12/09/2011 13:48:15.29
/// - Expose streamer caching parameters in the streamer descriptor.
///
/// \section Build98 Build 98 Wed 11/23/2011 13:24:48.37
/// - Initial VS2010 support (no optimizations)
///
/// \section Build91 Build 91 Tue 11/01/2011 12:05:31.28
/// - Added memory allocator statistics for user requested bytes and peak of requested bytes.
///
/// \section Build73 Build 73 Fri 09/16/2011 14:27:51.33
/// - Optimized load/store functions from floats with regular alignment, at least for SSE
///
/// \section Build50 Build 50 Mon 08/08/2011 14:11:26.12
/// - Added RapidXML implementaion. All allocations are piped trough the system allocator.
///
/// \section Build48 Build 48 Thu 08/04/2011 18:52:06.38
/// - Added exeption handling (when no debugger is present). Resolve symbols from (map) files.
/// - Allocation variadic macros. Crc32::Add function.
/// and it's a lot simpler to have one and no need to maintain independent layers.
///
/// \page ReleaseNotes Release Notes
///
/// From AZStd
///
/// \section Build139 Build 139 Mon 06/27/2011 12:01:04.16
/// - Added AZ_PRAGMA_ONCE which expands to #pragma once for the platforms that support it.
///
/// \section Build137 Build 137 Thu 06/02/2011 19:34:07.31
/// - Moved bind placeholders (_1,_2,_N) to AZStd::placeholders namespace as in the C++0x standard.
///
/// \section Build134 Build 134 Thu 04/28/2011 16:30:04.26
/// - rbtree (set/multiset/map/multimap) erase functions return iterator to next node (C++0x).
///
/// \section Build133 Build 133 Wed 04/27/2011 13:07:09.74
/// - rbtree (set/multiset/map/multimap) erase functions return iterator to next node (C++0x).
///
/// \section Build129 Build 129 Fri 04/08/2011 16:52:31.85
/// - Added extensions to get underlying buffer for bitsets.
///
/// \section Build126 Build 126 Fri 04/01/2011 12:44:19.21
/// - Containers are more permissive towards incomplete types. Although the standard defines the bahavior as undefined, we do support it in certain cases.
///
/// \section Build115 Build 115 Fri 03/11/2011 19:33:43.86
/// - Checked iterators are disabled by default.
/// - Speed up for check iterators, especially on windows.
///
/// \section Build111 Build 111 Tue 03/08/2011 22:21:12.89
/// - EBus (Event Bus) implementation.
///
/// \section Build108 Build 108 Thu 03/03/2011 14:00:29.12
/// - Added smart pointers implementation.
///
/// \section Build107 Build 107 Thu 02/24/2011 14:12:50.06
/// - Vector container uses allocator resize function to attempt to expand the current memory block before relocating.
///
/// \section Build104 Build 104 Tue 01/25/2011 13:35:20.32
/// - Fixed some relative path includes.
///
/// \section Build102 Build 102 Wed 01/12/2011 11:41:46.67
/// - Fixed a deque::pop_front bug.
///
/// \section Build93 Build 93 Mon 11/08/2010 10:25:18.71
/// - Added hash support for string and wstring.
///
/// \section Build92 Build 92 Thu 11/04/2010 17:44:12.60
/// - Fixed strict aliasing bug in endian_swap
///
/// \section Build90 Build 90 Tue 10/26/2010 21:40:50.87
/// - Removed the need to define any macros, you need to implement a default functions (if you use default allocator), fixed the assert format.
/// - Added concurrent_vector
///
/// \section Build89 Build 89 Mon 10/18/2010  0:21:56.16
/// - Added combinable
///
/// \section Build87 Build 87 Mon 10/11/2010 20:03:23.84
/// - New work stealing queue implementation, previous one has at least one extremely subtle bug
/// - Added missing include to the map.h
///
/// \section Build86 Build 86 Tue 09/14/2010  5:37:59.38
/// - Added necessary includes to the string.h
///
/// \section Build85 Build 85 Fri 09/03/2010 15:45:16.25
/// - Added a missing cast, causing a compile warning in certain cases.
///
/// \section Build83 Build 83 Mon 08/30/2010 14:15:17.94
/// - Fix to memset fill specializations.
///
/// \section Build79 Build 79 Fri 05/07/2010 10:50:41.82
/// - Hash table containers (list and vector) share the same allocator.
///
/// \section Build78 Build 78 Wed 05/05/2010 10:40:34.78
/// - Hash table multimap, erase fix.
///
/// \section Build77 Build 77 Wed 03/03/2010 18:51:24.73
/// - Timer fixes for windows 7.
///
/// \section Build76 Build 76 Tue 03/02/2010 14:01:32.66
/// - Fixed aligned_storege (align_to) typo
///
/// \section Build71 Build 71 Mon 01/04/2010 13:08:23.27
/// - Fixed issue with rbtree copy constructor.
///
/// \section Build69 Build 69 Fri 12/11/2009 18:07:44.51
/// - Added endian_swap utility.
///
/// \section Build68 Build 68 Wed 12/09/2009 16:05:50.75
/// - Added bitset template. Platform enum for tools. Removed direct use of AZSTD_DEFAULT_ALLOCATOR instead we use the system alloc/free macros.
///
/// \section Build65 Build 65 Tue 12/08/2009 16:38:11.91
/// - Added spin_mutex
///
/// \section Build54 Build 54 Mon 10/26/2009 19:56:48.29
/// - Removed thread constructors which take additional parameters for the thread function. These are in the standard, but conflict with our thread_desc extension. Since we have AZStd::function, they are unnecessary anyway.
///
/// \section Build51 Build 51 Mon 10/19/2009 10:46:04.76
/// - Release notes are auto-generated from now on.
///
/// \section Build50 Build 50 Mon 10/15/2009 11:15:23.11
/// - red-black tree rbtree
/// - map, multimap, set and multiset containers
/// - basic_string, string, wstring, stoi(d,f), to_string, tolower/toupper, (w)str_format
/// - Parallel: C++0x atomics, C++0x threads, C++0x mutex, recursive mutex
///
/// \section Build20 Build 20 Tue 05/05/2009 09:45:54.43
/// - chrono, time, ratio (clocks and utils)
/// - C++0x function, bind
/// - delegate
/// - Parallel: C++0x atomics, C++0x threads, C++0x mutex, recursive mutex
///
/// \section Build2 Build 13 Fri 02/20/2009 11:10:32.22
/// - Containers: slist, fixed_slist, fixed_unordered_set, fixed_unordered_multiset, fixed_unordered_map, fixed_unordered_multimap.
/// - Algorithms: heap_sort, insertion_sort, stable_sort, sort.
///
/// \section Build1 Build 1 Mon 01/10/2009 10:27:43.12
/// - Containers: array, vector, fixed_vector, list, fixed_list, intrusive_list, deque, queue, stack, priority_queue, unordered_map, unordered_multimap, unordered_set, unordered_multiset.
/// - Algorithms: find, find_if, heap, partial_sort.
/// - Type Traits: All \ref CTR1 (with or without compiler \ref CTR1 support).
/// - Parallel: atomic, mutex, lock, semaphore.
///
/// \section Build101 Build 101 Mon 07/11/2011 17:24:11.58
/// - Made SimpleLcgRandom a bit less crap
/// - Quaternion::CreateFromAngleAxis
///
/// \section Build94 Build 94 Mon 06/27/2011 14:32:56.19
/// - Basis functions, scale functions, and other misc functions added to Matrix3x3 and Transform.
///
/// \section Build93 Build 93 Thu 06/09/2011 20:15:38.65
/// - Basis functions, scale functions, and other misc functions added to Matrix3x3 and Transform.
///
/// \section Build89 Build 89 Wed 03/23/2011 18:36:34.64
/// - Added crc fix tool (/crcfix/crcfix.exe) which we can call to pre process all AZ_CRC macro calls in the source code.
///
/// \section Build88 Build 88 Thu 03/17/2011 18:11:55.51
/// - Added missing operators to Vector4
///
/// \section Build87 Build 87 Tue 03/15/2011 14:08:25.17
/// - Added AZ_CRC macro with preprocessing.
///
/// \section Build84 Build 84 Fri 02/25/2011 15:50:50.63
/// - Added VectorFloat::operator== and operator=, was getting some unnecessary float conversion
///
/// \section Build77 Build 77 Tue 01/18/2011 18:53:30.83
/// - Added generic HSM (Hierarchical State Machine).
///
/// \section Build72 Build 72 Wed 12/01/2010 11:02:07.47
/// - Allowed full reroute of the trace messages.
///
/// \section Build71 Build 71 Fri 11/05/2010 10:44:05.48
/// - Fixed the SF_OPEN_APPEND mode on windows.
///
/// \section Build69 Build 69 Tue 10/26/2010 22:02:36.42
/// - Pipe the default AZStd assert function trough the Trace system.
///
/// \section Build57 Build 57 Wed 12/09/2009 18:16:38.02
/// - Turn off stack tracers in release (use DebugOpt build for testing).
/// - Matrix3x3 SIMD optimization
///
/// \section Build56 Build 56 Fri 12/04/2009 14:22:43.81
/// - Matrix4x4 SIMD optimization
///
/// \section Build55 Build 55 Tue 12/01/2009 17:04:52.12
/// - Added DebugOptimized build. Added hook ups for Trace::Output.
///
/// \section Build52 Build 52 Fri 11/20/2009 11:45:27.64
/// - Added Vector*::CreateAxis* functions
///
/// \section Build50 Build 50 Wed 11/18/2009 17:27:44.66
/// - Added IsFinite function to all math classes
///
/// \section Build34 Build 34 Wed 10/21/2009 18:02:34.07
/// - Added various tests and intersection functions for points and segments.
///
/// \section Build33 Build 33 Tue 10/20/2009 17:21:23.50
/// - Fixed the plane gettransform function. Added some plane tests to the unit test.
///
/// \section Build26 Build 26 Mon 10/19/2009 16:47:36.51
/// - Auto-generated release notes is enabled.
///
/// \section Build30 Build 30 Wed 05/11/2011 12:01:59.92
/// - Added IPC (Inter Process Communication) support (windows only). SharedMemory/SharedMemoryRingBuffer/DirectSocket.
///
/// \section Build21 Build 21 Thu 03/24/2011 18:21:02.58
/// - Driller framework. Compression utils.
/// - Added HightPerformanceHeapAllocator scheme. Debug memory break points. Resize interface to the allocators (AZStd::vector - takes advantage). Report size and alignment to DeAllocate when possble. Customizable stack recording levels.
///
//
