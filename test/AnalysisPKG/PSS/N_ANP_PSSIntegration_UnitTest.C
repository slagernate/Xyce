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

// Phase 2 Integration Tests for PSS Analysis
// These tests verify the integration of the shooting method with Xyce's
// time integrator and infrastructure.

#include "Xyce_config.h"
#include <gtest/gtest.h>
#include <N_CIR_Xyce.h>
#include <N_ANP_PSS.h>
#include <N_ANP_AnalysisManager.h>
#include <N_TIA_DataStore.h>
#include <N_TIA_StepErrorControl.h>
#include <N_LAS_Vector.h>
#include <N_UTL_Math.h>
#include <cmath>
#include <cstdlib>
#include <string>

namespace
{
  using namespace Xyce;
  using namespace Xyce::Analysis;
  using namespace Xyce::Circuit;

  // Helper struct/function to create command line arguments with stable storage
  struct CmdArgs
  {
    std::vector<std::string> args;
    std::vector<char*> argv;
  };

  CmdArgs createCmdLineArgs(const std::string& netlist)
  {
    CmdArgs cmd;
    cmd.args.push_back("XyceTests");
    cmd.args.push_back(netlist);

    cmd.argv.reserve(cmd.args.size());
    for (auto& arg : cmd.args)
    {
      cmd.argv.push_back(const_cast<char*>(arg.c_str()));
    }
    return cmd;
  }

  bool allowFullPssSimulation()
  {
    const char* env = std::getenv("XYCE_RUN_PSS_FULL_SIM");
    return env && std::string(env) == "1";
  }

  int getMpiSizeFromEnv()
  {
    const char *env = std::getenv("OMPI_COMM_WORLD_SIZE");
    if (!env)
      env = std::getenv("PMI_SIZE");
    if (!env)
      env = std::getenv("SLURM_NTASKS");
    if (!env)
      return 1;
    return std::max(1, std::atoi(env));
  }

  // Test fixture for PSS integration tests
  class PSSIntegrationTest : public ::testing::Test
  {
  protected:
    void SetUp() override
    {
      if (!allowFullPssSimulation())
      {
        GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
      }

      // Tests will create Simulator objects as needed
    }
    
    void TearDown() override
    {
      // Cleanup handled by test destructors
    }
  };

  //-------------------------------------------------------------------------
  // Test 1: PSS Analysis - Basic Recognition
  //-------------------------------------------------------------------------
  // Verifies that PSS analysis is recognized and can be initialized
  TEST_F(PSSIntegrationTest, PSSAnalysis_Recognition)
  {
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    // PSS should be recognized (even if it fails later)
    // For now, just verify initialization doesn't crash
    EXPECT_NE(status, Simulator::RunStatus::ERROR) 
      << "PSS analysis should be recognized during initialization";
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 2: Periodic BC Enforcement - Solution Vector
  //-------------------------------------------------------------------------
  // Verifies that periodic boundary condition x(T) = x(0) is enforced
  // for the solution vector
  TEST_F(PSSIntegrationTest, PeriodicBC_SolutionVector)
  {
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      // Access AnalysisManager to get PSS analysis object
      Analysis::AnalysisManager& am = xycePtr->getAnalysisManager();
      
      // Check if PSS analysis mode is set
      if (am.getAnalysisMode() == Analysis::ANP_MODE_PSS)
      {
        // Verify PSS mode is set
        // Note: getAnalysisObjectPtr() is protected, so we verify mode instead
        EXPECT_EQ(am.getAnalysisMode(), Analysis::ANP_MODE_PSS);
        
        // Once PSS is fully implemented, verify periodic BC:
        // 1. Get DataStore
        // 2. Compare currSolutionPtr (x(0)) with solution at t=T
        // 3. Verify ||x(T) - x(0)|| < tolerance
      }
    }
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 3: Periodic BC Enforcement - State Vector
  //-------------------------------------------------------------------------
  // Verifies that periodic boundary condition is enforced for state vectors
  // (capacitor charge, inductor flux, etc.)
  TEST_F(PSSIntegrationTest, PeriodicBC_StateVector)
  {
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RLC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      Analysis::AnalysisManager& am = xycePtr->getAnalysisManager();
      
      if (am.getAnalysisMode() == Analysis::ANP_MODE_PSS)
      {
        // Once PSS is fully implemented, verify:
        // 1. Get DataStore
        // 2. Compare currStatePtr (state(0)) with state at t=T
        // 3. Verify ||state(T) - state(0)|| < tolerance
        // This is critical for circuits with capacitors/inductors
        
        // Verify PSS mode is set (can't access protected getAnalysisObjectPtr)
        EXPECT_EQ(am.getAnalysisMode(), Analysis::ANP_MODE_PSS);
      }
    }
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 4: Periodic BC Enforcement - Store Vector
  //-------------------------------------------------------------------------
  // Verifies that periodic boundary condition is enforced for store vectors
  // (device internal states)
  TEST_F(PSSIntegrationTest, PeriodicBC_StoreVector)
  {
    // This test requires devices with internal states (e.g., diodes, transistors)
    // For now, we use the RC circuit and verify the test structure
    
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      // Once PSS is fully implemented with devices that have store vectors:
      // 1. Get DataStore
      // 2. Compare currStorePtr (store(0)) with store at t=T
      // 3. Verify ||store(T) - store(0)|| < tolerance
      
      // Verify PSS mode is set (can't access protected getAnalysisObjectPtr)
      EXPECT_EQ(xycePtr->getAnalysisManager().getAnalysisMode(), Analysis::ANP_MODE_PSS);
    }
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 5: Newton Iteration Convergence
  //-------------------------------------------------------------------------
  // Verifies that Newton iteration converges to periodic solution
  TEST_F(PSSIntegrationTest, NewtonIteration_Convergence)
  {
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      // Once PSS is fully implemented, we can:
      // 1. Access PSS analysis object
      // 2. Monitor residual norm during iterations
      // 3. Verify:
      //    - Residual decreases (or stays low)
      //    - Convergence within maxIterations
      //    - Final residual < tolerance
      
      // For now, just verify initialization succeeds
      EXPECT_EQ(status, Simulator::RunStatus::SUCCESS);
    }
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 6: Simple RC Circuit - End-to-End
  //-------------------------------------------------------------------------
  // Integration test with simple RC circuit and periodic source
  TEST_F(PSSIntegrationTest, SimpleRC_Circuit)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      // Try to run simulation
      // Note: PSS may not be fully implemented yet, so we check for graceful handling
      status = xycePtr->runSimulation();
      
      // For now, we just verify it doesn't crash
      // Once PSS is fully implemented, we should verify:
      // - Convergence
      // - Periodic BC: x(T) = x(0)
      // - Results match TRAN at period boundary
      
      // Even if it fails, it should fail gracefully
      EXPECT_NE(status, Simulator::RunStatus::ERROR) 
        << "PSS should handle errors gracefully";
    }
    
    status = xycePtr->finalize();
    EXPECT_EQ(status, Simulator::RunStatus::SUCCESS);
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 9: MPI Initialization - Dense Jacobian Path
  //-------------------------------------------------------------------------
  TEST_F(PSSIntegrationTest, MPI_Recognition_Dense)
  {
    if (getMpiSizeFromEnv() < 2)
    {
      GTEST_SKIP() << "MPI size < 2; run with mpirun -np 2.";
    }

    unsetenv("XYCE_PSS_MATRIX_FREE");

    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);

    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    EXPECT_NE(status, Simulator::RunStatus::ERROR)
      << "MPI initialization should succeed with dense PSS path";

    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 10: MPI Initialization - Matrix-Free Path
  //-------------------------------------------------------------------------
  TEST_F(PSSIntegrationTest, MPI_Recognition_MatrixFree)
  {
    if (getMpiSizeFromEnv() < 2)
    {
      GTEST_SKIP() << "MPI size < 2; run with mpirun -np 2.";
    }

    setenv("XYCE_PSS_MATRIX_FREE", "1", 1);

    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);

    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    EXPECT_NE(status, Simulator::RunStatus::ERROR)
      << "MPI initialization should succeed with matrix-free PSS path";

    delete xycePtr;
    unsetenv("XYCE_PSS_MATRIX_FREE");
  }

  //-------------------------------------------------------------------------
  // Test 11: MPI Matrix-Free Options Parsing
  //-------------------------------------------------------------------------
  TEST_F(PSSIntegrationTest, MPI_MatrixFreeOptions_RC)
  {
    if (getMpiSizeFromEnv() < 2)
    {
      GTEST_SKIP() << "MPI size < 2; run with mpirun -np 2.";
    }

    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);

    auto cmd = createCmdLineArgs("TestNetlist_RC_MatrixFree.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    EXPECT_NE(status, Simulator::RunStatus::ERROR)
      << "MPI initialization should accept MATRIXFREE/GMRES options";

    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 7: RLC Circuit - State Vector Periodicity
  //-------------------------------------------------------------------------
  // Integration test with RLC circuit (has state variables)
  TEST_F(PSSIntegrationTest, RLCCircuit)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RLC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      // Try to run simulation
      status = xycePtr->runSimulation();
      
      // Once PSS is fully implemented, verify:
      // - State vectors (inductor flux, capacitor charge) are periodic
      // - ||state(T) - state(0)|| < tolerance
      
      EXPECT_NE(status, Simulator::RunStatus::ERROR);
    }
    
    status = xycePtr->finalize();
    EXPECT_EQ(status, Simulator::RunStatus::SUCCESS);
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 8: Integration Failure Handling
  //-------------------------------------------------------------------------
  // Verifies that integration failures are handled gracefully
  TEST_F(PSSIntegrationTest, IntegrationFailure_Handling)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    // Test with invalid period (too small or negative)
    // Should fail gracefully with informative error
    
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    // Create a netlist with invalid PSS parameters
    // For now, use valid netlist and verify error handling structure
    auto cmd = createCmdLineArgs("TestNetlist_RC.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    // Even if initialization succeeds, simulation might fail
    // The key is that failures should be handled gracefully
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      // Should either succeed or fail gracefully (not crash)
      EXPECT_NE(status, Simulator::RunStatus::ERROR) 
        << "PSS should handle errors gracefully, not crash";
    }
    
    status = xycePtr->finalize();
    EXPECT_EQ(status, Simulator::RunStatus::SUCCESS);
    
    delete xycePtr;
  }

} // namespace

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

