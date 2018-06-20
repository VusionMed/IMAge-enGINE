/*=========================================================================

Program:   Vusion ImageEngine
Module:    ROI interaction

=========================================================================*/

#include "qVusionRoiModule.h"
#include "treemodel.h"
#include "vuWindowWidget.h"
#include "ui_qVusionRoiModule.h"

////VTK related included files
#include <vtkImageActor.h>
#include <vtkRenderWindowInteractor.h>
#include "vtkObjectFactory.h"
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkImageAccumulate.h>
#include <vtkBoundedPlanePointPlacer.h>
#include <vtkPlane.h>
#include <vtkRenderWindow.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageToImageStencil.h>
#include <vtkExtractVOI.h>
#include <vtkImageChangeInformation.h>
#include <vtkNrrdReader.h>

//QT related included files
#include <qobject.h>
#include <qdebug.h>

//ITK fast grow cut
#include "itkVTKImageToImageFilter.h"
#include "FastGrowCutSegWrapperITK.h"
#include "vtkImageCast.h"
#include "itkImageToVTKImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkBinaryThresholdImageFilter.h"

void PrintVTKImageDataInfo(vtkImageData *imageData)
{
	if (!imageData) return;

	std::cout << "-------------------------------" << std::endl;
	std::cout << "Printing vtk image data info: " << std::endl;
	std::cout << "-------------------------------" << std::endl;

	int components;
	int dims[3];
	double origins[3];
	double spacing[3];
	int extent[6];
	double range[2];
	double center[3];
	//int index[3];

	components = imageData->GetNumberOfScalarComponents();
	std::cout << "image components: " << components << std::endl;

	imageData->GetDimensions(dims);
	std::cout << "image dims: " << dims[0] << "x" << dims[1] << "x" << dims[2] << std::endl;

	imageData->GetOrigin(origins);
	std::cout << "image origins: " << origins[0] << " " << origins[1] << " " << origins[2] << std::endl;

	imageData->GetSpacing(spacing);
	std::cout << "image spacing: " << spacing[0] << "x" << spacing[1] << "x" << spacing[2] << std::endl;

	imageData->GetExtent(extent);
	std::cout << "extent: " << extent[0] << "x" << extent[1] << "x" << extent[2] << "x" << extent[3] << "x" << extent[4] << "x" << extent[5] << endl;

	imageData->GetScalarRange(range);
	std::cout << "range: " << range[0] << " - " << range[1] << endl;

	double bounds[6];
	imageData->GetBounds(bounds);
	std::cout << "bounds: " << bounds[0] << "x" << bounds[1] << "x" << bounds[2] << "x" << bounds[3] << "x" << bounds[4] << "x" << bounds[5] << endl;

	center[0] = origins[0] + spacing[0] * .5*(extent[0] + extent[1]);
	center[1] = origins[1] + spacing[1] * .5*(extent[2] + extent[3]);
	center[2] = origins[2] + spacing[2] * .5*(extent[4] + extent[5]);
	std::cout << "center: " << center[0] << "x" << center[1] << "x" << center[2] << std::endl;
}

qVusionRoiModule::qVusionRoiModule(QWidget* p) : QWidget(p),
ui(new Ui::qVusionRoiModule)
{
	this->m_lblImgeQ = new PainterUndoRedo();

	qDebug() << "[VusionRoiModule] Initializing...";

	this->ui->setupUi(this);

	this->ui->deleteROIBtn->hide();
	//UI Connections	
	
	connect(ui->statisBrowser, SIGNAL(clicked(QModelIndex)), this, SLOT(onClickTreeView(QModelIndex)));
	
	connect(ui->loadBtn, SIGNAL(clicked()), this, SLOT(onLoadROI()));
	connect(ui->saveBtn, SIGNAL(clicked()), this, SLOT(onSaveROI()));
	
	connect(ui->outputROI, SIGNAL(clicked()), this, SLOT(onSaveRoiToCSV()));
	connect(ui->cnclBtn, SIGNAL(clicked()), this, SLOT(onUndo()));
	connect(ui->rdBtn, SIGNAL(clicked()), this, SLOT(onRedo()));

	//Fast grow cut
	connect(ui->growcutBtn, SIGNAL(clicked()), this, SLOT(onRunFastGrowCut()));

	this->setDisabled(true);
}

qVusionRoiModule::~qVusionRoiModule()
{
	if (this->m_lblImgeQ)
	{
		delete this->m_lblImgeQ;
		this->m_lblImgeQ = nullptr;
	}

}

void qVusionRoiModule::setDisplayPort(DisplayPort* _DP)
{
	m_DP = _DP;

	QWidget *orgItem(m_DP->getWindow(QString("Source")));
	if (!orgItem)
	{
		qDebug() << "[Segmentation] Display Port is set incorrectly !" << endl;
		return;
	}
	this->setEnabled(true);
	
	QStringList headers;
	headers << tr("T") << tr("1") << tr("2") << tr("3") << tr("4") << tr("5");
	m_roiModel = new TreeModel(headers);
	this->ui->statisBrowser->setModel(m_roiModel);

	m_srcWdw = static_cast <vuWindowWidget*> (orgItem);
	connect(ui->editBtn, SIGNAL(toggled(bool)), m_srcWdw, SLOT(onStartManualDraw(bool)));
	connect(ui->brushSpin, SIGNAL(valueChanged(int)), m_srcWdw, SLOT(onBrushSizeChanged(int)));
	connect(ui->brushValCombo, SIGNAL(currentIndexChanged(int)), m_srcWdw, SLOT(onBrushValueChanged(int)));
	connect(ui->contourBtn, SIGNAL(clicked()), this, SLOT(onAddContour()));
	connect(m_srcWdw, SIGNAL(sigSyncPaint(QString)), this, SLOT(onSyncPaint(QString)));
	connect(ui->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(onOpacityChanged(int)));

	this->ui->brushSpin->valueChanged(5);

	//1. Create a new label image and push it to the source window 
	vtkImageData *labelImage = vtkImageData::New();
	labelImage->SetDimensions(this->m_srcWdw->GetImageViewer()->GetBackgroundImage()->GetDimensions());
	labelImage->SetSpacing(this->m_srcWdw->GetImageViewer()->GetBackgroundImage()->GetSpacing());
	labelImage->SetOrigin(this->m_srcWdw->GetImageViewer()->GetBackgroundImage()->GetOrigin());
	labelImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
	memset((unsigned char *)(labelImage->GetScalarPointer()), 0, labelImage->GetScalarSize()*labelImage->GetNumberOfPoints());
	this->m_srcWdw->setLabelImage3D(labelImage);

	this->m_lblImgeQ->AddToList(m_srcWdw->GetImageViewer()->GetLabelImage());
	//qDebug() << "[VusionRoiModule] Push the label image to Stack, now the stack points at" << this->m_lblImgeQ->m_CurrentIndex << endl;
	this->ui->cnclBtn->setEnabled(this->m_lblImgeQ->UndoAllowed());
	this->ui->rdBtn->setEnabled(this->m_lblImgeQ->RedoAllowed());
}

void qVusionRoiModule::onAddContour()
{
	
	this->ui->editBtn->blockSignals(true);
	qDebug() << "rewinding new ROI button" << endl;
	this->ui->editBtn->setChecked(false);
	this->ui->editBtn->blockSignals(false);
	m_srcWdw->onStartContour();
}

void qVusionRoiModule::onSyncPaint(QString wdwName)
{
	qDebug() << "[VusionRoiModule] Paint on " << wdwName << "is finished" << endl;

	//1. Add edited label image to List
	if (wdwName == "Source")
	{
		//qDebug() << "[VusionRoiModule] Paint before pushing: " << this->m_lblImgeQ->m_CurrentIndex << endl;
		this->m_lblImgeQ->AddToList(m_srcWdw->GetImageViewer()->GetLabelImage());
		//qDebug() << "[VusionRoiModule] Paint after pushing: " << this->m_lblImgeQ->m_CurrentIndex << endl;
	}
	this->ui->cnclBtn->setEnabled(this->m_lblImgeQ->UndoAllowed());
	this->ui->rdBtn->setEnabled(this->m_lblImgeQ->RedoAllowed());

	//2. Update computing Window
	if (m_DP->getAllWindow().count() > 1)
		UpdateLabelImage();
}

void qVusionRoiModule::onSaveROI()
{
	if (m_srcWdw->GetImageViewer()->GetLabelImage()->GetScalarRange()[1] < 1)
	{
		info = new QMessageBox(this);
		info->setWindowTitle(tr("Please Draw ROI"));
		info->setText(tr("Please Draw at least one ROI at the Source window"));
		info->setStandardButtons(QMessageBox::Ok);
		info->exec();
		return;
	}

	//QString FileName = QFileDialog::getSaveFileName(this,
	//	tr("Save Label to..."),
	//	QDir::currentPath(),
	//	tr("Image (*.nrrd)")	
	//	);
	//
	//if (FileName.size() > 1)
	//{
	//	vtkMatrix4x4* mat = vtkMatrix4x4::New();
	//	mat->SetElement(0, 0, 1);
	//	mat->SetElement(1, 1, 1);
	//	mat->SetElement(2, 2, this->m_srcWdw->getImageData()->GetSpacing()[2]);

	//	vtkSmartPointer<vtkNRRDWriter> writer = vtkSmartPointer<vtkNRRDWriter>::New();
	//	writer->SetInputData(m_srcWdw->GetImageViewer()->GetLabelImage());
	//	writer->SetFileName(FileName.toStdString().c_str());
	//	writer->SetIJKToRASMatrix(mat);
	//	writer->UseCompressionOff();
	//	writer->Write();
	//}

}

void qVusionRoiModule::onLoadROI()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Load Label..."),
		QDir::currentPath(),
		tr("Image (*.nrrd)")
		);
	if (fileName.size() > 1)
	{

		vtkSmartPointer<vtkNrrdReader> reader = vtkSmartPointer<vtkNrrdReader>::New();
		if (reader->CanReadFile(fileName.toStdString().c_str()))
		{
			reader->SetFileName(fileName.toStdString().c_str());
			reader->Update();

			if (abs(reader->GetOutput()->GetSpacing()[2] - m_srcWdw->getImageData()->GetSpacing()[2]) > 0.08)
			{
				info = new QMessageBox(this);
				info->setWindowTitle(tr("Incompatible ROI"));
				info->setText(tr("The imported ROI was drawn on image with different slice thickness! "));
				info->setStandardButtons(QMessageBox::Ok);
				int ret = info->exec();
				return;
			}
			vtkImageData *labelImage = vtkImageData::New();
			labelImage->DeepCopy(reader->GetOutput());
			this->m_srcWdw->setLabelImage3D(labelImage);
			
			//int opa = this->ui->opacitySlider->value();
			//m_srcWdw->setLabelImageOpacity(opa / 100.0);
			//m_srcWdw->GetImageViewer()->SetImageLayerOpacity(2, opa / 100.0);
			//m_srcWdw->GetImageViewer()->Render();

			this->m_lblImgeQ->AddToList(m_srcWdw->GetImageViewer()->GetLabelImage());		
			this->ui->cnclBtn->setEnabled(this->m_lblImgeQ->UndoAllowed());
			this->ui->rdBtn->setEnabled(this->m_lblImgeQ->RedoAllowed());
			_modifiedLabelImage();
		}
	}
}

void qVusionRoiModule::onSaveRoiToCSV()
{
	QStringList childrenOne = m_roiModel->getrootItem()->listChild();
	if (childrenOne.count() < 1)
	{
		info = new QMessageBox(this);
		info->setWindowTitle(tr("Data Select ERROR"));
		info->setText(tr("No eligible ROI data for exporting."));
		//info->setWindowTitle(QString::fromLocal8Bit("Ã»ÓÐROIÊý¾Ý"));
		//info->setText(QString::fromLocal8Bit("Ã»ÓÐROIÊý¾Ý¿É¹©µ¼³ö"));
		info->setStandardButtons(QMessageBox::Ok);
		int ret = info->exec();
		return;
	}

	QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Save ROI statistics to..."), QDir::currentPath()));
	if (directory.count() > 0)
	{
		QDateTime current_date_time = QDateTime::currentDateTime();
		//QString current_date = current_date_time.toString("yyyy-MM-dd");
		QString current_time = current_date_time.toString("yyyyMMdd_hhmmss_");
		QFile file(directory + QString("\\") + current_time + QString("ROI.csv"));

		qDebug() << "Output .csv to: " << directory + QString("\\") + current_time + QString("ROI.csv") << endl;

		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
			return;

		QTextStream out(&file);

		childrenOne.removeAll("Source");

		foreach(QString childOne, childrenOne)
		{
			TreeItem* child1 = m_roiModel->getrootItem()->child(childOne);
			out << child1->data(0).toString() << "," << child1->data(1).toString() << "," << child1->data(2).toString()
				<< "," << child1->data(3).toString() << "," << child1->data(4).toString() << "," << child1->data(5).toString()
				<< "\n";
			QStringList childrenTwo = child1->listChild();
			foreach(QString childTwo, childrenTwo)
			{
				TreeItem* child2 = child1->child(childTwo);
				out << child2->data(0).toString() << "," << child2->data(1).toString() << "," << child2->data(2).toString()
					<< "," << child2->data(3).toString() << "," << child2->data(4).toString() << "," << child2->data(5).toString()
					<< "\n";
			}

			out << "\n";
		}
		file.close();
	}
	else
	{
		return;
	}
}

void qVusionRoiModule::onOpacityChanged(int opa)
{
	QHash < const QString, QWidget * > currentWindows = m_DP->getAllWindow();
	QHashIterator<const QString, QWidget * > wdwIter(currentWindows);

	while (wdwIter.hasNext())
	{
		wdwIter.next();
		vuWindowWidget *thisWindow = static_cast <vuWindowWidget*> (wdwIter.value());
		thisWindow->setLabelImageOpacity(opa / 100.0);
		thisWindow->GetImageViewer()->SetImageLayerOpacity(2,opa/100.0);
		thisWindow->GetImageViewer()->Render();
	}	
}

void qVusionRoiModule::onClickTreeView(const QModelIndex &index)
{
	//QModelIndex index = ui->statisBrowser->selectionModel()->currentIndex();
	//TreeModel *model = static_cast<TreeModel *>(ui->view->model());			
	curRoiDataindex = index;

	QString thisItemName = m_roiModel->data(index, 0).toString();
	qDebug() << "clicking at : " << thisItemName;
	QModelIndex tempInd(index);
	int treeLevel(0);
	while (tempInd.parent().isValid())
	{
		treeLevel++;
		tempInd = tempInd.parent();
		qDebug() << "and its father's name is : " << m_roiModel->data(tempInd, 0).toString();
	}

	m_roiModel->setRepsDefault();

}

void qVusionRoiModule::onUndo()
{
	//qDebug() << "[VusionRoiModule] Undo Last ROI change" << endl;

	if (this->m_lblImgeQ->m_CurrentIndex < 1) return;

	//qDebug() << "[VusionRoiModule] before updating index " << this->m_lblImgeQ->m_CurrentIndex << endl;
	this->m_lblImgeQ->DecrementCurrentIndex();

	this->_modifiedLabelImage();
	//qDebug() << "[VusionRoiModule] after updating index " << this->m_lblImgeQ->m_CurrentIndex << endl;
}

void qVusionRoiModule::onRedo()
{
	qDebug() << "[VusionRoiModule] Redo Last ROI change" << endl;

	if (this->m_lblImgeQ->m_CurrentIndex == this->m_lblImgeQ->m_EditedLabelImageList.size() - 1) return;

	this->m_lblImgeQ->IncrementCurrentIndex();

	//int currentIndex = this->m_lblImgeQ->m_CurrentIndex;
	this->_modifiedLabelImage();
}

void qVusionRoiModule::_modifiedLabelImage()
{
	//1. Update Source Window
	int currentIndex = this->m_lblImgeQ->m_CurrentIndex;
	if (currentIndex == -1) return;

	double start = GetTickCount();
	qDebug() << "[VusionRoiModule] Updating image using index " << currentIndex;
	
	vtkSmartPointer<vtkImageData> tmpImage = vtkSmartPointer<vtkImageData>::New();
	tmpImage->DeepCopy(this->m_lblImgeQ->m_EditedLabelImageList.at(currentIndex));
	m_srcWdw->setLabelImage3D(tmpImage);

	//2. Check undo redo button availability
	this->ui->cnclBtn->setEnabled(this->m_lblImgeQ->UndoAllowed());
	this->ui->rdBtn->setEnabled(this->m_lblImgeQ->RedoAllowed());

	//3. Update computing Window
	if (m_DP->getAllWindow().count() > 1)
		UpdateLabelImage();

}

void qVusionRoiModule::UpdateLabelImage() //Only Update 2D
{

	double start = GetTickCount();

	//1. Slice the label Image;
	vtkSmartPointer<vtkImageData> tmpImage = vtkSmartPointer<vtkImageData>::New();
	tmpImage->DeepCopy(this->m_srcWdw->GetImageViewer()->GetLabelImage());
	
	int slice = m_srcWdw->getCurrentSlice();
	vtkSmartPointer <vtkExtractVOI> ExtractVOI = vtkSmartPointer <vtkExtractVOI>::New();
	ExtractVOI->SetInputData(tmpImage);
	ExtractVOI->SetVOI(0, tmpImage->GetDimensions()[0] - 1, 0, tmpImage->GetDimensions()[0] - 1, slice, slice);
	ExtractVOI->Update();

	vtkSmartPointer <vtkImageChangeInformation> changeInfo = vtkSmartPointer <vtkImageChangeInformation>::New();
	changeInfo->SetInputData(ExtractVOI->GetOutput());
	changeInfo->SetOutputOrigin(0, 0, 0);
	changeInfo->SetExtentTranslation(0, 0, -slice);
	changeInfo->Update();

	//vtkSmartPointer<vtkImageData> labelImage2D = vtkSmartPointer<vtkImageData>::New();
	//labelImage2D->DeepCopy(changeInfo->GetOutput());
	double end1 = GetTickCount();
	qDebug() <<"[vusionRoiModule] Slice ROI takes time: " << end1 - start << endl;

	//2. Update All Other Windows

	bool update(false);
	QHash < const QString, QWidget * > currentWindows = m_DP->getAllWindow();
	QHashIterator<const QString, QWidget * > wdwIter(currentWindows);
	while (wdwIter.hasNext())
	{
		wdwIter.next();
		vuWindowWidget *thisWindow = static_cast <vuWindowWidget*> (wdwIter.value());
		QString wdwName = thisWindow->getImageName();
		if (wdwName != "Source")
		{
			double startLabel  = GetTickCount();
			thisWindow->setLabelImage2D(changeInfo->GetOutput(), slice);
			double endLabel = GetTickCount();
			qDebug() << "[vusionRoiModule] Set LabelImage2D takes time: " << endLabel - startLabel << endl;

			update = _UpdateStatisRoi(thisWindow);
			update = true;
		}
	}

	//
	//3. Update Statis Browswer
	//this->UpdateStatis();
	if (update)
	{
		qDebug() << "[vusionRoiModule] ROI tree view updated" << endl;
		this->ui->statisBrowser->expandAll();
		//this->ui->statisBrowser->collapseAll();
	}

	double end = GetTickCount();
	qDebug() << "[vusionRoiModule] Update ROI takes time: " << end - start << endl;
}

bool qVusionRoiModule::_UpdateStatisRoi(vuWindowWidget* thisWdw)
{
	double start = GetTickCount();

	bool updated = false;

	vtkImageData* lblImage = thisWdw->GetImageViewer()->GetLabelImage();
	vtkImageData* calcImage = thisWdw->GetImageViewer()->GetBackgroundImage();

	int z = m_srcWdw->getCurrentSlice();
	QString curSlice = QString("%1").arg(m_srcWdw->getCurrentSlice() + 1);
	//qDebug() << "[VusionRoiModule] Updating Slice " << curSlice;
	//qDebug() << "[VusionRoiModule] LabelImage range is " << lblImage->GetScalarRange()[0] << " ~ " << lblImage->GetScalarRange()[1] << endl;	
	//qDebug() << "[VusionRoiModule] CalcImage Spacing is: "<< calcImage->GetSpacing()[2] << endl;
	//qDebug() << "[VusionRoiModule] LblImage Value range is: " << lblImage->GetScalarRange()[0] << " ~ " << lblImage->GetScalarRange()[1] << endl;

	for (int i = 1; i < 10; i++)
	{
		
		QString RoiName = QString("ROI%1").arg(i);
		//qDebug() << "[VusionRoiModule] Treating " << RoiName << endl;
		
		bool updateROi = ( m_roiModel->isRoiPresent(RoiName, QString("%1").arg(m_srcWdw->getCurrentSlice() + 1)) == 2);
		//qDebug() << "[VusionRoiModule] Updating ROI? " << updateROi << endl;

		vtkSmartPointer<vtkImageToImageStencil> imageStencil = vtkImageToImageStencil::New();
		imageStencil->SetInputData(lblImage);
		imageStencil->ThresholdBetween(i, i);
		imageStencil->Update();

		//vtkImageStencilData* stencilData = imageStencil->GetOutput();
		//vtkIndent indent(2);
		//stencilData->PrintSelf(cout, indent);

		vtkSmartPointer<vtkImageAccumulate> imageAccumulate = vtkSmartPointer<vtkImageAccumulate>::New();
		imageAccumulate->SetStencilData(imageStencil->GetOutput());
		imageAccumulate->SetInputData(calcImage);
		imageAccumulate->Update();

		if (imageAccumulate->GetVoxelCount() > 0)
		{
			float scalingValue = thisWdw->getImageScale().first;
			float shiftValue = thisWdw->getImageScale().second;

			float thisArea = imageAccumulate->GetVoxelCount()*calcImage->GetSpacing()[0] * calcImage->GetSpacing()[1] * calcImage->GetSpacing()[2];
			float thisMean = *imageAccumulate->GetMean()*scalingValue + shiftValue;
			float thisstd = *imageAccumulate->GetStandardDeviation()*scalingValue + shiftValue;
			float thisMin = *imageAccumulate->GetMin()*scalingValue + shiftValue;
			float thisMax = *imageAccumulate->GetMax()*scalingValue + shiftValue;

			if (thisWdw->getImageName() == "ADC" ||
				thisWdw->getImageName() == "IVIM_D" ||
				thisWdw->getImageName() == "IVIM_Dstar" ||
				thisWdw->getImageName() == "sDKI_Dapp"
				)
			{
				thisMean = thisMean * 1000000;
				thisstd = thisstd * 1000000;
				thisMin = thisMin * 1000000;
				thisMax = thisMax * 1000000;
			}
			//qDebug() << "[VusionRoiModule] Area: " << thisArea << " Mean: " << thisMean << " Std: " << thisstd << " Min: " << thisMin << " Max: " << thisMax << endl;

			QStringList newRow;
			newRow << QString("%1").arg(thisArea) << QString("%1").arg(thisMean) << QString("%1").arg(thisstd) << QString("%1").arg(thisMin) << QString("%1").arg(thisMax);
			m_roiModel->insertROIs(newRow, this->ui->brushValCombo->itemText(i), QString("%1").arg(m_srcWdw->getCurrentSlice() + 1), thisWdw->getImageName());

			updated = true;
		}
		else if(updateROi)
		{
			//qDebug() << "Remove ROIS" << RoiName << "of window: " << thisWdw->getImageName() << " on slice: " << QString("%1").arg(m_srcWdw->getCurrentSlice() + 1)<< endl;
			m_roiModel->removeROIs(RoiName, m_srcWdw->getCurrentSlice());

			updated = true;
		}
	}

	double end = GetTickCount();
	qDebug() << "[vusionRoiModule] Update One ROI Statistics takes time: " << end - start << endl;

	return updated;
}

void qVusionRoiModule::Reset()
{	
	qDebug() << " [VusionRoiModule] Reset Module;" << endl;	
	this->ui->editBtn->setChecked(false);
	

	int rows = m_roiModel->rowCount();
	m_roiModel->removeRows(0, rows);
	
	if (this->m_lblImgeQ->m_EditedLabelImageList.count() > 0)
	{
		this->m_lblImgeQ->m_EditedLabelImageList.clear();
		this->m_lblImgeQ->m_CurrentIndex = -1;
	}

	QWidget *orgItem(m_DP->getWindow(QString("Source")));
	if (!orgItem)
	{
		qDebug() << "[Segmentation] Cannot Get Source Window !" << endl;
		return;
	}
	m_srcWdw = static_cast <vuWindowWidget*> (orgItem);
	connect(ui->editBtn, SIGNAL(toggled(bool)), m_srcWdw, SLOT(onStartManualDraw(bool)));
	connect(ui->brushSpin, SIGNAL(valueChanged(int)), m_srcWdw, SLOT(onBrushSizeChanged(int)));
	connect(ui->brushValCombo, SIGNAL(currentIndexChanged(int)), m_srcWdw, SLOT(onBrushValueChanged(int)));
	connect(ui->contourBtn, SIGNAL(clicked()), this, SLOT(onAddContour()));
	connect(m_srcWdw, SIGNAL(sigSyncPaint(QString)), this, SLOT(onSyncPaint(QString)));
	connect(ui->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(onOpacityChanged(int)));

	this->ui->brushValCombo->setCurrentIndex(0);
	this->ui->brushSpin->valueChanged(5);

	vtkImageData *labelImage = vtkImageData::New();
	labelImage->SetDimensions(this->m_srcWdw->GetImageViewer()->GetBackgroundImage()->GetDimensions());
	labelImage->SetSpacing(this->m_srcWdw->GetImageViewer()->GetBackgroundImage()->GetSpacing());
	labelImage->SetOrigin(this->m_srcWdw->GetImageViewer()->GetBackgroundImage()->GetOrigin());
	labelImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
	memset((unsigned char *)(labelImage->GetScalarPointer()), 0, labelImage->GetScalarSize()*labelImage->GetNumberOfPoints());
	this->m_srcWdw->setLabelImage3D(labelImage);

	this->m_lblImgeQ->AddToList(m_srcWdw->GetImageViewer()->GetLabelImage());
	qDebug() << "[VusionRoiModule] Push the label image to Stack, now the stack points at" << this->m_lblImgeQ->m_CurrentIndex << endl;
	this->ui->cnclBtn->setEnabled(this->m_lblImgeQ->UndoAllowed());
	this->ui->rdBtn->setEnabled(this->m_lblImgeQ->RedoAllowed());

	this->ui->cnclBtn->setEnabled(this->m_lblImgeQ->UndoAllowed());
	this->ui->rdBtn->setEnabled(this->m_lblImgeQ->RedoAllowed());

}

bool qVusionRoiModule::isRadiomicsReady()
{
	return this->m_roiModel->isRoiPresent("ROI1");
}

void qVusionRoiModule::onRunFastGrowCut()
{
	if (!this->m_srcWdw )
	{
		std::cerr << " " << std::endl;
		return;
	}

	vtkVusionMultiView *multiView = this->m_srcWdw->GetImageViewer();
	if (!multiView)
	{
		std::cerr << " " << std::endl;
		return;
	}

	vtkImageData* backgroundImage = multiView->GetBackgroundImage();
	vtkImageData *labelImage = multiView->GetLabelImage();
	if(!backgroundImage || !labelImage)
	{
		std::cerr << " " << std::endl;
		return;
	}

	//qDebug() << "[GrowCut] Runnnnn" << endl;

	typedef itk::Image<unsigned char, 3> LabelImageType;
	typedef itk::Image<short, 3> FGCImageType;
	typedef itk::VTKImageToImageFilter <LabelImageType>	VtkToItkConverterType;
	VtkToItkConverterType::Pointer vtkToItkImageFilter_label = VtkToItkConverterType::New();
	vtkToItkImageFilter_label->SetInput(labelImage);
	vtkToItkImageFilter_label->Update();
	//std::cout << " ------------------------------02" << std::endl;

	vtkSmartPointer <vtkImageCast> vtkCastImageFilter_back = vtkSmartPointer<vtkImageCast>::New();
	vtkCastImageFilter_back->SetInputData(backgroundImage);
	vtkCastImageFilter_back->SetOutputScalarTypeToShort();
	vtkCastImageFilter_back->Update();

	typedef itk::VTKImageToImageFilter <FGCImageType>	VtkToItkConverterType_back;
	VtkToItkConverterType_back::Pointer vtkToItkImageFilter_back = VtkToItkConverterType_back::New();
	vtkToItkImageFilter_back->SetInput(vtkCastImageFilter_back->GetOutput());
	vtkToItkImageFilter_back->Update();

	//verify label image here:
	bool findInsidePixel = false;
	bool findOutsidePixel = false;
	const unsigned int InsidePixel = this->ui->brushValCombo->currentIndex();
	qDebug() << "[GrowCut] inside value ="<<InsidePixel << endl;	
	const unsigned int OutsidePixel = 9;
	typedef itk::ImageRegionIterator <LabelImageType>	LabelImageIteratorType;
	LabelImageIteratorType labelIt(vtkToItkImageFilter_label->GetOutput(), vtkToItkImageFilter_label->GetOutput()->GetLargestPossibleRegion());
	labelIt.GoToBegin();
	while (!labelIt.IsAtEnd())
	{
		if (labelIt.Get() == InsidePixel)
		{
			findInsidePixel = true;
		}
		else if (labelIt.Get() == OutsidePixel)
		{
			findOutsidePixel = true;
		}
		else
		{
			labelIt.Set(0);
		}
		++labelIt;
	}
	
	if (InsidePixel == 9)
	{

		info = new QMessageBox(this);
		info->setWindowTitle(tr("Please draw INNER ROI"));
		info->setText(tr("Please use ROI1 ~ ROI8 to label inside of the interested region."));
		info->setStandardButtons(QMessageBox::Ok);
		info->exec();
		return;

	}

	if (findOutsidePixel == false)
	{
		info = new QMessageBox(this);
		info->setWindowTitle(tr("Please draw OUTTER ROI"));
		info->setText(tr("Please use ROI9 to label outside of the interested region."));
		info->setStandardButtons(QMessageBox::Ok);
		info->exec();
		return;
	}

	//FastGrowCut
	FGC::FastGrowCutWrapperITK<FGCImageType::PixelType, LabelImageType::PixelType> *fastGrowCut = \
		new FGC::FastGrowCutWrapperITK<FGCImageType::PixelType, LabelImageType::PixelType>();
	fastGrowCut->SetSourceVol(vtkToItkImageFilter_back->GetOutput());
	fastGrowCut->SetSeedVol(vtkToItkImageFilter_label->GetOutput());
	fastGrowCut->Initialization();
	fastGrowCut->RunFGC();

	typedef itk::BinaryThresholdImageFilter<LabelImageType, LabelImageType >  BinaryThresholdFilterType;
	BinaryThresholdFilterType::Pointer binaryThresholdFilter = BinaryThresholdFilterType::New();
	binaryThresholdFilter->SetInput(fastGrowCut->GetSeedVol());
	binaryThresholdFilter->SetLowerThreshold(InsidePixel);
	binaryThresholdFilter->SetUpperThreshold(InsidePixel);
	binaryThresholdFilter->SetOutsideValue(0);
	binaryThresholdFilter->SetInsideValue(InsidePixel);
	binaryThresholdFilter->Update();

	typedef itk::ImageToVTKImageFilter <LabelImageType> itkToVtkConverter;
	itkToVtkConverter::Pointer convItkToVtk = itkToVtkConverter::New();
	convItkToVtk->SetInput(binaryThresholdFilter->GetOutput());
	convItkToVtk->Update();

	vtkSmartPointer<vtkImageData> tmpImage = vtkSmartPointer<vtkImageData>::New();
	tmpImage->DeepCopy(convItkToVtk->GetOutput());

	m_srcWdw->setLabelImage3D(tmpImage);

	onSyncPaint("Source");

	std::cout << " ------------------------------Release" << std::endl;
	delete fastGrowCut;
	fastGrowCut = nullptr;
}
