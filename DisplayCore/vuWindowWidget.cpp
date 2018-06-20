#include "vuWindowWidget.h"
#include "QVTKInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkUDInteractorStyleImage.h"
#include <qdebug.h>
#include "qevent.h"
#include "ui_vuWindowWidget.h"
#include "vtkRendererCollection.h"
#include "qpen.h"
#include "vtkImageExtractComponents.h"
#include "vtkContourWidget.h"
#include "vtkJPEGWriter.h"
#include "vtkExtractVOI.h"
#include "vtkImageChangeInformation.h"
#include "vtkImageActor.h"
#include "vtkMatrix4x4.h"
#include "vtkImageActorPointPlacer.h"
#include <vtkOrientedGlyphContourRepresentation.h>
#include <vtkImageActorPointPlacer.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkPolyData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageToImageStencil.h>
#include <vtkImageStencilData.h>
#include <vtkImageStencil.h>
#include <vtkProperty.h>
#include <QDateTime>
#include <vtkCellPicker.h>

#define PAINT_OPACITY 0.3

vuWindowWidget::vuWindowWidget(QString imageLabel, vtkSmartPointer<vtkImageData> image, QPair<float, float> scalingPara, QWidget* p, bool Master, bool fusion) : QFrame(p),
isMaster(Master), isFusion(fusion), ui(new Ui::vuWindowWidget)
{
	Connections = vtkEventQtSlotConnect::New();

	//Setting UP UI
	this->ui->setupUi(this);

	uiImageWidget = new QVTKWidget(this);
	this->ui->verticalLayout->addWidget(uiImageWidget);
	//set ImageName
	this->ImageName = imageLabel;
	this->originalImageData = image;
	this->ImageScale = scalingPara;

	this->ui->uiBGCombo->addItem(imageLabel);

	this->interactState = Normal;
	this->m_brushSize = 5;
	this->m_brushValue = 0;
	this->labelOpacity = PAINT_OPACITY;

	painter = new vtkVusionPainter<unsigned char>();
	this->m_PaintStyle = NULL;//only new for "window1"
	
	
	//WARNING: this is MR-DIFFUSION specific
	if (!isFusion)
	{
		this->ui->label->setVisible(false);
		this->ui->label_2->setVisible(false);
		this->ui->uiFGCombo->setVisible(false);
	}

	if (!isMaster)
	{
		this->ui->compCombo->setVisible(false);
		this->ui->sliceSlider->setVisible(false);
		this->ui->sliceLabel->setVisible(false);
	}

	//init uiImageWidget
	this->uiImageWidget->installEventFilter(this);

	//if (!isMaster)
	//{
	//	this->imageViewer = vtkSmartPointer<vtkVusionSliceView>::New();
	//	this->uiImageWidget->SetRenderWindow(imageViewer->GetRenderWindow());
	//}
	//else {
	this->imageViewer = vtkSmartPointer<vtkVusionMultiView>::New();
	this->imageViewer->CreateDefaultLookupTableForLabelImage();
	this->uiImageWidget->SetRenderWindow(imageViewer->GetRenderWindow());
	vtkSmartPointer<QVTKInteractor> renderWindowInteractor = vtkSmartPointer<QVTKInteractor>::New();

	//vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	//vtkSmartPointer<vtkInteractorStyleImage> interactorStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
	//vtkSmartPointer<vtkUDInteractorStyleImage> interactorStyle = vtkSmartPointer<vtkUDInteractorStyleImage>::New();
	//renderWindowInteractor->SetInteractorStyle(interactorStyle);
	this->uiImageWidget->GetRenderWindow()->SetInteractor(renderWindowInteractor);

	//this->setBackGroundImage2D(image);
	if (isMaster)
	{
		onComponentChanged(0);
	}
	else
	{
		this->setBackGroundImage2D(image);
	}
	//interactorStyle->SetImageViewer(imageViewer);
	imageViewer->SetInteractorStyleToCameraMode();

	renderWindowInteractor->Initialize();
	//interactorStyle->SetDefaultWindowLevel(useCurrentWindowLevel);

	//This corresponds to the lookuptable in paint
	colorList << QColor(0, 0, 0) << QColor(128, 174, 128) << QColor(241, 214, 145) << QColor(177, 122, 101)
		<< QColor(111, 184, 210) << QColor(216, 101, 79) << QColor(221, 130, 101)
		<< QColor(144, 238, 144) << QColor(192, 104, 88) << QColor(220, 245, 20);

	connect(this->ui->uiColorButton, SIGNAL(toggled(bool)), this, SLOT(onToggleColor(bool)));
	connect(this->ui->uiLinkButton, SIGNAL(toggled(bool)), this, SLOT(onToggleLink(bool)));
	connect(this->ui->uiDimButton, SIGNAL(clicked()), this, SLOT(onSaveScreenShot()));
	connect(this->ui->uiPkButton, SIGNAL(toggled(bool)), this, SLOT(onPicker(bool)));

	this->setFrameStyle(QFrame::Box | QFrame::Plain);
	this->setStyleSheet("vuWindowWidget { border: 1px transparent rgb(168, 168, 168);}");

}

vuWindowWidget::~vuWindowWidget()
{
	if (this->m_PaintStyle)
	{
		this->m_PaintStyle->Delete();
		this->m_PaintStyle = NULL;
	}
	this->imageViewer = NULL;
	this->originalImageData = NULL;
	this->newContourWidget = NULL;
	this->Connections = NULL;
}

void vuWindowWidget::setMasterComponent(QString compTitle, std::vector<float> compItem, int currentSlice, int maxSlice)
{
	this->ui->compCombo->clear();
	std::vector<float>::iterator it;

	for (it = compItem.begin(); it != compItem.end(); it++)
	{
		QString compItem = compTitle + QString("%1").arg(*it);
		this->ui->compCombo->addItem(compItem);
		componentTextInfoList << compItem;
	}	

	this->ui->sliceLabel->setText(QString("Slice: %1/%2").arg(currentSlice + 1).arg(maxSlice + 1));

	this->ui->sliceSlider->setMinimum(1);
	this->ui->sliceSlider->setMaximum(maxSlice + 1);
	this->ui->sliceSlider->setValue(currentSlice + 1);

	this->imageViewer->SetSlice(0, currentSlice); //Manually set the view of the 3D image to currentslice when init the source image.
	this->imageViewer->SetSlice(2, currentSlice);

	this->ui->sliceSlider->setTracking(false);

	connect(this->ui->compCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onComponentChanged(int)));
	connect(this->ui->sliceSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliceSliderChanged(int)));
	connect(this->ui->sliceSlider, SIGNAL(sliderMoved(int)), this, SLOT(onSliceSliderMoved(int)));
	connect(this->ui->uiPkButton, SIGNAL(toggled(bool)), this, SLOT(onPicker(bool)));
}

void vuWindowWidget::loadForBrowsing(int currentSlice, int maxSlice)
{
	this->ui->sliceSlider->setVisible(true);
	this->ui->sliceLabel->setVisible(true);

	this->ui->sliceLabel->setText(QString("Slice: %1/%2").arg(currentSlice + 1).arg(maxSlice + 1));

	this->ui->sliceSlider->setMinimum(1);
	this->ui->sliceSlider->setMaximum(maxSlice + 1);
	this->ui->sliceSlider->setValue(currentSlice + 1);
	this->ui->sliceSlider->setTracking(false);
	
	this->imageViewer->SetSlice(0, currentSlice); //Manually set the view of the 3D image to currentslice when init the source image.
	this->imageViewer->SetSlice(2, currentSlice);
	this->ui->sliceSlider->setTracking(false);
	isMaster = true;

	connect(this->ui->sliceSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliceSliderChanged(int)));
	connect(this->ui->sliceSlider, SIGNAL(sliderMoved(int)), this, SLOT(onSliceSliderMoved(int)));
	connect(this->ui->uiPkButton, SIGNAL(toggled(bool)), this, SLOT(onPicker(bool)));
}

void vuWindowWidget::onStartManualDraw(bool status)
{
	if (status && this->interactState!= PlacingSeed)
	{
		if (this->GetRenderWindow()->GetInteractor())
		{
			this->interactState = Paint;
			//vtkSmartPointer<vtkMultiViewInteractorStyleImagePaint> paintStyle = vtkSmartPointer<vtkMultiViewInteractorStyleImagePaint>::New();
			if (!this->m_PaintStyle) this->m_PaintStyle = vtkMultiViewInteractorStyleImagePaint::New();
			this->GetRenderWindow()->GetInteractor()->SetInteractorStyle(this->m_PaintStyle);
			this->m_PaintStyle->SetMultiViewer(this->imageViewer);
			
			//this->imageViewer->SetImageLayerOpacity(2, PAINT_OPACITY);			
			//this->m_PaintStyle->GetPainter()->SetPainterValue(m_brushValue);
			//this->m_PaintStyle->GetPainter()->SetPainterDiameter(m_brushSize);
		}
	}
	else
	{
		this->interactState = Normal;
		this->imageViewer->SetInteractorStyleToCameraMode();
	}
}

void vuWindowWidget::onStartContour()
{
	this->interactState = PlacingSeed;
	this->imageViewer->SetInteractorStyleToCameraMode();

	int roiIdx = m_brushValue;

	//QString key = model->getKey(wdwName, RoiName, slice);	
	//qDebug() << "set RGB to " 
	//	<< colorList.at(roiIdx).red() / 255.0 
	//	<< colorList.at(roiIdx).green() / 255.0 
	//	<< colorList.at(roiIdx).blue() / 255.0;

	vtkImageActorPointPlacer* placer = vtkImageActorPointPlacer::New();
	placer->SetImageActor(imageViewer->GetImageActor(0));

	vtkSmartPointer< vtkOrientedGlyphContourRepresentation> rep = vtkOrientedGlyphContourRepresentation::New();
	rep->GetLinesProperty()->SetLineWidth(1.5);
	rep->GetLinesProperty()->SetColor(colorList.at(roiIdx).red() / 255.0, colorList.at(roiIdx).green() / 255.0, colorList.at(roiIdx).blue() / 255.0);
	rep->SetPointPlacer(placer);
	//rep->SetLineInterpolator(interpolator);

	newContourWidget = vtkSmartPointer<vtkContourWidget>::New();
	newContourWidget->SetRepresentation(rep);
	newContourWidget->SetInteractor(this->GetRenderWindow()->GetInteractor());
	newContourWidget->FollowCursorOn();
	newContourWidget->ContinuousDrawOff();
	newContourWidget->On();

	Connections->Connect(newContourWidget, vtkCommand::EndInteractionEvent,
		this, SLOT(onContourWidgetEnd(vtkObject*, unsigned long, void*, void*, vtkCommand*)));
}

void vuWindowWidget::onContourWidgetEnd(vtkObject*, unsigned long, void*, void*, vtkCommand*)
{
	//vtkIndent indent(2);
	//stencilData->PrintSelf(cout, indent);
	//newContourWidget->PrintSelf(cout, indent);
	qDebug() << "[VusionRoiModule] Contourwidget interaction ends" << endl;

	// Hide this contour widget
	vtkContourRepresentation *rep = vtkContourRepresentation::SafeDownCast(newContourWidget->GetRepresentation());

	vtkOrientedGlyphContourRepresentation *glyContour = vtkOrientedGlyphContourRepresentation::SafeDownCast(rep);
	//std::cout << "After close There are " << glyContour->GetNumberOfNodes() << " nodes and " << glyContour->GetNumberOfPaths() << " path" << std::endl;		

	vtkSmartPointer<vtkPolyData> path = vtkSmartPointer<vtkPolyData>::New();
	path = rep->GetContourRepresentationAsPolyData();

	vtkSmartPointer<vtkImageData> lblImage = vtkSmartPointer<vtkImageData>::New();

	if (this->imageViewer->GetLabelImage())
	{
		lblImage->DeepCopy(this->imageViewer->GetLabelImage());
	}
	else
	{
		lblImage->SetDimensions(this->imageViewer->GetBackgroundImage()->GetDimensions());
		lblImage->SetSpacing(this->imageViewer->GetBackgroundImage()->GetSpacing());
		lblImage->SetOrigin(this->imageViewer->GetBackgroundImage()->GetOrigin());
		lblImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		memset((unsigned char *)(lblImage->GetScalarPointer()), 0, lblImage->GetScalarSize()*lblImage->GetNumberOfPoints());
	}

	int roiValue = m_brushValue;

	qDebug() << "After close There are " << path->GetNumberOfPoints() << " points in the path." << endl;

	vtkSmartPointer<vtkPolyDataToImageStencil> polyDataToImageStencil =
		vtkSmartPointer<vtkPolyDataToImageStencil>::New();
	polyDataToImageStencil->SetTolerance(0);
	polyDataToImageStencil->SetInputData(path); // if version < 5 setinputConnection
	polyDataToImageStencil->SetOutputOrigin(lblImage->GetOrigin());
	polyDataToImageStencil->SetOutputSpacing(lblImage->GetSpacing());
	polyDataToImageStencil->SetOutputWholeExtent(lblImage->GetExtent());
	polyDataToImageStencil->Update();

	vtkSmartPointer<vtkImageStencil> imageStencil = vtkSmartPointer<vtkImageStencil>::New();
	//imageStencil->SetBackgroundInputData();
	imageStencil->SetInputData(lblImage);
	imageStencil->SetStencilData(polyDataToImageStencil->GetOutput());
	imageStencil->ReverseStencilOn();
	imageStencil->SetBackgroundValue(roiValue);
	imageStencil->Update();

	this->setLabelImage3D(imageStencil->GetOutput());
	
	emit sigSyncPaint(this->ImageName);

	newContourWidget -> Off();

	this->interactState = Normal;
	//newContourWidget = NULL;
	//newContourWidget->Delete();
}

void vuWindowWidget::onBrushSizeChanged(int size)
{
	this->m_brushSize = size;//Remove later?
	if (!this->m_PaintStyle) this->m_PaintStyle = vtkMultiViewInteractorStyleImagePaint::New();
	this->m_PaintStyle->GetPainter()->SetPainterDiameter(m_brushSize);
}

void vuWindowWidget::onBrushValueChanged(int val)
{
	qDebug() << "[SliceWindowWidget] receive brush value = " << val << endl;
	this->m_brushValue = val;//remove later?
	if (!this->m_PaintStyle) this->m_PaintStyle = vtkMultiViewInteractorStyleImagePaint::New();
	this->m_PaintStyle->GetPainter()->SetPainterValue(val);
}

void vuWindowWidget::onComponentChanged(int compoInd)
{
	vtkSmartPointer <vtkImageExtractComponents> scalarComponent = vtkSmartPointer <vtkImageExtractComponents>::New();
	scalarComponent->SetInputData(this->originalImageData);
	scalarComponent->SetComponents(compoInd);
	scalarComponent->Update();

	this->setBackGroundImage2D(scalarComponent->GetOutput());	
}

void vuWindowWidget::onSliceSliderChanged(int slice)
{
	//int maxslice  = this->ui->sliceLabel->text().split("/")[1].toInt();
	//this->ui->sliceLabel->setText(QString("Slice: %1/%2").arg(slice).arg(maxslice));
	emit sigImageWidgetSliceChanged(slice - 1, this->ImageName);
}

void vuWindowWidget::onSliceSliderMoved(int slice)
{
	int maxslice = this->ui->sliceLabel->text().split("/")[1].toInt();
	this->ui->sliceLabel->setText(QString("Slice: %1/%2").arg(slice).arg(maxslice));
}

void vuWindowWidget::onPicker(bool on)
{
	if (on)
	{
		this->interactState = Pick;
	}
	else
	{
		this->interactState = Normal;
	}
}

void vuWindowWidget::setBackGroundImage2D(vtkSmartPointer<vtkImageData> imageData)
{

	//if the qvtkWidget has been set up, the whole procedure don't need to be applied again
	//just update the image and slice info
	//handle other image window here
	//this->DisplayDicomInfo(imageData);		

	imageViewer->SetBackgroundImage(imageData);
	//imageViewer->SetSliceOrientationToXY();
	//imageViewer->GetRenderer()->ResetCamera();		



	
	if (this->ImageName == "cFA")
	{
		qDebug() << "[vuWindowWidget] set background Image to: " << this->ImageName << endl;
		double colorWindow = 255.0;
		double colorLevel = 127.5;
		imageViewer->GetWindowLevel(0)->SetWindow(colorWindow);
		imageViewer->GetWindowLevel(0)->SetLevel(colorLevel);
		imageViewer->Render();
	}
	else {
		if (this->ui->uiColorButton->isChecked())
		{
			//qDebug() << "Reset colormap to default" << endl;
			this->imageViewer->SetViewModeToColorMap(0);
		}
		else
		{
			//qDebug() << "Reset grayscale to default" << endl;
			this->imageViewer->SetViewModeToGrayScale(0);
		}
	}

	//vtkImageData *labelImage = vtkImageData::New();
	//labelImage->SetDimensions(this->imageViewer->GetBackgroundImage()->GetDimensions());
	//labelImage->SetSpacing(this->imageViewer->GetBackgroundImage()->GetSpacing());
	//labelImage->SetOrigin(this->imageViewer->GetBackgroundImage()->GetOrigin());
	//labelImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
	//memset((unsigned char *)(labelImage->GetScalarPointer()), 1, labelImage->GetScalarSize()*labelImage->GetNumberOfPoints());
	//this->imageViewer->SetLabelImage(labelImage);
	//this->imageViewer->SetViewModeToColorMap(2);

	//this->imageViewer->SetViewModeToGrayScale(0);
	
	//_ImageAutoFillWindow(this->uiImageWidget->size());
	
	//this->uiImageWidget->show();
	//this->uiImageWidget->GetRenderWindow()->Render();

	
	//imageViewer->SetSlice(0);	
	//imageViewer->SetColorWindow(colorWindow);
	//imageViewer->SetColorLevel(colorLevel);
	//vtkSmartPointer<vtkCornerAnnotation> cornerAnnotation =
	//vtkSmartPointer<vtkCornerAnnotation>::New();
	//cornerAnnotation->SetImageActor(imageViewer->GetImageActor());
	//cornerAnnotation->SetWindowLevel(imageViewer->GetWindowLevel());
	//cornerAnnotation->SetLinearFontScaleFactor(2);
	//cornerAnnotation->SetNonlinearFontScaleFactor(1);
	//cornerAnnotation->SetMaximumFontSize(12);
	//cornerAnnotation->SetMaximumFontSize(20);
	//cornerAnnotation->SetText(3, "<window>\n<level>");
	//cornerAnnotation->GetTextProperty()->SetFontFamilyToCourier();
	//imageViewer->GetRenderer()->AddViewProp(cornerAnnotation);
	//vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();	
	//Use QVTKInteractor rather than vtkRenderWindowInteractor!!! So that interactor start and end events are handled by qApp->exec() and qApp->Exit();
	//renderWindowInteractor->Initialize();
}

void vuWindowWidget::onToggleColor(bool isColor)
{	
	if (isFusion)
	{
		if (isColor)
		{
			imageViewer->SetViewModeToColorMap(1);
		}
		else
		{
			imageViewer->SetViewModeToGrayScale(1);
		}
		this->uiImageWidget->GetRenderWindow()->Render();
	}
	else {
		if (isColor)
		{
			imageViewer->SetViewModeToColorMap(0);
		}
		else
		{
			imageViewer->SetViewModeToGrayScale(0);
		}
		this->uiImageWidget->GetRenderWindow()->Render();
	}
}

void vuWindowWidget::onToggleLink(bool status)
{
	if (status)
	{
		this->setLineWidth(3);
		this->setMidLineWidth(0);
		this->setFrameStyle(QFrame::Box | QFrame::Plain);
		this->setStyleSheet("vuWindowWidget { border: 1px solid rgb(233, 89, 89);}");
	}
	else
	{
		this->setFrameStyle(QFrame::Box | QFrame::Plain);
		this->setStyleSheet("vuWindowWidget { border: 1px transparent rgb(168, 168, 168);}");
	}

	emit sigChangeWindowLinkStatus(this->ImageName, status);
}

bool vuWindowWidget::eventFilter(QObject *watched, QEvent *Fevent)
{
	if (Fevent->type() == QEvent::Resize)
	{
		QResizeEvent* e = static_cast<QResizeEvent*>(Fevent);
		//qDebug() << "window widget size: " << e->size();
		//qDebug() << "QVTKWidget size: " << this->ui->uiImageWidget->size();
		
		this->_ImageAutoFillWindow(e->size());

		return false;
	}
	else if (Fevent->type() == QEvent::KeyPress)
	{
		QKeyEvent* e2 = static_cast<QKeyEvent*>(Fevent);
		qDebug() << e2->text() << "and int is" << e2->key();
		if (e2->key() == 82) //Press 'R' or 'r'
		{			
			if (this->ui->uiColorButton->isChecked())
			{
				qDebug() << "color reset";
				this->imageViewer->SetViewModeToColorMap(0);
			}
			else
			{
				qDebug() << "gray reset";
				this->imageViewer->SetViewModeToGrayScale(0);
			}
			_ImageAutoFillWindow(this->uiImageWidget->size());
		}else if (e2->key() == 16777234) //Press <-
		{
			int curInd = this->ui->compCombo->currentIndex();
			int newind = curInd > 0 ? curInd - 1 : 0;
			this->ui->compCombo->setCurrentIndex(newind);
		}
		else if (e2->key() == 16777236) //Press ->
		{
			int curInd = this->ui->compCombo->currentIndex();
			int newind = curInd < this->ui->compCombo->count()-1 ? curInd + 1 : this->ui->compCombo->count()-1;
			this->ui->compCombo->setCurrentIndex(newind);
		}
		//else if (e2->key() == 84)
		//{
			//imageViewer->GetImageActor()->GetUserMatrix();
			//std::cout << "User Matrix zero " << imageViewer->GetImageActor()->GetUserMatrix() << std::endl;
			//for (int i = 0; i < 4; i++)
			//{
			//	std::cout << "row " << i << ": ";
			//	for (int j = 0; j < 4; j++)
			//	{
			//		std::cout << imageViewer->GetImageActor()->GetMatrix()->GetElement(i, j) << " ";
			//	}std::cout << std::endl;
			//}

			//newContourWidget = vtkSmartPointer<vtkContourWidget>::New();
			//newContourWidget->SetInteractor(this->imageViewer->GetRenderWindow()->GetInteractor());			
			//newContourWidget->FollowCursorOn();
			//newContourWidget->ContinuousDrawOff();
			//newContourWidget->On();	
		//}
		return true;
	}
	else if (Fevent->type() == QEvent::Wheel)
	{
		//qDebug() << "wheel event";
		return false;
	}
	else if (Fevent->type() == QEvent::MouseButtonPress || Fevent->type() == QEvent::MouseButtonRelease || Fevent->type() == QEvent::MouseMove)
	{

		QMouseEvent* e = static_cast<QMouseEvent*>(Fevent);

		if (e->source() == Qt::MouseEventNotSynthesized)
		{
			
			e->ignore();
			//qDebug() << "Real Mouse Button Pressed at [" << e->x() << "-" << e->y() << "] of" << imageName << " accepted? " << e->isAccepted() << endl;
			emit sigImageWidgetMouseEvent(e, this->ImageName);
			//return false;
		}

		switch (this->interactState)
		{
		case Paint:
			if (e->type() == QEvent::MouseButtonRelease)
			{
				if (e->button() == Qt::LeftButton)
				{
					qDebug() << "[vuWindowWidget] Send out signal indicating Paint Finish" << ImageName << endl;
					emit sigSyncPaint(this->ImageName);
				}
			}
			break;
		case Normal:
			if (e->type() == QEvent::MouseButtonRelease)
			{
				//qDebug() << "[SliceWindowWidget]window level event evoked. " ;
				emit sigWdwLevel(this->imageViewer->GetWindowLevel(0)->GetWindow(), this->imageViewer->GetWindowLevel(0)->GetLevel());
			}
			break;
		case Pick:
			if (e->type() == QEvent::MouseMove)
			{
				vtkSmartPointer<vtkRenderWindowInteractor> interactor = this->GetRenderWindow()->GetInteractor();
				//qDebug() << "now is placing seed widget"<<endl;
				
				//qDebug() << "widget state is: " << contourWidget->GetWidgetState();
				if (interactor)
				{
					//qDebug() << "interactor pos :" << interactor->GetEventPosition()[0] << interactor->GetEventPosition()[1] << interactor->GetEventPosition()[2] << endl;
					interactor->GetPicker()->Pick(interactor->GetEventPosition()[0],
						interactor->GetEventPosition()[1],
						0,  // always zero.
						GetRenderWindow()->GetRenderers()->GetFirstRenderer());

					double pos[3];
					interactor->GetPicker()->GetPickPosition(pos);

					int image_coordinate[3];
					double spacing[3];
					double origins[3];
					int dim[3];
					this->imageViewer->GetBackgroundImage()->GetDimensions(dim);
					this->imageViewer->GetBackgroundImage()->GetSpacing(spacing);
					this->imageViewer->GetBackgroundImage()->GetOrigin(origins);

					qDebug() << "pos is " << pos[0] << "-" << pos[1] << "-" << pos[2] << endl;
					qDebug() << "Space is " << spacing[0] << "-" << spacing[1] << "-" << spacing[2] << endl;
					qDebug() << "dim is " << dim[0] << "-" << dim[1] << "-" << dim[2] << endl;

					image_coordinate[0] = round((pos[0] - origins[0]) / spacing[0]);
					image_coordinate[1] = round((pos[1] - origins[1]) / spacing[1]);
					image_coordinate[2] = round((pos[2] - origins[2]) / spacing[2]);

					qDebug() << "Image Position : " << image_coordinate[0] << "-" << image_coordinate[1] << "-" << image_coordinate[2] << endl;

					if (image_coordinate[0] < dim[0] && image_coordinate[0]>=0
						&& image_coordinate[1] < dim[1] && image_coordinate[1]>=0
						&& image_coordinate[2] < dim[2] && image_coordinate[2]>=0
						)
					{
						qDebug() << "image_coordinate is " << image_coordinate[0] << "-" << image_coordinate[1] << "-" << image_coordinate[2];
						float rawVal = round(this->imageViewer->GetBackgroundImage()->GetScalarComponentAsFloat(image_coordinate[0], image_coordinate[1], image_coordinate[2], 0));
						float value = (ImageScale.first)*rawVal + ImageScale.second;
						qDebug() << " Value is " << value;
						emit sigPickerValue(ImageName, value);
					}
				}
			}
			return false;
			break;
		default:
			break;
		}
		return false;
		
	}
	else if (Fevent->type() == QEvent::Leave)
	{
		if (m_PaintStyle && interactState == Paint)
		{
			qDebug() << "[vuWindowWidget] mouse leaved this widget." << endl;
			m_PaintStyle->setCircleVisability(false);
		}
		return false;
	}
	else if (Fevent->type() == QEvent::Enter)
	{
		if (m_PaintStyle &&  interactState == Paint)
		{
			qDebug() << "[vuWindowWidget] mouse leaved this widget." << endl;
			m_PaintStyle->setCircleVisability(true);
		}
		return false;
	}
	return QObject::eventFilter(watched, Fevent);
}

void vuWindowWidget::_ImageAutoFillWindow(QSize wdwsize)
{
	vtkSmartPointer < vtkRendererCollection> renderers = this->uiImageWidget->GetRenderWindow()->GetRenderers();
	if (renderers)
	{
		renderers->GetNumberOfItems();
		renderers->InitTraversal();
		while (vtkSmartPointer < vtkRenderer> renderer = renderers->GetNextItem())
		{
			renderer->ResetCamera();
			double *imageBounds = renderer->ComputeVisiblePropBounds();
			//int windowSize[2]; windowSize[0] = e->size().width(); windowSize[1] = e->size().height();
			renderer->GetActiveCamera()->ParallelProjectionOn();
			double xFov = imageBounds[1] - imageBounds[0];
			double yFov = imageBounds[3] - imageBounds[2];
			double screenRatio = double(wdwsize.height()) / wdwsize.width();//height / width;
			double imageRatio = yFov / xFov;
			double parallelScale = imageRatio > screenRatio ? yFov : screenRatio / imageRatio*yFov;
			renderer->GetActiveCamera()->SetParallelScale(0.5*parallelScale);
		}
	}

	this->uiImageWidget->GetRenderWindow()->Render();
}

void vuWindowWidget::setLabelImage3D(vtkSmartPointer<vtkImageData> labelImage)
{
	this->imageViewer->SetLabelImage(labelImage);
	
	if (
		(!this->imageViewer->GetBackgroundImage()) &&
		(!this->imageViewer->GetForegroundImage())
		)
	{
		//Should
		this->imageViewer->SetSliceOrientation(2);
	}

	this->imageViewer->SetSlice(2, this->getCurrentSlice());
	this->imageViewer->SetViewModeToColorMap(2);

	this->imageViewer->SetImageLayerOpacity(2, this->labelOpacity);
	this->imageViewer->Render();
}

void vuWindowWidget::setLabelImage2D(vtkSmartPointer<vtkImageData> labelImage,int slice)
{
	this->imageViewer->SetLabelImage(labelImage);

	if (
		(!this->imageViewer->GetBackgroundImage()) &&
		(!this->imageViewer->GetForegroundImage())
		)
	{
		//Should
		this->imageViewer->SetSliceOrientation(2);
	}

	this->imageViewer->SetSlice(2, 0);
	this->imageViewer->SetViewModeToColorMap(2);
	this->imageViewer->SetImageLayerOpacity(2, this->labelOpacity);
	this->imageViewer->Render();
}

void vuWindowWidget::updateImage(vtkSmartPointer<vtkImageData> imageData, int slice)
{
	if (this->ui->sliceLabel->isVisible() && this->ui->sliceSlider->isVisible())
	{
		int maxslice = this->ui->sliceLabel->text().split("/")[1].toInt();
		this->ui->sliceLabel->setText(QString("Slice: %1/%2").arg(slice + 1).arg(maxslice));

		this->ui->sliceSlider->blockSignals(true);
		this->ui->sliceSlider->setValue(slice + 1);
		this->ui->sliceSlider->blockSignals(false);
	}

	if (isMaster)
	{
		//onComponentChanged(this->ui->compCombo->currentIndex());
		this->imageViewer->SetSlice(0, slice);
		//qDebug() << "[WindowWidget] Reslice label image to " << slice << endl;
		this->imageViewer->SetSlice(2, slice);
		this->imageViewer->Render();
	}
	else {
		this->originalImageData = imageData;
		setBackGroundImage2D(imageData);
	}
}

void vuWindowWidget::onSaveScreenShot()
{
	QString screenShotName;
	QDateTime current_date_time = QDateTime::currentDateTime();
	//QString current_date = current_date_time.toString("yyyy-MM-dd");
	QString current_time = current_date_time.toString("yyyyMMdd_hhmmss_");
	QTextStream(&screenShotName) << this->ImageName << "_" << current_time <<".jpg";
	//qDebug() << "Generate Screenshot";
	vtkSmartPointer <vtkJPEGWriter> writer = vtkSmartPointer <vtkJPEGWriter>::New();
	writer->SetInputData(this->uiImageWidget->cachedImage());
	writer->SetFileName(screenShotName.toStdString().c_str());
	writer->Write();
}

int vuWindowWidget::getCurrentSlice()
{
	if (isMaster)
	{
		return this->ui->sliceSlider->value() - 1;
	}
	else
	{
		return -1;
	}
}
