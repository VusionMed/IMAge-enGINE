/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiViewInteractorStyleImagePaint.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiViewInteractorStyleImagePaint - draw polygon during mouse move
// .SECTION Description
// This interactor style allows the user to draw a polygon in the render
// window using the left mouse button while mouse is moving.
// When the mouse button is released, a SelectionChangedEvent will be fired.

#ifndef vtkMultiViewInteractorStyleImagePaint_h
#define vtkMultiViewInteractorStyleImagePaint_h

#include "vtkInteractorStyleImage.h"
#include "vtkVusionMultiView.h"
#include <vector>      // For returning Polygon Points
#include "vtkVector.h" // For Polygon Points
#include "vtkVusionPainter.h"

class vtkUnsignedCharArray;
class vtkRegularPolygonSource;
class vtkActor;

class vtkMultiViewInteractorStyleImagePaint : public vtkInteractorStyleImage
{
public:
  static vtkMultiViewInteractorStyleImagePaint *New();
  vtkTypeMacro(vtkMultiViewInteractorStyleImagePaint, vtkInteractorStyleImage);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();

  // Description:
  // Whether to draw polygon in screen pixels. Default is ON
  //vtkSetMacro(DrawPolygonPixels, bool);
  //vtkGetMacro(DrawPolygonPixels, bool);
  //vtkBooleanMacro(DrawPolygonPixels, bool);

  //vtkSetMacro(RealTimePaintBrush, bool);
  //vtkGetMacro(RealTimePaintBrush, bool);
  //vtkBooleanMacro(RealTimePaintBrush, bool);

  //vtkSetObjectMacro(MultiViewer, vtkVusionMultiView);
  virtual void SetMultiViewer(vtkVusionMultiView *multiViewer);
  vtkGetObjectMacro(MultiViewer, vtkVusionMultiView);

  vtkGetObjectMacro(Painter, vtkVusionPainter<unsigned char>);
  // Description:
  // Get the current polygon points in display units
  std::vector<vtkVector2i> GetPolygonPoints();

  void setCircleVisability(bool);

protected:
  vtkMultiViewInteractorStyleImagePaint();
  ~vtkMultiViewInteractorStyleImagePaint();

  virtual void DrawPolygon();

  virtual void DrawCircles();

  virtual void PaintLastPoint();

  virtual void PaintPoints();

  virtual void OnMouseWheelForward();//Overwrite

  virtual void OnMouseWheelBackward();//Overwrite

  int ComputeMousePosition(int x, int y, double *convertedPos);// 0 = off image or failed, 1 = in image

  int StartPosition[2];
  int EndPosition[2];
  int Moving;

  bool DrawPolygonPixels;

  bool RealTimePaintBrush;

  vtkUnsignedCharArray *PixelArray;
  int m_NumberOfPixelComponents;

  vtkVusionMultiView * MultiViewer;
  vtkVusionPainter<unsigned char> *Painter;

  vtkRegularPolygonSource *CircleSource;
  vtkActor *CircleActor;

private:
  vtkMultiViewInteractorStyleImagePaint(const vtkMultiViewInteractorStyleImagePaint&);  // Not implemented
  void operator=(const vtkMultiViewInteractorStyleImagePaint&);  // Not implemented

  class vtkInternal;
  vtkInternal* Internal;

};

#endif
