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

#ifndef Xyce_SimpleODETest_h
#define Xyce_SimpleODETest_h

#include <vector>
#include <functional>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Xyce {
namespace Test {

// Simple ODE interface for testing shooting method
// dx/dt = f(x, t)
class SimpleODE
{
public:
  virtual ~SimpleODE() {}
  
  // Evaluate the ODE: f(x, t)
  virtual void evaluate(const std::vector<double>& x, double t, std::vector<double>& dxdt) = 0;
  
  // Get the dimension of the state vector
  virtual int dimension() const = 0;
  
  // Get analytical solution (if available) for comparison
  virtual bool hasAnalyticalSolution() const { return false; }
  virtual void analyticalSolution(double t, std::vector<double>& x) {}
};

// Driven ODE: dx/dt = -x + sin(2πt/T)
// Analytical solution exists for periodic BC
class DrivenSimpleODE : public SimpleODE
{
public:
  DrivenSimpleODE(double period) : period_(period) {}
  
  int dimension() const override { return 1; }
  
  void evaluate(const std::vector<double>& x, double t, std::vector<double>& dxdt) override
  {
    dxdt[0] = -x[0] + std::sin(2.0 * M_PI * t / period_);
  }
  
  bool hasAnalyticalSolution() const override { return true; }
  
  void analyticalSolution(double t, std::vector<double>& x) override
  {
    // Analytical periodic solution for dx/dt = -x + sin(2πt/T)
    // Particular solution: x(t) = P*cos(ωt) + Q*sin(ωt)
    // where ω = 2π/T, Q = 1/(ω²+1), P = -ω/(ω²+1)
    double omega = 2.0 * M_PI / period_;
    double Q = 1.0 / (1.0 + omega * omega);
    double P = -omega / (1.0 + omega * omega);
    x[0] = P * std::cos(omega * t) + Q * std::sin(omega * t);
  }
  
private:
  double period_;
};

// Van der Pol oscillator: dx/dt = y, dy/dt = μ(1-x²)y - x
// Autonomous oscillator with limit cycle
class VanDerPolODE : public SimpleODE
{
public:
  VanDerPolODE(double mu = 1.0) : mu_(mu) {}
  
  int dimension() const override { return 2; }
  
  void evaluate(const std::vector<double>& x, double t, std::vector<double>& dxdt) override
  {
    dxdt[0] = x[1];
    dxdt[1] = mu_ * (1.0 - x[0] * x[0]) * x[1] - x[0];
  }
  
private:
  double mu_;
};

} // namespace Test
} // namespace Xyce

#endif // Xyce_SimpleODETest_h

