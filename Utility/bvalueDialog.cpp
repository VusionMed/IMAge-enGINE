// std includes
#include <iostream>

// Qt includes
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDialog>
#include <QMouseEvent>
#include <QGroupBox>
#include <QPushButton>
#include <QProgressDialog>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <qdatetime.h>
#include <QFileDialog>
#include <qdebug.h>


// ctkWidgets includes
#include "bvalueDialog.h"


bValueDialog::bValueDialog(bool mustDTI, QWidget *parent) :
	QDialog(parent)
{
	this->ResolveOK = false;
	this->numOfB = 0;

	gradientDirection = GradientDirectionContainerType::New();
	buttonBox = new QDialogButtonBox(this);

	buttonBox->addButton(tr("Use Files info"), QDialogButtonBox::AcceptRole);
	buttonBox->addButton(tr("Use DICOM info"), QDialogButtonBox::RejectRole);
		//->addButton(;

	bValFrame = new QWidget;
	vecValFrame = new QWidget;
	QVBoxLayout *vlayout = new QVBoxLayout;
	QHBoxLayout *layout = new QHBoxLayout;
	QHBoxLayout *layout2 = new QHBoxLayout;
	//initialize directory from settings, then listen for changes
	int frameStyle = QFrame::Sunken | QFrame::Panel;
	
	bDirButton = new QPushButton(this);
	bDirButton->setText(tr("Choose file describing b values (.bval)"));
	bvalLabel = new QLabel;
	bvalLabel->setFrameStyle(frameStyle);
	if (settings.value("bValueFilePath", "") == "")
	{
		bValFilesPath = QString("./");
		settings.setValue("bValueFilePath", bValFilesPath);
		settings.sync();
	}
	else {
		bValFilesPath = settings.value("bValueFilePath").toString();
	}
	bvalLabel->setText(bValFilesPath);

	dtiCheckBox = new QCheckBox(this);
	dtiCheckBox->setText(tr("DTI Data"));

	vecDirButton = new QPushButton(this);
	vecDirButton->setText(tr("Choose file describing diffusion directions (.bvec)"));//选择弥散方向文件
	vecLabel = new QLabel;
	vecLabel->setFrameStyle(frameStyle);	
	//if (settings.value("bVecFilePath", "") == "")
	//{
	//	bVecFilesPath = QString("./");
	//	settings.setValue("bVecFilePath", bVecFilesPath);
	//	settings.sync();
	//}else{
	//	bVecFilesPath = settings.value("bVecFilePath").toString();
	//}
	//vecLabel->setText(bVecFilesPath);
	
	layout->addWidget(bDirButton);
	layout->addWidget(bvalLabel);
	bValFrame->setLayout(layout);
	
	
	layout2->addWidget(vecDirButton);
	layout2->addWidget(vecLabel);
	vecValFrame->setLayout(layout2);
	
	dispLabel = new QLabel(tr("Please choose .bval .bvec files or clicking cancel for attempting to directly read DICOM meta information."));//

	vlayout->addWidget(dispLabel);
	vlayout->addWidget(bValFrame);
	vlayout->addWidget(dtiCheckBox);
	vlayout->addWidget(vecValFrame);
	vlayout->addWidget(buttonBox);

	this->setLayout(vlayout);
	//setWindowTitle(tr("Remove Roi"));
	this->dtiCheckBox->setTristate(0);

	if (mustDTI)
	{
		this->dtiCheckBox->toggle();
		onIsDTI(1);
		this->dtiCheckBox->setDisabled(true);

		this->dtiCheckBox->setText(tr(".bvec file is required for this data. ")); //
	}
	else {
		this->vecValFrame->setVisible(false);
	}

	setWindowTitle(tr("Diffusion Data, please provide extra information. ")); //

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(this->vecDirButton, SIGNAL(clicked()), this, SLOT(onSetVecFile()));
	connect(this->bDirButton, SIGNAL(clicked()), this, SLOT(onSetbFile()));
	connect(this, SIGNAL(accepted()), this, SLOT(onAccept()));
	connect(this->dtiCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onIsDTI(int)));



	setLayout(vlayout);

}

void bValueDialog::onIsDTI(int status)
{
	if (status == 0)
	{
		this->vecValFrame->setVisible(false);
	}
	else if (status > 0)
	{
		this->vecValFrame->setVisible(true);

		if (settings.value("bVecFilePath", "") == "")
		{
			bVecFilesPath = QString("./");
			settings.setValue("bVecFilePath", bVecFilesPath);
			settings.sync();
		}
		else {
			bVecFilesPath = settings.value("bVecFilePath").toString();
		}
		vecLabel->setText(bVecFilesPath);
	}
}

void bValueDialog::onSetbFile()
{
	//const QFileDialog::Options options = QFlag(fileDialogOptionsWidget->value());
	//QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("importing .bval file"),
		bValFilesPath,
		tr("bVal Files (*.bval);;Text Files (*.txt)"));
	if (!fileName.isEmpty())
	{
		bValFilesPath = fileName;
		bvalLabel->setText(fileName);
	}
}

void bValueDialog::onSetVecFile()
{
	//const QFileDialog::Options options = QFlag(fileDialogOptionsWidget->value());
	//QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Importing .bvec file"),
		bVecFilesPath,
		tr("bVec Files (*.bvec);;Text Files (*.txt)"));
	if (!fileName.isEmpty())
	{
		bVecFilesPath = fileName;
		vecLabel->setText(fileName);
	}
}

bool bValueDialog::resolveFile()
{
	QFile bValueFile(bValFilesPath);
	bool success = bValueFile.open(QIODevice::ReadOnly);
	
	//Hanlding b values.
	QString inString = bValueFile.readAll();
	QStringList inLines = inString.split(QString("\n"),QString::SkipEmptyParts);
	QStringList bvalues = inLines[0].split(QString(" "),QString::SkipEmptyParts);
	bvalues.removeAll("\r"); bvalues.removeAll("\n");
	bValueFile.close();

	//qDebug() << "bvalues = " << bvalues << endl;
	for (int i = 0; i < bvalues.count(); i++)
	{
		bvalueList.push_back(bvalues[i].toFloat());
	}

	QStringList bval = bvalues;
	bval.removeDuplicates();
	this->numOfB = bval.count();
	//qDebug() << "number of b values = " << numOfB;

	//Hanlding direction vectors.
	if (dtiCheckBox->isChecked())
	{
		if (bVecFilesPath.size()<4)
		{			
			return false;
		}
		QFile bVecFile(bVecFilesPath);
		success = bVecFile.open(QIODevice::ReadOnly);

		inString = bVecFile.readAll();
		inLines = inString.split(QString("\n"), QString::SkipEmptyParts);
		bVecFile.close();

		QStringList xDir = inLines[0].split(QString(" "), QString::SkipEmptyParts);
		xDir.removeAll("\r"); xDir.removeAll("\n");
		QStringList yDir = inLines[1].split(QString(" "), QString::SkipEmptyParts);
		yDir.removeAll("\r"); yDir.removeAll("\n");
		QStringList zDir = inLines[2].split(QString(" "), QString::SkipEmptyParts);
		zDir.removeAll("\r"); zDir.removeAll("\n");

		//qDebug() << "dirs = " << inLines.count() << "num of dirs = " << xDir.count();
		//qDebug() << xDir;
		if (xDir.count() == yDir.count() && xDir.count() == zDir.count() && xDir.count() == bvalueList.size())
		{
			for (int i = 0; i < xDir.count(); i++)
			{
				GradientDirectionType temp;
				temp[0] = xDir[i].toDouble();
				temp[1] = yDir[i].toDouble();
				temp[2] = zDir[i].toDouble();
				//qDebug() << "pushing" << i << " :" << temp[0] << "-" << temp[1] << "-" << temp[2];
				gradientDirection->push_back(temp);
			}
		}
		else
		{
			success = false;
		}
	}

	return success;
}

void bValueDialog::onAccept()
{
	if (this->resolveFile())
	{
		settings.setValue("bValueFilePath", bValFilesPath);
		settings.setValue("bVecFilePath", bVecFilesPath);
		emit SignalbValLoaded(bvalueList, numOfB, gradientDirection);
		this->ResolveOK = true;
	}
	else
	{
		QMessageBox msgBox(QMessageBox::Warning, tr("Incorrect Files"), tr("Cannot correctly acquire information from .bval .bvec files. "), 0, this);
		msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
		msgBox.exec();	
		this->ResolveOK = false;
	}
}