#ifndef _itkComputedAdcFilter_hxx_
#define _itkComputedAdcFilter_hxx_

#include "itkComputedAdcFilter.h"
#include "itkAdcMapFilter.h"

namespace itk
{
	template< typename TInputPixelType, typename TOutputPixelType >
	ComputedAdcFilter< TInputPixelType, TOutputPixelType >
	::ComputedAdcFilter()
	{
		m_NumOfDiffDirections = 1;
		m_NthOutputDirection = 0;
		this->SetNumberOfThreads(1);
	}

	template< typename TInputPixelType, typename TOutputPixelType >
	void
	ComputedAdcFilter< TInputPixelType, TOutputPixelType >
	::BeforeThreadedGenerateData()
	{		
		
	}

	template< typename TInputPixelType, typename TOutputPixelType >
	void
	ComputedAdcFilter< TInputPixelType, TOutputPixelType >
	::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
	{
		const InputImageType *inputImage = const_cast< InputImageType * >(this->GetInput());
		OutputImageType * outputImage = static_cast< OutputImageType * >(this->GetOutput());

		typedef itk::AdcMapFilter <TInputPixelType, TInputPixelType> AdcMapFilterType;
		AdcMapFilterType::Pointer adcMap = AdcMapFilterType::New();
		adcMap->SetInput(inputImage);
		adcMap->SetBValueList(m_BValueList);
		adcMap->SetNumOfDiffDirections(m_NumOfDiffDirections);
		adcMap->SetNumberOfThreads(1);
		adcMap->Update();
		
		ConstInputIteratorType inputIt(adcMap->GetOutput(), outputRegionForThread);
		OutputIteratorType outputIt(outputImage, outputRegionForThread);

		inputIt.GoToBegin();
		outputIt.GoToBegin();

		if (m_NumOfDiffDirections == 1)
		{
			while (!inputIt.IsAtEnd())
			{
				outputIt.Set(inputIt.Get()[1]);
				++inputIt;
				++outputIt;
			}
		}

		//For multi direction data
		else
		{
			while (!inputIt.IsAtEnd())
			{
				TOutputPixelType outputResult = 0;
				if (this->m_NthOutputDirection)
				{
					this->m_NthOutputDirection = this->m_NthOutputDirection > this->m_NumOfDiffDirections ? this->m_NumOfDiffDirections : this->m_NthOutputDirection;
					outputResult = inputIt.Get()[2 * (this->m_NthOutputDirection - 1) + 1];
				}
				else //Average Adc
				{
					for (int direction = 0; direction < m_NumOfDiffDirections; direction++)
					{
						outputResult += inputIt.Get()[2 * direction + 1];
					}
					outputResult = outputResult / m_NumOfDiffDirections;
				}

				outputIt.Set(outputResult);
				outputResult = 0;
				++inputIt;
				++outputIt;
			}
		}
	}

	template< typename TInputPixelType, typename TOutputPixelType >
	void
	ComputedAdcFilter< TInputPixelType, TOutputPixelType >
	::PrintSelf(std::ostream& os, Indent indent) const
	{
		Superclass::PrintSelf(os, indent);
	}
} // end namespace itk
#endif
