/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

// std includes
#include <iostream>

// Qt includes
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QStringListModel>
#include <QWidgetAction>

// ctkWidgets includes
#include "ctkDirectoryButton.h"
#include "ctkFileDialog.h"
#include "ctkMessageBox.h"

// ctkDICOMCore includes
#include "ctkDICOMDatabase.h"
#include "ctkDICOMIndexer.h"

// ctkDICOMWidgets includes
#include "vuDICOMBrowser.h"
#include "ctkDICOMQueryResultsTabWidget.h"
#include "ctkDICOMQueryRetrieveWidget.h"
#include "ctkDICOMQueryWidget.h"
#include "ctkDICOMTableManager.h"

#include "ui_vuDICOMBrowser.h"

//logger
#include <ctkLogger.h>
static ctkLogger logger("vuDICOMBrowser");

//----------------------------------------------------------------------------
class vuDICOMBrowserPrivate: public Ui_vuDICOMBrowser
{
public:
  vuDICOMBrowser* const q_ptr;
  Q_DECLARE_PUBLIC(vuDICOMBrowser);

  vuDICOMBrowserPrivate(vuDICOMBrowser* );
  ~vuDICOMBrowserPrivate();

  ctkFileDialog* ImportDialog;
  ctkDICOMQueryRetrieveWidget* QueryRetrieveWidget;

  QSharedPointer<ctkDICOMDatabase> DICOMDatabase;
  QSharedPointer<ctkDICOMIndexer> DICOMIndexer;
  QProgressDialog *IndexerProgress;
  QProgressDialog *UpdateSchemaProgress;
  QProgressDialog *ExportProgress;

  void showIndexerDialog();
  void showUpdateSchemaDialog();

  // used when suspending the ctkDICOMModel
  QSqlDatabase EmptyDatabase;

  // local count variables to keep track of the number of items
  // added to the database during an import operation
  bool DisplayImportSummary;
  int PatientsAddedDuringImport;
  int StudiesAddedDuringImport;
  int SeriesAddedDuringImport;
  int InstancesAddedDuringImport;
};

//----------------------------------------------------------------------------
// vuDICOMBrowserPrivate methods

vuDICOMBrowserPrivate::vuDICOMBrowserPrivate(vuDICOMBrowser* parent): q_ptr(parent){
  DICOMDatabase = QSharedPointer<ctkDICOMDatabase> (new ctkDICOMDatabase);
  DICOMIndexer = QSharedPointer<ctkDICOMIndexer> (new ctkDICOMIndexer);
  IndexerProgress = 0;
  UpdateSchemaProgress = 0;
  ExportProgress = 0;
  DisplayImportSummary = true;
  PatientsAddedDuringImport = 0;
  StudiesAddedDuringImport = 0;
  SeriesAddedDuringImport = 0;
  InstancesAddedDuringImport = 0;
}

vuDICOMBrowserPrivate::~vuDICOMBrowserPrivate()
{
  if ( IndexerProgress )
    {
    delete IndexerProgress;
    }
  if ( UpdateSchemaProgress )
    {
    delete UpdateSchemaProgress;
    }
  if ( ExportProgress )
    {
    delete ExportProgress;
    }
}

void vuDICOMBrowserPrivate::showUpdateSchemaDialog()
{
  Q_Q(vuDICOMBrowser);
  if (UpdateSchemaProgress == 0)
    {
    //
    // Set up the Update Schema Progress Dialog
    //
    UpdateSchemaProgress = new QProgressDialog(
        q->tr("DICOM Schema Update"), "Cancel", 0, 100, q,
         Qt::WindowTitleHint | Qt::WindowSystemMenuHint);

    // We don't want the progress dialog to resize itself, so we bypass the label
    // by creating our own
    QLabel* progressLabel = new QLabel(q->tr("Initialization..."));
    UpdateSchemaProgress->setLabel(progressLabel);
    UpdateSchemaProgress->setWindowModality(Qt::ApplicationModal);
    UpdateSchemaProgress->setMinimumDuration(0);
    UpdateSchemaProgress->setValue(0);

    q->connect(DICOMDatabase.data(), SIGNAL(schemaUpdateStarted(int)),
            UpdateSchemaProgress, SLOT(setMaximum(int)));
    q->connect(DICOMDatabase.data(), SIGNAL(schemaUpdateProgress(int)),
            UpdateSchemaProgress, SLOT(setValue(int)));
    q->connect(DICOMDatabase.data(), SIGNAL(schemaUpdateProgress(QString)),
            progressLabel, SLOT(setText(QString)));

    // close the dialog
    q->connect(DICOMDatabase.data(), SIGNAL(schemaUpdated()),
            UpdateSchemaProgress, SLOT(close()));
    }
  UpdateSchemaProgress->show();
}

void vuDICOMBrowserPrivate::showIndexerDialog()
{
  Q_Q(vuDICOMBrowser);
  if (IndexerProgress == 0)
    {
    //
    // Set up the Indexer Progress Dialog
    //
    IndexerProgress = new QProgressDialog( q->tr("DICOM Import"), "Cancel", 0, 100, q,
         Qt::WindowTitleHint | Qt::WindowSystemMenuHint);

    // We don't want the progress dialog to resize itself, so we bypass the label
    // by creating our own
    QLabel* progressLabel = new QLabel(q->tr("Initialization..."));
    IndexerProgress->setLabel(progressLabel);
    IndexerProgress->setWindowModality(Qt::ApplicationModal);
    IndexerProgress->setMinimumDuration(0);
    IndexerProgress->setValue(0);

    q->connect(IndexerProgress, SIGNAL(canceled()), 
                 DICOMIndexer.data(), SLOT(cancel()));

    q->connect(DICOMIndexer.data(), SIGNAL(progress(int)),
            IndexerProgress, SLOT(setValue(int)));

    q->connect(DICOMIndexer.data(), SIGNAL(indexingFilePath(QString)),
            progressLabel, SLOT(setText(QString)));
    q->connect(DICOMIndexer.data(), SIGNAL(indexingFilePath(QString)),
            q, SLOT(onFileIndexed(QString)));

    // close the dialog
    q->connect(DICOMIndexer.data(), SIGNAL(indexingComplete()),
            IndexerProgress, SLOT(close()));
    // stop indexing and reset the database if canceled
    q->connect(IndexerProgress, SIGNAL(canceled()), 
            DICOMIndexer.data(), SLOT(cancel()));

    // allow users of this widget to know that the process has finished
    q->connect(IndexerProgress, SIGNAL(canceled()), 
            q, SIGNAL(directoryImported()));
    q->connect(DICOMIndexer.data(), SIGNAL(indexingComplete()),
            q, SIGNAL(directoryImported()));
    }
  IndexerProgress->show();
}

//----------------------------------------------------------------------------
// vuDICOMBrowser methods

//----------------------------------------------------------------------------
vuDICOMBrowser::vuDICOMBrowser(QWidget* _parent):Superclass(_parent),
    d_ptr(new vuDICOMBrowserPrivate(this))
{
  Q_D(vuDICOMBrowser);

  d->setupUi(this);

  // signals related to tracking inserts
  connect(d->DICOMDatabase.data(), SIGNAL(patientAdded(int,QString,QString,QString)), this,
                              SLOT(onPatientAdded(int,QString,QString,QString)));
  connect(d->DICOMDatabase.data(), SIGNAL(studyAdded(QString)), this, SLOT(onStudyAdded(QString)));
  connect(d->DICOMDatabase.data(), SIGNAL(seriesAdded(QString)), this, SLOT(onSeriesAdded(QString)));
  connect(d->DICOMDatabase.data(), SIGNAL(instanceAdded(QString)), this, SLOT(onInstanceAdded(QString)));

  connect(d->tableDensityComboBox ,SIGNAL(currentIndexChanged (const QString&)),
     this, SLOT(onTablesDensityComboBox(QString)));

  //Set ToolBar button style
//  d->ToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

  //Initialize Q/R widget
  d->QueryRetrieveWidget = new ctkDICOMQueryRetrieveWidget();
  d->QueryRetrieveWidget->setWindowModality ( Qt::ApplicationModal );

  //initialize directory from settings, then listen for changes
  QSettings settings;
  if (settings.value("DatabaseDirectory", "") == "")
  {
	  QString directory = QString("./");
	  settings.setValue("DatabaseDirectory", directory);
	  settings.sync();
  }

  QString databaseDirectory = settings.value("DatabaseDirectory").toString();
  this->setDatabaseDirectory(databaseDirectory);
  d->DirectoryButton->setDirectory(databaseDirectory);

  d->dicomTableManager->setDICOMDatabase(d->DICOMDatabase.data());

  // TableView signals
  connect(d->dicomTableManager, SIGNAL(patientsSelectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(onModelSelected(const QItemSelection&,const QItemSelection&)));
  connect(d->dicomTableManager, SIGNAL(studiesSelectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(onModelSelected(const QItemSelection&,const QItemSelection&)));
  connect(d->dicomTableManager, SIGNAL(seriesSelectionChanged(const QItemSelection&, const QItemSelection&)),
          this, SLOT(onModelSelected(const QItemSelection&,const QItemSelection&)));

  // set up context menus for working on selected patients, studies, series
  connect(d->dicomTableManager, SIGNAL(patientsRightClicked(const QPoint&)),
          this, SLOT(onPatientsRightClicked(const QPoint&)));
  connect(d->dicomTableManager, SIGNAL(studiesRightClicked(const QPoint&)),
          this, SLOT(onStudiesRightClicked(const QPoint&)));
  connect(d->dicomTableManager, SIGNAL(seriesRightClicked(const QPoint&)),
          this, SLOT(onSeriesRightClicked(const QPoint&)));

  connect(d->dicomTableManager, SIGNAL(seriesDoubleClicked(const QModelIndex&)),
	  this, SLOT(onStartAction()));

  connect(d->DirectoryButton, SIGNAL(directoryChanged(QString)), this, SLOT(setDatabaseDirectory(QString)));

  //Initialize import widget
  d->ImportDialog = new ctkFileDialog();
  QCheckBox* importCheckbox = new QCheckBox("Copy on import", d->ImportDialog);
  importCheckbox->setCheckState(Qt::Checked);
  d->ImportDialog->setBottomWidget(importCheckbox);
  d->ImportDialog->setFileMode(QFileDialog::Directory);
  d->ImportDialog->setLabelText(QFileDialog::Accept,"Import");
  d->ImportDialog->setWindowTitle("Import DICOM files from directory ...");
  d->ImportDialog->setWindowModality(Qt::ApplicationModal);

  //connect signal and slots
  connect(d->ImportDialog, SIGNAL(fileSelected(QString)),this,SLOT(onImportDirectory(QString)));

  connect(d->QueryRetrieveWidget, SIGNAL(canceled()), d->QueryRetrieveWidget, SLOT(hide()) );
  connect(d->QueryRetrieveWidget, SIGNAL(canceled()), this, SLOT(onQueryRetrieveFinished()) );


}

//----------------------------------------------------------------------------
vuDICOMBrowser::~vuDICOMBrowser()
{
  Q_D(vuDICOMBrowser);

  d->QueryRetrieveWidget->deleteLater();
  d->ImportDialog->deleteLater();
}

//----------------------------------------------------------------------------
bool vuDICOMBrowser::displayImportSummary()
{
  Q_D(vuDICOMBrowser);

  return d->DisplayImportSummary;
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::setDisplayImportSummary(bool onOff)
{
  Q_D(vuDICOMBrowser);

  d->DisplayImportSummary = onOff;
}

//----------------------------------------------------------------------------
int vuDICOMBrowser::patientsAddedDuringImport()
{
  Q_D(vuDICOMBrowser);

  return d->PatientsAddedDuringImport;
}

//----------------------------------------------------------------------------
int vuDICOMBrowser::studiesAddedDuringImport()
{
  Q_D(vuDICOMBrowser);

  return d->StudiesAddedDuringImport;
}

//----------------------------------------------------------------------------
int vuDICOMBrowser::seriesAddedDuringImport()
{
  Q_D(vuDICOMBrowser);

  return d->SeriesAddedDuringImport;
}

//----------------------------------------------------------------------------
int vuDICOMBrowser::instancesAddedDuringImport()
{
  Q_D(vuDICOMBrowser);

  return d->InstancesAddedDuringImport;
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::updateDatabaseSchemaIfNeeded()
{

  Q_D(vuDICOMBrowser);

  d->showUpdateSchemaDialog();
  d->DICOMDatabase->updateSchemaIfNeeded();
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::setDatabaseDirectory(const QString& directory)
{
  Q_D(vuDICOMBrowser);

  QSettings settings;
  settings.setValue("DatabaseDirectory", directory);
  settings.sync();

  //close the active DICOM database
  d->DICOMDatabase->closeDatabase();

  //open DICOM database on the directory
  QString databaseFileName = directory + QString("/ctkDICOM.sql");
  try
    {
    d->DICOMDatabase->openDatabase( databaseFileName );
    }
  catch (std::exception e)
    {
    std::cerr << "Database error: " << qPrintable(d->DICOMDatabase->lastError()) << "\n";
    d->DICOMDatabase->closeDatabase();
    return;
    }

  // update the database schema if needed and provide progress
  this->updateDatabaseSchemaIfNeeded();

  //pass DICOM database instance to Import widget
  d->QueryRetrieveWidget->setRetrieveDatabase(d->DICOMDatabase);

  // update the button and let any connected slots know about the change
  d->DirectoryButton->setDirectory(directory);
  d->dicomTableManager->updateTableViews();
  emit databaseDirectoryChanged(directory);
}

//----------------------------------------------------------------------------
QString vuDICOMBrowser::databaseDirectory() const
{
  QSettings settings;
  return settings.value("DatabaseDirectory").toString();
}

//------------------------------------------------------------------------------
void vuDICOMBrowser::setTagsToPrecache( const QStringList tags)
{
  Q_D(vuDICOMBrowser);
  d->DICOMDatabase->setTagsToPrecache(tags);
}

//------------------------------------------------------------------------------
const QStringList vuDICOMBrowser::tagsToPrecache()
{
  Q_D(vuDICOMBrowser);
  return d->DICOMDatabase->tagsToPrecache();
}



//----------------------------------------------------------------------------
ctkDICOMDatabase* vuDICOMBrowser::database(){
  Q_D(vuDICOMBrowser);
  return d->DICOMDatabase.data();
}

//----------------------------------------------------------------------------
ctkDICOMTableManager* vuDICOMBrowser::dicomTableManager()
{
  Q_D(vuDICOMBrowser);
  return d->dicomTableManager;
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onFileIndexed(const QString& filePath)
{
  // Update the progress dialog when the file name changes
  // - also allows for cancel button
  QCoreApplication::instance()->processEvents();
  //qDebug() << "Indexing \n\n\n\n" << filePath <<"\n\n\n";
  
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::openImportDialog()
{
  Q_D(vuDICOMBrowser);

  d->ImportDialog->show();
  d->ImportDialog->raise();
}

//----------------------------------------------------------------------------
//void vuDICOMBrowser::openExportDialog()
//{
//
//}

//----------------------------------------------------------------------------
void vuDICOMBrowser::openQueryDialog()
{
  Q_D(vuDICOMBrowser);

  d->QueryRetrieveWidget->show();
  d->QueryRetrieveWidget->raise();

}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onQueryRetrieveFinished()
{
  emit this->queryRetrieveFinished();
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onRemoveAction()
{
  Q_D(vuDICOMBrowser);
  QStringList selectedSeriesUIDs = d->dicomTableManager->currentSeriesSelection();

  if (selectedSeriesUIDs.count() > 0 )
  {
	  if (this->confirmDeleteSelectedUIDs(selectedSeriesUIDs))
	  {
		  foreach(const QString& uid, selectedSeriesUIDs)
		  {
			  d->DICOMDatabase->removeSeries(uid);
		  }
	  }
  }
  else {
	  QStringList selectedStudiesUIDs = d->dicomTableManager->currentStudiesSelection();
	  if (selectedStudiesUIDs.count() > 0 )
	  {
		  if (this->confirmDeleteSelectedUIDs(selectedStudiesUIDs))
		  {
			  foreach(const QString& uid, selectedStudiesUIDs)
			  {
				  d->DICOMDatabase->removeStudy(uid);
			  }
		  }
	  }
	  else {
		  QStringList selectedPatientUIDs = d->dicomTableManager->currentPatientsSelection();
		  if(selectedPatientUIDs.count() > 0 && this->confirmDeleteSelectedUIDs(selectedPatientUIDs))
		  foreach(const QString& uid, selectedPatientUIDs)
		  {
			  d->DICOMDatabase->removePatient(uid);
		  }
	  }
  }
  // Update the table views
  d->dicomTableManager->updateTableViews();
}

void vuDICOMBrowser::onStartAction()
{
	Q_D(vuDICOMBrowser);

	QStringList filePaths;

	QStringList selectedSeriesUIDs = d->dicomTableManager->currentSeriesSelection();

	foreach(const QString& uid, selectedSeriesUIDs)
	{		
		filePaths.append(d->DICOMDatabase->filesForSeries(uid));
		//qDebug() << "Files for This UID: " << d->DICOMDatabase->filesForSeries(uid) << endl;
		//qDebug() << "Description for this UID: " << d->DICOMDatabase->descriptionForSeries(uid) << endl;
	}

	//QStringList selectedStudiesUIDs = d->dicomTableManager->currentStudiesSelection();
	//foreach(const QString& uid, selectedStudiesUIDs)
	//{
	//	d->DICOMDatabase->removeStudy(uid);
	//}
	//QStringList selectedPatientUIDs = d->dicomTableManager->currentPatientsSelection();
	//foreach(const QString& uid, selectedPatientUIDs)
	//{
	//	d->DICOMDatabase->removePatient(uid);
	//}
	//// Update the table views
	//d->dicomTableManager->updateTableViews();
	////remove those filepath named "/PS_" or "/XX_"
	//for (QList<QString>::Iterator it = filePaths.begin(); it<filePaths.end(); it++)
	//{
	//	//QRegExp nonIM("*XX_*");
	//	//xxFiles = filePaths.indexOf("/XX_");
	//	//std::cout << "xx and PS " << nonIM.indexIn(*it);
	//	if ((*it).indexOf("/XX_")>0 || (*it).indexOf("/PS_")>0)
	//	{
	//		//filePaths.removeAt(xxFiles);		 
	//		filePaths.erase(it);
	//		//std::cout << " a File is removed" << std::endl;
	//	}
	//}

	if (filePaths.size() == 0)
	{
		QMessageBox info;
		info.setText("You have to select an entry in the dicom browser for viewing.");
		info.exec();
		return;
	}

	emit SignalDicomToDataManager(filePaths);

}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onRepairAction()
{
  Q_D(vuDICOMBrowser);

  QMessageBox* repairMessageBox;
  repairMessageBox = new QMessageBox;
  repairMessageBox->setWindowTitle("Database Repair");

  QStringList allFiles(d->DICOMDatabase->allFiles());

  QSet<QString> corruptedSeries;

  QStringList::const_iterator it;
  for (it = allFiles.constBegin(); it!= allFiles.constEnd();++it)
  {
    QString fileName(*it);
    QFile dicomFile(fileName);

    if(!dicomFile.exists())
    {
      QString seriesUid = d->DICOMDatabase->seriesForFile(fileName);
      corruptedSeries.insert(seriesUid);
    }
  }

  if (corruptedSeries.size() == 0)
  {
    repairMessageBox->setText("All the files in the local database are available.");
    repairMessageBox->addButton(QMessageBox::Ok);
    repairMessageBox->exec();
  }

  else
  {
    repairMessageBox->addButton(QMessageBox::Yes);
    repairMessageBox->addButton(QMessageBox::No);
    QSet<QString>::iterator i;
    for (i = corruptedSeries.begin(); i != corruptedSeries.end(); ++i)
      {
      QStringList fileList (d->DICOMDatabase->filesForSeries(*i));
      QString unavailableFileNames;
      QStringList::const_iterator it;
      for (it= fileList.constBegin(); it!= fileList.constEnd();++it)
      {
        unavailableFileNames.append(*it+"\n");
      }

      QString firstFile (*(fileList.constBegin()));
      QHash<QString,QString> descriptions (d->DICOMDatabase->descriptionsForFile(firstFile));

      repairMessageBox->setText("The files for the following series are not available on the disk: \nPatient Name: "
        + descriptions["PatientsName"]+ "\n"+
        "Study Desciption: " + descriptions["StudyDescription"]+ "\n"+
        "Series Desciption: " + descriptions["SeriesDescription"]+ "\n"+
        "Do you want to remove the series from the DICOM database? ");

      repairMessageBox->setDetailedText(unavailableFileNames);

      int selection = repairMessageBox->exec();
      if (selection == QMessageBox::Yes)
      {
        d->DICOMDatabase->removeSeries(*i);
        d->dicomTableManager->updateTableViews();
      }
    }
  }
}
//----------------------------------------------------------------------------
void vuDICOMBrowser::onTablesDensityComboBox(QString density)
{
  Q_D(vuDICOMBrowser);

  if ( density == "Comfortable")
  {
    d->dicomTableManager->setDisplayDensity(ctkDICOMTableManager::Comfortable);
  }
  else if ( density == "Cozy")
  {
    d->dicomTableManager->setDisplayDensity(ctkDICOMTableManager::Cozy);
  }
  else if ( density == "Compact")
  {
    d->dicomTableManager->setDisplayDensity(ctkDICOMTableManager::Compact);
  }
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onPatientAdded(int databaseID, QString patientID, QString patientName, QString patientBirthDate )
{
  Q_D(vuDICOMBrowser);
  Q_UNUSED(databaseID);
  Q_UNUSED(patientID);
  Q_UNUSED(patientName);
  Q_UNUSED(patientBirthDate);
  ++d->PatientsAddedDuringImport;
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onStudyAdded(QString studyUID)
{
  Q_D(vuDICOMBrowser);
  Q_UNUSED(studyUID);
  ++d->StudiesAddedDuringImport;
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onSeriesAdded(QString seriesUID)
{
  Q_D(vuDICOMBrowser);
  Q_UNUSED(seriesUID);
  ++d->SeriesAddedDuringImport;
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onInstanceAdded(QString instanceUID)
{
  Q_D(vuDICOMBrowser);
  Q_UNUSED(instanceUID);
  ++d->InstancesAddedDuringImport;
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onImportDirectory(QString directory)
{
  Q_D(vuDICOMBrowser);

  if (QDir(directory).exists())
  {
    QString targetDirectory;

    QCheckBox* copyOnImport = qobject_cast<QCheckBox*>(d->ImportDialog->bottomWidget());

    QMessageBox importTypeDialog(this);
	QString message(tr("Copy data to program folder (Recommended for source data stored in removable device) or indexing only."));
    importTypeDialog.setText(message);
    importTypeDialog.setIcon(QMessageBox::Question);

    importTypeDialog.addButton(tr("Copy"),QMessageBox::AcceptRole);
    importTypeDialog.addButton(tr("Add index"),QMessageBox::RejectRole);
    //importTypeDialog.setDontShowAgainSettingsKey( "MainWindow/DontConfirmCopyOnImport" );

    int selection = importTypeDialog.exec();

    if (selection== QMessageBox::AcceptRole)
    {
      copyOnImport->setCheckState(Qt::Checked);
    }
    else
    {
      copyOnImport->setCheckState(Qt::Unchecked);
    }

    // reset counts
    d->PatientsAddedDuringImport = 0;
    d->StudiesAddedDuringImport = 0;
    d->SeriesAddedDuringImport = 0;
    d->InstancesAddedDuringImport = 0;

    if (copyOnImport->checkState() == Qt::Checked)
    {
      targetDirectory = d->DICOMDatabase->databaseDirectory();
    }

    // show progress dialog and perform indexing
    d->showIndexerDialog();
    d->DICOMIndexer->addDirectory(*d->DICOMDatabase,directory,targetDirectory);

    // display summary result
    if (d->DisplayImportSummary)
    {
      QString message = "Directory import completed.\n\n";
      message += QString("%1 New Patients\n").arg(QString::number(d->PatientsAddedDuringImport));
      message += QString("%1 New Studies\n").arg(QString::number(d->StudiesAddedDuringImport));
      message += QString("%1 New Series\n").arg(QString::number(d->SeriesAddedDuringImport));
      message += QString("%1 New Instances\n").arg(QString::number(d->InstancesAddedDuringImport));
      QMessageBox::information(this,"DICOM Directory Import", message);
    }
  }
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onModelSelected(const QItemSelection &item1, const QItemSelection &item2)
{
  Q_UNUSED(item1);
  Q_UNUSED(item2);
  Q_D(vuDICOMBrowser);
  d->removeButton->setEnabled(true);
  d->procButton->setEnabled(true);
}

//----------------------------------------------------------------------------
bool vuDICOMBrowser::confirmDeleteSelectedUIDs(QStringList uids)
{
  Q_D(vuDICOMBrowser);

  if (uids.isEmpty())
    {
    return false;
    }

  QMessageBox confirmDeleteDialog(this);
  QString message("Do you want to delete the following selected items?");

  // add the information about the selected UIDs
  int numUIDs = uids.size();
  for (int i = 0; i < numUIDs; ++i)
  {
	  QString uid = uids.at(i);

	  // try using the given UID to find a descriptive string
	  QString patientName = d->DICOMDatabase->nameForPatient(uid);
	  QString studyDescription = d->DICOMDatabase->descriptionForStudy(uid);
	  QString seriesDescription = d->DICOMDatabase->descriptionForSeries(uid);

	  if (!patientName.isEmpty())
	  {
		  message += QString("\n") + patientName;
	  }
	  else if (!studyDescription.isEmpty())
	  {
		  message += QString("\n") + studyDescription;
	  }
	  else if (!seriesDescription.isEmpty())
	  {
		  message += QString("\n") + seriesDescription;
	  }
	  else
	  {
		  // if all other descriptors are empty, use the UID
		  message += QString("\n") + uid;
	  }

  }
  confirmDeleteDialog.setText(message);
  confirmDeleteDialog.setIcon(QMessageBox::Question);

  confirmDeleteDialog.addButton("Delete", QMessageBox::AcceptRole);
  confirmDeleteDialog.addButton("Cancel", QMessageBox::RejectRole);
  //confirmDeleteDialog.setDontShowAgainSettingsKey( "MainWindow/DontConfirmDeleteSelected");

  int response = confirmDeleteDialog.exec();

  if (response == QMessageBox::AcceptRole)
  {
	  return true;
  }
  else
  {
	  return false;
  }
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onPatientsRightClicked(const QPoint &point)
{
	Q_D(vuDICOMBrowser);

	// get the list of patients that are selected
	QStringList selectedPatientsUIDs = d->dicomTableManager->currentPatientsSelection();
	int numPatients = selectedPatientsUIDs.size();
	if (numPatients == 0)
	{
		qDebug() << "No patients selected!";
		return;
	}

	QMenu *patientsMenu = new QMenu(d->dicomTableManager);

	QString deleteString = QString("Delete ")
		+ QString::number(numPatients)
		+ QString(" selected patients");
	QAction *deleteAction = new QAction(deleteString, patientsMenu);

	patientsMenu->addAction(deleteAction);

	QString exportString = QString("Export ")
		+ QString::number(numPatients)
		+ QString(" selected patients to file system");
	QAction *exportAction = new QAction(exportString, patientsMenu);

	patientsMenu->addAction(exportAction);

	// the table took care of mapping it to a global position so that the
	// menu will pop up at the correct place over this table.
	QAction *selectedAction = patientsMenu->exec(point);

	if (selectedAction == deleteAction
		&& this->confirmDeleteSelectedUIDs(selectedPatientsUIDs))
	{
		qDebug() << "Deleting " << numPatients << " patients";
		foreach(const QString& uid, selectedPatientsUIDs)
		{
			d->DICOMDatabase->removePatient(uid);
			d->dicomTableManager->updateTableViews();
		}
	}
	else if (selectedAction == exportAction)
	{
		ctkFileDialog* directoryDialog = new ctkFileDialog();
		directoryDialog->setOption(QFileDialog::DontUseNativeDialog);
		directoryDialog->setOption(QFileDialog::ShowDirsOnly);
		directoryDialog->setFileMode(QFileDialog::DirectoryOnly);
		bool res = directoryDialog->exec();
		if (res)
		{
			QStringList dirs = directoryDialog->selectedFiles();
			QString dirPath = dirs[0];
			this->exportSelectedPatients(dirPath, selectedPatientsUIDs);
		}
		delete directoryDialog;
	}
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onStudiesRightClicked(const QPoint &point)
{
  Q_D(vuDICOMBrowser);

  // get the list of studies that are selected
  QStringList selectedStudiesUIDs = d->dicomTableManager->currentStudiesSelection();
  int numStudies = selectedStudiesUIDs.size();
  if (numStudies == 0)
    {
    qDebug() << "No studies selected!";
    return;
    }

  QMenu *studiesMenu = new QMenu(d->dicomTableManager);

  QString deleteString = QString("Delete ")
    + QString::number(numStudies)
    + QString(" selected studies");
  QAction *deleteAction = new QAction(deleteString, studiesMenu);

  studiesMenu->addAction(deleteAction);

  QString exportString = QString("Export ")
    + QString::number(numStudies)
    + QString(" selected studies to file system");
  QAction *exportAction = new QAction(exportString, studiesMenu);

  studiesMenu->addAction(exportAction);

  // the table took care of mapping it to a global position so that the
  // menu will pop up at the correct place over this table.
  QAction *selectedAction = studiesMenu->exec(point);

  if (selectedAction == deleteAction
      && this->confirmDeleteSelectedUIDs(selectedStudiesUIDs))
    {
    foreach (const QString& uid, selectedStudiesUIDs)
      {
      d->DICOMDatabase->removeStudy(uid);
      d->dicomTableManager->updateTableViews();
      }
    }
  else if (selectedAction == exportAction)
    {
    ctkFileDialog* directoryDialog = new ctkFileDialog();
    directoryDialog->setOption(QFileDialog::DontUseNativeDialog);
    directoryDialog->setOption(QFileDialog::ShowDirsOnly);
    directoryDialog->setFileMode(QFileDialog::DirectoryOnly);
    bool res = directoryDialog->exec();
    if (res)
      {
      QStringList dirs = directoryDialog->selectedFiles();
      QString dirPath = dirs[0];
      this->exportSelectedStudies(dirPath, selectedStudiesUIDs);
      }
    delete directoryDialog;
    }
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::onSeriesRightClicked(const QPoint &point)
{
  Q_D(vuDICOMBrowser);

  // get the list of series that are selected
  QStringList selectedSeriesUIDs = d->dicomTableManager->currentSeriesSelection();
  int numSeries = selectedSeriesUIDs.size();
  if (numSeries == 0)
    {
    qDebug() << "No series selected!";
    return;
    }

  QMenu *seriesMenu = new QMenu(d->dicomTableManager);

  QString deleteString = QString("Delete ")
    + QString::number(numSeries)
    + QString(" selected series");
  QAction *deleteAction = new QAction(deleteString, seriesMenu);

  seriesMenu->addAction(deleteAction);

  QString exportString = QString("Export ")
    + QString::number(numSeries)
    + QString(" selected series to file system");
  QAction *exportAction = new QAction(exportString, seriesMenu);
  seriesMenu->addAction(exportAction);

  // the table took care of mapping it to a global position so that the
  // menu will pop up at the correct place over this table.
  QAction *selectedAction = seriesMenu->exec(point);

  if (selectedAction == deleteAction
      && this->confirmDeleteSelectedUIDs(selectedSeriesUIDs))
    {
    foreach (const QString& uid, selectedSeriesUIDs)
      {
      d->DICOMDatabase->removeSeries(uid);
      d->dicomTableManager->updateTableViews();
      }
    }
  else if (selectedAction == exportAction)
    {
    ctkFileDialog* directoryDialog = new ctkFileDialog();
    directoryDialog->setOption(QFileDialog::DontUseNativeDialog);
    directoryDialog->setOption(QFileDialog::ShowDirsOnly);
    directoryDialog->setFileMode(QFileDialog::DirectoryOnly);
    bool res = directoryDialog->exec();
    if (res)
      {
      QStringList dirs = directoryDialog->selectedFiles();
      QString dirPath = dirs[0];
      this->exportSelectedSeries(dirPath, selectedSeriesUIDs);
      }
    delete directoryDialog;
    }
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::exportSelectedSeries(QString dirPath, QStringList uids)
{
  Q_D(vuDICOMBrowser);

  foreach (const QString& uid, uids)
    {
    QStringList filesForSeries = d->DICOMDatabase->filesForSeries(uid);

    // Use the first file to get the overall series information
    QString firstFilePath = filesForSeries[0];
    QHash<QString,QString> descriptions (d->DICOMDatabase->descriptionsForFile(firstFilePath));
    QString patientName = descriptions["PatientsName"];
    QString patientIDTag = QString("0010,0020");
    QString patientID = d->DICOMDatabase->fileValue(firstFilePath, patientIDTag);
    QString studyDescription = descriptions["StudyDescription"];
    QString seriesDescription = descriptions["SeriesDescription"];
    QString studyDateTag = QString("0008,0020");
    QString studyDate = d->DICOMDatabase->fileValue(firstFilePath,studyDateTag);
    QString seriesNumberTag = QString("0020,0011");
    QString seriesNumber = d->DICOMDatabase->fileValue(firstFilePath,seriesNumberTag);

    QString sep = "/";
    QString nameSep = "-";
    QString destinationDir = dirPath + sep + patientID;
    if (!patientName.isEmpty())
      {
      destinationDir += nameSep + patientName;
      }
    destinationDir += sep + studyDate;
    if (!studyDescription.isEmpty())
      {
      destinationDir += nameSep + studyDescription;
      }
    destinationDir += sep + seriesNumber;
    if (!seriesDescription.isEmpty())
      {
      destinationDir += nameSep + seriesDescription;
      }
    destinationDir += sep;

    // make sure only ascii characters are in the directory path
    destinationDir = destinationDir.toLatin1();
    // replace any question marks that were used as replacements for non ascii
    // characters with underscore
    destinationDir.replace("?", "_");

    // create the destination directory if necessary
    if (!QDir().exists(destinationDir))
      {
      if (!QDir().mkpath(destinationDir))
        {
        QString errorString =
          QString("Unable to create export destination directory:\n\n")
          + destinationDir
          + QString("\n\nHalting export.");
        QMessageBox createDirectoryErrorMessageBox(this);
        createDirectoryErrorMessageBox.setText(errorString);
        createDirectoryErrorMessageBox.setIcon(QMessageBox::Warning);
        createDirectoryErrorMessageBox.exec();
        return;
        }
      }

    // show progress
    if (d->ExportProgress == 0)
      {
      d->ExportProgress = new QProgressDialog(this->tr("DICOM Export"), "Close", 0, 100, this, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
      d->ExportProgress->setWindowModality(Qt::ApplicationModal);
      d->ExportProgress->setMinimumDuration(0);
      }
    QLabel *exportLabel = new QLabel(this->tr("Exporting series ") + seriesNumber);
    d->ExportProgress->setLabel(exportLabel);
    d->ExportProgress->setValue(0);

    int fileNumber = 0;
    int numFiles = filesForSeries.size();
    d->ExportProgress->setMaximum(numFiles);
    foreach (const QString& filePath, filesForSeries)
      {
      QString destinationFileName = destinationDir;

      QString fileNumberString;
      // sequentially number the files
      fileNumberString.sprintf("%06d", fileNumber);

      destinationFileName += fileNumberString + QString(".dcm");

      // replace non ASCII characters
      destinationFileName = destinationFileName.toLatin1();
      // replace any question marks that were used as replacements for non ascii
      // characters with underscore
      destinationFileName.replace("?", "_");

      if (!QFile::exists(filePath))
        {
        d->ExportProgress->setValue(numFiles);
        QString errorString = QString("Export source file not found:\n\n")
          + filePath
          + QString("\n\nHalting export.\n\nError may be fixed via Repair.");
        QMessageBox copyErrorMessageBox(this);
        copyErrorMessageBox.setText(errorString);
        copyErrorMessageBox.setIcon(QMessageBox::Warning);
        copyErrorMessageBox.exec();
        return;
      }
      if (QFile::exists(destinationFileName))
        {
        d->ExportProgress->setValue(numFiles);
        QString errorString = QString("Export destination file already exists:\n\n")
          + destinationFileName
          + QString("\n\nHalting export.");
        QMessageBox copyErrorMessageBox(this);
        copyErrorMessageBox.setText(errorString);
        copyErrorMessageBox.setIcon(QMessageBox::Warning);
        copyErrorMessageBox.exec();
        return;
        }

      bool copyResult = QFile::copy(filePath, destinationFileName);
      if (!copyResult)
        {
        d->ExportProgress->setValue(numFiles);
        QString errorString = QString("Failed to copy\n\n")
          + filePath
          + QString("\n\nto\n\n")
          + destinationFileName
          + QString("\n\nHalting export.");
        QMessageBox copyErrorMessageBox(this);
        copyErrorMessageBox.setText(errorString);
        copyErrorMessageBox.setIcon(QMessageBox::Warning);
        copyErrorMessageBox.exec();
        return;
        }

      fileNumber++;
      d->ExportProgress->setValue(fileNumber);
      }
    d->ExportProgress->setValue(numFiles);
    }
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::exportSelectedStudies(QString dirPath, QStringList uids)
{
  Q_D(vuDICOMBrowser);

  foreach (const QString& uid, uids)
    {
    QStringList seriesUIDs = d->DICOMDatabase->seriesForStudy(uid);
    this->exportSelectedSeries(dirPath, seriesUIDs);
    }
}

//----------------------------------------------------------------------------
void vuDICOMBrowser::exportSelectedPatients(QString dirPath, QStringList uids)
{
  Q_D(vuDICOMBrowser);

  foreach (const QString& uid, uids)
    {
    QStringList studiesUIDs = d->DICOMDatabase->studiesForPatient(uid);
    this->exportSelectedStudies(dirPath, studiesUIDs);
    }
}
