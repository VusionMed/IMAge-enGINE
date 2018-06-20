/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVusionContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVusionContourLineInterpolator
 * @brief   Defines API for interpolating/modifying nodes from a vtkVusionContourRepresentation
 *
 * vtkVusionContourLineInterpolator is an abstract base class for interpolators
 * that are used by the vtkVusionContourRepresentation class to interpolate
 * and/or modify nodes in a contour. Subclasses must override the virtual
 * method \c InterpolateLine. This is used by the contour representation
 * to give the interpolator a chance to define an interpolation scheme
 * between nodes. See vtkBezierContourLineInterpolator for a concrete
 * implementation. Subclasses may also override \c UpdateNode. This provides
 * a way for the representation to give the interpolator a chance to modify
 * the nodes, as the user constructs the contours. For instance, a sticky
 * contour widget may be implemented that moves nodes to nearby regions of
 * high gradient, to be used in contour-guided segmentation.
*/

#ifndef vtkVusionContourLineInterpolator_h
#define vtkVusionContourLineInterpolator_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderer;
class vtkVusionContourRepresentation;
class vtkIntArray;

class VTKINTERACTIONWIDGETS_EXPORT vtkVusionContourLineInterpolator : public vtkObject
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkVusionContourLineInterpolator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance vtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  virtual int InterpolateLine( vtkRenderer *ren,
                               vtkVusionContourRepresentation *rep,
                               int idx1, int idx2 ) = 0;

  /**
   * The interpolator is given a chance to update the node. For instance, the
   * vtkImageContourLineInterpolator updates the idx'th node in the contour,
   * so it automatically sticks to edges in the vicinity as the user
   * constructs the contour.
   * Returns 0 if the node (world position) is unchanged.
   */
  virtual int UpdateNode( vtkRenderer *,
                          vtkVusionContourRepresentation *,
                          double * vtkNotUsed(node), int vtkNotUsed(idx) );

  /**
   * Span of the interpolator. ie. the number of control points its supposed
   * to interpolate given a node.

   * The first argument is the current nodeIndex.
   * ie, you'd be trying to interpolate between nodes "nodeIndex" and
   * "nodeIndex-1", unless you're closing the contour in which case, you're
   * trying to interpolate "nodeIndex" and "Node=0".

   * The node span is returned in a vtkIntArray. The default node span is 1
   * (ie. nodeIndices is a 2 tuple (nodeIndex, nodeIndex-1)). However, it
   * need not always be 1. For instance, cubic spline interpolators, which
   * have a span of 3 control points, it can be larger. See
   * vtkBezierContourLineInterpolator for instance.
   */
  virtual void GetSpan( int nodeIndex, vtkIntArray *nodeIndices,
                        vtkVusionContourRepresentation *rep );

 protected:
  vtkVusionContourLineInterpolator();
  ~vtkVusionContourLineInterpolator();

private:
  vtkVusionContourLineInterpolator(const vtkVusionContourLineInterpolator&);
  void operator=(const vtkVusionContourLineInterpolator&);
};

#endif
