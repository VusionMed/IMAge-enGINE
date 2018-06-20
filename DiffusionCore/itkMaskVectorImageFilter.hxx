#ifndef _itkMaskVectorImageFilter_hxx_
#define _itkMaskVectorImageFilter_hxx_

#include "itkMaskVectorImageFilter.h"
#include "itkMinimumMaximumImageCalculator.h"

namespace itk
{
	template< typename TInputPixelType >
	MaskVectorImageFilter< TInputPixelType >
		::MaskVectorImageFilter()
	{
		m_MaskThreshold = 0;
		m_InputImage = ITK_NULLPTR;
		m_OutputImage = ITK_NULLPTR;
	}

	template< typename TInputPixelType >
	void
		MaskVectorImageFilter< TInputPixelType >
		::GenerateData()
	{
		m_InputImage = this->GetInput();
		m_OutputImage = this->GetOutput();

		//Use the first vector image element as Source image for mask
		VectorImageToImageType::Pointer vectorImageToImageFilter = VectorImageToImageType::New();
		vectorImageToImageFilter->SetIndex(0);
		vectorImageToImageFilter->SetInput(m_InputImage);
		vectorImageToImageFilter->Update();

		//Get max input image
		typedef MinimumMaximumImageCalculator< MaskImageType > CalculatorType;
		typename CalculatorType::Pointer calculator = CalculatorType::New();

		calculator->SetImage(vectorImageToImageFilter->GetOutput());
		calculator->Compute();

		//Get mask
		ConnectedFilterType::Pointer connectedThreshold = ConnectedFilterType::New();
		connectedThreshold->SetLower(0);
		connectedThreshold->SetUpper(0.01 * m_MaskThreshold * calculator->GetMaximum());
		connectedThreshold->SetReplaceValue(255);//Any value beyond 0 would be Okay.

		// initial seed: need to be user defined?
		InputImageType::IndexType seed;
		seed[0] = 0;
		seed[1] = 0;
		seed[2] = 0;
		connectedThreshold->SetSeed(seed);
		connectedThreshold->SetInput(vectorImageToImageFilter->GetOutput());
		connectedThreshold->Update();

		m_OutputImage->SetSpacing(m_InputImage->GetSpacing());
		m_OutputImage->SetOrigin(m_InputImage->GetOrigin());
		m_OutputImage->SetDirection(m_InputImage->GetDirection());
		m_OutputImage->SetRegions(m_InputImage->GetLargestPossibleRegion());
		m_OutputImage->Allocate();

		ConstInputIteratorType inputIt(m_InputImage, m_InputImage->GetLargestPossibleRegion());
		OutputIteratorType outputIt(m_OutputImage, m_InputImage->GetLargestPossibleRegion());
		MaskIteratorType maskIt(connectedThreshold->GetOutput(), m_InputImage->GetLargestPossibleRegion());

		VariableLengthVectorType zerosVector;
		zerosVector.SetSize(m_InputImage->GetVectorLength());
		zerosVector.Fill(0);

		inputIt.GoToBegin();
		outputIt.GoToBegin();
		maskIt.GoToBegin();
		while (!inputIt.IsAtEnd())
		{
			if (maskIt.Get() != 0)
			{
				outputIt.Set(zerosVector);
			}
			else
			{
				outputIt.Set(inputIt.Get());
			}

			++outputIt;
			++inputIt;
			++maskIt;
		}
	}

	template< typename TInputPixelType >
	void
		MaskVectorImageFilter< TInputPixelType >
		::PrintSelf(std::ostream& os, Indent indent) const
	{
		Superclass::PrintSelf(os, indent);
		//os << indent << "NumOfComponents: " << m_NumOfComponents << std::endl;
	}

} // end namespace itk

#endif