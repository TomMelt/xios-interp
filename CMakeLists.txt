cmake_minimum_required(VERSION 3.12)
project(xios_read_test LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- MPI ----
find_package(MPI REQUIRED COMPONENTS C CXX)

# ---- XIOS ----
# Pass -Dxios_DIR=/path/to/xios to cmake (same variable as nextsimdg uses)
# e.g. -Dxios_DIR=/home/melt/sync/cambridge/projects/current/sasip/xios
find_path(XIOS_INCLUDES NAMES xios.hpp HINTS ${xios_DIR}/inc)
find_library(XIOS_LIBRARY NAMES xios  HINTS ${xios_DIR}/lib)
set(XIOS_EXTERNS "${xios_DIR}/extern")

if(NOT XIOS_INCLUDES OR NOT XIOS_LIBRARY)
    message(FATAL_ERROR "XIOS not found. Pass -Dxios_DIR=/path/to/xios to cmake.")
endif()

# ---- HDF5 ----
find_package(HDF5 REQUIRED COMPONENTS C HL)

# ---- NetCDF (transitive dep of XIOS) ----
find_package(PkgConfig)
pkg_search_module(NETCDF netcdf)
if(NOT NETCDF_FOUND)
    message(FATAL_ERROR "netcdf not found")
endif()

find_program(NCGEN_EXECUTABLE NAMES ncgen)
if(NOT NCGEN_EXECUTABLE)
    message(FATAL_ERROR "ncgen not found")
endif()

# ---- Fortran runtime (needed by XIOS) ----
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(FORTRAN_RUNTIME_LIBS "-lgfortran -lmpi_mpifh")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(FORTRAN_RUNTIME_LIBS "-lifcore")
else()
    set(FORTRAN_RUNTIME_LIBS "")
endif()

# ---- Executable ----
add_executable(xios_read main.cpp)

target_include_directories(xios_read PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # xios_c_interface.hpp lives here
    ${XIOS_INCLUDES}
    ${XIOS_EXTERNS}/blitz
    ${XIOS_EXTERNS}/boost_extraction/include
    ${XIOS_EXTERNS}/rapidxml/include
    ${HDF5_INCLUDE_DIRS}
    ${NETCDF_INCLUDE_DIRS}
)

target_link_directories(xios_read PRIVATE
    ${HDF5_LIBRARY_DIRS}
    ${NETCDF_LIBRARY_DIRS}
)

target_link_libraries(xios_read PRIVATE
    ${XIOS_LIBRARY}
    ${HDF5_LIBRARIES}
    ${NETCDF_LIBRARIES}
    MPI::MPI_CXX
    MPI::MPI_C
    ${FORTRAN_RUNTIME_LIBS}
)

# ---- Generate NetCDF file from CDL ----
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xios_test_era5_forcing.nc
    COMMAND ${NCGEN_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/xios_test_era5_forcing.nc
        ${CMAKE_CURRENT_SOURCE_DIR}/xios_test_era5_forcing.cdl
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xios_test_era5_forcing.cdl
    COMMENT "Generating xios_test_era5_forcing.nc from CDL"
)

# Ensure the nc file is generated before the executable is considered "ready"
add_custom_target(generate_nc_files DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/xios_test_era5_forcing.nc)
add_dependencies(xios_read generate_nc_files)

# ---- Copy data files into build dir ----
add_custom_command(TARGET xios_read POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/iodef.xml
        $<TARGET_FILE_DIR:xios_read>
    COMMENT "Copying iodef.xml to build dir"
)
