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

#include "qMRMLScenePatientHierarchyModel.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"

// Patient Hierarchy includes
#include "qMRMLScenePatientHierarchyModel_p.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLDisplayNode.h>

// Qt includes
#include <QMimeData>

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModelPrivate::qMRMLScenePatientHierarchyModelPrivate(qMRMLScenePatientHierarchyModel& object)
: Superclass(object)
{
  this->NodeTypeColumn = -1;

  this->BeamIcon = QIcon(":Icons/Beam.png");
  this->ColorTableIcon = QIcon(":Icons/ColorTable.png");
  this->ContourIcon = QIcon(":Icons/Contour.png");
  this->DoseVolumeIcon = QIcon(":Icons/DoseVolume.png");
  this->IsocenterIcon = QIcon(":Icons/Isocenter.png");
  this->PatientIcon = QIcon(":Icons/Patient.png");
  this->PlanIcon = QIcon(":Icons/Plan.png");
  this->ShowInViewersIcon = QIcon(":Icons/ShowInViewers.png");
  this->StructureSetIcon = QIcon(":Icons/StructureSet.png");
  this->StudyIcon = QIcon(":Icons/Study.png");
  this->VolumeIcon = QIcon(":Icons/Volume.png");
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModelPrivate::init()
{
  Q_Q(qMRMLScenePatientHierarchyModel);
  this->Superclass::init();

  q->setNameColumn(0);
  q->setNodeTypeColumn(q->nameColumn());
  q->setVisibilityColumn(1);
  q->setIDColumn(2);

  q->setHorizontalHeaderLabels(
    QStringList() << "Nodes" << "Vis" << "IDs");

  q->horizontalHeaderItem(0)->setToolTip(QObject::tr("Node name and type"));
  q->horizontalHeaderItem(1)->setToolTip(QObject::tr("Show/hide branch or node"));
  q->horizontalHeaderItem(2)->setToolTip(QObject::tr("Node ID"));
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::qMRMLScenePatientHierarchyModel(QObject *vparent)
: Superclass(new qMRMLScenePatientHierarchyModelPrivate(*this), vparent)
{
  Q_D(qMRMLScenePatientHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::~qMRMLScenePatientHierarchyModel()
{
}

//------------------------------------------------------------------------------
QStringList qMRMLScenePatientHierarchyModel::mimeTypes()const
{
  QStringList types;
  types << "application/vnd.text.list";
  return types;
}

//------------------------------------------------------------------------------
QMimeData* qMRMLScenePatientHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (const QModelIndex &index, indexes)
  {
    // Only add one pointer per row
    if (index.isValid() && index.column() == 0)
    {
      QString text = data(index, PointerRole).toString();
      stream << text;
    }
  }

  mimeData->setData("application/vnd.text.list", encodedData);
  return mimeData;
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  if (!node)
    {
    return false;
    }
  return node->IsA("vtkMRMLNode");
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAParent(vtkMRMLNode* node)const
{
  vtkMRMLHierarchyNode* hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if ( hnode
    && SlicerRtCommon::IsPatientHierarchyNode(hnode)
    && hnode->GetAssociatedNodeID() == 0 )
    {
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
int qMRMLScenePatientHierarchyModel::nodeTypeColumn()const
{
  Q_D(const qMRMLScenePatientHierarchyModel);
  return d->NodeTypeColumn;
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::setNodeTypeColumn(int column)
{
  Q_D(qMRMLScenePatientHierarchyModel);
  d->NodeTypeColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLScenePatientHierarchyModel::maxColumnId()const
{
  Q_D(const qMRMLScenePatientHierarchyModel);
  int maxId = this->Superclass::maxColumnId();
  maxId = qMax(maxId, d->VisibilityColumn);
  maxId = qMax(maxId, d->NodeTypeColumn);
  maxId = qMax(maxId, d->NameColumn);
  maxId = qMax(maxId, d->IDColumn);
  return maxId;
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLScenePatientHierarchyModel);
  if (column == this->nameColumn())
  {
    item->setText(QString(node->GetName()));
    item->setToolTip(node->GetNodeTagName());
  }
  if (column == this->idColumn())
  {
    item->setText(QString(node->GetID()));
  }
  if (column == this->visibilityColumn())
  {
    int visible = -1;
    if (SlicerRtCommon::IsPatientHierarchyNode(node))
    {
      visible = vtkSlicerPatientHierarchyModuleLogic::GetBranchVisibility( vtkMRMLHierarchyNode::SafeDownCast(node) );
    }
    else if (node->IsA("vtkMRMLContourNode"))
    {
      vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
      visible = contourNode->GetDisplayVisibility();
    }
    else if (node->IsA("vtkMRMLDisplayableNode"))
    {
      vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(node);
      visible = displayableNode->GetDisplayVisibility();
    }

    // Always set a different icon to volumes. If not a volume then set the appropriate eye icon
    if (node->IsA("vtkMRMLVolumeNode"))
    {
      item->setIcon(d->ShowInViewersIcon);
    }
    // It should be fine to set the icon even if it is the same, but due
    // to a bug in Qt (http://bugreports.qt.nokia.com/browse/QTBUG-20248),
    // it would fire a superflous itemChanged() signal.
    else if (item->data(VisibilityRole).isNull() || item->data(VisibilityRole).toInt() != visible)
    {
      item->setData(visible, VisibilityRole);
      switch (visible)
      {
      case 0:
        item->setIcon(d->HiddenIcon);
        break;
      case 1:
        item->setIcon(d->VisibleIcon);
        break;
      case 2:
        item->setIcon(d->PartiallyVisibleIcon);
        break;
      default:
        break;
      }
    }
  }
  if (column == this->nodeTypeColumn())
  {
    if (SlicerRtCommon::IsPatientHierarchyNode(node))
    {
      if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel(node,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_PATIENT) )
      {
        item->setIcon(d->PatientIcon);
      }
      else if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel(node,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY) )
      {
        item->setIcon(d->StudyIcon);
      }
      else if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel(node,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES) )
      {
        if (node->IsA("vtkMRMLContourHierarchyNode"))
        {
          item->setIcon(d->StructureSetIcon);
        }
        //TODO: Set icon for plan
      }
      else if ( vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel(node,
        vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES) )
      {
        // Set icon for PH type subseries objects here
      }
      else
      {
        vtkWarningWithObjectMacro(node, "Invalid DICOM level found for node '" << node->GetName() << "'");
      }
    }
    else if (node->IsA("vtkMRMLVolumeNode"))
    {
      if (SlicerRtCommon::IsDoseVolumeNode(node))
      {
        item->setIcon(d->DoseVolumeIcon);
      }
      else
      {
        item->setIcon(d->VolumeIcon);
      }
    }
    else if (node->IsA("vtkMRMLColorTableNode"))
    {
      item->setIcon(d->ColorTableIcon);
    }
    else if (node->IsA("vtkMRMLContourNode"))
    {
      item->setIcon(d->ContourIcon);
    }
    else if (node->IsA("vtkMRMLAnnotationFiducialNode"))
    {
      // TODO: add check to make sure it is an actual isocenter
      item->setIcon(d->IsocenterIcon);
    }
    else if (node->IsA("vtkMRMLModelNode"))
    {
      // TODO: add check to make sure it is an actual beam
      item->setIcon(d->BeamIcon);
    }
  }
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  if ( item->column() == this->nameColumn() )
  {
    node->SetName(item->text().toLatin1().constData());
  }
  if ( item->column() == this->visibilityColumn()
    && !item->data(VisibilityRole).isNull() )
  {
    int visible = item->data(VisibilityRole).toInt();
    if (visible > -1)
    {
      if (SlicerRtCommon::IsPatientHierarchyNode(node))
      {
        vtkSlicerPatientHierarchyModuleLogic::SetBranchVisibility( vtkMRMLHierarchyNode::SafeDownCast(node), visible );
      }
      else if (node->IsA("vtkMRMLContourNode"))
      {
        vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
        contourNode->SetDisplayVisibility(visible);
      }
      else if (node->IsA("vtkMRMLDisplayableNode") && !node->IsA("vtkMRMLVolumeNode"))
      {
        vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(node);
        displayableNode->SetDisplayVisibility(visible);

        vtkMRMLDisplayNode* displayNode = displayableNode->GetDisplayNode();
        if (displayNode)
        {
          displayNode->SetSliceIntersectionVisibility(visible);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  if (action == Qt::IgnoreAction)
  {
    return true;
  }
  if (!this->mrmlScene())
  {
    std::cerr << "qMRMLScenePatientHierarchyModel::dropMimeData: Invalid MRML scene!" << std::endl;
    return false;
  }
  if (!data->hasFormat("application/vnd.text.list"))
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Plain text MIME type is expected");
    return false;
  }

  // Nothing can be dropped to the top level (patients can only be loaded at the moment from the DICOM browser)
  if (!parent.isValid())
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Items cannot be dropped on top level!");
    return false;
  }
  vtkMRMLNode* parentNode = this->mrmlNodeFromIndex(parent);
  if (!parentNode)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Unable to get parent node!");
    return false;
  }

  // Decode MIME data
  QByteArray encodedData = data->data("application/vnd.text.list");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  QStringList streamItems;
  int rows = 0;

  while (!stream.atEnd())
  {
    QString text;
    stream >> text;
    streamItems << text;
    ++rows;
  }

  if (rows == 0)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Unable to decode dropped MIME data!");
    return false;
  }
  if (rows > 1)
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: More than one data item decoded from dropped MIME data! Only the first one will be used.");
  }

  QString nodePointerString = streamItems[0];

  vtkMRMLNode* droppedNode = vtkMRMLNode::SafeDownCast(reinterpret_cast<vtkObject*>(nodePointerString.toULongLong()));
  if (!droppedNode)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::dropMimeData: Unable to get MRML node from dropped MIME text (" << nodePointerString.toLatin1().constData() << ")!");
    return false;
  }

  // Reparent the node
  return this->reparent(droppedNode, parentNode);
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::reparent(vtkMRMLNode* node, vtkMRMLNode* newParent)
{
  if (!node || this->parentNode(node) == newParent)
  {
    return false;
  }
  Q_ASSERT(newParent != node);

  if (!this->mrmlScene())
  {
    std::cerr << "qMRMLScenePatientHierarchyModel::reparent: Invalid MRML scene!" << std::endl;
    return false;
  }

  if (!this->canBeAParent(newParent))
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::reparent: Target parent node (" << newParent->GetName() << ") is not a valid patient hierarchy parent node!");
  }

  vtkMRMLHierarchyNode* parentPatientHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(newParent);

  // Get possible associated hierarchy node for reparented node
  vtkMRMLHierarchyNode* associatedPatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(this->mrmlScene(), node->GetID());
  vtkMRMLHierarchyNode* associatedNonPatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedNonPatientHierarchyNode(this->mrmlScene(), node->GetID());

  // Delete associated hierarchy node if it's not a patient hierarchy node. Should not occur unless it is an annotation hierarchy node
  // (they are an exception as they are needed for the Annotations module; TODO: review when Annotations module ha been improved)
  if (associatedNonPatientHierarchyNode && !associatedNonPatientHierarchyNode->IsA("vtkMRMLAnnotationHierarchyNode"))
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLScenePatientHierarchyModel::reparent: Reparented node had a non-patient hierarchy node associated!");
    this->mrmlScene()->RemoveNode(associatedNonPatientHierarchyNode);
  }

  // Assign parent and node if associated patient hierarchy node already exists and valid
  if (associatedPatientHierarchyNode)
  {
    associatedPatientHierarchyNode->SetParentNodeID(parentPatientHierarchyNode->GetID());
    associatedPatientHierarchyNode->SetAssociatedNodeID(node->GetID());
  }
  // Create patient hierarchy node if dropped from outside the tree (the potential nodes list) OR if deleted in previous check
  else
  {
    // If parent is a contour hierarchy node than create contour for the dropped contour representation
    if (parentPatientHierarchyNode->IsA("vtkMRMLContourHierarchyNode"))
    {
      // Create hierarchy node for contour node
      associatedPatientHierarchyNode = vtkMRMLDisplayableHierarchyNode::New();
      associatedPatientHierarchyNode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(this->mrmlScene()->AddNode(associatedPatientHierarchyNode));

      // Create contour node if dropped node is volume or model
      if ( (node->IsA("vtkMRMLModelNode") && !node->IsA("vtkMRMLAnnotationNode"))
        || node->IsA("vtkMRMLScalarVolumeNode") )
      {
        vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
        contourNode = vtkMRMLContourNode::SafeDownCast(this->mrmlScene()->AddNode(contourNode));
        std::string contourName(node->GetName());
        contourName.append(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX.c_str());
        contourName = this->mrmlScene()->GenerateUniqueName(contourName);
        contourNode->SetName(contourName.c_str());
        //contourNode->SetStructureName(roiLabel); //TODO: Utilize PH so that this variable is not needed
        if (node->IsA("vtkMRMLScalarVolumeNode"))
        {
          contourNode->SetAndObserveIndexedLabelmapVolumeNodeId(node->GetID());
        }
        else
        {
          contourNode->SetAndObserveClosedSurfaceModelNodeId(node->GetID());
        }
        contourNode->HideFromEditorsOff();
        associatedPatientHierarchyNode->SetAssociatedNodeID(contourNode->GetID());
      }
      else
      {
        associatedPatientHierarchyNode->SetAssociatedNodeID(node->GetID());
      }
    }
    else
    {
      associatedPatientHierarchyNode = vtkMRMLHierarchyNode::New();
      associatedPatientHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(this->mrmlScene()->AddNode(associatedPatientHierarchyNode));
      associatedPatientHierarchyNode->SetAssociatedNodeID(node->GetID());
    }

    std::string phNodeName;
    phNodeName = std::string(node->GetName()) + SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX;
    phNodeName = this->mrmlScene()->GenerateUniqueName(phNodeName);
    associatedPatientHierarchyNode->SetName(phNodeName.c_str());
    associatedPatientHierarchyNode->HideFromEditorsOff();
    associatedPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
      SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
    //TODO: Subseries level is the default for now. This and UID has to be specified before export (need the tag editor widget)
    associatedPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);

    associatedPatientHierarchyNode->SetParentNodeID(parentPatientHierarchyNode->GetID());
  }

  // Force updating the whole scene (TODO: this should not be needed)
  this->updateScene();

  return true;
}
