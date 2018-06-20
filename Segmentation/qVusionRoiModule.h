/*=========================================================================

Program:   Vusion ImageEngine
Module:    ROI interaction

=========================================================================*/
// .NAME qVusionRoiModule - helper class containing methods to operate ROIs. 
// .SECTION Description
// qVusionRoiModule is a helper object that contains methods to initiate, store
// and retrieve ROIs for giving renderwindow. Since it is a helper class,
// its members should not occupy memory. 


#ifndef qVusionRoiModule_h
#define qVusionRoiModule_h

//included qt
#include <QWidget>
#include <qdebug.h>
#include <QString>
#include <QTreeView>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QDateTime>

#include "treemodel.h"


//include vtk
#include <vtkSmartPointer.h>
#include <vtkInteractorObserver.h>
#include <vtkContourRepresentation.h>
#include <vtkImageData.h>
#include <..\DisplayCore\displayport.h>

//#include <qtextbrowser.h>
//included classes
class vtkImageActor;
class vtkImageTracerWidget;
class vtkContourWidget;
class vtkCollection;
class vtkPolyData;
class vtkContourRepresentation;
class vtkRenderWindow;
class vuWindowWidget;
class vtkEventQtSlotConnect;

namespace Ui {
	class qVusionRoiModule;
}

class PainterUndoRedo
{
public:
#define MaxLabelImageSize 5

	PainterUndoRedo() {
		m_CurrentIndex = -1;
		m_EditedLabelImageList.clear();
		m_EditedLabelImageList.reserve(MaxLabelImageSize);
	}

	~PainterUndoRedo()
	{
		this->RemoveFrom(0);
	}

	void AddToList(vtkImageData *data)
	{
		if (!data) return;

		vtkImageData *quequeData = vtkImageData::New();
		quequeData->DeepCopy(data);

		if (m_CurrentIndex < MaxLabelImageSize - 1)
		{
			this->m_EditedLabelImageList.insert(m_CurrentIndex + 1, quequeData);
			this->m_CurrentIndex++;
			this->RemoveFrom(m_CurrentIndex + 1);
		}
		else
		{
			//std::cout << "m_EditedLabelImageList.at(0) reference count: " << m_EditedLabelImageList.at(0)->GetReferenceCount() << std::endl;
			//release before pop front!
			if (this->m_EditedLabelImageList.at(0))
			{
				this->m_EditedLabelImageList.at(0)->Delete();
			}
			m_EditedLabelImageList.pop_front();
			m_EditedLabelImageList.push_back(quequeData);
		}

		//std::cout << "Add to list: m_EditedLabelImageList.size()= " << m_EditedLabelImageList.size() << std::endl;
	}

	void IncrementCurrentIndex()
	{
		if (m_CurrentIndex < MaxLabelImageSize - 1)
			m_CurrentIndex++;
	}

	void DecrementCurrentIndex()
	{
		if (m_CurrentIndex > -1)
			m_CurrentIndex--;
	}

	void RemoveFrom(int index)
	{
		if (index < 0) return;

		if (index < m_EditedLabelImageList.size())
		{
			for (int i = m_EditedLabelImageList.size() - 1; i >= index; i--)
			{
				if (m_EditedLabelImageList.at(i))
				{
					//std::cout << "m_EditedLabelImageList.at(i) reference count: " << m_EditedLabelImageList.at(i)->GetReferenceCount() << std::endl;
					m_EditedLabelImageList.at(i)->Delete();
				}
				m_EditedLabelImageList.removeAt(i);
			}
		}
	}

	bool UndoAllowed()
	{
		return (this->m_CurrentIndex > 0);
	}

	bool RedoAllowed()
	{
		if ((this->m_EditedLabelImageList.size() > 0) && (this->m_CurrentIndex != this->m_EditedLabelImageList.size() - 1))
		{
			return true;
		}

		return false;
	}

public:
	QList<vtkImageData *> m_EditedLabelImageList;
	int m_CurrentIndex;
};

class qVusionRoiModule : public QWidget
{
	Q_OBJECT

public:
	qVusionRoiModule(QWidget* p = NULL);
	~qVusionRoiModule();
	
	void setDisplayPort(DisplayPort* _DP);
	void UpdateLabelImage(); //vtkImageData* _lblImg
	void Reset(); //Refresh the module to initial status
	bool isRadiomicsReady();
signals:

	public slots :

	protected slots :


	void onClickTreeView(const QModelIndex &index);

	void onSaveRoiToCSV();

	void onUndo();

	void onRedo();

	void onSyncPaint(QString);

	void onAddContour();

	void onOpacityChanged(int);

	void onRunFastGrowCut();

	void onSaveROI();

	void onLoadROI();

protected:

	QList<QColor> colorList;

	bool _UpdateStatisRoi(vuWindowWidget* thisWdw);
	void _modifiedLabelImage();

	Ui::qVusionRoiModule *ui;
	
	PainterUndoRedo *m_lblImgeQ;

	vuWindowWidget* m_srcWdw;

	vtkSmartPointer<vtkImageData> m_contourImage;

	DisplayPort* m_DP;

	TreeModel* m_roiModel;

	QModelIndex curRoiDataindex;

	QMessageBox* info;

	QLabel* chartLabel;

};

#endif