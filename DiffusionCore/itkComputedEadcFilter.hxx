#ifndef _itkComputedEadcFilter_hxx_
#define _itkComputedEadcFilter_hxx_

#include "itkComputedEadcFilter.h"
#include "itkComputedAdcFilter.h"

namespace itk
{
	template< typename TInputPixelType, typename TOutputPixelType >
	ComputedEadcFilter< TInputPixelType, TOutputPixelType >
	::ComputedEadcFilter()
	{
		m_EadcBValue = 0;
		m_NumOfDiffDirections = 1;
		m_NthOutputDirection = 0;
		this->SetNumberOfThreads(1);
	}

	template< typename TInputPixelType, typename TOutputPixelType >
	void
	ComputedEadcFilter< TInputPixelType, TOutputPixelType >
	::BeforeThreadedGenerateData()
	{

	}

	template< typename TInputPixelType, typename TOutputPixelType >
	void
	ComputedEadcFilter< TInputPixelType, TOutputPixelType >
	::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
	{
		const InputImageType *inputImage = const_cast< InputImageType * >(this->GetInput());
		OutputImageType * outputImage = static_cast< OutputImageType * >(this->GetOutput());

		typedef itk::ComputedAdcFilter <TInputPixelType, TOutputPixelType> ComputedAdcFilterType;
		ComputedAdcFilterType::Pointer adcMap = ComputedAdcFilterType::New();
		adcMap->SetInput(inputImage);
		adcMap->SetBValueList(m_BValueList);
		adcMap->SetNumOfDiffDirections(m_NumOfDiffDirections);
		adcMap->SetNthOutputDirection(m_NthOutputDirection);
		adcMap->Update();

		ConstIteratorType inputIt(adcMap->GetOutput(), outputRegionForThread);
		OutputIteratorType outputIt(outputImage, outputRegionForThread);

		TOutputPixelType outputResult = 0;

		inputIt.GoToBegin();
		outputIt.GoToBegin();
		while (!inputIt.IsAtEnd())
		{
			// skip zero value
			outputResult = inputIt.Get() == 0 ? 0: exp(-inputIt.Get() * m_EadcBValue);
			outputIt.Set(outputResult);
			++inputIt;
			++outputIt;
		}
	}

	template< typename TInputPixelType, typename TOutputPixelType >
	void
	ComputedEadcFilter< TInputPixelType, TOutputPixelType >
	::PrintSelf(std::ostream& os, Indent indent) const
	{
		Superclass::PrintSelf(os, indent);
	}
} // end namespace itk
#endif
