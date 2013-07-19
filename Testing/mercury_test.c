/*
 * Copyright (C) 2013 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#include "mercury_test.h"
#ifdef NA_HAS_BMI
#include "na_bmi.h"
#endif
#ifdef NA_HAS_MPI
#include "na_mpi.h"
#endif
#ifdef NA_HAS_SSM
#include "na_ssm.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------
 *
 * Function:    HG_Test_client_init
 *
 *---------------------------------------------------------------------------
 */
na_class_t *HG_Test_client_init(int argc, char *argv[], char **port_name, int *rank)
{
    na_class_t *network_class = NULL;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bmi|mpi|ssm>\n", argv[0]);
        exit(0);
    }

#ifdef NA_HAS_MPI
    if (strcmp("mpi", argv[1]) == 0) {
        static char mpi_port_name[MPI_MAX_PORT_NAME];
        FILE *config;

        if (argc > 2 && (strcmp("static", argv[2]) == 0)) {
            network_class = NA_MPI_Init(NULL, MPI_INIT_STATIC);
        } else {
            network_class = NA_MPI_Init(NULL, 0);
            if ((config = fopen("port.cfg", "r")) != NULL) {
                size_t nread;

                nread = fread(mpi_port_name, sizeof(char), MPI_MAX_PORT_NAME, config);
                if (!nread) fprintf(stderr, "Could not read port name\n");
                fclose(config);
            }
        }

        if (rank) MPI_Comm_rank(MPI_COMM_WORLD, rank);
        if (port_name) *port_name = mpi_port_name;

    }
#endif

#ifdef NA_HAS_BMI
    if (strcmp("bmi", argv[1]) == 0) {
        network_class = NA_BMI_Init(NULL, NULL, 0);
        if (rank) *rank = 0;
        if (port_name) *port_name = getenv(HG_PORT_NAME);
    }
#endif


#ifdef NA_HAS_SSM
    if (strcmp("ssm", argv[1]) == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s ssm <tcp|udp|ib> port\n", argv[0]);
            exit(0);
        }
        network_class = NA_SSM_Init(argv[2], atoi(argv[3]), 0);
    }
#endif

    return network_class;
}

/*---------------------------------------------------------------------------
 *
 * Function:    HG_Test_server_init
 *
 *---------------------------------------------------------------------------
 */
na_class_t *HG_Test_server_init(int argc, char *argv[], unsigned int *max_number_of_peers)
{
    na_class_t *network_class = NULL;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bmi|mpi|ssm>\n", argv[0]);
        exit(0);
    }

#ifdef NA_HAS_MPI
    if (strcmp("mpi", argv[1]) == 0) {
        if (argc > 2 && (strcmp("static", argv[2]) == 0)) {
            network_class = NA_MPI_Init(NULL, MPI_INIT_SERVER_STATIC);
        } else {
            network_class = NA_MPI_Init(NULL, MPI_INIT_SERVER);
        }
    }
#endif


#ifdef NA_HAS_BMI
    if (strcmp("bmi", argv[1]) == 0) {
        char *listen_addr = getenv(HG_PORT_NAME);
        if (!listen_addr) {
            fprintf(stderr, "getenv(\"%s\") failed.\n", HG_PORT_NAME);
            return NULL;
        }
        network_class = NA_BMI_Init("bmi_tcp", listen_addr, BMI_INIT_SERVER);
    }
#endif

#ifdef NA_HAS_SSM
    if (strcmp("ssm", argv[1]) == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s ssm <tcp|udp|ib> port\n", argv[0]);
            exit(0);
        }
        network_class = NA_SSM_Init(argv[2], atoi(argv[3]), 0);
    }
#endif

#ifdef NA_HAS_MPI
    *max_number_of_peers = MPIEXEC_MAX_NUMPROCS;
#else
    *max_number_of_peers = 1;
#endif
    return network_class;
}
