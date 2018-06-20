
/*=========================================================================

Program:   Vusion ImageEngine
Module:    DicomHelper

=========================================================================*/
// .NAME DicomHelper 
// .SECTION Description: Providing methods to parsing the diffusion and dynamic information from dicom data. 
//


#ifndef DicomHelper_h
#define DicomHelper_h

#include <vtkSmartPointer.h>
#include <vtkDICOMReader.h>
#include <vtkDICOMMetaData.h>
#include <vnl/vnl_matrix.h>
#include <itkImage.h>
#include <itkVectorImage.h>
#include <itkVector.h>
#include <itkVectorContainer.h>
#include <itkByteSwapper.h>
#include <QWidget>
#include <vtkMatrix4x4.h>
#include <iostream>
#include <vector>

#define PI 3.141592653589793
#define RAD  (PI/180.0)

typedef unsigned short SourceImagePixelType;
typedef itk::Image < SourceImagePixelType, 3> SourceImageType;
typedef float DiffusionCalculatorPixelType;
typedef itk::Image <DiffusionCalculatorPixelType, 3> DiffusionCalculatorImageType;
typedef itk::VectorImage <float, 3> DiffusionCalculatorVectorImageType;

class DicomHelper : public QWidget
{

	Q_OBJECT


private:

	vtkDICOMTag AcqTime = vtkDICOMTag(0x0008, 0x0032);
	vtkDICOMTag SeriesTime = vtkDICOMTag(0x0008, 0x0031);
	vtkDICOMTag ContentTime = vtkDICOMTag(0x0008, 0x0033);
	vtkDICOMTag TriggerTime = vtkDICOMTag(0x0018, 0x1060);
	QList<vtkDICOMTag> frametimeTags;

	private slots:

public:
	DicomHelper(vtkStringArray* Files, QWidget* parent = 0);
	~DicomHelper() {};

	vtkSmartPointer <vtkImageData> imageData;//
	vtkSmartPointer <vtkImageData> sourceImageSlice;//

	DiffusionCalculatorVectorImageType::Pointer  processedImageData;//
	DiffusionCalculatorImageType::Pointer skullMask;

	float GetEchoTime()
	{
		return TE;
	}
	float GetFlipAngle()
	{
		return FA;
	}

	int numberOfComponents;
	int imageDimensions[3];

	float scaleSlopeSlice;
	float scaleInterceptSlice;
	float sliceGap;

	vnl_matrix<double> slice2PatMatrix;

	//Values: DIFFUSION, DCE, DSC
	std::string imageDataType;
	char imageOrientation;

	int numberOfGradDirection;
	int numberOfBValue;

	std::vector <float> BvalueList;
	std::vector <float> directionLabel;

	vnl_matrix<double> dki_MatrixA;

	enum diffusionType
	{
		DWI,
		IVIM,
		DTI,
		DKI,
		sDKI
	};

	diffusionType imageCalculationType;

	typedef itk::Vector<double, 3> GradientDirectionType;
	typedef itk::VectorContainer< unsigned int, GradientDirectionType >
		GradientDirectionContainerType;
	GradientDirectionContainerType::Pointer gradientDirection;

	std::vector <float> dynamicTime;

	vtkSmartPointer<vtkDICOMReader> GetDicomReader()
	{
		return DicomReader;
	}

	void ComputeITKVolumeImage();
	void ComputeCurrentSourceImage(int currentSlice);
	void ComputeITKSourceImage(vtkSmartPointer<vtkImageData> input);
	void PreProcessing(bool registration);

	vtkSmartPointer<vtkDICOMMetaData> GetMetaDataFromNthSeries(int seriesID);

	vtkMatrix4x4* Get4x4Matrix()
	{
		return m_MatrixPat;
	}

	void GetIJKToRASMatrix(vtkMatrix4x4* mat);

signals:

	private slots:

protected:
	vtkSmartPointer < vtkStringArray> m_InputFileNames; 
	virtual void SortMetaData(vtkDICOMMetaData * source); 
	
	std::vector<int> IsoImageLabel; // -1 means no isotropic image

private:

	//Configuration Related Parameters
	int OTSU_OPT;
	int IVIM_MIN_NUM_B;
	int DTI_MIN_NUM_GRAD_DIR;
	int DKI_MIN_NUM_GRAD_DIR;
	int DKI_MIN_NUM_B;

	vtkSmartPointer<vtkDICOMReader> DicomReader;
	vtkStringArray *FileNamesForDiffusionSeries;

	bool isEnhanced;
	float Tm2ms(QString tm);
	char* manuFacturer;
	std::vector<float> scaleSlopeList;
	bool sortingNeed;

	float scaleIntercept;
	float TE;
	float FA;

	vtkMatrix4x4* m_MatrixPat;

	void GetSliceToPatMatrix(vtkDICOMMetaData* metaData);

	void DicomTagForGE(vtkDICOMMetaData* metaData);
	void DicomTagForVusion(vtkDICOMMetaData* metaData);
	void DicomTagForPhilips(vtkDICOMMetaData* metaData);
	void DicomTagForSiemens(vtkDICOMMetaData* metaData);
	void DicomTagForOthers(vtkDICOMMetaData* metaData);

	void DicomInfo(vtkDICOMMetaData* metaData);

	void CorrectPHLGradientDirection(vtkDICOMMetaData* metaData);
	bool PerfusionInfo(vtkDICOMMetaData* metaData, vtkIntArray* fileIndexArray, vtkIntArray* frameIndexArray);
	void _GenerateImageData(vtkImageData* sourceData, std::vector<float> *sortBasedVector, bool isSorting, int avgNumber, GradientDirectionContainerType::Pointer directionContainer);
	void DiffusionPrepare(GradientDirectionContainerType::Pointer gradientDirs);
	void SetPerfusionCategory();


	//
	//FOLLOWING CODE is from 3D SLICER www.slicer.org
	//
	template <typename T>
	T CSAExtractFromString(const char *ptr)
	{
		T rval = *(reinterpret_cast<const T *>(ptr));
		itk::ByteSwapper<T>::SwapFromSystemToLittleEndian(&rval);
		return rval;
	}

	class CSAItem : public std::vector<std::string >
	{
	public:
		typedef std::vector<std::string> SuperClass;

		itk::uint32_t vm;
		std::string vr;

		CSAItem(unsigned int length) : SuperClass(length),
			vm(0)
		{
		}
		CSAItem() : SuperClass()
		{
		}
		CSAItem(const CSAItem & other) : SuperClass(other.size())
		{
			*this = other;
		}
		CSAItem & operator=(const CSAItem &other)
		{
			this->resize(0);
			for (CSAItem::const_iterator it = other.begin();
				it != other.end();
				++it)
			{
				this->push_back(*it);
			}
			this->vm = other.vm;
			this->vr = other.vr;
			return *this;
		}

		template <typename T>
		std::vector<T> AsVector() const
		{
			std::vector<T> rval;
			for (unsigned i = 0; i < this->size(); ++i)
			{
				if (!(*this)[i].empty())
				{
					T val = 0;
					std::stringstream convert((*this)[i]);
					convert >> val;
					rval.push_back(val);
				}
			}
			return rval;
		}
		void DebugPrint() const
		{
			std::cerr << "  VM = " << this->vm << " VR = " << this->vr << std::endl
				<< "    ";
			bool firstTime(false);
			for (CSAItem::const_iterator it = this->begin();
				it != this->end(); ++it)
			{
				if (firstTime)
				{
					firstTime = false;
				}
				else
				{
					std::cerr << " ";
				}
				std::cerr << *it;
			}
			std::cerr << std::endl;
		}
	};

	class CSAHeader : public std::map<std::string, CSAItem>
	{
	public:
		void DebugPrint() const
		{
			for (CSAHeader::const_iterator it = this->begin();
				it != this->end(); ++it)
			{
				std::cerr << it->first << std::endl;
				it->second.DebugPrint();
			}
		}
	};

	//void DecodeCSAHeader(CSAHeader &header, const std::string &infoString);
	void DecodeCSAHeader(CSAHeader &header, const char* info);

	bool ExtractSiemensBMatrix(CSAHeader *csaHeader, unsigned int strideVolume,
		vnl_matrix_fixed<double, 3, 3> &bMatrix);

	void SiemensDeMosaic();

};
#endif