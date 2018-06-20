#ifndef vusionSharedFunctions_h_
#define vusionSharedFunctions_h_

#include "vtkImageData.h"

namespace vusion
{
	void PrintVTKImageDataInfo(vtkImageData *imageData)
	{
		std::cout << "-------------------------------" << std::endl;
		std::cout << "Printing vtk image data info: " << std::endl;
		std::cout << "-------------------------------" << std::endl;

		int components;
		int dims[3];
		double origins[3];
		double spacing[3];
		int extent[6];
		double range[2];

		components = imageData->GetNumberOfScalarComponents();
		std::cout << "image components: " << components << std::endl;

		imageData->GetDimensions(dims);
		std::cout << "image dims: " << dims[0] << "x" << dims[1] << "x" << dims[2] << std::endl;

		imageData->GetOrigin(origins);
		std::cout << "image origins: " << origins[0] << " " << origins[1] << " " << origins[2] << std::endl;

		imageData->GetSpacing(spacing);
		std::cout << "image spacing: " << spacing[0] << "x" << spacing[1] << "x" << spacing[2] << std::endl;

		imageData->GetExtent(extent);
		std::cout << "extent: " << extent[0] << "x" << extent[1] << "x" << extent[2] << "x" << extent[3] << "x" << extent[4] << "x" << extent[5] << endl;

		imageData->GetScalarRange(range);
		std::cout << "range: " << range[0] << " - " << range[1] << endl;
	}
}
#endif
