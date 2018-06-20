
//Internal include
#include "DicomHelper.h"
#include "displayport.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qVusionRoiModule.h"
#include "vusionAboutDialog.h"
#include "settingDialog.h"
#include "displayport.h"
#include "vuDICOMBrowser.h"
#include "ctkDICOMDatabase.h"
#include "vuWindowWidget.h"


//QT include
#include <QNetworkConfigurationManager>
#include <QMessageBox>
#include <QStyleFactory>
#include <QStandardItemModel>
#include <QStandardItem>
#include <qfile.h>
#include <qdebug.h>
#include <qstring.h>
#include <QPainter>
#include "qevent.h"
#include <QInputDialog>
#include <QNetworkInterface>
#include <QTableWidgetItem>
#include <QTableWidget>
// CTK
#include <ctkFileDialog.h>

//VTK include
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
#include <vtkCornerAnnotation.h>
#include <vtkContourWidget.h>
#include <vtkContourRepresentation.h>
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
#include <vtkImageProperty.h>
#include <vtkCollection.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageShiftScale.h>

#include <vtkActor2DCollection.h>
#include <vtkContourWidget.h>
#include <vtkOrientedGlyphContourRepresentation.h>
#include <vtkProperty.h>
#include <vtkImageThreshold.h>
#include <vtkImageCast.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
	//qDebug() << "break while" << endl;
	this->m_DicomHelper = NULL;
	this->sourceImage = vtkSmartPointer<vtkImageData>::New();//VTK image pointer

	m_SourceImageCurrentSlice = 0;
	//m_SourceImageCurrentComponent = 0;
	m_SourceImageMaxSlice = 0;
	m_SourceImageMinSlice = 0;

	DicomUI = new vuDICOMBrowser(this);
	DicomUI->hide();
	
	this->ui = new Ui::MainWindow;
	this->ui->setupUi(this);

	showMaximized();

	ui->ViewFrame->setFocus();

	//Menu Button connections
	connect(ui->FileButton, SIGNAL(clicked()), this, SLOT(onStartdicom()));
	connect(DicomUI, SIGNAL(SignalDicomToDataManager(QStringList)), this, SLOT(OnImageFilesLoaded(QStringList)));
	connect(ui->SaveButton, SIGNAL(clicked()), this, SLOT(onExportImage()));

	connect(ui->InfoButton, SIGNAL(clicked()), this, SLOT(onAboutDialog()));

	connect(ui->settingBtn, SIGNAL(clicked()), this, SLOT(onOpenSettingPage()));

	//Diffusion Module Connections
	connect(ui->diffusionModule, SIGNAL(SignalTestButtonFired(bool , vtkSmartPointer <vtkImageData>, QString,  float,  float)), 
		this, SLOT(onProcButtonClicked(bool, vtkSmartPointer <vtkImageData>, const QString, const float, const float)));
	connect(this, SIGNAL(SignalSetSourceImage(DicomHelper*)), ui->diffusionModule, SLOT(onSetSourceImage(DicomHelper*)));
	connect(this, SIGNAL(SignalRecalcAll()), ui->diffusionModule, SLOT(onRecalcAll()));		

	//View Frame Connections
	connect(ui->ViewFrame, SIGNAL(signalWheel(const QString, int, Qt::Orientation)), this, SLOT(onWheelWdw(const QString, int, Qt::Orientation)));


	this->setWindowTitle(tr("IMAgenGINE_MRDP"));
	 	
}


void MainWindow::onStartdicom()
{
	//std::cout << "button clicked" << std::endl;
	DicomUI->setWindowFlags(Qt::Window);
	DicomUI->setWindowTitle(tr("DICOM BROWSER"));
	DicomUI->setAttribute(Qt::WA_QuitOnClose, true);
	DicomUI->resize(this->ui->ViewFrame->size());
	DicomUI->move(this->ui->ViewFrame->geometry().topLeft());
	DicomUI->show();

}

void MainWindow::onProcButtonClicked(bool addOrRemove, vtkSmartPointer <vtkImageData> data, const QString imageName, const float scale, const float slope)
{	
	if (addOrRemove) //true:add, false:remove
	{
		double start = GetTickCount();
		QWidget *wdwItem(this->ui->ViewFrame->getWindow(imageName));
		vuWindowWidget* vtkWindow;
		if (wdwItem != NULL)
		{
			//qDebug() << imageName << " window exists" <<endl;			
			vtkWindow = static_cast <vuWindowWidget*> (wdwItem);
			vtkWindow->updateImage(data,m_SourceImageCurrentSlice);
			vtkWindow->setImageScale(scale,slope);
			//vtkWIndow->setImageName()
		}
		else{
			vtkWindow = new vuWindowWidget(imageName, data, qMakePair<float, float>(scale, slope), this);
			this->ui->ViewFrame->insertWindow(vtkWindow, imageName);
			//std::cout << "... window created" << std::endl;
			connect(vtkWindow, SIGNAL(sigChangeWindowLinkStatus(const QString, bool)), this, SLOT(onChangeLinkStatus(const QString, bool)));
			connect(vtkWindow, SIGNAL(sigImageWidgetMouseEvent(QMouseEvent *, const QString)), this, SLOT(onBroadcastEvent(QMouseEvent *, const QString)));
			connect(vtkWindow, SIGNAL(sigPickerValue(QString, float)), this, SLOT(onPickerSig(QString, float)));

			//roi block start
			this->ui->roiModule->UpdateLabelImage();
			//roi block stop

		}
		
		
		double end = GetTickCount();
		qDebug() << "image viewer takes time: " << end - start << endl;
		qDebug() << "Entire calculation of " << imageName << " costs " << end - m_timeStart << "ms" <<endl;

	}
	else
	{

		QWidget *wdwItem(this->ui->ViewFrame->getWindow(imageName));
		ActiveWdw.removeOne(imageName);
		this->ui->ViewFrame->removeWindow(imageName);
	}
}

MainWindow::~MainWindow()
{

}

void MainWindow::OnImageFilesLoaded(const QStringList& fileLists)
{
	//1. Determine Whether It is Update or Init;
	bool isUpdate;
	isUpdate = this->ui->ViewFrame->getAllWindow().count() > 0 ? true: false;

	qDebug() << "[MainWindow] Loading New Series; Is this updating? " << isUpdate << endl;

	double start = GetTickCount();

	//2. Init DicomHelper

	QString loadedSeriesUID = DicomUI->database()->seriesForFile(fileLists[0]);
	StudyUID = DicomUI->database()->studyForSeries(loadedSeriesUID);

	//Secure the file is loadable:
	vtkSmartPointer < vtkDICOMReader> testReader = vtkSmartPointer < vtkDICOMReader>::New();
	QStringList filteredFiles;
	foreach(QString file, fileLists)
	{
		if (testReader->CanReadFile(file.toStdString().c_str())) //Remove The empty dcm files for Philips data
		{
			filteredFiles << file;
		}
	}
	vtkSmartPointer < vtkStringArray> loadingFiles = vtkSmartPointer < vtkStringArray>::New();
	loadingFiles->SetNumberOfValues(filteredFiles.size());
	for (int i = 0; i<filteredFiles.size(); i++)
	{
		loadingFiles->SetValue(i, filteredFiles.at(i).toStdString().c_str());

	}
	this->m_DicomHelper = new DicomHelper(loadingFiles,this);

	this->sourceImage = this->m_DicomHelper->imageData;

	m_SourceImageMaxSlice = this->sourceImage->GetDimensions()[2] - 1;

	DicomUI->hide();
	
	//4. 
	DisplayDicomInfo(this->sourceImage);

	m_SourceImageCurrentSlice = floor(m_SourceImageMaxSlice/2);
	m_DicomHelper->ComputeCurrentSourceImage(m_SourceImageCurrentSlice);
	

	this->ui->pickTable->clear();
	this->ui->pickTable->setRowCount(1);
	this->ui->pickTable->setColumnCount(1);
	QStringList vtitle; vtitle << tr("Value");
	QStringList htitle; htitle << tr("Source");
	this->ui->pickTable->setVerticalHeaderLabels(vtitle);
	this->ui->pickTable->setHorizontalHeaderLabels(htitle);
	this->ui->pickTable->verticalHeader()->setStretchLastSection(true);
	QTableWidgetItem* valItem = new QTableWidgetItem(QString(" "));
	this->ui->pickTable->setItem(0, 0, valItem);

	const QString orgLabel("Source"); //TO_DO: using Series name/protocol name instead


	if (isUpdate)
	{
		QWidget *orgItem(this->ui->ViewFrame->getWindow(orgLabel));
		disconnect(orgItem, 0, 0, 0);
		this->ui->diffusionModule->Reset();
		this->ui->ViewFrame->removeWindow(orgLabel);
	}

	vuWindowWidget* vtkwindow;
	if (m_DicomHelper->imageDataType.compare("DIFFUSION") == 0)
	{
		qDebug() << "[MainWindow]	Handling diffusion data" << endl ;
		ui->ProcModule->setCurrentIndex(0);
		vtkwindow = new vuWindowWidget(orgLabel, m_DicomHelper->imageData,
			qMakePair<float, float>(m_DicomHelper->scaleSlopeSlice, m_DicomHelper->scaleInterceptSlice), this, true);
		this->ui->ViewFrame->insertWindow(vtkwindow, orgLabel);

		vtkwindow->setMasterComponent("B value: ", m_DicomHelper->BvalueList, m_SourceImageCurrentSlice, m_SourceImageMaxSlice);
		connect(ui->diffusionModule, SIGNAL(signalSaveDcmUpdate(QString, int)), this, SLOT(OnSaveDcmUpdate(QString, int)));
	}
	else if (m_DicomHelper->imageDataType.compare("DCE") == 0)
	{
		//DEVELOPER NOTICE:
		//IMPLEMENT DCE MODULE HERE
		//
	}
	else if (m_DicomHelper->imageDataType.compare("DSC") == 0)
	{
		//DEVELOPER NOTICE:
		//IMPLEMENT DSC MODULE HERE
		//
	}
	else if(m_DicomHelper->imageDataType.compare("OTHERS") == 0)
	{	
		qDebug() << "[MainWindow] Handling View Only data << endl ";
		ui->ProcModule->setCurrentIndex(0);
		vtkwindow = new vuWindowWidget(orgLabel, m_DicomHelper->imageData,
			qMakePair<float, float>(m_DicomHelper->scaleSlopeSlice, m_DicomHelper->scaleInterceptSlice), this, false);
		this->ui->ViewFrame->insertWindow(vtkwindow, orgLabel);
		vtkwindow->loadForBrowsing(m_SourceImageCurrentSlice, m_SourceImageMaxSlice);		
		
	}
	else
	{
		QString MESSAGE;
		QTextStream(&MESSAGE) << tr("Data cannot be recognized.");
		QMessageBox msgBox(QMessageBox::Warning, tr("Unknown Data"), MESSAGE, 0, this);
		msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
		msgBox.exec();
	}

	connect(vtkwindow, SIGNAL(sigChangeWindowLinkStatus(const QString, bool)), this, SLOT(onChangeLinkStatus(const QString, bool)));
	connect(vtkwindow, SIGNAL(sigImageWidgetMouseEvent(QMouseEvent *, const QString)), this, SLOT(onBroadcastEvent(QMouseEvent *, const QString)));
	connect(vtkwindow, SIGNAL(sigImageWidgetSliceChanged(int, const QString)), this, SLOT(onImageWidgetSliceChanged(int, const QString)));
	connect(vtkwindow, SIGNAL(sigPickerValue(QString, float)), this, SLOT(onPickerSig(QString, float)));

	emit SignalSetSourceImage(m_DicomHelper);
	
	if (isUpdate)
	{
		this->ui->roiModule->Reset();
	}
	else
	{
		this->ui->roiModule->setDisplayPort(this->ui->ViewFrame);
	}
}

void MainWindow::DisplayDicomInfo(vtkSmartPointer <vtkImageData> imageData)
{

	const int dataDim = imageData->GetDataDimension();

	int dims[3];
	double origins[3];
	double spacing[3];
	int extent[6];

	double range[2];
	imageData->GetDimensions(dims);
	qDebug() << "image dims: " << dims[0] << "x" << dims[1] << "x" << dims[2] << endl;
	imageData->GetOrigin(origins);
	qDebug() << "image origins: " << origins[0] << " " << origins[1] << " " << origins[2] << endl;
	imageData->GetSpacing(spacing);
	qDebug() << "image spacing: " << spacing[0] << "x" << spacing[1] << "x" << spacing[2] << endl;
	imageData->GetExtent(extent);
	qDebug() << "extent: " << extent[0] << "x" << extent[1] << "x" << extent[2] << "x" << extent[3] << "x" << extent[4] << "x" << extent[5] << endl;
	imageData->GetScalarRange(range);
	qDebug() << "range: " << range[0] << "x" << range[1] << endl;

	vtkMedicalImageProperties* properties = m_DicomHelper->GetDicomReader()->GetMedicalImageProperties();
	QString imageInfo(tr("Patient Name : "));
	imageInfo.append(QLatin1String(properties->GetPatientName()));
	imageInfo.append("\n");
	imageInfo.append(tr("Study Name: "));
	imageInfo.append(QLatin1String(properties->GetStudyDescription()));
	imageInfo.append("\n");
	imageInfo.append(tr("Scan Name: "));
	imageInfo.append(QLatin1String(properties->GetSeriesDescription()));
	imageInfo.append("\n");
	imageInfo.append(tr("Scan Date: "));
	imageInfo.append(QLatin1String(properties->GetAcquisitionDate()));
	imageInfo.append("\n");

	imageInfo.append(tr("Image Dimension: [ %1 : %2 : %3 ]\n").arg(dims[0]).arg(dims[1]).arg(dims[2]));
	ui->infoBrowser->setText(imageInfo);
}

void MainWindow::ShareWindowEvent()
{
	//update source image
	double start = GetTickCount();
	m_DicomHelper->ComputeCurrentSourceImage(m_SourceImageCurrentSlice);
	double end = GetTickCount();
	qDebug() << "compute slice takes time: " << end - start << endl;

	QWidget *orgItem(this->ui->ViewFrame->getWindow(QString("Source")));
	vuWindowWidget *vtkwindow = static_cast <vuWindowWidget*> (orgItem);
	vtkwindow->updateImage(m_DicomHelper->sourceImageSlice, m_SourceImageCurrentSlice);

	m_timeStart = GetTickCount();
	if (m_DicomHelper->imageDataType.compare("OTHERS"))
	{
		//recaculate maps
		qDebug() << "sharewindow Event:" << m_SourceImageCurrentSlice << endl;
		emit SignalRecalcAll();
	}
	//roi block start
	this->ui->roiModule->UpdateLabelImage();
	//roi block stop
}

void MainWindow::onChangeLinkStatus(const QString widgetName, bool status)
{
	if (status && !ActiveWdw.contains(widgetName))
	{
		ActiveWdw << widgetName;
	}
	else
	{
		if (ActiveWdw.contains(widgetName))
		{
			ActiveWdw.removeOne(widgetName);
		}
	}
}

void MainWindow::onWheelWdw(const QString widgetName, int sliceSign, Qt::Orientation orient)
{
	if (sliceSign > 0) {
		if (orient == Qt::Vertical) {
			m_SourceImageCurrentSlice = m_SourceImageCurrentSlice < m_SourceImageMaxSlice ? (m_SourceImageCurrentSlice+1) : m_SourceImageMinSlice;
			ShareWindowEvent();
		}
	}
	else if (sliceSign < 0) {
		if (orient == Qt::Vertical) {
			m_SourceImageCurrentSlice = m_SourceImageCurrentSlice > m_SourceImageMinSlice ? (m_SourceImageCurrentSlice - 1) : m_SourceImageMaxSlice;
			ShareWindowEvent();
		}
	}
}

void MainWindow::onImageWidgetSliceChanged(int slice, const QString ImageName)
{
	// this slot is for slider bar of master window. 
	m_SourceImageCurrentSlice = slice;
	ShareWindowEvent();
}

void MainWindow::onPickerSig(QString itemName, float val)
{
	qDebug() << "Pick on [" << itemName << "] val = " << val << endl;
	if (itemName == "cFA")
	{
		return;
	}
	
	QString valString;
	if (itemName == "ADC"
		|| itemName == "IVIM_D"
		|| itemName == "IVIM_Dstar"
		|| itemName == "AD"
		|| itemName == "RD"
		|| itemName == "sDKI_Dapp"
		)
	{
		valString = QString("%1(um2/s)").arg(val*100000);
	}
	else
	{
		valString = QString("%1").arg(val);
	}

	int curColCount = this->ui->pickTable->columnCount();
	bool noExist(true);

	for (int i = 0; i < curColCount; i++)
	{
		if (this->ui->pickTable->horizontalHeaderItem(i)->text() == itemName)
		{
			qDebug() << "Found :" << itemName << "in the table" << endl;
			this->ui->pickTable->item(0, i)->setText(valString);
			noExist = false;
		}
	}

	QList<QTableWidgetItem*> existItems = this->ui->pickTable->findItems(itemName, Qt::MatchExactly);
	if (noExist)
	{
		qDebug() << "Not Found :" << itemName << "in the table" << endl;
		this->ui->pickTable->setColumnCount(curColCount +1);
		QTableWidgetItem* nameItem = new QTableWidgetItem(QString("%1").arg(itemName));
		this->ui->pickTable->setHorizontalHeaderItem(curColCount, nameItem);
		QTableWidgetItem* valItem = new QTableWidgetItem(valString);
		this->ui->pickTable->setItem(0, curColCount, valItem);
	}

	this->ui->pickTable->viewport()->update();
	//this->ui->pointPick->setText(QString("%1").arg(val));
}

void MainWindow::onBroadcastEvent(QMouseEvent* e, const QString ImageName)
{
	if (ActiveWdw.size() > 1)
	{
		const QEvent::Type t = e->type();
		//qDebug() << "BroadcastEvent event " << e->source() << " accepted? " << e->isAccepted()<< endl;
		QMouseEvent* newEvent;
		newEvent = new QMouseEvent(e->type(), e->localPos(), e->windowPos(),
			e->screenPos(), e->button(), e->buttons(),
			e->modifiers(), Qt::MouseEventSynthesizedByQt);

		for (int i = 0; i < ActiveWdw.size(); ++i) {
			if (ActiveWdw.at(i) != ImageName)
			{
				vuWindowWidget *thisWindow = static_cast <vuWindowWidget*> (ui->ViewFrame->getWindow(ActiveWdw.at(i)));
				//qDebug() << "broadcasting event to " << ActiveWdw.at(i);
				QVTKWidget* thisvtkWidget = thisWindow->getVTKWidget();
				QCoreApplication::sendEvent(thisvtkWidget, newEvent);
			}
		}
	}
}

void MainWindow::onExportImage()
{	
	qDebug() << "[MainWindow] Treating Image Type: " << m_DicomHelper->imageDataType.c_str() << endl;
	if (m_DicomHelper->imageDataType.compare("DIFFUSION") == 0 || m_DicomHelper->imageDataType.compare("OTHERS") == 0)
	{
		if (ActiveWdw.size() < 1)
		{
			info = new QMessageBox(this);
			info->setText(tr("Please at least select one window for export."));
			info->exec();
			return;
		}
		QString debugOut;
		debugOut += tr("Writing Dicom of");
		for (int i = 0; i < ActiveWdw.size(); ++i)
		{
			debugOut += QString(" ");
			debugOut += QString("[") + ActiveWdw.at(i) + QString("]");
		}
		qDebug() << debugOut;

		info = new QMessageBox(this);
		info->setWindowTitle(tr("Export Dicom"));
		info->setText(debugOut);
		info->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		int ret = info->exec();

		if (ret == QMessageBox::Ok)
		{

			QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Save Files to..."), QDir::currentPath()));

			if (directory.size() > 1)
			{
				m_progressD = new QProgressDialog(tr("Writing Files"), tr("Please Wait..."), 0, 100, this, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
				QLabel* progressLabel = new QLabel(tr("Writing Files, Please wait..."));
				m_progressD->setValue(m_progressD->maximum());
				m_progressD->setLabel(progressLabel);
				m_progressD->setWindowModality(Qt::ApplicationModal);
				m_progressD->setMinimumDuration(0);
				m_progressD->setAutoClose(false);
				m_progressD->show();
				ui->diffusionModule->onCalc3D(directory, ActiveWdw, 0);
			}
		}

	}
	else
	{
		//DEVELOPER NOTICE:
		//IMPLEMENT OTHER MODULE
		//
	}
}

void MainWindow::OnSaveDcmUpdate(QString txt, int prog) //This is now a messy general "write" call-back.
{
	if (prog == 100)
	{
		m_progressD->setLabelText(txt);
		m_progressD->setValue(prog);
		m_progressD->setCancelButtonText(tr("Finished"));
		m_progressD->hide();
		m_DicomHelper->ComputeCurrentSourceImage(m_SourceImageCurrentSlice);
	}
	else 
	{
		m_progressD->setLabelText(txt);
		m_progressD->setValue(prog);		
	}
}

void MainWindow::onAboutDialog()
{
	vusionAboutDialog about(this);
	about.exec();
}

void MainWindow::onOpenSettingPage()
{
	settingDialog setdialog(this);
	setdialog.exec();
}
