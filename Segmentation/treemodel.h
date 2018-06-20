/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include "treeitem.h"
#include "vtkSmartPointer.h"

class TreeItem;
class vtkOrientedGlyphContourRepresentation;
class vtkContourWidget;

//This should be managed by a configuration file
#define ACTIVE_CONTOUR_LENGTH 4.0
#define DEFAULT_CONTOUR_LENGTH 1.5

//! [0]
class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TreeModel(const QStringList &headers,
              QObject *parent = 0);
    ~TreeModel();
//! [0] //! [1]

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
//! [1]

//! [2]
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole) override;

    bool insertColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;

    // Roi is a special type of tree item.
    // if add
    //
    bool insertROIs(QStringList &newRow, QString roiName, QString sliceName, QString window);
    bool insertROIs(QStringList &newRow, QString roiName, QString sliceName, QString window,  QString component);

    bool removeROIs(QString RoiName);
    bool removeROIs(QString RoiName, int Slice);
	bool removeROIs(QString wdwName, QString RoiName);
	bool removeROIs(QString wdwName, QString RoiName, QString SliceName);

    int isRoiPresent(QString roiname, QString curSlice);
	int isRoiPresent(QString roiName);
	int isRoiPresent(QString wdwname, QString roiname, QString curSlice);

    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex() )override;

	// store vtkcontourRepresentaiton into the hash. 
	bool storeReps(QString WdwName, QString roiName, int slice, vtkOrientedGlyphContourRepresentation* rep);
	vtkOrientedGlyphContourRepresentation* getReps(QString WdwName, QString roiName, int slice);

	bool storeWidgets(QString WdwName, QString roiName, int slice, vtkContourWidget* rep);
	vtkContourWidget* getWidgets(QString WdwName, QString roiName, int slice);

    TreeItem* getrootItem();

    QStringList getRoiList() const;
	QStringList getRoiList(QString wdwName) const;
    QStringList getSliceList(QString) const;
	QStringList getSliceList(QString wdwName, QString RoiName) const;
	QStringList getWindowList() const;

	QColor getColor(QString roiName);
	void removeColor(QString roiName);
	void addColor(QString roiName, QColor roiColor);
	bool checkColorDuplicate(QColor roiColor);

	QString getKey(QString wdwName, QString roiName, int slice);

	void setRepsDefault();
	void setWidgetsDisabled(QString wdwName, int slice);
signals:
	void roiInserted(QString);

private:
    void setupModelData(const QStringList &lines, TreeItem *parent);
    TreeItem *getItem(const QModelIndex &index) const;
    TreeItem *rootItem;

	//QHash<int, QHash<QString, vtkOrientedGlyphContourRepresentation*> > RoiHash;
	QHash<QString, vtkOrientedGlyphContourRepresentation*> repHash;
	QHash<QString, vtkContourWidget*> widgetHash;
	QHash<QString, QColor> roiColorHash;
};
//! [2]

#endif // TREEMODEL_H
