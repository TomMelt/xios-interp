# test-xios-read

A simple project to test reading data using the XIOS C++ interface.

## Building the Project

To build the project, use the provided `comp.sh` script:

```bash
./comp.sh
```

### Important: XIOS Paths

The `comp.sh` script contains a hardcoded path to the XIOS installation. You will likely need to modify this path to match your own environment:

1. Open `comp.sh`.
2. Locate the line `-Dxios_DIR="/home/melt/sync/cambridge/projects/current/sasip/xios"`.
3. Update the path to point to your XIOS installation directory.

Alternatively, you can update the environment variables in `comp-env.sh` if applicable.

## Running the Code

After building, change into the build directory and run:

```bash
cd build
mpirun -n 1 ./xios_read
```

To run with more MPI processes:

```bash
mpirun -n 4 ./xios_read
```

### Input Files

The executable reads data from the following files, which are generated and copied into the build directory automatically as part of the CMake build step:

- `xios_test_era5_forcing.nc` — generated from `xios_test_era5_forcing.cdl` using `ncgen`
- `iodef.xml` — copied from the source directory
