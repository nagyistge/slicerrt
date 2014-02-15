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

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSubjectHierarchyConstants.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyVolumesPlugin.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Slicer includes
#include "qSlicerCoreApplication.h"
#include "vtkSlicerApplicationLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QStandardItem>
#include <QAction>

// STD includes
#include <set>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class qSlicerSubjectHierarchyVolumesPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyVolumesPlugin);
protected:
  qSlicerSubjectHierarchyVolumesPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyVolumesPluginPrivate(qSlicerSubjectHierarchyVolumesPlugin& object);
  ~qSlicerSubjectHierarchyVolumesPluginPrivate();
  void init();
public:
  QIcon LabelmapIcon;
  QIcon VolumeIcon;
  QIcon VolumeVisibilityOffIcon;
  QIcon VolumeVisibilityOnIcon;

  QAction* ToggleLabelmapOutlineDisplayAction;
  QAction* ShowVolumesInStudyAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyVolumesPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPluginPrivate::qSlicerSubjectHierarchyVolumesPluginPrivate(qSlicerSubjectHierarchyVolumesPlugin& object)
: q_ptr(&object)
{
  this->LabelmapIcon = QIcon(":Icons/Labelmap.png");
  this->VolumeIcon = QIcon(":Icons/Volume.png");
  this->VolumeVisibilityOffIcon = QIcon(":Icons/VolumeVisibilityOff.png");
  this->VolumeVisibilityOnIcon = QIcon(":Icons/VolumeVisibilityOn.png");

  this->ToggleLabelmapOutlineDisplayAction = NULL;
  this->ShowVolumesInStudyAction = NULL;
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPluginPrivate::~qSlicerSubjectHierarchyVolumesPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPlugin::qSlicerSubjectHierarchyVolumesPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyVolumesPluginPrivate(*this) )
{
  this->m_Name = QString("Volumes");

  Q_D(qSlicerSubjectHierarchyVolumesPlugin);
  d->init();
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyVolumesPlugin);

  this->ToggleLabelmapOutlineDisplayAction = new QAction("Toggle labelmap outline display",q);
  QObject::connect(this->ToggleLabelmapOutlineDisplayAction, SIGNAL(toggled(bool)), q, SLOT(toggleLabelmapOutlineDisplay(bool)));
  this->ToggleLabelmapOutlineDisplayAction->setCheckable(true);
  this->ToggleLabelmapOutlineDisplayAction->setChecked(false);

  this->ShowVolumesInStudyAction = new QAction("Show volumes in study",q);
  QObject::connect(this->ShowVolumesInStudyAction, SIGNAL(triggered()), q, SLOT(showVolumesInStudy()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyVolumesPlugin::~qSlicerSubjectHierarchyVolumesPlugin()
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerSubjectHierarchyVolumesPlugin::dependencies()const
{
  return QStringList() << "DICOM";
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyVolumesPlugin::canOwnSubjectHierarchyNode(
  vtkMRMLSubjectHierarchyNode* node, QString &role/*=QString()*/)
{
  role = QString();
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  // Volume
  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    role = QString("Scalar volume");
    return 0.5; // There are other plugins that can handle special volume nodes better, thus the relatively low value
  }

  // Study level (so that bulk show/hide of volumes is possible)
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    role = QString("Study (can show volumes)");
    return 0.5;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyVolumesPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  Q_D(qSlicerSubjectHierarchyVolumesPlugin);

  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::setIcon: NULL node or item given!";
    return false;
  }

  // Volume
  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    const char* labelmapAttribute = associatedNode->GetAttribute("LabelMap");
    if (labelmapAttribute && strcmp(labelmapAttribute, "0"))
    {
      item->setIcon(d->LabelmapIcon);
    }
    else
    {
      item->setIcon(d->VolumeIcon);
    }
    return true;
  }

  // Study level
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("DICOM")->setIcon(node, item);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::setVisibilityIcon: NULL node or item given!";
    return;
  }

  Q_D(qSlicerSubjectHierarchyVolumesPlugin);

  // Volume
  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    if (this->getDisplayVisibility(node) == 1)
    {
      item->setIcon(d->VolumeVisibilityOnIcon);
    }
    else
    {
      item->setIcon(d->VolumeVisibilityOffIcon);
    }
    return;
  }

  // Study level
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("DICOM")->setVisibilityIcon(node, item);
    return;
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::setDisplayVisibility: NULL node!";
    return;
  }

  vtkMRMLScalarVolumeNode* associatedVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node->GetAssociatedDataNode());
  // Volume
  if (associatedVolumeNode)
  {
    this->showVolume(associatedVolumeNode, visible);
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
  }
}

//-----------------------------------------------------------------------------
int qSlicerSubjectHierarchyVolumesPlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::getDisplayVisibility: NULL node!";
    return -1;
  }

  // Study level
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    return qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("DICOM")->getDisplayVisibility(node);
  }

  // Sanity checks for volume
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node->GetAssociatedDataNode());
  if (!volumeNode)
  {
    return -1;
  }
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolume: Unable to get selection node to show volume node " << node->GetName();
    return -1;
  }

  /// Update selection node based on current volumes visibility (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  this->updateSelectionNodeBasedOnCurrentVolumesVisibility();

  const char* labelmapAttribute = volumeNode->GetAttribute("LabelMap");
  if (labelmapAttribute && strcmp(labelmapAttribute, "0"))
  {
    if ( selectionNode->GetActiveLabelVolumeID() && !strcmp(selectionNode->GetActiveLabelVolumeID(), volumeNode->GetID()) )
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    if ( ( selectionNode->GetActiveVolumeID() && !strcmp(volumeNode->GetID(), selectionNode->GetActiveVolumeID()) )
      || ( selectionNode->GetSecondaryVolumeID() && !strcmp(volumeNode->GetID(), selectionNode->GetSecondaryVolumeID()) ) )
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::showVolume(vtkMRMLScalarVolumeNode* node, int visible/*=1*/)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolume: NULL node!";
    return;
  }

  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolume: Unable to get selection node to show volume node " << node->GetName();
    return;
  }

  // Get volume node
  vtkMRMLScalarVolumeNode* volumeNode = NULL;
  if ((volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node)) == NULL)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolume: Note to show node is not a volume node: " << node->GetName();
    return;
  }

  /// Update selection node based on current volumes visibility (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  this->updateSelectionNodeBasedOnCurrentVolumesVisibility();

  const char* labelmapAttribute = volumeNode->GetAttribute("LabelMap");
  if (labelmapAttribute && strcmp(labelmapAttribute, "0")) // Labelmap
  {
    if (visible)
    {
      if (selectionNode->GetActiveLabelVolumeID() && strlen(selectionNode->GetActiveLabelVolumeID()))
      {
        // Needed so that visibility icon is updated (could be done in a faster way, but there is no noticeable overhead)
        vtkMRMLScalarVolumeNode* originalLabelmapNode = vtkMRMLScalarVolumeNode::SafeDownCast(
          scene->GetNodeByID(selectionNode->GetActiveLabelVolumeID()) );
        this->showVolume(originalLabelmapNode, 0);
      }
      selectionNode->SetActiveLabelVolumeID(volumeNode->GetID());
    }
    else
    {
      selectionNode->SetActiveLabelVolumeID(NULL);
    }
    qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
  }
  else // Not labelmap
  {
    if (visible)
    {
      if (!selectionNode->GetActiveVolumeID() || !strlen(selectionNode->GetActiveVolumeID()))
      {
        selectionNode->SetActiveVolumeID(volumeNode->GetID());
      }
      else if (!selectionNode->GetSecondaryVolumeID() || !strlen(selectionNode->GetSecondaryVolumeID()))
      {
        selectionNode->SetSecondaryVolumeID(volumeNode->GetID());
      }
      else
      {
        // Show volume instead of the original secondary volume
        vtkMRMLScalarVolumeNode* originalSecondaryVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
          scene->GetNodeByID(selectionNode->GetSecondaryVolumeID()) );
        // Needed so that visibility icon is updated (could be done in a faster way, but there is no noticeable overhead)
        this->showVolume(originalSecondaryVolumeNode, 0);

        selectionNode->SetSecondaryVolumeID(volumeNode->GetID());
      }

      qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();

      // Make sure the secondary volume is shown in a semi-transparent way
      vtkMRMLSliceCompositeNode* compositeNode = NULL;
      int numberOfCompositeNodes = scene->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
      for (int i=0; i<numberOfCompositeNodes; i++)
      {
        compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( scene->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
        if (compositeNode && compositeNode->GetForegroundOpacity() == 0.0)
        {
          compositeNode->SetForegroundOpacity(0.5);
        }
      }
    }
    else
    {
      if ( selectionNode->GetActiveVolumeID() && !strcmp(volumeNode->GetID(), selectionNode->GetActiveVolumeID()) )
      {
        selectionNode->SetActiveVolumeID(NULL);
      }
      else if ( selectionNode->GetSecondaryVolumeID() && !strcmp(volumeNode->GetID(), selectionNode->GetSecondaryVolumeID()) )
      {
        selectionNode->SetSecondaryVolumeID(NULL);
      }
      qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
    }
  }

  // Get subject hierarchy node for the volume node and have the scene model updated
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(volumeNode);
  if (subjectHierarchyNode)
  {
    subjectHierarchyNode->Modified();
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::updateSelectionNodeBasedOnCurrentVolumesVisibility()
{
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::updateSelectionNodeBasedOnCurrentVolumesVisibility: Unable to get selection node";
    return;
  }

  // TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)

  // Determine labelmap selection (if the selection is different in the slice viewers, then the first one is set)
  std::string selectedLabelmapID = this->getSelectedLabelmapVolumeNodeID();
  selectionNode->SetActiveLabelVolumeID(selectedLabelmapID.c_str());

  // Determine background volume selection (if the selection is different in the slice viewers, then the first one is set)
  std::string selectedBackgroundVolumeID = this->getSelectedBackgroundVolumeNodeID();
  selectionNode->SetActiveVolumeID(selectedBackgroundVolumeID.c_str());

  // Determine foreground volume selection (if the selection is different in the slice viewers, then the first one is set)
  std::string selectedForegroundVolumeID = this->getSelectedForegroundVolumeNodeID();
  selectionNode->SetSecondaryVolumeID(selectedForegroundVolumeID.c_str());
}

//---------------------------------------------------------------------------
std::string qSlicerSubjectHierarchyVolumesPlugin::getSelectedLabelmapVolumeNodeID()
{
  // TODO: This method is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string selectedLabelmapID("");

  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::getSelectedLabelmapVolumeNodeID: Unable to get selection node";
    return selectedLabelmapID;
  }

  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  vtkMRMLSliceCompositeNode* compositeNode = NULL;
  const int numberOfCompositeNodes = scene->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");

  for (int i=0; i<numberOfCompositeNodes; i++)
  {
    compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( scene->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
    if (compositeNode && compositeNode->GetLabelVolumeID() && selectedLabelmapID.empty())
    {
      selectedLabelmapID = std::string(compositeNode->GetLabelVolumeID());
      break;
    }
  }

  return selectedLabelmapID;
}

//---------------------------------------------------------------------------
std::string qSlicerSubjectHierarchyVolumesPlugin::getSelectedBackgroundVolumeNodeID()
{
  // TODO: This method is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string selectedBackgroundVolumeID("");

  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::getSelectedBackgroundVolumeNodeID: Unable to get selection node";
    return selectedBackgroundVolumeID;
  }

  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  vtkMRMLSliceCompositeNode* compositeNode = NULL;
  const int numberOfCompositeNodes = scene->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");

  for (int i=0; i<numberOfCompositeNodes; i++)
  {
    compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( scene->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
    if (compositeNode && compositeNode->GetBackgroundVolumeID() && selectedBackgroundVolumeID.empty())
    {
      selectedBackgroundVolumeID = std::string(compositeNode->GetBackgroundVolumeID());
      break;
    }
  }

  return selectedBackgroundVolumeID;
}

//---------------------------------------------------------------------------
std::string qSlicerSubjectHierarchyVolumesPlugin::getSelectedForegroundVolumeNodeID()
{
  // TODO: This method is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string selectedForegroundVolumeID("");

  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::getSelectedForegroundVolumeNodeID: Unable to get selection node";
    return selectedForegroundVolumeID;
  }

  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  vtkMRMLSliceCompositeNode* compositeNode = NULL;
  const int numberOfCompositeNodes = scene->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");

  for (int i=0; i<numberOfCompositeNodes; i++)
  {
    compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( scene->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
    if (compositeNode && compositeNode->GetForegroundVolumeID() && selectedForegroundVolumeID.empty())
    {
      selectedForegroundVolumeID = std::string(compositeNode->GetForegroundVolumeID());
      break;
    }
  }

  return selectedForegroundVolumeID;
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyVolumesPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyVolumesPlugin);

  QList<QAction*> actions;
  actions << d->ToggleLabelmapOutlineDisplayAction << d->ShowVolumesInStudyAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::hideAllContextMenuActions()
{
  Q_D(qSlicerSubjectHierarchyVolumesPlugin);

  d->ToggleLabelmapOutlineDisplayAction->setVisible(false);
  d->ShowVolumesInStudyAction->setVisible(false);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::showContextMenuActionsForHandlingNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyVolumesPlugin);

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // Volume
  if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    const char* labelmapAttribute = associatedNode->GetAttribute("LabelMap");
    if (labelmapAttribute && strcmp(labelmapAttribute, "0"))
    {
      // Determine current state of the toggle labelmap outline checkbox (from the first slice view)
      vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast ( scene->GetNthNodeByClass( 0, "vtkMRMLSliceNode" ) );
      int useLabelOutline = sliceNode->GetUseLabelOutline();
      d->ToggleLabelmapOutlineDisplayAction->blockSignals(true);
      d->ToggleLabelmapOutlineDisplayAction->setChecked(useLabelOutline);
      d->ToggleLabelmapOutlineDisplayAction->blockSignals(false);

      d->ToggleLabelmapOutlineDisplayAction->setVisible(true);
    }
  }

  // Study level
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    d->ShowVolumesInStudyAction->setVisible(true);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::toggleLabelmapOutlineDisplay(bool checked)
{
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  vtkMRMLSliceNode* sliceNode = NULL;
  const int numberOfSliceNodes = scene->GetNumberOfNodesByClass("vtkMRMLSliceNode");

  for (int i=0; i<numberOfSliceNodes; i++)
  {
    sliceNode = vtkMRMLSliceNode::SafeDownCast ( scene->GetNthNodeByClass( i, "vtkMRMLSliceNode" ) );
    sliceNode->SetUseLabelOutline(checked);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyVolumesPlugin::showVolumesInStudy()
{
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolumesInStudy: Unable to get selection node";
    return;
  }

  std::set<vtkMRMLSubjectHierarchyNode*> subjectHierarchyNodesToUpdate;
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();

  // Collect subject hierarchy nodes of currently shown volumes to be able to update them in the scene model
  vtkMRMLSubjectHierarchyNode* activeVolumeShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(
    scene->GetNodeByID(selectionNode->GetActiveVolumeID()) );
  subjectHierarchyNodesToUpdate.insert(activeVolumeShNode);
  vtkMRMLSubjectHierarchyNode* secondaryVolumeShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(
    scene->GetNodeByID(selectionNode->GetSecondaryVolumeID()) );
  subjectHierarchyNodesToUpdate.insert(secondaryVolumeShNode);
  vtkMRMLSubjectHierarchyNode* labelVolumeShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(
    scene->GetNodeByID(selectionNode->GetActiveLabelVolumeID()) );
  subjectHierarchyNodesToUpdate.insert(labelVolumeShNode);

  // Hide all volumes before showing the ones from the study
  selectionNode->SetActiveVolumeID(NULL);
  selectionNode->SetSecondaryVolumeID(NULL);
  selectionNode->SetActiveLabelVolumeID(NULL);

  // Show volumes in study
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkSmartPointer<vtkCollection> childVolumeNodes = vtkSmartPointer<vtkCollection>::New();
  vtkMRMLSubjectHierarchyNode::SafeDownCast(currentNode)->GetAssociatedChildrenNodes(childVolumeNodes, "vtkMRMLScalarVolumeNode");
  childVolumeNodes->InitTraversal();
  for (int i=0; i<childVolumeNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(childVolumeNodes->GetItemAsObject(i));
    if (volumeNode)
    {
      // Get subject hierarchy node for the volume node
      vtkMRMLSubjectHierarchyNode* volumeSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(volumeNode);
      if (!volumeSubjectHierarchyNode)
      {
        qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::showVolumesInStudy: Unable to get subject hierarchy node!";
        continue;
      }

      const char* labelmapAttribute = volumeNode->GetAttribute("LabelMap");
      if (labelmapAttribute && strcmp(labelmapAttribute, "0"))
      {
        // Show first labelmap in study only
        if (!selectionNode->GetActiveLabelVolumeID())
        {
          selectionNode->SetActiveLabelVolumeID(volumeNode->GetID());
          subjectHierarchyNodesToUpdate.insert(volumeSubjectHierarchyNode);
        }
      }
      else
      {
        // Show first two volume in study only
        if (!selectionNode->GetActiveVolumeID())
        {
          selectionNode->SetActiveVolumeID(volumeNode->GetID());
          subjectHierarchyNodesToUpdate.insert(volumeSubjectHierarchyNode);
        }
        else if (!selectionNode->GetSecondaryVolumeID())
        {
          selectionNode->SetSecondaryVolumeID(volumeNode->GetID());
          subjectHierarchyNodesToUpdate.insert(volumeSubjectHierarchyNode);

          // Make sure the secondary volume is shown in a semi-transparent way
          vtkMRMLSliceCompositeNode* compositeNode = NULL;
          int numberOfCompositeNodes = scene->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
          for (int i=0; i<numberOfCompositeNodes; i++)
          {
            compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast ( scene->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
            if (compositeNode && compositeNode->GetForegroundOpacity() == 0.0)
            {
              compositeNode->SetForegroundOpacity(0.5);
            }
          }
        }
      }
    }
  }

  qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();

  // Update scene model for subject hierarchy nodes just hidden
  for (std::set<vtkMRMLSubjectHierarchyNode*>::iterator volumeShNodeIt = subjectHierarchyNodesToUpdate.begin();
    volumeShNodeIt != subjectHierarchyNodesToUpdate.end(); ++volumeShNodeIt)
  {
    if (*volumeShNodeIt)
    {
      (*volumeShNodeIt)->Modified();
    }
  }

}
