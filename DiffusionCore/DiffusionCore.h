/*===================================================================

DWI Core UI

===================================================================*/

#ifndef DiffusionCore_h
#define DiffusionCore_h


//include QT
#include <QWidget>
#include <qdebug.h>

//include ITK
#include <itkVectorImage.h>
#include <itkImage.h>
#include <itkRGBPixel.h>

//include VTK
#include <vtkSmartPointer.h>
#include <vtkObject.h>


typedef unsigned short SourceImagePixelType;
typedef float DiffusionCalculatorPixelType; 
typedef itk::Image < SourceImagePixelType, 3> SourceImageType;
typedef itk::Image <DiffusionCalculatorPixelType, 3> DiffusionCalculatorImageType;
typedef itk::VectorImage <DiffusionCalculatorPixelType, 3> DiffusionCalculatorVectorImageType;
typedef itk::RGBPixel<unsigned char> RGBpixelType;
typedef itk::Image<RGBpixelType, 3>  RGBImageType;
//jiangli end

class ctkFileDialog;

namespace Ui{
	class DiffusionModule;
}
class DicomHelper;
class QVTKWidget;
class vtkCamera;
class vtkEventQtSlotConnect;
class vtkImageData;
class vtkRenderWindowInteractor;
class DisplayPort;
class QGroupBox;
class QButtonGroup;
class QProgressDialog;
/**
* \brief DiffusionCore is a QWidget providing functionality for diffusion weighted image calculation.
*
* \sa 
* \ingroup 
*/
#define MAXWINDOWNUMBER 5 //difine the busiest viewing window layout as 5 by 5
class DiffusionCore : public QWidget
{

	Q_OBJECT

public:

	/**
	* \brief DiffusionCore(QWidget *parent) constructor.
	*
	* \param parent is a pointer to the parent widget
	*/
	DiffusionCore(QWidget *parent);

	/**
	* \brief DiffusionCore destructor.
	*/
	virtual ~DiffusionCore();

	/**
	* \brief CreateQtPartControl(QWidget *parent) sets the view objects from ui_DiffusionModule.h.
	*
	* \param parent is a pointer to the parent widget
	*/
	virtual void CreateQtPartControl(QWidget *parent);

	/**
	* \brief Reset the widget. This method has to be called before widget can start.
	*/
	void Reset();


signals:

	///// @brief emitted when dicomdata is imported.
	void SignalImageLoaded(bool);

	void SignalTestButtonFired(bool _istoggled, vtkSmartPointer <vtkImageData>, QString, float , float );

	void signal3DImage(vtkSmartPointer <vtkImageData>, QString);

	void signalSaveDcmUpdate(const QString, int);

	//void signalSaveDcmImg();	

	public slots:

	/////// @brief 
	/////// This slot retrieve source image.
	/////// 
	//void OnImageFilesLoaded(const QStringList&);

	///// @brief 
	///// This slot recalculate all displaying image.
	///// 
	void onRecalcAll();

	///// @brief
	///// In this slot, 3d calculation of input buttonID. 
	///// 
	void onCalc3D(QString directory, const QList<QString> & selectedWindow, int type = 0);


	protected slots:

	///// @brief
	///// In this slot, create/update SourceImage
	///// 
	void onSetSourceImage(DicomHelper*);
	
	///// @brief
	///// In this slot, call adc calculation and render the image
	///// 
	void onCalcADC(bool toggle);

	///// @brief
	///// In this slot, call cdwi calculation and render the image
	///// 
	void onCalcCDWI(bool toggle);

	///// @brief
	///// In this slot, call eADC calculation and render the image
	///// 
	void onCalcEADC(bool toggle);

	///// @brief
	/////In this slot, call FA calculation and render the image
	///// 
	void onCalcFA(bool toggle);

	///// @brief
	///// In this slot, call colorFA calculation and render the image
	/////
	void onCalcColorFA(bool toggle);
	void onCalcRD(bool _istoggled);
	void onCalcAD(bool _istoggled);

	///// @brief
	///// DKI button groups
	/////
	void onCalcMK(bool _istoggled);
	void onCalcRK(bool _istoggled);
	void onCalcAK(bool _istoggled);


	///// @brief
	///// In this slot, call IVIM calculation and render the image
	///// 
	void onCalcIVIM(bool toggle);

	///// @brief
	///// In this slot, call NODDI calculation without rendering the image.
	///// 
	void onCalcNODDI();

	///// @brief
	///// In this slot, update the adc image with filtering using input threshhold
	///// 
	void onThreshSlide(double threshhold);

	///// @brief
	///// In this slot, update the sDKI image with filtering using input threshhold
	///// 
	void onCalcSDKI(bool _istoggled);

protected:

	void PreCompute();//This should be moved to DicomHelper class.
	void AdcCalculator(vtkSmartPointer <vtkImageData> imageData, float& slope, float& intercept);
	void EAdcCalculator(vtkSmartPointer <vtkImageData> imageData, float& slope, float& intercept);
	void vtkImageWriter(const char* fileName, vtkImageData* Img);

	
protected:
	
	Ui::DiffusionModule* m_Controls;

	DicomHelper* m_DicomHelper;	

	DiffusionCalculatorVectorImageType::Pointer m_MaskVectorImage;//USE VectorImageType::Pointer
	RGBImageType::Pointer  m_ColorFaImage;

	vtkEventQtSlotConnect* Connections;

	int sourceScalarType = 0;

	double m_MaskThreshold;
	double m_ComputedBValue;

	//Configuration Settings

	int NODDI_MED_B;
	int	NODDI_MIN_B;
	
	int D_BVAL_THRESH;
	int Dstar_BVAL_THRESH;
	
	int SDKI_LOW_B_MIN;
	int SDKI_LOW_B_MAX;
	int SDKI_HIGH_B_MIN;
	int SDKI_HIGH_B_MAX;

	int	MIN_KURTOSIS;
	int	MAX_KURTOSIS;

	QButtonGroup* ButtonTable; // A QButtonGroup to store all algorithm Buttons;
	QList<int> Diff_ActiveWdw; // This should be synchronising to mainwindow activeWDW;
};


#endif //

