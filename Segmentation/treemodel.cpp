/****************************************************************************

Program:   Vusion ImageEngine
Module:    ROI interaction

****************************************************************************/

#include <QtWidgets>

#include "treeitem.h"
#include "treemodel.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkProperty.h"
#include "vtkContourWidget.h"

//! [0]
TreeModel::TreeModel(const QStringList &headers, QObject *parent)
    : QAbstractItemModel(parent)
{	
    QVector<QVariant> rootData;
    foreach (QString header, headers)
        rootData << header;

    rootItem = new TreeItem(rootData);

    //setupModelData(data.split(QString("\n")), rootItem);
}
//! [0]

//! [1]
TreeModel::~TreeModel()
{
    delete rootItem;
}
//! [1]

//! [2]
int TreeModel::columnCount(const QModelIndex & /* parent */) const
{
    return rootItem->columnCount();
}
//! [2]

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);

    return item->data(index.column());
}

//! [3]
Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}
//! [3]

//! [4]
TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}
//! [4]

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

//! [5]
QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem *parentItem = getItem(parent);

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}
//! [6]

bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

//! [7]
QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(index);
    TreeItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}
//! [7]

bool TreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

//! [8]
int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem = getItem(parent);

    return parentItem->childCount();
}
//! [8]
 TreeItem* TreeModel::getrootItem()
 {

     return rootItem;
 }

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index);

    return result;
}

bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

int TreeModel::isRoiPresent(QString roiname, QString curSlice)
{
    //roiFlag = 0, no such roi exists.
    //          1, this roi exists but not in this slice
    //          2, this roi exists in this slice

    int roiFlag(0);

    QStringList roiList = getRoiList();
	if (roiList.contains(roiname))
	{
		roiFlag = 1;
		QStringList sliceList = getSliceList(roiname);
		if (sliceList.contains(curSlice))
		{
			roiFlag = 2;
		}
	}
    return roiFlag;
}

int TreeModel::isRoiPresent(QString roiName)
{
	int roiFlag(0);
	QStringList roiList = getRoiList();
	if (roiList.contains(roiName))
	{
		roiFlag = 1;
	}
	//qDebug() << roiList<<endl;
	return roiFlag;
}

int TreeModel::isRoiPresent(QString wdwname, QString roiname, QString curSlice)
{
	int roiFlag(0);
	QStringList roiList = getRoiList(wdwname);
	if (roiList.contains(roiname))
	{
		roiFlag = 1;
		QStringList sliceList = getSliceList(wdwname,roiname);
		if (sliceList.contains(curSlice))
		{
			roiFlag = 2;
		}
	}
	return roiFlag;
}

QStringList TreeModel::getRoiList() const
{
    QStringList roiList;
    for(int i=0;i<rootItem->childCount();i++)
    {
        roiList << rootItem->child(i)->listChild();
    }
    roiList.removeDuplicates();
    return roiList;
}

QStringList TreeModel::getRoiList(QString wdwName) const
{
	QStringList roiList;
	TreeItem* wdw = rootItem->child(wdwName);
	if (wdw)
	{
		roiList << wdw->listChild();
	}
	//roiList.removeDuplicates();
	return roiList;
}

QStringList TreeModel::getSliceList(QString RoiName) const
{
    QStringList sliceList;
    TreeItem* roi = rootItem->child(0)->child(RoiName);
    if(roi)
    {
        if(roi->listChild().size() > 0)
        {
            if(roi->child(0)->listChild().size()>0)
            {
                sliceList<<roi->child(0)->listChild();
            }else
            {
                sliceList<<roi->listChild();
            }
        }
    }
    return sliceList;
}

QStringList TreeModel::getSliceList(QString wdwName, QString RoiName) const
{
	QStringList sliceList;
	TreeItem* wdw = rootItem->child(wdwName);
	if (wdw)
	{
		TreeItem* roi = wdw->child(RoiName);
		if (roi)
		{
			if (roi->listChild().size() > 0)
			{
				if (roi->child(0)->listChild().size() > 0)
				{
					sliceList << roi->child(0)->listChild();
				}
				else
				{
					sliceList << roi->listChild();
				}
			}
		}
	}
	return sliceList;
}

QStringList TreeModel::getWindowList() const
{
	QStringList wdwList = rootItem->listChild();
	return wdwList;
}



bool TreeModel::insertROIs(QStringList &newRow, QString roiName, QString sliceName, QString window)
{
    //qDebug()<<"adding "<< newRow <<"to"<<roiName<<" of "<<sliceName<<"to "<<window;
    QStringList childrenIe = rootItem->listChild();

    //Step1. Check childrenI
    if(newRow.size() != rootItem->columnCount()-1)
    {
        qDebug()<<"wrong length of newRow";
        return false;
    }
	TreeItem* thisWdw = rootItem->addChild(window);
	if (window == "ADC" ||
		window == "IVIM_D" ||
		window == "IVIM_Dstar" ||
		window == "RD"||
		window == "AD"||
		window == "sDKI_Dapp"
		)
	{
		
		thisWdw->setData(1, "Volume");
		thisWdw->setData(2, "Mean um2/s");
		thisWdw->setData(3, "Std um2/s");
		thisWdw->setData(4, "Min um2/s");
		thisWdw->setData(5, "Max um2/s");
	}
	else {
		thisWdw->setData(1, "Volume");
		thisWdw->setData(2, "Mean");
		thisWdw->setData(3, "Std");
		thisWdw->setData(4, "Min");
		thisWdw->setData(5, "Max");
	}
    //Step2.
    TreeItem* roiItem = thisWdw->addChild(roiName);
    TreeItem* sliceItem = roiItem->addChild(sliceName);
	
    for(int icol = 1; icol < rootItem->columnCount();icol++)
    {
        sliceItem->setData(icol, newRow[icol-1]);
    }
    roiItem->sum();

	//endInsertRows();
	emit roiInserted(roiName);
    return true;
}

bool TreeModel::insertROIs(QStringList &newRow, QString roiName, QString sliceName, QString window,  QString component)
{
    //Step1. Check childrenI
    QStringList childrenIe = rootItem->listChild();
    if(newRow.size() != rootItem->columnCount()-1)
    {
        return false;
    }

		TreeItem* thisWdw = rootItem->addChild(window);
		thisWdw->setData(1, "Volume");
		thisWdw->setData(2, "Mean");
		thisWdw->setData(3, "Std");
		thisWdw->setData(4, "Min");
		thisWdw->setData(5, "Max");

    //Step2.
    TreeItem* thisRoi = thisWdw->addChild(roiName);
    for(int icol = 1; icol < rootItem->columnCount();icol++)
    {
        thisRoi->setData(icol, "MD-Data");
    }
    TreeItem* thisComp = thisRoi->addChild(component);
    TreeItem* thisSlice = thisComp->addChild(sliceName);
    for(int icol = 1; icol < rootItem->columnCount();icol++)
    {
        thisSlice->setData(icol, newRow[icol-1]);
    }
    thisComp->sum();

	//endInsertRows();
	emit roiInserted(roiName);
    return true;

}

bool TreeModel::removeROIs(QString RoiName)
{
	
	//foreach(QString wdwName, rootItem->listChild())
	//{
	//	foreach(QString slice, getSliceList(RoiName))
	//	{
	//		QString key = getKey(wdwName, RoiName, slice.toInt() - 1);
	//		qDebug() << "remoing " << wdwName << " " << RoiName << "from repHash?" << repHash.contains(key);
	//		repHash.remove(key);
	//		widgetHash.remove(key);
	//	}
	//}

	//QList<QString> hashKey = repHash.keys();
	//qDebug() << "Current repHash contains: "<< hashKey;

    bool removed(false);
    for(int i=0;i<rootItem->childCount();i++)
    {
        TreeItem* wdw = rootItem->child(i);
        bool remOne = wdw->removeChildOf(RoiName);
        removed = remOne||removed;
    }

    for(int j = rootItem->childCount() -1 ;j >= 0; j--)
    {
        if(rootItem->child(j)->listChild().size()<1)
        {
            rootItem->removeChildren(j,1);
        }
    }

	//endRemoveRows();
	removeColor(RoiName);
    return removed;
}

bool TreeModel::removeROIs(QString wdwName, QString RoiName)
{


	//foreach(QString slice, getSliceList(RoiName))
	//{
	//	QString key = getKey(wdwName, RoiName, slice.toInt() - 1);
	//	qDebug() << "remoing " << wdwName << " " << RoiName << "from repHash?" << repHash.contains(key);
	//	repHash.remove(key);
	//	widgetHash.remove(key);
	//}


	//QList<QString> hashKey = repHash.keys();
	//qDebug() << "Current repHash contains: " << hashKey;

	

	TreeItem* wdw = rootItem->child(wdwName);
	bool removed = wdw->removeChildOf(RoiName);	

	if (wdw->childCount() < 1)
	{
		rootItem->removeChildOf(wdwName);
	}

	if (!isRoiPresent(RoiName))
	{
		removeColor(RoiName);
	}

	//endRemoveRows();
	return removed;
}

bool TreeModel::removeROIs(QString wdwName, QString RoiName, QString SliceName)
{
	//QString key = getKey(wdwName, RoiName, SliceName.toInt() - 1);	
	//repHash.remove(key);
	//widgetHash.remove(key);

	bool removed(false);
	TreeItem* wdw = rootItem->child(wdwName);
	if (wdw)
	{
		TreeItem* roi = wdw->child(RoiName);
		bool rem(false);
		if (roi)
		{
			TreeItem* roiChild = roi->child(0);
			if (roiChild)
			{				
				if (roiChild->childCount() < 1)
				{
					rem = roi->removeChildOf(SliceName);
					qDebug() << "Removing " << SliceName << " from " << roi->data(0).toString();
				}
				else
				{
					qDebug() << roi->childCount() << " components exist";
					for (int j = 0; j < roi->childCount(); j++)
					{
						qDebug() << "Removing " << SliceName << " from " << roi->child(j)->data(0).toString();
						rem = roi->child(j)->removeChildOf(SliceName);
					}

					for (int j = roi->childCount() - 1; j >= 0; j--)
					{
						if (roi->child(j)->listChild().size() < 1)
						{
							roi->removeChildren(j, 1);
						}
					}
				}
			}
			if (roi->listChild().size()<1)
			{
				wdw->removeChildOf(RoiName);
			}
			removed = removed || rem;
		}
	}

	if (!isRoiPresent(RoiName))
	{
		removeColor(RoiName);
	}
	if (wdw->listChild().size()<1)
	{
		rootItem->removeChildOf(wdwName);
	}

	//endRemoveRows();
	return removed;
}

bool TreeModel::removeROIs(QString RoiName, int Slice)
{
	QString SliceName = QString("%1").arg(Slice + 1);

	//foreach(QString wdwName, rootItem->listChild())
	//{
	//	QString key = getKey(wdwName, RoiName, SliceName.toInt() - 1);
	//	qDebug() << "remoing repHash? " << repHash.contains(key);
	//	repHash.remove(key);
	//	widgetHash.remove(key);
	//}

	//QList<QString> hashKey = repHash.keys();
	//qDebug() << "Current repHash contains: " << hashKey;

    bool removed(false);
    for(int i=0;i<rootItem->childCount();i++)
    {
        bool rem(false);
        TreeItem* roi = rootItem->child(i)->child(RoiName);
        if(roi)
        {
            if(roi->listChild().contains(SliceName))
            {
                rem = roi->removeChildOf(SliceName);
            }else
            {
                qDebug()<<roi->childCount()<<" components exist";
                for(int j=0;j<roi->childCount();j++)
                {
                    qDebug()<<"Removing "<<SliceName<<" from "<<roi->child(j)->data(0).toString();
                    rem = roi->child(j)->removeChildOf(SliceName);
                }

                for(int j = roi->childCount() -1 ;j >= 0; j--)
                {
                    if(roi->child(j)->listChild().size()<1)
                    {
                        roi->removeChildren(j,1);
                    }
                }
            }

			if (roi->childCount() > 0)
			{
				roi->sum();
			}

            if(roi->listChild().size()<1)
            {
                rootItem->child(i)->removeChildOf(RoiName);
				removeColor(RoiName);
            }
        }
        removed = removed||rem;
    }

    for(int j = rootItem->childCount() -1 ;j >= 0; j--)
    {
        if(rootItem->child(j)->listChild().size()<1)
        {
            rootItem->removeChildren(j,1);
        }
    }

	//endRemoveRows();

	return removed;
}

bool TreeModel::storeReps(QString WdwName, QString roiName, int slice, vtkOrientedGlyphContourRepresentation* rep)
{
	QString sliceString = QString("%1").arg(slice + 1);
	QString key = getKey(WdwName, roiName, slice);

	if (repHash.contains(key))
	{
		repHash[key] = rep;
		qDebug() << ">>>>>>>>>>>>>>>CALLBACK: Rewrite existing ROI ";
		QList<QString> hashKey = repHash.keys();
		qDebug() << "Current repHash contains: " << hashKey;
		return false;
	}
	else
	{
		repHash.insert(key, rep);
		qDebug() << ">>>>>>>>>>>>>>>CALLBACK: Create Roi ";
		QList<QString> hashKey = repHash.keys();
		qDebug() << "After storing, repHash contains: " << hashKey;
		return true;
	}

	//switch (this->isRoiPresent(WdwName, roiName, sliceString))
	//{
	//case 0:
	//case 1:
	//	(RoiHash)[slice].insert(roiName, rep);
	//	qDebug() << ">>>>>>>>>>>>>>>CALLBACK: Create new slice for exsiting ROI ";
	//	return true;
	//	break;
	//case 2:
	//  (RoiHash)[slice][roiName] = rep;
	//	qDebug() << ">>>>>>>>>>>>>>>CALLBACK: Rewrite existing ROI ";
	//	return true;
	//	break;
	//default:
	//	return false;
	//	break;
	//}

}

vtkOrientedGlyphContourRepresentation* TreeModel::getReps(QString WdwName, QString roiName, int slice)
{
	QString sliceString = QString("%1").arg(slice + 1);
	QString key = getKey(WdwName, roiName, slice);
	if (repHash.contains(key))
	{
		return repHash[key];
	}
	else
	{
		return nullptr;
	}

	//switch (this->isRoiPresent(roiName, sliceString))
	//{
	//case 0:
	//	return nullptr;
	//	break;
	//case 1:
	//	return nullptr;
	//	break;
	//case 2:
	//	return RoiHash[slice].value(roiName);
	//	break;
	//default:
	//	return false;
	//	break;
	//}
}

bool TreeModel::storeWidgets(QString WdwName, QString roiName, int slice, vtkContourWidget* widget)
{
	QString sliceString = QString("%1").arg(slice + 1);
	QString key = getKey(WdwName, roiName, slice);

	if (widgetHash.contains(key))
	{
		widgetHash[key] = widget;
		//qDebug() << ">>>>>>>>>>>>>>>CALLBACK: Rewrite existing ROI ";
		QList<QString> hashKey = repHash.keys();
		//qDebug() << "Current repHash contains: " << hashKey;
		return false;
	}
	else
	{
		widgetHash.insert(key, widget);
		//qDebug() << ">>>>>>>>>>>>>>>CALLBACK: Create Roi ";
		QList<QString> hashKey = repHash.keys();
		////;///////;;.'/qDebug() << "Current repHash contains: " << hashKey;
		return true;
	}
}

vtkContourWidget* TreeModel::getWidgets(QString WdwName, QString roiName, int slice)
{
	//QString sliceString = QString("%1").arg(slice + 1);
	QString key = getKey(WdwName, roiName, slice);
	if (repHash.contains(key))
	{
		return widgetHash[key];
	}
	else
	{
		return nullptr;
	}
}

QString TreeModel::getKey(QString wdwName, QString roiName, int slice)
{
	QString key = wdwName + QString("=") + roiName + QString("=") + QString("%1").arg(slice + 1);
	return key;
}

void TreeModel::setWidgetsDisabled(QString wdwName, int slice)
{
	QStringList roiList = getRoiList(wdwName);
	QStringList keyList; //generate a list of keys at this slice of this window. 
	foreach(QString roiName, roiList)
	{
		QString key = getKey(wdwName, roiName, slice);
		if (widgetHash.contains(key))
		{
			keyList << key;
		}
	}

	qDebug() << "exemption list is " << keyList;
	QHashIterator<QString, vtkContourWidget * > wdwIter(widgetHash);
	while (wdwIter.hasNext())
	{
		wdwIter.next();
		QStringList keyString = wdwIter.key().split("=");		
		if (!keyList.contains(wdwIter.key()) && keyString[0] == wdwName)
		{
			qDebug() << "diabling " << wdwIter.key();
			vtkContourWidget* widget = wdwIter.value();
			widget->Off();
		}
	}	
}

void TreeModel::setRepsDefault()
{
	QHashIterator<QString, vtkOrientedGlyphContourRepresentation * > wdwIter(repHash);
	while (wdwIter.hasNext())
	{
		wdwIter.next();
		vtkOrientedGlyphContourRepresentation* rep = wdwIter.value();
		rep->GetLinesProperty()->SetLineWidth(DEFAULT_CONTOUR_LENGTH);
	}
}

QColor TreeModel::getColor(QString roiName)
{
	if (roiColorHash.contains(roiName))
	{
		return roiColorHash[roiName];
	}
	else
	{
		return QColor(0, 0, 0);
	}
}

void TreeModel::addColor(QString roiName, QColor roiColor)
{
	if (roiColorHash.contains(roiName))
	{
		roiColorHash[roiName] = roiColor;
	}
	else
	{
		roiColorHash.insert(roiName, roiColor);
	}
}

void TreeModel::removeColor(QString roiName)
{
	if (roiColorHash.contains(roiName))
	{
		roiColorHash.remove(roiName);
	}
}

bool TreeModel::checkColorDuplicate(QColor roiColor)
{
	return roiColorHash.values().contains(roiColor);
}

void TreeModel::setupModelData(const QStringList &lines, TreeItem *parent)
{
    QList<TreeItem*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ')
                break;
            ++position;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split(" ", QString::SkipEmptyParts);
            QVector<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
                columnData << columnStrings[column];

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            TreeItem *parent = parents.last();
            parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
            for (int column = 0; column < columnData.size(); ++column)
                parent->child(parent->childCount() - 1)->setData(column, columnData[column]);
        }

        ++number;
    }
}
