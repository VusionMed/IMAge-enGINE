#ifndef utilities_hxx_
#define utilities_hxx_

// std
#include <iostream>
#include <fstream>
#include <limits>

// itk
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImportImageFilter.h"
//#include "itkVTKImageToImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkRegionOfInterestImageFilter.h"

#include <csignal>

namespace FGC
{
	template<typename itkImageType>
	typename itkImageType::Pointer createImage(typename itkImageType::SizeType size, int iniValue) {

		typename itkImageType::Pointer img = itkImageType::New();
		typename itkImageType::SpacingType spacing;
		typename itkImageType::IndexType start;
		start.Fill(0);
		spacing.Fill(1);
		spacing[itkImageType::ImageDimension - 1] = 2;

		typename itkImageType::RegionType region;
		img->SetSpacing(spacing);
		region.SetSize(size);
		region.SetIndex(start);
		img->SetRegions(region);
		img->Allocate();
		img->FillBuffer(static_cast<typename itkImageType::PixelType>(iniValue));

		const typename itkImageType::SpacingType& sp = img->GetSpacing();
		std::cout << "Spacing = ";
		std::cout << sp[0] << ", " << sp[1] << std::endl;


		return img;
	}
	
template<typename PixelType>
void FindITKImageROI(typename itk::Image<PixelType,3>::Pointer im, std::vector<long>& imROI) {

	typedef itk::Image<PixelType, 3> ITKImageType;

    typename ITKImageType::IndexType roiStart;
    typename ITKImageType::IndexType roiEnd;
    typename ITKImageType::IndexType start;
    typename ITKImageType::SizeType size;

    size = im->GetLargestPossibleRegion().GetSize();
    start = im->GetLargestPossibleRegion().GetIndex();
	std::cout << "image start index = " << start << std::endl;
	std::cout << "image start size = " << size << std::endl;

    roiStart[0] = 0; roiStart[1] = 0; roiStart[2] = 0;
    roiEnd[0] = 0; roiEnd[1] = 0; roiEnd[2] = 0;

    unsigned int ndims = im->GetImageDimension();

    bool foundLabel = false;
    itk::ImageRegionIteratorWithIndex< ITKImageType > label(im, im->GetBufferedRegion() );
    for(label.GoToBegin(); !label.IsAtEnd(); ++label) {
        if(label.Get() != 0) {
            typename ITKImageType::IndexType idx = label.GetIndex();
            for (unsigned i = 0; i < ndims; i++)  {
              if(!foundLabel)  {
                roiStart[i] = idx[i];
                roiEnd[i] = idx[i];
                }
              else
                {
                if(idx[i] <= roiStart[i])
                  {
                  roiStart[i] = idx[i];
                  }
                if(idx[i] >= roiEnd[i])
                  {
                  roiEnd[i] = idx[i];
                  }
                }
              }
          foundLabel = true;
          }
    }

    int radius = 17;
    for (unsigned i = 0; i < ndims; i++) {
      int diff = static_cast< int > (roiStart[i] - radius);
      if (diff >= start[i])  {
        roiStart[i] -= radius;
        }
      else  {
        roiStart[i] = start[i];
        }
      roiEnd[i] = (static_cast<unsigned int>(roiEnd[i] + radius) < size[i]) ? (roiEnd[i] + radius) : size[i]-1;

      }

    // copy ROI to vector
    imROI.resize(6);
    for(unsigned i = 0; i < 3; i++) {
        imROI[i] = roiStart[i];
        imROI[i + 3] = roiEnd[i];
    }
}

template<typename PixelType>
void ExtractITKImageROI(typename itk::Image<PixelType, 3>::Pointer  im, const std::vector<long>& imROI, \
                        std::vector<PixelType>& imROIVec) {

    // Copy itk image ROI to vector
    typedef itk::Image<PixelType, 3> ImageType;
    typename ImageType::IndexType index;
    long i,j,k,kk,DIMXYZ;

    DIMXYZ = (imROI[3] - imROI[0])*(imROI[4] - imROI[1])*(imROI[5] - imROI[2]);
    imROIVec.clear();
    imROIVec.resize(DIMXYZ);
    kk = 0;
    for(k = imROI[2]; k < imROI[5]; k++)
        for(j = imROI[1]; j < imROI[4]; j++)
            for(i = imROI[0]; i < imROI[3]; i++)  {
                index[0] = i; index[1] = j; index[2] = k;
                imROIVec[kk++] = im->GetPixel(index);
            }
}

template<typename PixelType>
void UpdateITKImageROI(const std::vector<PixelType>& imROIVec, const std::vector<long>& imROI,  \
                       typename itk::Image<PixelType, 3>::Pointer im) {

    typedef itk::Image<PixelType, 3> ImageType;
    typename ImageType::IndexType index;
    long i,j,k,kk;

    // Set non-ROI as zeros
    im->FillBuffer(0);
    kk = 0;
    for(k = imROI[2]; k < imROI[5]; k++)
        for(j = imROI[1]; j < imROI[4]; j++)
            for(i = imROI[0]; i < imROI[3]; i++)  {
            index[0] = i; index[1] = j; index[2] = k;
            im->SetPixel(index, imROIVec[kk++]);
        }
}

}// end of namespace

#endif
