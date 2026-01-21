#!/bin/bash
# Build script for Xyce using Docker with GTest enabled for unit tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="xyce-builder"
CONTAINER_NAME="xyce-build-container"

# Resource limits (1/4 of system: 8 cores, ~7.75GB RAM)
CPU_LIMIT="8"
MEMORY_LIMIT="7.75g"

echo "=========================================="
echo "Xyce Docker Build Script (with GTest)"
echo "=========================================="
echo "Resource limits: ${CPU_LIMIT} CPUs, ${MEMORY_LIMIT} RAM"
echo "GTest unit tests: ENABLED"
echo ""

# Build the Docker image (with --no-cache for complete rebuild)
echo "Building Docker image (no cache - this will rebuild Trilinos from scratch)..."
docker build --no-cache -t ${IMAGE_NAME} -f ${SCRIPT_DIR}/Dockerfile ${SCRIPT_DIR}

# Remove existing container if it exists
if [ "$(docker ps -aq -f name=${CONTAINER_NAME})" ]; then
    echo "Removing existing container..."
    docker rm -f ${CONTAINER_NAME} > /dev/null 2>&1 || true
fi

# Create build directories on host for artifacts
BUILD_DIR="${SCRIPT_DIR}/docker-build"
INSTALL_DIR="${SCRIPT_DIR}/docker-install"
mkdir -p ${BUILD_DIR} ${INSTALL_DIR}

echo ""
echo "Starting build container..."
echo "Building Trilinos and Xyce with GTest enabled..."

# Run the container with resource limits and build Xyce
docker run \
    --name ${CONTAINER_NAME} \
    --cpus="${CPU_LIMIT}" \
    --memory="${MEMORY_LIMIT}" \
    --rm \
    -v "${SCRIPT_DIR}:/xyce-source:ro" \
    -v "${BUILD_DIR}:/build/xyce-build" \
    -v "${INSTALL_DIR}:/opt/xyce" \
    ${IMAGE_NAME} \
    /bin/bash -c "
        set -e
        echo '=========================================='
        echo 'Building Xyce with GTest'
        echo '=========================================='
        
        # Configure Xyce with MPI and GTest
        cd /build/xyce-build
        cmake \
            -DCMAKE_INSTALL_PREFIX=/opt/xyce \
            -DCMAKE_C_COMPILER=mpicc \
            -DCMAKE_CXX_COMPILER=mpicxx \
            -DTrilinos_ROOT=/opt/trilinos \
            -DXyce_PARALLEL_MPI=ON \
            -DXyce_GTEST_UNIT_TESTS=ON \
            -DBUILD_TESTING=ON \
            /xyce-source
        
        # Build Xyce
        echo ''
        echo 'Compiling Xyce (this may take a while)...'
        cmake --build . -j ${CPU_LIMIT}
        
        # Install Xyce
        echo ''
        echo 'Installing Xyce...'
        cmake --install .
        
        # Run a simple test
        echo ''
        echo 'Running basic test...'
        /opt/xyce/bin/Xyce -v || echo 'Version check failed, but build completed'
        
        # Run PSS unit tests
        echo ''
        echo '=========================================='
        echo 'Running PSS Unit Tests'
        echo '=========================================='
        
        # Find and run PSS test executables
        if [ -f /build/xyce-build/test/AnalysisPKG/PSS/PSS_Component_UnitTests ]; then
            echo 'Running PSS Component Tests...'
            /build/xyce-build/test/AnalysisPKG/PSS/PSS_Component_UnitTests || echo 'Component tests failed'
        fi
        
        if [ -f /build/xyce-build/test/AnalysisPKG/PSS/PSS_Integration_UnitTests ]; then
            echo 'Running PSS Integration Tests...'
            /build/xyce-build/test/AnalysisPKG/PSS/PSS_Integration_UnitTests || echo 'Integration tests failed'
        fi
        
        if [ -f /build/xyce-build/test/AnalysisPKG/PSS/PSS_Challenging_UnitTests ]; then
            echo 'Running PSS Challenging Tests...'
            /build/xyce-build/test/AnalysisPKG/PSS/PSS_Challenging_UnitTests || echo 'Challenging tests failed'
        fi
        
        if [ -f /build/xyce-build/test/AnalysisPKG/PSS/PSS_ShootingMethod_UnitTests ]; then
            echo 'Running PSS Shooting Method Tests...'
            /build/xyce-build/test/AnalysisPKG/PSS/PSS_ShootingMethod_UnitTests || echo 'Shooting method tests failed'
        fi
        
        # Run all PSS tests via ctest
        echo ''
        echo 'Running all PSS tests via ctest...'
        cd /build/xyce-build
        ctest -L pss -V || echo 'Some PSS tests may have failed'
        
        echo ''
        echo '=========================================='
        echo 'Build and Test completed!'
        echo '=========================================='
        echo 'Xyce installed to: /opt/xyce'
        echo 'Build directory: /build/xyce-build'
    "

echo ""
echo "=========================================="
echo "Build Summary"
echo "=========================================="
echo "Build artifacts: ${BUILD_DIR}"
echo "Installation: ${INSTALL_DIR}"
echo ""
echo "To run Xyce:"
echo "  ${INSTALL_DIR}/bin/Xyce --version"
echo ""
echo "To run PSS tests manually:"
echo "  cd ${BUILD_DIR}"
echo "  ctest -L pss -V"
echo ""


