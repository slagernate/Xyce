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

#include <gtest/gtest.h>
#include "PSSShootingMethod.h"
#include "SimpleODETest.h"
#include <vector>
#include <cmath>
#include <memory>

// C++11 compatibility: make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace
{
  using namespace Xyce::Test;

  // Test fixture for shooting method tests
  class ShootingMethodTest : public ::testing::Test
  {
  protected:
    void SetUp() override
    {
      // Default setup
    }
    
    void TearDown() override
    {
      // Cleanup if needed
    }
  };

  //-------------------------------------------------------------------------
  // Test A: Driven PSS - Simple ODE
  //-------------------------------------------------------------------------
  TEST_F(ShootingMethodTest, DrivenSimpleODE_Euler)
  {
    double period = 1.0;
    DrivenSimpleODE ode(period);
    
    auto integrator = make_unique<EulerIntegrator>(1000);
    PSSShootingMethod shooting(std::move(integrator));
    
    PSSShootingMethod::Parameters params;
    params.mode = PSSShootingMethod::DRIVEN;
    params.period = period;
    params.tolerance = 1e-5;
    params.maxIterations = 20;
    shooting.setParameters(params);
    
    // Initial guess
    std::vector<double> x0 = {0.0};
    
    // Solve
    auto result = shooting.solve(ode, x0);
    
    // Verify convergence
    EXPECT_TRUE(result.converged) << "Shooting method did not converge";
    EXPECT_LT(result.residualNorm, params.tolerance) 
      << "Residual norm too large: " << result.residualNorm;
    EXPECT_LT(result.iterations, params.maxIterations)
      << "Too many iterations: " << result.iterations;
    
    // Verify periodicity: x(T) should equal x(0)
    std::vector<double> xT(1);
    EulerIntegrator integratorCheck(1000);
    integratorCheck.integrate(ode, result.initialCondition, 0.0, period, xT);
    
    double periodicityError = std::abs(xT[0] - result.initialCondition[0]);
    EXPECT_LT(periodicityError, params.tolerance)
      << "Periodicity not satisfied. Error: " << periodicityError;
  }

  TEST_F(ShootingMethodTest, DrivenSimpleODE_RK4)
  {
    double period = 1.0;
    DrivenSimpleODE ode(period);
    
    auto integrator = make_unique<RK4Integrator>(500);
    PSSShootingMethod shooting(std::move(integrator));
    
    PSSShootingMethod::Parameters params;
    params.mode = PSSShootingMethod::DRIVEN;
    params.period = period;
    params.tolerance = 1e-6;
    params.maxIterations = 20;
    shooting.setParameters(params);
    
    std::vector<double> x0 = {0.0};
    auto result = shooting.solve(ode, x0);
    
    EXPECT_TRUE(result.converged);
    EXPECT_LT(result.residualNorm, params.tolerance);
    
    // Compare with analytical solution
    if (ode.hasAnalyticalSolution()) {
      std::vector<double> xAnalytical(1);
      ode.analyticalSolution(0.0, xAnalytical);
      
      double error = std::abs(result.initialCondition[0] - xAnalytical[0]);
      EXPECT_LT(error, 1e-4) << "Solution differs from analytical. Error: " << error;
    }
  }

  //-------------------------------------------------------------------------
  // Test B: Autonomous PSS - Simple ODE
  //-------------------------------------------------------------------------
  TEST_F(ShootingMethodTest, AutonomousSimpleODE)
  {
    // For now, autonomous mode is not fully implemented
    // This test will be expanded later
    VanDerPolODE ode(1.0);
    
    auto integrator = std::make_unique<RK4Integrator>(1000);
    PSSShootingMethod shooting(std::move(integrator));
    
    PSSShootingMethod::Parameters params;
    params.mode = PSSShootingMethod::AUTONOMOUS;
    params.period = 6.0;  // Initial guess
    params.tolerance = 1e-5;
    params.maxIterations = 20;
    shooting.setParameters(params);
    
    std::vector<double> x0 = {2.0, 0.0};
    
    // This test will fail until autonomous mode is implemented
    // For now, just verify the structure works
    auto result = shooting.solve(ode, x0);
    
    // TODO: Add proper autonomous mode tests when implemented
  }

  //-------------------------------------------------------------------------
  // Test C: Jacobian Computation
  //-------------------------------------------------------------------------
  TEST_F(ShootingMethodTest, JacobianComputation)
  {
    double period = 1.0;
    DrivenSimpleODE ode(period);
    
    auto integrator = make_unique<RK4Integrator>(500);
    PSSShootingMethod shooting(std::move(integrator));
    
    PSSShootingMethod::Parameters params;
    params.mode = PSSShootingMethod::DRIVEN;
    params.period = period;
    shooting.setParameters(params);
    
    std::vector<double> x0 = {0.5};
    
    // Compute residual
    std::vector<double> residual(1);
    shooting.computeResidual(ode, x0, residual);
    
    // Compute Jacobian
    std::vector<std::vector<double> > jacobian;
    shooting.computeJacobian(ode, x0, jacobian);
    
    // Verify Jacobian is 1x1
    EXPECT_EQ(jacobian.size(), 1);
    EXPECT_EQ(jacobian[0].size(), 1);
    
    // Verify Jacobian is reasonable (not NaN, not Inf)
    EXPECT_TRUE(std::isfinite(jacobian[0][0]))
      << "Jacobian contains non-finite value";
    
    // For this simple ODE, we can verify the Jacobian makes sense
    // The Jacobian should be approximately d/dx0 (x(T) - x(0))
    // For small perturbations, this should be close to the identity minus something
  }

  //-------------------------------------------------------------------------
  // Test D: Convergence Robustness
  //-------------------------------------------------------------------------
  TEST_F(ShootingMethodTest, ConvergenceWithPoorInitialGuess)
  {
    double period = 1.0;
    DrivenSimpleODE ode(period);
    
    auto integrator = make_unique<RK4Integrator>(500);
    PSSShootingMethod shooting(std::move(integrator));
    
    PSSShootingMethod::Parameters params;
    params.mode = PSSShootingMethod::DRIVEN;
    params.period = period;
    params.tolerance = 1e-5;
    params.maxIterations = 30;  // More iterations for poor guess
    shooting.setParameters(params);
    
    // Try with a poor initial guess
    std::vector<double> x0 = {10.0};  // Far from solution
    
    auto result = shooting.solve(ode, x0);
    
    // Should still converge, but may take more iterations
    EXPECT_TRUE(result.converged) 
      << "Should converge even with poor initial guess";
    EXPECT_LT(result.residualNorm, params.tolerance);
  }

  TEST_F(ShootingMethodTest, ConvergenceWithDifferentTolerances)
  {
    double period = 1.0;
    DrivenSimpleODE ode(period);
    
    std::vector<double> tolerances = {1e-4, 1e-5, 1e-6};
    
    for (double tol : tolerances) {
      auto integrator = make_unique<RK4Integrator>(500);
      PSSShootingMethod shooting(std::move(integrator));
      
      PSSShootingMethod::Parameters params;
      params.mode = PSSShootingMethod::DRIVEN;
      params.period = period;
      params.tolerance = tol;
      params.maxIterations = 20;
      shooting.setParameters(params);
      
      std::vector<double> x0 = {0.0};
      auto result = shooting.solve(ode, x0);
      
      EXPECT_TRUE(result.converged) 
        << "Should converge with tolerance " << tol;
      EXPECT_LT(result.residualNorm, tol)
        << "Residual should be less than tolerance " << tol;
    }
  }

  //-------------------------------------------------------------------------
  // Test E: Phase Condition (for autonomous mode)
  //-------------------------------------------------------------------------
  TEST_F(ShootingMethodTest, PhaseCondition)
  {
    VanDerPolODE ode(1.0);
    
    auto integrator = make_unique<RK4Integrator>(500);
    PSSShootingMethod shooting(std::move(integrator));
    
    PSSShootingMethod::Parameters params;
    params.mode = PSSShootingMethod::AUTONOMOUS;
    params.refNode = 0;  // Use first state variable
    shooting.setParameters(params);
    
    // Test phase condition computation
    std::vector<double> x0 = {2.0, 0.0};
    double phase = shooting.computePhaseCondition(ode, x0);
    
    // Phase condition should be finite
    EXPECT_TRUE(std::isfinite(phase))
      << "Phase condition should be finite";
    
    // For Van der Pol at x=[2,0], dx/dt = [0, -2], so phase should be 0
    // (first component of dx/dt)
    std::vector<double> dxdt(2);
    ode.evaluate(x0, 0.0, dxdt);
    EXPECT_NEAR(phase, dxdt[0], 1e-10)
      << "Phase condition should equal dx/dt at reference node";
  }

} // namespace

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

