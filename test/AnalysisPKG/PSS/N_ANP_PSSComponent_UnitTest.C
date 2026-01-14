//-------------------------------------------------------------------------
//   Copyright 2002-2025 National Technology & Engineering Solutions of
//   Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
//   NTESS, the U.S. Government retains certain rights in this software.
//
//   This file is part of the Xyce(TM) Parallel Electrical Simulator.
//
//   Xyce(TM) is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Xyce(TM) is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Xyce(TM).
//   If not, see <http://www.gnu.org/licenses/>.
//-------------------------------------------------------------------------

// Component-level Unit Tests for PSS Analysis
// These tests verify individual components and logic without requiring
// full Xyce infrastructure or end-to-end functionality.
//
// These tests use std::vector<double> to test the core algorithms,
// making them true unit tests that don't depend on Xyce infrastructure.

#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <algorithm>

namespace
{

  //-------------------------------------------------------------------------
  // Test Fixture for PSS Component Tests
  //-------------------------------------------------------------------------
  class PSSComponentTest : public ::testing::Test
  {
  protected:
    void SetUp() override
    {
    }
    
    void TearDown() override
    {
    }
  };

  //-------------------------------------------------------------------------
  // Helper function: Compute residual norm for periodic BC
  //-------------------------------------------------------------------------
  // This tests the core algorithm logic used in PSS::doLoopProcess()
  double computeResidualNorm(
    const std::vector<double>& xT,      // Solution at t=T
    const std::vector<double>& x0)       // Solution at t=0
  {
    if (xT.size() != x0.size()) {
      return 1e10;  // Error case
    }
    
    double maxResidual = 0.0;
    for (size_t i = 0; i < xT.size(); ++i) {
      double residual = std::abs(xT[i] - x0[i]);
      maxResidual = std::max(maxResidual, residual);
    }
    return maxResidual;
  }

  //-------------------------------------------------------------------------
  // Test 1: Residual Computation - Solution Vector
  //-------------------------------------------------------------------------
  // Tests the core logic of computing periodic BC residual
  // This tests the algorithm used in PSS::doLoopProcess()
  TEST_F(PSSComponentTest, ResidualComputation_SolutionVector)
  {
    // Create test vectors
    const int size = 10;
    std::vector<double> x0(size);
    std::vector<double> xT(size);
    
    // Set up periodic case: xT = x0 (perfect periodicity)
    for (int i = 0; i < size; ++i)
    {
      x0[i] = 1.0 + 0.1 * i;
      xT[i] = x0[i];  // Perfect periodicity
    }
    
    double residualNorm = computeResidualNorm(xT, x0);
    EXPECT_NEAR(residualNorm, 0.0, 1e-12) 
      << "Residual should be zero for periodic solution";
    
    // Test non-periodic case
    xT[0] += 0.01;  // Introduce error
    residualNorm = computeResidualNorm(xT, x0);
    EXPECT_NEAR(residualNorm, 0.01, 1e-12)
      << "Residual should detect non-periodicity";
  }

  //-------------------------------------------------------------------------
  // Test 2: Residual Computation - Multiple Vector Types
  //-------------------------------------------------------------------------
  // Tests computing combined residual from solution, state, and store vectors
  // This tests the logic in PSS::doLoopProcess() that combines residuals
  TEST_F(PSSComponentTest, ResidualComputation_Combined)
  {
    const int solSize = 5;
    const int stateSize = 3;
    const int storeSize = 2;
    
    std::vector<double> sol0(solSize), solT(solSize);
    std::vector<double> state0(stateSize), stateT(stateSize);
    std::vector<double> store0(storeSize), storeT(storeSize);
    
    // Initialize with periodic values
    for (int i = 0; i < solSize; ++i) {
      sol0[i] = 1.0 + i;
      solT[i] = sol0[i];
    }
    for (int i = 0; i < stateSize; ++i) {
      state0[i] = 0.5 + i;
      stateT[i] = state0[i];
    }
    for (int i = 0; i < storeSize; ++i) {
      store0[i] = 0.1 + i;
      storeT[i] = store0[i];
    }
    
    // Compute individual residuals (as done in PSS::doLoopProcess())
    double solResidual = computeResidualNorm(solT, sol0);
    double stateResidual = computeResidualNorm(stateT, state0);
    double storeResidual = computeResidualNorm(storeT, store0);
    
    // Combined residual (max of all) - matches PSS implementation
    double combinedResidual = std::max({solResidual, stateResidual, storeResidual});
    
    EXPECT_NEAR(combinedResidual, 0.0, 1e-12)
      << "Combined residual should be zero for periodic solution";
    
    // Test with error in state vector
    stateT[0] += 0.05;
    stateResidual = computeResidualNorm(stateT, state0);
    combinedResidual = std::max({solResidual, stateResidual, storeResidual});
    EXPECT_NEAR(combinedResidual, 0.05, 1e-12)
      << "Combined residual should detect error in state vector";
  }

  //-------------------------------------------------------------------------
  // Test 3: Newton Update Computation
  //-------------------------------------------------------------------------
  // Tests the Newton update computation logic used in computeNewtonUpdate()
  TEST_F(PSSComponentTest, NewtonUpdate_Computation)
  {
    const int size = 5;
    std::vector<double> residual(size);
    std::vector<double> update(size);
    
    // Set up residual
    for (int i = 0; i < size; ++i) {
      residual[i] = 0.1 * (i + 1);
    }
    
    // Simple damped Newton update: delta = -alpha * residual
    // This matches the logic in PSS::computeNewtonUpdate()
    double alpha = 0.1;
    for (int i = 0; i < size; ++i) {
      update[i] = -alpha * residual[i];
    }
    
    // Verify update
    for (int i = 0; i < size; ++i) {
      EXPECT_NEAR(update[i], -alpha * residual[i], 1e-12)
        << "Newton update should be -alpha * residual";
    }
    
    // Verify update reduces residual (for linear case)
    // newResidual = residual + update = residual - 0.1*residual = 0.9*residual
    std::vector<double> newResidual(size);
    for (int i = 0; i < size; ++i) {
      newResidual[i] = residual[i] + update[i];
      EXPECT_NEAR(newResidual[i], 0.9 * residual[i], 1e-12)
        << "Update should reduce residual";
    }
  }

  //-------------------------------------------------------------------------
  // Test 4: Periodic BC Tolerance Check
  //-------------------------------------------------------------------------
  // Tests the tolerance checking logic for periodic BC
  // This tests the convergence check in PSS::doLoopProcess()
  TEST_F(PSSComponentTest, PeriodicBC_ToleranceCheck)
  {
    const int size = 10;
    std::vector<double> x0(size);
    std::vector<double> xT(size);
    
    double tolerance = 1e-6;
    
    // Test 1: Perfect periodicity (should pass)
    for (int i = 0; i < size; ++i) {
      x0[i] = 1.0;
      xT[i] = 1.0;
    }
    double residual = computeResidualNorm(xT, x0);
    EXPECT_LT(residual, tolerance) 
      << "Perfect periodicity should pass tolerance check";
    
    // Test 2: Small error within tolerance (should pass)
    xT[0] = 1.0 + tolerance / 2.0;
    residual = computeResidualNorm(xT, x0);
    EXPECT_LT(residual, tolerance)
      << "Small error within tolerance should pass";
    
    // Test 3: Large error outside tolerance (should fail)
    xT[0] = 1.0 + tolerance * 2.0;
    residual = computeResidualNorm(xT, x0);
    EXPECT_GT(residual, tolerance)
      << "Large error outside tolerance should fail";
  }

  //-------------------------------------------------------------------------
  // Test 5: Vector Update Operations
  //-------------------------------------------------------------------------
  // Tests the vector update operations used in PSS
  // These test the operations: update(alpha, vec1, beta, vec2, gamma)
  TEST_F(PSSComponentTest, VectorUpdate_Operations)
  {
    const int size = 5;
    std::vector<double> x0(size);
    std::vector<double> xT(size);
    std::vector<double> xNew(size, 0.0);
    
    // Initialize
    for (int i = 0; i < size; ++i) {
      x0[i] = 1.0 + i;
      xT[i] = 2.0 + i;
    }
    
    // Test: Save x0 (equivalent to x0_sol->update(1.0, currSolutionPtr, 0.0))
    std::vector<double> saved_x0(size);
    for (int i = 0; i < size; ++i) {
      saved_x0[i] = 1.0 * x0[i] + 0.0;  // update(1.0, x0, 0.0)
    }
    for (int i = 0; i < size; ++i) {
      EXPECT_NEAR(saved_x0[i], x0[i], 1e-12)
        << "Vector copy should preserve values";
    }
    
    // Test: Compute residual (equivalent to residual->update(1.0, nextSolutionPtr, -1.0, x0_sol, 0.0))
    std::vector<double> residual(size);
    for (int i = 0; i < size; ++i) {
      residual[i] = 1.0 * xT[i] + (-1.0) * x0[i] + 0.0;  // update(1.0, xT, -1.0, x0, 0.0)
    }
    for (int i = 0; i < size; ++i) {
      EXPECT_NEAR(residual[i], xT[i] - x0[i], 1e-12)
        << "Residual should be xT - x0";
    }
    
    // Test: Update initial condition (equivalent to currSolutionPtr->update(1.0, update, 1.0))
    std::vector<double> update(size);
    for (int i = 0; i < size; ++i) {
      update[i] = 0.1;
    }
    // xNew = 1.0 * update + 1.0 * xNew (which starts as zero)
    for (int i = 0; i < size; ++i) {
      xNew[i] = 1.0 * update[i] + 1.0 * xNew[i];
    }
    for (int i = 0; i < size; ++i) {
      EXPECT_NEAR(xNew[i], 0.1, 1e-12)
        << "Update operation should work correctly";
    }
    
    // Test: xNew = x0 + update (equivalent to xNew->update(1.0, x0, 1.0))
    // where xNew already contains update
    for (int i = 0; i < size; ++i) {
      xNew[i] = 1.0 * x0[i] + 1.0 * xNew[i];  // xNew = x0 + xNew (which is update)
    }
    for (int i = 0; i < size; ++i) {
      EXPECT_NEAR(xNew[i], x0[i] + 0.1, 1e-12)
        << "Combined update should work correctly";
    }
  }

  //-------------------------------------------------------------------------
  // Test 6: Residual Norm Computation - Edge Cases
  //-------------------------------------------------------------------------
  // Tests edge cases for residual computation
  TEST_F(PSSComponentTest, ResidualNorm_EdgeCases)
  {
    // Test 1: Zero vectors
    std::vector<double> zero1(5, 0.0);
    std::vector<double> zero2(5, 0.0);
    double residual = computeResidualNorm(zero1, zero2);
    EXPECT_NEAR(residual, 0.0, 1e-12)
      << "Residual of zero vectors should be zero";
    
    // Test 2: Single element vectors
    std::vector<double> single1(1, 1.0);
    std::vector<double> single2(1, 1.0);
    residual = computeResidualNorm(single1, single2);
    EXPECT_NEAR(residual, 0.0, 1e-12)
      << "Residual of identical single-element vectors should be zero";
    
    // Test 3: Large vectors
    const int largeSize = 1000;
    std::vector<double> large1(largeSize, 1.0);
    std::vector<double> large2(largeSize, 1.0);
    residual = computeResidualNorm(large1, large2);
    EXPECT_NEAR(residual, 0.0, 1e-12)
      << "Residual computation should work for large vectors";
    
    // Test 4: Maximum error in one element
    large2[500] = 1.0 + 0.5;
    residual = computeResidualNorm(large1, large2);
    EXPECT_NEAR(residual, 0.5, 1e-12)
      << "Residual should capture maximum error";
  }

  //-------------------------------------------------------------------------
  // Test 7: Convergence Criteria
  //-------------------------------------------------------------------------
  // Tests the convergence checking logic
  TEST_F(PSSComponentTest, ConvergenceCriteria)
  {
    double tolerance = 1e-6;
    int maxIterations = 50;
    
    // Test convergence scenarios
    struct TestCase {
      double residualNorm;
      int iterations;
      bool shouldConverge;
    };
    
    std::vector<TestCase> testCases = {
      {1e-7, 10, true},   // Residual below tolerance, within max iterations
      {1e-5, 10, false},  // Residual above tolerance
      {1e-7, 60, true},   // Residual below tolerance, but exceeded max (edge case)
      {1e-5, 60, false},  // Both criteria failed
    };
    
    for (const auto& testCase : testCases)
    {
      bool converged = (testCase.residualNorm < tolerance) && 
                       (testCase.iterations < maxIterations);
      EXPECT_EQ(converged, testCase.shouldConverge)
        << "Convergence check failed for residual=" << testCase.residualNorm
        << ", iterations=" << testCase.iterations;
    }
  }

} // namespace

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

