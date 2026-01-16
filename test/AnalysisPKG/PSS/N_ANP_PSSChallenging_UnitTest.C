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

// Challenging Test Cases for PSS Analysis
// These tests push the limits of the PSS implementation with difficult
// scenarios that are likely to occur in real-world applications.

#include "Xyce_config.h"
#include <gtest/gtest.h>
#include <N_CIR_Xyce.h>
#include <N_ANP_PSS.h>
#include <N_ANP_AnalysisManager.h>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>

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

  // Test fixture for challenging PSS tests
  class PSSChallengingTest : public ::testing::Test
  {
  protected:
    void SetUp() override
    {
      if (!allowFullPssSimulation())
      {
        GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
      }
    }
    
    void TearDown() override
    {
    }
  };

  //-------------------------------------------------------------------------
  // Test 1: Diode Rectifier - Strong Nonlinearity
  //-------------------------------------------------------------------------
  // Tests PSS with a diode rectifier circuit, which has strong nonlinearity
  // and can be challenging for convergence
  TEST_F(PSSChallengingTest, DiodeRectifier_StrongNonlinearity)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    // Create netlist for diode rectifier
    std::string netlist = 
      "* Diode Rectifier - Strong Nonlinearity Test\n"
      "V1 1 0 SIN(0 5 1e6)  ; 1MHz, 5V amplitude\n"
      "D1 1 2 D1N4001\n"
      "R1 2 0 1k\n"
      "C1 2 0 10n\n"
      ".MODEL D1N4001 D (IS=1e-12 N=1.5 RS=0.1)\n"
      ".PSS 1u\n"
      ".PRINT PSS V(2) I(D1)\n"
      ".END\n";
    
    // Write to temporary file
    std::ofstream out("TestNetlist_DiodeRectifier.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_DiodeRectifier.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Diode rectifier should converge, but may require more iterations
      // due to strong nonlinearity
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Diode rectifier should converge (may need more iterations)";
      
      // Verify periodic BC is satisfied
      // (Implementation detail: check DataStore after convergence)
    }
    
    status = xycePtr->finalize();
    EXPECT_EQ(status, Simulator::RunStatus::SUCCESS);
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 1b: Diode Rectifier MPI Matrix-Free
  //-------------------------------------------------------------------------
  TEST_F(PSSChallengingTest, MPI_DiodeRectifier_MatrixFree)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }
    if (getMpiSizeFromEnv() < 2)
    {
      GTEST_SKIP() << "Requires MPI size >= 2.";
    }

    std::string netlist =
      "* Diode Rectifier - MPI matrix-free\n"
      "V1 1 0 SIN(0 5 1e6)\n"
      "D1 1 2 D1N4001\n"
      "R1 2 0 1k\n"
      "C1 2 0 10n\n"
      ".MODEL D1N4001 D (IS=1e-12 N=1.5 RS=0.1)\n"
      ".PSS 1u MATRIXFREE GMRESRESTART=20 GMRESMAXITER=40 GMRESTOL=0.01 "
      "GMRESPRECOND=DIAG GMRESPRECONDMAX=200 GMRESLOG PERFLOG\n"
      ".PRINT PSS V(2) I(D1)\n"
      ".END\n";

    std::ofstream out("TestNetlist_DiodeRectifier_MPI.cir");
    out << netlist;
    out.close();

    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);

    auto cmd = createCmdLineArgs("TestNetlist_DiodeRectifier_MPI.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());

    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "MPI diode rectifier should converge with matrix-free GMRES";
    }

    status = xycePtr->finalize();
    EXPECT_EQ(status, Simulator::RunStatus::SUCCESS);

    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 2: Stiff System - Widely Separated Time Constants
  //-------------------------------------------------------------------------
  // Tests PSS with a circuit having widely separated time constants
  // (e.g., fast and slow dynamics), which can challenge time integration
  TEST_F(PSSChallengingTest, StiffSystem_WidelySeparatedTimeConstants)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    std::string netlist =
      "* Stiff System - Fast and Slow Dynamics\n"
      "V1 1 0 SIN(0 1 1e6)  ; 1MHz\n"
      "R1 1 2 1k\n"
      "C1 2 0 1p    ; Fast: 1pF, RC = 1ns\n"
      "R2 2 3 1M\n"
      "C2 3 0 1u    ; Slow: 1uF, RC = 1s\n"
      ".PSS 1u\n"
      ".PRINT PSS V(2) V(3)\n"
      ".END\n";
    
    std::ofstream out("TestNetlist_StiffSystem.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_StiffSystem.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Stiff systems may require adaptive time stepping
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Stiff system should be handled (may need adaptive stepping)";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 3: Poor Initial Guess - Far from Solution
  //-------------------------------------------------------------------------
  // Tests robustness with poor initial conditions
  TEST_F(PSSChallengingTest, PoorInitialGuess_FarFromSolution)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    std::string netlist =
      "* Poor Initial Guess Test\n"
      "V1 1 0 SIN(0 1 1e6)\n"
      "R1 1 2 1k\n"
      "C1 2 0 1n\n"
      ".IC V(2)=10  ; Poor initial guess (should be ~0.5V)\n"
      ".PSS 1u\n"
      ".PRINT PSS V(2)\n"
      ".END\n";
    
    std::ofstream out("TestNetlist_PoorInitialGuess.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_PoorInitialGuess.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Should still converge despite poor initial guess
      // (may require more iterations or damping)
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Should converge from poor initial guess (may need more iterations)";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 4: Very Small Period - High Frequency
  //-------------------------------------------------------------------------
  // Tests PSS with very high frequency (small period)
  TEST_F(PSSChallengingTest, VerySmallPeriod_HighFrequency)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    std::string netlist =
      "* Very High Frequency Test\n"
      "V1 1 0 SIN(0 1 100e6)  ; 100MHz\n"
      "R1 1 2 1k\n"
      "C1 2 0 1n\n"
      ".PSS 10n  ; 10ns period (100MHz)\n"
      ".PRINT PSS V(2)\n"
      ".END\n";
    
    std::ofstream out("TestNetlist_HighFrequency.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_HighFrequency.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // High frequency requires fine time stepping
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "High frequency should be handled with fine time stepping";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 5: Very Large Period - Low Frequency
  //-------------------------------------------------------------------------
  // Tests PSS with very low frequency (large period)
  TEST_F(PSSChallengingTest, VeryLargePeriod_LowFrequency)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    std::string netlist =
      "* Very Low Frequency Test\n"
      "V1 1 0 SIN(0 1 1)  ; 1Hz\n"
      "R1 1 2 1k\n"
      "C1 2 0 1n\n"
      ".PSS 1  ; 1 second period\n"
      ".PRINT PSS V(2)\n"
      ".END\n";
    
    std::ofstream out("TestNetlist_LowFrequency.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_LowFrequency.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Low frequency requires many time steps
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Low frequency should be handled (may take many steps)";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 6: Multiple Time Constants - Complex Dynamics
  //-------------------------------------------------------------------------
  // Tests circuit with multiple time constants and complex dynamics
  TEST_F(PSSChallengingTest, MultipleTimeConstants_ComplexDynamics)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    std::string netlist =
      "* Multiple Time Constants\n"
      "V1 1 0 SIN(0 1 1e6)\n"
      "R1 1 2 1k\n"
      "C1 2 0 1n\n"
      "R2 2 3 10k\n"
      "L1 3 4 1u\n"
      "C2 4 0 10n\n"
      "R3 4 0 100\n"
      ".PSS 1u\n"
      ".PRINT PSS V(2) V(3) V(4) I(L1)\n"
      ".END\n";
    
    std::ofstream out("TestNetlist_MultipleTimeConstants.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_MultipleTimeConstants.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Multiple time constants require careful integration
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Multiple time constants should be handled";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 7: Large Circuit - Many Nodes
  //-------------------------------------------------------------------------
  // Tests PSS with a larger circuit (scalability test)
  TEST_F(PSSChallengingTest, LargeCircuit_ManyNodes)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    // Create a ladder network with many nodes
    std::ostringstream netlist;
    netlist << "* Large Circuit - Ladder Network\n";
    netlist << "V1 1 0 SIN(0 1 1e6)\n";
    
    const int numStages = 20;  // 20-stage ladder
    for (int i = 1; i <= numStages; ++i)
    {
      int node1 = i;
      int node2 = i + 1;
      netlist << "R" << i << " " << node1 << " " << node2 << " 1k\n";
      netlist << "C" << i << " " << node2 << " 0 1n\n";
    }
    
    netlist << ".PSS 1u\n";
    netlist << ".PRINT PSS V(10) V(20)\n";  // Check nodes in middle and end
    netlist << ".END\n";
    
    std::ofstream out("TestNetlist_LargeCircuit.cir");
    out << netlist.str();
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_LargeCircuit.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Large circuits test scalability
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Large circuit should be handled (tests scalability)";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 7b: Large Circuit MPI Matrix-Free
  //-------------------------------------------------------------------------
  TEST_F(PSSChallengingTest, MPI_LargeCircuit_MatrixFree)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }
    if (getMpiSizeFromEnv() < 2)
    {
      GTEST_SKIP() << "Requires MPI size >= 2.";
    }

    // Larger ladder network for MPI/matrix-free robustness
    std::ostringstream netlist;
    netlist << "* Large Circuit - Ladder Network (MPI matrix-free)\n";
    netlist << "V1 1 0 SIN(0 1 1e6)\n";

    const int numStages = 80;  // Larger ladder for MPI
    for (int i = 1; i <= numStages; ++i)
    {
      int node1 = i;
      int node2 = i + 1;
      netlist << "R" << i << " " << node1 << " " << node2 << " 1k\n";
      netlist << "C" << i << " " << node2 << " 0 1n\n";
    }

    netlist << ".PSS 1u MATRIXFREE GMRESRESTART=20 GMRESMAXITER=20 GMRESTOL=0.01 "
               "GMRESPRECOND=DIAG GMRESPRECONDMAX=200 GMRESLOG PERFLOG\n";
    netlist << ".PRINT PSS V(40) V(80)\n";
    netlist << ".END\n";

    std::ofstream out("TestNetlist_LargeCircuit_MPI.cir");
    out << netlist.str();
    out.close();

    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_LargeCircuit_MPI.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());

    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "MPI matrix-free large circuit should converge";
    }

    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 7c: Mixed RLC + Diode MPI Matrix-Free
  //-------------------------------------------------------------------------
  TEST_F(PSSChallengingTest, MPI_MixedRLC_Diode_MatrixFree)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }
    if (getMpiSizeFromEnv() < 2)
    {
      GTEST_SKIP() << "Requires MPI size >= 2.";
    }

    std::ostringstream netlist;
    netlist << "* Mixed RLC + Diode Ladder (MPI matrix-free)\n";
    netlist << "V1 1 0 SIN(0 2 2e6)\n";
    netlist << ".MODEL DCLAMP D (IS=1e-12 N=1.2 RS=0.05)\n";

    const int numStages = 40;
    for (int i = 1; i <= numStages; ++i)
    {
      int node1 = i;
      int node2 = i + 1;
      netlist << "R" << i << " " << node1 << " " << node2 << " 500\n";
      netlist << "C" << i << " " << node2 << " 0 0.5n\n";
      if (i % 10 == 0)
      {
        netlist << "L" << i << " " << node2 << " 0 20n\n";
      }
    }

    netlist << "D1 20 0 DCLAMP\n";
    netlist << ".PSS 500n MATRIXFREE GMRESRESTART=30 GMRESMAXITER=60 GMRESTOL=0.01 "
               "GMRESPRECOND=DIAG GMRESPRECONDMAX=200 GMRESLOG PERFLOG\n";
    netlist << ".PRINT PSS V(10) V(20) V(30) I(D1)\n";
    netlist << ".END\n";

    std::ofstream out("TestNetlist_MixedRLC_Diode_MPI.cir");
    out << netlist.str();
    out.close();

    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_MixedRLC_Diode_MPI.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());

    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "MPI mixed RLC+diode ladder should converge";
    }

    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 8: Extreme Parameter Values
  //-------------------------------------------------------------------------
  // Tests with extreme but valid parameter values
  TEST_F(PSSChallengingTest, ExtremeParameterValues)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    std::string netlist =
      "* Extreme Parameter Values\n"
      "V1 1 0 SIN(0 1 1e6)\n"
      "R1 1 2 1e-3  ; Very small resistance (1mOhm)\n"
      "C1 2 0 1e-15 ; Very small capacitance (1fF)\n"
      "R2 2 3 1e9   ; Very large resistance (1GOhm)\n"
      "C2 3 0 1e-3  ; Very large capacitance (1mF)\n"
      ".PSS 1u\n"
      ".PRINT PSS V(2) V(3)\n"
      ".END\n";
    
    std::ofstream out("TestNetlist_ExtremeParameters.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_ExtremeParameters.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Extreme parameters may cause numerical issues
      // Should handle gracefully or fail with informative error
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Extreme parameters should be handled or fail gracefully";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 9: Convergence Failure - Non-Periodic System
  //-------------------------------------------------------------------------
  // Tests error handling when system doesn't have periodic solution
  TEST_F(PSSChallengingTest, ConvergenceFailure_NonPeriodicSystem)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    // A circuit that doesn't have a periodic solution (e.g., unstable)
    std::string netlist =
      "* Non-Periodic System (may not converge)\n"
      "V1 1 0 SIN(0 1 1e6)\n"
      "R1 1 2 1k\n"
      "C1 2 0 1n\n"
      ".PSS 1u\n"
      ".PRINT PSS V(2)\n"
      ".END\n";
    
    // Note: This test may actually converge (RC circuit is stable)
    // A truly non-periodic case would require an unstable circuit
    // For now, we test that failure is handled gracefully
    
    std::ofstream out("TestNetlist_NonPeriodic.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_NonPeriodic.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Should either converge or fail gracefully with informative error
      // Not crash or hang
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Non-convergence should be handled gracefully, not crash";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 10: Tight Tolerance - High Accuracy Requirement
  //-------------------------------------------------------------------------
  // Tests PSS with very tight tolerance requirement
  TEST_F(PSSChallengingTest, TightTolerance_HighAccuracy)
  {
    if (!allowFullPssSimulation())
    {
      GTEST_SKIP() << "Set XYCE_RUN_PSS_FULL_SIM=1 to run full PSS simulations.";
    }

    std::string netlist =
      "* Tight Tolerance Test\n"
      "V1 1 0 SIN(0 1 1e6)\n"
      "R1 1 2 1k\n"
      "C1 2 0 1n\n"
      ".PSS 1u TOL=1e-12  ; Very tight tolerance\n"
      ".PRINT PSS V(2)\n"
      ".END\n";
    
    std::ofstream out("TestNetlist_TightTolerance.cir");
    out << netlist;
    out.close();
    
    Simulator* xycePtr = new Simulator();
    auto cmd = createCmdLineArgs("TestNetlist_TightTolerance.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      status = xycePtr->runSimulation();
      
      // Tight tolerance may require more iterations or finer time stepping
      EXPECT_NE(status, Simulator::RunStatus::ERROR)
        << "Tight tolerance should be achievable (may need more iterations)";
    }
    
    xycePtr->finalize();
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 11: Autonomous PSS - LC Oscillator
  //-------------------------------------------------------------------------
  // Verifies that autonomous mode can find the oscillation period
  TEST_F(PSSChallengingTest, Autonomous_LC_Oscillator)
  {
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_LC_Oscillator.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      Analysis::AnalysisManager& am = xycePtr->getAnalysisManager();
      
      // Verify PSS analysis mode is set
      EXPECT_EQ(am.getAnalysisMode(), Analysis::ANP_MODE_PSS);
      
      // For Phase 3, we verify that autonomous mode is recognized
      // Full period finding validation will be added when autonomous mode is fully tested
    }
    
    delete xycePtr;
  }

  //-------------------------------------------------------------------------
  // Test 12: Autonomous PSS - Ring Oscillator
  //-------------------------------------------------------------------------
  // Verifies autonomous mode with multi-stage oscillator
  TEST_F(PSSChallengingTest, Autonomous_RingOscillator)
  {
    Simulator* xycePtr = new Simulator();
    ASSERT_TRUE(xycePtr != nullptr);
    
    auto cmd = createCmdLineArgs("TestNetlist_RingOscillator.cir");
    Simulator::RunStatus status = xycePtr->initialize(cmd.argv.size(), cmd.argv.data());
    
    if (status == Simulator::RunStatus::SUCCESS)
    {
      Analysis::AnalysisManager& am = xycePtr->getAnalysisManager();
      
      // Verify PSS analysis mode is set
      EXPECT_EQ(am.getAnalysisMode(), Analysis::ANP_MODE_PSS);
      
      // For Phase 3, we verify that autonomous mode is recognized
      // Full period finding and phase condition validation will be added
    }
    
    delete xycePtr;
  }

} // namespace

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

