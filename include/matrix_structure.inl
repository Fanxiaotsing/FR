/*!
 * \file matrix_structure.inl
 * \brief In-Line subroutines of the <i>matrix_structure.hpp</i> file.


 *                                
 * \version 0.1.0
 *
 * High Fidelity Large Eddy Simulation (HiFiLES) Code.
 * Copyright (C) 2014 Aerospace Computing Laboratory (ACL).
 *
 * HiFiLES is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HiFiLES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HiFiLES.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#pragma once

inline void CSysMatrix::SetValZero(void) { 
	for (unsigned long index = 0; index < nnz*nVar*nEqn; index++) 
		matrix[index] = 0.0;
}

inline void CSysMatrix::ScaleVals(double val_scale) { 
	for (unsigned long index = 0; index < nnz*nVar*nVar; index++) 
		matrix[index] *= val_scale; 
}

inline CSysMatrixVectorProduct::CSysMatrixVectorProduct(CSysMatrix & matrix_ref, solution* FlowSol) {
  sparse_matrix = &matrix_ref;
  this->FlowSol = FlowSol;
}

inline void CSysMatrixVectorProduct::operator()(const CSysVector & u, CSysVector & v) const {
  if (sparse_matrix == NULL) {
    cerr << "CSysMatrixVectorProduct::operator()(const CSysVector &, CSysVector &): " << endl; 
    cerr << "pointer to sparse matrix is NULL." << endl;
    throw(-1);
  }
  sparse_matrix->MatrixVectorProduct(u, v);
}

/*
inline CJacobiPreconditioner::CJacobiPreconditioner(CSysMatrix & matrix_ref, CGeometry *geometry_ref, CConfig *config_ref) {
  sparse_matrix = &matrix_ref;
  geometry = geometry_ref;
  config = config_ref;  
}

inline void CJacobiPreconditioner::operator()(const CSysVector & u, CSysVector & v) const {
  if (sparse_matrix == NULL) {
    cerr << "CJacobiPreconditioner::operator()(const CSysVector &, CSysVector &): " << endl; 
    cerr << "pointer to sparse matrix is NULL." << endl;
    throw(-1);
  }
  sparse_matrix->ComputeJacobiPreconditioner(u, v, geometry, config);
}
*/
inline CLU_SGSPreconditioner::CLU_SGSPreconditioner(CSysMatrix & matrix_ref, solution* FlowSol) {
  sparse_matrix = &matrix_ref;
  this->FlowSol = FlowSol;
}

inline void CLU_SGSPreconditioner::operator()(const CSysVector & u, CSysVector & v) const {
  if (sparse_matrix == NULL) {
    cerr << "CLU_SGSPreconditioner::operator()(const CSysVector &, CSysVector &): " << endl; 
    cerr << "pointer to sparse matrix is NULL." << endl;
    throw(-1);
  }
  sparse_matrix->ComputeLU_SGSPreconditioner(u, v);
}
/*
inline CLineletPreconditioner::CLineletPreconditioner(CSysMatrix & matrix_ref, CGeometry *geometry_ref, CConfig *config_ref) {
  sparse_matrix = &matrix_ref;
  geometry = geometry_ref;
  config = config_ref;  
}

inline void CLineletPreconditioner::operator()(const CSysVector & u, CSysVector & v) const {
  if (sparse_matrix == NULL) {
    cerr << "CLineletPreconditioner::operator()(const CSysVector &, CSysVector &): " << endl; 
    cerr << "pointer to sparse matrix is NULL." << endl;
    throw(-1);
  }
  sparse_matrix->ComputeLineletPreconditioner(u, v, geometry, config);
}
*/
