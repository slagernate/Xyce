# PSS Analysis Implementation Plan - Final (No GPU)

## Implementation Decisions

Based on roadblock analysis:

1. **Periodic BC:** Option A - Implement at analysis level (not integrator level), unit test first
2. **Jacobian:** Option A - Start with finite difference, unit test first
3. **Initial Guess:** Unit test convergence with various guesses
4. **Device Models:** Start simple (R, L, C, diodes), plan expansion to BSIM4, unit test device compatibility
5. **Performance:** Multi-core CPU parallelism (OpenMP), unit test algorithm speed and multi-core execution
6. **Phase Condition:** Unit test phase condition computation

**Note:** GPU acceleration is **NOT** included in this plan. Rationale:
- Xyce has no existing GPU infrastructure
- Time integration is sequential (not GPU-friendly)
- Device model evaluation is memory-bound, not compute-bound
- CPU multi-core parallelism is simpler and sufficient for most cases
- GPU can be reconsidered later if needed for very large circuits

## Phase 0: Unit Tests for Core Algorithms (CRITICAL - DO FIRST)

### 0.1 Shooting Method Core Algorithm Tests

**Location:** `test/AnalysisPKG/PSS/`

**Test Files:**
- `N_ANP_PSSShooting_UnitTest.C` - Core algorithm unit tests
- `N_ANP_PSSShooting_UnitTest.h`
- `SimpleODETest.C` - Simple ODE test problems for validation

**Test Cases:**

**A. Driven PSS - Simple ODE:**
```cpp
TEST(ShootingMethod, DrivenSimpleODE) {
  // Test: dx/dt = -x + sin(2πt/T), period T
  // Verify: shooting iteration converges
  // Verify: x(T) = x(0) within tolerance
  // Verify: matches analytical solution
}
```

**B. Autonomous PSS - Simple ODE:**
```cpp
TEST(ShootingMethod, AutonomousSimpleODE) {
  // Test: Van der Pol or similar oscillator
  // Verify: period is found correctly
  // Verify: phase condition (dV/dt = 0) works
  // Verify: x(T) = x(0)
}
```

**C. Jacobian Computation (Finite Difference):**
```cpp
TEST(ShootingMethod, JacobianFiniteDifference) {
  // Test: finite difference Jacobian computation
  // Verify: Jacobian accuracy vs analytical (if available)
  // Verify: Jacobian produces correct Newton convergence
  // Test: numerical stability of finite difference
}
```

**D. Convergence Robustness:**
```cpp
TEST(ShootingMethod, ConvergenceRobustness) {
  // Test: convergence with poor initial guess
  // Test: convergence with different tolerances
  // Test: failure cases (non-periodic systems)
  // Test: multiple periodic solutions
}
```

**E. Phase Condition:**
```cpp
TEST(ShootingMethod, PhaseCondition) {
  // Test: dV/dt = 0 condition computation
  // Test: solution uniqueness with phase condition
  // Test: different reference nodes
  // Test: phase condition in Newton iteration
}
```

### 0.2 Periodic Boundary Condition Tests

**Test File:** `N_ANP_PSSPeriodicBC_UnitTest.C`

```cpp
TEST(PeriodicBC, Enforcement) {
  // Test: BC constraint x(T) = x(0) is satisfied after convergence
  // Test: BC residual computation accuracy
  // Test: BC in Newton iteration (constraint handling)
  // Test: BC with different state vector sizes
}
```

### 0.3 Performance and Multi-Core Tests

**Test File:** `N_ANP_PSSPerformance_UnitTest.C`

**A. Algorithm Speed:**
```cpp
TEST(PSSPerformance, ShootingIterationSpeed) {
  // Benchmark: shooting iteration time
  // Benchmark: Jacobian computation time
  // Benchmark: full PSS solve time
  // Compare: finite difference vs adjoint (if implemented)
}
```

**B. Multi-Core Execution (OpenMP):**
```cpp
TEST(PSSPerformance, MultiCoreExecution) {
  // Test: parallel Jacobian computation (finite difference)
  // Test: OpenMP thread safety
  // Test: scaling with number of cores (1, 2, 4, 8+)
  // Benchmark: speedup vs serial
  // Test: load balancing across threads
}
```

**Note:** Multi-core parallelism will use OpenMP for:
- Parallel finite difference Jacobian computation (each perturbation is independent)
- Thread-safe time integration calls
- Load balancing across available CPU cores

### 0.4 Device Model Compatibility Tests

**Test File:** `N_ANP_PSSDeviceModel_UnitTest.C`

**A. Simple Devices:**
```cpp
TEST(PSSDeviceModel, SimpleDevices) {
  // Test: Resistor (R) with periodic BC
  // Test: Capacitor (C) with periodic BC
  // Test: Inductor (L) with periodic BC
  // Test: Diode with periodic BC
  // Verify: device state is periodic
}
```

**B. BSIM4 Preparation:**
```cpp
TEST(PSSDeviceModel, BSIM4Compatibility) {
  // Test: BSIM4 MOSFET with periodic BC (when ready)
  // Test: BSIM4 state variables are periodic
  // Test: BSIM4 charge/flux conservation
  // Note: This test will be added when BSIM4 support is implemented
}
```

### 0.5 Integration with Simple Time Stepper

**Test File:** `N_ANP_PSSIntegration_UnitTest.C`

```cpp
TEST(ShootingIntegration, SimpleTimeStepper) {
  // Test: shooting works with Euler method
  // Test: shooting works with RK4
  // Test: shooting works with Xyce's time integrator interface
  // Verify: accuracy vs analytical solution
}
```

## Phase 1: Minimal Xyce Integration

After all Phase 0 unit tests pass:

1. Create PSS analysis class skeleton
2. Register parser (`.PSS` command)
3. Register factory
4. Basic output system (minimal initially)
5. Integration test: PSS command is recognized

## Phase 2: Driven PSS with Simple Circuits

1. Integrate shooting method with Xyce time integrator
2. Test with simple circuits:
   - RC circuit with periodic source
   - RLC circuit with sinusoidal input
   - Diode rectifier
3. Compare results with long TRAN simulation
4. Fix integration issues

## Phase 3: Autonomous PSS

1. Implement autonomous mode
2. Add phase condition computation
3. Test with simple oscillators:
   - LC oscillator
   - Ring oscillator
   - Relaxation oscillator
4. Verify period finding accuracy

## Phase 4: Performance Optimization (CPU Multi-Core)

1. **Multi-Core Jacobian Computation (OpenMP):**
   - Parallelize finite difference perturbations
   - Each perturbation runs on separate thread
   - Thread-safe time integration calls
   - Test multi-core scaling (1, 2, 4, 8+ cores)
   - Benchmark performance improvement

2. **Algorithm Optimization:**
   - Optimize shooting iteration
   - Reduce unnecessary computations
   - Memory optimization
   - Consider adjoint methods (future: reduce Jacobian cost from O(N) to O(1))

3. **MPI Parallelization (if applicable):**
   - Leverage existing Xyce MPI infrastructure
   - For distributed memory systems
   - Coordinate with existing solver infrastructure

## Phase 5: BSIM4 Device Model Support

1. **Investigation:**
   - Analyze BSIM4 state variables
   - Identify periodic BC requirements
   - Check charge/flux conservation

2. **Implementation:**
   - Add BSIM4 periodic BC support
   - Unit test BSIM4 with periodic conditions
   - Integration test with BSIM4 circuits

3. **Validation:**
   - Compare PSS results with TRAN for BSIM4 circuits
   - Verify accuracy and convergence

## Phase 6: Full Testing and Validation

1. Comprehensive regression test suite
2. Cross-validation with TRAN/HB/MPDE
3. Performance benchmarking
4. Documentation

## Updated File Structure

### Phase 0 Test Files:
- `test/AnalysisPKG/PSS/N_ANP_PSSShooting_UnitTest.C`
- `test/AnalysisPKG/PSS/N_ANP_PSSShooting_UnitTest.h`
- `test/AnalysisPKG/PSS/N_ANP_PSSPeriodicBC_UnitTest.C`
- `test/AnalysisPKG/PSS/N_ANP_PSSPerformance_UnitTest.C`
- `test/AnalysisPKG/PSS/N_ANP_PSSDeviceModel_UnitTest.C`
- `test/AnalysisPKG/PSS/N_ANP_PSSIntegration_UnitTest.C`
- `test/AnalysisPKG/PSS/SimpleODETest.C` - Test problem definitions
- `test/AnalysisPKG/PSS/CMakeLists.txt`

### Implementation Files (Phases 1-6):
- `src/AnalysisPKG/N_ANP_PSS.h`
- `src/AnalysisPKG/N_ANP_PSS.C`
- `src/AnalysisPKG/N_ANP_PSSShooting.h` - Core algorithm (unit tested in Phase 0)
- `src/AnalysisPKG/N_ANP_PSSShooting.C`
- `src/AnalysisPKG/N_ANP_PSSParallel.h` - OpenMP parallel support
- `src/AnalysisPKG/N_ANP_PSSParallel.C`
- `src/IOInterfacePKG/N_IO_OutputterPSS.h`
- `src/IOInterfacePKG/N_IO_OutputterPSS.C`

## Unit Test Implementation Details

### Test Framework

Use Google Test framework (if `Xyce_GTEST_UNIT_TESTS` enabled):
- Follow existing pattern in `test/` directories
- Integrates with CMake/CTest
- Enables parallel test execution

### Test Organization

```
test/AnalysisPKG/PSS/
├── CMakeLists.txt
├── N_ANP_PSSShooting_UnitTest.C      # Core algorithm tests
├── N_ANP_PSSPeriodicBC_UnitTest.C    # BC enforcement tests
├── N_ANP_PSSPerformance_UnitTest.C   # Performance/multi-core tests
├── N_ANP_PSSDeviceModel_UnitTest.C   # Device compatibility tests
├── N_ANP_PSSIntegration_UnitTest.C   # Time integrator integration
└── SimpleODETest.C                    # Test problem definitions
```

### Performance Test Requirements

**Multi-Core Tests:**
- Test with 1, 2, 4, 8+ cores
- Measure speedup and efficiency
- Test load balancing
- Test thread safety
- Verify correctness (results match serial version)

## Implementation Decisions Summary

| Component | Decision | Unit Test Priority |
|-----------|----------|-------------------|
| Periodic BC | Analysis level (Option A) | **CRITICAL** - Test first |
| Jacobian | Finite difference (Option A) | **CRITICAL** - Test first |
| Initial Guess | Startup periods, DC OP | **HIGH** - Test convergence |
| Device Models | Start simple → BSIM4 | **HIGH** - Test compatibility |
| Performance | Multi-core CPU (OpenMP) | **MEDIUM** - Test speed/scaling |
| Phase Condition | dV/dt = 0 | **HIGH** - Test correctness |

## Success Criteria

### Phase 0 (Unit Tests):
- ✅ All shooting method unit tests pass
- ✅ Jacobian finite difference verified accurate
- ✅ Periodic BC enforcement tested and working
- ✅ Convergence tested with various initial guesses
- ✅ Phase condition tested and correct
- ✅ Performance tests show expected multi-core scaling
- ✅ Device model tests pass for simple devices
- ✅ Algorithm is correct before Xyce integration

### Phase 1-6 (Integration):
- ✅ PSS command works in netlist
- ✅ Driven PSS produces correct results
- ✅ Autonomous PSS finds period correctly
- ✅ Results match TRAN/HB where applicable
- ✅ Multi-core parallelization works (OpenMP)
- ✅ BSIM4 support works (Phase 5)
- ✅ Performance acceptable

## Recommended Starting Point

**IMMEDIATE ACTION: Start Phase 0**

1. Create test directory structure
2. Implement standalone shooting method (no Xyce dependencies initially)
3. Write comprehensive unit tests
4. Get all tests passing on simple ODEs
5. Then integrate with Xyce infrastructure

This test-driven approach ensures:
- Core algorithm is correct before complex integration
- Bugs caught early
- Rapid iteration on algorithm improvements
- Confidence before large integration effort
- Performance characteristics understood early

