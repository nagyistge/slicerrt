/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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

// SlicerQt includes
#include "qSlicerContoursModuleWidget.h"
#include "ui_qSlicerContoursModule.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// Contours includes
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkConvertContourRepresentations.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Contours
class qSlicerContoursModuleWidgetPrivate: public Ui_qSlicerContoursModule
{
  Q_DECLARE_PUBLIC(qSlicerContoursModuleWidget);
protected:
  qSlicerContoursModuleWidget* const q_ptr;
public:
  qSlicerContoursModuleWidgetPrivate(qSlicerContoursModuleWidget& object);
  ~qSlicerContoursModuleWidgetPrivate();
  vtkSlicerContoursModuleLogic* logic() const;
public:
  /// List of currently selected contour nodes. Contains the selected
  /// contour node or the children of the selected contour hierarchy node
  std::vector<vtkMRMLContourNode*> SelectedContourNodes;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerContoursModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidgetPrivate::qSlicerContoursModuleWidgetPrivate(qSlicerContoursModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
  this->SelectedContourNodes.clear();
}

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidgetPrivate::~qSlicerContoursModuleWidgetPrivate()
{
  this->SelectedContourNodes.clear();
}

//-----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic*
qSlicerContoursModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerContoursModuleWidget);
  return vtkSlicerContoursModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerContoursModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidget::qSlicerContoursModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerContoursModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidget::~qSlicerContoursModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerContoursModuleWidget);

  d->ModuleWindowInitialized = true;

  this->contourNodeChanged( d->MRMLNodeComboBox_Contour->currentNode() );
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::setup()
{
  Q_D(qSlicerContoursModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect( d->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(contourNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->comboBox_ChangeActiveRepresentation, SIGNAL(currentIndexChanged(int)), this, SLOT(activeRepresentationComboboxSelectionChanged(int)) );
  connect( d->pushButton_ApplyChangeRepresentation, SIGNAL(clicked()), this, SLOT(applyChangeRepresentationClicked()) );
  connect( d->horizontalSlider_OversamplingFactor, SIGNAL(valueChanged(int)), this, SLOT(oversamplingFactorChanged(int)) );
  connect( d->SliderWidget_TargetReductionFactorPercent, SIGNAL(valueChanged(double)), this, SLOT(targetReductionFactorPercentChanged(double)) );

  d->label_NoReferenceWarning->setVisible(false);
  d->label_NewConversionWarning->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_ActiveSelected->setVisible(false);
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType qSlicerContoursModuleWidget::getRepresentationTypeOfSelectedContours()
{
  Q_D(qSlicerContoursModuleWidget);

  bool sameRepresentationTypes = true;
  vtkMRMLContourNode::ContourRepresentationType representationType = vtkMRMLContourNode::None;

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (representationType == vtkMRMLContourNode::None)
    {
      representationType = (*it)->GetActiveRepresentationType();
    }
    else if ((*it)->GetActiveRepresentationType() == vtkMRMLContourNode::None) // Sanity check
    {
      vtkWarningWithObjectMacro((*it), "getRepresentationTypeOfSelectedContours: Invalid representation type (None) found for the contour node '" << (*it)->GetName() << "'!")
    }
    else if (representationType != (*it)->GetActiveRepresentationType())
    {
      sameRepresentationTypes = false;
    }
  }

  if (sameRepresentationTypes)
  {
    return representationType;
  }
  else
  {
    return vtkMRMLContourNode::None;
  }
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getReferenceVolumeNodeIdOfSelectedContours(QString &referenceVolumeNodeId)
{
  Q_D(qSlicerContoursModuleWidget);

  referenceVolumeNodeId.clear();
  bool sameReferenceVolumeNodeId = true;
  bool allCreatedFromLabelmap = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if ( !(*it)->HasBeenCreatedFromIndexedLabelmap() )
    {
      allCreatedFromLabelmap = false;
    }
    else
    {
      continue;
    }

    if (referenceVolumeNodeId.isEmpty())
    {
      referenceVolumeNodeId = QString( (*it)->GetRasterizationReferenceVolumeNodeId() );
    }
    else if (referenceVolumeNodeId.compare( (*it)->GetRasterizationReferenceVolumeNodeId() ))
    {
      sameReferenceVolumeNodeId = false;
      referenceVolumeNodeId.clear();
    }
  }

  return allCreatedFromLabelmap || sameReferenceVolumeNodeId;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getOversamplingFactorOfSelectedContours(double &oversamplingFactor)
{
  Q_D(qSlicerContoursModuleWidget);

  oversamplingFactor = 0.0;
  bool sameOversamplingFactor = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (oversamplingFactor == 0.0)
    {
      oversamplingFactor = (*it)->GetRasterizationOversamplingFactor();
    }
    else if (oversamplingFactor != (*it)->GetRasterizationOversamplingFactor())
    {
      sameOversamplingFactor = false;
      oversamplingFactor = -1.0;
    }
  }

  return sameOversamplingFactor;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getTargetReductionFactorOfSelectedContours(double &targetReductionFactor)
{
  Q_D(qSlicerContoursModuleWidget);

  targetReductionFactor = 0.0;
  bool sameTargetReductionFactor = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (targetReductionFactor == 0.0)
    {
      targetReductionFactor = (*it)->GetDecimationTargetReductionFactor();
    }
    else if (targetReductionFactor != (*it)->GetDecimationTargetReductionFactor())
    {
      sameTargetReductionFactor = false;
      targetReductionFactor = -1.0;
    }
  }

  return sameTargetReductionFactor;
}

//TODO: delete functions that are not needed from here...
//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::selectedContoursContainRepresentation(vtkMRMLContourNode::ContourRepresentationType representationType, bool allMustContain/*=true*/)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (allMustContain && !(*it)->RepresentationExists(representationType))
    {
      // At least one misses the requested representation
      return false;
    }
    else if (!allMustContain && (*it)->RepresentationExists(representationType))
    {
      // At least one has the requested representation
      return true;
    }
  }

  if (allMustContain)
  {
    // All contours have the requested representation
    return true;
  }
  else
  {
    // None of the contours have the requested representation
    return false;
  }
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isConversionNeeded(vtkMRMLContourNode* contourNode, vtkMRMLContourNode::ContourRepresentationType targetRepresentation)
{
  Q_D(qSlicerContoursModuleWidget);

  bool referenceVolumeNodeChanged = ( d->MRMLNodeComboBox_ReferenceVolume->currentNodeId().compare(
                                      QString(contourNode->GetRasterizationReferenceVolumeNodeId()) ) );
  bool oversamplingFactorChanged = ( fabs(this->getOversamplingFactor()
                                   - contourNode->GetRasterizationOversamplingFactor()) > EPSILON );
  bool targetReductionFactorChanged = ( fabs(d->SliderWidget_TargetReductionFactorPercent->value()
                                      - contourNode->GetDecimationTargetReductionFactor()) > EPSILON );

  if ( targetRepresentation == (int)vtkMRMLContourNode::RibbonModel )
  {
    // Not implemented yet
    return false;
  }
  else if ( targetRepresentation == (int)vtkMRMLContourNode::IndexedLabelmap )
  {
    return ( !contourNode->HasBeenCreatedFromIndexedLabelmap()
          && ( !contourNode->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap)
            || referenceVolumeNodeChanged || oversamplingFactorChanged ) );
  }
  else if ( targetRepresentation == (int)vtkMRMLContourNode::ClosedSurfaceModel )
  {
    return ( ( !contourNode->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap)
            || referenceVolumeNodeChanged || oversamplingFactorChanged )
          || ( !contourNode->RepresentationExists(vtkMRMLContourNode::ClosedSurfaceModel)
            || targetReductionFactorChanged ) );
  }

  return false;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isConversionNeededForSelectedNodes(vtkMRMLContourNode::ContourRepresentationType representationToConvertTo, bool checkOnlyExistingRepresentations/*=false*/)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (!checkOnlyExistingRepresentations || (*it)->RepresentationExists(representationToConvertTo))
    {
      if (this->isConversionNeeded((*it), (vtkMRMLContourNode::ContourRepresentationType)d->comboBox_ChangeActiveRepresentation->currentIndex()))
      {
        return true;
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isConversionToLabelmapPossibleForSelectedNodes()
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (!(*it)->IsLabelmapConversionPossible())
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isLabelmapAvailableForConversionToClosedSurfaceModelForSelectedNodes()
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    // If the contour does not have indexed labelmap representation and cannot be converted to it either then return false
    if (!(*it)->GetIndexedLabelmapVolumeNodeId() && !(*it)->IsLabelmapConversionPossible())
    {
      return false;
    }
  }

  return true;
}
//TODO: ... to here

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  d->SelectedContourNodes.clear();

  if (!this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    d->comboBox_ChangeActiveRepresentation->setEnabled(false);
    d->label_ActiveRepresentation->setText(tr("[No node is selected]"));
    d->label_ActiveRepresentation->setToolTip(tr(""));
    return;
  }

  // Create list of selected contour nodes
  if (node->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
    if (contourNode)
    {
      d->SelectedContourNodes.push_back(contourNode);
    }
  }
  else if ( node->IsA("vtkMRMLContourHierarchyNode")
    && SlicerRtCommon::IsPatientHierarchyNode(node) )
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLContourHierarchyNode::SafeDownCast(node)->GetChildrenContourNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      vtkWarningWithObjectMacro(node, "contourNodeChanged: Selected contour hierarchy node has no children contour nodes!");
      return;
    }

    // Collect contour nodes in the hierarchy and determine whether their active representation types are the same
    for (int i=0; i<childContourNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(childContourNodes->GetItemAsObject(i));
      if (contourNode)
      {
        d->SelectedContourNodes.push_back(contourNode);
      }
    }
  }
  else
  {
    vtkErrorWithObjectMacro(node, "contourNodeChanged: Invalid node type for ContourNode!");
    return;
  }

  // Update UI from selected contours nodes list
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_ActiveSelected->setVisible(false);
  d->label_NewConversionWarning->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  vtkMRMLNode* selectedNode = d->MRMLNodeComboBox_Contour->currentNode();
  if (!this->mrmlScene() || !selectedNode || !d->ModuleWindowInitialized)
  {
    d->comboBox_ChangeActiveRepresentation->setEnabled(false);
    d->label_ActiveRepresentation->setText(tr("[No node is selected]"));
    d->label_ActiveRepresentation->setToolTip(tr(""));
    return;
  }

  d->comboBox_ChangeActiveRepresentation->setEnabled(true);

  // Select the representation type shared by all the children contour nodes
  vtkMRMLContourNode::ContourRepresentationType representationType = this->getRepresentationTypeOfSelectedContours();
  if (representationType != vtkMRMLContourNode::None)
  {
    d->label_ActiveRepresentation->setText(d->comboBox_ChangeActiveRepresentation->itemText((int)representationType));
    d->label_ActiveRepresentation->setToolTip(tr(""));
  }
  else
  {
    d->label_ActiveRepresentation->setText(tr("Various"));
    d->label_ActiveRepresentation->setToolTip(tr("The selected hierarchy node contains contours with different active representation types"));
  }

  // Get reference volume node ID for the selected contour nodes
  QString referenceVolumeNodeId;
  bool sameReferenceVolumeNodeId = this->getReferenceVolumeNodeIdOfSelectedContours(referenceVolumeNodeId);

  // Set reference volume on the GUI
  if (!referenceVolumeNodeId.isEmpty())
  {
    if (sameReferenceVolumeNodeId)
    {
      d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
    }
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(referenceVolumeNodeId);
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);
  }
  // If the selected contours do not have a reference volume (have been created from labelmap), then leave it as empty
  else if (sameReferenceVolumeNodeId)
  {
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(NULL);
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);
  }

  // Get the oversampling factor of the selected contour nodes
  double oversamplingFactor = 0.0;
  bool sameOversamplingFactor = this->getOversamplingFactorOfSelectedContours(oversamplingFactor);

  // Set the oversampling factor on the GUI
  if (oversamplingFactor != -1.0)
  {
    if (sameOversamplingFactor)
    {
      d->horizontalSlider_OversamplingFactor->blockSignals(true);
    }
    d->horizontalSlider_OversamplingFactor->setValue( (int)(log(oversamplingFactor)/log(2.0)) );
    d->horizontalSlider_OversamplingFactor->blockSignals(false);
  }
  d->lineEdit_OversamplingFactor->setText( QString::number(this->getOversamplingFactor()) );

  // Get target reduction factor for the selected contour nodes
  double targetReductionFactor;
  bool sameTargetReductionFactor = this->getTargetReductionFactorOfSelectedContours(targetReductionFactor);

  // Set the oversampling factor on the GUI ([1] applies here as well)
  if (targetReductionFactor != -1.0)
  {
    if (sameTargetReductionFactor)
    {
      d->SliderWidget_TargetReductionFactorPercent->blockSignals(true);
    }
    d->SliderWidget_TargetReductionFactorPercent->setValue(targetReductionFactor);
    d->SliderWidget_TargetReductionFactorPercent->blockSignals(false);
  }

  // Update apply button state, warning labels, etc.
  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::activeRepresentationComboboxSelectionChanged(int index)
{
  UNUSED_VARIABLE(index);

  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::showConversionParameterControlsForTargetRepresentation(vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  Q_D(qSlicerContoursModuleWidget);

  //TODO: start test
  vtkMRMLContourNode::ContourRepresentationType target = (vtkMRMLContourNode::ContourRepresentationType)d->comboBox_ChangeActiveRepresentation->currentIndex();
  if (target != targetRepresentationType)
  {
    int i=0; ++i;
  }
  //TODO: end test

  d->MRMLNodeComboBox_ReferenceVolume->setVisible(true);
  d->label_ReferenceVolume->setVisible(true);
  d->horizontalSlider_OversamplingFactor->setVisible(true);
  d->lineEdit_OversamplingFactor->setVisible(true);
  d->label_OversamplingFactor->setVisible(true);
  d->label_TargetReductionFactor->setVisible(true);
  d->SliderWidget_TargetReductionFactorPercent->setVisible(true);

  if ((targetRepresentationType != (int)vtkMRMLContourNode::IndexedLabelmap
    && targetRepresentationType != (int)vtkMRMLContourNode::ClosedSurfaceModel)
    || !this->mrmlScene() )
  {
    d->MRMLNodeComboBox_ReferenceVolume->setVisible(false);
    d->label_ReferenceVolume->setVisible(false);
    d->horizontalSlider_OversamplingFactor->setVisible(false);
    d->lineEdit_OversamplingFactor->setVisible(false);
    d->label_OversamplingFactor->setVisible(false);
  }
  if ( targetRepresentationType != (int)vtkMRMLContourNode::ClosedSurfaceModel
    || !this->mrmlScene() )
  {
    d->label_TargetReductionFactor->setVisible(false);
    d->SliderWidget_TargetReductionFactorPercent->setVisible(false);
  }
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChanged()
{
  return this->haveConversionParametersChangedForIndexedLabelmap()
      || this->haveConversionParametersChangedForClosedSurfaceModel();
}
// TODO: delete
static QString safeReferenceVolumeNodeId;
static double safeOversamplingFactor;
//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChangedForIndexedLabelmap()
{
  Q_D(qSlicerContoursModuleWidget);

  //TODO: Currently selected items may change between the start of the updateWidgetsFromChangeActiveRepresentationGroup call and the actual call of this function!
  if ( d->MRMLNodeComboBox_ReferenceVolume->currentNodeId().compare(safeReferenceVolumeNodeId)
    || fabs(this->getOversamplingFactor() - safeOversamplingFactor) > EPSILON )
  {
    int i=0; ++i;
  }
  safeReferenceVolumeNodeId = d->MRMLNodeComboBox_ReferenceVolume->currentNodeId();
  safeOversamplingFactor = this->getOversamplingFactor();
  //TODO: end test

  // Get reference volume node ID for the selected contour nodes
  QString referenceVolumeNodeId;
  bool sameReferenceVolumeNodeId = this->getReferenceVolumeNodeIdOfSelectedContours(referenceVolumeNodeId);
  bool referenceVolumeNodeChanged = ( ! ( sameReferenceVolumeNodeId && 
    (referenceVolumeNodeId.isEmpty() || !d->MRMLNodeComboBox_ReferenceVolume->currentNodeId().compare(referenceVolumeNodeId)) ) );

  // Get the oversampling factor of the selected contour nodes
  double oversamplingFactor = 0.0;
  bool sameOversamplingFactor = this->getOversamplingFactorOfSelectedContours(oversamplingFactor);
  bool oversamplingFactorChanged = ( !sameOversamplingFactor
    || fabs(this->getOversamplingFactor() - oversamplingFactor) > EPSILON );

  return referenceVolumeNodeChanged || oversamplingFactorChanged;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChangedForClosedSurfaceModel()
{
  Q_D(qSlicerContoursModuleWidget);

  // Get target reduction factor for the selected contour nodes
  double targetReductionFactor;
  bool sameTargetReductionFactor = this->getTargetReductionFactorOfSelectedContours(targetReductionFactor);
  bool targetReductionFactorChanged = ( !sameTargetReductionFactor
    || fabs(d->SliderWidget_TargetReductionFactorPercent->value() - targetReductionFactor) > EPSILON );

  return targetReductionFactorChanged;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isIntermediateLabelmapConversionNecessary(vtkMRMLContourNode* contourNode, vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  // TODO: Change here if conversion is available to ribbon model
  if (targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel)
  {
    if ( !contourNode->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap)
      || this->haveConversionParametersChangedForIndexedLabelmap() )
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isSuitableSourceAvailableForConversionForContour(vtkMRMLContourNode* contourNode, vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  if ( targetRepresentationType == vtkMRMLContourNode::RibbonModel
    && !contourNode->RepresentationExists(vtkMRMLContourNode::RibbonModel) )
  {
    return false;
  }
  else if ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
    && !contourNode->RepresentationExists(vtkMRMLContourNode::RibbonModel)
    && !contourNode->RepresentationExists(vtkMRMLContourNode::ClosedSurfaceModel) )
  {
    return false;
  }
  else if ( targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel
    && this->isIntermediateLabelmapConversionNecessary(contourNode, targetRepresentationType)
    && !contourNode->RepresentationExists(vtkMRMLContourNode::RibbonModel) )
  {
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isSuitableSourceAvailableForConversionForAllSelectedContours(vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (!this->isSuitableSourceAvailableForConversionForContour(*it, targetRepresentationType))
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isNewConversionNecessaryForContour(vtkMRMLContourNode* contourNode, vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  // Determine if the conversion parameters of the target representation has changed
  bool targetConversionParametersChanged = ( ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
                                            && this->haveConversionParametersChangedForIndexedLabelmap() )
                                          || ( targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel
                                            && this->haveConversionParametersChangedForClosedSurfaceModel() ) );

  return (targetConversionParametersChanged || this->isIntermediateLabelmapConversionNecessary(contourNode, targetRepresentationType));
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isNewConversionNecessaryForAnySelectedContour(vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (this->isNewConversionNecessaryForContour(*it, targetRepresentationType))
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isReferenceVolumeSelectionValidForContour(vtkMRMLContourNode* contourNode, vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  Q_D(qSlicerContoursModuleWidget);

  if ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
    || this->isIntermediateLabelmapConversionNecessary(contourNode, targetRepresentationType) )
  {
    if (d->MRMLNodeComboBox_ReferenceVolume->currentNode())
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isReferenceVolumeSelectionValidForAllSelectedContours(vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (!this->isReferenceVolumeSelectionValidForContour(*it, targetRepresentationType))
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::updateWidgetsFromChangeActiveRepresentationGroup()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_ActiveSelected->setVisible(false);
  d->label_NewConversionWarning->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);

  // Get representation type for the selected contour nodes and the target type
  vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = this->getRepresentationTypeOfSelectedContours();
  vtkMRMLContourNode::ContourRepresentationType targetRepresentationType
    = (vtkMRMLContourNode::ContourRepresentationType)d->comboBox_ChangeActiveRepresentation->currentIndex();

  // If target representation type matches all active representations
  if (targetRepresentationType == representationTypeInSelectedNodes)
  {
    d->label_ActiveSelected->setVisible(true);
    if (this->haveConversionParametersChanged() && this->isSuitableSourceAvailableForConversionForAllSelectedContours(targetRepresentationType))
    {
      this->showConversionParameterControlsForTargetRepresentation(targetRepresentationType);
      d->pushButton_ApplyChangeRepresentation->setEnabled(true);
    }
    else
    {
      d->pushButton_ApplyChangeRepresentation->setEnabled(false);
    }

    return;
  }

  // If any selected contour lacks a suitable source representation for the actual conversion, then show warning and hide all conversion parameters
  if (!this->isSuitableSourceAvailableForConversionForAllSelectedContours(targetRepresentationType))
  {
    d->label_NoSourceWarning->setVisible(true);
    this->showConversionParameterControlsForTargetRepresentation(vtkMRMLContourNode::None);
    d->pushButton_ApplyChangeRepresentation->setEnabled(false);

    return;
  }

  // Show conversion parameters for selected target representation
  this->showConversionParameterControlsForTargetRepresentation(targetRepresentationType);

  // If there is no reference volume selected but should be, then show warning and disable the apply button
  if (!this->isReferenceVolumeSelectionValidForAllSelectedContours(targetRepresentationType))
  {
    d->label_NoReferenceWarning->setVisible(true);
    d->pushButton_ApplyChangeRepresentation->setEnabled(false);

    return;
  }

  // If every condition is fine, then enable Apply button
  d->pushButton_ApplyChangeRepresentation->setEnabled(true);

  // Show new conversion message if needed
  if (this->isNewConversionNecessaryForAnySelectedContour(targetRepresentationType))
  {
    d->label_NewConversionWarning->setVisible(true);
  }
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  if (!this->mrmlScene() || !d->ModuleWindowInitialized)
  {
    return;
  }
  if (!node)
  {
    d->label_NoReferenceWarning->setVisible(true);
    return;
  }

  d->label_NoReferenceWarning->setVisible(false);

  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::oversamplingFactorChanged(int value)
{
  Q_D(qSlicerContoursModuleWidget);
  UNUSED_VARIABLE(value);

  d->lineEdit_OversamplingFactor->setText( QString::number(this->getOversamplingFactor()) );

  this->updateWidgetsFromChangeActiveRepresentationGroup();
}
//-----------------------------------------------------------------------------

double qSlicerContoursModuleWidget::getOversamplingFactor()
{
  Q_D(qSlicerContoursModuleWidget);

  return pow(2.0, d->horizontalSlider_OversamplingFactor->value());
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::targetReductionFactorPercentChanged(double value)
{
  UNUSED_VARIABLE(value);

  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::applyChangeRepresentationClicked()
{
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // TODO: Workaround for update issues
  this->mrmlScene()->StartState(vtkMRMLScene::BatchProcessState);

  int convertToRepresentationType = d->comboBox_ChangeActiveRepresentation->currentIndex();

  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
  double oversamplingFactor = this->getOversamplingFactor();
  double targetReductionFactor = d->SliderWidget_TargetReductionFactorPercent->value();

  // Apply change representation on all selected contours
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    bool conversionNeeded = this->isConversionNeeded((*it), (vtkMRMLContourNode::ContourRepresentationType)convertToRepresentationType);
    bool labelmapConversionNeeded = this->isConversionNeeded((*it), vtkMRMLContourNode::IndexedLabelmap);

    // Set conversion parameters
    if (conversionNeeded)
    {
      if (referenceVolumeNode)
      {
        (*it)->SetAndObserveRasterizationReferenceVolumeNodeId(referenceVolumeNode->GetID());
      }
      (*it)->SetRasterizationOversamplingFactor(oversamplingFactor);
      (*it)->SetDecimationTargetReductionFactor(targetReductionFactor / 100.0);
    }

    // Do conversion if necessary
    if (convertToRepresentationType == (int)vtkMRMLContourNode::RibbonModel)
    {
      (*it)->SetActiveRepresentationByType(vtkMRMLContourNode::RibbonModel);
    }
    else if ( convertToRepresentationType == (int)vtkMRMLContourNode::IndexedLabelmap
           && (*it)->IsLabelmapConversionPossible() )
    {
      if (conversionNeeded)
      {
        // Delete original representation and re-convert
        vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
        converter->SetContourNode(*it);
        converter->ReconvertRepresentation(vtkMRMLContourNode::IndexedLabelmap);
      }

      vtkMRMLScalarVolumeNode* indexedLabelmapNode = (*it)->GetIndexedLabelmapVolumeNode();
      if (!indexedLabelmapNode)
      {
        vtkErrorWithObjectMacro((*it), "applyChangeRepresentationClicked: Failed to get '"
          << d->comboBox_ChangeActiveRepresentation->currentText().toLatin1().constData()
          << "' representation for contour node '" << (*it)->GetName() << "' !");
      }
      else
      {
        (*it)->SetActiveRepresentationByNode(indexedLabelmapNode);
      }
    }
    else if (convertToRepresentationType == (int)vtkMRMLContourNode::ClosedSurfaceModel)
    {
      if (conversionNeeded)
      {
        // Re-convert labelmap if necessary
        if (labelmapConversionNeeded)
        {
          if (!(*it)->IsLabelmapConversionPossible())
          {
            vtkErrorWithObjectMacro((*it), "applyChangeRepresentationClicked: Unable to convert to '"
              << d->comboBox_ChangeActiveRepresentation->currentText().toLatin1().constData()
              << "' representation for contour node '" << (*it)->GetName() << "' !");
            continue;
          }

          vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
          converter->SetContourNode(*it);
          converter->ReconvertRepresentation(vtkMRMLContourNode::IndexedLabelmap);
        }

        // Delete original representation and re-convert
        vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
        converter->SetContourNode(*it);
        converter->ReconvertRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);
      }

      vtkMRMLModelNode* closedSurfaceModelNode = (*it)->GetClosedSurfaceModelNode();
      if (!closedSurfaceModelNode)
      {
        vtkErrorWithObjectMacro((*it), "applyChangeRepresentationClicked: Failed to get '"
          << d->comboBox_ChangeActiveRepresentation->currentText().toLatin1().constData()
          << "' representation for contour node '" << (*it)->GetName() << "' !");
      }
      else
      {
        (*it)->SetActiveRepresentationByNode((vtkMRMLDisplayableNode*)closedSurfaceModelNode);
      }
    }
  }

  // Select the representation type shared by all the children contour nodes
  vtkMRMLContourNode::ContourRepresentationType representationType = this->getRepresentationTypeOfSelectedContours();
  if (representationType != vtkMRMLContourNode::None)
  {
    d->label_ActiveRepresentation->setText(d->comboBox_ChangeActiveRepresentation->itemText((int)representationType));
    d->label_ActiveRepresentation->setToolTip(tr(""));
  }
  else
  {
    d->label_ActiveRepresentation->setText(tr("Various"));
    d->label_ActiveRepresentation->setToolTip(tr("The selected hierarchy node contains contours with different active representation types"));
  }

  this->updateWidgetsFromChangeActiveRepresentationGroup();

  this->mrmlScene()->EndState(vtkMRMLScene::BatchProcessState);

  QApplication::restoreOverrideCursor();
}
