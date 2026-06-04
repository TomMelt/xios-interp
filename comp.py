#!/usr/bin/env python
import os
import shutil
import subprocess
import click


@click.command()
@click.option(
    "--mpi",
    "-m",
    type=click.Choice(["ON", "OFF"]),
    default="OFF",
    show_default=True,
    help="Enable or disable MPI.",
)
@click.option(
    "--xios",
    "-x",
    type=click.Choice(["ON", "OFF"]),
    default="OFF",
    show_default=True,
    help="Enable or disable XIOS.",
)
@click.option(
    "--threads",
    "-t",
    type=click.Choice(["ON", "OFF"]),
    default="ON",
    show_default=True,
    help="Enable or disable threads.",
)
@click.option(
    "--caliper",
    type=click.Choice(["ON", "OFF"]),
    default="OFF",
    show_default=True,
    help="Enable or disable caliper profiling.",
)
@click.option(
    "--scorep",
    "-s",
    type=click.Choice(["ON", "OFF"]),
    default="OFF",
    show_default=True,
    help="Enable or disable SCOREP.",
)
@click.option(
    "--ccache",
    type=click.Choice(["ON", "OFF"]),
    default="ON",
    show_default=True,
    help="Use ccache for compilation.",
)
@click.option(
    "--build",
    "-b",
    type=click.Choice(["Debug", "Release", "RelWithDebInfo"]),
    default="Release",
    show_default=True,
    help="Build mode Debug or Release.",
)
@click.option(
    "--make-target",
    type=str,
    default="",
    show_default=True,
    help="Make target e.g., nextsim or testModelArrayRef.",
)
@click.option(
    "-j",
    "--jobs",
    type=int,
    default=8,
    show_default=True,
    help="Number of jobs to build with. Default is 8.",
)
def compile(mpi, xios, threads, caliper, scorep, ccache, build, make_target, jobs):
    """Compile the project with the specified options."""

    xios_dir = "/home/melt/sync/cambridge/projects/current/sasip/xios"
    dir_name = "build"
    cmake_command = "cmake .. -DPython_EXECUTABLE=$(which python) -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_PREFIX_PATH=/software/spack/var/spack/environments/nextsim/.spack-env/view"

    if ccache == "ON":
        cmake_command += " -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"

    if mpi == "ON":
        dir_name += "-mpi"
        cmake_command += " -DCMAKE_C_COMPILER=$(which mpicc) -DCMAKE_CXX_COMPILER=$(which mpic++) -DENABLE_MPI=ON"
    else:
        cmake_command += (
            " -DCMAKE_C_COMPILER=$(which gcc) -DCMAKE_CXX_COMPILER=$(which g++)"
        )

    if xios == "ON":
        cmake_command += f" -DENABLE_XIOS=ON -Dxios_DIR={xios_dir}"
        dir_name += "-xios"

    if caliper == "ON":
        cmake_command += f" -DWITH_CALIPER=ON"
        dir_name += "-caliper"

    if build == "Debug":
        dir_name += "-dbg"
    elif build == "RelWithDebInfo":
        dir_name += "-rdb"

    if scorep == "ON":
        dir_name += "-scorep"
        os.environ["SCOREP_WRAPPER_INSTRUMENTER_FLAGS"] = "--thread=omp"
        cmake_command += (
            " -DCMAKE_C_COMPILER=/software/scorep-8.4/_build/bin/scorep-gcc"
            " -DCMAKE_CXX_COMPILER=/software/scorep-8.4/_build/bin/scorep-g++"
        )

    # Remove existing directory if it exists
    if os.path.exists(dir_name):
        shutil.rmtree(dir_name)

    # Create and navigate to the build directory
    os.makedirs(dir_name)
    os.chdir(dir_name)

    # Run cmake command
    cmake_command += f" -DCMAKE_BUILD_TYPE={build} -DWITH_THREADS={threads}"
    print("running command ::\n", cmake_command)
    subprocess.run(cmake_command, shell=True, check=True)

    # Run make command
    make_command = f"make -j {jobs} {make_target}"
    subprocess.run(make_command, shell=True, check=True)


if __name__ == "__main__":
    compile()
