/*!
 * \file eles_hexas.cpp

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

#include <iomanip>
#include <iostream>
#include <cmath>

#if defined _ACCELERATE_BLAS
#include <Accelerate/Accelerate.h>
#endif

#if defined _MKL_BLAS
#include "mkl.h"
#include "mkl_spblas.h"
#endif

#if defined _STANDARD_BLAS
extern "C"
{
#include "cblas.h"
}
#endif

#include "../include/global.h"
#include "../include/eles.h"
#include "../include/eles_hexas.h"
#include "../include/array.h"
#include "../include/funcs.h"
#include "../include/error.h"
#include "../include/cubature_1d.h"
#include "../include/cubature_quad.h"
#include "../include/cubature_hexa.h"

using namespace std;

// #### constructors ####

// default constructor

eles_hexas::eles_hexas()
{	

}

void eles_hexas::setup_ele_type_specific()
{
#ifndef _MPI
  cout << "Initializing hexas" << endl;
#endif

  ele_type=4;
  n_dims=3;

  if (run_input.equation==0)
    n_fields=5;
  else if (run_input.equation==1)
    n_fields=1;
  else
    FatalError("Equation not supported");

  if (run_input.turb_model==1)
    n_fields++;

  n_inters_per_ele=6;

  n_upts_per_ele=(order+1)*(order+1)*(order+1);
  upts_type=run_input.upts_type_hexa;
  set_loc_1d_upts();
  set_loc_upts();
  set_vandermonde();

  set_inters_cubpts();
  set_volume_cubpts();
  set_opp_volume_cubpts();

  n_ppts_per_ele=p_res*p_res*p_res;
  n_peles_per_ele=(p_res-1)*(p_res-1)*(p_res-1);
  n_verts_per_ele = 8;

  set_loc_ppts();
  set_opp_p();

  n_fpts_per_inter.setup(6);

  n_fpts_per_inter(0)=(order+1)*(order+1);
  n_fpts_per_inter(1)=(order+1)*(order+1);
  n_fpts_per_inter(2)=(order+1)*(order+1);
  n_fpts_per_inter(3)=(order+1)*(order+1);
  n_fpts_per_inter(4)=(order+1)*(order+1);
  n_fpts_per_inter(5)=(order+1)*(order+1);

  n_fpts_per_ele=n_inters_per_ele*(order+1)*(order+1);

  set_tloc_fpts();

  set_tnorm_fpts();

  set_opp_0(run_input.sparse_hexa);
  set_opp_1(run_input.sparse_hexa);
  set_opp_2(run_input.sparse_hexa);
  set_opp_3(run_input.sparse_hexa);

  if(viscous)
    {
      // Compute hex filter matrix
      if(filter) compute_filter_upts();

      set_opp_4(run_input.sparse_hexa);
      set_opp_5(run_input.sparse_hexa);
      set_opp_6(run_input.sparse_hexa);

      temp_grad_u.setup(n_fields,n_dims);
    }

  temp_u.setup(n_fields);
  temp_f.setup(n_fields,n_dims);
}

// #### methods ####

// set shape

/*void eles_hexas::set_shape(array<int> &in_n_spts_per_ele)
{
  //TODO: this is inefficient, copies by value
  n_spts_per_ele = in_n_spts_per_ele;

  // Computing maximum number of spts per ele for all elements
  int max_n_spts_per_ele = 0;
  for (int i=0;i<n_eles;++i) {
    if (n_spts_per_ele(i) > max_n_spts_per_ele)
      max_n_spts_per_ele = n_spts_per_ele(i);
  }

    shape.setup(n_dims,max_n_spts_per_ele,n_eles);
}
*/

void eles_hexas::set_connectivity_plot()
{
  int vertex_0,vertex_1,vertex_2,vertex_3,vertex_4,vertex_5,vertex_6,vertex_7;
  int count=0;
  int k,l,m;

  for(k=0;k<p_res-1;++k){
      for(l=0;l<p_res-1;++l){
          for(m=0;m<p_res-1;++m){
              vertex_0=m+(p_res*l)+(p_res*p_res*k);
              vertex_1=vertex_0+1;
              vertex_2=vertex_0+p_res+1;
              vertex_3=vertex_0+p_res;
              vertex_4=vertex_0+p_res*p_res;
              vertex_5=vertex_4+1;
              vertex_6=vertex_4+p_res+1;
              vertex_7=vertex_4+p_res;

              connectivity_plot(0,count) = vertex_0;
              connectivity_plot(1,count) = vertex_1;
              connectivity_plot(2,count) = vertex_2;
              connectivity_plot(3,count) = vertex_3;
              connectivity_plot(4,count) = vertex_4;
              connectivity_plot(5,count) = vertex_5;
              connectivity_plot(6,count) = vertex_6;
              connectivity_plot(7,count) = vertex_7;
              count++;
            }
        }
    }
}


// set location of 1d solution points in standard interval (required for tensor product elements)

void eles_hexas::set_loc_1d_upts(void)
{
  if(upts_type==0)
    {
      int get_order=order;

      array<double> loc_1d_gauss_pts(order+1);

#include "../data/loc_1d_gauss_pts.dat"

      loc_1d_upts=loc_1d_gauss_pts;
    }
  else if(upts_type==1)
    {
      int get_order=order;

      array<double> loc_1d_gauss_lobatto_pts(order+1);

#include "../data/loc_1d_gauss_lobatto_pts.dat"

      loc_1d_upts=loc_1d_gauss_lobatto_pts;
    }
  else
    {
      cout << "ERROR: Unknown solution point type.... " << endl;
    }
}

// set location of 1d shape points in standard interval (required for tensor product element)

void eles_hexas::set_loc_1d_spts(array<double> &loc_1d_spts, int in_n_1d_spts)
{
  int i;

  for(i=0;i<in_n_1d_spts;++i)
    {
      loc_1d_spts(i)=-1.0+((2.0*i)/(1.0*(in_n_1d_spts-1)));
    }
}



// set location of solution points in standard element

void eles_hexas::set_loc_upts(void)
{
  int i,j,k;

  int upt;

  loc_upts.setup(n_dims,n_upts_per_ele);

  for(i=0;i<(order+1);++i)
    {
      for(j=0;j<(order+1);++j)
        {
          for(k=0;k<(order+1);++k)
            {
              upt=k+(order+1)*j+(order+1)*(order+1)*i;

              loc_upts(0,upt)=loc_1d_upts(k);
              loc_upts(1,upt)=loc_1d_upts(j);
              loc_upts(2,upt)=loc_1d_upts(i);
            }
        }
    }
}

// set location of flux points in standard element

void eles_hexas::set_tloc_fpts(void)
{
  int i,j,k;

  int fpt;

  tloc_fpts.setup(n_dims,n_fpts_per_ele);

  for(i=0;i<n_inters_per_ele;++i)
    {
      for(j=0;j<(order+1);++j)
        {
          for(k=0;k<(order+1);++k)
            {
              fpt=k+((order+1)*j)+((order+1)*(order+1)*i);

              // for tensor prodiuct elements flux point location depends on solution point location

              if(i==0)
                {
                  tloc_fpts(0,fpt)=loc_1d_upts(order-k);
                  tloc_fpts(1,fpt)=loc_1d_upts(j);
                  tloc_fpts(2,fpt)=-1.0;

                }
              else if(i==1)
                {
                  tloc_fpts(0,fpt)=loc_1d_upts(k);
                  tloc_fpts(1,fpt)=-1.0;
                  tloc_fpts(2,fpt)=loc_1d_upts(j);
                }
              else if(i==2)
                {
                  tloc_fpts(0,fpt)=1.0;
                  tloc_fpts(1,fpt)=loc_1d_upts(k);
                  tloc_fpts(2,fpt)=loc_1d_upts(j);
                }
              else if(i==3)
                {
                  tloc_fpts(0,fpt)=loc_1d_upts(order-k);
                  tloc_fpts(1,fpt)=1.0;
                  tloc_fpts(2,fpt)=loc_1d_upts(j);
                }
              else if(i==4)
                {
                  tloc_fpts(0,fpt)=-1.0;
                  tloc_fpts(1,fpt)=loc_1d_upts(order-k);
                  tloc_fpts(2,fpt)=loc_1d_upts(j);
                }
              else if(i==5)
                {
                  tloc_fpts(0,fpt)=loc_1d_upts(k);
                  tloc_fpts(1,fpt)=loc_1d_upts(j);
                  tloc_fpts(2,fpt)=1.0;
                }
            }
        }
    }
}

void eles_hexas::set_inters_cubpts(void)
{

  n_cubpts_per_inter.setup(n_inters_per_ele);
  loc_inters_cubpts.setup(n_inters_per_ele);
  weight_inters_cubpts.setup(n_inters_per_ele);
  tnorm_inters_cubpts.setup(n_inters_per_ele);

  cubature_quad cub_quad(inters_cub_order);
  int n_cubpts_quad = cub_quad.get_n_pts();

  for (int i=0;i<n_inters_per_ele;++i)
    n_cubpts_per_inter(i) = n_cubpts_quad;

  for (int i=0;i<n_inters_per_ele;++i) {

      loc_inters_cubpts(i).setup(n_dims,n_cubpts_per_inter(i));
      weight_inters_cubpts(i).setup(n_cubpts_per_inter(i));
      tnorm_inters_cubpts(i).setup(n_dims,n_cubpts_per_inter(i));

      for (int j=0;j<n_cubpts_quad;++j) {

          if (i==0) {
              loc_inters_cubpts(i)(0,j)=cub_quad.get_r(j);
              loc_inters_cubpts(i)(1,j)=cub_quad.get_s(j);
              loc_inters_cubpts(i)(2,j)=-1.0;
            }
          else if (i==1) {
              loc_inters_cubpts(i)(0,j)=cub_quad.get_r(j);
              loc_inters_cubpts(i)(1,j)=-1.;
              loc_inters_cubpts(i)(2,j)=cub_quad.get_s(j);
            }
          else if (i==2) {
              loc_inters_cubpts(i)(0,j)=1.0;
              loc_inters_cubpts(i)(1,j)=cub_quad.get_r(j);
              loc_inters_cubpts(i)(2,j)=cub_quad.get_s(j);
            }
          else if (i==3) {
              loc_inters_cubpts(i)(0,j)=cub_quad.get_r(j);
              loc_inters_cubpts(i)(1,j)=1.0;
              loc_inters_cubpts(i)(2,j)=cub_quad.get_s(j);
            }
          else if (i==4) {
              loc_inters_cubpts(i)(0,j)=-1.0;
              loc_inters_cubpts(i)(1,j)=cub_quad.get_r(j);
              loc_inters_cubpts(i)(2,j)=cub_quad.get_s(j);
            }
          else if (i==5) {
              loc_inters_cubpts(i)(0,j)=cub_quad.get_r(j);
              loc_inters_cubpts(i)(1,j)=cub_quad.get_s(j);
              loc_inters_cubpts(i)(2,j)=1.0;
            }

          weight_inters_cubpts(i)(j) = cub_quad.get_weight(j);

          if (i==0) {
              tnorm_inters_cubpts(i)(0,j)= 0.;
              tnorm_inters_cubpts(i)(1,j)= 0.;
              tnorm_inters_cubpts(i)(2,j)= -1.;
            }
          else if (i==1) {
              tnorm_inters_cubpts(i)(0,j)= 0.;
              tnorm_inters_cubpts(i)(1,j)= -1.;
              tnorm_inters_cubpts(i)(2,j)= 0.;
            }
          else if (i==2) {
              tnorm_inters_cubpts(i)(0,j)= 1.;
              tnorm_inters_cubpts(i)(1,j)= 0.;
              tnorm_inters_cubpts(i)(2,j)= 0.;
            }
          else if (i==3) {
              tnorm_inters_cubpts(i)(0,j)= 0.;
              tnorm_inters_cubpts(i)(1,j)= 1.;
              tnorm_inters_cubpts(i)(2,j)= 0.;
            }
          else if (i==4) {
              tnorm_inters_cubpts(i)(0,j)= -1.;
              tnorm_inters_cubpts(i)(1,j)= 0.;
              tnorm_inters_cubpts(i)(2,j)= 0.;
            }
          else if (i==5) {
              tnorm_inters_cubpts(i)(0,j)= 0.;
              tnorm_inters_cubpts(i)(1,j)= 0.;
              tnorm_inters_cubpts(i)(2,j)= 1.;
            }

        }
    }

  set_opp_inters_cubpts();
}

void eles_hexas::set_volume_cubpts(void)
{
  cubature_hexa cub_hexa(volume_cub_order);
  int n_cubpts_hexa = cub_hexa.get_n_pts();
  n_cubpts_per_ele = n_cubpts_hexa;

  loc_volume_cubpts.setup(n_dims,n_cubpts_hexa);
  weight_volume_cubpts.setup(n_cubpts_hexa);

  for (int i=0;i<n_cubpts_hexa;++i)
    {
      loc_volume_cubpts(0,i) = cub_hexa.get_r(i);
      loc_volume_cubpts(1,i) = cub_hexa.get_s(i);
      loc_volume_cubpts(2,i) = cub_hexa.get_t(i);

      //cout << "x=" << loc_volume_cubpts(0,i) << endl;
      //cout << "y=" << loc_volume_cubpts(1,i) << endl;
      //cout << "z=" << loc_volume_cubpts(2,i) << endl;
      weight_volume_cubpts(i) = cub_hexa.get_weight(i);
      //cout << "wgt=" << weight_volume_cubpts(i) << endl;
    }
}

// Compute the surface jacobian determinant on a face
double eles_hexas::compute_inter_detjac_inters_cubpts(int in_inter,array<double> d_pos)
{
  double output = 0.;
  double xr, xs, xt;
  double yr, ys, yt;
  double zr, zs, zt;
  double temp0,temp1,temp2;

  xr = d_pos(0,0);
  xs = d_pos(0,1);
  xt = d_pos(0,2);
  yr = d_pos(1,0);
  ys = d_pos(1,1);
  yt = d_pos(1,2);
  zr = d_pos(2,0);
  zs = d_pos(2,1);
  zt = d_pos(2,2);

  double xu=0.;
  double yu=0.;
  double zu=0.;
  double xv=0.;
  double yv=0.;
  double zv=0.;

  // From calculus, for a surface parameterized by two parameters
  // u and v, than jacobian determinant is
  //
  // || (xu i + yu j + zu k) cross ( xv i + yv j + zv k)  ||

  if (in_inter==0) // u=r, v=s
    {
      xu = xr;
      yu = yr;
      zu = zr;

      xv = xs;
      yv = ys;
      zv = zs;
    }
  else if (in_inter==1) // u=r, v=t
    {
      xu = xr;
      yu = yr;
      zu = zr;

      xv = xt;
      yv = yt;
      zv = zt;
    }
  else if (in_inter==2) //u=s, v=t
    {
      xu = xs;
      yu = ys;
      zu = zs;

      xv = xt;
      yv = yt;
      zv = zt;
    }
  else if (in_inter==3) //u=r,v=t
    {
      xu = xr;
      yu = yr;
      zu = zr;

      xv = xt;
      yv = yt;
      zv = zt;
    }
  else if (in_inter==4) //u=s,v=t
    {
      xu = xs;
      yu = ys;
      zu = zs;

      xv = xt;
      yv = yt;
      zv = zt;
    }
  else if (in_inter==5) //u=r,v=s
    {
      xu = xr;
      yu = yr;
      zu = zr;

      xv = xs;
      yv = ys;
      zv = zs;
    }

  temp0 = yu*zv-zu*yv;
  temp1 = zu*xv-xu*zv;
  temp2 = xu*yv-yu*xv;

  output = sqrt(temp0*temp0+temp1*temp1+temp2*temp2);

  return output;

}

// set location of plot points in standard element

void eles_hexas::set_loc_ppts(void)
{
  int i,j,k;

  int ppt;

  loc_ppts.setup(n_dims,n_ppts_per_ele);

  for(k=0;k<p_res;++k)
    {
      for(j=0;j<p_res;++j)
        {
          for(i=0;i<p_res;++i)
            {
              ppt=i+(p_res*j)+(p_res*p_res*k);

              loc_ppts(0,ppt)=-1.0+((2.0*i)/(1.0*(p_res-1)));
              loc_ppts(1,ppt)=-1.0+((2.0*j)/(1.0*(p_res-1)));
              loc_ppts(2,ppt)=-1.0+((2.0*k)/(1.0*(p_res-1)));
            }
        }
    }

}

// set transformed normal at flux points

void eles_hexas::set_tnorm_fpts(void)
{
  int i,j,k;

  int fpt;

  tnorm_fpts.setup(n_dims,n_fpts_per_ele);

  for(i=0;i<n_inters_per_ele;++i)
    {
      for(j=0;j<(order+1);++j)
        {
          for(k=0;k<(order+1);++k)
            {
              fpt=k+((order+1)*j)+((order+1)*(order+1)*i);

              if(i==0)
                {
                  tnorm_fpts(0,fpt)=0.0;
                  tnorm_fpts(1,fpt)=0.0;
                  tnorm_fpts(2,fpt)=-1.0;
                }
              else if(i==1)
                {
                  tnorm_fpts(0,fpt)=0.0;
                  tnorm_fpts(1,fpt)=-1.0;
                  tnorm_fpts(2,fpt)=0.0;
                }
              else if(i==2)
                {
                  tnorm_fpts(0,fpt)=1.0;
                  tnorm_fpts(1,fpt)=0.0;
                  tnorm_fpts(2,fpt)=0.0;
                }
              else if(i==3)
                {
                  tnorm_fpts(0,fpt)=0.0;
                  tnorm_fpts(1,fpt)=1.0;
                  tnorm_fpts(2,fpt)=0.0;
                }
              else if(i==4)
                {
                  tnorm_fpts(0,fpt)=-1.0;
                  tnorm_fpts(1,fpt)=0.0;
                  tnorm_fpts(2,fpt)=0.0;
                }
              else if(i==5)
                {
                  tnorm_fpts(0,fpt)=0.0;
                  tnorm_fpts(1,fpt)=0.0;
                  tnorm_fpts(2,fpt)=1.0;
                }
            }
        }
    }
}

// Filtering operators for use in subgrid-scale modelling
void eles_hexas::compute_filter_upts(void)
{
  int i,j,k,l,m,n,N,N2;
  double dlt, k_c, sum, norm;
  N = order+1;

  array<double> X(N), B(N);
  array<double> beta(N,N);

  filter_upts_1D.setup(N,N);

  X = loc_1d_upts;

  N2 = N/2;
  // If N is odd, round up N/2
  if(N % 2 != 0){N2 += 1;}
  // Cutoff wavenumber
  k_c = 1.0/run_input.filter_ratio;
  // Approx resolution in element (assumes uniform point spacing)
  // Interval is [-1:1]
  dlt = 2.0/order;

  // Normalised solution point separation
  for (i=0;i<N;++i)
    for (j=0;j<N;++j)
      beta(j,i) = (X(j)-X(i))/dlt;

  // Build high-order-commuting Vasilyev filter
  // Only use high-order filters for high enough order
  if(run_input.filter_type==0 and N>=3)
    {
      if (rank==0) cout<<"Building high-order-commuting Vasilyev filter"<<endl;
      array<double> C(N);
      array<double> A(N,N);

      for (i=0;i<N;++i)
        {
          B(i) = 0.0;
          C(i) = 0.0;
          for (j=0;j<N;++j)
            A(i,j) = 0.0;

        }
      // Populate coefficient matrix
      for (i=0;i<N;++i)
        {
          // Populate constraints matrix
          B(0) = 1.0;
          // Gauss filter weights
          B(1) =  exp(-pow(pi,2)/24.0);
          B(2) = -B(1)*pow(pi,2)/k_c/12.0;

          if(N % 2 == 1 && i+1 == N2)
            B(2) = 0.0;

          for (j=0;j<N;++j)
            {
              A(j,0) = 1.0;
              A(j,1) = cos(pi*k_c*beta(j,i));
              A(j,2) = -beta(j,i)*pi*sin(pi*k_c*beta(j,i));

              if(N % 2 == 1 && i+1 == N2)
                A(j,2) = pow(beta(j,i),3);

            }

          // Enforce filter moments
          for (k=3;k<N;++k)
            {
              for (j=0;j<N;++j)
                A(j,k) = pow(beta(j,i),k+1);

              B(k) = 0.0;
            }

          // Solve linear system by inverting A using
          // Gauss-Jordan method
          gaussj(N,A,B);
          for (j=0;j<N;++j)
            filter_upts_1D(j,i) = B(j);

        }
    }
  else if(run_input.filter_type==1) // Discrete Gaussian filter
    {
      if (rank==0) cout<<"Building discrete Gaussian filter"<<endl;
      int ctype, index;
      double k_R, k_L, coeff;
      double res_0, res_L, res_R;
      array<double> alpha(N);
      cubature_1d cub_1d(inters_cub_order);
      int n_cubpts_1d = cub_1d.get_n_pts();
      array<double> wf(n_cubpts_1d);

      if(N != n_cubpts_1d)
        {
          FatalError("WARNING: To build Gaussian filter, the interface cubature order must equal solution order, e.g. inters_cub_order=9 if order=4, inters_cub_order=7 if order=3, inters_cub_order=5 if order=2. Exiting");
        }
      for (j=0;j<n_cubpts_1d;j++)
        wf(j) = cub_1d.get_weight(j);

      // Determine corrected filter width for skewed quadrature points
      // using iterative constraining procedure
      // ctype options: (-1) no constraining, (0) constrain moment, (1) constrain cutoff frequency
      ctype = -1;
      if(ctype>=0)
        {
          for (i=0;i<N2;++i)
            {
              for (j=0;j<N;++j)
                B(j) = beta(j,i);

              k_L = 0.1; k_R = 1.0;
              res_L = flt_res(N,wf,B,k_L,k_c,ctype);
              res_R = flt_res(N,wf,B,k_R,k_c,ctype);
              alpha(i) = 0.5*(k_L+k_R);
              for (j=0;j<1000;++j)
                {
                  res_0 = flt_res(N,wf,B,k_c,alpha(i),ctype);
                  if(abs(res_0)<1.e-12) return;
                  if(res_0*res_L>0.0)
                    {
                      k_L = alpha(i);
                      res_L = res_0;
                    }
                  else
                    {
                      k_R = alpha(i);
                      res_R = res_0;
                    }
                  if(j==999)
                    {
                      alpha(i) = k_c;
                      ctype = -1;
                    }
                }
              alpha(N-i-1) = alpha(i);
            }
        }

      else if(ctype==-1) // no iterative constraining
        for (i=0;i<N;++i)
          alpha(i) = k_c;

      sum = 0.0;
      for (i=0;i<N;++i)
        {
          norm = 0.0;
          for (j=0;j<N;++j)
            {
              filter_upts_1D(i,j) = wf(j)*exp(-6.0*pow(alpha(i)*beta(i,j),2));
              norm += filter_upts_1D(i,j);
            }
          for (j=0;j<N;++j)
            {
              filter_upts_1D(i,j) /= norm;
              sum += filter_upts_1D(i,j);
            }
        }
    }
  else if(run_input.filter_type==2) // Modal coefficient filter
    {
      if (rank==0) cout<<"Building modal filter"<<endl;

      // Compute restriction-prolongation filter
      compute_modal_filter_1d(filter_upts_1D, vandermonde, inv_vandermonde, N, order);

      sum = 0;
      for(i=0;i<N;i++)
        for(j=0;j<N;j++)
          sum+=filter_upts_1D(i,j);
    }
  else // Simple average for low order
    {
      if (rank==0) cout<<"Building average filter"<<endl;
      sum=0;
      for (i=0;i<N;i++)
        {
          for (j=0;j<N;j++)
            {
              filter_upts_1D(i,j) = 1.0/N;
              sum+=1.0/N;
            }
        }
    }

  // Build 3D filter on ideal (reference) element.
  // This construction is unique to hexa elements but the resulting
  // matrix will be of the same(?) dimension for all 3D element types.
  int ii=0;
  filter_upts.setup(n_upts_per_ele,n_upts_per_ele);
  sum=0;
  for (i=0;i<N;++i)
    {
      for (j=0;j<N;++j)
        {
          for (k=0;k<N;++k)
            {
              int jj=0;
              for (l=0;l<N;++l)
                {
                  for (m=0;m<N;++m)
                    {
                      for (n=0;n<N;++n)
                        {
                          filter_upts(ii,jj) = filter_upts_1D(k,n)*filter_upts_1D(j,m)*filter_upts_1D(i,l);
                          sum+=filter_upts(ii,jj);
                          ++jj;
                        }
                    }
                }
              ++ii;
            }
        }
    }
}


//#### helper methods ####


int eles_hexas::read_restart_info(ifstream& restart_file)
{

  string str;
  // Move to triangle element
  while(1) {
      getline(restart_file,str);
      if (str=="HEXAS") break;

      if (restart_file.eof()) return 0;
    }

  getline(restart_file,str);
  restart_file >> order_rest;
  getline(restart_file,str);
  getline(restart_file,str);
  restart_file >> n_upts_per_ele_rest;
  getline(restart_file,str);
  getline(restart_file,str);

  loc_1d_upts_rest.setup(order_rest+1);

  for (int i=0;i<order_rest+1;++i)
    restart_file >> loc_1d_upts_rest(i);

  set_opp_r();

  return 1;
}

// write restart info
void eles_hexas::write_restart_info(ofstream& restart_file)        
{
  restart_file << "HEXAS" << endl;

  restart_file << "Order" << endl;
  restart_file << order << endl;

  restart_file << "Number of solution points per hexahedral element" << endl;
  restart_file << n_upts_per_ele << endl;

  restart_file << "Location of solution points in 1D" << endl;
  for (int i=0;i<order+1;++i) {
      restart_file << loc_1d_upts(i) << " ";
    }
  restart_file << endl;



}

// initialize the vandermonde matrix
void eles_hexas::set_vandermonde(void)
{
  vandermonde.setup(order+1,order+1);

  for (int i=0;i<order+1;i++)
    for (int j=0;j<order+1;j++)
      vandermonde(i,j) = eval_legendre(loc_1d_upts(i),j);

  // Store its inverse
  inv_vandermonde = inv_array(vandermonde);
}

// evaluate nodal basis

double eles_hexas::eval_nodal_basis(int in_index, array<double> in_loc)
{
  int i,j,k;

  double nodal_basis;

  i=(in_index/((order+1)*(order+1)));
  j=(in_index-((order+1)*(order+1)*i))/(order+1);
  k=in_index-((order+1)*j)-((order+1)*(order+1)*i);

  nodal_basis=eval_lagrange(in_loc(0),k,loc_1d_upts)*eval_lagrange(in_loc(1),j,loc_1d_upts)*eval_lagrange(in_loc(2),i,loc_1d_upts);

  return nodal_basis;
}

// evaluate nodal basis using restart points
//
double eles_hexas::eval_nodal_basis_restart(int in_index, array<double> in_loc)
{
  int i,j,k;

  double nodal_basis;

  i=(in_index/((order_rest+1)*(order_rest+1)));
  j=(in_index-((order_rest+1)*(order_rest+1)*i))/(order_rest+1);
  k=in_index-((order_rest+1)*j)-((order_rest+1)*(order_rest+1)*i);

  nodal_basis=eval_lagrange(in_loc(0),k,loc_1d_upts_rest)*eval_lagrange(in_loc(1),j,loc_1d_upts_rest)*eval_lagrange(in_loc(2),i,loc_1d_upts_rest);

  return nodal_basis;
}

// evaluate derivative of nodal basis

double eles_hexas::eval_d_nodal_basis(int in_index, int in_cpnt, array<double> in_loc)
{
  int i,j,k;

  double d_nodal_basis;

  i=(in_index/((order+1)*(order+1)));
  j=(in_index-((order+1)*(order+1)*i))/(order+1);
  k=in_index-((order+1)*j)-((order+1)*(order+1)*i);

  if(in_cpnt==0)
    {
      d_nodal_basis=eval_d_lagrange(in_loc(0),k,loc_1d_upts)*eval_lagrange(in_loc(1),j,loc_1d_upts)*eval_lagrange(in_loc(2),i,loc_1d_upts);
    }
  else if(in_cpnt==1)
    {
      d_nodal_basis=eval_lagrange(in_loc(0),k,loc_1d_upts)*eval_d_lagrange(in_loc(1),j,loc_1d_upts)*eval_lagrange(in_loc(2),i,loc_1d_upts);
    }
  else if(in_cpnt==2)
    {
      d_nodal_basis=eval_lagrange(in_loc(0),k,loc_1d_upts)*eval_lagrange(in_loc(1),j,loc_1d_upts)*eval_d_lagrange(in_loc(2),i,loc_1d_upts);
    }
  else
    {
      cout << "ERROR: Invalid component requested ... " << endl;
    }

  return d_nodal_basis;
}

// evaluate nodal shape basis

double eles_hexas::eval_nodal_s_basis(int in_index, array<double> in_loc, int in_n_spts)
{
  int i,j,k;
  double nodal_s_basis;

  if (is_perfect_cube(in_n_spts))
    {
      int n_1d_spts = round(pow(in_n_spts,1./3.));
      array<double> loc_1d_spts(n_1d_spts);
      set_loc_1d_spts(loc_1d_spts,n_1d_spts);

      i=(in_index/(n_1d_spts*n_1d_spts));
      j=(in_index-(n_1d_spts*n_1d_spts*i))/n_1d_spts;
      k=in_index-(n_1d_spts*j)-(n_1d_spts*n_1d_spts*i);

      nodal_s_basis=eval_lagrange(in_loc(0),k,loc_1d_spts)*eval_lagrange(in_loc(1),j,loc_1d_spts)*eval_lagrange(in_loc(2),i,loc_1d_spts);
    }
  else if (in_n_spts==20) // Quadratic hex with 20 nodes
    {
      if (in_index==0)
        nodal_s_basis = (1./8.*(-1+in_loc(0)))*(-1+in_loc(1))*(-1+in_loc(2))*(in_loc(0)+2+in_loc(1)+in_loc(2));
      else if (in_index==1)
        nodal_s_basis = -(1./8.*(in_loc(0)+1))*(-1+in_loc(1))*(-1+in_loc(2))*(-in_loc(0)+2+in_loc(1)+in_loc(2));
      else if (in_index==2)
        nodal_s_basis = -(1./8.*(in_loc(0)+1))*(in_loc(1)+1)*(-1+in_loc(2))*(in_loc(0)-2+in_loc(1)-in_loc(2));
      else if (in_index==3)
        nodal_s_basis = (1./8.*(-1+in_loc(0)))*(in_loc(1)+1)*(-1+in_loc(2))*(-in_loc(0)-2+in_loc(1)-in_loc(2));
      else if (in_index==4)
        nodal_s_basis = -(1./8.*(-1+in_loc(0)))*(-1+in_loc(1))*(in_loc(2)+1)*(in_loc(0)+2+in_loc(1)-in_loc(2));
      else if (in_index==5)
        nodal_s_basis = (1./8.*(in_loc(0)+1))*(-1+in_loc(1))*(in_loc(2)+1)*(-in_loc(0)+2+in_loc(1)-in_loc(2));
      else if (in_index==6)
        nodal_s_basis = (1./8.*(in_loc(0)+1))*(in_loc(1)+1)*(in_loc(2)+1)*(in_loc(0)-2+in_loc(1)+in_loc(2));
      else if (in_index==7)
        nodal_s_basis = -(1./8.*(-1+in_loc(0)))*(in_loc(1)+1)*(in_loc(2)+1)*(-in_loc(0)-2+in_loc(1)+in_loc(2));
      else if (in_index==8)
        nodal_s_basis = -(1./4.*(-1+in_loc(1)))*(-1+in_loc(2))*(in_loc(0)*in_loc(0)-1);
      else if (in_index==9)
        nodal_s_basis = (1./4.*(in_loc(0)+1))*(-1+in_loc(2))*(in_loc(1)*in_loc(1)-1);
      else if (in_index==10)
        nodal_s_basis  = (1./4.*(in_loc(1)+1))*(-1+in_loc(2))*(in_loc(0)*in_loc(0)-1);
      else if (in_index==11)
        nodal_s_basis  = -(1./4.*(-1+in_loc(0)))*(-1+in_loc(2))*(in_loc(1)*in_loc(1)-1);
      else if (in_index==12)
        nodal_s_basis  = -(1./4.*(-1+in_loc(0)))*(-1+in_loc(1))*(in_loc(2)*in_loc(2)-1);
      else if (in_index==13)
        nodal_s_basis  = (1./4.*(in_loc(0)+1))*(-1+in_loc(1))*(in_loc(2)*in_loc(2)-1);
      else if (in_index==14)
        nodal_s_basis  = -(1./4.*(in_loc(0)+1))*(in_loc(1)+1)*(in_loc(2)*in_loc(2)-1);
      else if (in_index==15)
        nodal_s_basis  = (1./4.*(-1+in_loc(0)))*(in_loc(1)+1)*(in_loc(2)*in_loc(2)-1);
      else if (in_index==16)
        nodal_s_basis  = (1./4.*(-1+in_loc(1)))*(in_loc(2)+1)*(in_loc(0)*in_loc(0)-1);
      else if (in_index==17)
        nodal_s_basis  = -(1./4.*(in_loc(0)+1))*(in_loc(2)+1)*(in_loc(1)*in_loc(1)-1);
      else if (in_index==18)
        nodal_s_basis  = -(1./4.*(in_loc(1)+1))*(in_loc(2)+1)*(in_loc(0)*in_loc(0)-1);
      else if (in_index==19)
        nodal_s_basis  = (1./4.*(-1+in_loc(0)))*(in_loc(2)+1)*(in_loc(1)*in_loc(1)-1);
    }
  else
    {
      cout << "in_n_spts = " << in_n_spts << endl;
      FatalError("Shape basis not implemented yet, exiting");
    }

  return nodal_s_basis;
}

// evaluate derivative of nodal shape basis

void eles_hexas::eval_d_nodal_s_basis(array<double> &d_nodal_s_basis, array<double> in_loc, int in_n_spts)
{
  int i,j,k;

  if (is_perfect_cube(in_n_spts))
    {
      int n_1d_spts = round(pow(in_n_spts,1./3.));
      array<double> loc_1d_spts(n_1d_spts);
      set_loc_1d_spts(loc_1d_spts,n_1d_spts);

      for (int m=0;m<in_n_spts;++m)
        {
          i=(m/(n_1d_spts*n_1d_spts));
          j=(m-(n_1d_spts*n_1d_spts*i))/n_1d_spts;
          k=m-(n_1d_spts*j)-(n_1d_spts*n_1d_spts*i);

          d_nodal_s_basis(m,0)=eval_d_lagrange(in_loc(0),k,loc_1d_spts)*eval_lagrange(in_loc(1),j,loc_1d_spts)*eval_lagrange(in_loc(2),i,loc_1d_spts);
          d_nodal_s_basis(m,1)=eval_lagrange(in_loc(0),k,loc_1d_spts)*eval_d_lagrange(in_loc(1),j,loc_1d_spts)*eval_lagrange(in_loc(2),i,loc_1d_spts);
          d_nodal_s_basis(m,2)=eval_lagrange(in_loc(0),k,loc_1d_spts)*eval_lagrange(in_loc(1),j,loc_1d_spts)*eval_d_lagrange(in_loc(2),i,loc_1d_spts);

        }

    }
  else if (in_n_spts==20)
    {
      d_nodal_s_basis(0 ,0) = (1./8.*(in_loc(2)-1))*(-1+in_loc(1))*(in_loc(1)+in_loc(2)+2*in_loc(0)+1);
      d_nodal_s_basis(1 ,0) =  -(1./8.*(in_loc(2)-1))*(-1+in_loc(1))*(in_loc(1)+in_loc(2)-2*in_loc(0)+1) ;
      d_nodal_s_basis(2 ,0) =  -(1./8.*(in_loc(2)-1))*(in_loc(1)+1)*(in_loc(1)-in_loc(2)+2*in_loc(0)-1);
      d_nodal_s_basis(3 ,0) =  (1./8.*(in_loc(2)-1))*(in_loc(1)+1)*(in_loc(1)-in_loc(2)-2*in_loc(0)-1);
      d_nodal_s_basis(4 ,0) =  -(1./8.*(in_loc(2)+1))*(-1+in_loc(1))*(in_loc(1)-in_loc(2)+2*in_loc(0)+1);
      d_nodal_s_basis(5 ,0) =  (1./8.*(in_loc(2)+1))*(-1+in_loc(1))*(in_loc(1)-in_loc(2)-2*in_loc(0)+1);
      d_nodal_s_basis(6 ,0) =  (1./8.*(in_loc(2)+1))*(in_loc(1)+1)*(in_loc(1)+in_loc(2)+2*in_loc(0)-1);
      d_nodal_s_basis(7 ,0) =  -(1./8.*(in_loc(2)+1))*(in_loc(1)+1)*(in_loc(1)+in_loc(2)-1-2*in_loc(0));
      d_nodal_s_basis(8 ,0) =  -(1./2.)*in_loc(0)*(in_loc(2)-1)*(-1+in_loc(1));
      d_nodal_s_basis(9 ,0) =  (1./4.*(in_loc(2)-1))*(in_loc(1)*in_loc(1)-1);
      d_nodal_s_basis(10,0) =   (1./2.)*in_loc(0)*(in_loc(2)-1)*(in_loc(1)+1);
      d_nodal_s_basis(11,0) =   -(1./4.*(in_loc(2)-1))*(in_loc(1)*in_loc(1)-1);
      d_nodal_s_basis(12,0) =   -(1./4.*(-1+in_loc(1)))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(13,0) =   (1./4.*(-1+in_loc(1)))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(14,0) =   -(1./4.*(in_loc(1)+1))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(15,0) =   (1./4.*(in_loc(1)+1))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(16,0) =   (1./2.)*in_loc(0)*(in_loc(2)+1)*(-1+in_loc(1));
      d_nodal_s_basis(17,0) =   -(1./4.*(in_loc(2)+1))*(in_loc(1)*in_loc(1)-1);
      d_nodal_s_basis(18,0) =   -(1./2.)*in_loc(0)*(in_loc(2)+1)*(in_loc(1)+1);
      d_nodal_s_basis(19,0) =   (1./4.*(in_loc(2)+1))*(in_loc(1)*in_loc(1)-1);

      d_nodal_s_basis(0 ,1) = (1./8.*(in_loc(2)-1))*(-1+in_loc(0))*(in_loc(0)+in_loc(2)+2*in_loc(1)+1);
      d_nodal_s_basis(1 ,1) =  -(1./8.*(in_loc(2)-1))*(in_loc(0)+1)*(-in_loc(0)+in_loc(2)+2*in_loc(1)+1);
      d_nodal_s_basis(2 ,1) =  -(1./8.*(in_loc(2)-1))*(in_loc(0)+1)*(in_loc(0)-in_loc(2)+2*in_loc(1)-1);
      d_nodal_s_basis(3 ,1) =  (1./8.*(in_loc(2)-1))*(-1+in_loc(0))*(-in_loc(0)-in_loc(2)+2*in_loc(1)-1);
      d_nodal_s_basis(4 ,1) =  -(1./8.*(in_loc(2)+1))*(-1+in_loc(0))*(in_loc(0)-in_loc(2)+2*in_loc(1)+1);
      d_nodal_s_basis(5 ,1) =  (1./8.*(in_loc(2)+1))*(in_loc(0)+1)*(-in_loc(0)-in_loc(2)+2*in_loc(1)+1);
      d_nodal_s_basis(6 ,1) =  (1./8.*(in_loc(2)+1))*(in_loc(0)+1)*(in_loc(0)+in_loc(2)-1+2*in_loc(1));
      d_nodal_s_basis(7 ,1) =  -(1./8.*(in_loc(2)+1))*(-1+in_loc(0))*(-in_loc(0)+in_loc(2)-1+2*in_loc(1));
      d_nodal_s_basis(8 ,1) =  -(1./4.*(in_loc(2)-1))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(9 ,1) =  (1./2.)*in_loc(1)*(in_loc(2)-1)*(in_loc(0)+1);
      d_nodal_s_basis(10,1) =   (1./4.*(in_loc(2)-1))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(11,1) =   -(1./2.)*in_loc(1)*(in_loc(2)-1)*(-1+in_loc(0));
      d_nodal_s_basis(12,1) =   -(1./4.*(-1+in_loc(0)))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(13,1) =   (1./4.*(in_loc(0)+1))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(14,1) =   -(1./4.*(in_loc(0)+1))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(15,1) =   (1./4.*(-1+in_loc(0)))*(in_loc(2)*in_loc(2)-1);
      d_nodal_s_basis(16,1) =   (1./4.*(in_loc(2)+1))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(17,1) =   -(1./2.)*in_loc(1)*(in_loc(2)+1)*(in_loc(0)+1);
      d_nodal_s_basis(18,1) =   -(1./4.*(in_loc(2)+1))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(19,1) =   (1./2.)*in_loc(1)*(in_loc(2)+1)*(-1+in_loc(0));

      d_nodal_s_basis(0 ,2) = (1./8.*(-1+in_loc(0)))*(-1+in_loc(1))*(in_loc(1)+in_loc(0)+2*in_loc(2)+1);
      d_nodal_s_basis(1 ,2) =  -(1./8.*(in_loc(1)-1))*(in_loc(0)+1)*(-in_loc(0)+in_loc(1)+2*in_loc(2)+1);
      d_nodal_s_basis(2 ,2) =  -(1./8.*(in_loc(0)+1))*(in_loc(1)+1)*(in_loc(1)+in_loc(0)-2*in_loc(2)-1);
      d_nodal_s_basis(3 ,2) =  (1./8.*(-1+in_loc(0)))*(in_loc(1)+1)*(in_loc(1)-in_loc(0)-2*in_loc(2)-1);
      d_nodal_s_basis(4 ,2) =  -(1./8.*(-1+in_loc(0)))*(-1+in_loc(1))*(in_loc(1)+in_loc(0)-2*in_loc(2)+1);
      d_nodal_s_basis(5 ,2) =  (1./8.*(in_loc(0)+1))*(-1+in_loc(1))*(in_loc(1)-in_loc(0)-2*in_loc(2)+1);
      d_nodal_s_basis(6 ,2) =  (1./8.*(in_loc(0)+1))*(in_loc(1)+1)*(in_loc(1)+in_loc(0)+2*in_loc(2)-1);
      d_nodal_s_basis(7 ,2) =  -(1./8.*(-1+in_loc(0)))*(in_loc(1)+1)*(in_loc(1)-in_loc(0)+2*in_loc(2)-1);
      d_nodal_s_basis(8 ,2) =  -(1./4.*(-1+in_loc(1)))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(9 ,2) =  (1./4.*(in_loc(0)+1))*(in_loc(1)*in_loc(1)-1);
      d_nodal_s_basis(10,2) =   (1./4.*(in_loc(1)+1))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(11,2) =   -(1./4.*(-1+in_loc(0)))*(in_loc(1)*in_loc(1)-1);
      d_nodal_s_basis(12,2) =   -(1./2.)*in_loc(2)*(-1+in_loc(0))*(-1+in_loc(1));
      d_nodal_s_basis(13,2) =   (1./2.)*in_loc(2)*(in_loc(0)+1)*(-1+in_loc(1));
      d_nodal_s_basis(14,2) =   -(1./2.)*in_loc(2)*(in_loc(0)+1)*(in_loc(1)+1);
      d_nodal_s_basis(15,2) =   (1./2.)*in_loc(2)*(-1+in_loc(0))*(in_loc(1)+1);
      d_nodal_s_basis(16,2) =   (1./4.*(-1+in_loc(1)))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(17,2) =   -(1./4.*(in_loc(0)+1))*(in_loc(1)*in_loc(1)-1);
      d_nodal_s_basis(18,2) =   -(1./4.*(in_loc(1)+1))*(in_loc(0)*in_loc(0)-1);
      d_nodal_s_basis(19,2) =   (1./4.*(-1+in_loc(0)))*(in_loc(1)*in_loc(1)-1);
    }
  else
    {
      FatalError("Shape basis not implemented yet, exiting");
    }
}

void eles_hexas::fill_opp_3(array<double>& opp_3)
{
  int i,j,k;
  array<double> loc(n_dims);

  for(i=0;i<n_fpts_per_ele;++i)
    {
      for(j=0;j<n_upts_per_ele;++j)
        {
          for(k=0;k<n_dims;++k)
            {
              loc(k)=loc_upts(k,j);
            }

          opp_3(j,i)=eval_div_vcjh_basis(i,loc);
        }
    }
}

// evaluate divergence of vcjh basis

double eles_hexas::eval_div_vcjh_basis(int in_index, array<double>& loc)
{
  int i,j,k;
  double eta;
  double div_vcjh_basis;
  int scheme = run_input.vcjh_scheme_hexa;
  
  if (scheme==0)
    eta = run_input.eta_hexa;    
  else if (scheme < 5)
    eta = compute_eta(run_input.vcjh_scheme_hexa,order);

  i=(in_index/n_fpts_per_inter(0));
  j=(in_index-(n_fpts_per_inter(0)*i))/(order+1);
  k=in_index-(n_fpts_per_inter(0)*i)-((order+1)*j);

  if (scheme < 5) {

    if(i==0)
      div_vcjh_basis = -eval_lagrange(loc(0),order-k,loc_1d_upts) * eval_lagrange(loc(1),j,loc_1d_upts) * eval_d_vcjh_1d(loc(2),0,order,eta);
    else if(i==1)
      div_vcjh_basis = -eval_lagrange(loc(0),k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_vcjh_1d(loc(1),0,order,eta);
    else if(i==2)
      div_vcjh_basis = eval_lagrange(loc(1),k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_vcjh_1d(loc(0),1,order,eta);
    else if(i==3)
      div_vcjh_basis = eval_lagrange(loc(0),order-k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_vcjh_1d(loc(1),1,order,eta);
    else if(i==4)
      div_vcjh_basis = -eval_lagrange(loc(1),order-k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_vcjh_1d(loc(0),0,order,eta);
    else if(i==5)
      div_vcjh_basis = eval_lagrange(loc(0),k,loc_1d_upts) * eval_lagrange(loc(1),j,loc_1d_upts) * eval_d_vcjh_1d(loc(2),1,order,eta);

  }
  // OFR scheme
  else if (scheme == 5) {

    if(i==0)
      div_vcjh_basis = -eval_lagrange(loc(0),order-k,loc_1d_upts) * eval_lagrange(loc(1),j,loc_1d_upts) * eval_d_ofr_1d(loc(2),0,order);
    else if(i==1)
      div_vcjh_basis = -eval_lagrange(loc(0),k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_ofr_1d(loc(1),0,order);
    else if(i==2)
      div_vcjh_basis = eval_lagrange(loc(1),k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_ofr_1d(loc(0),1,order);
    else if(i==3)
      div_vcjh_basis = eval_lagrange(loc(0),order-k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_ofr_1d(loc(1),1,order);
    else if(i==4)
      div_vcjh_basis = -eval_lagrange(loc(1),order-k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_ofr_1d(loc(0),0,order);
    else if(i==5)
      div_vcjh_basis = eval_lagrange(loc(0),k,loc_1d_upts) * eval_lagrange(loc(1),j,loc_1d_upts) * eval_d_ofr_1d(loc(2),1,order);

  }
  // OESFR scheme
  else if (scheme == 6) {

    if(i==0)
      div_vcjh_basis = -eval_lagrange(loc(0),order-k,loc_1d_upts) * eval_lagrange(loc(1),j,loc_1d_upts) * eval_d_oesfr_1d(loc(2),0,order);
    else if(i==1)
      div_vcjh_basis = -eval_lagrange(loc(0),k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_oesfr_1d(loc(1),0,order);
    else if(i==2)
      div_vcjh_basis = eval_lagrange(loc(1),k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_oesfr_1d(loc(0),1,order);
    else if(i==3)
      div_vcjh_basis = eval_lagrange(loc(0),order-k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_oesfr_1d(loc(1),1,order);
    else if(i==4)
      div_vcjh_basis = -eval_lagrange(loc(1),order-k,loc_1d_upts) * eval_lagrange(loc(2),j,loc_1d_upts) * eval_d_oesfr_1d(loc(0),0,order);
    else if(i==5)
      div_vcjh_basis = eval_lagrange(loc(0),k,loc_1d_upts) * eval_lagrange(loc(1),j,loc_1d_upts) * eval_d_oesfr_1d(loc(2),1,order);

  }
  
  return div_vcjh_basis;
}

// Get position of 1d solution point
double eles_hexas::get_loc_1d_upt(int in_index)
{
  return loc_1d_upts(in_index);
}

/*! Calculate element volume */
double eles_hexas::calc_ele_vol(double& detjac)
{
  double vol;
  // Element volume = |Jacobian|*width*height*span of reference element
  vol = detjac*8.;
  return vol;
}

/*! Calculate element reference length for timestep calculation */
double eles_hexas::calc_h_ref_specific(int in_ele)
  {
    FatalError("Reference length calculation not implemented for this element!")
  }
