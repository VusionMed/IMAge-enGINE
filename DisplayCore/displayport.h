/*=========================================================================

Program:   Vusion ImageEngine
Module:    Display Port

=========================================================================*/
// .NAME DisplayPort - QFrame to hold displaying windows. 
// .SECTION Description


#ifndef DISPLAYPORT_H
#define DISPLAYPORT_H

#include <QObject>
#include <QWidget>
#include <QGridLayout>
#include <qframe.h>

class DisplayPort : public QFrame
{
	Q_OBJECT
public:

	//static const std::string Widget_ID;

	/**
	* \brief DisplayPort(QWidget *parent) constructor. DisplayPort is an expansion of QGridlayout
	*
	*/
	explicit DisplayPort(QWidget* parent = NULL, Qt::WindowFlags f = 0);

	/**
	* \brief DiffusionCore destructor.
	*/
	virtual ~DisplayPort();

	/**
	* \brief insertWindow(QWidget* wdw, std::string imageLabel) insert wdw widget whose name is imageLabel
	*
	* \param wdw is any QWidget, imageLabel is one unique name of this QWidget.
	*/
	void insertWindow(QWidget* wdw, const QString imageLabel);

	/**
	* \brief removeWindow(std::string imageLabel) remove wdw widget whose name is imageLabel
	*
	* \param imageLabel is one unique name of the QWidget going to be removed.
	*/
	void removeWindow(const QString imageLabel);

	/**
	* \brief PrintWdwLayout() print the widget names in current displayport
	*/
	void PrintWdwLayout();

	/**
	* \brief QWidget* getWindow(std::string imageLabel) returns the widget whose name is imageLabel.
	* \param imageLabel is one unique name of the QWidget going to be retrieved.
	*/
	QWidget* getWindow(const QString imageLabel);

	/**
	* \brief QHash < const std::string, QWidget * > getAllWindow(); get all widgets currently exist in display port
	*/
	QHash < const QString, QWidget * > getAllWindow();

	///**
	//* \brief std::vector<QWidget* > getAllWindowNames() get all widgets' name currently exist in display port
	//*/
	//std::vector< const QString > getAllWindowNames();
signals:
	//void signalFocusIn(const QString);
	void signalWheel(const QString, int, Qt::Orientation);
	//void signalMouseEvent(QMouseEvent *,const QString);
	//void signalResizeEvent(const QString, const QSize, const QSize);
	//void signalMouseReleasedIn(QMouseEvent *);
	//void signalKeyEvent(QKeyEvent* );

	public slots:
	void onLabelWdw(const QString imageLabel);
	void onRemoveLabelWdw(const QString imageLabel);

protected:
	
	//void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
	void index2Pos(int index, int& row, int& col);
	void pos2Index(int& index, int row, int col);
	bool eventFilter(QObject *, QEvent *); 

	void onLabelWdw(QObject *);
	void onRemoveLabelWdw(QObject *);

private:
	QGridLayout* gridlayout;
	int getWidgetInd(const QString imageLabel);
	std::vector< QString > DC_LayoutMap; //remove const for container vector (VS2015)
};

#endif // DISPLAYPORT_H
