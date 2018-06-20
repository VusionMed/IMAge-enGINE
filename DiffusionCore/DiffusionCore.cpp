
//This Pack
#include "DiffusionCore.h"
#include "ui_DiffusionCore.h"
#include "DicomHelper.h"

#include "time.h"
#include "SharedFunctions.h"
#include "vtkDICOMVUSIONGenerator.h"

#include "itkCommand.h"


// VTK
#include <vtkDataObjectToTable.h>
#include <vtkElevationFilter.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkCamera.h>
#include <vtkCornerAnnotation.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkImageMapper3D.h>
#include <vtkImageProperty.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>
#include <vtkImageIterator.h>
#include <vtkImageViewer2.h>
#include <vtkPicker.h>
#include <vtkAssemblyPath.h>
#include <vtkContourWidget.h>
#include <vtkMedicalImageProperties.h>
#include <vtkWindowToImageFilter.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkVectorText.h>
#include <vtkStringArray.h>
#include <vtkRendererCollection.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkPNGWriter.h>
#include <vtkExtractVOI.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageAppendComponents.h>
#include <vtkScalarBarActor.h>
#include <vtkQtTableView.h>
#include <QVTKWidget.h>
#include <QVTKInteractor.h>
#include <vtkImageShiftScale.h>
#include <vtkImageAppend.h>
#include <vtkDICOMMRGenerator.h>
#include <vtkDICOMWriter.h>
#include <vtkImageWeightedSum.h>

#include <vtkImageActorPointPlacer.h>
// ITK
#include <itkCastImageFilter.h>
#include <itkComposeImageFilter.h>
#include <itkShiftScaleImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkVTKImageToImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkVectorIndexSelectionCastImageFilter.h>

#include <itkTensorCDwiImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkComputedAdcFilter.h>
#include <itkAdcMapFilter.h>
#include <itkComputedDwiFilter.h>
#include <itkComputedEadcFilter.h>
#include <itkMaskVectorImageFilter.h>
#include <itkGetDiffusionImageFilter.h>
#include <itkTensor.h>

// Qt
#include <QCheckBox>
#include <QMessageBox>
#include <QDialog>
#include <QMouseEvent>
#include <QGroupBox>
#include <QProgressDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <qdatetime.h>
#include "qsettings.h"
//#include <qbuttongroup.h>

//standard


class FullImageLabelTable
{
public:
	FullImageLabelTable()
	{
		m_ImageLabelTable.push_back("Source");//0
		m_ImageLabelTable.push_back("ADC");//1
		m_ImageLabelTable.push_back("eADC");//2
		m_ImageLabelTable.push_back("cDWI");//3
		m_ImageLabelTable.push_back("IVIM");//4
		m_ImageLabelTable.push_back("SEM");//5
		m_ImageLabelTable.push_back("FA");//6
		m_ImageLabelTable.push_back("cFA");//7
		m_ImageLabelTable.push_back("AK");//8
		m_ImageLabelTable.push_back("RK");//9
		m_ImageLabelTable.push_back("MK");//10
		m_ImageLabelTable.push_back("AD");//11
		m_ImageLabelTable.push_back("RD");//12
		m_ImageLabelTable.push_back("sDKI");//13
	}
	~FullImageLabelTable() {}

	int GetImageLabelIndex(const QString &str)
	{
		for (int i = 0; i < m_ImageLabelTable.size(); i++)
		{
			if (!QString::compare(m_ImageLabelTable.at(i), str, Qt::CaseInsensitive)) // 0 is equal
			//if(str.contains(m_ImageLabelTable.at(i)))
			{
				return i;
			}
		}

		return -1;
	}

	void RemoveAndRenameRedundantStr(QString str, QList<QString> & list)
	{
		if (list.isEmpty()) return;

		//qDebug() << "Before remove" << list << endl;

		QList<unsigned int> targetIndex;
		for (int i = 0; i < list.size(); i++)
		{
			if (list.at(i).contains(str))
				targetIndex.push_back(i);
		}

		//first remove redundant
		if (targetIndex.size() > 1)
		{
			for (int i = targetIndex.size() - 1; i > 0; i--)
			{
				list.removeAt(targetIndex.at(i));
			}
		}

		//second rename str
		if (!targetIndex.isEmpty())
		{
			list[targetIndex.at(0)] = str;
		}

	}
protected:
	QList<QString> m_ImageLabelTable;
};



//TRY SetNumberOfThreads(1) to solve the multi-thread ranmdom results
//Notify Wenxing
#define DEFAULTTHRESH 10
#define DEFAULTB 2000
#define BUTTON_GROUP_ID 300

DiffusionCore::DiffusionCore(QWidget *parent)
	:QWidget(parent)
	//, dicomHelp(nullptr)
{	
	//Setting configuration variables
	QSettings setting("MR_Config.ini", QSettings::IniFormat);

	NODDI_MED_B = setting.value("NODDI_MED_B").toInt();
	NODDI_MIN_B = setting.value("NODDI_MIN_B").toInt();

	D_BVAL_THRESH = setting.value("D_BVAL_THRESH").toInt();
	Dstar_BVAL_THRESH = setting.value("Dstar_BVAL_THRESH").toInt();

	SDKI_LOW_B_MIN = setting.value("SDKI_LOW_B_MIN").toInt();
	SDKI_LOW_B_MAX = setting.value("SDKI_LOW_B_MAX").toInt();
	SDKI_HIGH_B_MIN = setting.value("SDKI_HIGH_B_MIN").toInt();
	SDKI_HIGH_B_MAX = setting.value("SDKI_HIGH_B_MAX").toInt();

	MIN_KURTOSIS = setting.value("MIN_KURTOSIS").toInt();
	MAX_KURTOSIS = setting.value("MAX_KURTOSIS").toInt();

	//qDebug() << "Constant from Setting is " << NODDI_MED_B << NODDI_MIN_B << D_BVAL_THRESH << F_THRESH << endl;

	this->m_Controls = nullptr;	
	this->ButtonTable = new QButtonGroup;
	this->m_MaskVectorImage = DiffusionCalculatorVectorImageType::New();
	this->m_ColorFaImage = RGBImageType::New();
	m_MaskThreshold = DEFAULTTHRESH/5;
	m_ComputedBValue = DEFAULTB;
	
	CreateQtPartControl(this);
}

DiffusionCore::~DiffusionCore()
{
	//delete m_Controls;
	//this->m_Controls = NULL;
	//this->m_MaskVectorImage->Delete();
	this->m_MaskVectorImage = ITK_NULLPTR;
}

void DiffusionCore::CreateQtPartControl(QWidget *parent)
{
	if (!m_Controls)
	{				
		this->m_Controls = new Ui::DiffusionModule;
		this->m_Controls->setupUi(parent);		

		m_Controls->ThreshSlider->setMaximum(100); //maximum threshhold value;
		m_Controls->ThreshSlider->setMinimum(1);//DTI bug while caculating ADC EADC, @Wenxing
		m_Controls->ThreshSlider->setDecimals(0);
		m_Controls->ThreshSlider->setSingleStep(1);
		m_Controls->ThreshSlider->setTickInterval(1);
		m_Controls->ThreshSlider->setValue(DEFAULTTHRESH);
		m_Controls->ThreshSlider->setTracking(false);
		//connect Buttons and handle visibility

		this->m_Controls->ADCTool->setDisabled(true);
		this->m_Controls->DTITool->setDisabled(true);
		this->m_Controls->ivimToggle->setDisabled(true);
		this->m_Controls->semToggle->setVisible(false);
		this->m_Controls->DKITool->setDisabled(true);
		this->m_Controls->noddiButton->setDisabled(true);
		this->m_Controls->sdkiToggle->setDisabled(true);

		ButtonTable->setExclusive(false); //non-exclusive button group. 
		ButtonTable->addButton(m_Controls->adcToggle, BUTTON_GROUP_ID + 1);
		ButtonTable->addButton(m_Controls->eadcToggle, BUTTON_GROUP_ID + 2);
		ButtonTable->addButton(m_Controls->cdwiToggle, BUTTON_GROUP_ID + 3);
		ButtonTable->addButton(m_Controls->faToggle, BUTTON_GROUP_ID + 4);
		ButtonTable->addButton(m_Controls->colorFAToggle, BUTTON_GROUP_ID + 5);
		ButtonTable->addButton(m_Controls->ivimToggle, BUTTON_GROUP_ID + 6);
		ButtonTable->addButton(m_Controls->mkToggle, BUTTON_GROUP_ID + 7);
		ButtonTable->addButton(m_Controls->rkToggle, BUTTON_GROUP_ID + 8);
		ButtonTable->addButton(m_Controls->akToggle, BUTTON_GROUP_ID + 9);
		ButtonTable->addButton(m_Controls->rdToggle, BUTTON_GROUP_ID + 10);
		ButtonTable->addButton(m_Controls->adToggle, BUTTON_GROUP_ID + 11);
		ButtonTable->addButton(m_Controls->sdkiToggle, BUTTON_GROUP_ID + 12);
		//Connect Toggles
		connect(m_Controls->adcToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcADC(bool)));
		connect(m_Controls->eadcToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcEADC(bool)));
		connect(m_Controls->cdwiToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcCDWI(bool)));
		connect(m_Controls->faToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcFA(bool)));
		connect(m_Controls->colorFAToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcColorFA(bool)));
		connect(m_Controls->ivimToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcIVIM(bool)));
		connect(m_Controls->mkToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcMK(bool)));
		connect(m_Controls->rkToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcRK(bool)));
		connect(m_Controls->akToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcAK(bool)));
		connect(m_Controls->adToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcAD(bool)));
		connect(m_Controls->rdToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcRD(bool)));
		connect(m_Controls->sdkiToggle, SIGNAL(toggled(bool)), this, SLOT(onCalcSDKI(bool)));

		//Connect buttons triggering long-time calculation
		connect(m_Controls->noddiButton, SIGNAL(clicked()), this, SLOT(onCalcNODDI()));

		//Connect Sliders
		connect(m_Controls->ThreshSlider, SIGNAL(valueChanged(double)), this, SLOT(onThreshSlide(double)));
		
		m_Controls->Thresh->hide();		
	}
}

void DiffusionCore::onSetSourceImage(DicomHelper* dicomData)
{
	//Reset all button;
	for (int i = BUTTON_GROUP_ID + 1; i < BUTTON_GROUP_ID + 12; i++)
	{
		if (ButtonTable->button(i)->isChecked())
		{
			ButtonTable->button(i)->setChecked(false);
		}
	}

	m_Controls->Thresh->hide();


	m_DicomHelper = dicomData;

	std::cout << m_DicomHelper->imageDataType;

	if (m_DicomHelper->imageDataType != "DIFFUSION")
	{
		qDebug() << "not DIFFUSION" << endl;
		//Disable all the buttons
		this->m_Controls->ADCTool->setEnabled(false);
		this->m_Controls->ivimToggle->setEnabled(false);
		this->m_Controls->DTITool->setEnabled(false);
		this->m_Controls->DKITool->setEnabled(false);
		this->m_Controls->sdkiToggle->setEnabled(false);
		return;
	}		
	
	switch (m_DicomHelper->imageCalculationType)
	{
	case 0:
		qDebug() << "[SetSourceImage] DWI OK" << endl;
		this->m_Controls->ADCTool->setEnabled(true);
		this->m_Controls->DTITool->setEnabled(false);
		this->m_Controls->ivimToggle->setEnabled(false);
		this->m_Controls->DKITool->setEnabled(false);
		break;

	case 1:
		qDebug() << "[SetSourceImage] IVIM OK" << endl;
		this->m_Controls->ADCTool->setEnabled(false);
		this->m_Controls->ivimToggle->setEnabled(true);
		this->m_Controls->DTITool->setEnabled(false);
		this->m_Controls->DKITool->setEnabled(false);

		//enable sdki
		{
			bool enableSDKIDapp = false;
			bool enableSDKIKapp = false;
			for (int i = 0; i < m_DicomHelper->BvalueList.size(); i++)
			{
				if (!enableSDKIDapp)
				{
					if ((m_DicomHelper->BvalueList.at(i) > SDKI_LOW_B_MIN) &&
						(m_DicomHelper->BvalueList.at(i) < SDKI_LOW_B_MAX)
						)
					{
						enableSDKIDapp = true;
					}
				}
				else
				{
					if ((m_DicomHelper->BvalueList.at(i) > SDKI_HIGH_B_MIN) &&
						(m_DicomHelper->BvalueList.at(i) < SDKI_HIGH_B_MAX)
						)
					{
						enableSDKIKapp = true;
						break;
					}
				}
			}

			this->m_Controls->sdkiToggle->setEnabled((enableSDKIDapp && enableSDKIKapp));
		}
		//enable ADCTool
		this->m_Controls->ADCTool->setEnabled(true);
		break;
	case 2:
		qDebug() << "[SetSourceImage] DTI OK" << endl;
		this->m_Controls->ADCTool->setEnabled(true);
		this->m_Controls->ivimToggle->setEnabled(false);
		this->m_Controls->DTITool->setEnabled(true);
		this->m_Controls->DKITool->setEnabled(false);
		break;
	case 3:
		qDebug() << "[SetSourceImage] DKI OK" << endl;
		this->m_Controls->ADCTool->setEnabled(true);
		this->m_Controls->ivimToggle->setEnabled(false);
		this->m_Controls->DTITool->setEnabled(true);
		this->m_Controls->DKITool->setEnabled(true);
	default:
		break;
	}

	onRecalcAll();
}

void DiffusionCore::onCalc3D(QString directory, const QList<QString> & selectedWindow, int type)
{
	if (directory.isEmpty() && type == 0)
	{
		qDebug() << "Please select a valid folder to save the dicom images!" << endl;
		return;
	}

	if (selectedWindow.empty())
	{
		qDebug() << "Please select at one window!!!" << endl;
		return;
	}

	QList<QString> tmpWindow = selectedWindow;
	FullImageLabelTable labelTable;
	labelTable.RemoveAndRenameRedundantStr("IVIM", tmpWindow);//f, DStar, D
	labelTable.RemoveAndRenameRedundantStr("sDKI", tmpWindow);//Dapp and Kapp
	QDateTime time = QDateTime::currentDateTime();
	QString timestr = time.toString("yyyyMMdd-hhmmss-");

	for (int windowIndex = 0; windowIndex < tmpWindow.size(); ++windowIndex)
	{
		int componentsToWrite = 1;
		bool splitComponents = false;
		vtkSmartPointer<vtkImageAppend> appendImageData = vtkSmartPointer<vtkImageAppend>::New();

		emit signalSaveDcmUpdate(tr("Saving %1 image out of %2 ").arg(windowIndex + 1).arg(tmpWindow.size()), 0);
		for (int sliceIndex = 0; sliceIndex < m_DicomHelper->imageDimensions[2]; sliceIndex++)
		{
			this->m_DicomHelper->ComputeCurrentSourceImage(sliceIndex);
			this->PreCompute();

			vtkSmartPointer<vtkImageData> sliceImage = vtkSmartPointer<vtkImageData>::New();
			float slope = 1;
			float intercept = 0;

			switch (labelTable.GetImageLabelIndex(tmpWindow.at(windowIndex)))
			{
			case 0://source
				sliceImage->DeepCopy(m_DicomHelper->sourceImageSlice);
				break;
			case 1://Adc

				this->AdcCalculator(sliceImage, slope, intercept);
				break;
			case 2://eAdc

				this->EAdcCalculator(sliceImage, slope, intercept);
				break;
			case 3://cDwi
				   //DEVELOPER NOTICE:
				   //IMPLEMENT YOUR FUNCTION HERE
				   //USING onCalcADC as an example
				break;
			case 4: //IVIM

				//DEVELOPER NOTICE:
				//IMPLEMENT YOUR FUNCTION HERE
				//USING onCalcADC as an example
				break;
			case 5://SEM
				   //DEVELOPER NOTICE:
				   //IMPLEMENT YOUR FUNCTION HERE
				   //USING onCalcADC as an example
				break;
			case 6://FA
				   //DEVELOPER NOTICE:
				   //IMPLEMENT YOUR FUNCTION HERE
				   //USING onCalcADC as an example
				break;
			case 7://cFA

				//DEVELOPER NOTICE:
				//IMPLEMENT YOUR FUNCTION HERE
				//USING onCalcADC as an example
				break;

			case 8://AK
				   //DEVELOPER NOTICE:
				   //IMPLEMENT YOUR FUNCTION HERE
				   //USING onCalcADC as an example
				break;
			case 9://RK
				   //DEVELOPER NOTICE:
				   //IMPLEMENT YOUR FUNCTION HERE
				   //USING onCalcADC as an example
				break;
			case 10://MK
					//DEVELOPER NOTICE:
					//IMPLEMENT YOUR FUNCTION HERE
					//USING onCalcADC as an example
				break;
			case 11://AD
					//DEVELOPER NOTICE:
					//IMPLEMENT YOUR FUNCTION HERE
					//USING onCalcADC as an example
				break;
			case 12://RD
					//DEVELOPER NOTICE:
					//IMPLEMENT YOUR FUNCTION HERE
					//USING onCalcADC as an example
				break;
			case 13://sDKI
				//DEVELOPER NOTICE:
				//IMPLEMENT YOUR FUNCTION HERE
				//USING onCalcADC as an example
				break;
			case 14://left here for future use
				break;
			default:
				std::cout << "not a valid image type, end of writing" << std::endl;
				return;
				//break;
			}
			//scale sliceImage to real value
			vtkSmartPointer <vtkImageShiftScale> scaleSliceImage = vtkSmartPointer <vtkImageShiftScale>::New();
			scaleSliceImage->SetInputData(sliceImage);
			scaleSliceImage->SetShift(intercept / slope);
			scaleSliceImage->SetScale(slope);
			scaleSliceImage->ClampOverflowOn();
			scaleSliceImage->SetOutputScalarTypeToFloat();
			scaleSliceImage->Update();
			appendImageData->AddInputConnection(scaleSliceImage->GetOutputPort());

			int prog = floor((sliceIndex*1.0 / (m_DicomHelper->imageDimensions[2] - 1)) * 80.0);
			//qDebug() << "calculating slice: " << sliceIndex <<" of "<< m_DicomHelper->imageDimensions[2] - 1 << "progress is " << prog << endl;
			emit signalSaveDcmUpdate(tr("Saving %1 image out of %2").arg(windowIndex + 1).arg(tmpWindow.size()), prog);
		}//end of slice loop

		//composeImage
		appendImageData->SetAppendAxis(2);
		appendImageData->Update();//it holds real world value

		vusion::PrintVTKImageDataInfo(appendImageData->GetOutput());

		if (splitComponents) componentsToWrite = appendImageData->GetOutput()->GetNumberOfScalarComponents();

		for (int dicomWriteIndex = 0; dicomWriteIndex < componentsToWrite; dicomWriteIndex++)
		{
			QString imageNm(timestr + tmpWindow.at(windowIndex));

			if (!QString::compare(tmpWindow.at(windowIndex), QString("cdwi"), Qt::CaseInsensitive))//0 is equal
			{
				imageNm = imageNm + QString("-bValue-") + QString::number(m_ComputedBValue) + QString("-");
			}

			vtkSmartPointer<vtkImageData> imageDataToWrite = NULL;
			if (splitComponents)
			{
				vtkSmartPointer<vtkImageExtractComponents> extractComponentsFilter = vtkSmartPointer<vtkImageExtractComponents>::New();
				extractComponentsFilter->SetInputConnection(appendImageData->GetOutputPort());
				extractComponentsFilter->SetComponents(dicomWriteIndex);
				extractComponentsFilter->Update();
				imageDataToWrite = extractComponentsFilter->GetOutput();
				imageNm = imageNm + QString("-Component-") + QString::number(dicomWriteIndex + 1) + QString("-");
			}
			else
			{
				imageDataToWrite = appendImageData->GetOutput();
			}



			double range[2];
			imageDataToWrite->GetScalarRange(range);
			double rescaleSlope = range[1] > range[0] ? 4095 / (range[1] - range[0]) : 1;
			double rescaleIntercept = -range[0];

			qDebug() << "[Output] imageData to write ranged from " << range[0] << " to " << range[1] << endl;

			vtkSmartPointer <vtkImageShiftScale> scaleVolumeImage = vtkSmartPointer <vtkImageShiftScale>::New();
			scaleVolumeImage->SetInputData(imageDataToWrite);
			scaleVolumeImage->SetShift(rescaleIntercept);
			scaleVolumeImage->SetScale(rescaleSlope);
			scaleVolumeImage->ClampOverflowOn();
			scaleVolumeImage->SetOutputScalarTypeToUnsignedShort();//Change to double type in future?
			scaleVolumeImage->Update();


			switch (type)
			{
			case 0://DICOM
			{
				vtkSmartPointer <vtkDICOMVUSIONGenerator> generator = vtkSmartPointer <vtkDICOMVUSIONGenerator>::New();
				vtkSmartPointer <vtkDICOMMetaData> metaData = m_DicomHelper->GetMetaDataFromNthSeries(0); //vtkSmartPointer <vtkDICOMMetaData>::New();

				if (!labelTable.GetImageLabelIndex(tmpWindow.at(windowIndex)))//source
				{
					generator->SetUseSourcePrivateTagOn();
				}
				generator->SetPatientMatrix(m_DicomHelper->Get4x4Matrix());

				double ww = (range[1] - range[0]);
				double wl = (range[1] - range[0]) / 2;

				metaData->SetAttributeValue(DC::SeriesDescription, imageNm.toStdString());
				metaData->SetAttributeValue(DC::ProtocolName, imageNm.toStdString());
				metaData->SetAttributeValue(DC::RescaleIntercept, rescaleIntercept / rescaleSlope);
				metaData->SetAttributeValue(DC::RescaleSlope, 1.0 / rescaleSlope);
				//metaData->SetAttributeValue(DC::Manufacturer, "Vusion");
				metaData->SetAttributeValue(DC::WindowCenter, wl);
				metaData->SetAttributeValue(DC::WindowWidth, ww);

				std::string fileName = "%s\\IM-" + imageNm.toStdString() + "-000%d.dcm";
				vtkSmartPointer <vtkDICOMWriter> writer = vtkSmartPointer <vtkDICOMWriter>::New();
				writer->SetInputConnection(scaleVolumeImage->GetOutputPort());
				writer->SetMetaData(metaData);
				writer->SetGenerator(generator);
				writer->SetFileSliceOrder(2);
				writer->SetFilePattern(fileName.c_str());
				writer->SetPatientMatrix(m_DicomHelper->Get4x4Matrix());
				writer->SetMemoryRowOrderToFileNative();
				//writer->SetFileSliceOrderToReverse();
				writer->SetFilePrefix(directory.toStdString().c_str());
				writer->Write();
				break;
			}
			}

		}
	}

	emit signalSaveDcmUpdate(tr("Finished"), 100);

}

void DiffusionCore::onCalcADC(bool _istoggled) //SLOT of adcToggle
{
	double start = GetTickCount();
	vtkSmartPointer <vtkImageData> calculatedAdc;
	const QString imageName(this->m_Controls->adcToggle->text());
	float scale(1.0), slope(0.0);

	if (_istoggled)
	{
		this->m_Controls->Thresh->setVisible(_istoggled);
		calculatedAdc = vtkSmartPointer <vtkImageData>::New();
		if (m_DicomHelper->imageCalculationType == DicomHelper::DWI || DicomHelper::IVIM)
		{
			this->AdcCalculator(calculatedAdc, scale, slope);
		}else{
			return;			
		}
	}
	else
	{
		Diff_ActiveWdw.removeOne(BUTTON_GROUP_ID + 1);
		calculatedAdc = NULL;
	}
	emit SignalTestButtonFired(_istoggled, calculatedAdc, imageName, scale, slope);
	double end = GetTickCount();
	qDebug() << "ADC calculation time: " << end - start;
}

void DiffusionCore::AdcCalculator(vtkSmartPointer <vtkImageData> imageData, float& slope, float& intercept)
{
	if (!this->m_MaskVectorImage) return;

	typedef itk::ComputedAdcFilter <DiffusionCalculatorPixelType, DiffusionCalculatorPixelType> AdcMapFilterType;
	AdcMapFilterType::Pointer adcMap = AdcMapFilterType::New();
	adcMap->SetInput(this->m_MaskVectorImage);
	adcMap->SetBValueList(this->m_DicomHelper->BvalueList);
	adcMap->SetNumOfDiffDirections(this->m_DicomHelper->numberOfGradDirection);
	adcMap->SetNumberOfThreads(1);
	adcMap->Update();

	//Rescale signal intensity to display
	typedef itk::RescaleIntensityImageFilter < DiffusionCalculatorImageType, SourceImageType> RescaleIntensityImageType;
	RescaleIntensityImageType::Pointer rescaleFilter = RescaleIntensityImageType::New();
	rescaleFilter->SetInput(adcMap->GetOutput());
	rescaleFilter->SetOutputMaximum(4095);
	rescaleFilter->SetOutputMinimum(0);
	rescaleFilter->Update();

	slope = 1 / rescaleFilter->GetScale();
	intercept = -1 * rescaleFilter->GetShift() / rescaleFilter->GetScale();

	///////////////////////////////////////////
	//ITK to VTK
	typedef itk::ImageToVTKImageFilter <SourceImageType> itkToVtkConverter;
	itkToVtkConverter::Pointer convItkToVtk = itkToVtkConverter::New();
	convItkToVtk->SetInput(rescaleFilter->GetOutput());
	convItkToVtk->Update();

	imageData->DeepCopy(convItkToVtk->GetOutput());
}

void DiffusionCore::onCalcEADC(bool _istoggled) //SLOT of eadcToggle
{
	vtkSmartPointer <vtkImageData> calculatedEAdc;
	const QString imageName(this->m_Controls->eadcToggle->text());
	float scale(1.0), slope(0.0);

	if (_istoggled)
	{
		this->m_Controls->Thresh->setVisible(_istoggled);

		calculatedEAdc = vtkSmartPointer <vtkImageData>::New();

		if (m_DicomHelper->imageCalculationType == DicomHelper::DWI || DicomHelper::IVIM)
		{
			this->EAdcCalculator(calculatedEAdc, scale, slope);
		}
		else {
			return;
		}
	}
	else
	{
		Diff_ActiveWdw.removeOne(BUTTON_GROUP_ID + 2);
		calculatedEAdc = NULL;
	}

	emit SignalTestButtonFired(_istoggled, calculatedEAdc, imageName, scale, slope);
}

void DiffusionCore::EAdcCalculator(vtkSmartPointer <vtkImageData> imageData, float& scale, float& slope)
{
	if (!this->m_MaskVectorImage) return;

	typedef itk::ComputedEadcFilter <DiffusionCalculatorPixelType, DiffusionCalculatorPixelType> ComputedEadcFilterType;
	ComputedEadcFilterType::Pointer computedEadc = ComputedEadcFilterType::New();
	computedEadc->SetInput(this->m_MaskVectorImage);
	computedEadc->SetBValueList(this->m_DicomHelper->BvalueList);
	computedEadc->SetNumOfDiffDirections(this->m_DicomHelper->numberOfGradDirection);
	computedEadc->SetEadcBValue(this->m_DicomHelper->BvalueList.at(this->m_DicomHelper->BvalueList.size() - 1));//Get from UI input
	computedEadc->Update();

	//Rescale signal intensity to display
	typedef itk::RescaleIntensityImageFilter < DiffusionCalculatorImageType, SourceImageType> RescaleIntensityImageType;
	RescaleIntensityImageType::Pointer rescaleFilter = RescaleIntensityImageType::New();
	rescaleFilter->SetInput(computedEadc->GetOutput());
	rescaleFilter->SetOutputMaximum(4095.0);
	rescaleFilter->SetOutputMinimum(0.0);
	rescaleFilter->Update();
	//std::cout << "rescaleFilter: inputMaximum = " << rescaleFilter->GetInputMaximum() << std::endl;
	//std::cout << "rescaleFilter: inputMinimum = " << rescaleFilter->GetInputMinimum() << std::endl;
	scale = 1 / rescaleFilter->GetScale();
	slope = -1 * rescaleFilter->GetShift() / rescaleFilter->GetScale();

	///////////////////////////////////////////
	//ITK to VTK
	typedef itk::ImageToVTKImageFilter <SourceImageType> itkToVtkConverter;
	itkToVtkConverter::Pointer convItkToVtk = itkToVtkConverter::New();
	convItkToVtk->SetInput(rescaleFilter->GetOutput());
	convItkToVtk->Update();

	imageData->DeepCopy(convItkToVtk->GetOutput());
}

void DiffusionCore::onCalcCDWI(bool _istoggled)  //SLOT of cdwiToggle
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onThreshSlide(double maskThreshold) //SLOT of cdwiToggle
{
	m_MaskThreshold = maskThreshold/5; //slider value is 5 fold of real value. 
	DiffusionCore::onRecalcAll();
}


void DiffusionCore::onCalcFA(bool _istoggled)  //SLOT of faToggle
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcMK(bool _istoggled)  //SLOT of faToggle
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcRK(bool _istoggled)  //SLOT of faToggle
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcAK(bool _istoggled)  //SLOT of faToggle
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcRD(bool _istoggled)  //SLOT of faToggle
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcAD(bool _istoggled)  //SLOT of faToggle
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcColorFA(bool _istoggled)
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcIVIM(bool _istoggled)
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}

void DiffusionCore::onCalcSDKI(bool _istoggled)
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}


void DiffusionCore::onRecalcAll( )
{	
	if (m_DicomHelper->imageDataType == "DIFFUSION")
	{
		this->PreCompute();
		//if (!this->m_MaskVectorImage) return;
		qDebug() << "end of update ";
		//Retrigger All Checked Buttons. 
		for (int i = BUTTON_GROUP_ID + 1; i <= BUTTON_GROUP_ID + 12; i++)
		{
			if (ButtonTable->button(i)->isChecked())
			{
				ButtonTable->button(i)->blockSignals(true);
				ButtonTable->button(i)->setChecked(false);
				ButtonTable->button(i)->blockSignals(false);
				ButtonTable->button(i)->setChecked(true);
			}
		}
	}
}

//void DiffusionCore::PreCompute(DicomHelper* dicomData, DiffusionCalculatorVectorImageType::Pointer _MaskVectorImage)
void DiffusionCore::PreCompute()
{

	//Mask image filter
	typedef itk::MaskVectorImageFilter <DiffusionCalculatorPixelType> MaskFilterType;
	MaskFilterType::Pointer maskFilter = MaskFilterType::New();
	maskFilter->SetInput(m_DicomHelper->processedImageData);
	maskFilter->SetMaskThreshold(m_MaskThreshold);//Get from UI
	maskFilter->Update();

	//double start = GetTickCount();
	m_MaskVectorImage->Graft(maskFilter->GetOutput());
	//double end = GetTickCount();

}

void DiffusionCore::Reset() 
{
	m_MaskThreshold = DEFAULTTHRESH / 5;
	m_ComputedBValue = DEFAULTB;
	m_Controls->Thresh->hide();
	for (int i = BUTTON_GROUP_ID + 1; i < BUTTON_GROUP_ID + 12; i++)
	{
		if (ButtonTable->button(i)->isChecked())
		{		
			ButtonTable->button(i)->setChecked(false);
		}
	}
}

void DiffusionCore::onCalcNODDI()
{
	//DEVELOPER NOTICE:
	//IMPLEMENT YOUR FUNCTION HERE
	//USING onCalcADC as an example
}


//This funciton is for debugging purpose
void DiffusionCore::vtkImageWriter(const char* fileName, vtkImageData* Img)
{
	unsigned short dimensionBuffer[3];
	dimensionBuffer[0] = m_DicomHelper->imageDimensions[0];
	dimensionBuffer[1] = m_DicomHelper->imageDimensions[1];
	dimensionBuffer[2] = m_DicomHelper->imageDimensions[2];
	FILE* pFile;
	//qDebug() << dimensionBuffer[0] << endl;
	pFile = fopen(fileName, "wb");
	fwrite(&dimensionBuffer, sizeof(unsigned short), 3, pFile);
	void *ptr;
	int* extent = Img->GetExtent();
	int dataSize = extent[1] - extent[0] + 1;
	qDebug() << "z range:" << extent[4] << "---" << extent[5] << endl;
	for (int idxZ = extent[4]; idxZ <= extent[5]; ++idxZ)
	{
		for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
		{
			ptr = Img->GetScalarPointer(extent[0], idxY, idxZ);
			fwrite(ptr, sizeof(VTK_FLOAT), dataSize, pFile);
		}
	}
	fclose(pFile);
}