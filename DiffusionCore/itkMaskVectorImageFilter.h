#ifndef _itkMaskVectorImageFilter_h_
#define _itkMaskVectorImageFilter_h_

#include "itkImageToImageFilter.h"
#include "itkVectorImage.h"
#include "itkImage.h"
#include "itkVariableLengthVector.h"
#include "itkImageRegionIterator.h"
#include "itkSmartPointer.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkConnectedThresholdImageFilter.h"

namespace itk
{
	template< typename TInputPixelType >
	class MaskVectorImageFilter :
		public ImageToImageFilter< VectorImage<TInputPixelType, 3>, VectorImage<TInputPixelType, 3> >
	{
	public:
		/** Standard class typedefs. */
		typedef MaskVectorImageFilter                           Self;
		typedef ImageToImageFilter< VectorImage<TInputPixelType, 3>, VectorImage<TInputPixelType, 3> > Superclass;
		typedef SmartPointer< Self >                            Pointer;
		typedef SmartPointer< const Self >                      ConstPointer;

		/** Method for creation through the object factory. */
		itkNewMacro(Self);


		/** Typedef to describe the type of pixel. */
		typedef typename Superclass::InputImageType						 InputImageType;
		typedef typename Superclass::OutputImageType					 OutputImageType;
		typedef Image< TInputPixelType, 3 >								 MaskImageType;
		typedef VariableLengthVector <TInputPixelType>					 VariableLengthVectorType;

		typedef ImageRegionConstIterator <InputImageType>	ConstInputIteratorType;
		typedef ImageRegionIterator <OutputImageType>	OutputIteratorType;
		typedef ImageRegionConstIterator <MaskImageType>	MaskIteratorType;

		typedef VectorIndexSelectionCastImageFilter <InputImageType, MaskImageType> VectorImageToImageType;
		typedef ConnectedThresholdImageFilter<MaskImageType, MaskImageType> ConnectedFilterType;

		/** Run-time type information (and related methods). */
		itkTypeMacro(MaskVectorImageFilter, ImageToImageFilter);

		/** Set/Get the amount to Shift each Pixel. The shift is followed by a Scale.
		*/
		//itkSetMacro(DiffB0Image, MaskImageType);
		//itkGetConstMacro(DiffB0Image, MaskImageType);
		//void SetDiffB0Image(MaskImageType *DiffB0Image){ m_DiffB0Image = DiffB0Image; }

		itkSetMacro(MaskThreshold, TInputPixelType);
		itkGetConstMacro(MaskThreshold, TInputPixelType);

		//void SetBValueList(std::vector<TInputPixelType> bValueList){ m_BValueList = bValueList; }
		//std::vector<TInputPixelType> GetBValueList(){ return m_BValueList; }

	protected:
		MaskVectorImageFilter();
		~MaskVectorImageFilter(){}

		void PrintSelf(std::ostream & os, Indent indent) const ITK_OVERRIDE;

		/** Initialize some accumulators before the threads run. */
		//void BeforeThreadedGenerateData() ITK_OVERRIDE;

		/** Tally accumulated in threads. */
		//void AfterThreadedGenerateData() ITK_OVERRIDE;

		/** single-thread version GenerateData. */
		void GenerateData();

		//virtual void GenerateOutputInformation() ITK_OVERRIDE;

	private:
		MaskVectorImageFilter(const Self &); //ITK_DELETE_FUNCTION;
		void operator=(const Self &);//ITK_DELETE_FUNCTION;

		TInputPixelType m_MaskThreshold;
		const MaskImageType *m_DiffB0Image;
		const InputImageType * m_InputImage;
		OutputImageType * m_OutputImage;
	};
}

#include "itkMaskVectorImageFilter.hxx"

#endif