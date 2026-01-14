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
// Purpose       : Main analysis loop
// Special Notes :
// Scope         : protected
// Creator       : 
// Creation Date : 
//-----------------------------------------------------------------------------
bool PSS::doLoopProcess()
{
  // TODO: Implement shooting method integration with Xyce
  // For now, just return success
  return true;
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

