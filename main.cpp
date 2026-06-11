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
static const int X_DIM_INTERP = 10;
static const int Y_DIM_INTERP = 6;
static const int N_TIMESTEPS = 3;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* ---- Initialize XIOS client and context ---- */

    MPI_Fint nullComm_F = MPI_Comm_c2f(MPI_COMM_NULL);
    MPI_Fint clientComm_F;

    std::string clientId = "client";
    cxios_init_client(clientId.c_str(), clientId.size(), &nullComm_F, &clientComm_F);

    std::string contextId = "nextSIM-DG";
    cxios_context_initialize(contextId.c_str(), contextId.size(), &clientComm_F);

    /* ---- Set up domain local geometry (1 rank = full domain) ---- */
    // The domain "era5" is already defined in iodef.xml with ni_glo=5, nj_glo=3.
    // We only need to set the local partitioning and coordinate values.

    std::string domainId = "era5";
    xios::CDomain* domain;
    cxios_domain_handle_create(&domain, domainId.c_str(), domainId.size());

    std::string domainType = "rectilinear";
    cxios_set_domain_type(domain, domainType.c_str(), domainType.size());

    int ni = X_DIM;
    int nj = Y_DIM;
    int ibegin = 0;
    int jbegin = 0;
    cxios_set_domain_ni(domain, ni);
    cxios_set_domain_nj(domain, nj);
    cxios_set_domain_ni_glo(domain, ni);
    cxios_set_domain_nj_glo(domain, nj);
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

    /* ---- Set up interpolation domain geometry ---- */
    std::string domainInterpId = "era5Interp";
    xios::CDomain* domainInterp;
    cxios_domain_handle_create(&domainInterp, domainInterpId.c_str(), domainInterpId.size());

    std::string domainInterpType = "rectilinear";
    cxios_set_domain_type(domainInterp, domainInterpType.c_str(), domainInterpType.size());

    int niInterp = X_DIM_INTERP;
    int njInterp = Y_DIM_INTERP;
    int ibeginInterp = 0;
    int jbeginInterp = 0;
    cxios_set_domain_ni(domainInterp, niInterp);
    cxios_set_domain_nj(domainInterp, njInterp);
    cxios_set_domain_ni_glo(domainInterp, niInterp);
    cxios_set_domain_nj_glo(domainInterp, njInterp);
    cxios_set_domain_ibegin(domainInterp, ibeginInterp);
    cxios_set_domain_jbegin(domainInterp, jbeginInterp);

    std::vector<double> lonvalueInterp(niInterp);
    for (int i = 0; i < niInterp; i++)
        lonvalueInterp[i] = static_cast<double>(i) * 0.5;
    cxios_set_domain_lonvalue_1d(domainInterp, lonvalueInterp.data(), &niInterp);

    std::vector<double> latvalueInterp(njInterp);
    for (int j = 0; j < njInterp; j++)
        latvalueInterp[j] = static_cast<double>(j) * 0.5;
    cxios_set_domain_latvalue_1d(domainInterp, latvalueInterp.data(), &njInterp);

    /* ---- Set up interpolation order and renormalization ---- */
    xios::CInterpolateDomain* interpDomain;
    std::string interpId = "interpolate";
    cxios_xml_tree_add_interpolatedomaintodomain(
        domainInterp, &interpDomain, interpId.c_str(), interpId.size());
    cxios_set_interpolate_domain_order(interpDomain, 1);
    cxios_set_interpolate_domain_renormalize(interpDomain, false);

    /* ---- Set up calendar ---- */

    xios::CCalendarWrapper* calendar;
    cxios_get_current_calendar_wrapper(&calendar);

    std::string startDate = "2023-03-17 00:00:00";
    cxios_date start = cxios_date_convert_from_string(startDate.c_str(), startDate.size());
    cxios_set_calendar_wrapper_date_start_date(calendar, start);

    cxios_duration timestep = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 }; // 1 hour
    cxios_set_calendar_wrapper_timestep(calendar, timestep);
    cxios_update_calendar_timestep(calendar);


    /* ---- Create an input field and link it to the base field ---- */

    std::string fieldGroupId = "field_definition";
    std::string inputFieldId = "tairEra5";
    std::string netcdfVarName = "tair";
    std::string gridId = "HGridEra5"; // already defined in iodef.xml

    xios::CFieldGroup* fieldGroup; // create field group object for manipulating fields at run-time
    cxios_fieldgroup_handle_create(&fieldGroup, fieldGroupId.c_str(), fieldGroupId.size());

    xios::CField* inputField; // create object that links to field tairEra5
    cxios_xml_tree_add_field(fieldGroup, &inputField, inputFieldId.c_str(), inputFieldId.size());

    // link tairEra5 to HGridEra5
    cxios_set_field_grid_ref(inputField, gridId.c_str(), gridId.size());
    // link tairEra5 to its netcdf name
    cxios_set_field_name(inputField, netcdfVarName.c_str(), netcdfVarName.size());

    // set read access and operation
    cxios_set_field_read_access(inputField, true);
    std::string fieldOperation = "instant";
    cxios_set_field_operation(inputField, fieldOperation.c_str(), fieldOperation.size());

    /* ---- Create an input field for the interpolated field ---- */

    std::string baseFieldInterpId = "tairEra5Interp";
    std::string gridInterpId = "HGridEra5Interp";

    xios::CField* inputFieldInterp;
    cxios_xml_tree_add_field(fieldGroup, &inputFieldInterp, baseFieldInterpId.c_str(), baseFieldInterpId.size());
    cxios_set_field_field_ref(inputFieldInterp, inputFieldId.c_str(), inputFieldId.size());
    cxios_set_field_grid_ref(inputFieldInterp, gridInterpId.c_str(), gridInterpId.size());

    /* ---- Create a file for reading and attach the input field ---- */

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

    /* ---- Close context definition ---- */

    cxios_context_close_definition();

    /* ---- Read tair at each timestep ---- */

    std::vector<double> data(Y_DIM * X_DIM);
    std::vector<double> dataInterp(Y_DIM_INTERP * X_DIM_INTERP);

    for (int ts = 0; ts < N_TIMESTEPS; ts++) {
        cxios_read_data_k82(
            inputFieldId.c_str(), inputFieldId.size(), data.data(), X_DIM, Y_DIM);

        cxios_read_data_k82(
            baseFieldInterpId.c_str(), baseFieldInterpId.size(), dataInterp.data(), X_DIM_INTERP, Y_DIM_INTERP);

        if (rank == 0) {
            printf("\n=== Timestep %d ===\n", ts);
            printf("Original (5x3):\n");
            for (int j = 0; j < Y_DIM; j++) {
                for (int i = 0; i < X_DIM; i++) {
                    printf("%.1f ", data[j * X_DIM + i]);
                }
                printf("\n");
            }
            printf("Interpolated (10x6):\n");
            for (int j = 0; j < Y_DIM_INTERP; j++) {
                for (int i = 0; i < X_DIM_INTERP; i++) {
                    printf("%.1f ", dataInterp[j * X_DIM_INTERP + i]);
                }
                printf("\n");
            }
        }

        /* Advance calendar by 1 hour for next timestep */
        if (ts < N_TIMESTEPS - 1) {
            cxios_update_calendar(ts+1);
        }
    }

    /* ---- Finalize ---- */

    cxios_context_finalize();
    cxios_finalize();

    MPI_Finalize();
    return 0;
}
