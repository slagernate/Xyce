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

#include "PSSShootingMethod.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Xyce {
namespace Test {

//-------------------------------------------------------------------------
// Euler Integrator Implementation
//-------------------------------------------------------------------------
void EulerIntegrator::integrate(
  SimpleODE& ode,
  const std::vector<double>& x0,
  double t0,
  double t1,
  std::vector<double>& x1)
{
  int dim = ode.dimension();
  x1 = x0;
  std::vector<double> dxdt(dim);
  
  double dt = (t1 - t0) / numSteps_;
  double t = t0;
  
  for (int i = 0; i < numSteps_; ++i) {
    ode.evaluate(x1, t, dxdt);
    for (int j = 0; j < dim; ++j) {
      x1[j] += dt * dxdt[j];
    }
    t += dt;
  }
}

//-------------------------------------------------------------------------
// RK4 Integrator Implementation
//-------------------------------------------------------------------------
void RK4Integrator::integrate(
  SimpleODE& ode,
  const std::vector<double>& x0,
  double t0,
  double t1,
  std::vector<double>& x1)
{
  int dim = ode.dimension();
  x1 = x0;
  std::vector<double> k1(dim), k2(dim), k3(dim), k4(dim);
  std::vector<double> temp(dim);
  
  double dt = (t1 - t0) / numSteps_;
  double t = t0;
  
  for (int i = 0; i < numSteps_; ++i) {
    // k1 = f(x, t)
    ode.evaluate(x1, t, k1);
    
    // k2 = f(x + dt*k1/2, t + dt/2)
    for (int j = 0; j < dim; ++j) {
      temp[j] = x1[j] + 0.5 * dt * k1[j];
    }
    ode.evaluate(temp, t + 0.5 * dt, k2);
    
    // k3 = f(x + dt*k2/2, t + dt/2)
    for (int j = 0; j < dim; ++j) {
      temp[j] = x1[j] + 0.5 * dt * k2[j];
    }
    ode.evaluate(temp, t + 0.5 * dt, k3);
    
    // k4 = f(x + dt*k3, t + dt)
    for (int j = 0; j < dim; ++j) {
      temp[j] = x1[j] + dt * k3[j];
    }
    ode.evaluate(temp, t + dt, k4);
    
    // x = x + dt*(k1 + 2*k2 + 2*k3 + k4)/6
    for (int j = 0; j < dim; ++j) {
      x1[j] += dt * (k1[j] + 2.0 * k2[j] + 2.0 * k3[j] + k4[j]) / 6.0;
    }
    
    t += dt;
  }
}

//-------------------------------------------------------------------------
// PSS Shooting Method Implementation
//-------------------------------------------------------------------------
PSSShootingMethod::PSSShootingMethod(std::unique_ptr<TimeIntegrator> integrator)
  : integrator_(std::move(integrator))
{
}

PSSShootingMethod::Result PSSShootingMethod::solve(
  SimpleODE& ode,
  const std::vector<double>& initialGuess)
{
  lastResult_ = Result();
  lastResult_.initialCondition = initialGuess;
  lastResult_.period = params_.period;
  
  int dim = ode.dimension();
  std::vector<double> x0 = initialGuess;
  
  for (int iter = 0; iter < params_.maxIterations; ++iter) {
    lastResult_.iterations = iter + 1;
    
    if (params_.mode == DRIVEN) {
      // Driven mode: solve x(T) = x(0)
      if (!newtonStep(ode, x0)) {
        break;
      }
      
      // Check convergence
      std::vector<double> residual(dim);
      computeResidual(ode, x0, residual);
      
      lastResult_.residualNorm = 0.0;
      for (int i = 0; i < dim; ++i) {
        lastResult_.residualNorm += residual[i] * residual[i];
      }
      lastResult_.residualNorm = std::sqrt(lastResult_.residualNorm);
      
      if (lastResult_.residualNorm < params_.tolerance) {
        lastResult_.converged = true;
        break;
      }
    } else {
      // Autonomous mode: solve [x(T) - x(0), phase] = 0
      // This is more complex and will be implemented later
      // For now, just return not converged
      break;
    }
  }
  
  lastResult_.initialCondition = x0;
  return lastResult_;
}

void PSSShootingMethod::computeResidual(
  SimpleODE& ode,
  const std::vector<double>& x0,
  std::vector<double>& residual)
{
  int dim = ode.dimension();
  std::vector<double> xT(dim);
  
  // Integrate from 0 to T
  integrator_->integrate(ode, x0, 0.0, params_.period, xT);
  
  // Residual: r = x(T) - x(0)
  for (int i = 0; i < dim; ++i) {
    residual[i] = xT[i] - x0[i];
  }
}

double PSSShootingMethod::computePhaseCondition(
  SimpleODE& ode,
  const std::vector<double>& x0)
{
  // Phase condition: dV/dt = 0 at reference node
  // For ODE dx/dt = f(x, t), this is f(x, 0)[refNode] = 0
  std::vector<double> dxdt(ode.dimension());
  ode.evaluate(x0, 0.0, dxdt);
  return dxdt[params_.refNode];
}

void PSSShootingMethod::computeJacobian(
  SimpleODE& ode,
  const std::vector<double>& x0,
  std::vector<std::vector<double> >& jacobian)
{
  int dim = ode.dimension();
  jacobian.resize(dim);
  for (int i = 0; i < dim; ++i) {
    jacobian[i].resize(dim);
  }
  
  std::vector<double> residual0(dim);
  computeResidual(ode, x0, residual0);
  
  // Finite difference: J[i][j] = (r_i(x0 + h*e_j) - r_i(x0)) / h
  std::vector<double> xPert = x0;
  for (int j = 0; j < dim; ++j) {
    xPert[j] += fdStepSize_;
    std::vector<double> residualPert(dim);
    computeResidual(ode, xPert, residualPert);
    
    for (int i = 0; i < dim; ++i) {
      jacobian[i][j] = (residualPert[i] - residual0[i]) / fdStepSize_;
    }
    
    xPert[j] = x0[j];  // Reset
  }
}

void PSSShootingMethod::computeJacobianAutonomous(
  SimpleODE& ode,
  const std::vector<double>& x0,
  double period,
  std::vector<std::vector<double> >& jacobian)
{
  // For autonomous mode, Jacobian includes derivatives w.r.t. period
  // This will be implemented later
  (void)ode;
  (void)x0;
  (void)period;
  (void)jacobian;
}

bool PSSShootingMethod::newtonStep(
  SimpleODE& ode,
  std::vector<double>& x0)
{
  int dim = ode.dimension();
  
  // Compute residual
  std::vector<double> residual(dim);
  computeResidual(ode, x0, residual);
  
  // Compute Jacobian
  std::vector<std::vector<double> > jacobian;
  computeJacobian(ode, x0, jacobian);
  
  // Solve J * delta = -residual
  // Simple Gaussian elimination for small systems
  // For larger systems, would use a proper linear solver
  
  // Create augmented matrix [J | -r]
  std::vector<std::vector<double> > aug(dim);
  for (int i = 0; i < dim; ++i) {
    aug[i].resize(dim + 1);
    for (int j = 0; j < dim; ++j) {
      aug[i][j] = jacobian[i][j];
    }
    aug[i][dim] = -residual[i];
  }
  
  // Gaussian elimination
  for (int i = 0; i < dim; ++i) {
    // Find pivot
    int pivot = i;
    for (int k = i + 1; k < dim; ++k) {
      if (std::abs(aug[k][i]) > std::abs(aug[pivot][i])) {
        pivot = k;
      }
    }
    std::swap(aug[i], aug[pivot]);
    
    // Eliminate
    double pivotVal = aug[i][i];
    if (std::abs(pivotVal) < 1e-12) {
      return false;  // Singular matrix
    }
    
    for (int k = i + 1; k < dim; ++k) {
      double factor = aug[k][i] / pivotVal;
      for (int j = i; j <= dim; ++j) {
        aug[k][j] -= factor * aug[i][j];
      }
    }
  }
  
  // Back substitution
  std::vector<double> delta(dim);
  for (int i = dim - 1; i >= 0; --i) {
    delta[i] = aug[i][dim];
    for (int j = i + 1; j < dim; ++j) {
      delta[i] -= aug[i][j] * delta[j];
    }
    delta[i] /= aug[i][i];
  }
  
  // Update: x0 = x0 + delta
  for (int i = 0; i < dim; ++i) {
    x0[i] += delta[i];
  }
  
  return true;
}

} // namespace Test
} // namespace Xyce

