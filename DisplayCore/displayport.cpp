#include "displayport.h"
#include "qdebug.h"
#include "qevent.h"

DisplayPort::DisplayPort(QWidget* p, Qt::WindowFlags f) :QFrame(p, f | Qt::MSWindowsOwnDC)
{
	//this->DC_gridlayout = grid;
	//this->setParent(Parent);
	gridlayout = new QGridLayout(this);
	gridlayout->setHorizontalSpacing(1);
	gridlayout->setVerticalSpacing(1);
	this->setLayout(gridlayout);	
}

DisplayPort::~DisplayPort()
{
	//delete DC_LayoutMap;
}

void DisplayPort::index2Pos(int i, int& row, int& col)
{
	int Tf = floor(sqrt(i));
	int Tc = ceil(sqrt(i));
	if (Tf == Tc)
	{
		row = Tf - 1;
		col = Tf - 1;
	}
	else{
		if (i < (Tf*Tf + Tc))
		{
			row = i - Tf*Tf - 1;
			col = Tc - 1;
		}
		else
		{
			row = Tc - 1;
			col = Tc - Tc*Tc + i - 1;
		}
	}
}

void DisplayPort::pos2Index(int& i, int row, int col)
{
	if (row >= col)
	{
		i = row*row + row + col + 1;
	}
	else
	{
		i = col*col + row + 1;
	}
}

void DisplayPort::insertWindow(QWidget* wdgt, const QString imageLabel)
{
	DC_LayoutMap.push_back(imageLabel);
	int index = DC_LayoutMap.size();

	//qDebug()<<"Inserting: "<<imageLabel << "at" <<index;
	int row(-9), col(-9);
	index2Pos(index, row, col);
	//qDebug()<<"row = "<<row<<" col ="<<col<<endl;
	//QFrame* wdw;
	//wdw->setLineWidth(2);
	//wdw->setMidLineWidth(0);
	//wdw->setFrameShape(QFrame::Box);
	//wdw->setFrameShadow(QFrame::Raised);
	//wdw = new QFrame(this);
	//frameHash.insert(imageLabel, wdw);

	//QHBoxLayout *wdwlayout = new QHBoxLayout;
	//wdw->setLayout(wdwlayout);
	//wdwlayout->setContentsMargins(2, 2, 2, 2);
	this->gridlayout->addWidget(wdgt, row, col);
	//wdgt->setParent(wdw);

	//QWidget * Widget = wdw->childAt(wdw->geometry().center());
	//if (!Widget){ qDebug() << "Cannot get widget <" << imageLabel << "> "<< endl; }
	//wdwlayout->addWidget(wdgt);
	//wdw->show(); 

	wdgt->show();
	if (imageLabel.right(5) != QString("Chart"))
	{
		wdgt->installEventFilter(this);
	}
	this->gridlayout->update();
}

bool DisplayPort::eventFilter(QObject *watched, QEvent *Fevent)
{

	int flag(-1), counter(-1); //the first one cannot be removed
	int row(-1), col(-1);
	for (counter = 1; (counter < DC_LayoutMap.size() + 1) && flag < 0;)
	{
		index2Pos(counter, row, col);
		QLayoutItem *Item = this->gridlayout->itemAtPosition(row, col);
		//QFrame * frameWidget = static_cast<QFrame* >(Item->widget());
		//QObject * curWidget = frameWidget->childAt(10, 10);
		QObject * curWidget = static_cast<QObject* >(Item->widget());
		//qDebug() << "[DISPLAYPORT]" << DC_LayoutMap[counter - 1].c_str() << "_" << imageLabel.c_str() << "_" << endl;
		if (watched == curWidget)//if equal
		{
			flag = 1;
			//qDebug() << "Yes, Found widget at counter = " << DC_LayoutMap[counter - 1] << endl;
		}
		else{
			//qDebug() << "No, event is at widget " << DC_LayoutMap[counter - 1] << endl;
			counter++;
		}
	}

	// qDebug()<<"mouse is in widget ["<<row<<"-"<<col<<"] "<<DC_LayoutMap[counter-1]<<endl;
	if (flag>0){	
		if (Fevent->type() == QEvent::MouseButtonDblClick)
		{
			/*qDebug() << "focus caught in [" << row << "-" << col << "] " <<DC_LayoutMap[counter-1]<<endl;
			emit signalFocusIn(DC_LayoutMap[counter - 1]);*/
			return true;
		}else if (Fevent->type() == QEvent::Wheel)
		{
			//qDebug() << "wheel event";
			QWheelEvent* e = static_cast<QWheelEvent*>(Fevent);
			emit signalWheel(DC_LayoutMap[counter - 1], e->delta(), e->orientation());
			return false;
		}

		//else if (Fevent->type() == QEvent::MouseButtonPress || Fevent->type() == QEvent::MouseButtonRelease || Fevent->type() == QEvent::MouseMove)
		//{			
		//	qDebug() << "Mouse event caught at [" << row << "-" << col << "] " << DC_LayoutMap[counter - 1] << endl;
		//	QMouseEvent* e = static_cast<QMouseEvent*>(Fevent);	
		//	if (Fevent->type() == QEvent::MouseButtonPress && e->modifiers() == Qt::ShiftModifier)
		//	{
		//		qDebug() << "focus caught in [" << row << "-" << col << "] " << DC_LayoutMap[counter - 1] << endl;
		//		emit signalFocusIn(DC_LayoutMap[counter - 1]);
		//		return true;
		//	}
		//	if (e->source() == Qt::MouseEventNotSynthesized)
		//	{
		//		//onLabelWdw(DC_LayoutMap[counter - 1]);
		//		e->ignore();
		//		//qDebug() << "Real Mouse Button Pressed at [" << e->x() << "-" << e->y() << "] of" << DC_LayoutMap[counter - 1] << " accepted? " << e->isAccepted() << endl;
		//		emit signalMouseEvent(e, DC_LayoutMap[counter - 1]);
		//	}else{
		//		//qDebug() << "Synthetic Mouse Button Pressed at [" << e->x() << "-" << e->y() << "] of" << DC_LayoutMap[counter - 1] << " accepted? " << e->isAccepted()<< endl;
		//	}
		//	return false;
		//}else if (Fevent->type() == QEvent::KeyPress || Fevent->type() == QEvent::KeyRelease)
		//{
		//	QKeyEvent* e = static_cast<QKeyEvent*>(Fevent);
		//	if (!e->isAccepted())
		//	{
		//		//onLabelWdw(DC_LayoutMap[counter - 1]);
		//		//qDebug() << "Key [" <<e->text() << "] is" << "pressed" << endl;
		//		//QMouseEvent* newEvent;
		//		//newEvent = new QMouseEvent(e->type(), e->localPos(), e->windowPos(),
		//		//	e->screenPos(), e->button(), e->buttons(),
		//		//	e->modifiers(), Qt::MouseEventSynthesizedByQt);
		//		//newEvent->ignore();
		//		//emit signalKeyEvent(e, DC_LayoutMap[counter - 1]);
		//	}
		//	else{
		//		//qDebug() << " Key accepted? " << e->isAccepted() << endl;
		//	}
		//	return false;
		//}
	}

	return QObject::eventFilter(watched, Fevent);
}

QWidget* DisplayPort::getWindow(const QString imageLabel)
{
	int counter = getWidgetInd(imageLabel);
	if (counter > 0) //imageLabel exists
	{
		int row(-1), col(-1);
		index2Pos(counter, row, col);
		//qDebug() << "[DISPLAYPORT] Retrieving widget " << imageLabel << "from <" << row << "-" << col << ">" << endl;
		QLayoutItem *Item = this->gridlayout->itemAtPosition(row, col);
		//QFrame * frameWidget = static_cast<QFrame* >(Item->widget());
		//QWidget * curWidget = frameWidget->childAt(40,40);
		QWidget * curWidget = static_cast<QWidget* >(Item->widget());
		//QWidget * curWidget = (frameHash.value(imageLabel))->childAt((frameHash.value(imageLabel))->geometry().center());
		if (!curWidget)
		{
			qDebug() << "Cannot get widget <" << imageLabel <<"> at " << row << "-" << col << endl;
		}

		return curWidget;
	}
	else
	{

		return NULL;
	}
}

QHash <const QString, QWidget * >  DisplayPort::getAllWindow()
{
	QHash <const QString, QWidget * > curWidgets;
	std::vector< QString >::iterator it; //remove const for container vector (VS2015)
	int index;
	for (it = DC_LayoutMap.begin(), index = 1; it != DC_LayoutMap.end(); it++, index++)
	{
		int row, col;
		index2Pos(index, row, col);
		QLayoutItem *Item = this->gridlayout->itemAtPosition(row, col);
		//QFrame * frameWidget = static_cast<QFrame* >(Item->widget());
		QWidget * Widget = static_cast<QWidget* >(Item->widget());
		if (Widget != NULL && it->right(5)!=QString("Chart") ) {
			curWidgets.insert(*it, Widget);
		}
		else{
			qDebug() << "Cannot get widget at " << row << "-" << col << endl;
		}
	}
	return curWidgets;
}

int DisplayPort::getWidgetInd(const QString imageLabel)
{
	int flag(-1), counter(-1); //the first one cannot be removed
	for (counter = 1; (counter < DC_LayoutMap.size() + 1) && flag < 0;)
	{
		//qDebug() << "[DISPLAYPORT]" << DC_LayoutMap[counter - 1].c_str() << "_" << imageLabel.c_str() << "_" << endl;
		if (DC_LayoutMap[counter - 1] == imageLabel)//if equal
		{
			flag = 1;
			//qDebug() << "Found widget at counter = " << counter << endl;
		}
		else{
			counter++;
		}
	}

	if (flag > 0)
	{
		return counter;
	}
	else
	{
		return -1;
	}

}

void DisplayPort::removeWindow(const QString imageLabel)
{

	int counter = getWidgetInd(imageLabel);

	//2. If found.

	if (counter > 0) //imageLabel exists
	{
		//2.1, Adjust the LayoutMap.
		int rowF(-1), colF(-1), rowT(-1), colT(-1);
		//qDebug() << "[DISPLAYPORT] counter = " << counter << endl;
		index2Pos(counter, rowT, colT); //deleting pos
		index2Pos(DC_LayoutMap.size(), rowF, colF); //last pos
		//qDebug() << "[DISPLAYPORT]moving window from " << rowF << ":" << colF << " to " << rowT << ":" << colT << endl;
		QLayoutItem *ItemF = this->gridlayout->itemAtPosition(rowF, colF);
		QWidget * WidgetF = ItemF->widget();
		QLayoutItem *ItemT = this->gridlayout->itemAtPosition(rowT, colT);
		QWidget * WidgetT = ItemT->widget();

		if (WidgetF) {
			//qDebug() << "moving widget";
			this->gridlayout->addWidget(WidgetF, rowT, colT); //move F to T pos.
		}
		if (WidgetT) {

			//qDebug() << "deleting widget";
			//WidgetT->removeEventFilter(this);
			//QFrame * frameWidget = static_cast<QFrame* >(WidgetT);
			//QWidget * curWidget = frameWidget->childAt(30, 30);
			//if (curWidget == NULL){ qDebug() << "Not Getting chart Widget"; }

			if (imageLabel.right(5) != QString("Chart"))
			{				
				WidgetT->removeEventFilter(this);
			}
			//else{ qDebug() << "removing Chart widget"; }

			this->gridlayout->removeWidget(WidgetT);
			WidgetT->setParent(NULL);
			delete WidgetT;
		}

		//STEP2, modify the DC_Windoes accordingly
		std::iter_swap(DC_LayoutMap.begin() + counter - 1, DC_LayoutMap.end() - 1);
		DC_LayoutMap.pop_back();
		//this->PrintWdwLayout();
	}

}

void DisplayPort::PrintWdwLayout()
{
	qDebug() << "[DISPLAYPORT] TOTAL " << DC_LayoutMap.size() << " WINDOWS ARE RENDERING" << endl;
	std::vector< QString >::iterator it; //remove const for containder vector (VS2015)
	int index;
	for (it = DC_LayoutMap.begin(), index = 1; it != DC_LayoutMap.end(); it++, index++)
	{
		int row, col;
		index2Pos(index, row, col);
		qDebug() << "[DISPLAYPORT] <" << row << " , " << col << "> renders->" << (*it) << endl;
	}
}

void DisplayPort::onLabelWdw(const QString imageLabel)
{

	qDebug() << "labeling wdw " << imageLabel << endl;
	int index = getWidgetInd(imageLabel);
	int row, col;
	index2Pos(index, row, col);
	QLayoutItem *Item = this->gridlayout->itemAtPosition(row, col);
	QFrame * frame = static_cast<QFrame* >(Item->widget());

	frame->setLineWidth(3);
	frame->setMidLineWidth(0);
	frame->setFrameStyle(QFrame::Box | QFrame::Plain);
	frame->setStyleSheet("border: 1px solid rgb(233, 89, 89);");
	//frame->setFrameShape(QFrame::Box);
	//frame->setFrameShadow(QFrame::Raised);
}

void DisplayPort::onRemoveLabelWdw(const QString imageLabel)
{
	int index = getWidgetInd(imageLabel);
	int row, col;
	index2Pos(index, row, col);
	QLayoutItem *Item = this->gridlayout->itemAtPosition(row, col);
	QFrame * frame = static_cast<QFrame* >(Item->widget());
	frame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	frame->setStyleSheet("border: 1px transparent rgb(233, 89, 89);");
}
