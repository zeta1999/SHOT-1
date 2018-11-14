/**
   The Supporting Hyperplane Optimization Toolkit (SHOT).

   @author Andreas Lundell, Åbo Akademi University

   @section LICENSE 
   This software is licensed under the Eclipse Public License 2.0. 
   Please see the README and LICENSE files for more information.
*/

#pragma once
#include "../Structs.h"
#include "ffunc.hpp"
#include <sstream>

namespace SHOT
{

// Forward declarations

class Problem;
typedef std::shared_ptr<Problem> ProblemPtr;

class Variable;
typedef std::shared_ptr<Variable> VariablePtr;
typedef std::map<VariablePtr, double> SparseVariableVector;
typedef std::vector<VariablePtr> Variables;

class ObjectiveFunction;
typedef std::shared_ptr<ObjectiveFunction> ObjectiveFunctionPtr;

class LinearObjectiveFunction;
typedef std::shared_ptr<LinearObjectiveFunction> LinearObjectiveFunctionPtr;

class QuadraticObjectiveFunction;
typedef std::shared_ptr<QuadraticObjectiveFunction> QuadraticObjectiveFunctionPtr;

class NonlinearObjectiveFunction;
typedef std::shared_ptr<NonlinearObjectiveFunction> NonlinearObjectiveFunctionPtr;

class LinearTerm;
typedef std::shared_ptr<LinearTerm> LinearTermPtr;
class LinearTerms;

class QuadraticTerm;
typedef std::shared_ptr<QuadraticTerm> QuadraticTermPtr;
class QuadraticTerms;

class NonlinearExpression;
typedef std::shared_ptr<NonlinearExpression> NonlinearExpressionPtr;

class Constraint;
typedef std::shared_ptr<Constraint> ConstraintPtr;
typedef std::vector<ConstraintPtr> Constraints;

class NumericConstraint;
typedef std::shared_ptr<NumericConstraint> NumericConstraintPtr;
typedef std::vector<NumericConstraintPtr> NumericConstraints;

class LinearConstraint;
typedef std::shared_ptr<LinearConstraint> LinearConstraintPtr;
typedef std::vector<LinearConstraintPtr> LinearConstraints;

class QuadraticConstraint;
typedef std::shared_ptr<QuadraticConstraint> QuadraticConstraintPtr;
typedef std::vector<QuadraticConstraintPtr> QuadraticConstraints;

class NonlinearConstraint;
typedef std::shared_ptr<NonlinearConstraint> NonlinearConstraintPtr;
typedef std::vector<NonlinearConstraintPtr> NonlinearConstraints;

class ExpressionVariable;
typedef std::shared_ptr<ExpressionVariable> ExpressionVariablePtr;

struct NumericConstraintValue
{
    NumericConstraintPtr constraint;

    // Considering a constraint L <= f(x) <= U:
    double functionValue;      // This is the function value of f(x)
    bool isFulfilledLHS;       // Is L <= f(x)?
    double normalizedLHSValue; // This is the value of L - f(x)
    bool isFulfilledRHS;       // Is f(x) <= U
    double normalizedRHSValue; // This is the value of f(x) - U
    bool isFulfilled;          // Is L <= f(x) <= U?
    double error;              // max(0, max(L - f(x), f(x) - U)
    double normalizedValue;    // max(L - f(x), f(x)-U)
};

typedef std::vector<NumericConstraintValue> NumericConstraintValues;

//typedef std::vector<PairCoordinateValue> SparseMatrix;

typedef mc::FFGraph FactorableFunctionGraph;
typedef std::shared_ptr<FactorableFunctionGraph> FactorableFunctionGraphPtr;
typedef mc::FFVar FactorableFunction;
typedef std::shared_ptr<FactorableFunction> FactorableFunctionPtr;
typedef std::vector<FactorableFunctionPtr> FactorableFunctions;

typedef mc::Interval Interval;
typedef std::vector<Interval> IntervalVector;

enum class E_VariableType
{
    Real,
    Binary,
    Integer,
    Semicontinuous
};

enum class E_Curvature
{
    None,
    Convex,
    Concave,
    Nonconvex,
    Indeterminate
};

// Begin exception definitions

class VariableNotFoundException : public std::exception
{
  private:
    std::string errorMessage;

  public:
    VariableNotFoundException(std::string message) : errorMessage(message)
    {
    }

    inline const char *what() const throw()
    {
        std::stringstream message;
        message << "Could not find variable ";
        message << errorMessage;

        return (message.str().c_str());
    }
};

class ConstraintNotFoundException : public std::exception
{
  private:
    std::string errorMessage;

  public:
    ConstraintNotFoundException(std::string message) : errorMessage(message)
    {
    }

    inline const char *what() const throw()
    {
        std::stringstream message;
        message << "Could not find constraint ";
        message << errorMessage;

        return (message.str().c_str());
    }
};

class OperationNotImplementedException : public std::exception
{
  private:
    std::string errorMessage;

  public:
    OperationNotImplementedException(std::string message) : errorMessage(message)
    {
    }

    inline const char *what() const throw()
    {
        std::stringstream message;
        message << "The following operation is not implemented: ";
        message << errorMessage;

        return (message.str().c_str());
    }
};

// End exception definitions

} // namespace SHOT
