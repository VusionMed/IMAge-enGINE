///////////////////////////////////////////////////////////////////
// 
// COPYRIGHT KONINKLIJKE PHILIPS ELECTRONICS N.V. 2015           
// All rights are reserved. Reproduction in whole or in part is   
// prohibited without the written consent of the copyright owner. 
//
// primary author(s):
//      Xiaodong TAO
//      Weiping LIU
//
// Philips Research China, Imaging System Team
//
////////////////////////////////////////////////////////////////////
#include <vector>
#include <itkVector.h>
#include <itkVectorContainer.h>

#ifndef DKIHelper_H
#define DKIHelper_H

typedef itk::Vector<double, 3> GradientDirectionType;
typedef itk::VectorContainer< unsigned int, GradientDirectionType > GradientDirectionContainerType;

void MakeMatrixA(const std::vector<float> & bValues, const GradientDirectionContainerType::Pointer gradientDirection, vnl_matrix<double> &  A)
{
	unsigned int N = gradientDirection->size();
	GradientDirectionContainerType::Iterator directionIterator = gradientDirection->Begin();
	for (unsigned int r = 0; r < N; ++r)
	{
		//first b value is zero
		double bvalue = static_cast<double>( bValues.at(r) );
		/*if ((r < 26) && (r > 0)) bvalue = 1250;*/
		double gx = directionIterator->Value()[0];
		double gy = directionIterator->Value()[1];
		double gz = directionIterator->Value()[2];
		directionIterator++;
		//cout << "diffusion encoding: " << bvalue << " " <<gx << " " << gy<< " " << gz << endl;
		A[r][0] = -bvalue * 1 * gx * gx;
		A[r][1] = -bvalue * 2 * gx * gy;
		A[r][2] = -bvalue * 1 * gy * gy;
		A[r][3] = -bvalue * 2 * gx * gz;
		A[r][4] = -bvalue * 2 * gy * gz;
		A[r][5] = -bvalue * 1 * gz * gz;
		A[r][6] = 1;
		if (A.columns() > 7)
		{
			A[r][6] = (bvalue*bvalue / 6) * gx*gx*gx*gx;
			A[r][7] = (bvalue*bvalue / 6) * gy*gy*gy*gy;
			A[r][8] = (bvalue*bvalue / 6) * gz*gz*gz*gz;
			A[r][9] = (bvalue*bvalue / 6) * 4 * gx*gx*gx*gy;
			A[r][10] = (bvalue*bvalue / 6) * 4 * gx*gx*gx*gz;
			A[r][11] = (bvalue*bvalue / 6) * 4 * gy*gy*gy*gx;
			A[r][12] = (bvalue*bvalue / 6) * 4 * gy*gy*gy*gz;
			A[r][13] = (bvalue*bvalue / 6) * 4 * gz*gz*gz*gx;
			A[r][14] = (bvalue*bvalue / 6) * 4 * gz*gz*gz*gy;
			A[r][15] = (bvalue*bvalue) * gx*gx*gy*gy;
			A[r][16] = (bvalue*bvalue) * gx*gx*gz*gz;
			A[r][17] = (bvalue*bvalue) * gy*gy*gz*gz;
			A[r][18] = (bvalue*bvalue) * 2 * gx*gx*gy*gz;
			A[r][19] = (bvalue*bvalue) * 2 * gy*gy*gx*gz;
			A[r][20] = (bvalue*bvalue) * 2 * gz*gz*gx*gy;
			A[r][21] = 1;
		}
	}
}

void MakeMatrixC(const float maxBvalue, const GradientDirectionContainerType::Pointer gradientDirection, vnl_matrix<double> &  C)
{
	unsigned int N = gradientDirection->size();
	GradientDirectionContainerType::Iterator directionIterator = gradientDirection->Begin();
	for (unsigned int r = 0; r < 3*N; ++r)
	{
		double gx = directionIterator->Value()[0];
		double gy = directionIterator->Value()[1];
		double gz = directionIterator->Value()[2];
		if ((r == N) || (r == 2 * N))
			directionIterator = gradientDirection->Begin();
		else
			directionIterator++;

		switch (r / N)
		{
		case 0:
			C[r][0] = -1 * gx*gx;
			C[r][1] = -2 * gx*gy;
			C[r][2] = -2 * gx*gz;
			C[r][3] = -1 * gy*gy;
			C[r][4] = -2 * gy*gz;
			C[r][5] = -1 * gz*gz;
			C[r][6] = 0;
			C[r][7] = 0;
			C[r][8] = 0;
			C[r][9] = 0;
			C[r][10] = 0;
			C[r][11] = 0;
			C[r][12] = 0;
			C[r][13] = 0;
			C[r][14] = 0;
			C[r][15] = 0;
			C[r][16] = 0;
			C[r][17] = 0;
			C[r][18] = 0;
			C[r][19] = 0;
			C[r][20] = 0;
			break;
		case 1:
			C[r][0] = 0;
			C[r][1] = 0;
			C[r][2] = 0;
			C[r][3] = 0;
			C[r][4] = 0;
			C[r][5] = 0;
			C[r][6] = -1 * gx*gx*gx*gx;
			C[r][7] = -4 * gx*gx*gx*gy;
			C[r][8] = -4 * gx*gx*gx*gz;
			C[r][9] = -6 * gx*gx*gy*gy;
			C[r][10] = -12 * gx*gx*gy*gz;
			C[r][11] = -6 * gx*gx*gz*gz;
			C[r][12] = -4 * gx*gy*gy*gy;
			C[r][13] = -12 * gx*gy*gy*gz;
			C[r][14] = -12 * gx*gy*gz*gz;
			C[r][15] = -4 * gx*gz*gz*gz;
			C[r][16] = -1 * gy*gy*gy*gy;
			C[r][17] = -4 * gy*gy*gy*gz;
			C[r][18] = -6 * gy*gy*gz*gz;
			C[r][19] = -4 * gy*gz*gz*gz;
			C[r][20] = -1 * gz*gz*gz*gz;
			break;
		case 2:
			C[r][0] = (-3 / maxBvalue) * 1 * gx*gx;
			C[r][1] = (-3 / maxBvalue) * 2 * gx*gy;
			C[r][2] = (-3 / maxBvalue) * 2 * gx*gz;
			C[r][3] = (-3 / maxBvalue) * 1 * gy*gy;
			C[r][4] = (-3 / maxBvalue) * 2 * gy*gz;
			C[r][5] = (-3 / maxBvalue) * 1 * gz*gz;
			C[r][6] = 1 * gx*gx*gx*gx;
			C[r][7] = 4 * gx*gx*gx*gy;
			C[r][8] = 4 * gx*gx*gx*gz;
			C[r][9] = 6 * gx*gx*gy*gy;
			C[r][10] = 12 * gx*gx*gy*gz;
			C[r][11] = 6 * gx*gx*gz*gz;
			C[r][12] = 4 * gx*gy*gy*gy;
			C[r][13] = 12 * gx*gy*gy*gz;
			C[r][14] = 12 * gx*gy*gz*gz;
			C[r][15] = 4 * gx*gz*gz*gz;
			C[r][16] = 1 * gy*gy*gy*gy;
			C[r][17] = 4 * gy*gy*gy*gz;
			C[r][18] = 6 * gy*gy*gz*gz;
			C[r][19] = 4 * gy*gz*gz*gz;
			C[r][20] = 1 * gz*gz*gz*gz;
			break;
		}
	
	}
}



#endif