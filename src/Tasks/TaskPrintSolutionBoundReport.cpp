﻿/**
   The Supporting Hyperplane Optimization Toolkit (SHOT).

   @author Andreas Lundell, Åbo Akademi University

   @section LICENSE
   This software is licensed under the Eclipse Public License 2.0.
   Please see the README and LICENSE files for more information.
*/

#include "TaskPrintSolutionBoundReport.h"
namespace SHOT
{

TaskPrintSolutionBoundReport::TaskPrintSolutionBoundReport(EnvironmentPtr envPtr)
    : TaskBase(envPtr)
{
    itersSinceLastPrintout = 0;
    timeLastPrintout = 0;
}

TaskPrintSolutionBoundReport::~TaskPrintSolutionBoundReport() {}

void TaskPrintSolutionBoundReport::run()
{
    double currElapsedTime = env->timing->getElapsedTime("Total");

    if(itersSinceLastPrintout > 20 || currElapsedTime - timeLastPrintout > 10.0)
    {
        double absGap = env->results->getAbsoluteObjectiveGap();
        double relGap = env->results->getRelativeObjectiveGap();

        double dualBound = env->results->getDualBound();
        double primalBound = env->results->getPrimalBound();

        env->output->outputSummary(
            "                                                                                     ");

#ifdef _WIN32
        env->output->outputSummary(
            "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
#else
        env->output->outputSummary(
            "─────────────────────────────────────────────────────────────────────────────────────");
#endif

        auto tmpLine = boost::format(" At %1% s the obj. bound is %|24t|[%2%, %3%] %|46t|with abs/rel gap %4% / %5%")
            % env->timing->getElapsedTime("Total") % UtilityFunctions::toStringFormat(dualBound, "%.3f", true)
            % UtilityFunctions::toStringFormat(primalBound, "%.3f", true)
            % UtilityFunctions::toStringFormat(absGap, "%.4f", true)
            % UtilityFunctions::toStringFormat(relGap, "%.4f", true);
        env->output->outputSummary(tmpLine.str());

        if(env->solutionStatistics.numberOfConstraintsRemovedInPresolve > 0
            || env->solutionStatistics.numberOfVariableBoundsTightenedInPresolve > 0)
        {
            tmpLine = boost::format(" Presolve: %1% constraint(s) removed and %2% variable bounds tightened.")
                % env->solutionStatistics.numberOfConstraintsRemovedInPresolve
                % env->solutionStatistics.numberOfVariableBoundsTightenedInPresolve;
            env->output->outputSummary(tmpLine.str());
        }

        if(env->dualSolver->MIPSolver->interiorPts.size() > 1)
        {
            env->output->outputSummary(
                " Number of interior points: " + std::to_string(env->dualSolver->MIPSolver->interiorPts.size()));
        }

        if(env->solutionStatistics.numberOfIntegerCuts > 0)
        {
            env->output->outputSummary(
                " Number of integer cuts added: " + std::to_string(env->solutionStatistics.numberOfIntegerCuts));
        }

#ifdef _WIN32
        env->output->outputSummary(
            "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ");
#else
        env->output->outputSummary(
            "─────────────────────────────────────────────────────────────────────────────────────");
#endif

        env->output->outputSummary("");

        itersSinceLastPrintout = 0;
        timeLastPrintout = currElapsedTime;
    }
    else
    {
        itersSinceLastPrintout++;
    }
}

std::string TaskPrintSolutionBoundReport::getType()
{
    std::string type = typeid(this).name();
    return (type);
}
} // namespace SHOT