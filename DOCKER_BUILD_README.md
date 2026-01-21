# Docker Build Setup for Xyce

This directory contains Docker configuration files to build Trilinos and Xyce with resource limits (1/4 of system resources: 8 CPU cores, ~7.75GB RAM).

## Files

- **Dockerfile**: Multi-stage Docker build that:
  - Installs all prerequisites (CMake, GCC, GFortran, OpenMPI, bison, flex, BLAS/LAPACK, SuiteSparse, FFTW)
  - Builds Trilinos 14.4 with MPI support
  - Sets up environment for building Xyce

- **build.sh**: Build script that:
  - Builds the Docker image
  - Runs the container with resource limits
  - Builds Xyce from the latest commit
  - Preserves build artifacts in `docker-build/` and `docker-install/`

- **.dockerignore**: Excludes unnecessary files from Docker build context

## Prerequisites

- Docker installed and running
- Sufficient disk space (~10-15GB for the full build)
- Network access to download dependencies

## Usage

### Quick Start

Simply run the build script:

```bash
./build.sh
```

This will:
1. Build the Docker image (includes Trilinos build)
2. Build Xyce from the current repository
3. Install Xyce to `docker-install/`

**Note**: The full build process can take 2-4 hours depending on your system:
- Trilinos build: ~1-2 hours
- Xyce build: ~30-60 minutes

### Resource Limits

The build uses 1/4 of system resources:
- **CPU**: 8 cores (out of 32)
- **Memory**: 7.75GB (out of ~31GB)

These limits are set in `build.sh` and can be adjusted if needed.

### Build Artifacts

After a successful build:
- **Build directory**: `docker-build/` - Contains CMake build files
- **Installation**: `docker-install/` - Contains installed Xyce binaries and libraries

### Running Xyce

After the build completes, you can run Xyce from the installation directory:

```bash
docker-install/bin/Xyce --version
```

Or run it inside a container:

```bash
docker run --rm -v $(pwd):/workspace xyce-builder /opt/xyce/bin/Xyce /workspace/your_circuit.cir
```

## Build Configuration

### Trilinos Configuration

Trilinos is built with MPI support using the following packages (matching `cmake/trilinos/trilinos-MPI-base.cmake`):
- NOX, LOCA, EpetraExt, TrilinosCouplings
- Ifpack, AztecOO, Belos, Teuchos
- Amesos (with KLU), Amesos2 (with KLU2 and Basker)
- Sacado, Stokhos, ROL
- Zoltan, Isorropia (for MPI parallelism)
- AMD (from SuiteSparse)

### Xyce Configuration

Xyce is built with:
- MPI parallelism enabled
- Trilinos from `/opt/trilinos`
- All standard Xyce features

## Troubleshooting

### Build Fails

1. Check Docker has enough resources allocated
2. Verify network connectivity for downloading dependencies
3. Check disk space: `df -h`
4. Review build logs in the container output

### MPI Issues

If MPI-related errors occur:
- Verify OpenMPI is properly installed in the container
- Check that Trilinos was built with MPI enabled
- Ensure MPI compilers (mpicc, mpicxx, mpifort) are available

### Memory Issues

If the build runs out of memory:
- Reduce the `-j` parallelism in the build commands
- Increase Docker's memory limit
- Build Trilinos and Xyce separately

## Manual Build Steps

If you need to build manually or debug:

1. Build the Docker image:
   ```bash
   docker build -t xyce-builder -f Dockerfile .
   ```

2. Run container interactively:
   ```bash
   docker run -it --cpus="8" --memory="7.75g" \
     -v $(pwd):/xyce-source:ro \
     -v $(pwd)/docker-build:/build/xyce-build \
     -v $(pwd)/docker-install:/opt/xyce \
     xyce-builder /bin/bash
   ```

3. Inside the container, configure and build:
   ```bash
   cd /build/xyce-build
   cmake -DCMAKE_INSTALL_PREFIX=/opt/xyce \
         -DCMAKE_C_COMPILER=mpicc \
         -DCMAKE_CXX_COMPILER=mpicxx \
         -DTrilinos_ROOT=/opt/trilinos \
         -DXyce_PARALLEL_MPI=ON \
         /xyce-source
   cmake --build . -j 8
   cmake --install .
   ```

## Notes

- The Dockerfile uses multi-stage builds to keep the final image size manageable
- Trilinos is built in the image, so subsequent Xyce builds are faster
- The Xyce source is mounted read-only to preserve the repository
- Build artifacts are preserved on the host for inspection





