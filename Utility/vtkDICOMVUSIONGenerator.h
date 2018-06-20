#ifndef vtkDICOMVUSIONGenerator_h
#define vtkDICOMVUSIONGenerator_h

#include "vtkDICOMModule.h" // For export macro
#include "vtkDICOMMRGenerator.h"

#include <vector>
#include <array>

class vtkImageData;
//! Generate DICOM data objects for MR images.
/*!
*  Generate a DICOM data set belonging to one of the MR SOP classes.
*  The assumption is that images have been read into VTK as MR, and
*  are being written out as derived images after being processed.
*  The specific IOD classes supported are as follows:
*  - MR Image, 1.2.840.10008.5.1.4.1.1.4
*/
//class VTKDICOM_EXPORT vtkDICOMVUSIONGenerator : public vtkDICOMMRGenerator
class vtkDICOMVUSIONGenerator : public vtkDICOMMRGenerator
{
public:
        //! Static method for construction.
        static vtkDICOMVUSIONGenerator *New();
        vtkTypeMacro(vtkDICOMVUSIONGenerator, vtkDICOMMRGenerator);

        //! Print information about this object.
        virtual void PrintSelf(ostream& os, vtkIndent indent);

        //! Generate an instance of one of the supported classes.
        /*!
        *  This is the primary interface method of this class.  Given the
        *  information for a vtkImageData object, it will populate the
        *  attributes of the supplied vtkDICOMMetaData object.
        */
		virtual bool GenerateInstance(vtkInformation *info); 

		virtual void SetUseSourcePrivateTagOn() {
			m_UseSourceInstansePrivateTag = true;
		}

		virtual void SetUseSourcePrivateTagOff() {
			m_UseSourceInstansePrivateTag = false;
		}

		virtual void SetAnonymizingDicomOn() {
			m_AnonymizingDicom = true;
			this->Modified();
		}

		virtual void SetAnonymizingDicomOff() {
			m_AnonymizingDicom = false;
			this->Modified();
		}

		virtual void SetDiffusionBValueTag(vtkImageData * imageData, const std::vector<float> & diffusionBValueList);

		virtual void SetDiffusionGradientOrientationTag(vtkImageData * imageData, const std::vector<std::array<float, 3>> & gradientDirection);

protected:
        vtkDICOMVUSIONGenerator();
        ~vtkDICOMVUSIONGenerator();

        //! Generate the Series Module.
        //virtual bool GenerateMRSeriesModule(vtkDICOMMetaData *source);

        //! Generate the Image Module.
        //virtual bool GenerateMRImageModule(vtkDICOMMetaData *source);

        //! Instantiate a DICOM Secondary Capture Image object.
        //virtual bool GenerateMRInstance(vtkInformation *info);

        //! Generate VUSION Module.
		virtual bool GenerateVUSIONModule(vtkDICOMMetaData *source);

		virtual void CopySourceInstanceTags(vtkDICOMMetaData *source);

		virtual void AnonymizingDicomTag();

		virtual void WriteDiffusionBValueTag();

		virtual void WriteDiffusionGradientOrientationTag();

private:
        vtkDICOMVUSIONGenerator(const vtkDICOMVUSIONGenerator&);  // Not implemented.
        void operator=(const vtkDICOMVUSIONGenerator&);  // Not implemented.

		bool m_UseSourceInstansePrivateTag;
		bool m_AnonymizingDicom;
		vtkImageData * m_SoureImageData;
		std::vector<float> m_DiffusionBValue;
		std::vector<std::array<float, 3>> m_GradientDirection;
};

#endif // vtkDICOMVUISONGenerator_h
