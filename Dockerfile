# Multi-stage Dockerfile for building Trilinos and Xyce
FROM debian:latest AS base

# Install all prerequisites
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gfortran \
    libopenmpi-dev \
    openmpi-bin \
    bison \
    flex \
    libblas-dev \
    liblapack-dev \
    libfftw3-dev \
    git \
    wget \
    pkg-config \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Build Trilinos stage
FROM base AS trilinos-build

# Set environment variables for MPI
ENV CC=mpicc
ENV CXX=mpicxx
ENV FC=mpifort

# Clone and build SuiteSparse (required for Trilinos/AMD)
RUN git clone --depth 1 --branch v7.8.3 https://github.com/DrTimothyAldenDavis/SuiteSparse.git suitesparse-src && \
    mkdir suitesparse-build && cd suitesparse-build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/suitesparse \
        -DSUITESPARSE_ENABLE_PROJECTS="suitesparse_config;amd" \
        ../suitesparse-src && \
    cmake --build . -j 8 && \
    cmake --install . && \
    cd .. && rm -rf suitesparse-src suitesparse-build

# Clone Trilinos 14.4
RUN git clone --depth 1 --branch trilinos-release-14-4-branch https://github.com/trilinos/Trilinos.git trilinos-src

# Copy Xyce CMake cache file (we'll mount the Xyce repo later)
# For now, we'll create a minimal version inline
WORKDIR /build/trilinos-build

# Configure Trilinos with MPI
RUN cmake \
    -DCMAKE_INSTALL_PREFIX=/opt/trilinos \
    -DCMAKE_C_COMPILER=mpicc \
    -DCMAKE_CXX_COMPILER=mpicxx \
    -DCMAKE_Fortran_COMPILER=mpifort \
    -DTrilinos_ENABLE_NOX=ON \
    -DNOX_ENABLE_LOCA=ON \
    -DTrilinos_ENABLE_EpetraExt=ON \
    -DEpetraExt_BUILD_BTF=ON \
    -DEpetraExt_BUILD_EXPERIMENTAL=ON \
    -DEpetraExt_BUILD_GRAPH_REORDERINGS=ON \
    -DTrilinos_ENABLE_TrilinosCouplings=ON \
    -DTrilinos_ENABLE_Ifpack=ON \
    -DTrilinos_ENABLE_AztecOO=ON \
    -DTrilinos_ENABLE_Belos=ON \
    -DTrilinos_ENABLE_Teuchos=ON \
    -DTrilinos_ENABLE_Amesos=ON \
    -DAmesos_ENABLE_KLU=ON \
    -DTrilinos_ENABLE_Sacado=ON \
    -DTrilinos_ENABLE_Stokhos=ON \
    -DTrilinos_ENABLE_ROL=ON \
    -DTrilinos_ENABLE_Amesos2=ON \
    -DAmesos2_ENABLE_KLU2=ON \
    -DAmesos2_ENABLE_Basker=ON \
    -DTrilinos_ENABLE_COMPLEX_DOUBLE=ON \
    -DTrilinos_ENABLE_ALL_OPTIONAL_PACKAGES=OFF \
    -DTPL_ENABLE_AMD=ON \
    -DTPL_AMD_INCLUDE_DIRS=/opt/suitesparse/include/suitesparse \
    -DTPL_AMD_LIBRARIES=/opt/suitesparse/lib/libamd.so \
    -DTPL_ENABLE_BLAS=ON \
    -DTPL_ENABLE_LAPACK=ON \
    -DTPL_ENABLE_MPI=ON \
    -DTrilinos_ENABLE_Zoltan=ON \
    -DTrilinos_ENABLE_Isorropia=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE \
    ../trilinos-src

# Build and install Trilinos
RUN cmake --build . -j 8 && \
    cmake --install .

# Build Xyce stage
FROM base AS xyce-build

# Copy Trilinos installation from previous stage
COPY --from=trilinos-build /opt/trilinos /opt/trilinos
COPY --from=trilinos-build /opt/suitesparse /opt/suitesparse

# Set environment variables for MPI
ENV CC=mpicc
ENV CXX=mpicxx
ENV FC=mpifort
ENV Trilinos_ROOT=/opt/trilinos
ENV PATH="/opt/trilinos/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/trilinos/lib:/opt/suitesparse/lib"

# The Xyce source will be mounted or copied at runtime
# Create build directory
WORKDIR /build/xyce-build

# Default command - will be overridden by build script
CMD ["/bin/bash"]

