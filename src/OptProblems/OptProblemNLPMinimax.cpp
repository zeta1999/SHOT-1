#include "OptProblemNLPMinimax.h"
#include "OSExpressionTree.h"
#include "vector"

OptProblemNLPMinimax::OptProblemNLPMinimax()
{
	//problemInstance = NULL;

}

OptProblemNLPMinimax::~OptProblemNLPMinimax()
{
	//delete problemInstance;
}

void OptProblemNLPMinimax::reformulate(OSInstance *originalInstance)
{
	OSInstance *newInstance = NULL;
	newInstance = new OSInstance();

	std::cout << "h221" << std::endl;
	this->setObjectiveFunctionNonlinear(isConstraintNonlinear(originalInstance, -1));

	std::cout << "h222" << std::endl;
	this->setTypeOfObjectiveMinimize(true /*originalInstance->instanceData->objectives->obj[0]->maxOrMin == "min"*/);

	std::cout << "h223" << std::endl;
	this->copyVariables(originalInstance, newInstance, true);

	this->copyObjectiveFunction(originalInstance, newInstance);

	this->copyConstraints(originalInstance, newInstance);

	this->copyLinearTerms(originalInstance, newInstance);

	this->copyQuadraticTerms(originalInstance, newInstance);

	this->copyNonlinearExpressions(originalInstance, newInstance);

	this->setProblemInstance(newInstance);

	this->setNonlinearConstraintIndexes();

	if (this->isObjectiveFunctionNonlinear())
	{
		int tmpVal = originalInstance->getConstraintNumber();

		setNonlinearObjectiveConstraintIdx(tmpVal);	// Sets a virtual constraint

		setNonlinearObjectiveVariableIdx(originalInstance->getVariableNumber());
		muindex = originalInstance->getVariableNumber() + 1;
	}
	else
	{
		muindex = originalInstance->getVariableNumber();
	}
}

void OptProblemNLPMinimax::copyVariables(OSInstance *source, OSInstance *destination, bool integerRelaxed)
{
	int numVar = source->getVariableNumber();

	if (this->isObjectiveFunctionNonlinear())
	{
		destination->setVariableNumber(numVar + 2);
	}
	else
	{
		destination->setVariableNumber(numVar + 1);
	}

	vector < std::string > varNames;
	varNames.assign(source->getVariableNames(), source->getVariableNames() + numVar);

	vector<char> varTypes;
	varTypes.assign(source->getVariableTypes(), source->getVariableTypes() + numVar);

	vector<double> varLBs;
	varLBs.assign(source->getVariableLowerBounds(), source->getVariableLowerBounds() + numVar);

	vector<double> varUBs;
	varUBs.assign(source->getVariableUpperBounds(), source->getVariableUpperBounds() + numVar);

	/*auto varTypes = source->getVariableTypes();
	 auto varLBs = source->getVariableLowerBounds();
	 auto varUBs = source->getVariableUpperBounds();
	 */
	//int numVar = source->instanceData->variables->numberOfVariables;
	if (destination->getVariableNumber() == 0)
	{
		destination->setVariableNumber(numVar);
	}

	if (integerRelaxed)
	{
		for (int i = 0; i < numVar; i++)
		{
			std::string name = varNames.at(i);
			double lb = varLBs.at(i);
			double ub = varUBs.at(i);
			char type = 'C';
			/*
			 if (lb < -1000000000000)
			 {
			 lb = -10000000000;
			 }

			 if (ub > 10000000000000)
			 {
			 ub = 10000000000;
			 }
			 */
			destination->addVariable(i, name, lb, ub, type);
		}
	}
	else
	{
		for (int i = 0; i < numVar; i++)
		{

			std::string name = varNames.at(i);
			double lb = varLBs.at(i);
			double ub = varUBs.at(i);
			char type = varTypes.at(i);

			destination->addVariable(i, name, lb, ub, type);

		}
	}

	double tmpObjBound = Settings::getInstance().getDoubleSetting("MinimaxObjectiveBound", "InteriorPoint");
	double tmpObjUpperBound = Settings::getInstance().getDoubleSetting("MinimaxUpperBound", "InteriorPoint");

	if (this->isObjectiveFunctionNonlinear())
	{
		destination->addVariable(numVar, "mu", -tmpObjBound, tmpObjBound, 'C');
		destination->addVariable(numVar + 1, "tempobjvar", -tmpObjBound, tmpObjUpperBound, 'C');
	}
	else
	{
		destination->addVariable(numVar, "tempobjvar", -tmpObjBound, tmpObjUpperBound, 'C');
	}

	destination->bVariablesModified = true;
}

void OptProblemNLPMinimax::copyObjectiveFunction(OSInstance *source, OSInstance *destination)
{
	int numVar = source->getVariableNumber();

	destination->setObjectiveNumber(1);

	SparseVector * newobjcoeff = new SparseVector(1);

	if (this->isObjectiveFunctionNonlinear())
	{
		newobjcoeff->indexes[0] = numVar + 1;
	}
	else
	{
		newobjcoeff->indexes[0] = numVar;
	}

	newobjcoeff->values[0] = 1.0;

	destination->addObjective(-1, "newobj", "min", 0.0, 1.0, newobjcoeff);
	delete newobjcoeff;

	//destination->bObjectivesModified = true;
}

void OptProblemNLPMinimax::copyConstraints(OSInstance *source, OSInstance *destination)
{
	int numCon = source->getConstraintNumber();

	if (this->isObjectiveFunctionNonlinear())
	{
		destination->setConstraintNumber(numCon + 1);
	}
	else
	{
		destination->setConstraintNumber(numCon);
	}

	for (int i = 0; i < numCon; i++)
	{
		std::string name = source->instanceData->constraints->con[i]->name;
		double lb = source->instanceData->constraints->con[i]->lb;
		double ub = source->instanceData->constraints->con[i]->ub;
		double constant = source->instanceData->constraints->con[i]->constant;

		destination->addConstraint(i, name, lb, ub, constant);
	}

	if (this->isObjectiveFunctionNonlinear())
	{
		//double tmpObjBound = Settings::getInstance().getDoubleSetting("MinimaxObjectiveBound", "InteriorPoint");
		destination->addConstraint(numCon, "objconstr", -OSDBL_MAX, -source->instanceData->objectives->obj[0]->constant,
				0.0);
	}

	//destination->bConstraintsModified = true;
}

void OptProblemNLPMinimax::copyLinearTerms(OSInstance *source, OSInstance *destination)
{
	int row_nonz = 0;
	int obj_nonz = 0;
	int varIdx = 0;

	int rowNum = source->getConstraintNumber();
	int varNum = source->getVariableNumber();
	int elemNum = source->getLinearConstraintCoefficientNumber();

	int nonlinConstrs = getNumberOfNonlinearConstraints(source);

	SparseMatrix *m_linearConstraintCoefficientsInRowMajor = source->getLinearConstraintCoefficientsInRowMajor();

	/*int *rowIndices, *colIndices;
	 double * elements;*/

	int numTotalElements = elemNum + nonlinConstrs;

	if (this->isObjectiveFunctionNonlinear())
	{
		numTotalElements = numTotalElements + 2 + source->instanceData->objectives->obj[0]->numberOfObjCoef;
	}

	std::vector<int> rowIndices;
	std::vector<int> colIndices;
	std::vector<double> elements;

	rowIndices.reserve(numTotalElements);
	colIndices.reserve(numTotalElements);
	elements.reserve(numTotalElements);

	for (int rowIdx = 0; rowIdx < rowNum; rowIdx++)
	{
		if (m_linearConstraintCoefficientsInRowMajor != NULL && m_linearConstraintCoefficientsInRowMajor->valueSize > 0)
		{
			row_nonz = m_linearConstraintCoefficientsInRowMajor->starts[rowIdx + 1]
					- m_linearConstraintCoefficientsInRowMajor->starts[rowIdx];

			for (int j = 0; j < row_nonz; j++)
			{
				varIdx =
						m_linearConstraintCoefficientsInRowMajor->indexes[m_linearConstraintCoefficientsInRowMajor->starts[rowIdx]
								+ j];

				double tmpVal =
						m_linearConstraintCoefficientsInRowMajor->values[m_linearConstraintCoefficientsInRowMajor->starts[rowIdx]
								+ j];

				rowIndices.push_back(rowIdx);
				colIndices.push_back(
						m_linearConstraintCoefficientsInRowMajor->indexes[m_linearConstraintCoefficientsInRowMajor->starts[rowIdx]
								+ j]);
				elements.push_back(
						m_linearConstraintCoefficientsInRowMajor->values[m_linearConstraintCoefficientsInRowMajor->starts[rowIdx]
								+ j]);
			}
		}
		// Inserts the objective function variable into nonlinear constraints
		if (isConstraintNonlinear(source, rowIdx))
		{
			rowIndices.push_back(rowIdx);

			if (this->isObjectiveFunctionNonlinear())
			{
				colIndices.push_back(varNum + 1);
			}
			else
			{
				colIndices.push_back(varNum);
			}

			if (source->getConstraintTypes()[rowIdx] == 'L')
			{
				elements.push_back(-1.0);
			}
			else if (source->getConstraintTypes()[rowIdx] == 'G')
			{
				elements.push_back(1.0);
			}
		}
	}

	if (this->isObjectiveFunctionNonlinear())
	{
		auto objCoeffs = source->getObjectiveCoefficients()[0];
		int numObjCoeffs = source->getObjectiveCoefficientNumbers()[0];

		for (int i = 0; i < numObjCoeffs; i++)
		{
			rowIndices.push_back(rowNum);
			colIndices.push_back(objCoeffs->indexes[i]);
			elements.push_back(objCoeffs->values[i]);
		}

		rowIndices.push_back(rowNum);
		colIndices.push_back(varNum);
		elements.push_back(-1.0);

		rowIndices.push_back(rowNum);
		colIndices.push_back(varNum + 1);
		elements.push_back(-1.0);
	}

	CoinPackedMatrix *matrix = new CoinPackedMatrix(false, &rowIndices.at(0), &colIndices.at(0), &elements.at(0),
			numTotalElements);

	int numnonz = matrix->getNumElements();
	int valuesBegin = 0;
	int valuesEnd = numnonz - 1;
	int startsBegin = 0;
	int indexesBegin = 0;
	int indexesEnd = numnonz - 1;

	int startsEnd = this->isObjectiveFunctionNonlinear() ? varNum + 1 : varNum;

	auto tmp = const_cast<int*>(matrix->getVectorStarts());

	destination->setLinearConstraintCoefficients(numnonz, matrix->isColOrdered(),
			const_cast<double*>(matrix->getElements()), valuesBegin, valuesEnd, const_cast<int*>(matrix->getIndices()),
			indexesBegin, indexesEnd, const_cast<int*>(matrix->getVectorStarts()), startsBegin, startsEnd);

	//destination->bConstraintsModified = true;

}

void OptProblemNLPMinimax::copyQuadraticTerms(OSInstance *source, OSInstance *destination)
{
	int numQuadTerms = source->getNumberOfQuadraticTerms();

	if (numQuadTerms > 0)
	{
		auto quadTerms = source->getQuadraticTerms();
		auto varRowIndexes = quadTerms->rowIndexes;

		std::vector<int> varOneIndexes;
		std::vector<int> varTwoIndexes;
		std::vector<double> coefficients;
		std::vector<int> rowIndexes;

		varOneIndexes.reserve(numQuadTerms);
		varTwoIndexes.reserve(numQuadTerms);
		coefficients.reserve(numQuadTerms);
		rowIndexes.reserve(numQuadTerms);

		for (int i = 0; i < numQuadTerms; i++)
		{
			varOneIndexes.push_back(quadTerms->varOneIndexes[i]);
			varTwoIndexes.push_back(quadTerms->varTwoIndexes[i]);
			coefficients.push_back(quadTerms->coefficients[i]);

			if (varRowIndexes[i] != -1)
			{
				rowIndexes.push_back(quadTerms->rowIndexes[i]);
			}
			else
			{
				rowIndexes.push_back(source->getConstraintNumber());
			}
		}

		if (varOneIndexes.size() > 0)
		{
#ifdef linux
			destination->setQuadraticCoefficients(varOneIndexes.size(), &rowIndexes.at(0), &varOneIndexes.at(0),
					&varTwoIndexes.at(0), &coefficients.at(0), 0, varOneIndexes.size() - 1);

#elif _WIN32
			destination->setQuadraticTerms(varOneIndexes.size(), &rowIndexes.at(0), &varOneIndexes.at(0),
					&varTwoIndexes.at(0), &coefficients.at(0), 0, varOneIndexes.size() - 1);

#else
			destination->setQuadraticCoefficients(varOneIndexes.size(), &rowIndexes.at(0), &varOneIndexes.at(0),
					&varTwoIndexes.at(0), &coefficients.at(0), 0, varOneIndexes.size() - 1);
#endif

		}
	}
}

void OptProblemNLPMinimax::copyNonlinearExpressions(OSInstance *source, OSInstance *destination)
{
	int numNonlinearExpressions = source->getNumberOfNonlinearExpressions();
	destination->instanceData->nonlinearExpressions = new NonlinearExpressions();
	destination->instanceData->nonlinearExpressions->numberOfNonlinearExpressions = numNonlinearExpressions;

	if (numNonlinearExpressions > 0)
	{
		//destination->instanceData->nonlinearExpressions = new NonlinearExpressions();
		destination->instanceData->nonlinearExpressions->nl = new Nl*[numNonlinearExpressions];

		for (int i = 0; i < numNonlinearExpressions; i++)
		{
			int rowIdx = source->instanceData->nonlinearExpressions->nl[i]->idx;

			//int valsAdded = 0;

			//auto nlNodeVec = source->getNonlinearExpressionTreeInPrefix(rowIdx);

			destination->instanceData->nonlinearExpressions->nl[i] = new Nl();
#ifdef linux
			destination->instanceData->nonlinearExpressions->nl[i]->osExpressionTree = new ScalarExpressionTree(
					*source->instanceData->nonlinearExpressions->nl[i]->osExpressionTree);

#elif _WIN32
			destination->instanceData->nonlinearExpressions->nl[i]->osExpressionTree = new OSExpressionTree(
					*source->instanceData->nonlinearExpressions->nl[i]->osExpressionTree);

#else
			destination->instanceData->nonlinearExpressions->nl[i]->osExpressionTree = new ScalarExpressionTree(
					*source->instanceData->nonlinearExpressions->nl[i]->osExpressionTree);
#endif

			//auto tmp = ((OSnLNode*)nlNodeVec[0])->createExpressionTreeFromPrefix(nlNodeVec);
			//destination->instanceData->nonlinearExpressions->nl[i]->osExpressionTree->m_treeRoot = tmp;

			if (rowIdx != -1)
			{
				destination->instanceData->nonlinearExpressions->nl[i]->idx = rowIdx;
			}
			else
			{
				destination->instanceData->nonlinearExpressions->nl[i]->idx = source->getConstraintNumber();
			}

			//nlNodeVec.clear();
			//valsAdded++;

		}
	}

}

IndexValuePair OptProblemNLPMinimax::getMostDeviatingConstraint(std::vector<double> point)
{
	IndexValuePair valpair;

	std::vector<int> idxNLCs = this->getNonlinearOrQuadraticConstraintIndexes();

	if (idxNLCs.size() == 0)	//Only a quadratic objective function and quadratic constraints
	{
		valpair.idx = -1;
		valpair.value = 0.0;
	}
	else
	{
		std::vector<double> constrDevs(idxNLCs.size());

		for (int i = 0; i < idxNLCs.size(); i++)
		{
			constrDevs.at(i) = calculateConstraintFunctionValue(idxNLCs.at(i), point);
		}

		auto biggest = std::max_element(std::begin(constrDevs), std::end(constrDevs));
		valpair.idx = idxNLCs.at(std::distance(std::begin(constrDevs), biggest));
		valpair.value = *biggest;
	}

	return valpair;
}

bool OptProblemNLPMinimax::isConstraintsFulfilledInPoint(std::vector<double> point, double eps)
{
	std::vector<int> idxNLCs = this->getNonlinearOrQuadraticConstraintIndexes();

	for (int i = 0; i < getNumberOfNonlinearConstraints(); i++)
	{
		double tmpVal = calculateConstraintFunctionValue(idxNLCs.at(i), point);
		if (tmpVal > eps) return false;
	}

	return true;
}

double OptProblemNLPMinimax::calculateConstraintFunctionValue(int idx, std::vector<double> point)
{
	double tmpVal = 0.0;

	//if (idx != this->getNonlinearObjectiveConstraintIdx())	// Not the objective function
	//{
	tmpVal = getProblemInstance()->calculateFunctionValue(idx, &point.at(0), true);
	ProcessInfo::getInstance().numFunctionEvals++;

	if (getProblemInstance()->getConstraintTypes()[idx] == 'L')
	{
		tmpVal = tmpVal - getProblemInstance()->instanceData->constraints->con[idx]->ub; // -problemInstance->getConstraintConstants()[idx];
	}
	else if (getProblemInstance()->getConstraintTypes()[idx] == 'G')
	{
		tmpVal = -tmpVal + getProblemInstance()->instanceData->constraints->con[idx]->lb; // +problemInstance->getConstraintConstants()[idx];
		//std::cout << "Lin value is: "<< tmpVal << std::endl;
	}
	else if (getProblemInstance()->getConstraintTypes()[idx] == 'E')
	{
		tmpVal = tmpVal - getProblemInstance()->instanceData->constraints->con[idx]->lb; // +problemInstance->getConstraintConstants()[idx];
		//std::cout << "Lin value is: "<< tmpVal << std::endl;
	}

	if (point.size() > muindex) tmpVal = tmpVal + point.at(muindex);

	return tmpVal;
}

SparseVector* OptProblemNLPMinimax::calculateConstraintFunctionGradient(int idx, std::vector<double> point)
{

	SparseVector* tmpVector;
	tmpVector = getProblemInstance()->calculateConstraintFunctionGradient(&point.at(0), idx, true);
	ProcessInfo::getInstance().numGradientEvals++;

	int number = tmpVector->number;

	if (getProblemInstance()->getConstraintTypes()[idx] == 'G')
	{
		for (int i = 0; i < number; i++)
		{
			tmpVector->values[i] = -tmpVector->values[i];
		}
	}
	return tmpVector;
}
