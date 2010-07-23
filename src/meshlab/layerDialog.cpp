/****************************************************************************
* MeshLab                                                           o o     *
* An extendible mesh processor                                    o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005, 2006                                          \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
/****************************************************************************
History

$Log: stdpardialog.cpp,v $

****************************************************************************/

#include <GL/glew.h>
#include <QtGui>

#include "ui_layerDialog.h"
#include "layerDialog.h"
#include "mainwindow.h"

using namespace std;

LayerDialog::LayerDialog(QWidget *parent )    : QDockWidget(parent)
{
	ui = new Ui::layerDialog();
	setWindowFlags( windowFlags() | Qt::WindowStaysOnTopHint | Qt::SubWindow);
	setVisible(false);
	LayerDialog::ui->setupUi(this);
	mw=qobject_cast<MainWindow *>(parent);

	// The following connection is used to associate the click with the change of the current mesh. 
	connect(ui->layerTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem * , int  )) , this,  SLOT(toggleStatus(QTreeWidgetItem * , int ) ) );
	
	connect(ui->layerTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem * )) , this,  SLOT(adaptColumns(QTreeWidgetItem *)));
	connect(ui->layerTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem * )) , this,  SLOT(adaptColumns(QTreeWidgetItem *)));


	connect(ui->addButton, SIGNAL(clicked()), mw, SLOT(openIn()) );
	connect(ui->deleteButton, SIGNAL(clicked()), mw, SLOT(delCurrentMesh()) );

	this->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->layerTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(ui->layerTreeWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showContextMenu(const QPoint&)));
	connect(ui->menuButton, SIGNAL(clicked()), this, SLOT(showLayerMenu()));
}
void LayerDialog::toggleStatus (QTreeWidgetItem * item , int col)
{
	MeshTreeWidgetItem *mItem = dynamic_cast<MeshTreeWidgetItem *>(item);
	if(!mItem) return; // user clicked on other info 
	else{
    int clickedId= mItem->m->id();
		switch(col)
		{
		case 0 :
			{
				//the user has clicked on one of the eyes
        MeshDocument  *md= mw->GLA()->meshDoc;
				// NICE TRICK.
				// If the user has pressed ctrl when clicking on the eye icon, only that layer will remain visible
				// Very useful for comparing meshes

				if(QApplication::keyboardModifiers() == Qt::ControlModifier)
          foreach(MeshModel *mp, md->meshList)
				{
					mp->visible=false;
          mw->GLA()->updateLayerSetVisibility(mp->id(), mp->visible);
				}

        if(md->getMesh(clickedId)->visible)  md->getMesh(clickedId)->visible = false;
        else   md->getMesh(clickedId)->visible = true;

				//Update current GLArea visibility 
				//TODO. Evitare il metodo GLA()
        mw->GLA()->updateLayerSetVisibility(md->getMesh(clickedId)->id(), md->getMesh(clickedId)->visible);
			}
		case 1 :

		case 2 :

		case 3 :
			//the user has chosen to switch the layer
      mw->GLA()->meshDoc->setCurrentMesh(clickedId);
			break;
		}
		//make sure the right row is colored or that they right eye is drawn (open or closed)
		updateTable();
		mw->GLA()->update();
	}
}

void LayerDialog::showEvent ( QShowEvent * /* event*/ )
{
	updateTable();
}

void LayerDialog::showLayerMenu()
{
	foreach (QWidget *widget, QApplication::topLevelWidgets()) {
		MainWindow* mainwindow = dynamic_cast<MainWindow*>(widget);
		if (mainwindow)
		{
			mainwindow->layerMenu()->popup(ui->menuButton->mapToGlobal(QPoint(10,10)));
			return;
		}
	}
}

void LayerDialog::showContextMenu(const QPoint& pos)
{
	// switch layer
	MeshTreeWidgetItem *mItem = dynamic_cast<MeshTreeWidgetItem *>(ui->layerTreeWidget->itemAt(pos.x(),pos.y()));
	if(!mItem) return; // user clicked on other info 
	else{
    if (mItem->m)
      mw->GLA()->meshDoc->setCurrentMesh(mItem->m->id());

		foreach (QWidget *widget, QApplication::topLevelWidgets()) {
			MainWindow* mainwindow = dynamic_cast<MainWindow*>(widget);
			if (mainwindow)
			{
				mainwindow->layerMenu()->popup(ui->layerTreeWidget->mapToGlobal(pos));
				return;
			}
		}
	}
}

void LayerDialog::updateLog(GLLogStream &log)
{
	QList< pair<int,QString> > &logStringList=log.S;
	ui->logPlainTextEdit->clear();
	//ui->logPlainTextEdit->setFont(QFont("Courier",10));

	pair<int,QString> logElem;
	QString preWarn    = "<font face=\"courier\" size=3 color=\"red\"> Warning: " ;
	QString preSystem  = "<font face=\"courier\" size=2 color=\"grey\">" ;
	QString preFilter  = "<font face=\"courier\" size=2 color=\"black\">" ;

	QString post   = "</font>";

	foreach(logElem, logStringList){
		QString logText = logElem.second;
		if(logElem.first == GLLogStream::SYSTEM)  logText = preSystem + logText + post;
		if(logElem.first == GLLogStream::WARNING) logText = preWarn + logText + post;
		if(logElem.first == GLLogStream::FILTER)  logText = preFilter + logText + post;
		ui->logPlainTextEdit->appendHtml(logText);
	}
}

void LayerDialog::updateTable()
{
	//TODO:Check if the current viewer is a GLArea
	if(!isVisible()) return;
	if(isVisible() & !mw->GLA())
	{
		setVisible(false);
		//The layer dialog cannot be opened unless a new document is opened
		return;
	}
  MeshDocument *md=mw->GLA()->meshDoc;

	ui->layerTreeWidget->clear();
	ui->layerTreeWidget->setColumnCount(4);
	ui->layerTreeWidget->header()->hide();
  foreach(MeshModel* mmd, md->meshList)
	{
		//Restore mesh visibility according to the current visibility map
		//very good to keep viewer state consistent
    if( mw->GLA()->visibilityMap.contains(mmd->id()))
      mmd->visible =mw->GLA()->visibilityMap[mmd->id()];

    MeshTreeWidgetItem *item = new MeshTreeWidgetItem(mmd);
    if(mmd== mw->GLA()->mm()) {
			item->setBackground(3,QBrush(Qt::yellow));
			item->setForeground(3,QBrush(Qt::blue));
		}
    ui->layerTreeWidget->addTopLevelItem(item);

		//Adding default annotations
    addDefaultNotes(item, mmd);

		//Adding tags

	}
	for(int i=0; i< ui->layerTreeWidget->columnCount(); i++)
		ui->layerTreeWidget->resizeColumnToContents(i);
}

void LayerDialog::adaptColumns(QTreeWidgetItem * item)
{
	item->setExpanded(item->isExpanded());
	for(int i=0; i< ui->layerTreeWidget->columnCount(); i++)
		ui->layerTreeWidget->resizeColumnToContents(i);

}

//Add default annotations for each mesh about faces and vertices number
void LayerDialog::addDefaultNotes(QTreeWidgetItem * parent, MeshModel *meshModel)
{
	QTreeWidgetItem *faces = new QTreeWidgetItem();
	faces->setText(3, QString("Faces"));
	faces->setText(4, QString::number(meshModel->cm.fn));	
	parent->addChild(faces);
	updateColumnNumber(faces);

	QTreeWidgetItem *vertices = new QTreeWidgetItem();
	vertices->setText(3, QString("Vertices"));
	vertices->setText(4, QString::number(meshModel->cm.vn));	
	parent->addChild(vertices);
	updateColumnNumber(vertices);
}

//Add, if necessary, columns to the treeWidget. 
//It must be called every time a new treeWidget item is added to the tree.
void LayerDialog::updateColumnNumber(QTreeWidgetItem * item)
{
	int columnChild= item->columnCount();
	int columnParent = ui->layerTreeWidget->columnCount();
	if(columnChild - columnParent>0)
		ui->layerTreeWidget->setColumnCount(columnParent + (columnChild-columnParent));
}

LayerDialog::~LayerDialog()
{
	delete ui;
}

MeshTreeWidgetItem::MeshTreeWidgetItem(MeshModel *meshModel)
{
  if(meshModel->visible)  setIcon(0,QIcon(":/images/layer_eye_open.png"));
  else setIcon(0,QIcon(":/images/layer_eye_close.png"));

	setIcon(1,QIcon(":/images/layer_edit_unlocked.png"));

  setText(2, QString::number(meshModel->id()));
	
	QString meshName = meshModel->shortName(); 
	setText(3, meshName);	

	m=meshModel;
}



