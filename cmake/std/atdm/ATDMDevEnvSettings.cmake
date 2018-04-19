###############################################################################
#
#              Standard ATDM Configuration of Trilnos
#
###############################################################################

INCLUDE("${CMAKE_CURRENT_LIST_DIR}/utils/ATDMDevEnvUtils.cmake")

#
# A) Assert the right env vars are set and set local defaults
#

IF (NOT "$ENV{ATDM_CONFIG_COMPLETED_ENV_SETUP}" STREQUAL "TRUE")
  MESSAGE(FATAL_ERROR
    "Error, env var ATDM_CONFIG_COMPLETED_ENV_SETUP not set to true!"
    "  Check load-env.sh output for error loading the env!"
    ) 
ENDIF()

ASSERT_DEFINED(ENV{ATDM_CONFIG_KNOWN_SYSTEM_NAME})
ASSERT_DEFINED(ENV{ATDM_CONFIG_COMPILER})
ASSERT_DEFINED(ENV{ATDM_CONFIG_BUILD_TYPE})
ASSERT_DEFINED(ENV{ATDM_CONFIG_BLAS_LIB})
ASSERT_DEFINED(ENV{ATDM_CONFIG_LAPACK_LIB})
ASSERT_DEFINED(ENV{BOOST_ROOT})
ASSERT_DEFINED(ENV{HDF5_ROOT})
ASSERT_DEFINED(ENV{NETCDF_ROOT})

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_OPENMP OFF)
ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_PTHREADS OFF)
ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_CUDA OFF)

IF (ATDM_USE_OPENMP)
  SET(ATDM_NODE_TYPE OPENMP)
ELSEIF (ATDM_USE_PTHREADS)
  SET(ATDM_NODE_TYPE THREAD)
ELSEIF (ATDM_USE_CUDA)
  SET(ATDM_NODE_TYPE CUDA)
ELSE()
  SET(ATDM_NODE_TYPE SERIAL)
ENDIF()

SET(ATDM_BOUNDS_CHECK OFF)
IF ("$ENV{ATDM_CONFIG_BUILD_TYPE}" STREQUAL "DEBUG")
  SET(ATDM_BOUNDS_CHECK ON)
ENDIF()

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(SHARED_LIBS OFF)

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(USE_HWLOC OFF)
iF (ATDM_USE_HWLOC AND "$ENV{ATDM_CONFIG_HWLOC_LIBS}" STREQUAL "")
  MESSAGE(FATAL_ERROR
    "Error, env var USE_HWLOC=$ATDM_USE_HWLOC but env var"
    " HWLOC_LIBS is not set!")
ENDIF()

SET(ATDM_JOB_NAME_KEYS_STR
  "$ENV{ATDM_CONFIG_COMPILER}-$ENV{ATDM_CONFIG_BUILD_TYPE}-${ATDM_NODE_TYPE}")
PRINT_VAR(ATDM_JOB_NAME_KEYS_STR)

ATDM_SET_ATDM_VAR_FROM_ENV_AND_DEFAULT(MPI_EXEC mpiexec)


#
# B) Look for tweaks file(s) for this build and load the file(s) if it exists
# 

SET(ATDM_TWEAKS_FILE_DEFAULT_DEFAULT
  "${CMAKE_CURRENT_LIST_DIR}/$ENV{ATDM_CONFIG_KNOWN_SYSTEM_NAME}/tweaks/${ATDM_JOB_NAME_KEYS_STR}.cmake")
IF (EXISTS "${ATDM_TWEAKS_FILE_DEFAULT_DEFAULT}")
  SET(ATDM_TWEAKS_FILES_DEFAULT "${ATDM_TWEAKS_FILE_DEFAULT_DEFAULT}")
ELSE()
  SET(ATDM_TWEAKS_FILES_DEFAULT "")
ENDIF()

ADVANCED_SET(ATDM_TWEAKS_FILES "${ATDM_TWEAKS_FILES_DEFAULT}"
  CACHE FILEPATH
  "Extra *.cmake file to include to define tweaks for this ATDM build ${ATDM_JOB_NAME_KEYS_STR}"
  )
PRINT_VAR(ATDM_TWEAKS_FILES)

FOREACH(ATDM_TREAKS_FILE ${ATDM_TWEAKS_FILES})
  MESSAGE("-- " "Including ATDM build treaks file ${ATDM_TREAKS_FILE} ...")
  TRIBITS_TRACE_FILE_PROCESSING(PROJECT  INCLUDE "${ATDM_TREAKS_FILE}")
  INCLUDE("${ATDM_TREAKS_FILE}")
ENDFOREACH()

#
# C) Set the compilers
#

# All ATDM builds of Trilinos are MPI builds!
ATDM_SET_ENABLE(TPL_ENABLE_MPI ON)

ASSERT_DEFINED(ENV{MPICC})
ASSERT_DEFINED(ENV{MPICXX})
ASSERT_DEFINED(ENV{MPIF90})

ATDM_SET_CACHE(CMAKE_C_COMPILER "$ENV{MPICC}" CACHE FILEPATH)
ATDM_SET_CACHE(CMAKE_CXX_COMPILER "$ENV{MPICXX}" CACHE FILEPATH)
ATDM_SET_CACHE(CMAKE_Fortran_COMPILER "$ENV{MPIF90}" CACHE FILEPATH)
ATDM_SET_ENABLE(Trilinos_ENABLE_Fortran OFF)  # ToDo: Remove this once SPARC is using this!

#
# D) Set up basic compiler flags, link flags etc.
#

ATDM_SET_CACHE(BUILD_SHARED_LIBS "${ATDM_SHARED_LIBS}" CACHE BOOL)
ATDM_SET_CACHE(CMAKE_BUILD_TYPE "$ENV{ATDM_CONFIG_BUILD_TYPE}" CACHE STRING)
ATDM_SET_CACHE(CMAKE_C_FLAGS "$ENV{EXTRA_C_FLAGS}" CACHE STRING)
ATDM_SET_CACHE(CMAKE_CXX_FLAGS "$ENV{EXTRA_CXX_FLAGS}" CACHE STRING)

#
# E) Set up other misc options
#

# Currently, EMPIRE configures of Trilinos have this enabled by default.  But
# really we should elevate every subpackage that ATDM uses to Primary Tested.
# That is the right solution.
ATDM_SET_ENABLE(Trilinos_ENABLE_SECONDARY_TESTED_CODE ON)

# Other various options
ATDM_SET_CACHE(CMAKE_SKIP_RULE_DEPENDENCY ON CACHE BOOL)
ATDM_SET_CACHE(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL)
ATDM_SET_CACHE(MPI_EXEC ${ATDM_MPI_EXEC} CACHE FILEPATH)
ATDM_SET_CACHE(MPI_EXEC_PRE_NUMPROCS_FLAGS "$ENV{ATDM_CONFIG_MPI_PRE_FLAGS}" CACHE STRING)
ATDM_SET_CACHE(MPI_EXEC_POST_NUMPROCS_FLAGS "$ENV{ATDM_CONFIG_MPI_POST_FLAG}" CACHE STRING)
ATDM_SET_CACHE(Trilinos_VERBOSE_CONFIGURE OFF CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_DEBUG OFF CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_EXPLICIT_INSTANTIATION ON CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_INSTALL_CMAKE_CONFIG_FILES ON CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_DEVELOPMENT_MODE OFF CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ASSERT_MISSING_PACKAGES ON CACHE BOOL)
ATDM_SET_CACHE(Trilinos_ENABLE_OpenMP "${ATDM_USE_OPENMP}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_OpenMP "${ATDM_USE_OPENMP}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Pthread "${ATDM_USE_PTHREADS}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Cuda_UVM "${ATDM_USE_CUDA}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_CXX11_DISPATCH_LAMBDA ON CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Cuda_Lambda "${ATDM_USE_CUDA}" CACHE BOOL)
ATDM_SET_CACHE(Kokkos_ENABLE_Debug_Bounds_Check "${ATDM_BOUNDS_CHECK}" CACHE BOOL)
ATDM_SET_CACHE(KOKKOS_ENABLE_DEBUG "${ATDM_BOUNDS_CHECK}" CACHE BOOL)
ATDM_SET_CACHE(KOKKOS_ARCH "$ENV{ATDM_CONFIG_KOKKOS_ARCH}" CACHE STRING)
ATDM_SET_CACHE(EpetraExt_ENABLE_HDF5 OFF CACHE BOOL)
ATDM_SET_CACHE(MueLu_ENABLE_Epetra OFF CACHE BOOL)
ATDM_SET_CACHE(Panzer_ENABLE_FADTYPE "Sacado::Fad::DFad<RealType>" CACHE STRING)
ATDM_SET_CACHE(Phalanx_KOKKOS_DEVICE_TYPE "${ATDM_NODE_TYPE}" CACHE STRING)
ATDM_SET_CACHE(Phalanx_SHOW_DEPRECATED_WARNINGS OFF CACHE BOOL)
ATDM_SET_CACHE(Tpetra_INST_CUDA "${ATDM_USE_CUDA}" CACHE BOOL)
ATDM_SET_CACHE(DART_TESTING_TIMEOUT 600 CACHE STRING)

#
# F) TPL locations and enables
#
# Since this is special ATDM configuration of Trilinos, it makes sense to go
# ahead and enable all of the TPLs by default that are used by the ATDM
# applications.
#

# CUDA
ATDM_SET_ENABLE(TPL_ENABLE_CUDA "${ATDM_USE_CUDA}")

# CUSPARSE
ATDM_SET_ENABLE(TPL_ENABLE_CUSPARSE "${ATDM_USE_CUDA}")

# Pthread
ATDM_SET_ENABLE(TPL_ENABLE_Pthread OFF)

# BLAS
ATDM_SET_ENABLE(TPL_ENABLE_BLAS ON)
ATDM_SET_CACHE(TPL_BLAS_LIBRARIES "$ENV{ATDM_CONFIG_BLAS_LIB}" CACHE FILEPATH)

# Boost
ATDM_SET_ENABLE(TPL_ENABLE_Boost ON)
ATDM_SET_CACHE(Boost_INCLUDE_DIRS "$ENV{BOOST_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(TPL_Boost_LIBRARIES
   "$ENV{BOOST_ROOT}/lib/libboost_program_options.so;$ENV{BOOST_ROOT}/lib/libboost_system.so"
  CACHE FILEPATH)

# BoostLib
ATDM_SET_ENABLE(TPL_ENABLE_BoostLib ON)
ATDM_SET_CACHE(BoostLib_INCLUDE_DIRS "$ENV{BOOST_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(TPL_BoostLib_LIBRARIES
  "$ENV{BOOST_ROOT}/lib/libboost_program_options.a;$ENV{BOOST_ROOT}/lib/libboost_system.a"
   CACHE FILEPATH)

# HWLOC
ATDM_SET_ENABLE(TPL_ENABLE_HWLOC ${ATDM_USE_HWLOC})
ATDM_SET_CACHE(TPL_HWLOC_LIBRARIES "$ENV{ATDM_CONFIG_HWLOC_LIBS}" CACHE FILEPATH)

# LAPACK
ATDM_SET_ENABLE(TPL_ENABLE_LAPACK ON)
ATDM_SET_CACHE(TPL_LAPACK_LIBRARIES "$ENV{ATDM_CONFIG_LAPACK_LIB}" CACHE FILEPATH)

# HDF5
ATDM_SET_ENABLE(TPL_ENABLE_HDF5 ON)
ATDM_SET_CACHE(HDF5_INCLUDE_DIRS "$ENV{HDF5_ROOT}/include" CACHE FILEPATH)
IF ("$ENV{ATDM_CONFIG_HDF5_LIBS}" STREQUAL "")
  MESSAGE(FATAL_ERROR "Error: ToDo: Implement default HDF5 libs")
  #SET(HDF5_LIBS
  #  "$ENV{HDF5_ROOT}/lib/libhdf5_hl.a;$ENV{HDF5_ROOT}/lib/libhdf5.a;-lz;-ldl")
ENDIF()
ATDM_SET_CACHE(TPL_HDF5_LIBRARIES "$ENV{ATDM_CONFIG_HDF5_LIBS}" CACHE FILEPATH)

# Netcdf
ATDM_SET_ENABLE(TPL_ENABLE_Netcdf ON)
IF ("$ENV{ATDM_CONFIG_NETCDF_LIBS}" STREQUAL "")
  MESSAGE(FATAL_ERROR "Error: ToDo: Implement default Netcdf libs")
  #SET(NETCDF_LIBS
  #  "$ENV{NETCDF_ROOT}/lib/libnetcdf.a;$ENV{HDF5_ROOT}/lib/libhdf5_hl.a;$ENV{HDF5_ROOT}/lib/libhdf5.a;-lz;-ldl")
ENDIF()
ATDM_SET_CACHE(Netcdf_INCLUDE_DIRS "$ENV{NETCDF_ROOT}/include" CACHE FILEPATH)
ATDM_SET_CACHE(TPL_Netcdf_LIBRARIES "$ENV{ATDM_CONFIG_NETCDF_LIBS}" CACHE FILEPATH)
