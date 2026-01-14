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
#include <N_LAS_Vector.h>
#include <N_UTL_Math.h>

namespace Xyce {
namespace Analysis {

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
  analysisManager_.getStepErrorControl().currentTimeStep = period_ / 100.0; // Initial step size
  analysisManager_.getStepErrorControl().minTimeStep = period_ / 1e6; // Very small min step
  analysisManager_.getStepErrorControl().maxTimeStep = period_ / 10.0; // Reasonable max step
  analysisManager_.getStepErrorControl().finalTime = period_;
  
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
    bool integrateSuccess = integrateOnePeriod();
    
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
      stateResidualNorm = stateResidual->normInf();
      delete stateResidual;
    }
    
    if (ds.storeSize > 0)
    {
      Linear::Vector *storeResidual = ds.builder_.createStoreVector();
      storeResidual->update(1.0, *(ds.nextStorePtr), -1.0, *x0_store, 0.0);
      storeResidualNorm = storeResidual->normInf();
      delete storeResidual;
    }
    
    // Compute overall residual norm (max of all components)
    double residualNorm = residual->normInf();
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
    sec.updateTimeStep(tiaParams_);
    
    // Update coefficients for next step
    analysisManager_.getWorkingIntegrationMethod().updateCoeffs();
  }
  
  return true;
}

//-----------------------------------------------------------------------------
// Function      : PSS::computeNewtonUpdate
// Purpose       : Compute Newton update using finite difference Jacobian
// Special Notes : Simple implementation - can be optimized later
// Scope         : private
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
Linear::Vector *PSS::computeNewtonUpdate(Linear::Vector *residual, Linear::Vector *x0)
{
  // Simple damped Newton: delta = -alpha * residual
  // where alpha is a damping factor
  // TODO: Implement full Jacobian computation and linear solve
  
  TimeIntg::DataStore &ds = *(analysisManager_.getDataStore());
  Linear::Vector *update = ds.builder_.createVector();
  double alpha = 0.1; // Damping factor
  update->update(-alpha, *residual, 0.0);
  
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
    paramName.toUpper();

    if (paramName == "NUMPERIODS" || paramName == "NP")
    {
      ++linePosition;
      if (linePosition >= numFields)
      {
        Report::UserError0().at(netlist_filename, parsed_line[0].lineNumber_)
          << ".PSS NUMPERIODS requires a value";
        return false;
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
      option_block.addParam(Util::Param("REFNODE", parsed_line[linePosition].string_));
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

class PSSAnalysisReg : public IO::RegisterPkgOptionsReg
{
public:
  PSSAnalysisReg(PSSFactoryBase &factory) : factory_(factory) {}
  bool operator()(const Util::OptionBlock &option_block)
  {
    factory_.setPSSAnalysisOptionBlock(option_block);
    return true;
  }
private:
  PSSFactoryBase &factory_;
};

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

  void setPSSAnalysisOptionBlock(const Util::OptionBlock &option_block)
  {
    pssAnalysisOptionBlock_ = option_block;
  }

  void setTimeIntegratorOptionBlock(const Util::OptionBlock &option_block)
  {
    timeIntegratorOptionBlock_ = option_block;
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
      IO::createRegistrationOptions(*factory, &PSSFactory::setTimeIntegratorOptionBlock));

  return true;
}

} // namespace Analysis
} // namespace Xyce

