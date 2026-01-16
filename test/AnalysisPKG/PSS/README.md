# PSS Analysis Unit Tests

This directory contains unit tests for the Periodic Steady State (PSS) analysis implementation, following a test-driven development approach.

## Files

### Core Implementation
- `PSSShootingMethod.h` / `PSSShootingMethod.C` - Shooting method implementation for PSS
  - Supports both driven and autonomous modes
  - Finite difference Jacobian computation
  - Simple Newton iteration solver

### Test Infrastructure
- `SimpleODETest.h` / `SimpleODETest.C` - Simple ODE test problems
  - `DrivenSimpleODE`: dx/dt = -x + sin(2πt/T) (has analytical solution)
  - `VanDerPolODE`: Van der Pol oscillator (autonomous)

### Time Integrators
- `EulerIntegrator`: Simple Euler method
- `RK4Integrator`: 4th order Runge-Kutta (more accurate)

### Unit Tests
- `N_ANP_PSSShooting_UnitTest.C` - Comprehensive unit tests for shooting method
  - Driven PSS tests (Euler and RK4)
  - Autonomous PSS tests (placeholder for future implementation)
  - Jacobian computation tests
  - Convergence robustness tests
  - Phase condition tests

## Building and Running Tests

The tests are integrated into Xyce's CMake build system. To build and run:

```bash
# Configure with GTest enabled
cmake -DXyce_GTEST_UNIT_TESTS=ON <other options> ..

# Build
make

# Run tests
ctest -L pss
# Or run specific test
./test/AnalysisPKG/PSS/PSS_ShootingMethod_UnitTests

## Running Full-Simulation Tests (MPI Builds)

When Xyce is built with MPI enabled, running multiple full-simulation tests in a
single process can trigger MPI_Finalize issues. To avoid this, run each test in
a separate process using the helper script:

```
./test/AnalysisPKG/PSS/run_pss_full_sim_tests.sh /path/to/build
```

This script sets `XYCE_RUN_PSS_FULL_SIM=1` and runs each integration/challenging
test individually. Matrix-free runs can use `.PSS MATRIXFREE` with optional
`GMRESRESTART`, `GMRESMAXITER`, `GMRESTOL`, `GMRESPRECOND`, `GMRESPRECONDMAX`,
`GMRESLOG`, and `PERFLOG`.
```

## Test Coverage

### Phase 0 Unit Tests (Complete)
- ✅ Driven PSS with simple ODE
- ✅ Jacobian computation (finite difference)
- ✅ Convergence with various initial guesses
- ✅ Convergence with different tolerances
- ✅ Phase condition computation
- ✅ Autonomous PSS (structure ready, implementation pending)

### Phase 2 Component Unit Tests (Complete - No Infrastructure Needed)
- ✅ Residual computation - Solution vector
- ✅ Residual computation - Combined (solution, state, store)
- ✅ Newton update computation
- ✅ Periodic BC tolerance checking
- ✅ Vector update operations
- ✅ Residual norm edge cases
- ✅ Convergence criteria

**Note:** These are true unit tests using `std::vector<double>` to test core algorithms
in isolation. They don't require Xyce infrastructure and can run immediately.

### Phase 2 Integration Tests (Structure Created, Implementation Pending)
- ⏳ integrateOnePeriod() - Basic integration test
- ⏳ Periodic BC enforcement - Solution vector (end-to-end)
- ⏳ Periodic BC enforcement - State vector (end-to-end)
- ⏳ Periodic BC enforcement - Store vector (end-to-end)
- ⏳ Newton iteration convergence (end-to-end)
- ⏳ Simple RC circuit integration test
- ⏳ RLC circuit integration test
- ⏳ Integration failure handling

**Note:** Integration tests require full Xyce infrastructure (AnalysisManager, DataStore, 
time integrator, etc.) and test the full stack. Component tests above test the algorithms
without infrastructure.

### Future Tests (To Be Added)
- Performance/multi-core tests
- Device model compatibility tests
- BSIM4 device model tests

## Implementation Status

**Current Status:** Phase 0 - Core Algorithm Unit Tests
- Basic shooting method implementation complete
- Unit tests for driven mode working
- Autonomous mode structure in place (needs implementation)
- Simple ODE test problems available

**Next Steps:**
1. Complete autonomous mode implementation
2. Add periodic BC enforcement tests
3. Add performance tests
4. Integrate with Xyce infrastructure (Phase 1)

