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

//-----------------------------------------------------------------------------
//
// Purpose        : PSS (Periodic Steady State) analysis class
//
// Special Notes  : Implements shooting method for periodic steady state analysis
//                  Supports both driven and autonomous modes
//
// Creator        : 
//
// Creation Date  : 
//
//
//-----------------------------------------------------------------------------

#ifndef Xyce_N_ANP_PSS_h
#define Xyce_N_ANP_PSS_h

#include <N_ANP_fwd.h>
#include <N_IO_fwd.h>
#include <N_LAS_fwd.h>
#include <N_TOP_fwd.h>
#include <N_UTL_fwd.h>

#include <N_ANP_AnalysisBase.h>
#include <N_ANP_RegisterAnalysis.h>
#include <N_ANP_StepEvent.h>
#include <N_TIA_TIAParams.h>
#include <N_UTL_Listener.h>
#include <N_UTL_OptionBlock.h>

namespace Xyce {
namespace Analysis {

typedef Util::ListenerAutoSubscribe<StepEvent> StepEventListener;

//-------------------------------------------------------------------------
// Class         : PSS
// Purpose       : PSS analysis class
// Special Notes :
// Creator       : 
// Creation Date : 
//-------------------------------------------------------------------------
class PSS : public AnalysisBase, public StepEventListener
{
public:
  PSS(
    AnalysisManager &                   analysis_manager,
    Linear::System *                    linear_system_ptr,
    Nonlinear::Manager &                nonlinear_manager,
    Loader::Loader &                    loader,
    Topo::Topology &                    topology,
    IO::InitialConditionsManager &      initial_conditions_manager,
    IO::RestartMgr &                    restart_manager);

  virtual ~PSS();

  void notify(const StepEvent &event);

  void setTIAParams(const TimeIntg::TIAParams &tia_params)
  {
    tiaParams_ = tia_params;
  }

  const TimeIntg::TIAParams &getTIAParams() const override
  {
    return tiaParams_;
  }

  TimeIntg::TIAParams &getTIAParams() override
  {
    return tiaParams_;
  }

  bool setAnalysisParams(const Util::OptionBlock & paramsBlock);

  void registerLinearSystem(Linear::System * linear_system_ptr)
  {
    linearSystemPtr_ = linear_system_ptr;
  }

  void stepCallback();
  void registerParentAnalysis(AnalysisBase * parentPtr)
  {
    parentAnalysisPtrVec_.push_back(parentPtr);
  };

  void finalExpressionBasedSetup() override;

  bool getDCOPFlag() const override
  {
    return false;  // PSS does not require DCOP
  }

protected:
  virtual bool doRun();
  virtual bool doInit();
  virtual bool doLoopProcess();
  virtual bool doProcessSuccessfulStep();
  virtual bool doProcessFailedStep();
  virtual bool doFinish();
  virtual bool doHandlePredictor();

public:
  bool processSuccessfulDCOP();
  bool processFailedDCOP();

  void printStepHeader(std::ostream &os);
  void printProgress(std::ostream &os);

private:
  // Helper functions for shooting method
  bool integrateOnePeriod();
  bool integrateOnePeriod(double period); // Overload for autonomous mode with variable period
  Linear::Vector *computeNewtonUpdate(Linear::Vector *residual, Linear::Vector *x0);
  Linear::Vector *computeNewtonUpdateAutonomous(Linear::Vector *residual, Linear::Vector *x0, double &periodUpdate);
  bool computeJacobianFiniteDifference(Linear::Vector *x0, Linear::Vector *residual0, std::vector<double> &jacobian);
  bool computeJacobianFiniteDifferenceAutonomous(Linear::Vector *x0, double period, Linear::Vector *residual0,
                                                 double phaseCondition, std::vector<double> &jacobian);
  Linear::Vector *computeResidualVector(Linear::Vector *x0);
  Linear::Vector *computeResidualVector(Linear::Vector *x0, double period);
  double computePerturbation(double value) const;
  bool solveNewtonSystemMatrixFree(Linear::Vector *x0, Linear::Vector *residual, Linear::Vector *update);
  bool solveNewtonSystemMatrixFreeAutonomous(Linear::Vector *x0, Linear::Vector *residual, double phaseCondition,
                                             double period, Linear::Vector *update, double &periodUpdate);
  double computePhaseCondition(Linear::Vector *x0); // Phase condition: dV/dt = 0 at refNode
  AnalysisManager &     analysisManager_;
  Loader::Loader &      loader_;
  Linear::System *      linearSystemPtr_;
  Nonlinear::Manager &  nonlinearManager_;
  Topo::Topology &      topology_;
  IO::InitialConditionsManager & initialConditionsManager_;
  IO::RestartMgr &      restartManager_;

  TimeIntg::TIAParams   tiaParams_;

  // PSS parameters
  double                period_;
  bool                  periodGiven_;
  int                   numPeriods_;
  bool                  numPeriodsGiven_;
  double                tStart_;
  bool                  tStartGiven_;
  double                tStop_;
  bool                  tStopGiven_;
  bool                  autonomousMode_;
  std::string           refNode_;
  bool                  refNodeGiven_;
  int                   maxIterations_;
  double                tolerance_;
  int                   startUpPeriods_;
  bool                  startUpPeriodsGiven_;
  bool                  matrixFree_;
  int                   gmresRestart_;
  int                   gmresMaxIter_;
  bool                  gmresLog_;
  bool                  gmresPrecondDiag_;
  int                   gmresPrecondMaxN_;

  std::vector<AnalysisBase *> parentAnalysisPtrVec_;
};

bool registerPSSFactory(FactoryBlock &factory_block);

} // namespace Analysis
} // namespace Xyce

#endif // Xyce_N_ANP_PSS_h

