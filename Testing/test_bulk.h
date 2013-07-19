/*
 * Copyright (C) 2013 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#ifndef TEST_BULK_H
#define TEST_BULK_H

#include "mercury_macros.h"
#include "mercury_proc.h"

/* Dummy function that needs to be shipped */
size_t bla_write(int fildes, const void *buf, size_t nbyte);

#ifdef HG_HAS_BOOST
/* Generate processor and struct for required input/output structs
 * MERCURY_GEN_PROC( struct_type_name, fields )
 */
MERCURY_GEN_PROC( bla_write_in_t, ((hg_int32_t)(fildes)) ((hg_bulk_t)(bulk_handle)) )
MERCURY_GEN_PROC( bla_write_out_t, ((hg_uint64_t)(ret)) )
#else
/* Define bla_write_in_t */
typedef struct {
    hg_int32_t fildes;
    hg_bulk_t bulk_handle;
} bla_write_in_t;

/* Define hg_proc_bla_write_in_t */
static HG_INLINE int
hg_proc_bla_write_in_t(hg_proc_t proc, void *data)
{
    int ret = HG_SUCCESS;
    bla_write_in_t *struct_data = (bla_write_in_t *) data;

    ret = hg_proc_int32_t(proc, &struct_data->fildes);
    if (ret != HG_SUCCESS) {
        HG_ERROR_DEFAULT("Proc error");
        ret = HG_FAIL;
        return ret;
    }

    ret = hg_proc_hg_bulk_t(proc, &struct_data->bulk_handle);
    if (ret != HG_SUCCESS) {
        HG_ERROR_DEFAULT("Proc error");
        ret = HG_FAIL;
        return ret;
    }

    return ret;
}

/* Define bla_write_out_t */
typedef struct {
    hg_uint64_t ret;
} bla_write_out_t;

/* Define hg_proc_bla_write_out_t */
static HG_INLINE int
hg_proc_bla_write_out_t(hg_proc_t proc, void *data)
{
    int ret = HG_SUCCESS;
    bla_write_out_t *struct_data = (bla_write_out_t *) data;

    ret = hg_proc_uint64_t(proc, &struct_data->ret);
    if (ret != HG_SUCCESS) {
        HG_ERROR_DEFAULT("Proc error");
        ret = HG_FAIL;
        return ret;
    }

    return ret;
}
#endif

#endif /* TEST_BULK_H */
