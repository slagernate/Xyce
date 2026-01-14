//-------------------------------------------------------------------------
//   Standalone test program (no GTest required)
//   Tests basic shooting method functionality
//-------------------------------------------------------------------------

#include "PSSShootingMethod.h"
#include "SimpleODETest.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <memory>

using namespace Xyce::Test;

// C++11 compatibility: make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

bool testDrivenSimpleODE()
{
  std::cout << "\n=== Test: Driven Simple ODE ===" << std::endl;
  
  double period = 1.0;
  DrivenSimpleODE ode(period);
  
  auto integrator = make_unique<RK4Integrator>(500);
  PSSShootingMethod shooting(std::move(integrator));
  
  PSSShootingMethod::Parameters params;
  params.mode = PSSShootingMethod::DRIVEN;
  params.period = period;
  params.tolerance = 1e-5;
  params.maxIterations = 20;
  shooting.setParameters(params);
  
  std::vector<double> x0 = {0.0};
  auto result = shooting.solve(ode, x0);
  
  std::cout << "Converged: " << (result.converged ? "YES" : "NO") << std::endl;
  std::cout << "Iterations: " << result.iterations << std::endl;
  std::cout << "Residual norm: " << std::scientific << result.residualNorm << std::endl;
  std::cout << "Initial condition: " << std::fixed << result.initialCondition[0] << std::endl;
  
  // Verify periodicity
  std::vector<double> xT(1);
  RK4Integrator integratorCheck(500);
  integratorCheck.integrate(ode, result.initialCondition, 0.0, period, xT);
  double periodicityError = std::abs(xT[0] - result.initialCondition[0]);
  std::cout << "Periodicity error: " << std::scientific << periodicityError << std::endl;
  
  // Compare with analytical solution
  if (ode.hasAnalyticalSolution()) {
    std::vector<double> xAnalytical(1);
    ode.analyticalSolution(0.0, xAnalytical);
    double error = std::abs(result.initialCondition[0] - xAnalytical[0]);
    std::cout << "Analytical solution: " << std::fixed << xAnalytical[0] << std::endl;
    std::cout << "Error vs analytical: " << std::scientific << error << std::endl;
  }
  
  bool passed = result.converged && 
                result.residualNorm < params.tolerance &&
                periodicityError < params.tolerance;
  
  std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
  return passed;
}

bool testJacobianComputation()
{
  std::cout << "\n=== Test: Jacobian Computation ===" << std::endl;
  
  double period = 1.0;
  DrivenSimpleODE ode(period);
  
  auto integrator = make_unique<RK4Integrator>(500);
  PSSShootingMethod shooting(std::move(integrator));
  
  PSSShootingMethod::Parameters params;
  params.mode = PSSShootingMethod::DRIVEN;
  params.period = period;
  shooting.setParameters(params);
  
  std::vector<double> x0 = {0.5};
  
  std::vector<double> residual(1);
  shooting.computeResidual(ode, x0, residual);
  std::cout << "Residual: " << residual[0] << std::endl;
  
  std::vector<std::vector<double> > jacobian;
  shooting.computeJacobian(ode, x0, jacobian);
  
  std::cout << "Jacobian: " << jacobian[0][0] << std::endl;
  std::cout << "Jacobian is finite: " << (std::isfinite(jacobian[0][0]) ? "YES" : "NO") << std::endl;
  
  bool passed = std::isfinite(jacobian[0][0]);
  std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
  return passed;
}

bool testPhaseCondition()
{
  std::cout << "\n=== Test: Phase Condition ===" << std::endl;
  
  VanDerPolODE ode(1.0);
  
  auto integrator = make_unique<RK4Integrator>(500);
  PSSShootingMethod shooting(std::move(integrator));
  
  PSSShootingMethod::Parameters params;
  params.mode = PSSShootingMethod::AUTONOMOUS;
  params.refNode = 0;
  shooting.setParameters(params);
  
  std::vector<double> x0 = {2.0, 0.0};
  double phase = shooting.computePhaseCondition(ode, x0);
  
  std::cout << "Phase condition: " << phase << std::endl;
  
  // Verify it matches dx/dt at reference node
  std::vector<double> dxdt(2);
  ode.evaluate(x0, 0.0, dxdt);
  double expected = dxdt[0];
  
  std::cout << "Expected (dx/dt[0]): " << expected << std::endl;
  std::cout << "Difference: " << std::abs(phase - expected) << std::endl;
  
  bool passed = std::isfinite(phase) && std::abs(phase - expected) < 1e-10;
  std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
  return passed;
}

int main()
{
  std::cout << "PSS Shooting Method Standalone Tests" << std::endl;
  std::cout << "=====================================" << std::endl;
  
  int passed = 0;
  int total = 0;
  
  if (testDrivenSimpleODE()) passed++;
  total++;
  
  if (testJacobianComputation()) passed++;
  total++;
  
  if (testPhaseCondition()) passed++;
  total++;
  
  std::cout << "\n=====================================" << std::endl;
  std::cout << "Summary: " << passed << "/" << total << " tests passed" << std::endl;
  
  return (passed == total) ? 0 : 1;
}

