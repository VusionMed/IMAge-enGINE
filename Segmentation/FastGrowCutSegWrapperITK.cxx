#ifndef _FASTGROWCUTWrapperITK_CXX
#define _FASTGROWCUTWrapperITK_CXX


#include <iostream>
#include "FastGrowCutSegWrapperITK.h"

//#include "vtkObjectFactory.h"
//#include "vtkSmartPointer.h"

#include "itkImage.h"
#include "itkTimeProbe.h"

//vtkCxxRevisionMacro(vtkFastGrowCutSeg, "$Revision$"); //necessary?
//vtkStandardNewMacro(vtkFastGrowCutSeg); //for the new() macro

//----------------------------------------------------------------------------
namespace FGC {
	template<typename SrcPixelType, typename LabPixelType>
	FastGrowCutWrapperITK <SrcPixelType, LabPixelType>
	::FastGrowCutWrapperITK() {
		m_SourceVol = NULL;
		m_SeedVol = NULL;
		m_fastGC = NULL;
	}

	template<typename SrcPixelType, typename LabPixelType>
	FastGrowCutWrapperITK <SrcPixelType, LabPixelType>
	::~FastGrowCutWrapperITK() 
	{
		//these functions decrement reference count on the vtkImageData's (incremented by the SetMacros)
		if (this->m_SourceVol)
		{
			
			this->SetSourceVol(NULL);
		}

		if (this->m_SeedVol)
		{
			this->SetSeedVol(NULL);
		}

		if (m_fastGC != NULL) {
			delete m_fastGC;
		}
	}

	template<typename SrcPixelType, typename LabPixelType>
	void FastGrowCutWrapperITK <SrcPixelType, LabPixelType>
		::SetSourceVol(SrcImageType *sourceVol)
	{
		if (sourceVol == m_SourceVol) return;
		m_SourceVol = sourceVol;
	}

	template<typename SrcPixelType, typename LabPixelType>
	void FastGrowCutWrapperITK <SrcPixelType, LabPixelType>
		::SetSeedVol(LabImageType *seedVol)
	{
		if (seedVol == m_SeedVol) return;
		m_SeedVol = seedVol;
	}

	template<typename SrcPixelType, typename LabPixelType>
	itk::Image<LabPixelType, 3>	 * FastGrowCutWrapperITK <SrcPixelType, LabPixelType>
		::GetSeedVol()
	{
		return m_SeedVol;
	}

	template<typename SrcPixelType, typename LabPixelType>
	void FastGrowCutWrapperITK <SrcPixelType, LabPixelType>
	::Initialization() {
		InitializationFlag = false;
		if (m_fastGC == NULL) {
			m_fastGC = new FGC::FastGrowCut<SrcPixelType, LabPixelType>();
		}
	}


	template<typename SrcPixelType, typename LabPixelType>
	void FastGrowCutWrapperITK <SrcPixelType, LabPixelType>
		::RunFGC() {

		itk::TimeProbe timer;

		timer.Start();

		//QProgressBar* computationProgressBar =  new QProgressBar;
		//qSlicerApplication::application()->mainWindow()->statusBar()->addPermanentWidget(computationProgressBar);

		// Find ROI
		if (!InitializationFlag) {
			FGC::FindITKImageROI<LabPixelType>(m_SeedVol, m_imROI);
			std::cout << "image ROI = [" << m_imROI[0] << "," << m_imROI[1] << "," << m_imROI[2] << ";"  \
				<< m_imROI[3] << "," << m_imROI[4] << "," << m_imROI[5] << "]" << std::endl;
			FGC::ExtractITKImageROI<SrcPixelType>(m_SourceVol, m_imROI, m_imSrcVec);
		}

		FGC::ExtractITKImageROI<LabPixelType>(m_SeedVol, m_imROI, m_imSeedVec);

		// Initialize FastGrowCut
		std::vector<long> imSize(3);
		for (int i = 0; i < 3; i++) {
			imSize[i] = m_imROI[i + 3] - m_imROI[i];
		}

		m_fastGC->SetSourceImage(m_imSrcVec);
		m_fastGC->SetSeedlImage(m_imSeedVec);
		m_fastGC->SetImageSize(imSize);
		m_fastGC->SetWorkMode(InitializationFlag);

		//     computationProgressBar->setValue(10);

			// Do Segmentation
		m_fastGC->DoSegmentation();
		//m_fastGC->GetForegroundmage(m_imLabVec);
		m_fastGC->GetLabeImage(m_imLabVec);

		//    computationProgressBar->setValue(90);

			// Update result
		FGC::UpdateITKImageROI<LabPixelType>(m_imLabVec, m_imROI, m_SeedVol);

		//    computationProgressBar->setValue(100);
		   // delete computationProgressBar;
		timer.Stop();

		if (!InitializationFlag)
			std::cout << "Initial fast GrowCut segmentation time: " << timer.GetMeanTime() << " seconds\n";
		else
			std::cout << "Adaptive fast GrowCut segmentation time: " << timer.GetMeanTime() << " seconds\n";

	}
}

#endif