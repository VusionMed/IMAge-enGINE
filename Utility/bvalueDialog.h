#ifndef bValueDialog_H
#define bValueDialog_H

#include <QDialog>
#include <QButtonGroup>
#include <qcolor.h>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <itkVector.h>
#include <itkVectorContainer.h>
#include <QSettings>
#include <QCheckBox>

class bValueDialog : public QDialog
{
	Q_OBJECT


private:
	bool resolveFile();

	QWidget* bValFrame;
	QWidget* vecValFrame;
	QLabel *dispLabel;
	QCheckBox *dtiCheckBox;

	QPushButton *bDirButton;
	QPushButton *vecDirButton;
	QLabel* bvalLabel;
	QLabel* vecLabel;
	QDialogButtonBox* buttonBox;

	std::vector<float> bvalueList;
	typedef itk::Vector<double, 3> GradientDirectionType;
	typedef itk::VectorContainer< unsigned int, GradientDirectionType >
		GradientDirectionContainerType;
	GradientDirectionContainerType::Pointer gradientDirection;

	int numOfB;

	int numOfB0Image;

	QString bValFilesPath;
	QString bVecFilesPath;
	QSettings settings;
	//QString curSliceString;
	


public:
	
	std::vector<float> GetBValueList()
	{
		return bvalueList;
	}
	
	GradientDirectionContainerType::Pointer GetGradientDirection()
	{
		return gradientDirection;
	}

	bool isDTI()
	{
		return this->dtiCheckBox->isChecked();
	}

	explicit bValueDialog(bool mustDTI, QWidget *parent = 0); 

	bool ResolveOK;
signals:
	//\brief: Init Roi in the way of 1: using previous
	//\2: using a square 3: using a circle 4: free hand drawing.
	void SignalbValLoaded(std::vector<float>,int, GradientDirectionContainerType::Pointer);	
	private slots:
	void onAccept();
	void onSetVecFile();
	void onSetbFile();
	void onIsDTI(int);

};

#endif // bValueDialog_H