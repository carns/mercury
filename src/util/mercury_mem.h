/*
 * Copyright (C) 2013-2017 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#ifndef MERCURY_MEM_H
#define MERCURY_MEM_H

#include "mercury_util_config.h"

/**
 * Purpose: memory related utility functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get system default page size.
 *
 * \return page size on success or negative on failure
 */
HG_UTIL_EXPORT long
hg_mem_get_page_size(void);

/**
 * Allocate size bytes and return a pointer to the allocated memory.
 * The memory address will be a multiple of alignment, which must be a power of
 * two, and size should be a multiple of alignment.
 *
 * \param alignment [IN]        alignment size
 * \param size [IN]             total requested size
 *
 * \return a pointer to the allocated memory, or NULL in case of failure
 */
HG_UTIL_EXPORT void *
hg_mem_aligned_alloc(size_t alignment, size_t size);

/**
 * Free memory allocated from hg_aligned_alloc().
 *
 * \param mem_ptr [IN]          pointer to allocated memory
 */
HG_UTIL_EXPORT void
hg_mem_aligned_free(void *mem_ptr);

/**
 * Create/open a shared-memory mapped file of size \size with name \name.
 *
 * \param name [IN]             name of mapped file
 * \param size [IN]             total requested size
 * \param create [IN]           create file if not existing
 *
 * \return a pointer to the mapped memory region, or NULL in case of failure
 */
void *
hg_mem_shm_map(const char *name, size_t size, hg_util_bool_t create);

/**
 * Unmap a previously mapped region and close the file.
 *
 * \param name [IN]             name of mapped file
 * \param mem_ptr [IN]          pointer to mapped memory region
 * \param size [IN]             size range of the mapped region
 *
 * \return non-negative on success, or negative in case of failure
 */
int
hg_mem_shm_unmap(const char *name, void *mem_ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MERCURY_MEM_H */
