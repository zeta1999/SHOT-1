/**
   The Supporting Hyperplane Optimization Toolkit (SHOT).

   @author Andreas Lundell, Åbo Akademi University

   @section LICENSE
   This software is licensed under the Eclipse Public License 2.0.
   Please see the README and LICENSE files for more information.
*/

#include "MIPSolverOsiCbc.h"

namespace SHOT
{

MIPSolverOsiCbc::MIPSolverOsiCbc(EnvironmentPtr envPtr)
{
    env = envPtr;

    initializeProblem();
    checkParameters();
}

MIPSolverOsiCbc::~MIPSolverOsiCbc() {}

bool MIPSolverOsiCbc::initializeProblem()
{
    discreteVariablesActivated = true;

    if(env->reformulatedProblem->objectiveFunction->properties.isMinimize)
    {
        this->cutOff = SHOT_DBL_MAX;
    }
    else
    {
        this->cutOff = SHOT_DBL_MIN;
    }

    osiInterface = std::make_unique<OsiClpSolverInterface>();
    coinModel = std::make_unique<CoinModel>();

    cachedSolutionHasChanged = true;
    isVariablesFixed = false;

    checkParameters();
    return (true);
}

bool MIPSolverOsiCbc::addVariable(std::string name, E_VariableType type, double lowerBound, double upperBound)
{
    int index = numberOfVariables;

    try
    {
        coinModel->setColumnBounds(index, lowerBound, upperBound);
        coinModel->setColName(index, name.c_str());

        switch(type)
        {
        case E_VariableType::Real:
            break;

        case E_VariableType::Integer:
            isProblemDiscrete = true;
            coinModel->setInteger(index);
            break;

        case E_VariableType::Binary:
            isProblemDiscrete = true;
            coinModel->setInteger(index);
            break;

        case E_VariableType::Semicontinuous:
            isProblemDiscrete = true;
            coinModel->setInteger(index);
            break;

        default:
            break;
        }
    }
    catch(std::exception& e)
    {
        env->output->outputError("Cbc exception caught when adding variable to model: ", e.what());
        return (false);
    }

    variableTypes.push_back(type);
    variableNames.push_back(name);
    variableLowerBounds.push_back(lowerBound);
    variableUpperBounds.push_back(upperBound);
    numberOfVariables++;
    return (true);
}

bool MIPSolverOsiCbc::initializeObjective() { return (true); }

bool MIPSolverOsiCbc::addLinearTermToObjective(double coefficient, int variableIndex)
{
    try
    {
        coinModel->setColObjective(variableIndex, coefficient);
    }
    catch(std::exception& e)
    {
        env->output->outputError("Cbc exception caught when adding linear term to objective: ", e.what());
        return (false);
    }

    return (true);
}

bool MIPSolverOsiCbc::addQuadraticTermToObjective(double coefficient, int firstVariableIndex, int secondVariableIndex)
{
    // Not implemented
    return (false);
}

bool MIPSolverOsiCbc::finalizeObjective(bool isMinimize, double constant)
{
    try
    {
        if(constant != 0.0)
            coinModel->setObjectiveOffset(constant);

        if(isMinimize)
        {
            coinModel->setOptimizationDirection(1.0);
            isMinimizationProblem = true;
        }
        else
        {
            coinModel->setOptimizationDirection(-1.0);
            isMinimizationProblem = false;
        }
    }
    catch(std::exception& e)
    {
        env->output->outputError("Cbc exception caught when adding objective function to model: ", e.what());
        return (false);
    }

    return (true);
}

bool MIPSolverOsiCbc::initializeConstraint() { return (true); }

bool MIPSolverOsiCbc::addLinearTermToConstraint(double coefficient, int variableIndex)
{
    try
    {
        coinModel->setElement(numberOfConstraints, variableIndex, coefficient);
    }
    catch(std::exception& e)
    {
        env->output->outputError("Cbc exception caught when adding linear term to constraint: ", e.what());
        return (false);
    }

    return (true);
}

bool MIPSolverOsiCbc::addQuadraticTermToConstraint(double coefficient, int firstVariableIndex, int secondVariableIndex)
{
    // Not implemented
    return (false);
}

bool MIPSolverOsiCbc::finalizeConstraint(std::string name, double valueLHS, double valueRHS, double constant)
{
    int index = numberOfConstraints;
    try
    {
        if(valueLHS <= valueRHS)
        {
            coinModel->setRowBounds(index, valueLHS - constant, valueRHS - constant);
        }
        else
        {
            coinModel->setRowBounds(index, valueRHS - constant, valueLHS - constant);
        }

        coinModel->setRowName(index, name.c_str());
    }
    catch(std::exception& e)
    {
        env->output->outputError("Cbc exception caught when adding constraint to model: ", e.what());
        return (false);
    }

    numberOfConstraints++;
    return (true);
}

bool MIPSolverOsiCbc::finalizeProblem()
{
    try
    {
        osiInterface->loadFromCoinModel(*coinModel);
        cbcModel = std::make_unique<CbcModel>(*osiInterface);
        CbcMain0(*cbcModel);

        if(!env->settings->getBoolSetting("Console.DualSolver.Show", "Output"))
        {
            cbcModel->setLogLevel(0);
            osiInterface->setHintParam(OsiDoReducePrint, false, OsiHintTry);
        }

        setSolutionLimit(1);
    }
    catch(std::exception& e)
    {
        env->output->outputError("Cbc exception caught when finalizing model", e.what());
        return (false);
    }

    return (true);
}

void MIPSolverOsiCbc::initializeSolverSettings()
{
    if(cbcModel->haveMultiThreadSupport())
    {
        cbcModel->setNumberThreads(env->settings->getIntSetting("MIP.NumberOfThreads", "Dual"));
    }

    cbcModel->setAllowableGap(env->settings->getDoubleSetting("ObjectiveGap.Absolute", "Termination") / 1.0);
    cbcModel->setAllowableFractionGap(env->settings->getDoubleSetting("ObjectiveGap.Absolute", "Termination") / 1.0);
    cbcModel->setMaximumSolutions(solLimit);
    cbcModel->setMaximumSavedSolutions(env->settings->getIntSetting("MIP.SolutionPool.Capacity", "Dual"));

    // Cbc has problems with too large cutoff values
    if(isMinimizationProblem && abs(this->cutOff) < 10e20)
    {
        cbcModel->setCutoff(this->cutOff);

        env->output->outputInfo("     Setting cutoff value to " + std::to_string(cutOff) + " for minimization.");
    }
    else if(!isMinimizationProblem && abs(this->cutOff) < 10e20)
    {
        cbcModel->setCutoff(this->cutOff);
        env->output->outputInfo("     Setting cutoff value to " + std::to_string(cutOff) + " for maximization.");
    }
}

int MIPSolverOsiCbc::addLinearConstraint(
    const std::vector<PairIndexValue>& elements, double constant, bool isGreaterThan)
{
    CoinPackedVector cut;

    for(int i = 0; i < elements.size(); i++)
    {
        cut.insert(elements.at(i).index, elements.at(i).value);
    }

    // Adds the cutting plane
    if(isGreaterThan)
        osiInterface->addRow(cut, -constant, osiInterface->getInfinity());
    else
        osiInterface->addRow(cut, -osiInterface->getInfinity(), -constant);

    return (osiInterface->getNumRows() - 1);
}

void MIPSolverOsiCbc::activateDiscreteVariables(bool activate)
{
    if(activate)
    {
        env->output->outputInfo("Activating MIP strategy");

        for(int i = 0; i < numberOfVariables; i++)
        {
            if(variableTypes.at(i) == E_VariableType::Integer || variableTypes.at(i) == E_VariableType::Binary)
            {
                osiInterface->setInteger(i);
            }
        }

        discreteVariablesActivated = true;
    }
    else
    {
        env->output->outputInfo("Activating LP strategy");
        for(int i = 0; i < numberOfVariables; i++)
        {
            if(variableTypes.at(i) == E_VariableType::Integer || variableTypes.at(i) == E_VariableType::Binary)
            {
                osiInterface->setContinuous(i);
            }
        }

        discreteVariablesActivated = false;
    }
}

E_ProblemSolutionStatus MIPSolverOsiCbc::getSolutionStatus()
{
    E_ProblemSolutionStatus MIPSolutionStatus;

    if(cbcModel->isProvenOptimal())
    {
        MIPSolutionStatus = E_ProblemSolutionStatus::Optimal;
    }
    else if(cbcModel->isProvenInfeasible())
    {
        MIPSolutionStatus = E_ProblemSolutionStatus::Infeasible;
    }
    else if(cbcModel->isSolutionLimitReached())
    {
        MIPSolutionStatus = E_ProblemSolutionStatus::SolutionLimit;
    }
    else if(cbcModel->isSecondsLimitReached())
    {
        MIPSolutionStatus = E_ProblemSolutionStatus::TimeLimit;
    }
    else if(cbcModel->isNodeLimitReached())
    {
        MIPSolutionStatus = E_ProblemSolutionStatus::NodeLimit;
    }
    else
    {
        env->output->outputError("MIP solver return status unknown.");
    }

    return (MIPSolutionStatus);
}

E_ProblemSolutionStatus MIPSolverOsiCbc::solveProblem()
{
    E_ProblemSolutionStatus MIPSolutionStatus;
    cachedSolutionHasChanged = true;

    try
    {
        cbcModel = std::make_unique<CbcModel>(*osiInterface);

        initializeSolverSettings();

        // Adding the MIP starts provided
        for(auto it = MIPStarts.begin(); it != MIPStarts.end(); ++it)
        {
            cbcModel->setMIPStart(*it);
        }

        MIPStarts.clear();

        CbcMain0(*cbcModel);

        if(!env->settings->getBoolSetting("Console.DualSolver.Show", "Output"))
        {
            cbcModel->setLogLevel(0);
            osiInterface->setHintParam(OsiDoReducePrint, false, OsiHintTry);
        }

        cbcModel->branchAndBound();

        MIPSolutionStatus = getSolutionStatus();
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when solving subproblem with Cbc", e.what());
        MIPSolutionStatus = E_ProblemSolutionStatus::Error;
    }

    return (MIPSolutionStatus);
}

int MIPSolverOsiCbc::increaseSolutionLimit(int increment)
{
    this->solLimit += increment;

    this->setSolutionLimit(this->solLimit);

    return (this->solLimit);
}

void MIPSolverOsiCbc::setSolutionLimit(long int limit) { this->solLimit = limit; }

int MIPSolverOsiCbc::getSolutionLimit() { return (this->solLimit); }

void MIPSolverOsiCbc::setTimeLimit(double seconds)
{
    try
    {
        cbcModel->setMaximumSeconds(seconds);
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when setting time limit in Cbc", e.what());
    }
}

void MIPSolverOsiCbc::setCutOff(double cutOff)
{
    double cutOffTol = env->settings->getDoubleSetting("MIP.CutOffTolerance", "Dual");

    try
    {

        if(isMinimizationProblem)
        {
            this->cutOff = cutOff + cutOffTol;

            env->output->outputInfo(
                "     Setting cutoff value to " + std::to_string(this->cutOff) + " for minimization.");
        }
        else
        {
            this->cutOff = cutOff - cutOffTol;

            env->output->outputInfo(
                "     Setting cutoff value to " + std::to_string(this->cutOff) + " for maximization.");
        }
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when setting cut off value", e.what());
    }
}

void MIPSolverOsiCbc::setCutOffAsConstraint(double cutOff) {}

void MIPSolverOsiCbc::addMIPStart(VectorDouble point)
{
    std::vector<std::pair<std::string, double>> variableValues;

    for(int i = 0; i < point.size(); i++)
    {
        std::pair<std::string, double> tmpPair;

        tmpPair.first = variableNames.at(i);
        tmpPair.second = point.at(i);

        variableValues.push_back(tmpPair);
    }
    try
    {
        if(env->dualSolver->MIPSolver->hasAuxilliaryObjectiveVariable())
        {
            std::pair<std::string, double> tmpPair;

            tmpPair.first = variableNames.back();
            tmpPair.second = env->results->getPrimalBound();

            variableValues.push_back(tmpPair);
        }

        MIPStarts.push_back(variableValues);
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when adding MIP start to Cbc", e.what());
    }
}

void MIPSolverOsiCbc::writeProblemToFile(std::string filename)
{
    try
    {
        osiInterface->writeLp(filename.c_str(), "");
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when saving model to file in Cbc", e.what());
    }
}

double MIPSolverOsiCbc::getObjectiveValue(int solIdx)
{
    bool isMIP = getDiscreteVariableStatus();

    double objVal = NAN;

    if(!isMIP && solIdx > 0) // LP problems only have one solution!
    {
        env->output->outputError(
            "Cannot obtain solution with index " + std::to_string(solIdx) + " in Cbc since the problem is LP/QP!");

        return (objVal);
    }

    try
    {
        // Fixes some strange behavior with the objective value when solving MIP vs LP problems
        if(isMIP && isMinimizationProblem)
        {
            objVal = 1.0;
        }
        else if(isMIP && !isMinimizationProblem)
        {
            objVal = -1.0;
        }
        else
        {
            objVal = 1.0;
        }

        if(isMIP)
        {
            objVal *= cbcModel->savedSolutionObjective(solIdx);
        }
        else
        {
            objVal *= cbcModel->getObjValue();
        }
    }
    catch(std::exception& e)
    {
        env->output->outputError(
            "Error when obtaining objective value for solution index " + std::to_string(solIdx) + " in Cbc", e.what());
    }

    return (objVal);
}

void MIPSolverOsiCbc::deleteMIPStarts()
{
    // Not implemented
}

VectorDouble MIPSolverOsiCbc::getVariableSolution(int solIdx)
{
    bool isMIP = getDiscreteVariableStatus();
    int numVar = cbcModel->getNumCols();
    VectorDouble solution(numVar);

    try
    {
        if(isMIP)
        {
            auto tmpSol = cbcModel->savedSolution(solIdx);
            for(int i = 0; i < numVar; i++)
            {
                solution.at(i) = tmpSol[i];
            }
        }
        else
        {
            auto tmpSol = cbcModel->bestSolution();

            for(int i = 0; i < numVar; i++)
            {
                solution.at(i) = tmpSol[i];
            }
        }
    }
    catch(std::exception& e)
    {
        env->output->outputError(
            "Error when reading solution with index " + std::to_string(solIdx) + " in Cbc", e.what());
    }
    return (solution);
}

int MIPSolverOsiCbc::getNumberOfSolutions()
{
    int numSols = 0;
    bool isMIP = getDiscreteVariableStatus();

    try
    {
        if(isMIP)
            numSols = cbcModel->getSolutionCount();
        else
            numSols = 1;
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when obtaining number of solutions in Cbc", e.what());
    }

    return (numSols);
}

void MIPSolverOsiCbc::fixVariable(int varIndex, double value) { updateVariableBound(varIndex, value, value); }

void MIPSolverOsiCbc::updateVariableBound(int varIndex, double lowerBound, double upperBound)
{
    try
    {
        osiInterface->setColBounds(varIndex, lowerBound, upperBound);
    }
    catch(std::exception& e)
    {
        env->output->outputError(
            "Error when updating variable bounds for variable index" + std::to_string(varIndex) + " in Cbc", e.what());
    }
}

void MIPSolverOsiCbc::updateVariableLowerBound(int varIndex, double lowerBound)
{
    try
    {
        osiInterface->setColLower(varIndex, lowerBound);
    }
    catch(std::exception& e)
    {
        env->output->outputError(
            "Error when updating variable bounds for variable index" + std::to_string(varIndex) + " in Cbc", e.what());
    }
}

void MIPSolverOsiCbc::updateVariableUpperBound(int varIndex, double upperBound)
{
    try
    {
        osiInterface->setColUpper(varIndex, upperBound);
    }
    catch(std::exception& e)
    {
        env->output->outputError(
            "Error when updating variable bounds for variable index" + std::to_string(varIndex) + " in Cbc", e.what());
    }
}

PairDouble MIPSolverOsiCbc::getCurrentVariableBounds(int varIndex)
{
    PairDouble tmpBounds;

    try
    {
        tmpBounds.first = osiInterface->getColLower()[varIndex];
        tmpBounds.second = osiInterface->getColUpper()[varIndex];
    }
    catch(std::exception& e)
    {
        env->output->outputError(
            "Error when obtaining variable bounds for variable index" + std::to_string(varIndex) + " in Cbc", e.what());
    }

    return (tmpBounds);
}

bool MIPSolverOsiCbc::supportsQuadraticObjective() { return (false); }

bool MIPSolverOsiCbc::supportsQuadraticConstraints() { return (false); }

double MIPSolverOsiCbc::getDualObjectiveValue()
{
    double objVal = NAN;

    try
    {
        objVal = cbcModel->getBestPossibleObjValue();
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when obtaining dual objective value in Cbc", e.what());
    }

    return (objVal);
}

std::pair<VectorDouble, VectorDouble> MIPSolverOsiCbc::presolveAndGetNewBounds()
{
    return (std::make_pair(variableLowerBounds, variableUpperBounds));
}

void MIPSolverOsiCbc::writePresolvedToFile(std::string filename)
{
    // Not implemented
}

void MIPSolverOsiCbc::checkParameters()
{
    env->settings->updateSetting("MIP.NumberOfThreads", "Dual", 1);

    // Some features are not available in Cbc
    env->settings->updateSetting("TreeStrategy", "Dual", static_cast<int>(ES_TreeStrategy::MultiTree));
    env->settings->updateSetting(
        "Reformulation.Quadratics.Strategy", "Model", static_cast<int>(ES_QuadraticProblemStrategy::Nonlinear));

    // For stability
    env->settings->updateSetting("Tolerance.TrustLinearConstraintValues", "Primal", false);
}

int MIPSolverOsiCbc::getNumberOfExploredNodes()
{
    try
    {
        return (cbcModel->getNodeCount());
    }
    catch(std::exception& e)
    {
        env->output->outputError("Error when getting number of explored nodes", e.what());
        return 0;
    }
}
} // namespace SHOT