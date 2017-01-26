/*
 * TaskCheckConstraintTolerance.cpp
 *
 *  Created on: Mar 27, 2015
 *      Author: alundell
 */

#include <TaskCheckConstraintTolerance.h>

TaskCheckConstraintTolerance::TaskCheckConstraintTolerance(std::string taskIDTrue)
{
	processInfo = ProcessInfo::getInstance();
	settings = SHOTSettings::Settings::getInstance();
	taskIDIfTrue = taskIDTrue;
}

TaskCheckConstraintTolerance::~TaskCheckConstraintTolerance()
{
	// TODO Auto-generated destructor stub
}

void TaskCheckConstraintTolerance::run()
{
	auto currIter = processInfo->getCurrentIteration();

	if (currIter->maxDeviation < settings->getDoubleSetting("ConstrTermTolMILP", "Algorithm")
			&& currIter->solutionStatus == E_ProblemSolutionStatus::Optimal
			&& currIter->type == E_IterationProblemType::MIP)
	{
		processInfo->terminationReason = E_TerminationReason::ConstraintTolerance;
		processInfo->tasks->setNextTask(taskIDIfTrue);
	}

	return;
}

std::string TaskCheckConstraintTolerance::getType()
{
	std::string type = typeid(this).name();
	return (type);

}
