#include <DicomHelper.h>

#include <algorithm>
#include <math.h>

//VTK INCLUDE
#include <vtkDICOMValue.h>
#include <vtkMedicalImageProperties.h>
#include <vtkDICOMApplyRescale.h>
#include <vtkImageData.h>
#include <vtkStringArray.h>
#include <vtkIntArray.h>
#include <vtkExtractVOI.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageShiftScale.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageAppend.h>
#include <vtkImageWeightedSum.h>
#include <vtkImageBlend.h>

//ITK INCLUDE
#include <itkVTKImageToImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkShiftScaleImageFilter.h>
#include <itkComposeImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkVectorIndexSelectionCastImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkImageToVTKImageFilter.h>

#include <qdebug.h>
#include <QMessageBox>

#include "bvalueDialog.h"

#include "itkMaskImageFilter.h"

#include "DKIHelper.h"
#include "vtkDICOMFileSorter.h" //????:??series,??????series?MetaData

bool cmp(std::pair<float, int> p1, std::pair<float, int> p2)
{
	if (p1.first < p2.first) return 1;
	return 0;

}

//----------------------------------------------------------------------------------------------
// \initialize DicomHelper 
// \Input: filename list
//----------------------------------------------------------------------------------------------
DicomHelper::DicomHelper(vtkStringArray* Files, QWidget* parent):QWidget(parent)
{
	QSettings setting("MR_Config.ini", QSettings::IniFormat);

	OTSU_OPT = setting.value("MASK_OPT").toInt();
	IVIM_MIN_NUM_B = setting.value("IVIM_NUM_B").toInt();
	DTI_MIN_NUM_GRAD_DIR = setting.value("DTI_NUM_GRAD_DIR").toInt();
	DKI_MIN_NUM_GRAD_DIR = setting.value("DKI_NUM_GRAD_DIR").toInt();
	DKI_MIN_NUM_B = setting.value("DKI_NUM_B").toInt();

	//DICOM reader(input: filenames, TimeAsVectorOn for dynamic data)
	DicomReader = vtkSmartPointer<vtkDICOMReader>::New();
	DicomReader->SetFileNames(Files);
	DicomReader->TimeAsVectorOn();
	DicomReader->AutoRescaleOff();
	DicomReader->SetMemoryRowOrderToFileNative();
	DicomReader->Update();
	DicomReader->UpdateInformation();

	m_MatrixPat = DicomReader->GetPatientMatrix();

	vtkDICOMMetaData* DicomMetaData = vtkDICOMMetaData::New();
	DicomMetaData = DicomReader->GetMetaData();

	//Parameters initialization
	imageData = vtkSmartPointer<vtkImageData>::New();
	sourceImageSlice = vtkSmartPointer<vtkImageData>::New();
	processedImageData = DiffusionCalculatorVectorImageType::New();
	gradientDirection = GradientDirectionContainerType::New();

	//itkSourceImageData = DiffusionCalculatorVectorImageType::New();

	imageDataType = "OTHERS";	
	sortingNeed = false;
	scaleIntercept = 0;

	this->imageOrientation = 'T';

	/*Source images for viewing are already scaled to float type, So the scale parameters are set to 1 and 0*/
	scaleSlopeSlice = 1;
	scaleInterceptSlice = 0;
	TE = 0.0;
	FA = 0.0;

	/*for diffusion, the number of components is the diffusion encoding numbers: includs b value and direction;
	  for perfusion, the number of components is the number of dynamics.*/
	numberOfComponents = DicomReader->GetNumberOfScalarComponents(); 
	DicomReader->GetOutput()->GetDimensions(imageDimensions);

	/*Diffusion parameters*/
	numberOfBValue = 0;
	numberOfGradDirection = 0;

	IsoImageLabel.push_back(0);
	slice2PatMatrix.set_size(3, 3);
	slice2PatMatrix.set_identity();

	frametimeTags.append(vtkDICOMTag(0x0018, 0x1060));//trigger time
	frametimeTags.append(vtkDICOMTag(0x0008, 0x0033));//Content time
	frametimeTags.append(vtkDICOMTag(0x0008, 0x0031));//Series times
	frametimeTags.append(vtkDICOMTag(0x0008, 0x0032));//Acquisition times
	//reading DICOM info to the parameters
	this->DicomInfo(DicomMetaData);
	
	m_InputFileNames = Files;//????:??series,??????series?MetaData
	
	//m_bvalListOK = false; //Useless

};

//----------------------------------------------------------------------------------------------
// \GE DICOM tag
//----------------------------------------------------------------------------------------------
void DicomHelper::DicomTagForGE(vtkDICOMMetaData* metaData)
{
	vtkDICOMTag DiffusionBValues = vtkDICOMTag(0x0043, 0x1039);//    vtkDICOMTag(0x0043, 0x1039)
	vtkDICOMTag GradDirectionNo = vtkDICOMTag(0x0019, 0x10e0);

	vtkDICOMTag PEDirectioin    =       vtkDICOMTag(0x0018, 0x1312);// COL or ROW?

	vtkDICOMTag DiffusionDirectionRL =      vtkDICOMTag(0x0019, 0x10bb);//if COL, i; if RWO, j
	vtkDICOMTag DiffusionDirectionFH  =     vtkDICOMTag(0x0019, 0x10bc);//if COL, j; if RWO, i
	vtkDICOMTag DiffusionDirectionAP   =    vtkDICOMTag(0x0019, 0x10bd);// slice; k


	vtkIntArray* fileIndexArray = vtkIntArray::New();
	fileIndexArray = DicomReader->GetFileIndexArray();
	vtkIntArray* frameIndexArray = vtkIntArray::New();
	frameIndexArray = DicomReader->GetFrameIndexArray();

	//
	//1. Get image scaled parameters
	//
	//For some DICOM, the slope parameter is different among the components
	float scaleSlope;
	for (int i = 0; i < numberOfComponents; i++)
	{
		scaleSlope = 1;
		scaleSlopeList.push_back(scaleSlope);
	}

	//
	//2. Determine imageDataType and treat accordingly
	//
	int index = fileIndexArray->GetComponent(0, numberOfComponents - 2);
	int frameIndex = frameIndexArray->GetComponent(0, numberOfComponents - 2);
	if (this->numberOfComponents < 2)
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
	else if (metaData->GetAttributeValue(0, 0, GradDirectionNo).AsInt() > 1 ||
		metaData->GetAttributeValue(index, frameIndex, DiffusionBValues).AsInt() > 0 ||
		metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionAP).AsFloat() > 0 ||
		metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionFH).AsFloat() > 0 ||
		metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionRL).AsFloat() > 0
		)
	{
		imageDataType = "DIFFUSION";
		bValueDialog* dialog = new bValueDialog(false, this);
		//connect(dialog, SIGNAL(SignalbValLoaded(std::vector<float>, int, GradientDirectionContainerType::Pointer)), this, SLOT(onLoadbValFromFile(std::vector<float>, int, GradientDirectionContainerType::Pointer)));
		//int ret = dialog->exec();
		std::string PEdirection;
		float b_value = 0;
		GradientDirectionType direction;
		GradientDirectionContainerType::Pointer rawGradientDirection = GradientDirectionContainerType::New();

		if (dialog->exec())
		{
			if (!dialog->ResolveOK) // return and label this image type as "OTHERS"
			{
				this->imageDataType = "OTHERS";
				return;
			}
			BvalueList = dialog->GetBValueList();

			if (dialog->isDTI()) //if .bvec is provided, using .bvec
			{
				rawGradientDirection = dialog->GetGradientDirection();
			}
			else //Otherwise, get directions from dicom
			{
				for (int i = 0; i < numberOfComponents; i++) //i = i + numberOfGradDirection
				{
					int index = fileIndexArray->GetComponent(0, i);
					int frameIndex = frameIndexArray->GetComponent(0, i);

					// get diffusion direction value as float;
					float AP_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionAP).AsFloat();
					float FH_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionFH).AsFloat();
					float RL_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionRL).AsFloat();

					PEdirection = metaData->GetAttributeValue(0, 0, PEDirectioin).AsString();
					if (PEdirection.compare("COL") == 0)
					{
						direction[0] = RL_direction;
						direction[1] = FH_direction;
						direction[2] = AP_direction;
					}
					else if (PEdirection.compare("ROW") == 0)
					{
						direction[0] = FH_direction;
						direction[1] = RL_direction;
						direction[2] = AP_direction;
					}

					rawGradientDirection->push_back(direction);
				}
			}
			if (BvalueList.size() != numberOfComponents || rawGradientDirection->size() != numberOfComponents)
			{
				QMessageBox msgBox(QMessageBox::Warning, tr("Incorrect File"), tr("Cannot retrieve info correctly from provided .bvec .bval files. "), 0, this);
				msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
				if (msgBox.exec() == QMessageBox::AcceptRole)
				{
					this->imageDataType = "OTHERS";
					return;
				}
			}
		}
		else
		{
			for (int i = 0; i < numberOfComponents; i++) //i = i + numberOfGradDirection
			{
				int index = fileIndexArray->GetComponent(0, i);
				int frameIndex = frameIndexArray->GetComponent(0, i);
				// get b value as vtkDICOMValue;(GE store b value as IS, which need to be transfered) 
				/*	# Parse this:
				# (0043,1039) IS [1000001250\8\0\0] #  16, 4 Unknown Tag & Data
				# GE Discovery w750*/
				QString bvalStr = QString::fromStdString(metaData->GetAttributeValue(index, frameIndex, DiffusionBValues).AsString());
				b_value = float(bvalStr.split('\\')[0].toInt() % 100000);

				// get diffusion direction value as float;
				float AP_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionAP).AsFloat();
				float FH_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionFH).AsFloat();
				float RL_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionRL).AsFloat();

				PEdirection = metaData->GetAttributeValue(0, 0, PEDirectioin).AsString();
				if (PEdirection.compare("COL") == 0)
				{
					direction[0] = RL_direction;
					direction[1] = FH_direction;
					direction[2] = AP_direction;
				}
				else if (PEdirection.compare("ROW") == 0)
				{
					direction[0] = FH_direction;
					direction[1] = RL_direction;
					direction[2] = AP_direction;
				}
				//Push back b value and gradient direction to vector
				BvalueList.push_back((float)b_value);
				rawGradientDirection->push_back(direction);
			}
		}

		this->DiffusionPrepare(rawGradientDirection);
	}
	else if(PerfusionInfo(metaData, fileIndexArray, frameIndexArray))
	{
		SetPerfusionCategory();
	}
	else
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}

};

//----------------------------------------------------------------------------------------------
// \Vusion DICOM tag
//----------------------------------------------------------------------------------------------
void DicomHelper::DicomTagForVusion(vtkDICOMMetaData* metaData)
{
	vtkDICOMTag ScaleSlop = vtkDICOMTag(0x0040, 0x9225);
	vtkDICOMTag ScaleInterpcept = vtkDICOMTag(0x0040, 0x9224);

	vtkDICOMTag DiffusionBValues = vtkDICOMTag(0x0018, 0x9087);//b value tag
	vtkDICOMTag GradDirectionNo = vtkDICOMTag(0x0018, 0x9089);//Gradient orientation tag

	vtkIntArray* fileIndexArray = vtkIntArray::New();
	fileIndexArray = DicomReader->GetFileIndexArray();
	vtkIntArray* frameIndexArray = vtkIntArray::New();
	frameIndexArray = DicomReader->GetFrameIndexArray();

	//
	//1. Get image scaled parameters
	//
	//For some DICOM, the slope parameter is different among the components
	float scaleSlope;
	for (int i = 0; i < numberOfComponents; i++)
	{
		int index = fileIndexArray->GetComponent(0, i);
		int framIndex = frameIndexArray->GetComponent(0, i);
		if (metaData->GetAttributeValue(index, framIndex, ScaleSlop).IsValid())
		{
			scaleSlope = metaData->GetAttributeValue(index, framIndex, ScaleSlop).AsFloat();
			qDebug() << "[DicomHelper] Component [" << i << "] has scale slope:" << scaleSlope;
		}
		else
			scaleSlope = 1;

		scaleSlopeList.push_back(scaleSlope);
	}
	if (metaData->GetAttributeValue(0, 0, ScaleInterpcept).IsValid())
	{
		scaleIntercept = metaData->GetAttributeValue(0, 0, ScaleInterpcept).AsFloat();
	}

	//
	//2. Determine imageDataType and treat accordingly
	//
	int index = fileIndexArray->GetComponent(0, numberOfComponents - 2);
	int frameIndex = frameIndexArray->GetComponent(0, numberOfComponents - 2);

	if (this->numberOfComponents < 2)
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
	else if (metaData->GetAttributeValue(0, 0, GradDirectionNo).AsInt() > 1 ||
		metaData->GetAttributeValue(index, frameIndex, DiffusionBValues).AsInt() > 0 ||
		metaData->HasAttribute(DiffusionBValues)
		)
	{
		imageDataType = "DIFFUSION";


		float b_value = 0;

		GradientDirectionType direction;
		GradientDirectionContainerType::Pointer rawGradientDirection = GradientDirectionContainerType::New();

		for (int i = 0; i < numberOfComponents; i++) //i = i + numberOfGradDirection
		{
			int index = fileIndexArray->GetComponent(0, i);
			int frameIndex = frameIndexArray->GetComponent(0, i);
			// get b value as vtkDICOMValue;(GE store b value as IS, which need to be transfered) 
			b_value = metaData->GetAttributeValue(index, frameIndex, DiffusionBValues).AsFloat();

			vtkDICOMValue gradientOrientationValue = metaData->GetAttributeValue(index, frameIndex, GradDirectionNo);
			double tmpValue[3] = { 0.0, 0.0, 1.0 };
			if (gradientOrientationValue.GetNumberOfValues() == 3)
			{
				gradientOrientationValue.GetValues(tmpValue, 3);
			}
			direction[0] = tmpValue[0];
			direction[1] = tmpValue[1];
			direction[2] = tmpValue[2];

			//Push back b value and gradient direction to vector
			BvalueList.push_back((float)b_value);
			rawGradientDirection->push_back(direction);

		}
		this->DiffusionPrepare(rawGradientDirection);

	}
	else if (PerfusionInfo(metaData, fileIndexArray, frameIndexArray))
	{
		SetPerfusionCategory();
	}
	else
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
}

//----------------------------------------------------------------------------------------------
// \Philips DICOM tag
//----------------------------------------------------------------------------------------------
void DicomHelper::DicomTagForPhilips(vtkDICOMMetaData* metaData)
{
	// default Philips
	vtkDICOMTag ScaleSlop = vtkDICOMTag(0x2005, 0x100E);
	vtkDICOMTag ScaleInterpcept = vtkDICOMTag(0x2005, 0x100D);
	vtkDICOMTag SliceGap = vtkDICOMTag(0x0018, 0x0088);
	vtkDICOMTag IsDiffusionSeries = vtkDICOMTag(0x2005, 0x1014); 
	vtkDICOMTag DiffusionBValues = metaData->GetAttributeValue(vtkDICOMTag(0x0018, 0x9087)).IsValid()?
		vtkDICOMTag(0x0018, 0x9087): vtkDICOMTag(0x2001, 0x1003);

	vtkDICOMTag GradDirectionString = vtkDICOMTag(0x2001, 0x1004);

	vtkDICOMTag DiffusionDirectionRL = vtkDICOMTag(0x2005, 0x10b0); //x direction
	vtkDICOMTag DiffusionDirectionFH = vtkDICOMTag(0x2005, 0x10b2); //y direction
	vtkDICOMTag DiffusionDirectionAP = vtkDICOMTag(0x2005, 0x10b1);//z direction
	vtkDICOMTag GradDirectionNo = vtkDICOMTag(0x2005, 0x1415);
	vtkDICOMTag NrOfBvalues = vtkDICOMTag(0x2005, 0x1414);

	vtkDICOMTag IsDynamicSeries = vtkDICOMTag(0x2001, 0x1012); 
	vtkDICOMTag NrOfDynamics = vtkDICOMTag(0x2001, 0x1081);
	vtkDICOMTag DynamicTime = vtkDICOMTag(0x2005, 0x10A0);

	vtkIntArray* fileIndexArray = vtkIntArray::New();
	fileIndexArray = DicomReader->GetFileIndexArray();
	vtkIntArray* frameIndexArray = vtkIntArray::New();
	frameIndexArray = DicomReader->GetFrameIndexArray();

	//
	//1. Get image scaled parameters
	//
	//For some DICOM, the slope parameter is different among the components
	float scaleSlope;
	for (int i = 0; i < numberOfComponents; i++)
	{
		int index = fileIndexArray->GetComponent(0, i);
		int framIndex = frameIndexArray->GetComponent(0, i);
		if (metaData->GetAttributeValue(index, framIndex, ScaleSlop).IsValid())
		{
			scaleSlope = metaData->GetAttributeValue(index, framIndex, ScaleSlop).AsFloat();
			//qDebug() << "[DicomHelper] Component [" << i << "] has scale slope:" << scaleSlope;
		}
		else
			scaleSlope = 1;

		scaleSlopeList.push_back(scaleSlope);
	}

	if (metaData->GetAttributeValue(0, 0, ScaleInterpcept).IsValid())
	{
		scaleIntercept = metaData->GetAttributeValue(0, 0, ScaleInterpcept).AsFloat();
	}

	//
	//2. Determine imageDataType and treat accordingly
	//
	int index = fileIndexArray->GetComponent(0, numberOfComponents - 2);
	int frameIndex = frameIndexArray->GetComponent(0, numberOfComponents - 2);

	if (this->numberOfComponents < 2)
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
	else if (//isDiffusionSerires == Y
		metaData->GetAttributeValue(IsDiffusionSeries).AsString()[0] == 'Y' ||
		//number of gradient direction  > 1
		metaData->GetAttributeValue(0, 0, GradDirectionNo).AsInt() > 1 ||
		//number of b values > 1
		metaData->GetAttributeValue(0, 0, NrOfBvalues).AsInt() > 1 ||
		//the b value of last component > 0 
		metaData->GetAttributeValue(index, frameIndex, DiffusionBValues).AsInt() > 0 ||
		//the gradient direction of last component > 0
		metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionAP).AsFloat() > 0 ||
		metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionFH).AsFloat() > 0 ||
		metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionRL).AsFloat() > 0
		)
	{
		imageDataType = "DIFFUSION";

		//numberOfGradDirection = metaData->GetAttributeValue(0, 0, GradDirectionNo).AsInt();
		//numberOfBValue = metaData->GetAttributeValue(0, 0, NrOfBvalues).AsInt();

		//
		//2.0 With Draw B values and vectors directly from DICOM
		//

		GradientDirectionType direction;
		GradientDirectionContainerType::Pointer rawGradientDirection = GradientDirectionContainerType::New();

		float b_value = 0;
		qDebug() << "[DicomHelper] Number of Components = " << numberOfComponents << endl;
		for (int i = 0; i < numberOfComponents; i++) //i = i + numberOfGradDirection
		{
			int index = fileIndexArray->GetComponent(0, i);
			int frameIndex = frameIndexArray->GetComponent(0, i);
			// get b value
			b_value = metaData->GetAttributeValue(index, frameIndex, DiffusionBValues).AsFloat();

			// get diffusion direction value as float;
			float AP_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionAP).AsFloat();
			float FH_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionFH).AsFloat();
			float RL_direction = metaData->GetAttributeValue(index, frameIndex, DiffusionDirectionRL).AsFloat();

			direction[0] = RL_direction;
			direction[1] = AP_direction;
			direction[2] = FH_direction;

			BvalueList.push_back((float)b_value);
			rawGradientDirection->push_back(direction);
		}

		this->DiffusionPrepare(rawGradientDirection);

		this->CorrectPHLGradientDirection(metaData);
		qDebug() << "[DicomHelper]PHILIPS number of b values:" << numberOfBValue << endl;
		qDebug() << "[DicomHelper]PHILIPS number of components: " << numberOfComponents << endl;
		qDebug() << "[DicomHelper]PHILIPS number of gradient direction:" << numberOfGradDirection << endl;
		qDebug() << "[DicomHelper] Loaded " << endl;

	}
	else if (PerfusionInfo(metaData, fileIndexArray, frameIndexArray))
	{
		SetPerfusionCategory();
	}
	else
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
}

//----------------------------------------------------------------------------------------------
// \Siemens DICOM tag
//----------------------------------------------------------------------------------------------
void DicomHelper::DicomTagForSiemens(vtkDICOMMetaData* metaData)
{
	// relevant Siemens private tags
	/* https://nmrimaging.wordpress.com/tag/dicom/
	For SIEMENS MRI:
	The software version at least B15V (0018; 1020), follow tag value would be useful
	0019; 100A;  Number Of Images In Mosaic
	0019; 100B;  Slice Measurement Duration
	0019; 100C;  B_value
	0019; 100D; Diffusion Directionality
	0019; 100E; Diffusion Gradient Direction
	0019; 100F;  Gradient Mode
	0019; 1027;  B_matrix
	0019; 1028;  Bandwidth Per Pixel Phase Encode
	*/

	//0. Check if Mosaic
	std::string SImageType;
	SImageType = metaData->GetAttributeValue(vtkDICOMTag(0x0008, 0x0008)).AsString();	
	std::cout << SImageType << std::endl;

	if (SImageType.find("MOSAIC") != std::string::npos)
	{
		std::cout << "[DicomHelper] SliceMosaic......" << std::endl;
	}

	//Check software version and CSA header
	std::string softwareVersion;
	softwareVersion = metaData->GetAttributeValue(vtkDICOMTag(0x0018, 0x1020)).AsString();
	std::cout << "[DicomHelper] Loading Data with Siemens Software Version: " << softwareVersion << std::endl;

	float scaleSlope;
	for (int i = 0; i < numberOfComponents; i++)
	{
		scaleSlope = 1;
		scaleSlopeList.push_back(scaleSlope);
	}


	//1. Check if Diffusion
	
	int tempBValue = -123;
	bool describeAsDiff(false);
	bool bValueTagExist(false);

	if (metaData->GetAttributeValue(vtkDICOMTag(0x0051, 0x1016)).IsValid())
	{
		std::string imgDes;
		imgDes = metaData->GetAttributeValue(vtkDICOMTag(0x0051, 0x1016)).AsString();
		std::cout << SImageType << std::endl;
		if (imgDes.find("DIFFUSION") != std::string::npos)
		{
			// IsDiffusionSeries = true;
			describeAsDiff = true;
		}
	}

	if (metaData->GetAttributeValue(vtkDICOMTag(0x0019, 0x100c)).IsValid())
	{
		tempBValue = metaData->GetAttributeValue(vtkDICOMTag(0x0019, 0x100c)).AsInt();
		if (tempBValue >= 0)
		{
			bValueTagExist = true;
		}
	}
	
	//2. Extract DWI if diffusion
	vtkIntArray* fileIndexArray = vtkIntArray::New();
	fileIndexArray = DicomReader->GetFileIndexArray();
	vtkIntArray* frameIndexArray = vtkIntArray::New();
	frameIndexArray = DicomReader->GetFrameIndexArray();

	if (this->numberOfComponents < 2)
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
	else if (describeAsDiff&&bValueTagExist)
	{
		imageDataType = "DIFFUSION";

		qDebug() << "[DicomHelper] Siemens Diffusion Data. Number of component = " << numberOfComponents << endl;

		GradientDirectionType direction;
		GradientDirectionContainerType::Pointer rawGradientDirection = GradientDirectionContainerType::New();

		int numberOfB0Images(0);		
		int former_bValue = -1;
		int b_value = 0;

		for (int i = 0; i < numberOfComponents; i++) //i = i + numberOfGradDirection
		{
			int index = fileIndexArray->GetComponent(0, i);
			int frameIndex = frameIndexArray->GetComponent(0, i);
			// get b value as vtkDICOMValue;(GE store b value as IS, which need to be transfered) 
			vtkDICOMValue dicomBValue = metaData->GetAttributeValue(index, frameIndex, vtkDICOMTag(0x0019, 0x100c));

			b_value = dicomBValue.AsInt();

			qDebug() << "[DicomHelper] Siemens Bvalue of component " << i << " is " << b_value << endl;
			if (metaData->GetAttributeValue(index, frameIndex, vtkDICOMTag(0x0019, 0x100e)).IsValid())
			{
				vtkDICOMValue gradientOrientationValue = metaData->GetAttributeValue(index, frameIndex, vtkDICOMTag(0x0019, 0x100e));
				double tmpValue[3] = { 0.0, 0.0, 1.0 };
				if (gradientOrientationValue.GetNumberOfValues() == 3)
				{
					gradientOrientationValue.GetValues(tmpValue, 3);
				}

				direction[0] = tmpValue[0];
				direction[1] = tmpValue[1];
				direction[2] = tmpValue[2];
			}
			else
			{
				direction[0] = 0;
				direction[1] = 0;
				direction[2] = 0;
			}
			// Number of B0 images acquired (current for DKI or NODDI)
			if (b_value == 0)   numberOfB0Images++;

			//check whether the DICOM date is disorder based on b values (current for GE data)
			if (b_value < former_bValue)	sortingNeed = true;

			//Push back b value and gradient direction to vector
			BvalueList.push_back((float)b_value);
			former_bValue = b_value;
			rawGradientDirection->push_back(direction);
		}

		bool useSiemensBMatrix = false; //Use Siemens BMatrix to get more accurate B value
		if (useSiemensBMatrix)
		{
			if (metaData->GetAttributeValue(vtkDICOMTag(0x0029, 0x1010)).IsValid())
			{
				int numberOfB0Images(0);

				for (int i = 0; i < numberOfComponents; i++) //i = i + numberOfGradDirection
				{
					int index = fileIndexArray->GetComponent(0, i);
					int frameIndex = frameIndexArray->GetComponent(0, i);

					CSAHeader csaHeader;

					const unsigned char* diffusionInfo = metaData->GetAttributeValue(index, frameIndex, vtkDICOMTag(0x0029, 0x1010)).GetUnsignedCharData();
					const char* diffusionInfoString = reinterpret_cast<const char*>(diffusionInfo);
					this->DecodeCSAHeader(csaHeader, diffusionInfoString);

					/* check b value for current stride */
					double bValue = -123;

					vnl_matrix_fixed<double, 3, 3> bMatrix(0.0);

					bool hasBMatrix = ExtractSiemensBMatrix(&csaHeader, i, bMatrix);
					if (hasBMatrix && (bValue != 0))
					{
						std::cout << "=============================================" << std::endl;
						std::cout << "BMatrix calculations..." << std::endl;

						// UNC comments: The principal eigenvector of the bmatrix is to be extracted as
						// it's the gradient direction and trace of the matrix is the b-value

						// UNC comments: Computing the decomposition
						vnl_svd<double> svd(bMatrix);

						// UNC comments: Extracting the principal eigenvector i.e. the gradient direction
						direction[0] = svd.U(0, 0);
						direction[1] = svd.U(1, 0);
						direction[2] = svd.U(2, 0);

						std::cout << "BMatrix: " << std::endl;
						std::cout << bMatrix[0][0] << std::endl;
						std::cout << bMatrix[0][1] << "\t" << bMatrix[1][1] << std::endl;
						std::cout << bMatrix[0][2] << "\t" << bMatrix[1][2] << "\t" << bMatrix[2][2] << std::endl;

						// UNC comments: The b-value si the trace of the bmatrix
						const double bmatrixCalculatedBValue = bMatrix[0][0] + bMatrix[1][1] + bMatrix[2][2];
						std::cout << bmatrixCalculatedBValue << std::endl;

						// UNC comments: Even if the bmatrix is null, the svd decomposition set the 1st eigenvector
						// to (1,0,0). So we force the gradient direction to 0 if the bvalue is null
						if (bmatrixCalculatedBValue < 1e-2)
						{
							std::cout << "B0 image detected from bmatrix trace: gradient direction forced to 0" << std::endl;

							bValue = 0;
							std::cout << "[DicomHelper] Gradient coordinates: " << direction[0]
								<< " " << direction[1]
								<< " " << direction[2] << "BValue = " << bValue << std::endl;
						}
						else
						{
							bValue = bmatrixCalculatedBValue;
							std::cout << "[DicomHelper] Gradient coordinates: " << direction[0]
								<< " " << direction[1]
								<< " " << direction[2] << "BValue = " << bValue << std::endl;
						}

						BvalueList.push_back((bValue < 1e-2) ? 0.0 : bValue);

						rawGradientDirection->push_back(direction);
					}
					else
					{
						qDebug() << "[DicomHelper] bMatrix does not exit in CSA header" << endl;
					}
				}
			}
		}

		this->DiffusionPrepare(rawGradientDirection);
	}
	else if (PerfusionInfo(metaData, fileIndexArray, frameIndexArray))
	{
		SetPerfusionCategory();
	}
	else
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
}

//----------------------------------------------------------------------------------------------
// \Other Vendor's generic DICOM tag
//----------------------------------------------------------------------------------------------
void DicomHelper::DicomTagForOthers(vtkDICOMMetaData* metaData)
{
	if (manuFacturer[0] == 'A' || manuFacturer[0] == 'a')
	{
		qDebug() << "[DicomHelper] Anke data" << endl;
		vtkDICOMTag ScaleSlop = vtkDICOMTag(0x0028, 0x1053);
	}


	vtkIntArray* fileIndexArray = vtkIntArray::New();
	fileIndexArray = DicomReader->GetFileIndexArray();
	vtkIntArray* frameIndexArray = vtkIntArray::New();
	frameIndexArray = DicomReader->GetFrameIndexArray();
	
	vtkDICOMTag ScaleSlop = vtkDICOMTag(0x5501, 0x1154);
	float scaleSlope;
	for (int i = 0; i < numberOfComponents; i++)
	{
		int index = fileIndexArray->GetComponent(0, i);
		int framIndex = frameIndexArray->GetComponent(0, i);
		if (metaData->GetAttributeValue(index, framIndex, ScaleSlop).IsValid())
		{
			scaleSlope = metaData->GetAttributeValue(index, framIndex, ScaleSlop).AsFloat();
			qDebug() << "[DicomHelper] Component [" << i << "] has scale slope:" << scaleSlope;
		}
		else
			scaleSlope = 1;
		//if ((manuFacturer[0] == 'N') || (manuFacturer[0] == 'n'))
		//{
			scaleSlope = 100000 / scaleSlope;
		//}
		scaleSlopeList.push_back(scaleSlope);
	}


	if (this->numberOfComponents < 2)
	{
		imageDataType = "OTHERS";
		_GenerateImageData(imageData, nullptr, false, 1, nullptr);
		qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
	}
	else {
		QMessageBox* info = new QMessageBox(this);
		info->setWindowTitle(tr("Please select data type"));
		info->setText(tr("The data type cannot be identified automatcally. Please choose the correct type."));
		info->addButton(tr("Diffusion Data"), QMessageBox::AcceptRole);
		info->addButton(tr("Perfusion Data"), QMessageBox::RejectRole);
		info->addButton(tr("Neither（Display Only）"), QMessageBox::HelpRole);
		int ret = info->exec();
		if (ret == QMessageBox::AcceptRole)
		{
			imageDataType = "DIFFUSION";

			GradientDirectionContainerType::Pointer rawGradientDirection = GradientDirectionContainerType::New();

			qDebug() << "[DicomHelper] Num of Components = " << this->numberOfComponents<<endl;
			
			bValueDialog* dialog = new bValueDialog(true, this);


			if (dialog->exec() && dialog->ResolveOK)
			{

				BvalueList = dialog->GetBValueList();
				rawGradientDirection = dialog->GetGradientDirection();
				qDebug() << "[DicomHelper] Num of BVals = " << BvalueList.size() <<  endl;
			}
			else {
				QString MESSAGE;
				QTextStream(&MESSAGE) << tr(".bvec and .bval files must be provided for diffusion processing. ");
				QMessageBox msgBox(QMessageBox::Warning, tr("More information required"), MESSAGE, 0, this);
				msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
				if (msgBox.exec() == QMessageBox::AcceptRole)
				{
					this->imageDataType = "OTHERS";
					return;
				}
			}
			this->DiffusionPrepare(rawGradientDirection);

		}
		else if (ret == QMessageBox::RejectRole)
		{
			imageDataType = "PERFUSION";
			PerfusionInfo(metaData, fileIndexArray, frameIndexArray);
		}
		else if (ret == QMessageBox::HelpRole)
		{
			imageDataType == "OTHERS";
			_GenerateImageData(imageData, nullptr, false, 1, nullptr);
			qDebug() << "[DicomHelper] Not Diffusion nor Not Perfusion, just image viewer" << endl;
		}
	}
}

//----------------------------------------------------------------------------------------------
// \DICOM information loading
// \INPUT: vtkDICOM metadata
//----------------------------------------------------------------------------------------------
void DicomHelper::DicomInfo(vtkDICOMMetaData* metaData)
{
	//which manufacture? DICOM tag is different
	manuFacturer = DicomReader->GetMedicalImageProperties()->GetManufacturer();

	//qDebug() <<"[DicomHelper] number of components" << numberOfComponents<<endl;
	switch (manuFacturer[0])
	{
	case 'G':
	case 'g': 
		this->DicomTagForGE(metaData);
		break;
	case 'V':
	case 'v': 
		this->DicomTagForVusion(metaData);
		break;
	case 'P':
	case 'p': 
		this->DicomTagForPhilips(metaData);
		break;
	case 'S':
	case 's':		
		this->DicomTagForSiemens(metaData);
		break;
	default:
		qWarning() << "[DicomHelper] Not supported Manufacturer DICOM, it is from" << manuFacturer << endl;
		DicomTagForOthers(metaData);
	}
};

void  DicomHelper::DiffusionPrepare(GradientDirectionContainerType::Pointer gradientDirs)
{
	int numberOfB0Images(0);
	float before = -1;

	for (int i = 0; i < BvalueList.size(); i++) //i = i + numberOfGradDirection
	{
		float b_value = this->BvalueList.at(i);
		// Number of B0 images acquired (current for DKI or NODDI)
		if (b_value == 0)   numberOfB0Images++;
		//check whether the DICOM date is disorder based on b values (current for GE data)
		if (b_value < before)	sortingNeed = true;

		before = b_value;
	}

	qDebug() << "[DicomHelper] Number of B0 = " << numberOfB0Images<<"Sorting Needed =" << sortingNeed << endl;
	this->_GenerateImageData(imageData, &BvalueList, sortingNeed, numberOfB0Images, gradientDirs);
	qDebug() << "[DicomHelper] BvalueList size = " << BvalueList.size() << "before = " << before << endl;

	before = -1;
	
	for (unsigned int i = 0; i < BvalueList.size(); i++)
	{
		if (BvalueList.at(i) != before)
		{
			numberOfBValue++;
			before = BvalueList.at(i);
		}
		qDebug() << "b value:" << BvalueList.at(i) << endl;
	}

	qDebug() << "ToTal B Value number = " << numberOfBValue << endl;
	if (numberOfBValue > 1)
	{
		numberOfGradDirection = (numberOfComponents - 1) / (numberOfBValue - 1);
	}
	else //Solving a bug for DynaCAD generated ADC dicom.
	{
		numberOfGradDirection = 0;
		this->imageDataType = "OTHERS";
	}

	qDebug() << "Constant from Setting is " << this->IVIM_MIN_NUM_B << this->DTI_MIN_NUM_GRAD_DIR << this->DKI_MIN_NUM_GRAD_DIR << this->DKI_MIN_NUM_B << endl;

	//what calculation these data apply?
	if (numberOfGradDirection < DTI_MIN_NUM_GRAD_DIR && numberOfBValue < IVIM_MIN_NUM_B)
	{
		qDebug() << "[DicomHelper] Diffusion Dicom DWI OK " << endl;
		imageCalculationType = DWI;
	}
	if (numberOfGradDirection < DTI_MIN_NUM_GRAD_DIR && numberOfBValue >= IVIM_MIN_NUM_B)
	{
		qDebug() << "[DicomHelper] Diffusion Dicom IVIM OK " << endl;
		imageCalculationType = IVIM;
	}
	if (numberOfGradDirection >= DTI_MIN_NUM_GRAD_DIR)
	{
		imageCalculationType = DTI;
		qDebug() << "[DicomHelper] Diffusion Dicom DTI OK " << endl;
		dki_MatrixA.set_size(gradientDirection->Size(), 7);
		MakeMatrixA(BvalueList, gradientDirection, dki_MatrixA);
	}
	if (numberOfGradDirection >= DKI_MIN_NUM_GRAD_DIR && numberOfBValue > DKI_MIN_NUM_B)
	{
		imageCalculationType = DKI;
		qDebug() << "[DicomHelper] Diffusion Dicom DKI OK " << endl;
		dki_MatrixA.set_size(gradientDirection->Size(), 22);
		MakeMatrixA(BvalueList, gradientDirection, dki_MatrixA);
	}
}

//----------------------------------------------------------------------------------------------
// \Perfusion DICOM information loading
// \Fill the perfusion related parameters
//----------------------------------------------------------------------------------------------
bool DicomHelper::PerfusionInfo(vtkDICOMMetaData* metaData,
	vtkIntArray* fileIndexArray, vtkIntArray* frameIndexArray)
{
	bool frameTimeOK;
	int tagIndex = 0;
	while ((tagIndex < frametimeTags.size()))
	{
		frameTimeOK = true;
		//read frame time list;
		QList<float> frametimeList;
		vtkDICOMTag frametimeTag = frametimeTags.at(tagIndex);
		float firstFrametime;
		float frametime;
		for (int i = 0; i < numberOfComponents; i++) //i = i + numberOfGradDirection
		{
			int index = fileIndexArray->GetComponent(0, i);
			int framIndex = frameIndexArray->GetComponent(0, i);

			if ((tagIndex == 1) || (tagIndex == 2) || (tagIndex == 3))
			{
				QString frametimeQ = QString::fromStdString(metaData->GetAttributeValue(index, framIndex, frametimeTag).AsString());
				frametime = Tm2ms(frametimeQ);
			}
			else
			{
				frametime = metaData->GetAttributeValue(index, framIndex, frametimeTag).AsFloat();
			}
			if (i == 0)
				firstFrametime = frametime;

			frametimeList.append((frametime - firstFrametime) / 1000);


		}
		//check framtimelist is valid;
		float deltaT = frametimeList.at(1);
		for (int i = 1; i < frametimeList.size(); i++)
		{
			
			float deltaT_i = frametimeList.at(i) - frametimeList.at(i - 1);
			frameTimeOK = frameTimeOK
				&& (frametimeList.at(i) > 0)               //dynamic time is larger than 0;
				&& (deltaT_i > 0)                          //frametimeList.at(i) > frametimeList.at(i-1);
				&& (abs(deltaT_i - deltaT) < deltaT*0.01); //deltT should be all most the same;
			qDebug() << "time:" << frametimeList.at(i) <<" "<<frameTimeOK<<endl;

		}
		if (frameTimeOK)
		{
			this->dynamicTime = frametimeList.toVector().toStdVector();
			return true;
		}
		else
		{
			tagIndex++;
		}

	}
	return frameTimeOK;
};

float DicomHelper::Tm2ms(QString tmQ)
{
	QString hhmmss;
	QString ssfrac;
	if (tmQ.size() < 6) return 0;
	if (tmQ.contains('.'))
	{
		hhmmss = tmQ.split('.')[0];
		ssfrac = "0." + tmQ.split('.')[1];
	}
	else
	{
		hhmmss = tmQ;
		ssfrac = "0";
	}

	float sec;
	if (hhmmss.size() == 6) //HHMMSS
		sec = hhmmss.left(2).toFloat() * 60 * 60 + hhmmss.mid(2, 2).toFloat() * 60 + hhmmss.right(2).toFloat();
	else if (hhmmss.size() == 4) //HHMM
		sec = hhmmss.left(2).toFloat() * 60 * 60 + hhmmss.mid(2, 2).toFloat() * 60;
	else if (hhmmss.size() == 2) //HH
		sec = hhmmss.left(2).toFloat() * 60 * 60;

	sec = sec + ssfrac.toFloat();
	return sec * 1000;
}


//----------------------------------------------------------------------------------------------
// \The matrix descripts the slice location to Patient Position
// \From Philips
//----------------------------------------------------------------------------------------------
void DicomHelper::GetSliceToPatMatrix(vtkDICOMMetaData* metaData)
{
	vtkDICOMTag ImagingOrientation = vtkDICOMTag(0x2001, 0x100B); //trans cor sag
	vtkDICOMTag ImagingDirectionRL = vtkDICOMTag(0x2005, 0x1002);//RL
	vtkDICOMTag ImagingDirectionFH = vtkDICOMTag(0x2005, 0x1001);//FH
	vtkDICOMTag ImagingDirectionAP = vtkDICOMTag(0x2005, 0x1000);//AP

	itk::Vector<double, 3> ang;
	const char *image_orientation;
	if (metaData->GetAttributeValue(0, 0, ImagingOrientation).IsValid())
	{
		//std::string image_orientation
		image_orientation = metaData->GetAttributeValue(0, 0, ImagingOrientation).GetCharData();
		if (!image_orientation)
			image_orientation = "TRANSVERSAL";
	}
	if (metaData->GetAttributeValue(0, 0, ImagingDirectionRL).IsValid()
		&& metaData->GetAttributeValue(0, 0, ImagingDirectionFH).IsValid()
		&& metaData->GetAttributeValue(0, 0, ImagingDirectionAP).IsValid())
	{
		ang[0] = metaData->GetAttributeValue(0, 0, ImagingDirectionRL).AsDouble();
		ang[1] = metaData->GetAttributeValue(0, 0, ImagingDirectionAP).AsDouble();
		ang[2] = metaData->GetAttributeValue(0, 0, ImagingDirectionFH).AsDouble();
	}


	vnl_matrix<double> slice_apat(3,3);
	if (strcmp(image_orientation, "TRANSVERSAL"))
	{
		slice_apat(0, 0) = 0.0; slice_apat(0, 1) = -1.0; slice_apat(0, 2) = 0.0;
		slice_apat(1, 0) = -1.0; slice_apat(1, 1) = 0.0; slice_apat(1, 2) = 0.0;
		slice_apat(2, 0) = 0.0; slice_apat(2, 1) =  0.0; slice_apat(2, 2) = 1.0;
	}
	else if (strcmp(image_orientation, "CORONAL"))
	{
		slice_apat(0, 0) = 0.0; slice_apat(0, 1) = 0.0; slice_apat(0, 2) = 1.0;
		slice_apat(1, 0) = -1.0; slice_apat(1, 1) = 0.0; slice_apat(1, 2) = 0.0;
		slice_apat(2, 0) = 0.0; slice_apat(2, 1) = 1.0; slice_apat(2, 2) = 0.0;
	}
	else if (strcmp(image_orientation, "SAGITTAL"))
	{
		slice_apat(0, 0) = 0.0; slice_apat(0, 1) = 0.0; slice_apat(0, 2) = 1.0;
		slice_apat(1, 0) = 0.0; slice_apat(1, 1) = -1.0; slice_apat(1, 2) = 0.0;
		slice_apat(2, 0) = -1.0; slice_apat(2, 1) = 0.0; slice_apat(2, 2) = 0.0;
	}

	vnl_matrix<double> apat_pat(3, 3);
	double	sx;  double	sy;  double	sz;
	double	cx;  double	cy;  double	cz;

	//if (right_handed)
	//{
	//	SGMAT_invert_vec(ang, &ang);
	//}

	sx = sin(-ang[0] * RAD); sy = sin(-ang[1] * RAD); sz = sin(-ang[2] * RAD);
	cx = cos(-ang[0] * RAD); cy = cos(-ang[1] * RAD); cz = cos(-ang[2] * RAD);
	
	apat_pat(0,0) = cy * cz;
	apat_pat(0,1) = -sz * cx + sx * sy * cz;
	apat_pat(0,2) = sx * sz + sy * cx * cz;

	apat_pat(1, 0) = sz * cy;
	apat_pat(1, 1) = cx * cz + sx * sy * sz;
	apat_pat(1, 2) = -sx * cz + sy * sz * cx;

	apat_pat(2, 0) = -sy;
	apat_pat(2, 1) = sx * cy;
	apat_pat(2, 2) = cx * cy;

	//slice2PatMatrix.set_size(3, 3);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3;j++)
		{
			slice2PatMatrix(i, j) = apat_pat(0, j)*slice_apat(i, 0) + apat_pat(1, j)*slice_apat(i, 1) + apat_pat(2, j)*slice_apat(i, 2);
		}
		
	}
	vnl_matrix<double> multiplier(3, 3);
	multiplier(0, 0) = 0.0; multiplier(0, 1) = -1.0; multiplier(0, 2) = 0.0;
	multiplier(1, 0) = -1.0; multiplier(1, 1) = 0.0; multiplier(1, 2) = 0.0;
	multiplier(2, 0) = 0.0; multiplier(2, 1) = 0.0; multiplier(2, 2) = 1.0;

	slice2PatMatrix = slice2PatMatrix.transpose()*multiplier;
};


//----------------------------------------------------------------------------------------------
// \Sorting the source Images 
// \It's designed for GE data
// \For some GE diffusion data, the b values are disordered.
//----------------------------------------------------------------------------------------------
void DicomHelper::_GenerateImageData(vtkImageData* sourceData, std::vector<float> *sortBasedVector, bool isSorting,
	int avgNumber, GradientDirectionContainerType::Pointer directionContainer)
{
	sourceData->SetDimensions(imageDimensions);
	sourceData->SetSpacing(DicomReader->GetOutput()->GetSpacing());
	sourceData->SetExtent(DicomReader->GetOutput()->GetExtent());
	sourceData->SetInformation(DicomReader->GetOutput()->GetInformation());
	sourceData->AllocateScalars(VTK_FLOAT, (numberOfComponents - avgNumber + 1));

	if (numberOfComponents == 1)
	{
		//for NONE image:
		vtkSmartPointer <vtkImageShiftScale> scaleVolumeImage = vtkSmartPointer <vtkImageShiftScale>::New();
		scaleVolumeImage->SetInputConnection(DicomReader->GetOutputPort());
		scaleVolumeImage->SetShift(scaleIntercept);
		scaleVolumeImage->SetScale(1.0 / scaleSlopeList[0]);
		scaleVolumeImage->ClampOverflowOn();
		scaleVolumeImage->SetOutputScalarTypeToFloat();//Change to double type in future?
		scaleVolumeImage->Update();

		sourceData->DeepCopy(scaleVolumeImage->GetOutput());
		return;
	}


	int length = numberOfComponents;
	std::vector<int> paraOrderIndex;
	std::vector< std::pair<float, int> > vectorPair;
	//gradientDirection->Reserve(length);
	for (int i = 0; i < length; i++)
	{
		//gradientDirection->SetElement(i,directionContainer->GetElement(i));
		paraOrderIndex.push_back(i);
		if (isSorting)
			vectorPair.push_back(std::make_pair((*sortBasedVector)[i], paraOrderIndex[i]));
	}

	if (isSorting)
	{
		std::stable_sort(vectorPair.begin(), vectorPair.end(), cmp);
		for (int i = 0; i < length; i++)
		{
			(*sortBasedVector)[i] = vectorPair[i].first;
			paraOrderIndex.at(i) = vectorPair[i].second;
			//gradientDirection->SetElement(i, directionContainer->GetElement(paraOrderIndex.at(i)));		
		}
	}
	if (directionContainer)
	{
		for (int i = 0; i < length; i++)
		{
			gradientDirection->Reserve(length);
			gradientDirection->SetElement(i, directionContainer->GetElement(paraOrderIndex.at(i)));
		}
	}

	//average B0 images
	vtkSmartPointer<vtkImageWeightedSum> averageB0Image = vtkSmartPointer<vtkImageWeightedSum>::New();
	vtkSmartPointer<vtkImageAppendComponents> imageAppend2 = vtkSmartPointer<vtkImageAppendComponents>::New();

	qDebug() << "[DicomHelper]Image Components Generating" << endl;

	for (int i = 0; i < numberOfComponents; i++)
	{
		vtkSmartPointer <vtkImageExtractComponents> scalarComponent = vtkSmartPointer <vtkImageExtractComponents>::New();
		scalarComponent->SetInputConnection(DicomReader->GetOutputPort());
		scalarComponent->SetComponents(paraOrderIndex.at(i));
		scalarComponent->Update();

		vtkSmartPointer <vtkImageShiftScale> scaleVolumeImage = vtkSmartPointer <vtkImageShiftScale>::New();
		scaleVolumeImage->SetInputConnection(scalarComponent->GetOutputPort());
		scaleVolumeImage->SetShift(scaleIntercept);
		scaleVolumeImage->SetScale(1.0 / scaleSlopeList[i]);
		scaleVolumeImage->ClampOverflowOn();
		scaleVolumeImage->SetOutputScalarTypeToFloat();//Change to double type in future?
		scaleVolumeImage->Update();

		if (i < avgNumber)
		{
			//B0 images
			averageB0Image->AddInputConnection(scaleVolumeImage->GetOutputPort());
			averageB0Image->SetWeight(i, 1.0 / (double)(avgNumber));
			if (i > 0)
			{
				BvalueList.erase(BvalueList.begin() + 1);
				gradientDirection->erase(gradientDirection->begin() + 1);
			}
		}
		else if (i == avgNumber)
		{
			averageB0Image->Update();
			imageAppend2->SetInputConnection(averageB0Image->GetOutputPort());
			imageAppend2->AddInputConnection(scaleVolumeImage->GetOutputPort());
		}
		else
		{

			imageAppend2->AddInputConnection(scaleVolumeImage->GetOutputPort());
		}		
	}
	imageAppend2->Update();
	sourceData->DeepCopy(imageAppend2->GetOutput());
	//update number of components after average B0 images
	numberOfComponents = numberOfComponents - avgNumber + 1;

	qDebug() << "[DicomHelper]Image Components Generated" << endl;
}

//----------------------------------------------------------------------------------------------
// \Extract one slice from the 3D image data
//----------------------------------------------------------------------------------------------
void DicomHelper::ComputeCurrentSourceImage(int currentSlice)
{
	if (!imageData) return;

	vtkSmartPointer <vtkExtractVOI> ExtractVOI = vtkSmartPointer <vtkExtractVOI>::New();
	//ExtractVOI->SetInputData(m_DicomHelper->GetDicomReader()->GetOutput());
	ExtractVOI->SetInputData(imageData);
	ExtractVOI->SetVOI(0, this->imageDimensions[0] - 1, 0, this->imageDimensions[1] - 1, currentSlice, currentSlice);
	ExtractVOI->Update();

	//Maybe we can make use of the extent info here rather than set it back to 0 via changeInformationFiter
	//Forget it. Results error for dicom writing if extent info is not correctted.
	vtkSmartPointer <vtkImageChangeInformation> changeInfo = vtkSmartPointer <vtkImageChangeInformation>::New();
	changeInfo->SetInputData(ExtractVOI->GetOutput());
	changeInfo->SetOutputOrigin(0, 0, 0);
	changeInfo->SetExtentTranslation(0, 0, -currentSlice);
	changeInfo->Update();

	if (this->imageDataType.compare("OTHERS"))
		ComputeITKSourceImage( changeInfo->GetOutput() );

    ////////////////////////////////////////////////////////////
	//do not compatable with writing source image
	//brutally disable for now. decent way left for future.
	sourceImageSlice->DeepCopy(changeInfo->GetOutput());
	//{
		//double range[2];
		//imageData->GetScalarRange(range);
		//double rescaleSlope = range[1] > range[0] ? 4095 / (range[1] - range[0]) : 1;
		//double rescaleIntercept = -range[0];
		//vtkSmartPointer <vtkImageShiftScale> scaleVolumeImage = vtkSmartPointer <vtkImageShiftScale>::New();
		//scaleVolumeImage->SetInputConnection(changeInfo->GetOutputPort());
		//scaleVolumeImage->SetShift(rescaleIntercept);
		//scaleVolumeImage->SetScale(rescaleSlope);
		//scaleVolumeImage->ClampOverflowOn();
		//scaleVolumeImage->SetOutputScalarTypeToUnsignedShort();//Change to double type in future?
		//scaleVolumeImage->Update();

		//scaleSlopeSlice = rescaleSlope;
		//scaleInterceptSlice = rescaleIntercept;

		////used for rendering: unsigned short
		//sourceImageSlice->DeepCopy(scaleVolumeImage->GetOutput());
	//}

}

void DicomHelper::ComputeITKVolumeImage()
{
	ComputeITKSourceImage(imageData);
}

void DicomHelper::ComputeITKSourceImage(vtkSmartPointer<vtkImageData> input)
{

	typedef itk::VTKImageToImageFilter <DiffusionCalculatorImageType>	VtkToItkConverterType;//3D
	typedef itk::CastImageFilter< DiffusionCalculatorImageType, DiffusionCalculatorImageType >	CastFilterType;
	typedef itk::ComposeImageFilter<DiffusionCalculatorImageType>		ImageToVectorImageType;
	typedef itk::MedianImageFilter<DiffusionCalculatorImageType, DiffusionCalculatorImageType >  MedianFilterType;

	ImageToVectorImageType::Pointer imageToVectorImageFilter = ImageToVectorImageType::New();
	/*std::vector<int>::iterator isoIt = IsoImageLabel.begin() + 1;*/
	int vectorIndex = 0;
	for (int i = 0; i < numberOfComponents; i++)
	{
		//Handle each scalar component 
		vtkSmartPointer <vtkImageExtractComponents> scalarComponent = vtkSmartPointer <vtkImageExtractComponents>::New();
		scalarComponent->SetInputData(input);
		scalarComponent->SetComponents(i);
		scalarComponent->Update();

		//VTK to ITK Image Data
		VtkToItkConverterType::Pointer vtkToItkImageFilter = VtkToItkConverterType::New();
		vtkToItkImageFilter->SetInput(scalarComponent->GetOutput());
		vtkToItkImageFilter->Update();

		//unsigned short image to float image
		CastFilterType::Pointer castFilter = CastFilterType::New();
		castFilter->SetInput(vtkToItkImageFilter->GetOutput());
		castFilter->Update();


		//if ((i != *isoIt) || (isoIt == IsoImageLabel.end()) || (numberOfGradDirection < 6))
			imageToVectorImageFilter->SetInput(i, castFilter->GetOutput());
		
	}

	imageToVectorImageFilter->Update();
	processedImageData->Graft(imageToVectorImageFilter->GetOutput());
}

//----------------------------------------------------------------------------------------------
// \Apply registration if it is choosed
//----------------------------------------------------------------------------------------------
void DicomHelper::PreProcessing(bool registration)
{
	typedef itk::Image<DiffusionCalculatorPixelType, 2> DiffusionImageType2D;
	typedef itk::VectorImage<DiffusionCalculatorPixelType,2> DiffusionVectorImageType2D;

	typedef itk::VTKImageToImageFilter <DiffusionCalculatorImageType>	VtkToItkConverterType;//3D
	typedef itk::ImageToVTKImageFilter < DiffusionCalculatorImageType> itkToVtkConverter;

	typedef itk::CastImageFilter< DiffusionCalculatorImageType, DiffusionCalculatorImageType >	CastFilterType;
	typedef itk::VectorContainer< SourceImagePixelType, DiffusionCalculatorImageType::Pointer > ImageContainerType;
	typedef itk::ComposeImageFilter<DiffusionCalculatorImageType>		ImageToVectorImageType;

	//store b0 images (all slices)
	ImageContainerType::Pointer imageContainer = ImageContainerType::New();
	imageContainer->Reserve(imageDimensions[2]);

	double fwhm = 0.10; //1.25
	if (imageCalculationType == DKI)
		fwhm = 1.0;
	double gauss_std = fwhm / sqrt(8 * log(2));
	//Append multiple components for vtk image data
	vtkSmartPointer<vtkImageAppendComponents> imageAppend = vtkSmartPointer<vtkImageAppendComponents>::New();
	//Use Otsu median filter according to config File: Ming
	vtkDICOMMetaData* metaData = vtkDICOMMetaData::New();
	metaData = DicomReader->GetMetaData();
	QString Body = QString::fromLatin1(metaData->GetAttributeValue(0, 0, vtkDICOMTag(0x0018, 0x0015)).AsString().c_str());

	bool useOtsu(false);
	if (OTSU_OPT <= 2 && OTSU_OPT > 0)
	{
		useOtsu = (OTSU_OPT == 2 || Body.contains("HEAD", Qt::CaseInsensitive) || Body.contains("BRAIN", Qt::CaseInsensitive));
	}

	skullMask = ITK_NULLPTR;
	for (int i = 0; i < numberOfComponents; i++)
	{
		//Handle each scalar component 
		vtkSmartPointer <vtkImageExtractComponents> scalarComponent = vtkSmartPointer <vtkImageExtractComponents>::New();
		scalarComponent->SetInputData(imageData);
		scalarComponent->SetComponents(i);
		scalarComponent->Update();

		//VTK to ITK Image Data
		VtkToItkConverterType::Pointer vtkToItkImageFilter = VtkToItkConverterType::New();
		vtkToItkImageFilter->SetInput(scalarComponent->GetOutput());
		vtkToItkImageFilter->Update();

		//unsigned short image to float image
		CastFilterType::Pointer castFilter = CastFilterType::New();
		castFilter->SetInput(vtkToItkImageFilter->GetOutput());
		castFilter->Update();		

		DiffusionCalculatorImageType::Pointer maskedImage = castFilter->GetOutput();
		maskedImage->DisconnectPipeline();
	

		itkToVtkConverter::Pointer convItkToVtk = itkToVtkConverter::New();

		convItkToVtk->SetInput(maskedImage);

		if (i == 0)
		{
			imageContainer->SetElement(0, maskedImage);
		}

		convItkToVtk->Update();
		vtkSmartPointer <vtkImageShiftScale> scaleVolumeImage = vtkSmartPointer <vtkImageShiftScale>::New();
		scaleVolumeImage->SetInputData(convItkToVtk->GetOutput());
		scaleVolumeImage->SetShift(0);
		scaleVolumeImage->SetScale(1);
		scaleVolumeImage->Update();


		if (i == 0)
			imageAppend->SetInputConnection(scaleVolumeImage->GetOutputPort());
		else
			imageAppend->AddInputConnection(scaleVolumeImage->GetOutputPort());

		int prog = round(i*1.0 / (numberOfComponents - 1) * 100);
	}

	imageAppend->Update();
	imageData->DeepCopy(imageAppend->GetOutput());
}

vtkSmartPointer<vtkDICOMMetaData> DicomHelper::GetMetaDataFromNthSeries(int seriesID)
{
	vtkSmartPointer <vtkDICOMFileSorter> sorter =
		vtkSmartPointer <vtkDICOMFileSorter>::New();
	// Provide an array containing a list of filenames.
	sorter->SetInputFileNames(m_InputFileNames);
	// Update the sorter (i.e. perform the sort).
	sorter->Update();
	// Get the first series.
	int numberOfSeries = sorter->GetNumberOfSeries();

	if (numberOfSeries < 1) return NULL;

	seriesID = seriesID < 0 ? 0 : seriesID;
	seriesID = seriesID > numberOfSeries - 1 ? numberOfSeries - 1 : seriesID;

	vtkStringArray * nthSeriesFiles = sorter->GetFileNamesForSeries(seriesID);
	if (!nthSeriesFiles) return NULL;

	vtkSmartPointer <vtkDICOMReader> reader =
		vtkSmartPointer <vtkDICOMReader>::New();
	// Provide a vtkStringArray containing a list of filenames.
	reader->SetFileNames(nthSeriesFiles);
	// Read the meta data via UpdateInformation()
	reader->TimeAsVectorOn();
	reader->AutoRescaleOff();
	reader->UpdateInformation();
	vtkSmartPointer<vtkDICOMMetaData> nthSeriesMetaData = vtkSmartPointer<vtkDICOMMetaData>::New();
	//firstSeriesMetaData->DeepCopy(reader->GetMetaData());
	nthSeriesMetaData = reader->GetMetaData();

	this->SortMetaData(nthSeriesMetaData);

	return nthSeriesMetaData;
}

//????: temp fix file index here for dicom write, frame index not fixed
void DicomHelper::SortMetaData(vtkDICOMMetaData * source)
{
	if (source->HasAttribute(DC::PerFrameFunctionalGroupsSequence))
	{
		std::cout << "Enhanced Multi-Frame MR Meta Data is not sorted for now." << std::endl;
		return;
	}

	if (!source->GetFileIndexArray()) return;

	if (this->sortingNeed && (manuFacturer[0] == 'G' || manuFacturer[0] == 'g'))
	{
		vtkIntArray * fileMap = source->GetFileIndexArray();
		int components = fileMap->GetNumberOfComponents();
		int slices = fileMap->GetNumberOfTuples();

		vtkIntArray * tmpFileArray = vtkIntArray::New();
		tmpFileArray->SetNumberOfComponents(components);
		tmpFileArray->SetNumberOfTuples(slices);

		for (int i = 0; i < slices; i++)
		{
			for (int j = 0; j < components; j++)
			{
				int fileIndex = fileMap->GetComponent(i, components -1 - j);
				tmpFileArray->SetComponent(i, j, fileIndex);
			}
		}
		source->SetFileIndexArray(tmpFileArray);
		tmpFileArray->Delete();		
	}
}

void DicomHelper::CorrectPHLGradientDirection(vtkDICOMMetaData* metaData)
{
	vtkDICOMTag ImagingOrientaion2Patient = vtkDICOMTag(0x0020, 0x0037);

	std::string  tmp = metaData->GetAttributeValue(0, 0, ImagingOrientaion2Patient).AsString();
	QString Img2PatQ = QString::fromStdString(tmp);
	QStringList Img2PatQL = Img2PatQ.split('\\');
	//float Img2Pat[6];
	double readV[3];
	double phaseV[3];
	double sliceV[3];
	for (int i = 0; i<3; i++)
	{
		readV[i] = Img2PatQL.at(i).toDouble();
		phaseV[i] = Img2PatQL.at(i + 3).toDouble();
	}
	sliceV[0] = readV[1] * phaseV[2] - readV[2] * phaseV[1];
	sliceV[1] = readV[0] * phaseV[2] - readV[2] * phaseV[0];
	sliceV[2] = readV[0] * phaseV[1] - readV[1] * phaseV[0];
	double norm = sliceV[0] * sliceV[0] + sliceV[1] * sliceV[1] + sliceV[2] * sliceV[2];
	sliceV[0] = sliceV[0] / norm;
	sliceV[1] = sliceV[1] / norm;
	sliceV[2] = sliceV[2] / norm;
	for (int i = 0; i < numberOfGradDirection; i++)
	{
		GradientDirectionType oldD = this->gradientDirection->GetElement(i);
		GradientDirectionType newD;
		newD[0] = readV[0] * oldD[1] + readV[1] * oldD[0] + readV[2] * oldD[2];
		newD[1] = phaseV[0] * oldD[1] + phaseV[1] * oldD[0] + phaseV[2] * oldD[2];
		newD[2] = sliceV[0] * oldD[1] + sliceV[1] * oldD[0] + sliceV[2] * oldD[2];
		if (newD.GetNorm() > 0)
		{
			newD = newD / newD.GetNorm();
		}
		qDebug() << "old direction:" << oldD[0] << " " << oldD[1] << " " << oldD[2] << endl;
		qDebug() << "new direction:" << newD[0] << " " << newD[1] << " " << newD[2] << endl;
		this->gradientDirection->SetElement(i, newD);
	}
}

void DicomHelper::DecodeCSAHeader(CSAHeader &header, const char* info)
{
	//
	// the reference used to write this code is here:
	// http://nipy.sourceforge.net/nibabel/dicom/siemens_csa.html
	
	//const char *info = infoString.c_str();

	const bool isCSA2 = info[0] == 'S' && info[1] == 'V'
		&& info[2] == '1' && info[3] == '0';
	unsigned int offset;

	if (isCSA2)
	{
		offset = 8; // past SV10 + unused 4 bytes
	}
	else
	{
		offset = 0;
	}
	const itk::uint32_t numberOfTags =
		this->CSAExtractFromString<itk::uint32_t>(info + offset);
	offset += sizeof(itk::uint32_t); // skip numberOfTags;
	offset += sizeof(itk::uint32_t); // skip unused2

	//std::cout << "[DicomHelper] number of CSA Tags: " << numberOfTags << endl;

	for (unsigned i = 0; i < numberOfTags; ++i)
	{
		// tag name is 64 bytes null terminated.
		std::string tagName = info + offset;
		offset += 64;                        // skip tag name
		itk::uint32_t vm = this->CSAExtractFromString<itk::uint32_t>(info + offset);
		offset += sizeof(itk::uint32_t);

		CSAItem current(vm);
		current.vm = vm;

		// vr = 3 bytes of string + 1 for pad
		char vr[4];
		for (unsigned j = 0; j < 3; ++j)
		{
			vr[j] = info[offset + j];
		}
		vr[3] = '\0';
		current.vr = vr;
		offset += 4; // after VR
		offset += 4; // skip syngodt

		const itk::int32_t nItems =
			this->CSAExtractFromString<itk::int32_t>(info + offset);
		offset += 4;
		offset += 4; // skip xx

		for (int j = 0; j < nItems; ++j)
		{
			// 4 items in XX, first being item length
			const itk::int32_t  itemLength =
				this->CSAExtractFromString<itk::int32_t>(info + offset);
			offset += 16;
			std::string valueString;
			valueString = info + offset;
			offset += itemLength;
			while ((offset % 4) != 0)
			{
				++offset;
			}
			if (j < static_cast<int>(vm))
			{
				current[j] = valueString;
			}
		}
		header[tagName] = current;
	}
}

bool DicomHelper::ExtractSiemensBMatrix(CSAHeader *csaHeader, unsigned int strideVolume,
	vnl_matrix_fixed<double, 3, 3> &bMatrix)
{
	std::vector<double> valueArray;
	CSAHeader::const_iterator csaIt;


	if ((csaIt = csaHeader->find("B_matrix")) == csaHeader->end() ||
		(valueArray = csaIt->second.AsVector<double>()).size() != 6)
	{
		qDebug() << "[DicomHelper] Siemens DWI: Not found B_Matrix" << endl;
		return false;
	}


	// UNC comments: Fill out the 3x3 bmatrix with the 6 components read from the
	// DICOM header.
	bMatrix[0][0] = valueArray[0];
	bMatrix[0][1] = valueArray[1];
	bMatrix[0][2] = valueArray[2];
	bMatrix[1][1] = valueArray[3];
	bMatrix[1][2] = valueArray[4];
	bMatrix[2][2] = valueArray[5];
	bMatrix[1][0] = bMatrix[0][1];
	bMatrix[2][0] = bMatrix[0][2];
	bMatrix[2][1] = bMatrix[1][2];
	return true;
}

void DicomHelper::SetPerfusionCategory()
{
	QMessageBox* tip = new QMessageBox(this);
	tip->setWindowTitle(tr("Perfusion type"));
	tip->setText(tr("Dynamic data imported, treat it as: "));//自动判断DICOM为灌注增强数据, 下面进行那种类型分析
															 //tip->setInformativeText(tr("请选择："));
	tip->addButton(tr("DCE"), QMessageBox::AcceptRole);
	tip->addButton(tr("DSC"), QMessageBox::AcceptRole);//磁敏感增强分析
	tip->addButton(tr("Neither"), QMessageBox::AcceptRole);//都不是
	int ret = tip->exec();
	if (ret == 0)
	{
		imageDataType = "DCE";
		this->_GenerateImageData(this->imageData, &this->dynamicTime, false, 1, nullptr);//the third parameter used for diffusion gradient direction number
	}
	else if (ret == 1)
	{
		imageDataType = "DSC";
		this->_GenerateImageData(this->imageData, &this->dynamicTime, false, 1, nullptr);//the third parameter used for diffusion gradient direction number
	}
	else
	{
		imageDataType = "OTHERS";
		this->_GenerateImageData(this->imageData, nullptr, false, 1, nullptr);

	}
}

void DicomHelper::GetIJKToRASMatrix(vtkMatrix4x4* mat)
{
	// this is the full matrix including the spacing and origin
	mat->Identity();

	int row, col;
	for (row = 0; row<3; row++)
	{
		for (col = 0; col<3; col++)
		{
			mat->SetElement(row, col, DicomReader->GetOutput()->GetSpacing()[col] * m_MatrixPat->GetElement(row, col));
		}
		mat->SetElement(row, 3, m_MatrixPat->GetElement(row, 3));
	}
}

void DicomHelper::SiemensDeMosaic()
{
	/** Not Implemented */
}