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



#ifndef _RIGID_REGISTRATION_3D_H
#define _RIGID_REGISTRATION_3D_H

#include "itkImage.h"

typedef unsigned short SourceImagePixelType;

typedef itk::Image < SourceImagePixelType, 3> SourceImageType;

typedef float DiffusionCalculatorPixelType;
typedef itk::Image <DiffusionCalculatorPixelType, 3> DiffusionCalculatorImageType;

//void RigidRegistration3D(DiffusionCalculatorImageType* targetImage,
//	DiffusionCalculatorImageType* MovingImage, DiffusionCalculatorImageType::Pointer resultImage);

//template <typename T> //T pixel Type;
void RigidRegistration2D(itk::Image<float, 2>* targetImage, itk::Image<float, 2>* MovingImage, itk::Image<float, 2>* resultImage);



#endif