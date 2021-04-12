/*!
 * \file cubature_tet.h

 *                          Peter Vincent, David Williams (alphabetical by surname).
 *         - Current development: Aerospace Computing Laboratory (ACL)
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

#include <string>
#include "array.h"

class cubature_tet
{
public:

  // #### constructors ####

  // default constructor
  cubature_tet();

  // constructor 1
  cubature_tet(int in_rule); // set by order

  // copy constructor
  cubature_tet(const cubature_tet& in_cubature);

  // assignment
  cubature_tet& operator=(const cubature_tet& in_cubature);

  // destructor
  ~cubature_tet();

  // #### methods ####

  // method to get number of cubature points
  int get_n_pts(void);

  // method to get r location of cubature point
  double get_r(int in_pos);

  // method to get s location of cubature point
  double get_s(int in_pos);

  // method to get s location of cubature point
  double get_t(int in_pos);

  // method to get weight location of cubature point
  double get_weight(int in_pos);

  // #### members ####

  // cubature rule
  int rule;

  // number of cubature points
  int n_pts;

  // location of cubature points
  array<double> locs;

  // weight of cubature points
  array<double> weights;
  
  // cubature data file
  ifstream datfile;
};
