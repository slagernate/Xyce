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

#include <Xyce_config.h>

#include <N_ANP_PSS.h>
#include <N_ANP_AnalysisManager.h>
#include <N_ANP_RegisterAnalysis.h>
#include <N_IO_CmdParse.h>
#include <N_IO_OptionBlock.h>
#include <N_IO_PkgOptionsMgr.h>
#include <N_IO_CircuitBlock.h>
#include <N_UTL_OptionBlock.h>
#include <N_UTL_Param.h>
#include <N_ERH_ErrorMgr.h>
#include <N_TIA_DataStore.h>
#include <N_TIA_StepErrorControl.h>
#include <N_TIA_WorkingIntegrationMethod.h>
#include <N_LAS_Vector.h>
#include <N_LAS_Builder.h>
#include <N_LAS_Matrix.h>
#include <N_LAS_System.h>
#include <N_LAS_Problem.h>
#include <N_LAS_Solver.h>
#include <N_PDS_Comm.h>
#include <N_PDS_ParMap.h>
#include <N_LOA_Loader.h>
#include <N_NLS_Manager.h>
#include <N_TOP_Topology.h>
#include <N_UTL_Math.h>
#include <N_UTL_MachDepParams.h>
#include <N_UTL_ExtendedString.h>
#include <Teuchos_SerialDenseMatrix.hpp>
#include <Teuchos_SerialDenseSolver.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
namespace Xyce {
namespace Analysis {

namespace {
void configurePssStepControl(TimeIntg::StepErrorControl &sec, const TimeIntg::TIAParams &tiaParams, double period)
{
  const double defaultInit = period / 100.0;
  const double defaultMin = period / 1e6;
  const double defaultMax = period / 10.0;

  sec.currentTimeStep = (tiaParams.initialTimeStep > 0.0) ? tiaParams.initialTimeStep : defaultInit;

  if (tiaParams.minTimeStepGiven && tiaParams.minTimeStep > 0.0)
    sec.minTimeStep = tiaParams.minTimeStep;
  else
    sec.minTimeStep = defaultMin;

  if (tiaParams.maxTimeStepGiven && tiaParams.maxTimeStep > 0.0)
    sec.maxTimeStep = tiaParams.maxTimeStep;
  else
    sec.maxTimeStep = defaultMax;

  sec.finalTime = period;
}
} // namespace

//-----------------------------------------------------------------------------
// Function      : PSS::PSS
// Purpose       : Constructor
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
PSS::PSS(
  AnalysisManager &                   analysis_manager,
  Linear::System *                    linear_system_ptr,
  Nonlinear::Manager &                nonlinear_manager,
  Loader::Loader &                    loader,
  Topo::Topology &                    topology,
  IO::InitialConditionsManager &      initial_conditions_manager,
  IO::RestartMgr &                    restart_manager)
  : AnalysisBase(analysis_manager, "PSS"),
    StepEventListener(&analysis_manager),
    analysisManager_(analysis_manager),
    loader_(loader),
    linearSystemPtr_(linear_system_ptr),
    nonlinearManager_(nonlinear_manager),
    topology_(topology),
    initialConditionsManager_(initial_conditions_manager),
    restartManager_(restart_manager),
    period_(1.0),
    periodGiven_(false),
    numPeriods_(1),
    numPeriodsGiven_(false),
    tStart_(0.0),
    tStartGiven_(false),
    tStop_(0.0),
    tStopGiven_(false),
    autonomousMode_(false),
    refNode_(""),
    refNodeGiven_(false),
    maxIterations_(50),
    tolerance_(1e-6),
    startUpPeriods_(0),
    startUpPeriodsGiven_(false)
{
}

//-----------------------------------------------------------------------------
// Function      : PSS::~PSS
// Purpose       : Destructor
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
PSS::~PSS()
{
}

//-----------------------------------------------------------------------------
// Function      : PSS::notify
// Purpose       : Handle step events
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
void PSS::notify(const StepEvent &event)
{
}

//-----------------------------------------------------------------------------
// Function      : PSS::stepCallback
// Purpose       : Step callback
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
void PSS::stepCallback()
{
}

//-----------------------------------------------------------------------------
// Function      : PSS::setAnalysisParams
// Purpose       : Set analysis parameters from option block
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::setAnalysisParams(const Util::OptionBlock & paramsBlock)
{
  for (Util::ParamList::const_iterator it = paramsBlock.begin(), end = paramsBlock.end(); it != end; ++it)
  {
    const std::string &tag = (*it).tag();
    
    if (tag == "PERIOD")
    {
      period_ = (*it).getImmutableValue<double>();
      periodGiven_ = true;
    }
    else if (tag == "NUMPERIODS")
    {
      numPeriods_ = (*it).getImmutableValue<int>();
      numPeriodsGiven_ = true;
    }
    else if (tag == "TSTART")
    {
      tStart_ = (*it).getImmutableValue<double>();
      tStartGiven_ = true;
    }
    else if (tag == "TSTOP")
    {
      tStop_ = (*it).getImmutableValue<double>();
      tStopGiven_ = true;
    }
    else if (tag == "AUTONOMOUS")
    {
      autonomousMode_ = true;
    }
    else if (tag == "REFNODE")
    {
      refNode_ = (*it).stringValue();
      refNodeGiven_ = true;
    }
    else if (tag == "MAXITER")
    {
      maxIterations_ = (*it).getImmutableValue<int>();
    }
    else if (tag == "TOL")
    {
      tolerance_ = (*it).getImmutableValue<double>();
    }
    else if (tag == "STARTPERIODS")
    {
      startUpPeriods_ = (*it).getImmutableValue<int>();
      startUpPeriodsGiven_ = true;
    }
  }
  
  // Set defaults
  if (!tStopGiven_ && periodGiven_)
  {
    tStop_ = period_ * numPeriods_;
  }
  
  if (!periodGiven_)
  {
    Report::UserError0() << "PSS analysis requires PERIOD parameter";
    return false;
  }
  
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::finalExpressionBasedSetup
// Purpose       : Final expression-based setup
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
void PSS::finalExpressionBasedSetup()
{
}

//-----------------------------------------------------------------------------
// Function      : PSS::doInit
// Purpose       : Initialize analysis
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doInit()
{
  // Create time integrator if not already created
  if (!analysisManager_.getWorkingIntegrationMethod().isTimeIntegrationMethodCreated())
  {
    analysisManager_.createTimeIntegratorMethod(tiaParams_, TimeIntg::methodsEnum::ONESTEP);
  }

  // Initialize time integrator
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  analysisManager_.getStepErrorControl().initialTime = 0.0;
  analysisManager_.getStepErrorControl().currentTime = 0.0;
  analysisManager_.getStepErrorControl().nextTime = 0.0;
  configurePssStepControl(analysisManager_.getStepErrorControl(), tiaParams_, period_);
  
  // Initialize time integrator
  analysisManager_.getWorkingIntegrationMethod().initialize(tiaParams_);
  
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::doRun
// Purpose       : Run analysis
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doRun()
{
  return doInit() && doLoopProcess() && doFinish();
}

//-----------------------------------------------------------------------------
// Function      : PSS::doLoopProcess
// Purpose       : Main analysis loop - implements shooting method
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doLoopProcess()
{
  bool bsuccess = true;
  
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  TimeIntg::StepErrorControl &sec = analysisManager_.getStepErrorControl();
  
  // For autonomous mode, period is unknown - use initial guess if not given
  if (autonomousMode_ && !periodGiven_)
  {
    // Use a reasonable initial guess (e.g., from startup periods or default)
    if (startUpPeriodsGiven_ && startUpPeriods_ > 0)
    {
      // Estimate period from startup simulation if available
      // For now, use a default guess
      period_ = 1e-9; // Default: 1ns (typical for oscillators)
    }
    else
    {
      period_ = 1e-9; // Default initial guess
    }
  }
  
  // Validate autonomous mode requirements
  if (autonomousMode_)
  {
    if (!refNodeGiven_ || refNode_.empty())
    {
      Report::UserError0() << "PSS: AUTONOMOUS mode requires REFNODE to be specified";
      return false;
    }
  }
  
  // Save initial conditions (x(0))
  Linear::Vector *x0_sol = ds.builder_.createVector();
  Linear::Vector *x0_state = ds.builder_.createStateVector();
  Linear::Vector *x0_store = ds.builder_.createStoreVector();
  
  x0_sol->putScalar(0.0);
  x0_state->putScalar(0.0);
  x0_store->putScalar(0.0);
  
  // Newton iteration loop
  for (int iter = 0; iter < maxIterations_; ++iter)
  {
    // Save current initial condition
    x0_sol->update(1.0, *(ds.currSolutionPtr), 0.0);
    if (ds.stateSize > 0)
      x0_state->update(1.0, *(ds.currStatePtr), 0.0);
    if (ds.storeSize > 0)
      x0_store->update(1.0, *(ds.currStorePtr), 0.0);
    
    // Integrate from t=0 to t=T
    // For autonomous mode, use the current period (which may be updated)
    bool integrateSuccess;
    if (autonomousMode_)
    {
      integrateSuccess = integrateOnePeriod(period_);
    }
    else
    {
      integrateSuccess = integrateOnePeriod();
    }
    
    if (!integrateSuccess)
    {
      Report::UserWarning0() << "PSS: Integration failed at iteration " << iter;
      bsuccess = false;
      break;
    }
    
    // Compute residual: r = x(T) - x(0)
    // Include solution, state, and store vectors
    Linear::Vector *residual = ds.builder_.createVector();
    residual->update(1.0, *(ds.nextSolutionPtr), -1.0, *x0_sol, 0.0);
    
    // Also check state and store vectors for periodicity
    double stateResidualNorm = 0.0;
    double storeResidualNorm = 0.0;
    
    if (ds.stateSize > 0)
    {
      Linear::Vector *stateResidual = ds.builder_.createStateVector();
      stateResidual->update(1.0, *(ds.nextStatePtr), -1.0, *x0_state, 0.0);
      double stateNormResult[1];
      stateResidual->infNorm(stateNormResult);
      stateResidualNorm = stateNormResult[0];
      delete stateResidual;
    }
    
    if (ds.storeSize > 0)
    {
      Linear::Vector *storeResidual = ds.builder_.createStoreVector();
      storeResidual->update(1.0, *(ds.nextStorePtr), -1.0, *x0_store, 0.0);
      double storeNormResult[1];
      storeResidual->infNorm(storeNormResult);
      storeResidualNorm = storeNormResult[0];
      delete storeResidual;
    }
    
    // Compute overall residual norm (max of all components)
    double residualNormResult[1];
    residual->infNorm(residualNormResult);
    double residualNorm = residualNormResult[0];
    residualNorm = std::max(residualNorm, stateResidualNorm);
    residualNorm = std::max(residualNorm, storeResidualNorm);
    
    if (residualNorm < tolerance_)
    {
      // Converged! Update solution arrays
      ds.updateSolDataArrays();
      if (ds.stateSize > 0)
      {
        Linear::Vector *tmp = ds.lastStatePtr;
        ds.lastStatePtr = ds.currStatePtr;
        ds.currStatePtr = ds.nextStatePtr;
        ds.nextStatePtr = tmp;
      }
      if (ds.storeSize > 0)
      {
        Linear::Vector *tmp = ds.lastStorePtr;
        ds.lastStorePtr = ds.currStorePtr;
        ds.currStorePtr = ds.nextStorePtr;
        ds.nextStorePtr = tmp;
      }
      
      Report::UserInfo0() << "PSS: Converged in " << iter + 1 << " iterations, residual = " << residualNorm;
      delete residual;
      break;
    }
    
    if (iter == maxIterations_ - 1)
    {
      Report::UserWarning0() << "PSS: Did not converge after " << maxIterations_ << " iterations, residual = " << residualNorm;
      delete residual;
      bsuccess = false;
      break;
    }
    
    // Compute Jacobian and solve for update
    // For now, use simple finite difference
    // TODO: Use Xyce's linear solver for better performance
    Linear::Vector *update = computeNewtonUpdate(residual, x0_sol);
    
    // Update initial condition: x(0) = x(0) + delta
    ds.currSolutionPtr->update(1.0, *update, 1.0);
    
    // Also update state and store vectors if needed
    // For now, use same update strategy (can be refined)
    if (ds.stateSize > 0)
    {
      // Simple: restore from saved x0_state, will be updated by integration
      ds.currStatePtr->update(1.0, *x0_state, 0.0);
    }
    if (ds.storeSize > 0)
    {
      // Simple: restore from saved x0_store, will be updated by integration
      ds.currStorePtr->update(1.0, *x0_store, 0.0);
    }
    
    // Reset time to 0 for next iteration
    sec.currentTime = 0.0;
    sec.nextTime = 0.0;
    analysisManager_.getWorkingIntegrationMethod().initialize(tiaParams_);
    
    delete residual;
    delete update;
  }
  
  delete x0_sol;
  delete x0_state;
  delete x0_store;
  
  return bsuccess;
}

//-----------------------------------------------------------------------------
// Function      : PSS::integrateOnePeriod
// Purpose       : Integrate from t=0 to t=T (one period)
// Special Notes :
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::integrateOnePeriod()
{
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  TimeIntg::StepErrorControl &sec = analysisManager_.getStepErrorControl();
  
  // Reset time
  sec.initialTime = 0.0;
  sec.currentTime = 0.0;
  sec.nextTime = 0.0;
  sec.finalTime = period_;
  
  // Integration loop
  while (sec.currentTime < period_ - 1e-12)
  {
    // Update next time
    sec.nextTime = std::min(sec.currentTime + sec.currentTimeStep, period_);
    
    // Take integration step
    doHandlePredictor();
    loader_.updateSources();
    
    // Nonlinear solve
    sec.newtonConvergenceStatus = nonlinearManager_.solve();
    
    if (sec.newtonConvergenceStatus <= 0)
    {
      // Step failed
      return false;
    }
    
    // Complete step
    analysisManager_.getWorkingIntegrationMethod().updateLeadCurrent();
    analysisManager_.getWorkingIntegrationMethod().stepLinearCombo();
    sec.evaluateStepError(loader_, tiaParams_);
    
    if (!sec.stepAttemptStatus)
    {
      // Step rejected
      analysisManager_.getWorkingIntegrationMethod().rejectStep(tiaParams_);
      continue;
    }
    
    // Step accepted
    analysisManager_.getWorkingIntegrationMethod().completeStep(tiaParams_);
    ds.updateSolDataArrays();
    
    // Update time
    sec.currentTime = sec.nextTime;
    // Update time step (StepErrorControl doesn't have updateTimeStep, so we manually update)
    // The time step is managed by the time integrator
    
    // Update coefficients for next step
    analysisManager_.getWorkingIntegrationMethod().updateCoeffs();
  }
  
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::integrateOnePeriod (overload for autonomous mode)
// Purpose       : Integrate from t=0 to t=T with specified period
// Special Notes : Used in autonomous mode when period is being updated
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::integrateOnePeriod(double period)
{
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  TimeIntg::StepErrorControl &sec = analysisManager_.getStepErrorControl();
  
  // Reset time
  sec.initialTime = 0.0;
  sec.currentTime = 0.0;
  sec.nextTime = 0.0;
  sec.finalTime = period;
  
  // Integration loop
  while (sec.currentTime < period - 1e-12)
  {
    // Update next time
    sec.nextTime = std::min(sec.currentTime + sec.currentTimeStep, period);
    
    // Take integration step
    doHandlePredictor();
    loader_.updateSources();
    
    // Nonlinear solve
    sec.newtonConvergenceStatus = nonlinearManager_.solve();
    
    if (sec.newtonConvergenceStatus <= 0)
    {
      // Step failed
      return false;
    }
    
    // Complete step
    analysisManager_.getWorkingIntegrationMethod().updateLeadCurrent();
    analysisManager_.getWorkingIntegrationMethod().stepLinearCombo();
    sec.evaluateStepError(loader_, tiaParams_);
    
    if (!sec.stepAttemptStatus)
    {
      // Step rejected
      analysisManager_.getWorkingIntegrationMethod().rejectStep(tiaParams_);
      continue;
    }
    
    // Step accepted
    analysisManager_.getWorkingIntegrationMethod().completeStep(tiaParams_);
    ds.updateSolDataArrays();
    
    // Update time
    sec.currentTime = sec.nextTime;
    
    // Update coefficients for next step
    analysisManager_.getWorkingIntegrationMethod().updateCoeffs();
  }
  
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computePhaseCondition
// Purpose       : Compute phase condition: dV/dt = 0 at reference node
// Special Notes : For autonomous mode, this provides uniqueness constraint
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
double PSS::computePhaseCondition(Linear::Vector *x0)
{
  // Phase condition: dV/dt = 0 at reference node
  // For autonomous oscillators, this provides uniqueness by fixing the phase
  // The condition is typically: dV/dt = 0 at t=0 for the reference node
  // This means the voltage at the reference node is at an extremum (max or min)
  
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  
  // Find the reference node index
  int refNodeIndex = -1;
  if (refNodeGiven_ && !refNode_.empty())
  {
    // Look up node index from topology solution node names
    const std::vector<const std::string *> &nodeNames = topology_.getSolutionNodeNames();
    for (size_t i = 0; i < nodeNames.size(); ++i)
    {
      if (nodeNames[i] && (*nodeNames[i] == refNode_))
      {
        refNodeIndex = static_cast<int>(i);
        break;
      }
    }
    if (refNodeIndex < 0)
    {
      Report::UserWarning0() << "PSS: REFNODE \"" << refNode_
                             << "\" not found in solution node list; using first node.";
    }
  }

  if (refNodeIndex < 0)
  {
    refNodeIndex = 0; // Default to first node
  }
  
  // Compute dV/dt at reference node
  // The derivative is computed from the circuit equations: dx/dt = f(x, t)
  // For autonomous circuits, at t=0: dx/dt = f(x(0), 0)
  // The phase condition is: dx/dt[refNode] = 0
  
  // We can approximate this by:
  // 1. Setting the circuit to the initial condition x(0)
  // 2. Evaluating the circuit equations to get dx/dt
  // 3. Returning dx/dt[refNode]
  
  // For Phase 3, we use a simplified approach:
  // The derivative can be approximated from the residual computation
  // or by evaluating the circuit equations directly
  
  // Simplified implementation: compute the derivative from the circuit evaluation
  // This requires evaluating the circuit at the current state
  // For now, we use a finite difference approximation:
  // dV/dt ≈ (V(t+dt) - V(t)) / dt evaluated at t=0
  
  // Get a small time step
  double dt = period_ / 1000.0; // Small fraction of period
  
  // Save current state
  Linear::Vector *x_save = ds.builder_.createVector();
  x_save->update(1.0, *x0, 0.0);
  
  // Evaluate circuit at t=0 with current state
  // This gives us dx/dt at t=0
  // For Phase 3, we approximate by using the time integrator's derivative computation
  
  // The phase condition is the derivative of the solution at the reference node
  // We can get this from the time integrator's derivative computation
  // For now, use a simplified approach: return the derivative from the residual
  
  // Actually, the phase condition for autonomous oscillators is typically:
  // dV/dt = 0 at the reference node, which means we want the voltage
  // to be at an extremum at t=0
  
  // A simpler approach for Phase 3: use the derivative from evaluating
  // the circuit equations at the current state
  // This is available from the time integrator's derivative computation
  
  // For now, return a placeholder that will be properly implemented
  // The actual implementation would:
  // 1. Set circuit state to x0
  // 2. Evaluate circuit equations: dx/dt = f(x0, 0)
  // 3. Return dx/dt[refNode]
  
  // Simplified: use the derivative from the next solution step
  // This is an approximation but works for Phase 3
  double phaseCondition = 0.0;
  
  // Try to get derivative from the time integrator
  // For autonomous mode, we want dV/dt = 0 at refNode
  // This is computed from the circuit equations evaluated at x(0), t=0
  
  // Phase 3 simplified implementation:
  // Use the difference between current and next solution as proxy for derivative
  // This is not exact but provides a working implementation
  if (ds.currSolutionPtr && ds.nextSolutionPtr)
  {
    // Compute approximate derivative: (x(dt) - x(0)) / dt
    // For small dt, this approximates dx/dt
    Linear::Vector *deriv = ds.builder_.createVector();
    deriv->update(1.0 / dt, *(ds.nextSolutionPtr), -1.0 / dt, *(ds.currSolutionPtr), 0.0);

    double localPhase = 0.0;
    int refLocal = ds.builder_.getSolutionMap()->globalToLocalIndex(refNodeIndex);
    if (refLocal >= 0)
    {
      localPhase = (*deriv)[refLocal];
    }

    const Parallel::Communicator *comm = ds.currSolutionPtr->pdsComm();
    if (comm)
      comm->sumAll(&localPhase, &phaseCondition, 1);
    else
      phaseCondition = localPhase;

    delete deriv;
  }
  
  delete x_save;
  
  return phaseCondition;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computeNewtonUpdateAutonomous
// Purpose       : Compute Newton update for autonomous mode (includes period)
// Special Notes : Solves extended system: [x(T)-x(0), phase] = 0 for [x(0), T]
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
Linear::Vector *PSS::computeNewtonUpdateAutonomous(Linear::Vector *residual, Linear::Vector *x0, double &periodUpdate)
{
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());

  // Compute phase condition
  double phaseCondition = computePhaseCondition(x0);

  // Adaptive damping based on combined residual norm
  double residualNormResult[1];
  residual->infNorm(residualNormResult);
  double residualNorm = residualNormResult[0];
  double combinedNorm = std::sqrt(residualNorm * residualNorm + phaseCondition * phaseCondition);

  double alpha = 0.1;
  if (combinedNorm > 1.0)
    alpha = 0.01;  // Smaller step for large residuals
  else if (combinedNorm < 0.1)
    alpha = 0.5;   // Larger step when close to solution

  Linear::Vector *update = ds.builder_.createVector();

  const Parallel::Communicator *comm = x0->pdsComm();
  if (!comm)
  {
    Report::UserWarning0() << "PSS: Missing communicator; using damped update.";
    update->update(-alpha, *residual, 0.0);
    periodUpdate = -alpha * phaseCondition * 1e-9;
    return update;
  }

  const char *matrixFreeEnv = std::getenv("XYCE_PSS_MATRIX_FREE");
  bool useMatrixFree = false;
  if (matrixFreeEnv)
  {
    useMatrixFree = (std::string(matrixFreeEnv) == "1");
  }
  else if (comm->numProc() > 1)
  {
    useMatrixFree = true;
  }
  if (useMatrixFree)
  {
    if (solveNewtonSystemMatrixFreeAutonomous(x0, residual, phaseCondition, period_, update, periodUpdate))
    {
      update->scale(alpha);
      periodUpdate *= alpha;
      return update;
    }
    Report::UserWarning0() << "PSS: Matrix-free solve failed; falling back to dense solve.";
  }

  const int n = x0->globalLength();
  const int base = x0->pmap()->indexBase();
  const int rank = comm->procID();

  std::vector<double> jacobian;
  if (!computeJacobianFiniteDifferenceAutonomous(x0, period_, residual, phaseCondition, jacobian))
  {
    Report::UserWarning0() << "PSS: Failed to compute autonomous Jacobian; using damped update.";
    update->update(-alpha, *residual, 0.0);
    periodUpdate = -alpha * phaseCondition * 1e-9;
    return update;
  }

  std::vector<double> delta(n + 1, 0.0);
  int solveStatus = 0;

  if (rank == 0)
  {
    Teuchos::SerialDenseMatrix<int, double> A(n + 1, n + 1);
    Teuchos::SerialDenseMatrix<int, double> B(n + 1, 1);
    Teuchos::SerialDenseMatrix<int, double> X(n + 1, 1);

    for (int i = 0; i < n + 1; ++i)
    {
      for (int j = 0; j < n + 1; ++j)
      {
        A(i, j) = jacobian[i * (n + 1) + j];
      }
    }

    for (int i = 0; i < n; ++i)
    {
      B(i, 0) = -residual->getElementByGlobalIndex(base + i);
    }
    B(n, 0) = -phaseCondition;

    Teuchos::SerialDenseSolver<int, double> solver;
    solver.setMatrix(Teuchos::rcp(&A, false));
    solver.setVectors(Teuchos::rcp(&X, false), Teuchos::rcp(&B, false));
    solver.factorWithEquilibration(true);
    solveStatus = solver.factor();
    if (solveStatus == 0)
      solveStatus = solver.solve();

    if (solveStatus == 0)
    {
      for (int i = 0; i < n + 1; ++i)
        delta[i] = X(i, 0);
    }
  }

  comm->bcast(&solveStatus, 1, 0);
  if (solveStatus != 0)
  {
    Report::UserWarning0() << "PSS: Autonomous linear solve failed; using damped update.";
    update->update(-alpha, *residual, 0.0);
    periodUpdate = -alpha * phaseCondition * 1e-9;
    return update;
  }

  comm->bcast(&delta[0], n + 1, 0);

  const Parallel::ParMap *map = x0->pmap();
  const int localLength = map->numLocalEntities();
  update->putScalar(0.0);
  for (int lid = 0; lid < localLength; ++lid)
  {
    int gid = map->localToGlobalIndex(lid);
    if (gid >= base)
      update->setElementByGlobalIndex(gid, alpha * delta[gid - base]);
  }
  periodUpdate = alpha * delta[n];

  return update;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computeJacobianFiniteDifferenceAutonomous
// Purpose       : Compute Jacobian for autonomous mode (includes period derivatives)
// Special Notes : Jacobian is (n+1) x (n+1): [x(T)-x(0), phase] w.r.t. [x(0), T]
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::computeJacobianFiniteDifferenceAutonomous(Linear::Vector *x0, double period, Linear::Vector *residual0,
                                                    double phaseCondition, std::vector<double> &jacobian)
{
  const Parallel::Communicator *comm = x0->pdsComm();
  if (!comm)
  {
    Report::UserWarning0() << "PSS: Missing communicator for autonomous Jacobian.";
    return false;
  }

  const Parallel::ParMap *map = x0->pmap();
  const int rank = comm->procID();
  const int numProcs = comm->numProc();
  const int n = x0->globalLength();
  const int base = map->indexBase();
  const int localRows = map->numLocalEntities();

  std::vector<int> localGids(localRows);
  for (int lid = 0; lid < localRows; ++lid)
    localGids[lid] = map->localToGlobalIndex(lid);

  std::vector<double> r0(localRows, 0.0);
  for (int i = 0; i < localRows; ++i)
    r0[i] = residual0->getElementByGlobalIndex(localGids[i]);

  std::vector<double> localJac(localRows * (n + 1), 0.0);
  std::vector<double> phaseRow;
  if (rank == 0)
    phaseRow.assign(n + 1, 0.0);

  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  Linear::Vector *xPert = ds.builder_.createVector();

  for (int j = 0; j < n; ++j)
  {
    xPert->update(1.0, *x0, 0.0);
    int gid = base + j;
    double xj = x0->getElementByGlobalIndex(gid);
    double dx = computePerturbation(xj);
    int lid = map->globalToLocalIndex(gid);
    if (lid >= 0)
      xPert->setElementByGlobalIndex(gid, xj + dx);

    Linear::Vector *rPert = computeResidualVector(xPert, period);
    double phiPert = computePhaseCondition(xPert);

    for (int i = 0; i < localRows; ++i)
    {
      double val = (rPert->getElementByGlobalIndex(localGids[i]) - r0[i]) / dx;
      localJac[i * (n + 1) + j] = val;
    }

    if (rank == 0)
      phaseRow[j] = (phiPert - phaseCondition) / dx;

    delete rPert;
  }

  double dT = computePerturbation(period);
  double periodPert = period + dT;
  Linear::Vector *rPertT = computeResidualVector(x0, periodPert);
  for (int i = 0; i < localRows; ++i)
  {
    localJac[i * (n + 1) + n] = (rPertT->getElementByGlobalIndex(localGids[i]) - r0[i]) / dT;
  }
  if (rank == 0)
    phaseRow[n] = 0.0;

  delete rPertT;
  delete xPert;

  if (rank == 0)
  {
    jacobian.assign((n + 1) * (n + 1), 0.0);
    for (int i = 0; i < localRows; ++i)
    {
      int row = localGids[i] - base;
      if (row >= 0 && row < n)
      {
        for (int j = 0; j < n + 1; ++j)
          jacobian[row * (n + 1) + j] = localJac[i * (n + 1) + j];
      }
    }

    for (int proc = 1; proc < numProcs; ++proc)
    {
      int count = 0;
      comm->recv(&count, 1, proc);
      if (count <= 0)
        continue;

      std::vector<int> gids(count);
      std::vector<double> vals(count * (n + 1));
      comm->recv(&gids[0], count, proc);
      comm->recv(&vals[0], count * (n + 1), proc);

      for (int i = 0; i < count; ++i)
      {
        int row = gids[i] - base;
        if (row >= 0 && row < n)
        {
          for (int j = 0; j < n + 1; ++j)
            jacobian[row * (n + 1) + j] = vals[i * (n + 1) + j];
        }
      }
    }

    for (int j = 0; j < n + 1; ++j)
      jacobian[n * (n + 1) + j] = phaseRow[j];
  }
  else
  {
    int count = localRows;
    comm->send(&count, 1, 0);
    if (count > 0)
    {
      comm->send(&localGids[0], count, 0);
      comm->send(&localJac[0], count * (n + 1), 0);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computeResidualVector
// Purpose       : Compute residual vector r = x(T) - x(0) for given x0
// Special Notes : Helper function for Jacobian computation
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
Linear::Vector *PSS::computeResidualVector(Linear::Vector *x0)
{
  return computeResidualVector(x0, period_);
}

//-----------------------------------------------------------------------------
// Function      : PSS::computeResidualVector
// Purpose       : Compute residual vector r = x(T) - x(0) for given x0 and period
// Special Notes : Helper function for Jacobian computation
// Scope         : private
//-----------------------------------------------------------------------------
Linear::Vector *PSS::computeResidualVector(Linear::Vector *x0, double period)
{
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  TimeIntg::StepErrorControl &sec = analysisManager_.getStepErrorControl();
  
  // Save current solution
  Linear::Vector *saved_sol = ds.builder_.createVector();
  saved_sol->update(1.0, *(ds.currSolutionPtr), 0.0);
  
  // Set initial condition to x0
  ds.currSolutionPtr->update(1.0, *x0, 0.0);
  
  // Reset time and adjust step parameters to match the period
  sec.currentTime = 0.0;
  sec.nextTime = 0.0;
  configurePssStepControl(sec, tiaParams_, period);
  analysisManager_.getWorkingIntegrationMethod().initialize(tiaParams_);
  
  // Integrate from 0 to T
  bool success = integrateOnePeriod(period);
  
  if (!success)
  {
    // Integration failed, return large residual
    Linear::Vector *largeResidual = ds.builder_.createVector();
    largeResidual->putScalar(1e10);
    ds.currSolutionPtr->update(1.0, *saved_sol, 0.0);
    delete saved_sol;
    return largeResidual;
  }
  
  // Compute residual: r = x(T) - x(0)
  Linear::Vector *residual = ds.builder_.createVector();
  residual->update(1.0, *(ds.nextSolutionPtr), -1.0, *x0, 0.0);
  
  // Restore solution
  ds.currSolutionPtr->update(1.0, *saved_sol, 0.0);
  delete saved_sol;
  
  return residual;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computePerturbation
// Purpose       : Compute a finite difference perturbation size
//-----------------------------------------------------------------------------
double PSS::computePerturbation(double value) const
{
  double scale = std::max(1.0, std::fabs(value));
  double step = std::sqrt(std::numeric_limits<double>::epsilon()) * scale;
  if (step == 0.0)
    step = 1.0e-12;
  return step;
}

//-----------------------------------------------------------------------------
// Function      : PSS::solveNewtonSystemMatrixFree
// Purpose       : Matrix-free GMRES solve for driven mode
//-----------------------------------------------------------------------------
bool PSS::solveNewtonSystemMatrixFree(Linear::Vector *x0, Linear::Vector *residual, Linear::Vector *update)
{
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  const int n = residual->globalLength();
  if (n <= 0)
  {
    update->putScalar(0.0);
    return true;
  }

  Linear::Vector *b = ds.builder_.createVector();
  b->update(-1.0, *residual, 0.0);
  double beta = std::sqrt(b->dotProduct(*b));
  if (beta == 0.0)
  {
    update->putScalar(0.0);
    delete b;
    return true;
  }

  const int restart = std::min(20, n);
  const int maxIters = restart;
  const double tol = std::max(1.0e-12, 0.1 * beta);

  std::vector<Linear::Vector *> v(restart + 1, nullptr);
  for (int i = 0; i < restart + 1; ++i)
    v[i] = ds.builder_.createVector();

  v[0]->update(1.0 / beta, *b, 0.0);

  std::vector<double> h((restart + 1) * restart, 0.0);
  std::vector<double> cs(restart, 0.0);
  std::vector<double> sn(restart, 0.0);
  std::vector<double> g(restart + 1, 0.0);
  g[0] = beta;

  Linear::Vector *w = ds.builder_.createVector();

  auto applyJacobian = [&](Linear::Vector *vec, Linear::Vector *out) {
    double eps = computePerturbation(0.0);
    Linear::Vector *xPert = ds.builder_.createVector();
    xPert->update(1.0, *x0, 0.0);
    xPert->update(eps, *vec, 1.0);

    Linear::Vector *rPert = computeResidualVector(xPert, period_);
    out->update(1.0 / eps, *rPert, -1.0 / eps, *residual, 0.0);

    delete rPert;
    delete xPert;
  };

  int iterCount = 0;
  int lastDim = restart;
  bool converged = false;
  for (iterCount = 0; iterCount < maxIters && !converged; iterCount += restart)
  {
    for (int j = 0; j < restart; ++j)
    {
      applyJacobian(v[j], w);

      for (int i = 0; i <= j; ++i)
      {
        double hij = w->dotProduct(*v[i]);
        h[i * restart + j] = hij;
        w->update(-hij, *v[i], 1.0);
      }

      double hNext = std::sqrt(w->dotProduct(*w));
      h[(j + 1) * restart + j] = hNext;
      if (hNext != 0.0)
        v[j + 1]->update(1.0 / hNext, *w, 0.0);
      else
        v[j + 1]->putScalar(0.0);

      for (int i = 0; i < j; ++i)
      {
        double temp = cs[i] * h[i * restart + j] + sn[i] * h[(i + 1) * restart + j];
        h[(i + 1) * restart + j] = -sn[i] * h[i * restart + j] + cs[i] * h[(i + 1) * restart + j];
        h[i * restart + j] = temp;
      }

      double denom = std::sqrt(h[j * restart + j] * h[j * restart + j] +
                               h[(j + 1) * restart + j] * h[(j + 1) * restart + j]);
      cs[j] = (denom == 0.0) ? 1.0 : h[j * restart + j] / denom;
      sn[j] = (denom == 0.0) ? 0.0 : h[(j + 1) * restart + j] / denom;
      h[j * restart + j] = cs[j] * h[j * restart + j] + sn[j] * h[(j + 1) * restart + j];
      h[(j + 1) * restart + j] = 0.0;

      double tempG = cs[j] * g[j];
      g[j + 1] = -sn[j] * g[j];
      g[j] = tempG;

      if (std::fabs(g[j + 1]) <= tol)
      {
        lastDim = j + 1;
        converged = true;
        break;
      }
    }

    if (converged)
      break;
  }

  if (!converged)
    lastDim = restart;

  std::vector<double> y(lastDim, 0.0);
  for (int i = lastDim - 1; i >= 0; --i)
  {
    double sum = g[i];
    for (int j = i + 1; j < lastDim; ++j)
      sum -= h[i * restart + j] * y[j];
    if (h[i * restart + i] == 0.0)
    {
      converged = false;
      break;
    }
    y[i] = sum / h[i * restart + i];
  }

  update->putScalar(0.0);
  if (converged)
  {
    for (int i = 0; i < lastDim; ++i)
      update->update(y[i], *v[i], 1.0);
  }

  delete w;
  delete b;
  for (Linear::Vector *vec : v)
    delete vec;

  return converged;
}

//-----------------------------------------------------------------------------
// Function      : PSS::solveNewtonSystemMatrixFreeAutonomous
// Purpose       : Matrix-free GMRES solve for autonomous mode (x0 and period)
//-----------------------------------------------------------------------------
bool PSS::solveNewtonSystemMatrixFreeAutonomous(Linear::Vector *x0, Linear::Vector *residual, double phaseCondition,
                                                double period, Linear::Vector *update, double &periodUpdate)
{
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  const int n = residual->globalLength();
  if (n <= 0)
  {
    update->putScalar(0.0);
    periodUpdate = 0.0;
    return true;
  }

  Linear::Vector *bX = ds.builder_.createVector();
  bX->update(-1.0, *residual, 0.0);
  double beta = std::sqrt(bX->dotProduct(*bX) + phaseCondition * phaseCondition);
  if (beta == 0.0)
  {
    update->putScalar(0.0);
    periodUpdate = 0.0;
    delete bX;
    return true;
  }

  const int restart = std::min(20, n + 1);
  const int maxIters = restart;
  const double tol = std::max(1.0e-12, 0.1 * beta);

  std::vector<Linear::Vector *> vX(restart + 1, nullptr);
  std::vector<double> vT(restart + 1, 0.0);
  for (int i = 0; i < restart + 1; ++i)
    vX[i] = ds.builder_.createVector();

  vX[0]->update(1.0 / beta, *bX, 0.0);
  vT[0] = -phaseCondition / beta;

  std::vector<double> h((restart + 1) * restart, 0.0);
  std::vector<double> cs(restart, 0.0);
  std::vector<double> sn(restart, 0.0);
  std::vector<double> g(restart + 1, 0.0);
  g[0] = beta;

  Linear::Vector *wX = ds.builder_.createVector();
  double wT = 0.0;

  auto applyJacobian = [&](Linear::Vector *vecX, double vecT, Linear::Vector *outX, double &outT) {
    double eps = computePerturbation(0.0);
    Linear::Vector *xPert = ds.builder_.createVector();
    xPert->update(1.0, *x0, 0.0);
    xPert->update(eps, *vecX, 1.0);
    double periodPert = period + eps * vecT;

    Linear::Vector *rPert = computeResidualVector(xPert, periodPert);
    double phiPert = computePhaseCondition(xPert);

    outX->update(1.0 / eps, *rPert, -1.0 / eps, *residual, 0.0);
    outT = (phiPert - phaseCondition) / eps;

    delete rPert;
    delete xPert;
  };

  int iterCount = 0;
  int lastDim = restart;
  bool converged = false;
  for (iterCount = 0; iterCount < maxIters && !converged; iterCount += restart)
  {
    for (int j = 0; j < restart; ++j)
    {
      applyJacobian(vX[j], vT[j], wX, wT);

      for (int i = 0; i <= j; ++i)
      {
        double hij = wX->dotProduct(*vX[i]) + wT * vT[i];
        h[i * restart + j] = hij;
        wX->update(-hij, *vX[i], 1.0);
        wT -= hij * vT[i];
      }

      double hNext = std::sqrt(wX->dotProduct(*wX) + wT * wT);
      h[(j + 1) * restart + j] = hNext;
      if (hNext != 0.0)
      {
        vX[j + 1]->update(1.0 / hNext, *wX, 0.0);
        vT[j + 1] = wT / hNext;
      }
      else
      {
        vX[j + 1]->putScalar(0.0);
        vT[j + 1] = 0.0;
      }

      for (int i = 0; i < j; ++i)
      {
        double temp = cs[i] * h[i * restart + j] + sn[i] * h[(i + 1) * restart + j];
        h[(i + 1) * restart + j] = -sn[i] * h[i * restart + j] + cs[i] * h[(i + 1) * restart + j];
        h[i * restart + j] = temp;
      }

      double denom = std::sqrt(h[j * restart + j] * h[j * restart + j] +
                               h[(j + 1) * restart + j] * h[(j + 1) * restart + j]);
      cs[j] = (denom == 0.0) ? 1.0 : h[j * restart + j] / denom;
      sn[j] = (denom == 0.0) ? 0.0 : h[(j + 1) * restart + j] / denom;
      h[j * restart + j] = cs[j] * h[j * restart + j] + sn[j] * h[(j + 1) * restart + j];
      h[(j + 1) * restart + j] = 0.0;

      double tempG = cs[j] * g[j];
      g[j + 1] = -sn[j] * g[j];
      g[j] = tempG;

      if (std::fabs(g[j + 1]) <= tol)
      {
        lastDim = j + 1;
        converged = true;
        break;
      }
    }

    if (converged)
      break;
  }

  if (!converged)
    lastDim = restart;

  std::vector<double> y(lastDim, 0.0);
  for (int i = lastDim - 1; i >= 0; --i)
  {
    double sum = g[i];
    for (int j = i + 1; j < lastDim; ++j)
      sum -= h[i * restart + j] * y[j];
    if (h[i * restart + i] == 0.0)
    {
      converged = false;
      break;
    }
    y[i] = sum / h[i * restart + i];
  }

  update->putScalar(0.0);
  periodUpdate = 0.0;
  if (converged)
  {
    for (int i = 0; i < lastDim; ++i)
    {
      update->update(y[i], *vX[i], 1.0);
      periodUpdate += y[i] * vT[i];
    }
  }

  delete wX;
  delete bX;
  for (Linear::Vector *vec : vX)
    delete vec;

  return converged;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computeJacobianFiniteDifference
// Purpose       : Compute Jacobian using finite differences
// Special Notes : J[i][j] = d(residual[i])/d(x0[j])
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::computeJacobianFiniteDifference(Linear::Vector *x0, Linear::Vector *residual0, std::vector<double> &jacobian)
{
  const Parallel::Communicator *comm = x0->pdsComm();
  if (!comm)
  {
    Report::UserWarning0() << "PSS: Missing communicator for Jacobian.";
    return false;
  }

  const Parallel::ParMap *map = x0->pmap();
  const int rank = comm->procID();
  const int numProcs = comm->numProc();
  const int n = x0->globalLength();
  const int base = map->indexBase();
  const int localRows = map->numLocalEntities();

  std::vector<int> localGids(localRows);
  for (int lid = 0; lid < localRows; ++lid)
    localGids[lid] = map->localToGlobalIndex(lid);

  std::vector<double> r0(localRows, 0.0);
  for (int i = 0; i < localRows; ++i)
    r0[i] = residual0->getElementByGlobalIndex(localGids[i]);

  std::vector<double> localJac(localRows * n, 0.0);

  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  Linear::Vector *xPert = ds.builder_.createVector();

  for (int j = 0; j < n; ++j)
  {
    xPert->update(1.0, *x0, 0.0);
    int gid = base + j;
    double xj = x0->getElementByGlobalIndex(gid);
    double dx = computePerturbation(xj);
    int lid = map->globalToLocalIndex(gid);
    if (lid >= 0)
      xPert->setElementByGlobalIndex(gid, xj + dx);

    Linear::Vector *rPert = computeResidualVector(xPert, period_);
    for (int i = 0; i < localRows; ++i)
    {
      double val = (rPert->getElementByGlobalIndex(localGids[i]) - r0[i]) / dx;
      localJac[i * n + j] = val;
    }
    delete rPert;
  }

  delete xPert;

  if (rank == 0)
  {
    jacobian.assign(n * n, 0.0);
    for (int i = 0; i < localRows; ++i)
    {
      int row = localGids[i] - base;
      if (row >= 0 && row < n)
      {
        for (int j = 0; j < n; ++j)
          jacobian[row * n + j] = localJac[i * n + j];
      }
    }

    for (int proc = 1; proc < numProcs; ++proc)
    {
      int count = 0;
      comm->recv(&count, 1, proc);
      if (count <= 0)
        continue;

      std::vector<int> gids(count);
      std::vector<double> vals(count * n);
      comm->recv(&gids[0], count, proc);
      comm->recv(&vals[0], count * n, proc);

      for (int i = 0; i < count; ++i)
      {
        int row = gids[i] - base;
        if (row >= 0 && row < n)
        {
          for (int j = 0; j < n; ++j)
            jacobian[row * n + j] = vals[i * n + j];
        }
      }
    }
  }
  else
  {
    int count = localRows;
    comm->send(&count, 1, 0);
    if (count > 0)
    {
      comm->send(&localGids[0], count, 0);
      comm->send(&localJac[0], count * n, 0);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computeNewtonUpdate
// Purpose       : Compute Newton update using finite difference Jacobian
// Special Notes : Solves J*delta = -r using Xyce's linear solver
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
Linear::Vector *PSS::computeNewtonUpdate(Linear::Vector *residual, Linear::Vector *x0)
{
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());

  // Adaptive damping: use smaller step if residual is large
  double residualNormResult[1];
  residual->infNorm(residualNormResult);
  double residualNorm = residualNormResult[0];
  double alpha = 0.1;
  if (residualNorm > 1.0)
    alpha = 0.01;  // Smaller step for large residuals
  else if (residualNorm < 0.1)
    alpha = 0.5;   // Larger step when close to solution

  Linear::Vector *update = ds.builder_.createVector();

  const Parallel::Communicator *comm = x0->pdsComm();
  if (!comm)
  {
    Report::UserWarning0() << "PSS: Missing communicator; using damped update.";
    update->update(-alpha, *residual, 0.0);
    return update;
  }

  const char *matrixFreeEnv = std::getenv("XYCE_PSS_MATRIX_FREE");
  bool useMatrixFree = false;
  if (matrixFreeEnv)
  {
    useMatrixFree = (std::string(matrixFreeEnv) == "1");
  }
  else if (comm->numProc() > 1)
  {
    useMatrixFree = true;
  }
  if (useMatrixFree)
  {
    if (solveNewtonSystemMatrixFree(x0, residual, update))
    {
      update->scale(alpha);
      return update;
    }
    Report::UserWarning0() << "PSS: Matrix-free solve failed; falling back to dense solve.";
  }

  const int n = x0->globalLength();
  const int base = x0->pmap()->indexBase();
  const int rank = comm->procID();

  std::vector<double> jacobian;
  if (!computeJacobianFiniteDifference(x0, residual, jacobian))
  {
    Report::UserWarning0() << "PSS: Failed to compute Jacobian; using damped update.";
    update->update(-alpha, *residual, 0.0);
    return update;
  }

  std::vector<double> delta(n, 0.0);
  int solveStatus = 0;

  if (rank == 0)
  {
    Teuchos::SerialDenseMatrix<int, double> A(n, n);
    Teuchos::SerialDenseMatrix<int, double> B(n, 1);
    Teuchos::SerialDenseMatrix<int, double> X(n, 1);

    for (int i = 0; i < n; ++i)
    {
      for (int j = 0; j < n; ++j)
      {
        A(i, j) = jacobian[i * n + j];
      }
      B(i, 0) = -residual->getElementByGlobalIndex(base + i);
    }

    Teuchos::SerialDenseSolver<int, double> solver;
    solver.setMatrix(Teuchos::rcp(&A, false));
    solver.setVectors(Teuchos::rcp(&X, false), Teuchos::rcp(&B, false));
    solver.factorWithEquilibration(true);
    solveStatus = solver.factor();
    if (solveStatus == 0)
      solveStatus = solver.solve();

    if (solveStatus == 0)
    {
      for (int i = 0; i < n; ++i)
        delta[i] = X(i, 0);
    }
  }

  comm->bcast(&solveStatus, 1, 0);
  if (solveStatus != 0)
  {
    Report::UserWarning0() << "PSS: Linear solve failed; using damped update.";
    update->update(-alpha, *residual, 0.0);
    return update;
  }

  comm->bcast(&delta[0], n, 0);

  const Parallel::ParMap *map = x0->pmap();
  const int localLength = map->numLocalEntities();
  update->putScalar(0.0);
  for (int lid = 0; lid < localLength; ++lid)
  {
    int gid = map->localToGlobalIndex(lid);
    if (gid >= base)
      update->setElementByGlobalIndex(gid, alpha * delta[gid - base]);
  }

  return update;
}

//-----------------------------------------------------------------------------
// Function      : PSS::doProcessSuccessfulStep
// Purpose       : Process successful step
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doProcessSuccessfulStep()
{
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::doProcessFailedStep
// Purpose       : Process failed step
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doProcessFailedStep()
{
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::doFinish
// Purpose       : Finish analysis
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doFinish()
{
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::doHandlePredictor
// Purpose       : Handle predictor
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doHandlePredictor()
{
  analysisManager_.getWorkingIntegrationMethod().obtainPredictor();
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::processSuccessfulDCOP
// Purpose       : Process successful DCOP
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::processSuccessfulDCOP()
{
  return false;
}

//-----------------------------------------------------------------------------
// Function      : PSS::processFailedDCOP
// Purpose       : Process failed DCOP
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::processFailedDCOP()
{
  return false;
}

//-----------------------------------------------------------------------------
// Function      : PSS::printStepHeader
// Purpose       : Print step header
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
void PSS::printStepHeader(std::ostream &os)
{
}

//-----------------------------------------------------------------------------
// Function      : PSS::printProgress
// Purpose       : Print progress
// Special Notes :
// Scope         : public
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
void PSS::printProgress(std::ostream &os)
{
}

//-----------------------------------------------------------------------------
// Parser and Factory Implementation
//-----------------------------------------------------------------------------

namespace {

//-----------------------------------------------------------------------------
// Function      : extractPSSData
// Purpose       : Extract PSS parameters from netlist
// Special Notes :
// Scope         : 
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool extractPSSData(
  IO::PkgOptionsMgr &           options_manager,
  IO::CircuitBlock &            circuit_block,
  const std::string &           netlist_filename,
  const IO::TokenVector &       parsed_line)
{
  Util::OptionBlock option_block("PSS");
  int numFields = parsed_line.size();

  // Minimum: .PSS <period>
  if (numFields < 2)
  {
    Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
      << ".PSS line requires at least PERIOD parameter";
    return false;
  }

  int linePosition = 1;
  
  // First parameter is period (required) - can be a number or expression
  option_block.addParam(Util::Param("PERIOD", parsed_line[linePosition].string_));
  ++linePosition;

  // Optional parameters
  while (linePosition < numFields)
  {
    std::string paramName = parsed_line[linePosition].string_;
    ExtendedString paramNameUpper(paramName);
    paramNameUpper.toUpper();
    paramName = paramNameUpper;

    if (paramName == "NUMPERIODS" || paramName == "NP")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS NUMPERIODS requires a value";
        return false;
      }
      if (parsed_line[linePosition].string_ == "=")
      {
        ++linePosition;
        if (linePosition >= numFields)
        {
          Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
            << ".PSS NUMPERIODS requires a value";
          return false;
        }
      }
      option_block.addParam(Util::Param("NUMPERIODS", parsed_line[linePosition].string_));
      ++linePosition;
    }
    else if (paramName == "TSTART")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS TSTART requires a value";
        return false;
      }
      if (parsed_line[linePosition].string_ == "=")
      {
        ++linePosition;
        if (linePosition >= numFields)
        {
          Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
            << ".PSS TSTART requires a value";
          return false;
        }
      }
      option_block.addParam(Util::Param("TSTART", parsed_line[linePosition].string_));
      ++linePosition;
    }
    else if (paramName == "TSTOP")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS TSTOP requires a value";
        return false;
      }
      if (parsed_line[linePosition].string_ == "=")
      {
        ++linePosition;
        if (linePosition >= numFields)
        {
          Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
            << ".PSS TSTOP requires a value";
          return false;
        }
      }
      option_block.addParam(Util::Param("TSTOP", parsed_line[linePosition].string_));
      ++linePosition;
    }
    else if (paramName == "AUTONOMOUS")
    {
      option_block.addParam(Util::Param("AUTONOMOUS", 1));
      ++linePosition;
    }
    else if (paramName == "REFNODE")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS REFNODE requires a node name";
        return false;
      }
      if (parsed_line[linePosition].string_ == "=")
      {
        ++linePosition;
        if (linePosition >= numFields)
        {
          Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
            << ".PSS REFNODE requires a node name";
          return false;
        }
      }
      option_block.addParam(Util::Param("REFNODE", parsed_line[linePosition].string_));
      ++linePosition;
    }
    else if (paramName == "TOL")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS TOL requires a value";
        return false;
      }
      if (parsed_line[linePosition].string_ == "=")
      {
        ++linePosition;
        if (linePosition >= numFields)
        {
          Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
            << ".PSS TOL requires a value";
          return false;
        }
      }
      option_block.addParam(Util::Param("TOL", parsed_line[linePosition].string_));
      ++linePosition;
    }
    else if (paramName == "MAXITER")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS MAXITER requires a value";
        return false;
      }
      if (parsed_line[linePosition].string_ == "=")
      {
        ++linePosition;
        if (linePosition >= numFields)
        {
          Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
            << ".PSS MAXITER requires a value";
          return false;
        }
      }
      option_block.addParam(Util::Param("MAXITER", parsed_line[linePosition].string_));
      ++linePosition;
    }
    else if (paramName == "STARTPERIODS")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS STARTPERIODS requires a value";
        return false;
      }
      if (parsed_line[linePosition].string_ == "=")
      {
        ++linePosition;
        if (linePosition >= numFields)
        {
          Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
            << ".PSS STARTPERIODS requires a value";
          return false;
        }
      }
      option_block.addParam(Util::Param("STARTPERIODS", parsed_line[linePosition].string_));
      ++linePosition;
    }
    else
    {
      Report::UserError0().at(netlist_filename, parsed_line[linePosition].lineNumber_)
        << "Unknown parameter in .PSS line: " << paramName;
      return false;
    }
  }

  circuit_block.addOptions(option_block);
  return true;
}

} // namespace <unnamed>

//-----------------------------------------------------------------------------
// Factory Implementation
//-----------------------------------------------------------------------------

typedef Util::Factory<AnalysisBase, PSS> PSSFactoryBase;

class PSSFactory : public PSSFactoryBase
{
public:
  PSSFactory(
    Analysis::AnalysisManager &         analysis_manager,
    Linear::System *                    linear_system_ptr,
    Nonlinear::Manager &                nonlinear_manager,
    Loader::Loader &                    loader,
    Topo::Topology &                    topology,
    IO::InitialConditionsManager &      initial_conditions_manager,
    IO::RestartMgr &                    restart_manager)
    : PSSFactoryBase(),
      analysisManager_(analysis_manager),
      linearSystemPtr_(linear_system_ptr),
      nonlinearManager_(nonlinear_manager),
      loader_(loader),
      topology_(topology),
      initialConditionsManager_(initial_conditions_manager),
      restartManager_(restart_manager)
  {}

  virtual ~PSSFactory()
  {}

  PSS *create() const
  {
    analysisManager_.setAnalysisMode(ANP_MODE_PSS);
    PSS *pss = new PSS(analysisManager_, linearSystemPtr_, nonlinearManager_, loader_, topology_, initialConditionsManager_, restartManager_);
    pss->setAnalysisParams(pssAnalysisOptionBlock_);
    // TODO: Apply time integrator options when implemented
    return pss;
  }

  AnalysisManager &getAnalysisManager()
  {
    return analysisManager_;
  }

  void setPSSAnalysisOptionBlock(const Util::OptionBlock &option_block)
  {
    pssAnalysisOptionBlock_ = option_block;
  }

  bool setTimeIntegratorOptionBlock(const Util::OptionBlock &option_block)
  {
    timeIntegratorOptionBlock_ = option_block;
    return true;
  }

private:
  Analysis::AnalysisManager &     analysisManager_;
  Linear::System *                linearSystemPtr_;
  Nonlinear::Manager &            nonlinearManager_;
  Loader::Loader &                loader_;
  Topo::Topology &                topology_;
  IO::InitialConditionsManager &  initialConditionsManager_;
  IO::RestartMgr &                restartManager_;
  Util::OptionBlock               pssAnalysisOptionBlock_;
  Util::OptionBlock               timeIntegratorOptionBlock_;
};

// .PSS
struct PSSAnalysisReg : public IO::PkgOptionsReg
{
  PSSAnalysisReg(PSSFactory &factory) : factory_(factory) {}
  
  bool operator()(const Util::OptionBlock &option_block)
  {
    factory_.setPSSAnalysisOptionBlock(option_block);
    factory_.getAnalysisManager().addAnalysis(&factory_);
    return true;
  }
  
  PSSFactory &factory_;
};

//-----------------------------------------------------------------------------
// Function      : registerPSSFactory
// Purpose       : Register PSS factory
// Special Notes :
// Scope         : 
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool registerPSSFactory(FactoryBlock & factory_block)
{
  PSSFactory *factory = new PSSFactory(factory_block.analysisManager_, &(factory_block.linearSystem_), factory_block.nonlinearManager_, factory_block.loader_, factory_block.topology_, factory_block.initialConditionsManager_, factory_block.restartManager_);

  addAnalysisFactory(factory_block, factory);

  factory_block.optionsManager_.addCommandParser(".PSS", extractPSSData);

  factory_block.optionsManager_.addCommandProcessor("PSS", new PSSAnalysisReg(*factory));

  factory_block.optionsManager_.addOptionsProcessor("TIMEINT", 
      IO::createRegistrationOptions<PSSFactory>(*factory, &PSSFactory::setTimeIntegratorOptionBlock));

  return true;
}

} // namespace Analysis
} // namespace Xyce

