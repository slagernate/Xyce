#!/usr/bin/env bash
set -euo pipefail

# Run PSS integration/challenging tests in separate processes to avoid MPI_Finalize issues.
# Requires a full Xyce build and netlists copied to the build directory.

if [[ -z "${1:-}" ]]; then
  echo "Usage: $0 <build_dir>"
  echo "Example: $0 /home/nates/github/Xyce/docker-build"
  exit 1
fi

BUILD_DIR="$1"
TEST_DIR="$BUILD_DIR/test/AnalysisPKG/PSS"

if [[ ! -d "$TEST_DIR" ]]; then
  echo "Test directory not found: $TEST_DIR"
  exit 1
fi

export XYCE_RUN_PSS_FULL_SIM=1
MPI_NP="${XYCE_MPI_NP:-1}"

run_tests() {
  local binary="$1"
  shift
  local tests=("$@")
  local run_cmd=("./$binary")

  if command -v mpirun >/dev/null 2>&1; then
    if [[ "$(id -u)" -eq 0 ]]; then
      run_cmd=("mpirun" "--allow-run-as-root" "-np" "$MPI_NP" "./$binary")
    elif [[ "$MPI_NP" -gt 1 ]]; then
      run_cmd=("mpirun" "-np" "$MPI_NP" "./$binary")
    fi
  fi

  if [[ ! -x "$TEST_DIR/$binary" ]]; then
    echo "Test binary not found: $TEST_DIR/$binary"
    exit 1
  fi

  for test in "${tests[@]}"; do
    echo "=== $binary --gtest_filter=$test ==="
    (cd "$TEST_DIR" && "${run_cmd[@]}" --gtest_filter="$test")
  done
}

# Integration tests
run_tests "PSS_Integration_UnitTests" \
  "PSSIntegrationTest.PSSAnalysis_Recognition" \
  "PSSIntegrationTest.PeriodicBC_SolutionVector" \
  "PSSIntegrationTest.PeriodicBC_StateVector" \
  "PSSIntegrationTest.PeriodicBC_StoreVector" \
  "PSSIntegrationTest.NewtonIteration_Convergence" \
  "PSSIntegrationTest.SimpleRC_Circuit" \
  "PSSIntegrationTest.RLCCircuit" \
  "PSSIntegrationTest.IntegrationFailure_Handling"

# Challenging tests
run_tests "PSS_Challenging_UnitTests" \
  "PSSChallengingTest.DiodeRectifier_StrongNonlinearity" \
  "PSSChallengingTest.StiffSystem_WidelySeparatedTimeConstants" \
  "PSSChallengingTest.PoorInitialGuess_FarFromSolution" \
  "PSSChallengingTest.VerySmallPeriod_HighFrequency" \
  "PSSChallengingTest.VeryLargePeriod_LowFrequency" \
  "PSSChallengingTest.MultipleTimeConstants_ComplexDynamics" \
  "PSSChallengingTest.LargeCircuit_ManyNodes" \
  "PSSChallengingTest.ExtremeParameterValues" \
  "PSSChallengingTest.ConvergenceFailure_NonPeriodicSystem" \
  "PSSChallengingTest.TightTolerance_HighAccuracy" \
  "PSSChallengingTest.Autonomous_LC_Oscillator" \
  "PSSChallengingTest.Autonomous_RingOscillator"

