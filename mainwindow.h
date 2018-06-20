#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//QT lib
#include <QtCharts>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QtWidgets/QDialog>
//VTK lib
#include <vtkSmartPointer.h>
#include <vtkObject.h>


namespace Ui {
class MainWindow;
class Dialog;
}

class Ui_MainWindow;
class vuDICOMBrowser;
class vtkImageData;
class QVTKWidget;
class DisplayPort;
class vtkCamera;
class DicomHelper;
class QStandardItemModel;
class vtkCollection;
class vtkEventQtSlotConnect;
class vtkContourRepresentation;
class QMessageBox;
class RoiDialog;
class RoiRemoveDialog;
class vuRoiModule;
class TreeModel;
class UserState;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

	explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	
signals:

	/// @brief
	/// When new source images are loaded, emit this signal to inform calculation module.
	/// 
	void SignalSetSourceImage(DicomHelper*);

	/// @brief
	/// When update event (slice-scrolling) is triggerd, emit this signal to inform calculation module to update slice. 
	/// 
	void SignalRecalcAll();

public slots :

protected slots :

    /// @brief
    /// In this slot, show DICOM widget as a pop-up window.
    /// 
	void onStartdicom();
	
	/// @brief
	/// This is the slot receiving calculated image from calculation module
	///
	void onProcButtonClicked(bool, vtkSmartPointer <vtkImageData>, const QString, const float, const float);

	/// @brief
	/// This is the slot loading new source images. 
	///
	void OnImageFilesLoaded(const QStringList& fileLists);

	/// @brief
	/// This is the slot responding to the writing process of calculation module. 
	///
	void OnSaveDcmUpdate(QString, int);

	/// @brief
	/// In this slot, react to grabFocus on a centain widget.
	/// 
	void onChangeLinkStatus(const QString, bool);

	/// @brief
	/// In this slot, react to mouse wheel event on a widget.
	/// 
	void onWheelWdw(const QString, int, Qt::Orientation);

	/// @brief
	/// In this slot, react to the value change of Master Widget's slice slider .
	/// 
	void onImageWidgetSliceChanged(int, const QString);

	/// @brief
	/// In this slot, react to mouse press\release\move event on linked widget.
	/// 
	void onBroadcastEvent(QMouseEvent *,const QString);

	/// @brief
	/// In this slot, call the export function of calculation module.
	/// 
	void onExportImage();

	/// @brief
	/// In this slot, show vusion tech info on a pop-up information dialog.
	/// 
	void onAboutDialog();

	/// @brief
	/// this is the slot receiving the picking value from window named itemName and show it in mainwindow.
	/// 
	void onPickerSig(QString itemName, float val);

	/// @brief
	/// this slot shows the setting page.
	/// 
	void onOpenSettingPage();

protected:

	void DisplayDicomInfo(vtkSmartPointer <vtkImageData> imageData);
	
	void ShareWindowEvent();

private:

	QList < QString >  ActiveWdw;

	vtkEventQtSlotConnect* Connections;

	Ui::MainWindow *ui;
	vuDICOMBrowser* DicomUI;
	QString StudyUID;

	QMessageBox* info;
	QProgressDialog* m_progressD;
	QChart* barchart;
	QChart* splineChart;
	
	DicomHelper* m_DicomHelper;
	vtkSmartPointer < vtkImageData > sourceImage;

	int sourceScalarType = 0;
	
	int m_SourceImageCurrentSlice;
	int m_SourceImageCurrentComponent;
	int m_SourceImageMaxSlice;
	int m_SourceImageMinSlice;

	bool useCurrentWindowLevel;
	bool isNewWidget;

	float curScalPara[2];

	QList<QString> uploadFiles;	
	QString tempLabelName;

	double m_timeStart;
	double m_timeEnd;

};


#endif // MAINWINDOW_H
