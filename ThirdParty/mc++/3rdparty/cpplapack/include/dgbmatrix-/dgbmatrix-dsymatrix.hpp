//=============================================================================
/*! dgbmatrix+dsymatrix operator */
inline _dgematrix operator+(const dgbmatrix& matA, const dsymatrix& matB)
{VERBOSE_REPORT;
#ifdef  CPPL_DEBUG
  if(matA.n!=matB.n || matA.m!=matB.n){
    ERROR_REPORT;
    std::cerr << "These two matrises can not make a summation." << std::endl
              << "Your input was (" << matA.m << "x" << matA.n << ") + (" << matB.n << "x" << matB.n << ")." << std::endl;
    exit(1);
  }
#endif//CPPL_DEBUG
  
  dgematrix newmat(matB.n,matB.n);
  for(long i=0; i<matA.m; i++){
    for(long j=0; j<matB.n; j++){
      newmat(i,j) = matB(i,j);
    }
    for(long j=std::max(long(0),i-matA.kl); j<std::min(matA.n,i+matA.ku+1); j++){
      newmat(i,j)+=matA(i,j);
    }
  }
  
  return _(newmat);
}

//=============================================================================
/*! dgbmatrix-dsymatrix operator */
inline _dgematrix operator-(const dgbmatrix& matA, const dsymatrix& matB)
{VERBOSE_REPORT;
#ifdef  CPPL_DEBUG
  if(matA.n!=matB.n || matA.m!=matB.n){
    ERROR_REPORT;
    std::cerr << "These two matrises can not make a summation." << std::endl
              << "Your input was (" << matA.m << "x" << matA.n << ") + (" << matB.n << "x" << matB.n << ")." << std::endl;
    exit(1);
  }
#endif//CPPL_DEBUG
  
  dgematrix newmat(matB.n,matB.n);
  for(long i=0; i<matA.m; i++){
    for(long j=0; j<matB.n; j++){
      newmat(i,j) = -matB(i,j);
    }
    for(long j=std::max(long(0),i-matA.kl); j<std::min(matA.n,i+matA.ku+1); j++){
      newmat(i,j) += matA(i,j);
    }
  }
  
  return _(newmat);
}

//=============================================================================
/*! dgbmatrix*dsymatrix operator */
inline _dgematrix operator*(const dgbmatrix& matA, const dsymatrix& matB)
{VERBOSE_REPORT;
#ifdef  CPPL_DEBUG
  if(matA.n!=matB.n){
    ERROR_REPORT;
    std::cerr << "These two matrises can not make a product." << std::endl
              << "Your input was (" << matA.m << "x" << matA.n << ") * (" << matB.n << "x" << matB.n << ")." << std::endl;
    exit(1);
  }
#endif//CPPL_DEBUG
  
  dgematrix newmat( matA.m, matB.n );
  newmat.zero();
  
  for(long i=0; i<newmat.m; i++){
    for(long j=0; j<newmat.n; j++){
      for(long k=std::max(long(0),i-matA.kl); k<std::min(matA.n,i+matA.ku+1); k++){
        newmat(i,j)+=matA(i,k)*matB(k,j);
      }
    }
  }
  
  return _(newmat);
}
