/*
  This program tests netcdf-4 parallel I/O using compression filters.

  Ed Hartnett, 12/10/2020
*/

#include "config.h"
#include <netcdf.h>
#include <netcdf_par.h>
#include <ccr_test.h>
#include <mpi.h>
#include <stdio.h>

#define FILE "tst_par_zlib.nc"
#define NDIMS 3
#define DIMSIZE 24
#define QTR_DATA (DIMSIZE * DIMSIZE / 4)
#define NUM_PROC 4
#define NUM_SLABS 10

int
main(int argc, char **argv)
{
    /* MPI stuff. */
    int mpi_size, mpi_rank;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;

    /* Netcdf-4 stuff. */
    int ncid, v1id, dimids[NDIMS];
    size_t start[NDIMS], count[NDIMS];

    int slab_data[DIMSIZE * DIMSIZE / 4]; /* one slab */
    char file_name[NC_MAX_NAME + 1];
    int i, res;

    /* Initialize MPI. */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if (mpi_rank == 0)
    {
       printf("\n*** Testing parallel writes with zlib.\n");
       printf("*** testing simple write with zlib...");
    }

    /* Create phony data. We're going to write a 24x24 array of ints,
       in 4 sets of 144. */
    for (i = 0; i < DIMSIZE * DIMSIZE / 4; i++)
       slab_data[i] = mpi_rank;

    /* Create a parallel netcdf-4 file. */
    /*nc_set_log_level(3);*/
    /* sprintf(file_name, "%s/%s", TEMP_LARGE, FILE); */
    sprintf(file_name, "%s", FILE);
    if ((res = nc_create_par(file_name, NC_NETCDF4, comm, info, &ncid))) ERR;

    /* Create three dimensions. */
    if (nc_def_dim(ncid, "d1", DIMSIZE, dimids)) ERR;
    if (nc_def_dim(ncid, "d2", DIMSIZE, &dimids[1])) ERR;
    if (nc_def_dim(ncid, "d3", NUM_SLABS, &dimids[2])) ERR;

    /* Create one var. */
    if ((res = nc_def_var(ncid, "v1", NC_INT, NDIMS, dimids, &v1id))) ERR;

    /* Setting deflate only will work for HDF5-1.10.2 and later
     * versions. */
    res = nc_def_var_deflate(ncid, 0, 0, 1, 1);

    /* Write metadata to file. */
    if ((res = nc_enddef(ncid))) ERR;

    /* Set up slab for this process. */
    start[0] = mpi_rank * DIMSIZE/mpi_size;
    start[1] = 0;
    count[0] = DIMSIZE/mpi_size;
    count[1] = DIMSIZE;
    count[2] = 1;
    /*printf("mpi_rank=%d start[0]=%d start[1]=%d count[0]=%d count[1]=%d\n",
      mpi_rank, start[0], start[1], count[0], count[1]);*/

    /* Not necessary, but harmless. */
    if (nc_var_par_access(ncid, v1id, NC_COLLECTIVE)) ERR;

    for (start[2] = 0; start[2] < NUM_SLABS; start[2]++)
    {
       /* nc_set_log_level(3); */
       /* Write slabs of phoney data. */
       if (nc_put_vara_int(ncid, v1id, start, count, slab_data)) ERR;
    }

    /* Close the netcdf file. */
    if ((res = nc_close(ncid)))	ERR;

    /* Shut down MPI. */
    MPI_Finalize();

    if (mpi_rank == 0)
    {
       SUMMARIZE_ERR;
       FINAL_RESULTS;
    }
    return 0;
}
