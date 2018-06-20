#ifndef _itkComputedEadcFilter_h_
#define _itkComputedEadcFilter_h_

#include "itkImageToImageFilter.h"
#include "itkVectorImage.h"
#include "itkImage.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkSmartPointer.h"
#include <vector>

namespace itk
{
	template< typename TInputPixelType, typename TOutputPixelType >
	class ComputedEadcFilter :
		public ImageToImageFilter< VectorImage<TInputPixelType, 3>, Image<TOutputPixelType, 3> >
	{
	public:
		/** Standard class typedefs. */
		typedef ComputedEadcFilter                           Self;
		typedef ImageToImageFilter< VectorImage<TInputPixelType, 3>, Image<TOutputPixelType, 3> > Superclass;
		typedef SmartPointer< Self >                            Pointer;
		typedef SmartPointer< const Self >                      ConstPointer;

		/** Method for creation through the object factory. */
		itkNewMacro(Self);

		/** Typedef to describe the type of pixel. */
		typedef typename Superclass::InputImageType						 InputImageType;
		typedef typename Superclass::OutputImageType					 OutputImageType;

		typedef itk::ImageRegionConstIterator <OutputImageType> ConstIteratorType;
		typedef itk::ImageRegionIterator <OutputImageType> OutputIteratorType;

		/** Run-time type information (and related methods). */
		itkTypeMacro(ComputedEadcFilter, ImageToImageFilter);

		/** Set/Get the amount to Shift each Pixel. The shift is followed by a Scale.
		*/

		void SetBValueList(std::vector<TInputPixelType> bValueList) { m_BValueList = bValueList; }
		std::vector<TInputPixelType> GetBValueList() { return m_BValueList; }

		itkSetMacro(EadcBValue, unsigned int);
		itkGetConstMacro(EadcBValue, unsigned int);

		itkSetMacro(NthOutputDirection, unsigned int);
		itkGetConstMacro(NthOutputDirection, unsigned int);

		itkSetMacro(NumOfDiffDirections, unsigned int);
		itkGetConstMacro(NumOfDiffDirections, unsigned int);

		void SetOutputToAverageEADC() {
			this->m_NthOutputDirection = 0;
			this->Modified();
		}

	protected:
		ComputedEadcFilter();
		~ComputedEadcFilter() {}

		void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

		/** Initialize some accumulators before the threads run. */
		void BeforeThreadedGenerateData() ITK_OVERRIDE;

		/** Tally accumulated in threads. */
		//void AfterThreadedGenerateData() ITK_OVERRIDE;

		/** Multi-thread version GenerateData. */
		void  ThreadedGenerateData(const OutputImageRegionType &
			outputRegionForThread, ThreadIdType threadId) ITK_OVERRIDE;

		//virtual void GenerateOutputInformation() ITK_OVERRIDE;

	private:
		ComputedEadcFilter(const Self &); //ITK_DELETE_FUNCTION;
		void operator=(const Self &);//ITK_DELETE_FUNCTION;

		std::vector<TInputPixelType> m_BValueList;
		unsigned int m_EadcBValue;
		unsigned int m_NthOutputDirection;
		unsigned int m_NumOfDiffDirections;

		//const InputImageType * m_InputImage;
		//OutputImageType * m_OutputImage;
	};
}

#include "itkComputedEadcFilter.hxx"

#endif