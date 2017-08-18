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

#ifndef CRYINCLUDE_CRYCOMMON_CRYDLMALLOC_H
#define CRYINCLUDE_CRYCOMMON_CRYDLMALLOC_H
#pragma once

typedef void* dlmspace;

#ifdef __cplusplus
extern "C"
{
#endif

typedef void* (* dlmmap_handler)(void* user, size_t sz);
typedef int (* dlmunmap_handler)(void* user, void* mem, size_t sz);
static void* const dlmmap_error = (void*)(INT_PTR)-1;

int dlmspace_create_overhead(void);
dlmspace dlcreate_mspace(size_t capacity, int locked, void* user = NULL, dlmmap_handler mmap = NULL, dlmunmap_handler munmap = NULL);
size_t dldestroy_mspace(dlmspace msp);
dlmspace dlcreate_mspace_with_base(void* base, size_t capacity, int locked);
int dlmspace_track_large_chunks(dlmspace msp, int enable);
void* dlmspace_malloc(dlmspace msp, size_t bytes);
void dlmspace_free(dlmspace msp, void* mem);
void* dlmspace_realloc(dlmspace msp, void* mem, size_t newsize);
void* dlmspace_calloc(dlmspace msp, size_t n_elements, size_t elem_size);
void* dlmspace_memalign(dlmspace msp, size_t alignment, size_t bytes);
void** dlmspace_independent_calloc(dlmspace msp, size_t n_elements, size_t elem_size, void* chunks[]);
void** dlmspace_independent_comalloc(dlmspace msp, size_t n_elements, size_t sizes[], void* chunks[]);
size_t dlmspace_footprint(dlmspace msp);
size_t dlmspace_max_footprint(dlmspace msp);
size_t dlmspace_usable_size(void* mem);
void dlmspace_malloc_stats(dlmspace msp);
size_t dlmspace_get_used_space(dlmspace msp);
int dlmspace_trim(dlmspace msp, size_t pad);
int dlmspace_mallopt(int, int);

#ifdef __cplusplus
}
#endif

#endif // CRYINCLUDE_CRYCOMMON_CRYDLMALLOC_H
