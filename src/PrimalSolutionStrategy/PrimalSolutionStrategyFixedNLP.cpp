/**
   The Supporting Hyperplane Optimization Toolkit (SHOT).

   @author Andreas Lundell, Åbo Akademi University

   @section LICENSE 
   This software is licensed under the Eclipse Public License 2.0. 
   Please see the README and LICENSE files for more information.
*/

#include "PrimalSolutionStrategyFixedNLP.h"

PrimalSolutionStrategyFixedNLP::PrimalSolutionStrategyFixedNLP(EnvironmentPtr envPtr)
{
    env = envPtr;

    originalNLPTime = env->settings->getDoubleSetting("FixedInteger.Frequency.Time", "Primal");
    originalNLPIter = env->settings->getIntSetting("FixedInteger.Frequency.Iteration", "Primal");

    switch (static_cast<ES_PrimalNLPSolver>(env->settings->getIntSetting("FixedInteger.Solver", "Primal")))
    {
    /*case (ES_PrimalNLPSolver::CuttingPlane):
    {
        env->process->usedPrimalNLPSolver = ES_PrimalNLPSolver::CuttingPlane;
        NLPSolver = new NLPSolverCuttingPlaneRelaxed(env);
        break;
    }*/
    case (ES_PrimalNLPSolver::Ipopt):
    {
        env->process->usedPrimalNLPSolver = ES_PrimalNLPSolver::Ipopt;
        NLPSolver = new NLPSolverIpoptRelaxed(env);
        break;
    }
#ifdef HAS_GAMS
    case (ES_PrimalNLPSolver::GAMS):
    {
        env->process->usedPrimalNLPSolver = ES_PrimalNLPSolver::GAMS;
        NLPSolver = new NLPSolverGAMS(env);
        break;
    }
#endif
    default:
        env->output->outputError("Error in solver definition for primal NLP solver. Check option 'Primal.FixedInteger.Solver'.");
        throw new ErrorClass("Error in solver definition for primal NLP solver. Check option 'Primal.FixedInteger.Solver'.");

        throw std::logic_error("Unknown PrimalNLPSolver setting.");
    }

    NLPSolver->setProblem(env->model->originalProblem->getProblemInstance());

    if (env->settings->getBoolSetting("FixedInteger.CreateInfeasibilityCut", "Primal"))
    {
        if (static_cast<ES_HyperplaneCutStrategy>(env->settings->getIntSetting("CutStrategy", "Dual")) == ES_HyperplaneCutStrategy::ESH)
        {
            if (static_cast<ES_RootsearchConstraintStrategy>(env->settings->getIntSetting(
                    "ESH.Linesearch.ConstraintStrategy", "Dual")) == ES_RootsearchConstraintStrategy::AllAsMaxFunct)
            {
                taskSelectHPPts = new TaskSelectHyperplanePointsLinesearch(env);
            }
            else
            {
                taskSelectHPPts = new TaskSelectHyperplanePointsIndividualLinesearch(env);
            }
        }
        else
        {
            taskSelectHPPts = new TaskSelectHyperplanePointsSolution(env);
        }
    }

    this->originalIterFrequency = env->settings->getIntSetting("FixedInteger.Frequency.Iteration", "Primal");
    this->originalTimeFrequency = env->settings->getDoubleSetting("FixedInteger.Frequency.Time", "Primal");
}

PrimalSolutionStrategyFixedNLP::~PrimalSolutionStrategyFixedNLP()
{
    delete taskSelectHPPts;
    delete NLPSolver;

    discreteVariableIndexes.clear();
    testedPoints.clear();
    fixPoint.clear();
}

bool PrimalSolutionStrategyFixedNLP::runStrategy()
{
    auto currIter = env->process->getCurrentIteration();

    NLPSolver->initializeProblem();

    int numVars = env->model->originalProblem->getNumberOfVariables();

    auto discreteVariableIndexes = env->model->originalProblem->getDiscreteVariableIndices();
    auto realVariableIndexes = env->model->originalProblem->getRealVariableIndices();

    bool isSolved;

    std::vector<PrimalFixedNLPCandidate> testPts;

    // Fix variables
    auto varTypes = env->model->originalProblem->getVariableTypes();

    if (env->process->primalFixedNLPCandidates.size() == 0)
    {
        env->solutionStatistics.numberOfIterationsWithoutNLPCallMIP++;
        return (false);
    }

    if (testedPoints.size() > 0)
    {
        for (int j = 0; j < env->process->primalFixedNLPCandidates.size(); j++)
        {
            for (int i = 0; i < testedPoints.size(); i++)
            {
                if (UtilityFunctions::isDifferentRoundedSelectedElements(
                        env->process->primalFixedNLPCandidates.at(j).point, testedPoints.at(i),
                        discreteVariableIndexes))
                {
                    testPts.push_back(env->process->primalFixedNLPCandidates.at(j));
                    testedPoints.push_back(env->process->primalFixedNLPCandidates.at(j).point);
                    break;
                }
            }
        }
    }
    else
    {
        testPts.push_back(env->process->primalFixedNLPCandidates.at(0));
        testedPoints.push_back(env->process->primalFixedNLPCandidates.at(0).point);
    }

    if (testPts.size() == 0)
    {
        env->solutionStatistics.numberOfIterationsWithoutNLPCallMIP++;
        return (false);
    }

    auto lbs = NLPSolver->getVariableLowerBounds();
    auto ubs = NLPSolver->getVariableUpperBounds();

    for (int j = 0; j < testPts.size(); j++)
    {
        auto oldPrimalBound = env->process->getPrimalBound();
        double timeStart = env->process->getElapsedTime("Total");
        VectorDouble fixedVariableValues(discreteVariableIndexes.size());

        int sizeOfVariableVector = NLPSolver->NLPProblem->getNumberOfVariables();

        VectorInteger startingPointIndexes(sizeOfVariableVector);
        VectorDouble startingPointValues(sizeOfVariableVector);

        // Sets the fixed values for discrete variables
        for (int k = 0; k < discreteVariableIndexes.size(); k++)
        {
            int currVarIndex = discreteVariableIndexes.at(k);

            auto tmpSolPt = UtilityFunctions::round(testPts.at(j).point.at(currVarIndex));

            fixedVariableValues.at(k) = tmpSolPt;

            // Sets the starting point to the fixed value
            if (env->settings->getBoolSetting("FixedInteger.Warmstart", "Primal"))
            {
                startingPointIndexes.at(currVarIndex) = currVarIndex;
                startingPointValues.at(currVarIndex) = tmpSolPt;
            }
        }

        if (env->settings->getBoolSetting("FixedInteger.Warmstart", "Primal"))
        {
            for (int k = 0; k < realVariableIndexes.size(); k++)
            {
                int currVarIndex = realVariableIndexes.at(k);

                if (NLPSolver->isObjectiveFunctionNonlinear() && currVarIndex == NLPSolver->getObjectiveFunctionVariableIndex())
                {
                    continue;
                }

                auto tmpSolPt = testPts.at(j).point.at(currVarIndex);

                startingPointIndexes.at(currVarIndex) = currVarIndex;
                startingPointValues.at(currVarIndex) = tmpSolPt;
            }
        }

        NLPSolver->setStartingPoint(startingPointIndexes, startingPointValues);

        NLPSolver->fixVariables(discreteVariableIndexes, fixedVariableValues);

        if (env->settings->getBoolSetting("Debug.Enable", "Output"))
        {
            std::string filename = env->settings->getStringSetting("Debug.Path", "Output") + "/primalnlp" + std::to_string(env->process->getCurrentIteration()->iterationNumber) + "_" + std::to_string(j);
            NLPSolver->saveProblemToFile(filename + ".txt");
            NLPSolver->saveOptionsToFile(filename + ".osrl");
        }

        auto solvestatus = NLPSolver->solveProblem();

        NLPSolver->unfixVariables();
        env->solutionStatistics.numberOfProblemsFixedNLP++;

        double timeEnd = env->process->getElapsedTime("Total");

        std::string sourceDesc;
        switch (testPts.at(j).sourceType)
        {
        case E_PrimalNLPSource::FirstSolution:
            sourceDesc = "SOLPT ";
            break;
        case E_PrimalNLPSource::FeasibleSolution:
            sourceDesc = "FEASPT";
            break;
        case E_PrimalNLPSource::InfeasibleSolution:
            sourceDesc = "UNFEAS";
            break;
        case E_PrimalNLPSource::SmallestDeviationSolution:
            sourceDesc = "SMADEV";
            break;
        case E_PrimalNLPSource::FirstSolutionNewDualBound:
            sourceDesc = "NEWDB";
            break;
        default:
            break;
        }

        //std::string solExpr = ((boost::format("(%.3f s)") % (timeEnd - timeStart)).str());

        if (solvestatus == E_NLPSolutionStatus::Feasible || solvestatus == E_NLPSolutionStatus::Optimal)
        {
            double tmpObj = NLPSolver->getObjectiveValue();
            auto variableSolution = NLPSolver->getSolution();

            if (env->model->originalProblem->isObjectiveFunctionNonlinear())
            {
                variableSolution.push_back(tmpObj);
            }

            auto mostDevConstr = env->model->originalProblem->getMostDeviatingConstraint(
                variableSolution);

            if (env->settings->getBoolSetting("FixedInteger.Frequency.Dynamic", "Primal"))
            {
                int iters = std::max(ceil(env->settings->getIntSetting("FixedInteger.Frequency.Iteration", "Primal") * 0.98),
                                     originalNLPIter);

                if (iters > std::max(0.1 * this->originalIterFrequency, 1.0))
                    env->settings->updateSetting("FixedInteger.Frequency.Iteration", "Primal", iters);

                double interval = std::max(
                    0.9 * env->settings->getDoubleSetting("FixedInteger.Frequency.Time", "Primal"),
                    originalNLPTime);

                if (interval > 0.1 * this->originalTimeFrequency)
                    env->settings->updateSetting("FixedInteger.Frequency.Time", "Primal", interval);

                env->process->addPrimalSolutionCandidate(variableSolution, E_PrimalSolutionSource::NLPFixedIntegers, currIter->iterationNumber);
            }

            env->report->outputIterationDetail(env->solutionStatistics.numberOfProblemsFixedNLP,
                                               ("NLP" + sourceDesc),
                                               env->process->getElapsedTime("Total"),
                                               currIter->numHyperplanesAdded,
                                               currIter->totNumHyperplanes,
                                               env->process->getDualBound(),
                                               env->process->getPrimalBound(),
                                               env->process->getAbsoluteObjectiveGap(),
                                               env->process->getRelativeObjectiveGap(),
                                               tmpObj,
                                               mostDevConstr.index,
                                               mostDevConstr.value,
                                               E_IterationLineType::PrimalNLP);
        }
        else
        {
            double tmpObj = NLPSolver->getObjectiveValue();

            // Utilize the solution point for adding a cutting plane / supporting hyperplane
            std::vector<SolutionPoint> solutionPoints(1);

            auto variableSolution = NLPSolver->getSolution();

            if (env->model->originalProblem->isObjectiveFunctionNonlinear())
            {
                variableSolution.push_back(tmpObj);
            }

            auto mostDevConstr = env->model->originalProblem->getMostDeviatingConstraint(variableSolution);

            if (env->settings->getBoolSetting("FixedInteger.CreateInfeasibilityCut", "Primal"))
            {
                SolutionPoint tmpSolPt;
                tmpSolPt.point = variableSolution;
                tmpSolPt.objectiveValue = env->model->originalProblem->calculateOriginalObjectiveValue(
                    variableSolution);
                tmpSolPt.iterFound = env->process->getCurrentIteration()->iterationNumber;
                tmpSolPt.maxDeviation = mostDevConstr;

                solutionPoints.at(0) = tmpSolPt;

                if (static_cast<ES_HyperplaneCutStrategy>(env->settings->getIntSetting(
                        "CutStrategy", "Dual")) == ES_HyperplaneCutStrategy::ESH)
                {
                    if (static_cast<ES_RootsearchConstraintStrategy>(env->settings->getIntSetting(
                            "ESH.Linesearch.ConstraintStrategy", "Dual")) == ES_RootsearchConstraintStrategy::AllAsMaxFunct)
                    {
                        static_cast<TaskSelectHyperplanePointsLinesearch *>(taskSelectHPPts)->run(solutionPoints);
                    }
                    else
                    {
                        static_cast<TaskSelectHyperplanePointsIndividualLinesearch *>(taskSelectHPPts)->run(solutionPoints);
                    }
                }
                else
                {
                    static_cast<TaskSelectHyperplanePointsSolution *>(taskSelectHPPts)->run(solutionPoints);
                }
            }

            if (env->settings->getBoolSetting("FixedInteger.Frequency.Dynamic", "Primal"))
            {
                int iters = ceil(env->settings->getIntSetting("FixedInteger.Frequency.Iteration", "Primal") * 1.02);

                if (iters < 10 * this->originalIterFrequency)
                    env->settings->updateSetting("FixedInteger.Frequency.Iteration", "Primal", iters);

                double interval = 1.1 * env->settings->getDoubleSetting("FixedInteger.Frequency.Time", "Primal");

                if (interval < 10 * this->originalTimeFrequency)
                    env->settings->updateSetting("FixedInteger.Frequency.Time", "Primal", interval);

                env->output->outputInfo(
                    "     Duration:  " + std::to_string(timeEnd - timeStart) + " s. New interval: " + std::to_string(interval) + " s or " + std::to_string(iters) + " iters.");
            }

            env->report->outputIterationDetail(env->solutionStatistics.numberOfProblemsFixedNLP,
                                               ("NLP" + sourceDesc),
                                               env->process->getElapsedTime("Total"),
                                               currIter->numHyperplanesAdded,
                                               currIter->totNumHyperplanes,
                                               env->process->getDualBound(),
                                               env->process->getPrimalBound(),
                                               env->process->getAbsoluteObjectiveGap(),
                                               env->process->getRelativeObjectiveGap(),
                                               NAN,
                                               mostDevConstr.index,
                                               mostDevConstr.value,
                                               E_IterationLineType::PrimalNLP);

            if (env->settings->getBoolSetting("HyperplaneCuts.UseIntegerCuts", "Dual") && env->model->originalProblem->getNumberOfIntegerVariables() == 0)
            {
                //Add integer cut.

                auto binVars = env->model->originalProblem->getBinaryVariableIndices();

                if (binVars.size() > 0)
                {
                    VectorInteger elements;

                    for (int i = 0; i < binVars.size(); i++)
                    {
                        if (testPts.at(j).point.at(binVars.at(i)) > 0.99)
                        {
                            elements.push_back(binVars.at(i));
                        }
                    }
                    env->process->integerCutWaitingList.push_back(elements);
                }
            }
        }

        env->solutionStatistics.numberOfIterationsWithoutNLPCallMIP = 0;
        env->solutionStatistics.timeLastFixedNLPCall = env->process->getElapsedTime("Total");
    }

    env->process->primalFixedNLPCandidates.clear();

    return (true);
}
