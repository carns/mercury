/*
 * Copyright (C) 2013-2019 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#include "mercury_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern hg_id_t hg_test_rpc_open_id_g;
extern hg_id_t hg_test_rpc_open_id_no_resp_g;

#define NINFLIGHT 32

struct forward_cb_args {
    hg_request_t *request;
    rpc_handle_t *rpc_handle;
};

//#define HG_TEST_DEBUG
#ifdef HG_TEST_DEBUG
#define HG_TEST_LOG_DEBUG(...)                                \
    HG_LOG_WRITE_DEBUG(HG_TEST_LOG_MODULE_NAME, __VA_ARGS__)
#else
#define HG_TEST_LOG_DEBUG(...) (void)0
#endif

/*---------------------------------------------------------------------------*/
/**
 * HG_Forward callback
 */
static hg_return_t
hg_test_rpc_forward_cb(const struct hg_cb_info *callback_info)
{
    hg_handle_t handle = callback_info->info.forward.handle;
    struct forward_cb_args *args = (struct forward_cb_args *) callback_info->arg;
    int rpc_open_ret;
    int rpc_open_event_id;
    rpc_open_out_t rpc_open_out_struct;
    hg_return_t ret = HG_SUCCESS;

    if (callback_info->ret != HG_SUCCESS) {
        HG_TEST_LOG_DEBUG("Return from callback info is not HG_SUCCESS");
        goto done;
    }

    /* Get output */
    ret = HG_Get_output(handle, &rpc_open_out_struct);
    if (ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not get output");
        goto done;
    }

    /* Get output parameters */
    rpc_open_ret = rpc_open_out_struct.ret;
    rpc_open_event_id = rpc_open_out_struct.event_id;
#if 0
    HG_TEST_LOG_DEBUG("rpc_open returned: %d with event_id: %d", rpc_open_ret,
        rpc_open_event_id);
    (void)rpc_open_ret;
    if (rpc_open_event_id != (int) args->rpc_handle->cookie) {
        HG_TEST_LOG_ERROR("Cookie did not match RPC response");
        goto done;
    }
#endif

    /* Free request */
    ret = HG_Free_output(handle, &rpc_open_out_struct);
    if (ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not free output");
        goto done;
    }

done:
    hg_request_complete(args->request);
    return ret;
}

/*---------------------------------------------------------------------------*/
/**
 * HG_Forward callback (no response)
 */
static hg_return_t
hg_test_rpc_forward_no_resp_cb(const struct hg_cb_info *callback_info)
{
    struct forward_cb_args *args = (struct forward_cb_args *) callback_info->arg;
    hg_return_t ret = HG_SUCCESS;

    if (callback_info->ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Return from callback info is not HG_SUCCESS");
        goto done;
    }

done:
    hg_request_complete(args->request);
    return ret;
}

/*---------------------------------------------------------------------------*/
static hg_return_t
hg_test_rpc_forward_reset_cb(const struct hg_cb_info *callback_info)
{
    struct forward_cb_args *args = (struct forward_cb_args *) callback_info->arg;
    hg_return_t ret = HG_SUCCESS;

    if (callback_info->ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Return from callback info is not HG_SUCCESS");
        goto done;
    }

    ret = HG_Reset(callback_info->info.forward.handle, HG_ADDR_NULL, 0);
    if (ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not reset handle");
        goto done;
    }

done:
    hg_request_complete(args->request);
    return ret;
}

/*---------------------------------------------------------------------------*/
static hg_return_t
hg_test_rpc(hg_context_t *context, hg_request_class_t *request_class,
    hg_addr_t addr, hg_id_t rpc_id, hg_cb_t callback, int in_size)
{
    hg_request_t *request = NULL;
    hg_handle_t handle;
    hg_return_t hg_ret = HG_SUCCESS;
    struct forward_cb_args forward_cb_args;
    hg_const_string_t rpc_open_path = MERCURY_TESTING_TEMP_DIRECTORY "/test.h5";
    rpc_handle_t rpc_open_handle;
    rpc_open_in_t  rpc_open_in_struct;
    uint64_t *tester;
    int i;

    request = hg_request_create(request_class);

    /* Create RPC request */
    hg_ret = HG_Create(context, addr, rpc_id, &handle);
    if (hg_ret != HG_SUCCESS) {
        if (hg_ret != HG_NO_MATCH)
            HG_TEST_LOG_ERROR("Could not create handle");
        goto done;
    }

    /* set additional field in mercury header */
    hg_ret = HG_Get_input_buf(handle, (void**)&tester, NULL);
    assert(hg_ret == HG_SUCCESS);
    *tester = 123456;

    /* Fill input structure */
    rpc_open_handle.cookie = 100;
    rpc_open_in_struct.path = calloc(in_size, 1);
    for(i=0; i<in_size-1; i++)
        rpc_open_in_struct.path[i] = 'a';

    rpc_open_in_struct.handle = rpc_open_handle;

    /* Forward call to remote addr and get a new request */
    HG_TEST_LOG_DEBUG("Forwarding rpc_open, op id: %u...", rpc_id);
    forward_cb_args.request = request;
    forward_cb_args.rpc_handle = &rpc_open_handle;
    hg_ret = HG_Forward(handle, callback, &forward_cb_args,
        &rpc_open_in_struct);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not forward call");
        goto done;
    }

    hg_request_wait(request, HG_MAX_IDLE_TIME, NULL);

    /* Complete */
    hg_ret = HG_Destroy(handle);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not destroy handle");
        goto done;
    }

done:
    hg_request_destroy(request);
    return hg_ret;
}

/*---------------------------------------------------------------------------*/
static hg_return_t
hg_test_rpc_reset(hg_context_t *context, hg_request_class_t *request_class,
    hg_addr_t addr, hg_id_t rpc_id, hg_cb_t callback)
{
    hg_request_t *request = NULL;
    hg_handle_t handle;
    hg_return_t hg_ret = HG_SUCCESS;
    struct forward_cb_args forward_cb_args;
    hg_const_string_t rpc_open_path = MERCURY_TESTING_TEMP_DIRECTORY "/test.h5";
    rpc_handle_t rpc_open_handle;
    rpc_open_in_t  rpc_open_in_struct;

    request = hg_request_create(request_class);

    /* Create request with invalid RPC id */
    hg_ret = HG_Create(context, HG_ADDR_NULL, 0, &handle);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not create handle");
        goto done;
    }

    /* Reset with valid addr and ID */
    hg_ret = HG_Reset(handle, addr, rpc_id);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not reset handle");
        goto done;
    }

    /* Fill input structure */
    rpc_open_handle.cookie = 100;
    rpc_open_in_struct.path = rpc_open_path;
    rpc_open_in_struct.handle = rpc_open_handle;

    /* Forward call to remote addr and get a new request */
    HG_TEST_LOG_DEBUG("Forwarding rpc_open, op id: %u...", rpc_id);
    forward_cb_args.request = request;
    forward_cb_args.rpc_handle = &rpc_open_handle;
    hg_ret = HG_Forward(handle, callback, &forward_cb_args,
        &rpc_open_in_struct);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not forward call");
        goto done;
    }

    hg_request_wait(request, HG_MAX_IDLE_TIME, NULL);

    /* Complete */
    hg_ret = HG_Destroy(handle);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not destroy handle");
        goto done;
    }

    hg_request_destroy(request);

done:
    return hg_ret;
}

/*---------------------------------------------------------------------------*/
static hg_return_t
hg_test_rpc_mask(hg_context_t *context, hg_request_class_t *request_class,
    hg_addr_t addr, hg_id_t rpc_id, hg_cb_t callback)
{
    hg_request_t *request = NULL;
    hg_handle_t handle;
    hg_return_t hg_ret = HG_SUCCESS;
    struct forward_cb_args forward_cb_args;
    hg_const_string_t rpc_open_path = MERCURY_TESTING_TEMP_DIRECTORY "/test.h5";
    rpc_handle_t rpc_open_handle;
    rpc_open_in_t  rpc_open_in_struct;

    request = hg_request_create(request_class);

    /* Create request with invalid RPC id */
    hg_ret = HG_Create(context, addr, rpc_id, &handle);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not create handle");
        goto done;
    }

    HG_Set_target_id(handle, 0);

    /* Fill input structure */
    rpc_open_handle.cookie = 100;
    rpc_open_in_struct.path = rpc_open_path;
    rpc_open_in_struct.handle = rpc_open_handle;

    /* Forward call to remote addr and get a new request */
    HG_TEST_LOG_DEBUG("Forwarding rpc_open, op id: %u...", rpc_id);
    forward_cb_args.request = request;
    forward_cb_args.rpc_handle = &rpc_open_handle;
    hg_ret = HG_Forward(handle, callback, &forward_cb_args,
        &rpc_open_in_struct);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not forward call");
        goto done;
    }

    hg_request_wait(request, HG_MAX_IDLE_TIME, NULL);

    /* Complete */
    hg_ret = HG_Destroy(handle);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not destroy handle");
        goto done;
    }

    hg_request_destroy(request);

done:
    return hg_ret;
}

/*---------------------------------------------------------------------------*/
static hg_return_t
hg_test_rpc_multiple(hg_context_t *context, hg_request_class_t *request_class,
    hg_addr_t addr, hg_uint8_t target_id, hg_id_t rpc_id, hg_cb_t callback)
{
    hg_request_t *request1 = NULL, *request2 = NULL;
    hg_handle_t handle1, handle2;
    struct forward_cb_args forward_cb_args1, forward_cb_args2;
    hg_return_t hg_ret = HG_SUCCESS;
    rpc_open_in_t  rpc_open_in_struct;
    hg_const_string_t rpc_open_path = MERCURY_TESTING_TEMP_DIRECTORY "/test.h5";
    rpc_handle_t rpc_open_handle1, rpc_open_handle2;
    /* Used for multiple in-flight RPCs */
    hg_request_t *request_m[NINFLIGHT];
    hg_handle_t handle_m[NINFLIGHT];
    struct forward_cb_args forward_cb_args_m[NINFLIGHT];
    rpc_handle_t rpc_open_handle_m[NINFLIGHT];
    unsigned int i;

    /* Create request 1 */
    request1 = hg_request_create(request_class);

    hg_ret = HG_Create(context, addr, rpc_id, &handle1);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not create handle");
        goto done;
    }
    hg_ret = HG_Set_target_id(handle1, target_id);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not set target ID to handle");
        goto done;
    }

    /* Fill input structure */
    rpc_open_handle1.cookie = 1;
    rpc_open_in_struct.path = rpc_open_path;
    rpc_open_in_struct.handle = rpc_open_handle1;

    /* Forward call to remote addr and get a new request */
    HG_TEST_LOG_DEBUG("Forwarding rpc_open, op id: %u...", rpc_id);
    forward_cb_args1.request = request1;
    forward_cb_args1.rpc_handle = &rpc_open_handle1;
    hg_ret = HG_Forward(handle1, callback, &forward_cb_args1,
        &rpc_open_in_struct);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not create handle");
        goto done;
    }

    /* Create request 2 */
    request2 = hg_request_create(request_class);

    hg_ret = HG_Create(context, addr, rpc_id, &handle2);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not create handle");
        goto done;
    }
    hg_ret = HG_Set_target_id(handle2, target_id);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not set target ID to handle");
        goto done;
    }

    /* Fill input structure */
    rpc_open_handle2.cookie = 2;
    rpc_open_in_struct.path = rpc_open_path;
    rpc_open_in_struct.handle = rpc_open_handle2;

    /* Forward call to remote addr and get a new request */
    HG_TEST_LOG_DEBUG("Forwarding rpc_open, op id: %u...", rpc_id);
    forward_cb_args2.request = request2;
    forward_cb_args2.rpc_handle = &rpc_open_handle2;
    hg_ret = HG_Forward(handle2, callback, &forward_cb_args2,
        &rpc_open_in_struct);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not forward call");
        goto done;
    }

    hg_request_wait(request2, HG_MAX_IDLE_TIME, NULL);
    hg_request_wait(request1, HG_MAX_IDLE_TIME, NULL);

    /* Complete */
    hg_ret = HG_Destroy(handle1);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not destroy handle");
        goto done;
    }
    hg_ret = HG_Destroy(handle2);
    if (hg_ret != HG_SUCCESS) {
        HG_TEST_LOG_ERROR("Could not destroy handle");
        goto done;
    }

    hg_request_destroy(request1);
    hg_request_destroy(request2);

    /**
     * Forwarding multiple requests
     */
    HG_TEST_LOG_DEBUG("Creating %u requests...", NINFLIGHT);
    for (i = 0; i < NINFLIGHT; i++) {
	    request_m[i] = hg_request_create(request_class);
	    hg_ret = HG_Create(context, addr, rpc_id, handle_m + i );
	    if (hg_ret != HG_SUCCESS) {
	        HG_TEST_LOG_ERROR("Could not create handle");
		    goto done;
	    }
	    hg_ret = HG_Set_target_id(handle_m[i], target_id);
	    if (hg_ret != HG_SUCCESS) {
	        HG_TEST_LOG_ERROR("Could not set target ID to handle");
	        goto done;
	    }
	    rpc_open_handle_m[i].cookie = i;
	    rpc_open_in_struct.path = rpc_open_path;
	    rpc_open_in_struct.handle = rpc_open_handle_m[i];
	    HG_TEST_LOG_DEBUG(" %d Forwarding rpc_open, op id: %u...", i, rpc_id);
	    forward_cb_args_m[i].request = request_m[i];
	    forward_cb_args_m[i].rpc_handle = &rpc_open_handle_m[i];
	    hg_ret = HG_Forward(handle_m[i], callback, &forward_cb_args_m[i],
	        &rpc_open_in_struct);
	    if (hg_ret != HG_SUCCESS) {
	        HG_TEST_LOG_ERROR("Could not forward call");
		    goto done;
	    }
    }

    /* Complete */
    for (i = 0; i < NINFLIGHT; i++) {
	    hg_request_wait(request_m[i], HG_MAX_IDLE_TIME, NULL);

	    hg_ret = HG_Destroy(handle_m[i]);
	    if (hg_ret != HG_SUCCESS) {
	        HG_TEST_LOG_ERROR("Could not destroy handle");
		    goto done;
	    }
	    hg_request_destroy(request_m[i]);
    }
    HG_TEST_LOG_DEBUG("Done");

done:
    return hg_ret;
}

/*---------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
    struct hg_test_info hg_test_info = { 0 };
    hg_return_t hg_ret;
    hg_id_t inv_id;
    int ret = EXIT_SUCCESS;
    int i;

    /* Initialize the interface */
    HG_Test_init(argc, argv, &hg_test_info);

    /* Simple RPC test */
    HG_TEST("simple RPC");
    for(i=100; i<10000; i+=100)
    {
        printf("Sending RPC that includes streng of length %d in input struct.\n", i);
        hg_ret = hg_test_rpc(hg_test_info.context, hg_test_info.request_class,
            hg_test_info.target_addr, hg_test_rpc_open_id_g,
            hg_test_rpc_forward_cb, i);
        if (hg_ret != HG_SUCCESS) {
            ret = EXIT_FAILURE;
            goto done;
        }
        HG_PASSED();
    }

done:
    if (ret != EXIT_SUCCESS)
        HG_FAILED();
    HG_Test_finalize(&hg_test_info);
    return ret;
}
