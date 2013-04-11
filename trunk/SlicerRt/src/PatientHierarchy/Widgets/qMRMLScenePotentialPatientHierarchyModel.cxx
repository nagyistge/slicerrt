/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "qMRMLScenePotentialPatientHierarchyModel.h"

// Patient Hierarchy includes
#include "qMRMLScenePotentialPatientHierarchyModel_p.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLNode.h>

// Qt includes
#include <QMimeData>

//------------------------------------------------------------------------------
qMRMLScenePotentialPatientHierarchyModelPrivate::qMRMLScenePotentialPatientHierarchyModelPrivate(qMRMLScenePotentialPatientHierarchyModel& object)
: Superclass(object)
{
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModelPrivate::init()
{
  Q_Q(qMRMLScenePotentialPatientHierarchyModel);
  this->Superclass::init();
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLScenePotentialPatientHierarchyModel::qMRMLScenePotentialPatientHierarchyModel(QObject *vparent)
: Superclass(new qMRMLScenePotentialPatientHierarchyModelPrivate(*this), vparent)
{
  Q_D(qMRMLScenePotentialPatientHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLScenePotentialPatientHierarchyModel::~qMRMLScenePotentialPatientHierarchyModel()
{
}

//------------------------------------------------------------------------------
QStringList qMRMLScenePotentialPatientHierarchyModel::mimeTypes()const
{
  QStringList types;
  types << "application/vnd.text.list";
  return types;
}

//------------------------------------------------------------------------------
QMimeData* qMRMLScenePotentialPatientHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (const QModelIndex &index, indexes)
  {
    if (index.isValid())
    {
      QString text = data(index, PointerRole).toString();
      stream << text;
    }
  }

  mimeData->setData("application/vnd.text.list", encodedData);
  return mimeData;
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLScenePotentialPatientHierarchyModel);

  item->setText(QString(node->GetName()));
  item->setToolTip(QString(node->GetNodeTagName()) + " type (" + QString(node->GetID()) + ")");
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  node->SetName(item->text().toLatin1().constData());
}

//------------------------------------------------------------------------------
bool qMRMLScenePotentialPatientHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  return vtkSlicerPatientHierarchyModuleLogic::IsPotentialPatientHierarchyNode(node);
}

//------------------------------------------------------------------------------
Qt::DropActions qMRMLScenePotentialPatientHierarchyModel::supportedDropActions()const
{
  return Qt::MoveAction;
}
