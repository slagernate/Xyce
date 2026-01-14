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
```

## Test Coverage

### Phase 0 Unit Tests (Complete)
- ✅ Driven PSS with simple ODE
- ✅ Jacobian computation (finite difference)
- ✅ Convergence with various initial guesses
- ✅ Convergence with different tolerances
- ✅ Phase condition computation
- ✅ Autonomous PSS (structure ready, implementation pending)

### Phase 2 Integration Tests (Structure Created, Implementation Pending)
- ⏳ integrateOnePeriod() - Basic integration test
- ⏳ Periodic BC enforcement - Solution vector
- ⏳ Periodic BC enforcement - State vector
- ⏳ Periodic BC enforcement - Store vector
- ⏳ Newton iteration convergence
- ⏳ Simple RC circuit integration test
- ⏳ RLC circuit integration test
- ⏳ Integration failure handling

**Note:** Phase 2 tests require full Xyce infrastructure (AnalysisManager, DataStore, time integrator, etc.). 
Currently, test structure is in place but tests are skipped until infrastructure is available.
These are integration tests rather than pure unit tests.

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

