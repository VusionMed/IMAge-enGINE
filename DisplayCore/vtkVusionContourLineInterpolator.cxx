/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVusionContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVusionContourLineInterpolator.h"

#include "vtkVusionContourRepresentation.h"
#include "vtkIntArray.h"


//----------------------------------------------------------------------
vtkVusionContourLineInterpolator::vtkVusionContourLineInterpolator()
{
}

//----------------------------------------------------------------------
vtkVusionContourLineInterpolator::~vtkVusionContourLineInterpolator()
{
}

//----------------------------------------------------------------------
int vtkVusionContourLineInterpolator::UpdateNode( vtkRenderer *,
                                            vtkVusionContourRepresentation *,
                 double * vtkNotUsed(node), int vtkNotUsed(idx) )
{
  return 0;
}

//----------------------------------------------------------------------
void vtkVusionContourLineInterpolator::GetSpan( int nodeIndex,
                                          vtkIntArray *nodeIndices,
                                          vtkVusionContourRepresentation *rep)
{
  int start = nodeIndex - 1;
  int end   = nodeIndex;
  int index[2];

  // Clear the array
  nodeIndices->Reset();
  nodeIndices->Squeeze();
  nodeIndices->SetNumberOfComponents(2);

  for ( int i = 0; i < 3; i++ )
  {
    index[0] = start++;
    index[1] = end++;

    if ( rep->GetClosedLoop() )
    {
      if ( index[0] < 0 )
      {
        index[0] += rep->GetNumberOfNodes();
      }
      if ( index[1] < 0 )
      {
        index[1] += rep->GetNumberOfNodes();
      }
      if ( index[0] >= rep->GetNumberOfNodes() )
      {
        index[0] -= rep->GetNumberOfNodes();
      }
      if ( index[1] >= rep->GetNumberOfNodes() )
      {
        index[1] -= rep->GetNumberOfNodes();
      }
    }

    if ( index[0] >= 0 && index[0] < rep->GetNumberOfNodes() &&
         index[1] >= 0 && index[1] < rep->GetNumberOfNodes() )
    {
      nodeIndices->InsertNextTypedTuple( index );
    }
  }
}

//----------------------------------------------------------------------
void vtkVusionContourLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
