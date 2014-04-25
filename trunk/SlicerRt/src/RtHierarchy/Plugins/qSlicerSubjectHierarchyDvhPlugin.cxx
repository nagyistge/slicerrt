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

// SlicerRt includes
#include "SlicerRtCommon.h"

// RTHierarchy Plugins includes
#include "qSlicerSubjectHierarchyDvhPlugin.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"
#include "qSlicerSubjectHierarchyChartsPlugin.h"

// DoseVolumeHistogram includes
#include "vtkMRMLDoseVolumeHistogramNode.h"
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLChartNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>
#include <QIcon>
#include <QStandardItem>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractModuleWidget.h"

// MRML widgets includes
#include "qMRMLNodeComboBox.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerSubjectHierarchyDvhPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyDvhPlugin);
protected:
  qSlicerSubjectHierarchyDvhPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyDvhPluginPrivate(qSlicerSubjectHierarchyDvhPlugin& object);
  ~qSlicerSubjectHierarchyDvhPluginPrivate();
public:
  QIcon DvhIcon;

  QIcon VisibleIcon;
  QIcon HiddenIcon;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyDvhPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPluginPrivate::qSlicerSubjectHierarchyDvhPluginPrivate(qSlicerSubjectHierarchyDvhPlugin& object)
 : q_ptr(&object)
{
  this->DvhIcon = QIcon(":Icons/DVH.png");

  this->VisibleIcon = QIcon(":Icons/VisibleOn.png");
  this->HiddenIcon = QIcon(":Icons/VisibleOff.png");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPluginPrivate::~qSlicerSubjectHierarchyDvhPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPlugin::qSlicerSubjectHierarchyDvhPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyDvhPluginPrivate(*this) )
{
  this->m_Name = QString("DVH");
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDvhPlugin::~qSlicerSubjectHierarchyDvhPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyDvhPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // DVH
  if ( associatedNode && associatedNode->IsA("vtkMRMLDoubleArrayNode")
    && associatedNode->GetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    return 1.0; // Only this plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyDvhPlugin::roleForPlugin()const
{
  return "Dose volume histogram";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyDvhPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyDvhPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    item->setIcon(d->DvhIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDvhPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::setVisibilityIcon: NULL node or item given!";
    return;
  }

  Q_D(qSlicerSubjectHierarchyDvhPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    if (this->getDisplayVisibility(node))
    {
      item->setIcon(d->VisibleIcon);
    }
    else
    {
      item->setIcon(d->HiddenIcon);
    }
  }
  else
  {
    // For all other owned nodes the visibility icon is set as default
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDvhPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::setDisplayVisibility: NULL node!";
    return;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    // Get chart for DVH array node
    vtkMRMLSubjectHierarchyNode* chartSubjectHierarchyNode = this->getChartForDvhArray(node);
    if (!chartSubjectHierarchyNode)
    {
      qCritical() << "qSlicerSubjectHierarchyDvhPlugin::getDisplayVisibility: Unable to get chart node for DVH array node!";
      return;
    }

    // Get chart visibility
    int chartVisible = qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Charts")->getDisplayVisibility(chartSubjectHierarchyNode);

    // Get DVH logic for DVH visibility setting
    qSlicerAbstractCoreModule* dvhModule = qSlicerApplication::application()->moduleManager()->module("DoseVolumeHistogram");
    vtkSlicerDoseVolumeHistogramModuleLogic* dvhLogic = vtkSlicerDoseVolumeHistogramModuleLogic::SafeDownCast(dvhModule->logic());

    // Show/hide the DVH plot
    vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node->GetAssociatedDataNode());
    if (visible)
    {
      if (!chartVisible)
      {
        // Show the chart itself
        qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Charts")->setDisplayVisibility(chartSubjectHierarchyNode, visible);
      }

      dvhLogic->AddDvhToSelectedChart(dvhArrayNode->GetID());
    }
    else if (chartVisible)
    {
      dvhLogic->RemoveDvhFromSelectedChart(dvhArrayNode->GetID());
    }

    // Trigger icon update
    node->Modified();
  }
  // Default
  else
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDisplayVisibility(node, visible);
  }
}

//---------------------------------------------------------------------------
int qSlicerSubjectHierarchyDvhPlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::getDisplayVisibility: NULL node!";
    return -1;
  }

  if (this->canOwnSubjectHierarchyNode(node))
  {
    // Get chart for DVH array node
    vtkMRMLSubjectHierarchyNode* chartSubjectHierarchyNode = this->getChartForDvhArray(node);
    if (!chartSubjectHierarchyNode)
    {
      qCritical() << "qSlicerSubjectHierarchyDvhPlugin::getDisplayVisibility: Unable to get chart node for DVH array node!";
      return -1;
    }

    // Get chart visibility
    int chartVisible = qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Charts")->getDisplayVisibility(chartSubjectHierarchyNode);

    // Get DVH logic for DVH visibility
    qSlicerAbstractCoreModule* dvhModule = qSlicerApplication::application()->moduleManager()->module("DoseVolumeHistogram");
    vtkSlicerDoseVolumeHistogramModuleLogic* dvhLogic = vtkSlicerDoseVolumeHistogramModuleLogic::SafeDownCast(dvhModule->logic());

    // Only return true if the chart is visible and the DVH array is added in the chart
    vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node->GetAssociatedDataNode());
    return (chartVisible && dvhLogic->IsDvhAddedToSelectedChart(dvhArrayNode->GetID()));
  }

  // Default
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->getDisplayVisibility(node);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDvhPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  qSlicerAbstractModuleWidget* moduleWidget = qSlicerSubjectHierarchyAbstractPlugin::switchToModule("DoseVolumeHistogram");
  if (moduleWidget)
  {
    // Get node selector combobox
    qMRMLNodeComboBox* nodeSelector = moduleWidget->findChild<qMRMLNodeComboBox*>("MRMLNodeComboBox_ParameterSet");

    // Get DVH parameter set node containing the current DVH array
    vtkMRMLDoseVolumeHistogramNode* parameterSetNode = this->getDvhParameterSetNodeForDvhArray(node->GetAssociatedDataNode());

    // Choose current data node
    if (nodeSelector)
    {
      nodeSelector->setCurrentNode(parameterSetNode);
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode* qSlicerSubjectHierarchyDvhPlugin::getDvhParameterSetNodeForDvhArray(vtkMRMLNode* dvhArrayNode)const
{
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::getDvhParameterSetNodeForDvhArray: Invalid MRML scene!";
    return false;
  }

  vtkSmartPointer<vtkCollection> dvhNodes = vtkSmartPointer<vtkCollection>::Take( scene->GetNodesByClass("vtkMRMLDoseVolumeHistogramNode") );
  vtkObject* nextObject = NULL;
  for (dvhNodes->InitTraversal(); (nextObject = dvhNodes->GetNextItemAsObject()); )
  {
    vtkMRMLDoseVolumeHistogramNode* dvhNode = vtkMRMLDoseVolumeHistogramNode::SafeDownCast(nextObject);
    if (dvhNode)
    {
      std::vector<vtkMRMLNode*> dvhArrayNodes;
      dvhNode->GetDvhDoubleArrayNodes(dvhArrayNodes);
      std::vector<vtkMRMLNode*>::iterator dvhArrayIt = std::find(dvhArrayNodes.begin(), dvhArrayNodes.end(), dvhArrayNode);
      if (dvhArrayIt != dvhArrayNodes.end())
      {
        return dvhNode;
      }
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* qSlicerSubjectHierarchyDvhPlugin::getChartForDvhArray(vtkMRMLSubjectHierarchyNode* dvhArraySubjectHierarchyNode)const
{
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyDvhPlugin::getChartForDvhArray: Invalid MRML scene!";
    return NULL;
  }

  // Get parameter set node for DVH, then chart from the parameter set node
  vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(
    dvhArraySubjectHierarchyNode->GetAssociatedDataNode() );
  vtkMRMLDoseVolumeHistogramNode* parameterSetNode = this->getDvhParameterSetNodeForDvhArray(dvhArrayNode);
  vtkMRMLChartNode* chartNode = parameterSetNode->GetChartNode();
  if (!chartNode)
  {
    // Create chart node if not specified
    chartNode = vtkMRMLChartNode::New();
    scene->AddNode(chartNode);
    chartNode->SetName(scene->GenerateUniqueName("Chart_DVH").c_str());
    chartNode->Delete(); // Return ownership to the scene only
    parameterSetNode->SetAndObserveChartNode(chartNode);
  }

  // Add chart to subject hierarchy if not added already
  vtkMRMLSubjectHierarchyNode* chartSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(chartNode);
  if (!chartSubjectHierarchyNode)
  {
    vtkMRMLSubjectHierarchyNode* seriesNode =
      dvhArraySubjectHierarchyNode->vtkMRMLSubjectHierarchyNode::GetAncestorAtLevel(
      vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES );
    chartSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      scene, seriesNode, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES,
      chartNode->GetName(), chartNode);
  }

  return chartSubjectHierarchyNode;
}
