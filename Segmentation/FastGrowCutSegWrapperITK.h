#ifndef _FASTGROWCUTWrapperITK_H
#define _FASTGROWCUTWrapperITK_H

#include "FastGrowCutSegmenter.h"

namespace FGC {
	template<typename SrcPixelType, typename LabPixelType>
	class  FastGrowCutWrapperITK
	{
	public:
		typedef itk::Image<SrcPixelType, 3>						 SrcImageType;
		typedef itk::Image<LabPixelType, 3>						 LabImageType;

		FastGrowCutWrapperITK();
		~FastGrowCutWrapperITK();
		//set parameters of grow cut
		//vtkSetObjectMacro(SourceVol, vtkImageData);
		//vtkSetObjectMacro(SeedVol, vtkImageData);
		//vtkSetObjectMacro(OutputVol, vtkImageData);

		//vtkSetMacro(InitializationFlag, bool);

		void SetSourceVol(SrcImageType *sourceVol);

		void SetSeedVol(LabImageType *seedVol);

		LabImageType* GetSeedVol();

		//processing functions
		void Initialization();
		void RunFGC();

	private:
		//vtk image data (from slicer)
		SrcImageType* m_SourceVol;
		LabImageType* m_SeedVol;

		std::vector<LabPixelType> m_imSeedVec;
		std::vector<LabPixelType> m_imLabVec;
		std::vector<SrcPixelType> m_imSrcVec;
		std::vector<long> m_imROI;

		//logic code
		FastGrowCut<SrcPixelType, LabPixelType> *m_fastGC;

		//state variables
		bool InitializationFlag;
	};
}//end of FGC namespace

#include "FastGrowCutSegWrapperITK.cxx"

#endif
