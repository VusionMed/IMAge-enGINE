/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVusionImageMapToWindowLevelColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVusionImageMapToWindowLevelColors
 * @brief   map the input image through a lookup table and window / level it
 *
 * The vtkVusionImageMapToWindowLevelColors filter will take an input image of any
 * valid scalar type, and map the first component of the image through a
 * lookup table.  This resulting color will be modulated with value obtained
 * by a window / level operation. The result is an image of type
 * VTK_UNSIGNED_CHAR. If the lookup table is not set, or is set to NULL, then
 * the input data will be passed through if it is already of type
 * VTK_UNSIGNED_CHAR.
 *
 * @sa
 * vtkLookupTable vtkScalarsToColors
*/

#ifndef vtkVusionImageMapToWindowLevelColors_h
#define vtkVusionImageMapToWindowLevelColors_h


//#include "vtkImagingColorModule.h" // For export macro
#include "vtkImageMapToColors.h"

class  vtkVusionImageMapToWindowLevelColors : public vtkImageMapToColors
{
public:
  static vtkVusionImageMapToWindowLevelColors *New();
  vtkTypeMacro(vtkVusionImageMapToWindowLevelColors,vtkImageMapToColors);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set / Get the Window to use -> modulation will be performed on the
   * color based on (S - (L - W/2))/W where S is the scalar value, L is
   * the level and W is the window.
   */
  vtkSetMacro( Window, double );
  vtkGetMacro( Window, double );
  //@}

  //@{
  /**
   * Set / Get the Level to use -> modulation will be performed on the
   * color based on (S - (L - W/2))/W where S is the scalar value, L is
   * the level and W is the window.
   */
  vtkSetMacro( Level, double );
  vtkGetMacro( Level, double );
  //@}

protected:
  vtkVusionImageMapToWindowLevelColors();
  ~vtkVusionImageMapToWindowLevelColors();

  virtual int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id);
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  double Window;
  double Level;

private:
	vtkVusionImageMapToWindowLevelColors(const vtkVusionImageMapToWindowLevelColors&);// Not implemented. //VTK_DELETE_FUNCTION;
  void operator=(const vtkVusionImageMapToWindowLevelColors&);// Not implemented. //VTK_DELETE_FUNCTION;
};

#endif







