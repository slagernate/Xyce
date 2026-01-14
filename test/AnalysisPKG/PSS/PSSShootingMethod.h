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

#ifndef Xyce_PSSShootingMethod_h
#define Xyce_PSSShootingMethod_h

#include "SimpleODETest.h"
#include <vector>
#include <memory>

namespace Xyce {
namespace Test {

// Simple time integrator interface
class TimeIntegrator
{
public:
  virtual ~TimeIntegrator() {}
  
  // Integrate ODE from t0 to t1 with initial condition x0
  // Returns final state at t1
  virtual void integrate(
    SimpleODE& ode,
    const std::vector<double>& x0,
    double t0,
    double t1,
    std::vector<double>& x1) = 0;
};

// Euler time integrator (simple, for testing)
class EulerIntegrator : public TimeIntegrator
{
public:
  EulerIntegrator(int numSteps = 1000) : numSteps_(numSteps) {}
  
  void integrate(
    SimpleODE& ode,
    const std::vector<double>& x0,
    double t0,
    double t1,
    std::vector<double>& x1) override;
  
private:
  int numSteps_;
};

// RK4 time integrator (more accurate)
class RK4Integrator : public TimeIntegrator
{
public:
  RK4Integrator(int numSteps = 1000) : numSteps_(numSteps) {}
  
  void integrate(
    SimpleODE& ode,
    const std::vector<double>& x0,
    double t0,
    double t1,
    std::vector<double>& x1) override;
  
private:
  int numSteps_;
};

// Shooting method for Periodic Steady State
class PSSShootingMethod
{
public:
  enum Mode {
    DRIVEN,      // Fixed period T
    AUTONOMOUS   // Unknown period T
  };
  
  struct Parameters {
    double tolerance = 1e-6;
    int maxIterations = 50;
    Mode mode = DRIVEN;
    double period = 1.0;  // For driven mode, or initial guess for autonomous
    int refNode = 0;      // Reference node for phase condition (autonomous)
  };
  
  struct Result {
    bool converged = false;
    int iterations = 0;
    double residualNorm = 0.0;
    std::vector<double> initialCondition;
    double period = 0.0;  // For autonomous mode
  };
  
  PSSShootingMethod(std::unique_ptr<TimeIntegrator> integrator);
  
  // Solve for periodic steady state
  Result solve(SimpleODE& ode, const std::vector<double>& initialGuess);
  
  // Set parameters
  void setParameters(const Parameters& params) { params_ = params; }
  const Parameters& getParameters() const { return params_; }
  
  // Get last result
  const Result& getLastResult() const { return lastResult_; }
  
  // Test helper methods (public for unit testing)
  // Compute residual: r = x(T) - x(0)
  void computeResidual(
    SimpleODE& ode,
    const std::vector<double>& x0,
    std::vector<double>& residual);
  
  // Compute phase condition: dV/dt = 0 at reference node
  double computePhaseCondition(
    SimpleODE& ode,
    const std::vector<double>& x0);
  
  // Compute Jacobian via finite difference
  void computeJacobian(
    SimpleODE& ode,
    const std::vector<double>& x0,
    std::vector<std::vector<double> >& jacobian);
  
private:
  
  // Compute Jacobian for autonomous mode (includes period derivative)
  void computeJacobianAutonomous(
    SimpleODE& ode,
    const std::vector<double>& x0,
    double period,
    std::vector<std::vector<double> >& jacobian);
  
  // Newton iteration step (driven mode)
  bool newtonStep(
    SimpleODE& ode,
    std::vector<double>& x0);
  
  // Newton iteration step (autonomous mode)
  bool newtonStepAutonomous(
    SimpleODE& ode,
    std::vector<double>& x0);
  
  std::unique_ptr<TimeIntegrator> integrator_;
  Parameters params_;
  Result lastResult_;
  
  // Finite difference step size
  double fdStepSize_ = 1e-8;
};

} // namespace Test
} // namespace Xyce

#endif // Xyce_PSSShootingMethod_h

