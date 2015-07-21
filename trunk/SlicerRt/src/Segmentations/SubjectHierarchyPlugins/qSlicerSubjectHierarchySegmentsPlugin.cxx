/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// Segmentations includes
#include "qSlicerSubjectHierarchySegmentsPlugin.h"

#include "qSlicerSubjectHierarchySegmentationsPlugin.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Qt includes
#include <QDebug>
#include <QIcon>
#include <QMessageBox>

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// MRML widgets includes
#include "qMRMLNodeComboBox.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Segmentations
class qSlicerSubjectHierarchySegmentsPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchySegmentsPlugin);
protected:
  qSlicerSubjectHierarchySegmentsPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchySegmentsPluginPrivate(qSlicerSubjectHierarchySegmentsPlugin& object);
  ~qSlicerSubjectHierarchySegmentsPluginPrivate();
  void init();
public:
  QIcon SegmentIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchySegmentsPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPluginPrivate::qSlicerSubjectHierarchySegmentsPluginPrivate(qSlicerSubjectHierarchySegmentsPlugin& object)
: q_ptr(&object)
, SegmentIcon(QIcon(":Icons/Segment.png"))
{
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPluginPrivate::init()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPluginPrivate::~qSlicerSubjectHierarchySegmentsPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPlugin::qSlicerSubjectHierarchySegmentsPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchySegmentsPluginPrivate(*this) )
{
  this->m_Name = QString("Segments");

  Q_D(qSlicerSubjectHierarchySegmentsPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPlugin::~qSlicerSubjectHierarchySegmentsPlugin()
{
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchySegmentsPlugin::canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent)const
{
  if (!parent || !node->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName()))
  {
    // Cannot reparent if node is not a segment or there is no parent
    return 0.0;
  }

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(parent->GetAssociatedNode());
  if (segmentationNode)
  {
    // If node is segment and parent is segmentation then can reparent
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchySegmentsPlugin::reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parentNode)
{
  if (!parentNode || !nodeToReparent->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName()) || !nodeToReparent->GetParentNode())
  {
    // Cannot reparent if node is not a segment or there is no parent
    return false;
  }

  // Get source and target segmentation nodes
  vtkMRMLSegmentationNode* fromSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(nodeToReparent->GetParentNode()->GetAssociatedNode());
  vtkMRMLSegmentationNode* toSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(parentNode->GetAssociatedNode());
  if (!fromSegmentationNode || !toSegmentationNode)
  {
    return false;
  }

  // Get segment ID
  const char* segmentId = nodeToReparent->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

  // Perform reparenting
  // Note: No actual subject hierarchy reparenting is done, because the function call below triggers
  //   removal and addition of segment subject hierarchy nodes as the segment is removed from source
  //   segmentation and added to target segmentation
  bool success = toSegmentationNode->GetSegmentation()->CopySegmentFromSegmentation(
    fromSegmentationNode->GetSegmentation(), segmentId, true );

  // Notify user if failed to reparent
  if (!success)
  {
    QString message = QString("Cannot convert source master representation '%1' into target master '%2', thus unable to move segment '%3' from segmentation '%4' to '%5'.\n\nWould you like to change the master representation of '%5' to '%1'?\n\nNote: This may result in unwanted data loss in %5.")
      .arg(fromSegmentationNode->GetSegmentation()->GetMasterRepresentationName()).arg(toSegmentationNode->GetSegmentation()->GetMasterRepresentationName()).arg(segmentId).arg(fromSegmentationNode->GetName()).arg(toSegmentationNode->GetName());
    QMessageBox::StandardButton answer =
      QMessageBox::question(NULL, tr("Failed to move segment between segmentations"), message,
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::Yes)
    {
      // Convert target segmentation to master representation of source segmentation
      bool successfulConversion = toSegmentationNode->GetSegmentation()->CreateRepresentation(
        fromSegmentationNode->GetSegmentation()->GetMasterRepresentationName() );
      if (!successfulConversion)
      {
        QString message = QString("Failed to convert %1 to %2!").arg(toSegmentationNode->GetName()).arg(fromSegmentationNode->GetSegmentation()->GetMasterRepresentationName());
        QMessageBox::warning(NULL, tr("Conversion failed"), message);
        return false;
      }

      // Change master representation of target to that of source
      toSegmentationNode->GetSegmentation()->SetMasterRepresentationName(
        fromSegmentationNode->GetSegmentation()->GetMasterRepresentationName() );

      // Retry reparenting
      return this->reparentNodeInsideSubjectHierarchy(nodeToReparent, parentNode);
    }
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchySegmentsPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
    {
    qCritical() << "qSlicerSubjectHierarchyVolumesPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
    }

  // Segment
  if (node->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName()))
  {
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchySegmentsPlugin::roleForPlugin()const
{
  return "Segment";
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchySegmentsPlugin::helpText()const
{
  //return QString("<p style=\" margin-top:4px; margin-bottom:1px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; font-weight:600; color:#000000;\">Create new Contour set from scratch</span></p>"
  //  "<p style=\" margin-top:0px; margin-bottom:11px; margin-left:26px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; color:#000000;\">Right-click on an existing Study node and select 'Create child contour set'. This menu item is only available for Study level nodes</span></p>");
  //TODO:
  return QString();
}

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchySegmentsPlugin::tooltip(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
    {
    qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::tooltip: Subject hierarchy node is NULL!";
    return QString("Invalid!");
    }

  // Get basic tooltip from abstract plugin
  QString tooltipString = Superclass::tooltip(node);

  vtkSegment* segment = vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyNode(node);
  if (!segment)
  {
    qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::tooltip: Unable to get segment for segment subject hierarchy node!";
    return tooltipString;
  }

  // Representations
  std::vector<std::string> containedRepresentationNames;
  segment->GetContainedRepresentationNames(containedRepresentationNames);
  tooltipString.append( QString(" (Representations: ") );
  if (containedRepresentationNames.empty())
  {
    tooltipString.append( QString("None!)") );
  }
  else
  {
    for (std::vector<std::string>::iterator reprIt = containedRepresentationNames.begin();
      reprIt != containedRepresentationNames.end(); ++reprIt)
    {
      tooltipString.append( reprIt->c_str() );
      tooltipString.append( ", " );
    }
    tooltipString = tooltipString.left(tooltipString.length()-2).append(")");
  }

  // Default color
  double defaultColor[3] = {0.0,0.0,0.0};
  segment->GetDefaultColor(defaultColor);
  tooltipString.append( QString(" (Default color: %1,%2,%3)").arg(
    (int)(defaultColor[0]*255)).arg((int)(defaultColor[1]*255)).arg((int)(defaultColor[2]*255)) );

  // Tags
  std::vector<std::string> tags;
  segment->GetTags(tags);
  tooltipString.append( QString(" (Tags: ") );
  if (tags.empty())
  {
    tooltipString.append( QString("None)") );
  }
  else
  {
    for (std::vector<std::string>::iterator tagIt=tags.begin(); tagIt!=tags.end(); ++tagIt)
    {
      tooltipString.append( tagIt->c_str() );
      tooltipString.append( ", " );
    }
    tooltipString = tooltipString.left(tooltipString.length()-2).append(")");
  }

  return tooltipString;
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchySegmentsPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::icon: NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchySegmentsPlugin);

  // Contour set
  if (this->canOwnSubjectHierarchyNode(node))
  {
    return d->SegmentIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchySegmentsPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::setDisplayVisibility: NULL node!";
    return;
  }

  // Get segmentation node and display node
  vtkMRMLSegmentationNode* segmentationNode =
    vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode(node);
  if (!segmentationNode)
  {
    qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::getDisplayVisibility: Unable to find segmentation node for segment subject hierarchy node " << node->GetName();
    return;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    segmentationNode->GetDisplayNode() );
  if (!displayNode)
    {
    qCritical() << "qMRMLSegmentsTableView::onSegmentTableItemChanged: No display node for segmentation!";
    return;
    }

  // Get segment ID
  const char* segmentId = node->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

  // Set visibility
  displayNode->SetSegmentVisibility(segmentId, (bool)visible);

  // Trigger update of visibility icon
  node->Modified();
}

//-----------------------------------------------------------------------------
int qSlicerSubjectHierarchySegmentsPlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::getDisplayVisibility: NULL node!";
    return -1;
  }

  // Get segmentation node and display node
  vtkMRMLSegmentationNode* segmentationNode =
    vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode(node);
  if (!segmentationNode)
  {
    qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::getDisplayVisibility: Unable to find segmentation node for segment subject hierarchy node " << node->GetName();
    return -1;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    segmentationNode->GetDisplayNode() );
  if (!displayNode)
    {
    qCritical() << "qMRMLSegmentsTableView::onSegmentTableItemChanged: No display node for segmentation!";
    return -1;
    }

  // Get segment ID
  const char* segmentId = node->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

  // Get visibility
  return (displayNode->GetSegmentVisibility(segmentId) ? 1 : 0);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchySegmentsPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchySegmentsPlugin);

  QList<QAction*> actions;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  qSlicerSubjectHierarchySegmentationsPlugin* segmentationsPlugin = qobject_cast<qSlicerSubjectHierarchySegmentationsPlugin*>(
    qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Segmentations") );

  // Segments plugin shows all segmentations plugin functions in segment context menu
  segmentationsPlugin->showContextMenuActionsForNode(node);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  // Switch to segmentations module and select node
  qSlicerAbstractModuleWidget* moduleWidget = qSlicerSubjectHierarchyAbstractPlugin::switchToModule("Segmentations");
  if (moduleWidget)
  {
    // Get node selector combobox
    qMRMLNodeComboBox* nodeSelector = moduleWidget->findChild<qMRMLNodeComboBox*>("MRMLNodeComboBox_Segmentation");

    // Select segmentation node corresponding to segment node
    vtkMRMLSegmentationNode* segmentationNode =
      vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode(node);
    if (!segmentationNode)
    {
      qCritical() << "qSlicerSubjectHierarchySegmentsPlugin::editProperties: Unable to find segmentation node for segment subject hierarchy node " << node->GetName();
      return;
    }
    if (nodeSelector)
    {
      nodeSelector->setCurrentNode(segmentationNode);
    }
  }
}