/*!
 * @brief Minimal working example: read tair from xios_test_era5_forcing.nc
 *        using only the XIOS C interface (no nextsimdg dependencies).
 *
 * @details This program demonstrates the XIOS client workflow:
 *   1. Initialize XIOS client and context
 *   2. Set up domain geometry (local indices)
 *   3. Configure calendar
 *   4. Close context definition
 *   5. Read field data at each timestep
 *
 * Requires: iodef.xml in the same directory
 *           xios_test_era5_forcing.nc in the same directory
 */

#include "xios_c_interface.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mpi.h>
#include <string>
#include <vector>

static const int X_DIM = 5;
static const int Y_DIM = 3;
static const int N_TIMESTEPS = 3;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* ---- 1. Initialize XIOS client and context ---- */

    MPI_Fint nullComm_F = MPI_Comm_c2f(MPI_COMM_NULL);
    MPI_Fint clientComm_F;
    cxios_init_client("client", 6, &nullComm_F, &clientComm_F);

    cxios_context_initialize("nextSIM-DG", 10, &clientComm_F);

    /* ---- 2. Set up domain local geometry (1 rank = full domain) ---- */

    xios::CDomain* domain;
    cxios_domain_handle_create(&domain, "era5", 4);

    int ni = X_DIM;
    int nj = Y_DIM;
    int ibegin = 0;
    int jbegin = 0;
    cxios_set_domain_ni(domain, ni);
    cxios_set_domain_nj(domain, nj);
    cxios_set_domain_ibegin(domain, ibegin);
    cxios_set_domain_jbegin(domain, jbegin);

    std::vector<double> lonvalue(ni);
    for (int i = 0; i < ni; i++)
        lonvalue[i] = static_cast<double>(i);
    cxios_set_domain_lonvalue_1d(domain, lonvalue.data(), &ni);

    std::vector<double> latvalue(nj);
    for (int j = 0; j < nj; j++)
        latvalue[j] = static_cast<double>(j);
    cxios_set_domain_latvalue_1d(domain, latvalue.data(), &nj);

    /* ---- 3. Set up calendar ---- */

    xios::CCalendarWrapper* calendar;
    cxios_get_current_calendar_wrapper(&calendar);

    cxios_date start = cxios_date_convert_from_string("2023-03-17 00:00:00", 19);
    cxios_set_calendar_wrapper_date_start_date(calendar, start);

    cxios_duration timestep = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 }; // 1 hour
    cxios_set_calendar_wrapper_timestep(calendar, timestep);
    cxios_update_calendar_timestep(calendar);

    /* ---- 4. Close context definition ---- */

    cxios_context_close_definition();

    /* ---- 5. Read tair at each timestep ---- */

    double data[Y_DIM][X_DIM];

    for (int ts = 0; ts < N_TIMESTEPS; ts++) {
        cxios_read_data_k82("tair", 8, data[0], X_DIM, Y_DIM);

        if (rank == 0) {
            printf("\n=== Timestep %d ===\n", ts);
            for (int j = 0; j < Y_DIM; j++) {
                for (int i = 0; i < X_DIM; i++) {
                    printf("%.1f ", data[j][i]);
                }
                printf("\n");
            }
        }

        /* Advance calendar by 1 hour for next timestep */
        if (ts < N_TIMESTEPS - 1) {
            cxios_update_calendar(1);
        }
    }

    /* ---- 6. Finalize ---- */

    cxios_context_finalize();
    cxios_finalize();

    MPI_Finalize();
    return 0;
}
