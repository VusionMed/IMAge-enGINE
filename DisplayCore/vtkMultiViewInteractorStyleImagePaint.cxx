/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiViewInteractorStyleImagePaint.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiViewInteractorStyleImagePaint.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVectorOperators.h"
#include "vtkCamera.h"

//#include "vtkPainter.h"
#include "vtkVusionPainter.h"
#include <vtkWorldPointPicker.h>
#include <vtkPoints.h>
#include <vtkParametricFunctionSource.h>
#include <vtkParametricSpline.h>
#include <ctime>
#include <vtkRegularPolygonSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
// Generic VTK pipeline elements
#include <vtkActor.h>
#include <vtkProperty.h>



vtkStandardNewMacro(vtkMultiViewInteractorStyleImagePaint);

//-----------------------------------------------------------------------------
class vtkMultiViewInteractorStyleImagePaint::vtkInternal
{
public:
  std::vector<vtkVector2i> points;
  int m_PixelComponents = 3;

  void AddPoint(const vtkVector2i &point)
    {
    this->points.push_back(point);
    }

  void AddPoint(int x, int y)
    {
    this->AddPoint(vtkVector2i(x, y));
    }

  vtkVector2i GetPoint(vtkIdType index) const
    {
    return this->points[index];
    }

  vtkIdType GetNumberOfPoints() const
    {
    return this->points.size();
    }

  void Clear()
    {
    this->points.clear();
    }

  void DrawPixels(const vtkVector2i& StartPos,
    const vtkVector2i& EndPos, unsigned char *pixels, int *size)
    {
    int x1=StartPos.GetX(), x2=EndPos.GetX();
    int y1=StartPos.GetY(), y2=EndPos.GetY();

    double x = x2 - x1;
    double y = y2 - y1;
    double length = sqrt( x*x + y*y );
    if(length == 0)
      {
      return;
      }
    double addx = x / length;
    double addy = y / length;

    x = x1;
    y = y1;
    int row, col;
    for(double i = 0; i < length; i += 1)
      {
      col = (int)x;
      row = (int)y;
      pixels[m_PixelComponents*(row*size[0]+col)] = 255 ^ pixels[m_PixelComponents*(row*size[0]+col)];
      pixels[m_PixelComponents*(row*size[0]+col)+1] = 255 ^ pixels[m_PixelComponents*(row*size[0]+col)+1];
      pixels[m_PixelComponents*(row*size[0]+col)+2] = 255 ^ pixels[m_PixelComponents*(row*size[0]+col)+2];
      x += addx;
      y += addy;
      }
    }
};

//----------------------------------------------------------------------------
vtkMultiViewInteractorStyleImagePaint::vtkMultiViewInteractorStyleImagePaint()
{
  this->Internal = new vtkInternal();
  this->StartPosition[0] = this->StartPosition[1] = 0;
  this->EndPosition[0] = this->EndPosition[1] = 0;
  this->Moving = 0;
  this->DrawPolygonPixels = false;
  this->RealTimePaintBrush = true;
  this->PixelArray = vtkUnsignedCharArray::New();
	this->m_NumberOfPixelComponents = 3;
	this->MultiViewer = NULL;
	this->Painter = new vtkVusionPainter<unsigned char>();

	this->CircleActor = vtkActor::New();
	this->CircleSource = vtkRegularPolygonSource::New();
	CircleSource->GeneratePolygonOff(); // Uncomment this line to generate only the outline of the circle
	CircleSource->SetNumberOfSides(50);
	CircleSource->SetRadius(10);
	CircleSource->SetCenter(0, 0, 0);	// Visualize
	CircleSource->SetNormal(0,0,1);
	vtkSmartPointer<vtkPolyDataMapper> mapper =
		vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(CircleSource->GetOutputPort());;
	CircleActor->SetMapper(mapper);
	CircleActor->GetProperty()->SetLineWidth(1.0);
	CircleActor->GetProperty()->SetColor(1, 1, 1);
	this->CircleActor->GetProperty()->SetOpacity(1.0);
	//CircleActor->VisibilityOff();
}

//----------------------------------------------------------------------------
vtkMultiViewInteractorStyleImagePaint::~vtkMultiViewInteractorStyleImagePaint()
{
  this->PixelArray->Delete();
  delete this->Internal;

  if (this->Painter)
  {
	  delete this->Painter;
	  this->Painter = nullptr;
  }

  if (this->CircleActor)
  {
	  this->CircleActor->Delete();
	  this->CircleActor = NULL;
  }

  if (this->CircleSource)
  {
	  this->CircleSource->Delete();
	  this->CircleSource = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkMultiViewInteractorStyleImagePaint::SetMultiViewer(vtkVusionMultiView *multiViewer)
{
	if (this->MultiViewer)
	{
		if (this->MultiViewer->GetRenderer()->HasViewProp(this->CircleActor))
		{
			this->MultiViewer->GetRenderer()->RemoveActor(this->CircleActor);
		}
	}

	this->MultiViewer = multiViewer;
	this->Modified();

	if (this->MultiViewer)
	{
		if (this->MultiViewer->GetBackgroundImage())
		{
			if (!this->MultiViewer->GetLabelImage())
			{
				//vtkSmartPointer<vtkImageData> labelImage = vtkSmartPointer<vtkImageData>::New();
				vtkImageData *labelImage = vtkImageData::New();
				labelImage->SetDimensions(this->MultiViewer->GetBackgroundImage()->GetDimensions());
				labelImage->SetSpacing(this->MultiViewer->GetBackgroundImage()->GetSpacing());
				labelImage->SetOrigin(this->MultiViewer->GetBackgroundImage()->GetOrigin());
				labelImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
				memset((unsigned char *)(labelImage->GetScalarPointer()),0, labelImage->GetScalarSize()*labelImage->GetNumberOfPoints());
				this->MultiViewer->SetLabelImage(labelImage);
				this->MultiViewer->SetSlice(2,this->MultiViewer->GetSlice(0));
				this->MultiViewer->SetViewModeToColorMap(2);
				//release label image
				//labelImage->Delete();
				//labelImage = NULL;
			}

	//		/*this->Painter->SetBackgroundImage(this->MultiViewer->GetBackgroundImage());
			//this->Painter->SetLabelImage(this->MultiViewer->GetLabelImage());			
			//this->Painter->SetPainterSliceOrientation(this->MultiViewer->GetSliceOrientation());	

			/*if (this->MultiViewer->GetLabelImage())
			{
				this->MultiViewer->SetViewModeToColorMap(2);
			}*/
		}

		// Create a circle
		if (!this->MultiViewer->GetRenderer()->HasViewProp(this->CircleActor))
		{
			this->MultiViewer->GetRenderer()->AddActor(this->CircleActor);
		}
	}
}

//----------------------------------------------------------------------------
std::vector<vtkVector2i> vtkMultiViewInteractorStyleImagePaint::GetPolygonPoints()
{
  return this->Internal->points;
}

//----------------------------------------------------------------------------
void vtkMultiViewInteractorStyleImagePaint::OnMouseMove()
{
	Superclass::OnMouseMove();

  if (!this->Interactor)
    {
    return;
    }

  this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
  this->EndPosition[1] = this->Interactor->GetEventPosition()[1];
  int *size = this->Interactor->GetRenderWindow()->GetSize();
  if (this->EndPosition[0] > (size[0]-1))
    {
	  this->EndPosition[0] = 0;// size[0] - 1;
    }
  if (this->EndPosition[0] < 0)
    {
    this->EndPosition[0] = 0;
    }
  if (this->EndPosition[1] > (size[1]-1))
    {
	  this->EndPosition[1] = 0;// size[1] - 1;
    }
  if (this->EndPosition[1] < 0)
    {
    this->EndPosition[1] = 0;
    }

  double mousePos[3];
  if (!this->ComputeMousePosition(EndPosition[0], EndPosition[1], mousePos))
  {
	  //this->CircleActor->VisibilityOff();
	  //this->MultiViewer->Render();
	  return;
  }

  //update circle
  {
	  ////correct rounding error for converted position
	  double motion_vector[3];
	  this->MultiViewer->GetRenderer()->GetActiveCamera()->GetDirectionOfProjection(motion_vector);
	  for (int i = 0; i < 3; i++)
	  {
		  motion_vector[i] *= -0.001;
		  mousePos[i] += motion_vector[i];
	  }

	  if(!this->CircleActor->GetVisibility()) this->CircleActor->VisibilityOn();
	  this->CircleSource->SetCenter(mousePos);
	  this->CircleSource->SetRadius(this->Painter->GetPainterRadius());
	  this->Painter->GetPainterValue() == 0 ? this->CircleActor->GetProperty()->SetColor(0, 0, 0) : this->CircleActor->GetProperty()->SetColor(0.847, 0.396, 0.310);
	  switch (this->MultiViewer->GetSliceOrientation())
	  {
	  case 0:
		  this->CircleSource->SetNormal(1, 0, 0);
		  break;
	  case 1:
		  this->CircleSource->SetNormal(0, 1, 0);
		  break;
	  case 2:
		  this->CircleSource->SetNormal(0, 0, 1);
		  break;
	  default:
		  break;
	  }
	  this->MultiViewer->Render();
  }

  if (!this->Moving)
  {
	  return;
  }

  //start painting...
  vtkVector2i lastPoint =
    this->Internal->GetPoint(
    this->Internal->GetNumberOfPoints() - 1);
  vtkVector2i newPoint(this->EndPosition[0], this->EndPosition[1]);

  double pixelMovedDistance = (lastPoint - newPoint).Norm();//assuming spacing is 1.0 isotropic, update later
	if(pixelMovedDistance > 1)//mouse changed position
    {
		//double *spacing = this->MultiViewer->GetBackgroundImage()->GetSpacing();
		int nSteps = (int)ceil(pixelMovedDistance / this->Painter->GetPainterRadius());

		for (size_t i = 0; i < nSteps; i++)
		{
			double t = (1.0 + i) / nSteps;
			vtkVector2i interpolatedPoint = t * newPoint + (1.0 - t) * lastPoint;
			this->Internal->AddPoint(interpolatedPoint);
			if (this->RealTimePaintBrush)
			{
				if (this->Internal->GetNumberOfPoints() > 1)
				{
					this->PaintLastPoint();
				}
			}			
		}

		//if (this->DrawPolygonPixels)
		//{
		//	this->DrawCircles();
		//}
    }
}

void vtkMultiViewInteractorStyleImagePaint::setCircleVisability(bool vis)
{
	if (vis)
	{
		this->CircleActor->VisibilityOn();
	}
	else {
		this->CircleActor->VisibilityOff();
	}
	this->MultiViewer->Render();
}
//----------------------------------------------------------------------------
void vtkMultiViewInteractorStyleImagePaint::OnLeftButtonDown()
{
  if (!this->Interactor)
    {
    return;
    }
  this->Moving = 1;

  vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();

  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];

  // delayed painting mode
  //{
	 // this->PixelArray->Initialize();
	 // this->PixelArray->SetNumberOfComponents(m_NumberOfPixelComponents);
	 // int *size = renWin->GetSize();
	 // this->PixelArray->SetNumberOfTuples(size[0]*size[1]);
	 // renWin->GetPixelData(0, 0, size[0]-1, size[1]-1, 1, this->PixelArray);
  //}
  
  this->Internal->Clear();
  this->Internal->AddPoint(this->StartPosition[0], this->StartPosition[1]);
  this->InvokeEvent(vtkCommand::StartInteractionEvent);

  if (this->RealTimePaintBrush)
  {
	  //this->CircleActor->VisibilityOn();
	  this->PaintLastPoint();
  }
}

//----------------------------------------------------------------------------
void vtkMultiViewInteractorStyleImagePaint::OnLeftButtonUp()
{
  if (!this->Interactor || !this->Moving)
    {
    return;
    }

  if(this->DrawPolygonPixels)
    {
    int *size = this->Interactor->GetRenderWindow()->GetSize();
    unsigned char *pixels = this->PixelArray->GetPointer(0);
    this->Interactor->GetRenderWindow()->SetPixelData(
      0, 0, size[0]-1, size[1]-1, pixels, 1);
    }

  this->Moving = 0;
  //this->CircleActor->VisibilityOff();
  //this->MultiViewer->Render();
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
  this->InvokeEvent(vtkCommand::EndInteractionEvent);

  if (!this->RealTimePaintBrush)
  {
	  double startT1 = clock();
	  this->PaintPoints();
	  double endT1 = clock();
	  std::cout << (endT1 - startT1) / CLOCKS_PER_SEC \
	  << " seconds (paintPoints)" << std::endl;
  }
 

  //Paint 2:
 // if (this->MultiViewer)
 // {
	//  vtkRenderWindowInteractor *rwiBrush = static_cast<vtkRenderWindowInteractor *>(this->MultiViewer->GetRenderWindow()->GetInteractor());
	//  if (!rwiBrush) return;

	//  vtkVusionPainter<unsigned char> *painter = new vtkVusionPainter<unsigned char>();
	//  std::cout << "Number of points catched= " << this->Internal->GetNumberOfPoints() << std::endl;

	//  vtkSmartPointer<vtkPoints> worldPoints =  vtkSmartPointer<vtkPoints>::New();

	//  for (vtkIdType i = 0; i <= this->Internal->GetNumberOfPoints() - 1; i++)
	//  {

	//	  //int i = this->Internal->GetNumberOfPoints() - 2;
	//	  //if (i < 0) return;
	//	  const vtkVector2i &pickedDisplayPoint = this->Internal->GetPoint(i);

	//	  rwiBrush->GetPicker()->Pick(pickedDisplayPoint[0],
	//		  pickedDisplayPoint[1],
	//		  0,  // always zero.
	//		  this->MultiViewer->GetRenderer());
	//	  double picked[3];
	//	  rwiBrush->GetPicker()->GetPickPosition(picked);
	//	  //std::cout << "World point position: " << picked[0] << " " << picked[1] << " " << picked[2] << std::endl;

	//	  //Off image, return. Use imageIndex is better.
	//	  if (picked[0] == 0 && picked[1] == 0 && picked[2] == 0)
	//	  {
	//		  continue;
	//	  }

	//	  worldPoints->InsertNextPoint(picked);
	//  }

	//  // Fit a spline to the points
	//  vtkSmartPointer<vtkParametricSpline> spline = vtkSmartPointer<vtkParametricSpline>::New();
	//  spline->SetPoints(worldPoints);
	//  vtkSmartPointer<vtkParametricFunctionSource> functionSource =
	//	  vtkSmartPointer<vtkParametricFunctionSource>::New();
	//  functionSource->SetParametricFunction(spline);
	//  functionSource->SetUResolution(5 * worldPoints->GetNumberOfPoints());
	//  functionSource->Update();

	//  //world points to index	  
	//  int numberOfFilteredPoints = functionSource->GetOutput()->GetNumberOfPoints();
	//  for (int i = 0; i < numberOfFilteredPoints; i++)
	//  {
	//	  double filteredWorldPos[3];
	//	  int fiteredIndex[3];
	//	  functionSource->GetOutput()->GetPoint(i, filteredWorldPos);
	//	  if (painter->GetImageIndexFromWorldPosition(filteredWorldPos, MultiViewer->GetBackgroundImage(), fiteredIndex))
	//	  {
	//		  for (int j = -5; j <= 5; j++)
	//		  {
	//			  int tmpIndex[3]; int ori[3] = { 1,0,0 };
	//			  tmpIndex[0] = fiteredIndex[0] + ori[0] * j;
	//			  tmpIndex[1] = fiteredIndex[1] + ori[1] * j;
	//			  tmpIndex[2] = 1+ fiteredIndex[2] + ori[2] * j;

	//			  painter->SetBackgroundImage(this->MultiViewer->GetBackgroundImage());
	//			  painter->SetLabelImage(this->MultiViewer->GetLabelImage());
	//			  painter->SetPainterImageIndex(tmpIndex);
	//			  painter->PaintIndex();
	//			  this->MultiViewer->SetLabelImage(painter->GetLabelImage());
	//			  //std::cout << "tmpIndex = " << tmpIndex[2] << std::endl;
	//		  }
	//	  }
	//  }


	//	//this->MultiViewer->SetLabelImage(painter->GetLabelImage());

	//	this->MultiViewer->SetSlice(2, this->MultiViewer->GetSlice(0));
	//	//std::cout << "multiviewer->getslice(2) = " << MultiViewer->GetSlice(2) << std::endl;
	//	this->MultiViewer->SetImageLayerOpacity(2, 1.0);
	//	this->MultiViewer->SetLookupTableToLinear(2);

	//	if (painter)
	//	{
	//		delete painter;
	//		painter = nullptr;
	//	}
	//}



  //============Brush mode end====================
}

//----------------------------------------------------------------------------
void vtkMultiViewInteractorStyleImagePaint::DrawPolygon()
{
  vtkNew<vtkUnsignedCharArray> tmpPixelArray;
  tmpPixelArray->DeepCopy(this->PixelArray);

  unsigned char *pixels = tmpPixelArray->GetPointer(0);
  //unsigned char *pixels = this->PixelArray->GetPointer(0);
  int *size = this->Interactor->GetRenderWindow()->GetSize();

  // draw each line segment
  for(vtkIdType i = 0; i < this->Internal->GetNumberOfPoints() - 1; i++)
    {
    const vtkVector2i &a = this->Internal->GetPoint(i);
    const vtkVector2i &b = this->Internal->GetPoint(i+1);

    this->Internal->DrawPixels(a, b, pixels, size);
    }

  // draw a line from the end to the start
  /*if(this->Internal->GetNumberOfPoints() >= 3)
    {
    const vtkVector2i &start = this->Internal->GetPoint(0);
    const vtkVector2i &end = this->Internal->GetPoint(this->Internal->GetNumberOfPoints() - 1);

    this->Internal->DrawPixels(start, end, pixels, size);
    }*/

  this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0]-1, size[1]-1, pixels, 1);

}

void vtkMultiViewInteractorStyleImagePaint::DrawCircles()
{
	//To implement for delayed painting mode
	//
}

void vtkMultiViewInteractorStyleImagePaint::PaintLastPoint()
{
	//Paint now
	//double startTotal = clock();
	//===============Test brush mode begin=========
	if (this->MultiViewer)
	{
		int lastPointIndex = this->Internal->GetNumberOfPoints() - 1;
		if (lastPointIndex < 0) return;

		const vtkVector2i pickedDisplayPoint = this->Internal->GetPoint(lastPointIndex);//&pickedDisplayPoint

		double picked[3];
		if (!this->ComputeMousePosition(pickedDisplayPoint[0], pickedDisplayPoint[1], picked)) return;	

		this->Painter->SetBackgroundImage(this->MultiViewer->GetBackgroundImage());
		this->Painter->SetLabelImage(this->MultiViewer->GetLabelImage());
		this->Painter->SetPainterWorldPostion(picked);
		//this->Painter->SetPainterShape(0);
		//this->Painter->SetPainterDiameter(10);
		//this->Painter->SetPainterValue(2);
		this->Painter->SetPainterSliceOrientation(this->MultiViewer->GetSliceOrientation());

		//double startPaint = clock();
		this->Painter->PaintPixel();
		//double endPaint = clock();
		//std::cout << (endPaint - startPaint) / CLOCKS_PER_SEC \
		//	<< " seconds (Paint!)" << std::endl;
		//this->MultiViewer->SetLabelImage(this->Painter->GetLabelImage());
		//this->MultiViewer->SetSlice(2, this->MultiViewer->GetSlice(0));
		//double startRender = clock();
		//this->MultiViewer->SetViewModeToColorMap(2);
		//this->MultiViewer->Render();
		//double endRender = clock();
		//std::cout << (endRender - startRender) / CLOCKS_PER_SEC \
		//	<< " seconds (RenderAll!)" << std::endl;
	}

	//double endTotal = clock();
	//std::cout << (endTotal - startTotal) / CLOCKS_PER_SEC \
	//<< " seconds (Total!)" << std::endl;
}

void vtkMultiViewInteractorStyleImagePaint::PaintPoints()
{
	//Delayed Paint now
	//===============Test brush mode begin=========
	if(this->MultiViewer)
	{
		 vtkRenderWindowInteractor *rwiBrush = static_cast<vtkRenderWindowInteractor *>(this->MultiViewer->GetRenderWindow()->GetInteractor());
		 if (!rwiBrush) return;
		 //vtkVusionPainter<unsigned char> *painter = new vtkVusionPainter<unsigned char>();
		 //std::cout <<"Number of points catched= " << this->Internal->GetNumberOfPoints() << std::endl;
		 //std::cout << "display postion: " << rwiBrush->GetEventPosition()[0] << " " << rwiBrush->GetEventPosition()[1] << std::endl;
		 //double startT2 = clock();
		 this->Painter->SetBackgroundImage(this->MultiViewer->GetBackgroundImage());
		 this->Painter->SetLabelImage(this->MultiViewer->GetLabelImage());
		 this->Painter->SetPainterSliceOrientation(this->MultiViewer->GetSliceOrientation());
		 for (vtkIdType i = 0; i < this->Internal->GetNumberOfPoints(); i++)
		 {
			 /*double startT21 = clock();*/
			  //int i = this->Internal->GetNumberOfPoints() - 2;
			  //if (i < 0) return;
			  const vtkVector2i &pickedDisplayPoint = this->Internal->GetPoint(i);

			  rwiBrush->GetPicker()->Pick(pickedDisplayPoint[0],
				  pickedDisplayPoint[1],
				  0,  // always zero.
				  this->MultiViewer->GetRenderer());
			  double picked[3];
			  rwiBrush->GetPicker()->GetPickPosition(picked);
			  //std::cout << "World point position: " << picked[0] << " " << picked[1] << " " << picked[2] << std::endl;
			 /* double endT21 = clock();
			  std::cout << (endT21 - startT21) / CLOCKS_PER_SEC \
				  << " seconds (display to world)" << std::endl;*/
			  //Off image, skip. Use imageIndex is better.
			  if (picked[0] == 0 && picked[1] == 0 && picked[2] == 0)
			  {
				  continue;
			  }
			  //this->Painter->SetBackgroundImage(this->MultiViewer->GetBackgroundImage());
			  //this->Painter->SetLabelImage(this->MultiViewer->GetLabelImage());
			  this->Painter->SetPainterWorldPostion(picked);
			  //this->Painter->SetPainterShape(3);
			  //this->Painter->SetPainterShape(0);
			  //this->Painter->SetPainterDiameter(10);
			  //this->Painter->SetPainterValue(2);

			  this->Painter->PaintPixel();
		 }
		 /*double endT2 = clock();
		 std::cout << (endT2 - startT2) / CLOCKS_PER_SEC \
			 << " seconds (total for loop)" << std::endl;*/

		 //double startT3 = clock();
		 //this->MultiViewer->SetLabelImage(this->Painter->GetLabelImage());//Take most of the painting time!
		 //this->MultiViewer->SetSlice(2, this->MultiViewer->GetSlice(0));
		 //std::cout << "multiviewer->getslice(2) = " << MultiViewer->GetSlice(2) << std::endl;
		 //this->MultiViewer->SetImageLayerOpacity(2, 1.0);
		 //this->MultiViewer->ImageAutoFillWindow();
		 this->MultiViewer->SetViewModeToColorMap(2);
		 /*double endT3 = clock();
		 std::cout << (endT3- startT3) / CLOCKS_PER_SEC \
			 << " seconds (setting multiViewer)" << std::endl;*/
	}

}

void vtkMultiViewInteractorStyleImagePaint::OnMouseWheelForward() {

}

void vtkMultiViewInteractorStyleImagePaint::OnMouseWheelBackward() {

}

int vtkMultiViewInteractorStyleImagePaint::ComputeMousePosition(int x, int y, double *convertedPos)
{
	if (!this->MultiViewer) return 0;
	vtkRenderWindowInteractor *rwiBrush = static_cast<vtkRenderWindowInteractor *>(this->MultiViewer->GetRenderWindow()->GetInteractor());
	if (!rwiBrush) return 0;

	rwiBrush->GetPicker()->Pick(x,
		y,
		0,  // always zero.
		this->MultiViewer->GetRenderer());
	rwiBrush->GetPicker()->GetPickPosition(convertedPos);

	//Off image, return. Use imageIndex is better.
	if (convertedPos[0] == 0 && convertedPos[1] == 0 && convertedPos[2] == 0)
	{
		return 0;
	}

	return 1;
}

//----------------------------------------------------------------------------
void vtkMultiViewInteractorStyleImagePaint::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  /*os << indent << "Moving: " << this->Moving << endl;
  os << indent << "DrawPolygonPixels: " << this->DrawPolygonPixels << endl;
  os << indent << "StartPosition: " << this->StartPosition[0] << "," << this->StartPosition[1] << endl;
  os << indent << "EndPosition: " << this->EndPosition[0] << "," << this->EndPosition[1] << endl;*/
}
