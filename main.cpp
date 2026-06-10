/*!
 * @brief Minimal working example: read tair from xios_test_era5_forcing.nc
 *        using only the XIOS C interface (no nextsimdg dependencies).
 *
 * @details This program demonstrates the XIOS client workflow:
 *   1. Initialize XIOS client and context
 *   2. Set up domain geometry (local indices)
 *   3. Configure calendar
 *   4. Create a file for reading and associate the field with it
 *   5. Close context definition
 *   6. Read field data at each timestep
 *
 * Requires: iodef.xml in the same directory
 *           xios_test_era5_forcing.nc in the same directory
 */

#include "xios_c_interface.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

    std::string clientId = "client";
    cxios_init_client(clientId.c_str(), clientId.size(), &nullComm_F, &clientComm_F);

    std::string contextId = "nextSIM-DG";
    cxios_context_initialize(contextId.c_str(), contextId.size(), &clientComm_F);

    /* ---- 2. Set up domain local geometry (1 rank = full domain) ---- */

    std::string domainId = "era5";
    xios::CDomain* domain;
    cxios_domain_handle_create(&domain, domainId.c_str(), domainId.size());

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

    std::string startDate = "2023-03-17 00:00:00";
    cxios_date start = cxios_date_convert_from_string(startDate.c_str(), startDate.size());
    cxios_set_calendar_wrapper_date_start_date(calendar, start);

    cxios_duration timestep = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 }; // 1 hour
    cxios_set_calendar_wrapper_timestep(calendar, timestep);
    cxios_update_calendar_timestep(calendar);

    /* ---- 4. Set up the grid (link domain to grid defined in XML) ---- */

    std::string gridId = "HGridEra5";
    xios::CGrid* grid;
    cxios_grid_handle_create(&grid, gridId.c_str(), gridId.size());
    cxios_xml_tree_add_domaintogrid(grid, &domain, domainId.c_str(), domainId.size());

    /* ---- 5. Create an input field and link it to the base field ---- */

    std::string fieldGroupId = "field_definition";
    std::string inputFieldId = "tairEra5_input";
    std::string baseFieldId = "tairEra5";
    std::string netcdfVarName = "tair";

    xios::CFieldGroup* fieldGroup;
    xios::CField* inputField;
    cxios_fieldgroup_handle_create(&fieldGroup, fieldGroupId.c_str(), fieldGroupId.size());
    cxios_xml_tree_add_field(fieldGroup, &inputField, inputFieldId.c_str(), inputFieldId.size());

    // Link input field to the base field and set the NetCDF variable name
    cxios_set_field_field_ref(inputField, baseFieldId.c_str(), baseFieldId.size());
    cxios_set_field_grid_ref(inputField, gridId.c_str(), gridId.size());
    cxios_set_field_name(inputField, netcdfVarName.c_str(), netcdfVarName.size());
    cxios_set_field_read_access(inputField, true);

    std::string fieldOperation = "instant";
    cxios_set_field_operation(inputField, fieldOperation.c_str(), fieldOperation.size());

    /* ---- 6. Create a file for reading and attach the input field ---- */

    std::string fileGroupId = "file_definition";
    std::string fileId = "era5_forcing";
    std::string fileName = "xios_test_era5_forcing";
    std::string fileType = "one_file";
    std::string fileMode = "read";
    std::string fileParAccess = "collective";

    xios::CFileGroup* fileGroup;
    xios::CFile* file;
    cxios_filegroup_handle_create(&fileGroup, fileGroupId.c_str(), fileGroupId.size());
    cxios_xml_tree_add_file(fileGroup, &file, fileId.c_str(), fileId.size());

    cxios_set_file_name(file, fileName.c_str(), fileName.size());
    cxios_set_file_type(file, fileType.c_str(), fileType.size());
    cxios_set_file_mode(file, fileMode.c_str(), fileMode.size());
    cxios_set_file_par_access(file, fileParAccess.c_str(), fileParAccess.size());

    cxios_duration outputFreq = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 }; // 1 hour
    cxios_set_file_output_freq(file, outputFreq);

    // Attach the input field to the file
    cxios_xml_tree_add_fieldtofile(file, &inputField, inputFieldId.c_str(), inputFieldId.size());

    /* ---- 7. Close context definition ---- */

    cxios_context_close_definition();

    /* ---- 8. Read tair at each timestep ---- */

    std::vector<double> data(Y_DIM * X_DIM);

    for (int ts = 0; ts < N_TIMESTEPS; ts++) {
        cxios_read_data_k82(
            inputFieldId.c_str(), inputFieldId.size(), data.data(), X_DIM, Y_DIM);

        if (rank == 0) {
            printf("\n=== Timestep %d ===\n", ts);
            for (int j = 0; j < Y_DIM; j++) {
                for (int i = 0; i < X_DIM; i++) {
                    printf("%.1f ", data[j * X_DIM + i]);
                }
                printf("\n");
            }
        }

        /* Advance calendar by 1 hour for next timestep */
        if (ts < N_TIMESTEPS - 1) {
            cxios_update_calendar(1);
        }
    }

    /* ---- 9. Finalize ---- */

    cxios_context_finalize();
    cxios_finalize();

    MPI_Finalize();
    return 0;
}
