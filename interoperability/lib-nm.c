/*
 * Copyright (c) 2020 UChicago Argonne, LLC
 *
 * See COPYRIGHT in top-level directory.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <mercury.h>
#include <mercury_macros.h>

#include "lib-nm.h"

#define NM_ID 1

static int shutdown_flag = 0;

struct progress_fn_args
{
    hg_context_t *context;
};

void* progress_fn(void* _arg)
{
    struct progress_fn_args *pargs = _arg;
    int ret;
    unsigned int actual_count;

    while(!shutdown_flag)
    {
        do{
            ret = HG_Trigger(pargs->context, 0, 1, &actual_count);
        }while((ret == HG_SUCCESS) && actual_count);

        if(!shutdown_flag)
            ret = HG_Progress(pargs->context, 100);

        if(ret != HG_SUCCESS && ret != HG_TIMEOUT)
        {
            fprintf(stderr, "Error: unexpected HG_Progress() error code %d\n", ret);
            assert(0);
        }
    }

    return(NULL);
}

static hg_return_t nm_noop_rpc_cb(hg_handle_t handle)
{
    hg_return_t ret = HG_SUCCESS;

    /* Send response back */
    ret = HG_Respond(handle, NULL, NULL, NULL);
    assert(ret == HG_SUCCESS);

    ret = HG_Destroy(handle);
    assert(ret == HG_SUCCESS);

    return ret;
}

void* nm_run_client(void* _arg)
{
    struct nm_client_args *nm_args = _arg;
    struct progress_fn_args pargs;
    pthread_t tid;
    int ret;
    hg_id_t nm_noop_id;

    /* create separate context for this component */
    pargs.context = HG_Context_create_id(nm_args->class, NM_ID);
    assert(pargs.context);

    nm_noop_id = MERCURY_REGISTER(nm_args->class, "nm_noop",
            void, void, NULL);

    /* create thread to drive progress */
    ret = pthread_create(&tid, NULL, progress_fn, &pargs);
    assert(ret == 0);

    sleep(1);

    shutdown_flag = 1;
    pthread_join(tid, NULL);

    HG_Context_destroy(pargs.context);

    return(NULL);
}

void* nm_run_server(void* _arg)
{
    struct nm_server_args *nm_args = _arg;
    struct progress_fn_args pargs;
    pthread_t tid;
    int ret;
    hg_id_t nm_noop_id;

    /* create separate context for this component */
    pargs.context = HG_Context_create_id(nm_args->class, NM_ID);
    assert(pargs.context);

    nm_noop_id = MERCURY_REGISTER(nm_args->class, "nm_noop",
            void, void, nm_noop_rpc_cb);

    /* create thread to drive progress */
    ret = pthread_create(&tid, NULL, progress_fn, &pargs);
    assert(ret == 0);

    sleep(1);

    shutdown_flag = 1;
    pthread_join(tid, NULL);

    HG_Context_destroy(pargs.context);

    return(NULL);
}
