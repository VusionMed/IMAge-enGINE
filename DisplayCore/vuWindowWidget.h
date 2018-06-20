/*=========================================================================

Program:   Vusion ImageEngine
Module:    WindowWidget

=========================================================================*/
// .NAME WindowWidget - subclass of QVTKWidget2. 
// .SECTION Description
// add float* scaling for storing the scale of the images rendered in this widget. 
// add QString imageName for storing the name of the images rendered in this widget.


#ifndef vuWindowWidget_h
#define vuWindowWidget_h


#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "qstring.h"
#include "qpair.h"

#include <vtkSmartPointer.h>
#include <vtkVusionMultiView.h>
#include <vtkImageData.h>
#include <qvtkwidget.h>
#include "vtkContourWidget.h"
#include "vtkMultiViewInteractorStyleImagePaint.h"

#include "vtkVusionPainter.h"

class vtkCornerAnnotation;
class vtkCamera;
class vtkEventQtSlotConnect;

namespace Ui {
	class vuWindowWidget;
}


class vuWindowWidget : public QFrame
{

	Q_OBJECT

public:

	/// Description: vuWindowWidget(QString imageLabel, vtkSmartPointer<vtkImageData> image, int mode, QWidget* p = NULL);
	/// input: mode = 1 2D, mode = 2 3D.
	vuWindowWidget(QString imageLabel, vtkSmartPointer<vtkImageData> image, QPair<float, float> scalingPara,
		QWidget* p = NULL, bool master = false, bool fusion = false);

	~vuWindowWidget();

	QString getImageName()
	{
		return this->ImageName;
	}

	void setImageName(QString name)
	{
		this->ImageName = name;
	}

	void setBackGroundImage2D(vtkSmartPointer<vtkImageData> imagedata);

	QPair<float, float> getImageScale()
	{
		return this->ImageScale;
	}

	void setImageScale(float slope, float intercept) {
		this->ImageScale.first = slope;
		this->ImageScale.second = intercept;
	}

	vtkSmartPointer<vtkImageData> getImageData()
	{
		return this->originalImageData;
	}

	void setImageData(vtkSmartPointer<vtkImageData> imgData)
	{
		this->originalImageData = imgData;
	}

	int getCurrentSlice();

	int getNumberOfComponent()
	{
		return this->componentTextInfoList.count();
	}

	void updateImage(vtkSmartPointer<vtkImageData> imageData, int slice);

	void setMasterComponent(QString compText, std::vector<float> compItem, int currentSlice, int maxSlice);

	void loadForBrowsing(int currentSlice, int maxSlice);

	vtkRenderWindow* GetRenderWindow()
	{
		return this->uiImageWidget->GetRenderWindow();
	}

	vtkVusionMultiView* GetImageViewer()
	{
		return this->imageViewer.GetPointer();
	}

	QVTKWidget* getVTKWidget()
	{
		return this->uiImageWidget;
	}

	vtkImageData* getOriginalImageData()
	{
		return this->originalImageData;
	}

	QStringList getComponentTxt()
	{
		return this->componentTextInfoList;
	}

	QList<QColor> getColorList()
	{
		return this->colorList;
	}

	void setLabelImage3D(vtkSmartPointer<vtkImageData> lblImage);

	void setLabelImage2D(vtkSmartPointer<vtkImageData> labelImage, int slice);

	void setLabelImageOpacity(float opa)
	{
		this->labelOpacity = opa;
	}

signals:
	void sigChangeWindowLinkStatus(const QString, bool);
	void sigImageWidgetMouseEvent(QMouseEvent *, const QString);
	void sigImageWidgetSliceChanged(int slice, const QString);
	void sigSyncPaint(QString);
	void sigPickerValue(QString, float);
	void sigWdwLevel(float, float);
	protected  slots:
	void onToggleColor(bool);
	void onToggleLink(bool);
	void onSliceSliderMoved(int slice);
	void onSaveScreenShot();
	void onContourWidgetEnd(vtkObject*, unsigned long, void*, void*, vtkCommand*);
	void onPicker(bool);

	public slots:
	void onSliceSliderChanged(int slice);	
	void onComponentChanged(int);
	void onStartManualDraw(bool status);
	void onBrushSizeChanged(int size);
	void onBrushValueChanged(int val);
	void onStartContour();

protected:
	bool eventFilter(QObject *, QEvent *);
	
private:

	int interactState;

	enum
	{
		//NullEvent = 0,
		Normal = 1,
		PlacingSeed = 2,
		Paint = 3,
		Pick = 4
	};

	//GUI
	Ui::vuWindowWidget *ui;
	bool isMaster;
	bool isFusion;
	int widgetState; //0: fresh widget. 1: 2d image rendered. 2: 3d image rendered

	QString ImageName;
	QStringList componentTextInfoList;

	int m_brushSize;
	int m_brushValue;
	float labelOpacity;

	QList<QColor> colorList;
	vtkVusionPainter<unsigned char> *painter;
	vtkMultiViewInteractorStyleImagePaint *m_PaintStyle;

	int currentComponent;

	//vtkSmartPointer<vtkImageData> imageData;
	QVTKWidget* uiImageWidget;
	vtkSmartPointer<vtkImageData> originalImageData;
	vtkSmartPointer<vtkEventQtSlotConnect> Connections;

	QPair<float, float> ImageScale;

	//vtkSmartPointer<vtkCornerAnnotation>  CornerAnnotation;	

	vtkSmartPointer<vtkVusionMultiView> imageViewer;

	vtkSmartPointer<vtkContourWidget> newContourWidget;

	void _ImageAutoFillWindow(QSize wdwsize);	

};

#endif