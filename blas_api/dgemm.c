/**************************************************************************************************
*                                                                                                 *
* This file is part of BLASFEO.                                                                   *
*                                                                                                 *
* BLASFEO -- BLAS For Embedded Optimization.                                                      *
* Copyright (C) 2016-2018 by Gianluca Frison.                                                     *
* Developed at IMTEK (University of Freiburg) under the supervision of Moritz Diehl.              *
* All rights reserved.                                                                            *
*                                                                                                 *
* This program is free software: you can redistribute it and/or modify                            *
* it under the terms of the GNU General Public License as published by                            *
* the Free Software Foundation, either version 3 of the License, or                               *
* (at your option) any later version                                                              *.
*                                                                                                 *
* This program is distributed in the hope that it will be useful,                                 *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                                  *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                                   *
* GNU General Public License for more details.                                                    *
*                                                                                                 *
* You should have received a copy of the GNU General Public License                               *
* along with this program.  If not, see <https://www.gnu.org/licenses/>.                          *
*                                                                                                 *
* The authors designate this particular file as subject to the "Classpath" exception              *
* as provided by the authors in the LICENSE file that accompained this code.                      *
*                                                                                                 *
* Author: Gianluca Frison, gianluca.frison (at) imtek.uni-freiburg.de                             *
*                                                                                                 *
**************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "../include/blasfeo_target.h"
#include "../include/blasfeo_common.h"
#include "../include/blasfeo_d_aux.h"
#include "../include/blasfeo_d_kernel.h"



#if defined(FORTRAN_BLAS_API)
#define blasfeo_dgemm dgemm_
#endif



void blasfeo_dgemm(char *ta, char *tb, int *pm, int *pn, int *pk, double *alpha, double *A, int *plda, double *B, int *pldb, double *beta, double *C, int *pldc)
	{

#if defined(PRINT_NAME)
	printf("\nblasfeo_dgemm %c %c %d %d %d %f %p %d %p %d %f %p %d\n", *ta, *tb, *pm, *pn, *pk, *alpha, A, *plda, B, *pldb, *beta, C, *pldc);
#endif

	int m = *pm;
	int n = *pn;
	int k = *pk;
	int lda = *plda;
	int ldb = *pldb;
	int ldc = *pldc;

	if(m<=0 | n<=0)
		return;

	int ii, jj;

	const int bs = 4;

// TODO visual studio alignment
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	double pU[3*4*K_MAX_STACK] __attribute__ ((aligned (64)));
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	double pU[2*4*K_MAX_STACK] __attribute__ ((aligned (64)));
#elif defined(TARGET_GENERIC)
	double pU[1*4*K_MAX_STACK];
#else
	double pU[1*4*K_MAX_STACK] __attribute__ ((aligned (64)));
#endif
	int sdu = (k+3)/4*4;
	sdu = sdu<K_MAX_STACK ? sdu : K_MAX_STACK;

	struct blasfeo_dmat sA, sB;
	int sda, sdb;
	int sA_size, sB_size;
	void *smat_mem, *smat_mem_align;
	int m1, n1, k1;
	int pack_B;

	if(*ta=='n' | *ta=='N')
		{
		if(*tb=='n' | *tb=='N')
			{
//			goto nn_2; // no pack
//			goto nn_m0; // pack A
//			goto nn_n0; // pack B
//			goto nn_1; // pack A and B
			if( k<=K_MAX_STACK )
				{
#if defined(TARGET_X64_INTEL_HASWELL)
				if( m<=48 & n<=48 )
					{
					goto nn_2; // small matrix: no pack
					}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=48 & n<=48 )
					{
					goto nn_m0; // small matrix: pack A
					}
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=24 & n<=24 )
					{
					goto nn_m0; // small matrix: pack A
					}
#else
				if( m<=8 & n<=8 )
					{
					goto nn_2; // small matrix: no pack
					}
#endif
#if defined(TARGET_X64_INTEL_HASWELL)
				if( m<=2*12 | n<=2*12 | k<448 )
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=2*8 | n<=2*8 | k<56 )
#elif defined(TARGET_X64_INTEL_CORE)
				if( m<=1*4 | n<=1*4 | k<8 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=2*8 | n<=2*8 | k<64 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A53)
				if( m<=1*12 | n<=1*12 | k<16 )
#else
				if( m<=1*4 | n<=1*4 | k<12 )
#endif
					{
					if( m<=n*4 )
						{
						goto nn_m0; // long matrix: pack A
						}
					else
						{
						goto nn_n0; // tall matrix: pack B
						}
					}
				}
			goto nn_1; // big matrix: pack A and B
			}
		else if(*tb=='t' | *tb=='T' | *tb=='c' | *tb=='C')
			{
//			goto nt_2; // no pack
//			goto nt_m0; // pack A
//			goto nt_n0; // pack B
//			goto nt_1; // pack A and B
			if( k<=K_MAX_STACK )
				{
#if defined(TARGET_X64_INTEL_HASWELL)
				if( m<=48 & n<=48 )
					{
					goto nt_2; // small matrix: no pack
					}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=48 & n<=48 )
					{
					goto nt_m0; // small matrix: pack A
					}
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=24 & n<=24 )
					{
					goto nt_m0; // small matrix: pack A
					}
#else
				if( m<=8 & n<=8 )
					{
					goto nt_2; // small matrix: no pack
					}
#endif
#if defined(TARGET_X64_INTEL_HASWELL)
				if( m<=2*12 | n<=2*12 | k<200 )
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=2*8 | n<=2*8 | k<56 )
#elif defined(TARGET_X64_INTEL_CORE)
				if( m<=1*4 | n<=1*4 | k<8 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=2*8 | n<=2*8 | k<64 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A53)
				if( m<=1*12 | n<=1*12 | k<16 )
#else
				if( m<=1*4 | n<=1*4 | k<12 )
#endif
					{
					if( m<=n )
						{
						goto nt_m0; // long matrix: pack A
						}
					else
						{
						goto nt_n0; // tall matrix: pack B
						}
					}
				}
			goto nt_1; // big matrix: pack A and B
			}
		else
			{
			printf("\nBLASFEO: dgemm: wrong value for tb\n");
			return;
			}
		}
	else if(*ta=='t' | *ta=='T' | *ta=='c' | *ta=='C')
		{
		if(*tb=='n' | *tb=='N')
			{
//			goto tn_m0; // pack A
//			goto tn_n0; // pack B
//			goto tn_1; // pack A and B
			if( k<=K_MAX_STACK )
				{
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=48 & n<=48 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=24 & n<=24 )
#else
				if( m<=8 & n<=8 )
#endif
					{
					goto tn_m0; // small matrix: pack A
					}
#if defined(TARGET_X64_INTEL_HASWELL)
				if( m<=2*12 | n<=2*12 | k<448 )
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=2*8 | n<=2*8 | k<56 )
#elif defined(TARGET_X64_INTEL_CORE)
				if( m<=1*4 | n<=1*4 | k<8 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=2*8 | n<=2*8 | k<64 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A53)
				if( m<=1*12 | n<=1*12 | k<16 )
#else
				if( m<=1*4 | n<=1*4 | k<12 )
#endif
					{
					if( m<=n )
						{
						goto tn_m0; // long matrix: pack A
						}
					else
						{
						goto tn_n0; // tall matrix: pack B
						}
					}
				}
			goto tn_1; // big matrix: pack A and B
			}
		else if(*tb=='t' | *tb=='T' | *tb=='c' | *tb=='C')
			{
//			goto tt_2; // no pack
//			goto tt_m0; // pack A
//			goto tt_n0; // pack B
//			goto tt_1; // pack A and B
			if( k<=K_MAX_STACK )
				{
#if defined(TARGET_X64_INTEL_HASWELL)
				if( m<=48 & n<=48 )
					{
					goto tt_2; // small matrix: no pack
					}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=48 & n<=48 )
					{
					goto tt_m0; // small matrix: pack A
					}
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=24 & n<=24 )
					{
					goto tt_m0; // small matrix: pack A
					}
#else
				if( m<=8 & n<=8 )
					{
					goto tt_2; // small matrix: no pack
					}
#endif
#if defined(TARGET_X64_INTEL_HASWELL)
				if( m<=2*12 | n<=2*12 | k<448 )
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE)
				if( m<=2*8 | n<=2*8 | k<56 )
#elif defined(TARGET_X64_INTEL_CORE)
				if( m<=1*4 | n<=1*4 | k<8 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A57)
				if( m<=2*8 | n<=2*8 | k<64 )
#elif defined(TARGET_ARMV8A_ARM_CORTEX_A53)
				if( m<=1*12 | n<=1*12 | k<16 )
#else
				if( m<=1*4 | n<=1*4 | k<12 )
#endif
					{
					if( m*4<=n | k<=4 ) // XXX k too !!!
						{
						goto tt_m0; // long matrix: pack A
						}
					else
						{
						goto tt_n0; // tall matrix: pack B
						}
					}
				}
			goto tt_1; // big matrix: pack A and B
			}
		}
	else
		{
		printf("\nBLASFEO: dgemm: wrong value for ta\n");
		return;
		}

	return;
	


nn_m0:
	
	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_nn_12_lib4(k, A+ii+0, lda, pU, sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_12x4_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nn_m0_left_4;
			}
		if(m-ii<=8)
			{
			goto nn_m0_left_8;
			}
		else
			{
			goto nn_m0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_nn_8_lib4(k, A+ii+0, lda, pU, sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_8x4_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nn_m0_left_4;
			}
		else
			{
			goto nn_m0_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_nn_4_lib4(k, A+ii, lda, pU);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_4x4_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_4x4_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto nn_m0_left_4;
		}
#endif
	goto nn_m0_return;

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nn_m0_left_12:
	kernel_dpack_nn_12_vs_lib4(k, A+ii, lda, pU, sdu, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_m0_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nn_m0_left_8:
	kernel_dpack_nn_8_vs_lib4(k, A+ii, lda, pU, sdu, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_m0_return;
#endif

nn_m0_left_4:
	kernel_dpack_nn_4_vs_lib4(k, A+ii, lda, pU, m-ii);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(jj=0; jj<n-8; jj+=12)
		{
		kernel_dgemm_nn_4x12_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(jj<n-4)
		{
		kernel_dgemm_nn_4x8_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(jj<n)
		{
		kernel_dgemm_nn_4x4_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_4x4_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto nn_m0_return;

nn_m0_return:
	return;



nn_n0:

	jj = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; jj<n-11; jj+=12)
		{
		kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
		kernel_dpack_tn_4_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu);
		kernel_dpack_tn_4_lib4(k, B+(jj+8)*ldb, ldb, pU+8*sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_nt_4x12_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_nt_4x12_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto nn_n0_left_4;
			}
		else if(n-jj<=8)
			{
			goto nn_n0_left_8;
			}
		else
			{
			goto nn_n0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; jj<n-7; jj+=8)
		{
		kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
		kernel_dpack_tn_4_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_nt_4x8_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
#if defined(TARGET_X64_INTEL_SANDY_BRIDGE)
			kernel_dgemm_nt_4x8_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
			kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
			kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto nn_n0_left_4;
			}
		else
			{
			goto nn_n0_left_8;
			}
		}
#else
	for(; jj<n-3; jj+=4)
		{
		kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_nt_4x4_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		goto nn_n0_left_4;
		}
#endif
	goto nn_n0_return;

#if defined(TARGET_X64_INTEL_HASWELL)
nn_n0_left_12:
	kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
	kernel_dpack_tn_4_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu);
	kernel_dpack_tn_4_vs_lib4(k, B+(jj+8)*ldb, ldb, pU+8*sdu, n-jj-8);
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_nt_4x12_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_n0_return;
#endif


#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nn_n0_left_8:
	kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
	kernel_dpack_tn_4_vs_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu, n-jj-4);
	for(ii=0; ii<m; ii+=4)
		{
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
		kernel_dgemm_nt_4x8_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
		}
	goto nn_n0_return;
#endif

nn_n0_left_4:
	kernel_dpack_tn_4_vs_lib4(k, B+(jj+0)*ldb, ldb, pU+0*sdu, n-jj-0);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(ii=0; ii<m-8; ii+=12)
		{
		kernel_dgemm_nt_12x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(ii<m-4)
		{
		kernel_dgemm_nt_8x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(ii<m)
		{
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto nn_n0_return;

nn_n0_return:
	return;



nn_1:

	k1 = (k+128-1)/128*128;
	n1 = (n+128-1)/128*128;
	sA_size = blasfeo_memsize_dmat(12, k1);
	sB_size = blasfeo_memsize_dmat(n1, k1);
	smat_mem = malloc(sA_size+sB_size+64);
	blasfeo_align_64_byte(smat_mem, &smat_mem_align);
	// TODO smaller for non-haswell !!!
	blasfeo_create_dmat(12, k, &sA, smat_mem_align);
	blasfeo_create_dmat(n, k, &sB, smat_mem_align+sA_size);

	sda = sA.cn;
	sdb = sB.cn;

//	blasfeo_pack_tran_dmat(k, n, B, ldb, &sB, 0, 0);
	pack_B = 1;

	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_nn_12_lib4(k, A+ii, lda, sA.pA, sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			if(pack_B)
				kernel_dpack_tn_4_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb);
			kernel_dgemm_nt_12x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			if(pack_B)
				kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
			kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		pack_B = 0;
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nn_1_left_4;
			}
		if(m-ii<=8)
			{
			goto nn_1_left_8;
			}
		else
			{
			goto nn_1_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_nn_8_lib4(k, A+ii, lda, sA.pA, sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			if(pack_B)
				kernel_dpack_tn_4_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb);
			kernel_dgemm_nt_8x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			if(pack_B)
				kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
			kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		pack_B = 0;
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nn_1_left_4;
			}
		else
			{
			goto nn_1_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_nn_4_lib4(k, A+ii, lda, sA.pA);
		for(jj=0; jj<n-3; jj+=4)
			{
			if(pack_B)
				kernel_dpack_tn_4_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb);
			kernel_dgemm_nt_4x4_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			if(pack_B)
				kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
			kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		pack_B = 0;
		}
	if(ii<m)
		{
		goto nn_1_left_4;
		}
#endif
	goto nn_1_return;

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nn_1_left_12:
	kernel_dpack_nn_12_vs_lib4(k, A+ii, lda, sA.pA, sda, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		if(pack_B)
			kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
		kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_1_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nn_1_left_8:
	kernel_dpack_nn_8_vs_lib4(k, A+ii, lda, sA.pA, sda, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		if(pack_B)
			kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
		kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_1_return;
#endif

nn_1_left_4:
	kernel_dpack_nn_4_vs_lib4(k, A+ii, lda, sA.pA, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		if(pack_B)
			kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
		kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_1_return;

nn_1_return:
	free(smat_mem);
	return;



nn_2:
	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-11; ii+=12)
		{
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_12x4_libcccc(k, alpha, A+ii, lda, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_12x4_vs_libcccc(k, alpha, A+ii, lda, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nn_2_left_4;
			}
		if(m-ii<=8)
			{
			goto nn_2_left_8;
			}
		else
			{
			goto nn_2_left_12;
			}
		}
#elif ! defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	for(; ii<m-3; ii+=4)
		{
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_4x4_libcccc(k, alpha, A+ii, lda, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_4x4_vs_libcccc(k, alpha, A+ii, lda, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto nn_2_left_4;
		}
#endif
	goto nn_2_return;

#if defined(TARGET_X64_INTEL_HASWELL)
nn_2_left_12:
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_12x4_vs_libcccc(k, alpha, A+ii, lda, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_2_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL)
nn_2_left_8:
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_8x4_vs_libcccc(k, alpha, A+ii, lda, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_2_return;
#endif

#if ! defined(TARGET_X64_INTEL_SANDY_BRIDGE)
nn_2_left_4:
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_4x4_vs_libcccc(k, alpha, A+ii, lda, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nn_2_return;
#endif

nn_2_return:
	return;



nt_m0:

	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_nn_12_lib4(k, A+ii+0, lda, pU, sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_12x4_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nt_m0_left_4;
			}
		if(m-ii<=8)
			{
			goto nt_m0_left_8;
			}
		else
			{
			goto nt_m0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_nn_8_lib4(k, A+ii+0, lda, pU, sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_8x4_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nt_m0_left_4;
			}
		else
			{
			goto nt_m0_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_nn_4_lib4(k, A+ii, lda, pU);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_4x4_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_4x4_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto nt_m0_left_4;
		}
#endif
	goto nt_m0_return;

#if defined(TARGET_X64_INTEL_HASWELL)
nt_m0_left_12:
	kernel_dpack_nn_12_vs_lib4(k, A+ii+0, lda, pU, sdu, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_m0_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nt_m0_left_8:
	kernel_dpack_nn_8_vs_lib4(k, A+ii+0, lda, pU, sdu, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_m0_return;
#endif

nt_m0_left_4:
	kernel_dpack_nn_4_vs_lib4(k, A+ii, lda, pU, m-ii);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(jj=0; jj<n-8; jj+=12)
		{
		kernel_dgemm_nt_4x12_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(jj<n-4)
		{
		kernel_dgemm_nt_4x8_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(jj<n)
		{
		kernel_dgemm_nt_4x4_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_4x4_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto nt_m0_return;

nt_m0_return:
	return;



nt_n0:

	jj = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; jj<n-11; jj+=12)
		{
		kernel_dpack_nn_12_lib4(k, B+jj, ldb, pU, sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_nt_4x12_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_nt_4x12_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto nt_n0_left_4;
			}
		else if(n-jj<=8)
			{
			goto nt_n0_left_8;
			}
		else
			{
			goto nt_n0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; jj<n-7; jj+=8)
		{
		kernel_dpack_nn_8_lib4(k, B+jj, ldb, pU, sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_nt_4x8_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
#if defined(TARGET_X64_INTEL_SANDY_BRIDGE)
			kernel_dgemm_nt_4x8_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
			kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
			kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto nt_n0_left_4;
			}
		else
			{
			goto nt_n0_left_8;
			}
		}
#else
	for(; jj<n-3; jj+=4)
		{
		kernel_dpack_nn_4_lib4(k, B+jj, ldb, pU);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_nt_4x4_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		goto nt_n0_left_4;
		}
#endif
	goto nt_n0_return;

#if defined(TARGET_X64_INTEL_HASWELL)
nt_n0_left_12:
	kernel_dpack_nn_12_vs_lib4(k, B+jj, ldb, pU, sdu, n-jj);
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_nt_4x12_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_n0_return;
#endif


#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nt_n0_left_8:
	kernel_dpack_nn_8_vs_lib4(k, B+jj, ldb, pU, sdu, n-jj);
	for(ii=0; ii<m; ii+=4)
		{
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
		kernel_dgemm_nt_4x8_vs_libc4cc(k, alpha, A+ii, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
		}
	goto nt_n0_return;
#endif

nt_n0_left_4:
	kernel_dpack_nn_4_vs_lib4(k, B+jj, ldb, pU, n-jj);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(ii=0; ii<m-8; ii+=12)
		{
		kernel_dgemm_nt_12x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(ii<m-4)
		{
		kernel_dgemm_nt_8x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(ii<m)
		{
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_nt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto nt_n0_return;

nt_n0_return:
	return;



nt_1:

	k1 = (k+128-1)/128*128;
	n1 = (n+128-1)/128*128;
	sA_size = blasfeo_memsize_dmat(12, k1);
	sB_size = blasfeo_memsize_dmat(n1, k1);
	smat_mem = malloc(sA_size+sB_size+64);
	blasfeo_align_64_byte(smat_mem, &smat_mem_align);
	// TODO smaller for non-haswell !!!
	blasfeo_create_dmat(12, k, &sA, smat_mem_align);
	blasfeo_create_dmat(n, k, &sB, smat_mem_align+sA_size);

	sda = sA.cn;
	sdb = sB.cn;

//	blasfeo_pack_dmat(n, k, B, ldb, &sB, 0, 0);
	for(ii=0; ii<k-3; ii+=4)
		{
		kernel_dpack_tt_4_lib4(n, B+ii*ldb, ldb, sB.pA+ii*bs, sdb);
		}
	if(ii<k)
		{
		kernel_dpack_tt_4_vs_lib4(n, B+ii*ldb, ldb, sB.pA+ii*bs, sdb, k-ii);
		}

	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_nn_12_lib4(k, A+ii, lda, sA.pA, sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_12x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nt_1_left_4;
			}
		if(m-ii<=8)
			{
			goto nt_1_left_8;
			}
		else
			{
			goto nt_1_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_nn_8_lib4(k, A+ii, lda, sA.pA, sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_8x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nt_1_left_4;
			}
		else
			{
			goto nt_1_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_nn_4_lib4(k, A+ii, lda, sA.pA);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_4x4_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto nt_1_left_4;
		}
#endif
	goto nt_1_return;

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nt_1_left_12:
	kernel_dpack_nn_12_vs_lib4(k, A+ii, lda, sA.pA, sda, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_1_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
nt_1_left_8:
	kernel_dpack_nn_8_vs_lib4(k, A+ii, lda, sA.pA, sda, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_1_return;
#endif

nt_1_left_4:
	kernel_dpack_nn_4_vs_lib4(k, A+ii, lda, sA.pA, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_1_return;

nt_1_return:
	free(smat_mem);
	return;



nt_2:
	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-11; ii+=12)
		{
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_12x4_libcccc(k, alpha, A+ii, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_12x4_vs_libcccc(k, alpha, A+ii, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto nt_2_left_4;
			}
		if(m-ii<=8)
			{
			goto nt_2_left_8;
			}
		else
			{
			goto nt_2_left_12;
			}
		}
#elif ! defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	for(; ii<m-3; ii+=4)
		{
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_4x4_libcccc(k, alpha, A+ii, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_4x4_vs_libcccc(k, alpha, A+ii, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto nt_2_left_4;
		}
#endif
	goto nt_2_return;

#if defined(TARGET_X64_INTEL_HASWELL)
nt_2_left_12:
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_12x4_vs_libcccc(k, alpha, A+ii, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_2_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL)
nt_2_left_8:
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_8x4_vs_libcccc(k, alpha, A+ii, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_2_return;
#endif

#if ! defined(TARGET_X64_INTEL_SANDY_BRIDGE)
nt_2_left_4:
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_4x4_vs_libcccc(k, alpha, A+ii, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto nt_2_return;
#endif

nt_2_return:
	return;



tn_m0:

	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu);
		kernel_dpack_tn_4_lib4(k, A+(ii+8)*lda, lda, pU+8*sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_12x4_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tn_m0_left_4;
			}
		if(m-ii<=8)
			{
			goto tn_m0_left_8;
			}
		else
			{
			goto tn_m0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_8x4_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tn_m0_left_4;
			}
		else
			{
			goto tn_m0_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_tn_4_lib4(k, A+ii*lda, lda, pU);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nn_4x4_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nn_4x4_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto tn_m0_left_4;
		}
#endif
	goto tn_m0_return;

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tn_m0_left_12:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
	kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+8)*lda, lda, pU+8*sdu, m-ii-8);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tn_m0_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tn_m0_left_8:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu, m-ii-4);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tn_m0_return;
#endif

tn_m0_left_4:
	kernel_dpack_tn_4_vs_lib4(k, A+ii*lda, lda, pU, m-ii);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(jj=0; jj<n-8; jj+=12)
		{
		kernel_dgemm_nn_4x12_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(jj<n-4)
		{
		kernel_dgemm_nn_4x8_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(jj<n)
		{
		kernel_dgemm_nn_4x4_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nn_4x4_vs_lib4ccc(k, alpha, pU, B+jj*ldb, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto tn_m0_return;

tn_m0_return:
	return;



tn_n0:

	jj = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; jj<n-11; jj+=12)
		{
		kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
		kernel_dpack_tn_4_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu);
		kernel_dpack_tn_4_lib4(k, B+(jj+8)*ldb, ldb, pU+8*sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x12_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_tt_4x12_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto tn_n0_left_4;
			}
		else if(n-jj<=8)
			{
			goto tn_n0_left_8;
			}
		else
			{
			goto tn_n0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; jj<n-7; jj+=8)
		{
		kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
		kernel_dpack_tn_4_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x8_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
#if defined(TARGET_X64_INTEL_SANDY_BRIDGE)
			kernel_dgemm_tt_4x8_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
			kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
			kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto tn_n0_left_4;
			}
		else
			{
			goto tn_n0_left_8;
			}
		}
#else
	for(; jj<n-3; jj+=4)
		{
		kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x4_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		goto tn_n0_left_4;
		}
#endif
	goto tn_n0_return;

#if defined(TARGET_X64_INTEL_HASWELL)
tn_n0_left_12:
	kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
	kernel_dpack_tn_4_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu);
	kernel_dpack_tn_4_vs_lib4(k, B+(jj+8)*ldb, ldb, pU+8*sdu, n-jj-8);
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_tt_4x12_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tn_n0_return;
#endif


#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tn_n0_left_8:
	kernel_dpack_tn_4_lib4(k, B+(jj+0)*ldb, ldb, pU);
	kernel_dpack_tn_4_vs_lib4(k, B+(jj+4)*ldb, ldb, pU+4*sdu, n-jj-4);
	for(ii=0; ii<m; ii+=4)
		{
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
		kernel_dgemm_tt_4x8_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
		}
	goto tn_n0_return;
#endif

tn_n0_left_4:
	kernel_dpack_tn_4_vs_lib4(k, B+(jj+0)*ldb, ldb, pU+0*sdu, n-jj-0);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(ii=0; ii<m-8; ii+=12)
		{
		kernel_dgemm_tt_12x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(ii<m-4)
		{
		kernel_dgemm_tt_8x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(ii<m)
		{
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto tn_n0_return;

tn_n0_return:
	return;



tn_1:

	k1 = (k+128-1)/128*128;
	n1 = (n+128-1)/128*128;
	sA_size = blasfeo_memsize_dmat(12, k1);
	sB_size = blasfeo_memsize_dmat(n1, k1);
	smat_mem = malloc(sA_size+sB_size+64);
	blasfeo_align_64_byte(smat_mem, &smat_mem_align);
	// TODO smaller for non-haswell !!!
	blasfeo_create_dmat(12, k, &sA, smat_mem_align);
	blasfeo_create_dmat(n, k, &sB, smat_mem_align+sA_size);

	sda = sA.cn;
	sdb = sB.cn;

//	blasfeo_pack_tran_dmat(k, n, B, ldb, &sB, 0, 0);
	pack_B = 1;

	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda);
		kernel_dpack_tn_4_lib4(k, A+(ii+8)*lda, lda, sA.pA+8*sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			if(pack_B)
				kernel_dpack_tn_4_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb);
			kernel_dgemm_nt_12x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			if(pack_B)
				kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
			kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		pack_B = 0;
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tn_1_left_4;
			}
		if(m-ii<=8)
			{
			goto tn_1_left_8;
			}
		else
			{
			goto tn_1_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			if(pack_B)
				kernel_dpack_tn_4_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb);
			kernel_dgemm_nt_8x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			if(pack_B)
				kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
			kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		pack_B = 0;
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tn_1_left_4;
			}
		else
			{
			goto tn_1_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
		for(jj=0; jj<n-3; jj+=4)
			{
			if(pack_B)
				kernel_dpack_tn_4_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb);
			kernel_dgemm_nt_4x4_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			if(pack_B)
				kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
			kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		pack_B = 0;
		}
		if(ii<m)
		{
		goto tn_1_left_4;
		}
#endif
	goto tn_1_return;

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tn_1_left_12:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
	kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+8)*lda, lda, sA.pA+8*sda, m-ii-8);
	for(jj=0; jj<n; jj+=4)
		{
		if(pack_B)
			kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
		kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tn_1_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tn_1_left_8:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda, m-ii-4);
	for(jj=0; jj<n; jj+=4)
		{
		if(pack_B)
			kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
		kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tn_1_return;
#endif

tn_1_left_4:
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+0)*lda, lda, sA.pA, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		if(pack_B)
			kernel_dpack_tn_4_vs_lib4(k, B+jj*ldb, ldb, sB.pA+jj*sdb, n-jj);
		kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tn_1_return;

tn_1_return:
free(smat_mem);
	return;



tt_m0:

	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu);
		kernel_dpack_tn_4_lib4(k, A+(ii+8)*lda, lda, pU+8*sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_12x4_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tt_m0_left_4;
			}
		if(m-ii<=8)
			{
			goto tt_m0_left_8;
			}
		else
			{
			goto tt_m0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_8x4_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tt_m0_left_4;
			}
		else
			{
			goto tt_m0_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_tn_4_lib4(k, A+ii*lda, lda, pU);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_4x4_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_4x4_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto tt_m0_left_4;
		}
#endif
	goto tt_m0_return;

#if defined(TARGET_X64_INTEL_HASWELL)
tt_m0_left_12:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
	kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+8)*lda, lda, pU+8*sdu, m-ii-8);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_12x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_m0_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tt_m0_left_8:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, pU);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+4)*lda, lda, pU+4*sdu, m-ii-4);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_8x4_vs_lib4ccc(k, alpha, pU, sdu, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_m0_return;
#endif

tt_m0_left_4:
	kernel_dpack_tn_4_vs_lib4(k, A+ii*lda, lda, pU, m-ii);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(jj=0; jj<n-8; jj+=12)
		{
		kernel_dgemm_nt_4x12_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(jj<n-4)
		{
		kernel_dgemm_nt_4x8_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(jj<n)
		{
		kernel_dgemm_nt_4x4_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_4x4_vs_lib4ccc(k, alpha, pU, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto tt_m0_return;

tt_m0_return:
	return;



tt_n0:

	jj = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; jj<n-11; jj+=12)
		{
		kernel_dpack_nn_12_lib4(k, B+jj, ldb, pU, sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x12_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_tt_4x12_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto tt_n0_left_4;
			}
		else if(n-jj<=8)
			{
			goto tt_n0_left_8;
			}
		else
			{
			goto tt_n0_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; jj<n-7; jj+=8)
		{
		kernel_dpack_nn_8_lib4(k, B+jj, ldb, pU, sdu);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x8_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
#if defined(TARGET_X64_INTEL_SANDY_BRIDGE)
			kernel_dgemm_tt_4x8_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
			kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
			kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto tt_n0_left_4;
			}
		else
			{
			goto tt_n0_left_8;
			}
		}
#else
	for(; jj<n-3; jj+=4)
		{
		kernel_dpack_nn_4_lib4(k, B+jj, ldb, pU);
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x4_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		goto tt_n0_left_4;
		}
#endif
	goto tt_n0_return;

#if defined(TARGET_X64_INTEL_HASWELL)
tt_n0_left_12:
	kernel_dpack_nn_12_vs_lib4(k, B+jj, ldb, pU, sdu, n-jj);
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_tt_4x12_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_n0_return;
#endif


#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tt_n0_left_8:
	kernel_dpack_nn_8_vs_lib4(k, B+jj, ldb, pU, sdu, n-jj);
	for(ii=0; ii<m; ii+=4)
		{
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE)
		kernel_dgemm_tt_4x8_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, sdu, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
#else
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+0)*ldc, ldc, C+ii+(jj+0)*ldc, ldc, m-ii, n-(jj+0));
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii, lda, pU, beta, C+ii+(jj+4)*ldc, ldc, C+ii+(jj+4)*ldc, ldc, m-ii, n-(jj+4));
#endif
		}
	goto tt_n0_return;
#endif

tt_n0_left_4:
	kernel_dpack_nn_4_vs_lib4(k, B+jj, ldb, pU, n-jj);
#if defined(TARGET_X64_INTEL_HASWELL)
	for(ii=0; ii<m-8; ii+=12)
		{
		kernel_dgemm_tt_12x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	if(ii<m-4)
		{
		kernel_dgemm_tt_8x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	else if(ii<m)
		{
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#else
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_tt_4x4_vs_libc4cc(k, alpha, A+ii*lda, lda, pU, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
#endif
	goto tt_n0_return;

tt_n0_return:
	return;



tt_1:

	k1 = (k+128-1)/128*128;
	n1 = (n+128-1)/128*128;
	sA_size = blasfeo_memsize_dmat(12, k1);
	sB_size = blasfeo_memsize_dmat(n1, k1);
	smat_mem = malloc(sA_size+sB_size+64);
	blasfeo_align_64_byte(smat_mem, &smat_mem_align);
	// TODO smaller for non-haswell !!!
	blasfeo_create_dmat(12, k, &sA, smat_mem_align);
	blasfeo_create_dmat(n, k, &sB, smat_mem_align+sA_size);

	sda = sA.cn;
	sdb = sB.cn;

//	blasfeo_pack_dmat(n, k, B, ldb, &sB, 0, 0);
	for(ii=0; ii<k-3; ii+=4)
		{
		kernel_dpack_tt_4_lib4(n, B+ii*ldb, ldb, sB.pA+ii*bs, sdb);
		}
	if(ii<k)
		{
		kernel_dpack_tt_4_vs_lib4(n, B+ii*ldb, ldb, sB.pA+ii*bs, sdb, k-ii);
		}

	ii = 0;
#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
	for(; ii<m-11; ii+=12)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda);
		kernel_dpack_tn_4_lib4(k, A+(ii+8)*lda, lda, sA.pA+8*sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_12x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tt_1_left_4;
			}
		if(m-ii<=8)
			{
			goto tt_1_left_8;
			}
		else
			{
			goto tt_1_left_12;
			}
		}
#elif defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57)
	for(; ii<m-7; ii+=8)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
		kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_8x4_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		if(m-ii<=4)
			{
			goto tt_1_left_4;
			}
		else
			{
			goto tt_1_left_8;
			}
		}
#else
	for(; ii<m-3; ii+=4)
		{
		kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
		for(jj=0; jj<n-3; jj+=4)
			{
			kernel_dgemm_nt_4x4_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(jj<n)
			{
			kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(ii<m)
		{
		goto tt_1_left_4;
		}
#endif
	goto tt_1_return;

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tt_1_left_12:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
	kernel_dpack_tn_4_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+8)*lda, lda, sA.pA+8*sda, m-ii-8);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_12x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_1_return;
#endif

#if defined(TARGET_X64_INTEL_HASWELL) | defined(TARGET_X64_INTEL_SANDY_BRIDGE) | defined(TARGET_ARMV8A_ARM_CORTEX_A57) | defined(TARGET_ARMV8A_ARM_CORTEX_A53)
tt_1_left_8:
	kernel_dpack_tn_4_lib4(k, A+(ii+0)*lda, lda, sA.pA);
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+4)*lda, lda, sA.pA+4*sda, m-ii-4);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_8x4_vs_lib44cc(k, alpha, sA.pA, sda, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_1_return;
#endif

tt_1_left_4:
	kernel_dpack_tn_4_vs_lib4(k, A+(ii+0)*lda, lda, sA.pA, m-ii);
	for(jj=0; jj<n; jj+=4)
		{
		kernel_dgemm_nt_4x4_vs_lib44cc(k, alpha, sA.pA, sB.pA+jj*sdb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_1_return;

tt_1_return:
	free(smat_mem);
	return;



tt_2:

	jj = 0;
#if defined(TARGET_X64_INTEL_HASWELL)
	for(; jj<n-11; jj+=12)
		{
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x12_libcccc(k, alpha, A+ii*lda, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_tt_4x12_vs_libcccc(k, alpha, A+ii*lda, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		if(n-jj<=4)
			{
			goto tt_2_left_4;
			}
		else if(n-jj<=8)
			{
			goto tt_2_left_8;
			}
		else
			{
			goto tt_2_left_12;
			}
		}
#elif ! defined(TARGET_X64_INTEL_SANDY_BRIDGE)
	for(; jj<n-3; jj+=4)
		{
		for(ii=0; ii<m-3; ii+=4)
			{
			kernel_dgemm_tt_4x4_libcccc(k, alpha, A+ii*lda, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc);
			}
		if(ii<m)
			{
			kernel_dgemm_tt_4x4_vs_libcccc(k, alpha, A+ii*lda, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
			}
		}
	if(jj<n)
		{
		goto tt_2_left_4;
		}
#endif
	goto tt_2_return;

#if defined(TARGET_X64_INTEL_HASWELL)
tt_2_left_12:
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_tt_4x12_vs_libcccc(k, alpha, A+ii*lda, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_2_return;
#endif


#if defined(TARGET_X64_INTEL_HASWELL)
tt_2_left_8:
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_tt_4x8_vs_libcccc(k, alpha, A+ii*lda, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_2_return;
#endif

#if ! defined(TARGET_X64_INTEL_SANDY_BRIDGE)
tt_2_left_4:
	for(ii=0; ii<m; ii+=4)
		{
		kernel_dgemm_tt_4x4_vs_libcccc(k, alpha, A+ii*lda, lda, B+jj, ldb, beta, C+ii+jj*ldc, ldc, C+ii+jj*ldc, ldc, m-ii, n-jj);
		}
	goto tt_2_return;
#endif

tt_2_return:
	return;



	}

