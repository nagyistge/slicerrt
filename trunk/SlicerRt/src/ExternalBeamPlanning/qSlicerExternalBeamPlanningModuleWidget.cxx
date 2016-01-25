/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// SlicerQt includes
#include "qSlicerExternalBeamPlanningModuleWidget.h"
#include "ui_qSlicerExternalBeamPlanningModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// QSlicer includes
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h> 
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <qSlicerAbstractCoreModule.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerCLIModuleLogic.h>

// SlicerRt includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLExternalBeamPlanningNode.h"
#include "vtkMRMLRTProtonBeamNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLLayoutNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerExternalBeamPlanningModuleWidgetPrivate: public Ui_qSlicerExternalBeamPlanningModule
{
  Q_DECLARE_PUBLIC(qSlicerExternalBeamPlanningModuleWidget);
protected:
  qSlicerExternalBeamPlanningModuleWidget* const q_ptr;
public:
  qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object);
  ~qSlicerExternalBeamPlanningModuleWidgetPrivate();
  vtkSlicerExternalBeamPlanningModuleLogic* logic() const;
public:
  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  int currentBeamRow;
  int totalBeamRows;
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::qSlicerExternalBeamPlanningModuleWidgetPrivate(qSlicerExternalBeamPlanningModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
  currentBeamRow = -1;
  totalBeamRows = 0;
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidgetPrivate::~qSlicerExternalBeamPlanningModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic*
qSlicerExternalBeamPlanningModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerExternalBeamPlanningModuleWidget);
  return vtkSlicerExternalBeamPlanningModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidget::qSlicerExternalBeamPlanningModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerExternalBeamPlanningModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModuleWidget::~qSlicerExternalBeamPlanningModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or apply it if there is one in the scene
  if (scene && this->getExternalBeamPlanningNode())
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
    if (node)
    {
      this->externalBeamPlanningNodeChanged (vtkMRMLExternalBeamPlanningNode::SafeDownCast(node));
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}
//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onEnter()
{
  if (!this->mrmlScene()) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLExternalBeamPlanningNode* paramNode = this->getExternalBeamPlanningNode();

  // Do we already have a parameter node?
  if (paramNode == NULL) {

    // If not, try to find one in the scene
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
    if (node) {
      paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);
    }

    // Otherwise, create a new one
    else {
      vtkSmartPointer<vtkMRMLExternalBeamPlanningNode> newNode = vtkSmartPointer<vtkMRMLExternalBeamPlanningNode>::New();
      paramNode = newNode;
      this->mrmlScene()->AddNode(newNode);
    }
  }

  // Alert everyone that the parameter node has changed
  this->externalBeamPlanningNodeChanged(paramNode);

  // This is not used?
  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
vtkMRMLExternalBeamPlanningNode* qSlicerExternalBeamPlanningModuleWidget::getExternalBeamPlanningNode ()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  
  return d->logic()->GetExternalBeamPlanningNode();
}

//-----------------------------------------------------------------------------
vtkMRMLRTPlanNode* qSlicerExternalBeamPlanningModuleWidget::getRTPlanNode ()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  
  vtkMRMLExternalBeamPlanningNode* paramNode = this->getExternalBeamPlanningNode ();
  if (!paramNode) {
    return NULL;
  }
  return paramNode->GetRTPlanNode ();
}

//-----------------------------------------------------------------------------
vtkMRMLRTBeamNode* qSlicerExternalBeamPlanningModuleWidget::getCurrentBeamNode(vtkMRMLExternalBeamPlanningNode* paramNode)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!paramNode)
  {
    return NULL;
  }

  vtkMRMLRTPlanNode* rtPlanNode = paramNode->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::getCurrentBeamNode: Invalid rtplan node!";
    return NULL;
  }

  QTableWidgetItem *item = NULL;
  item = d->tableWidget_Beams->item(d->currentBeamRow, 0);
  if (!item)
  {
    return NULL;
  }
  int beamNumber = item->text().toInt();
  vtkMRMLRTBeamNode* beamNode = rtPlanNode->GetRTBeamNode (beamNumber);

  return beamNode;
}

//-----------------------------------------------------------------------------
vtkMRMLRTBeamNode* qSlicerExternalBeamPlanningModuleWidget::getCurrentBeamNode()
{
  return this->getCurrentBeamNode (this->getExternalBeamPlanningNode());
}

//-----------------------------------------------------------------------------
std::string qSlicerExternalBeamPlanningModuleWidget::getCurrentBeamName ()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  return beamNode->GetBeamName ();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateWidgetFromRTBeam (vtkMRMLRTBeamNode* beamNode)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // If node is empty, remove all tabs
  if (!beamNode) {
    d->tabWidget->clear ();
    d->lineEdit_BeamName->setText ("");
    // GCS FIX How do I disconnect ?
    return;
  }

  qvtkConnect (beamNode, vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));

  /* GCS FIX, set radiation type somehow */
  d->lineEdit_BeamName->setText(beamNode->GetBeamName());

  // Enable appropriate tabs and widgets for this beam type and set 
  // widget values from MRML node
  vtkMRMLRTBeamNode::RTRadiationType radType = beamNode->GetRadiationType();
  if (radType == vtkMRMLRTBeamNode::Photon)
  {
    // Enable or disable widgets on the prescription tab 
    d->label_BeamType->setEnabled(true);
    d->comboBox_BeamType->setEnabled(true);
    
    // Disable unneeded fields
    // Just in case there is a common index between photons and protons
    d->label_BeamLineType->setEnabled(false);
    d->comboBox_BeamLineType->setEnabled(false);
    d->label_EnergyPrescription->setEnabled(false);
    d->checkBox_EnergyPrescription->setEnabled(false);
    d->label_MinimumEnergy->setEnabled(false);
    d->doubleSpinBox_MinimumEnergy->setEnabled(false);
    d->label_MaximumEnergy->setEnabled(false);
    d->doubleSpinBox_MaximumEnergy->setEnabled(false);

    // Disable unneeded tabs
    int index;
    index = d->tabWidget->indexOf(d->tabWidgetPageProtonBeamModel);
    if (index >= 0)
    {
      d->tabWidget->removeTab(index);
    }

    // Enable needed tabs and set widget values
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }

    index = d->tabWidget->indexOf(d->tabWidgetPagePhotonBeamModel);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPagePhotonBeamModel, "Beam Model");
    }
  }
  else if (radType == vtkMRMLRTBeamNode::Proton)
  {
    // Enable needed fields
    // Just in case there is a common index between photons and protons
    d->label_BeamLineType->setEnabled(true);
    d->comboBox_BeamLineType->setEnabled(true);
    d->label_EnergyPrescription->setEnabled(true);
    d->checkBox_EnergyPrescription->setEnabled(true);

    // no energy prescription, so must be disabled for the moment
    d->label_MinimumEnergy->setEnabled(false);
    d->doubleSpinBox_MinimumEnergy->setEnabled(false);
    d->label_MaximumEnergy->setEnabled(false);
    d->doubleSpinBox_MaximumEnergy->setEnabled(false);

    // Disable unneeded tabs
    int index;
    index = d->tabWidget->indexOf(d->tabWidgetPagePhotonBeamModel);
    if (index >= 0)
    {
      d->tabWidget->removeTab(index);
    }

    // Enable needed tabs and set widget values
    index = d->tabWidget->indexOf(d->tabWidgetPageRx);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageRx, "Prescription");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageProtonEnergy);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageProtonEnergy, "Energy");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageProtonBeamModel);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageProtonBeamModel, "Beam Model");
    }
  }
  else if (radType == vtkMRMLRTBeamNode::Electron)
  {
    /* Not implemented */
    d->tabWidget->clear ();
  }

  // Set values into beam parameters tab
  d->lineEdit_BeamName->setText(QString::fromStdString(beamNode->GetBeamName()));
  switch (beamNode->GetRadiationType())
  {
  case vtkMRMLRTBeamNode::Photon:
    d->comboBox_RadiationType->setCurrentIndex(1);
    break;
  case vtkMRMLRTBeamNode::Electron:
    d->comboBox_RadiationType->setCurrentIndex(2);
    break;
  default:
    d->comboBox_RadiationType->setCurrentIndex(0); // Proton
    break;
  }

  // Set values into prescription tab
  switch (beamNode->GetBeamType())
  {
  case vtkMRMLRTProtonBeamNode::Dynamic:
    d->comboBox_RadiationType->setCurrentIndex(1);
    break;
  default:
    d->comboBox_RadiationType->setCurrentIndex(0); // Static
    break;
  }

  //d->doubleSpinBox_RxDose->setValue(beamNode->GetRxDose()); The RxDose doesn't need to be reset, it is the same for the plan

  double iso[3];
  beamNode->GetIsocenterPosition (iso);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(iso);
  d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);

  double rdp[3] = {beamNode->GetReferenceDosePointPosition(0), beamNode->GetReferenceDosePointPosition(1), beamNode->GetReferenceDosePointPosition(2)};
  d->MRMLCoordinatesWidget_DosePointCoordinates->setCoordinates(rdp);

  // Set values into energy tab
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  d->doubleSpinBox_ProximalMargin->setValue(protonNode->GetProximalMargin());
  d->doubleSpinBox_DistalMargin->setValue(protonNode->GetDistalMargin());
  if (protonNode->GetBeamLineType() == true)
  {
    d->comboBox_BeamLineType->setCurrentIndex(0);
  }
  else
  {
    d->comboBox_BeamLineType->setCurrentIndex(1);
  }
  d->checkBox_EnergyPrescription->setChecked(protonNode->GetManualEnergyLimits());
  d->doubleSpinBox_MinimumEnergy->setValue(protonNode->GetMinimumEnergy());
  d->doubleSpinBox_MaximumEnergy->setValue(protonNode->GetMaximumEnergy());

  // Set values into geometry tab
  d->doubleSpinBox_SAD->setValue(beamNode->GetSAD());
  d->RangeWidget_XJawsPosition->setValues(-beamNode->GetX1Jaw(), beamNode->GetX2Jaw());
  d->RangeWidget_YJawsPosition->setValues(-beamNode->GetY1Jaw(), beamNode->GetY2Jaw());
  d->SliderWidget_CollimatorAngle->setValue(beamNode->GetCollimatorAngle());
  d->SliderWidget_GantryAngle->blockSignals(true);
  d->SliderWidget_GantryAngle->setValue(beamNode->GetGantryAngle());
  d->SliderWidget_GantryAngle->blockSignals(false);
  d->SliderWidget_CouchAngle->setValue(beamNode->GetCouchAngle());
  d->doubleSpinBox_Smearing->setValue(beamNode->GetSmearing());
  d->doubleSpinBox_BeamWeight->setValue(beamNode->GetBeamWeight());

  // Set values into proton beam model
  if (beamNode->GetRadiationType() == vtkMRMLRTBeamNode::Proton)
  {
    d->doubleSpinBox_ApertureDistance->setValue(protonNode->GetApertureOffset());
    switch (protonNode->GetAlgorithm())
    {
    case vtkMRMLRTProtonBeamNode::CGS:
      d->comboBox_Algorithm->setCurrentIndex(1);
      break;
    case vtkMRMLRTProtonBeamNode::DGS:
      d->comboBox_Algorithm->setCurrentIndex(2);
      break;
    case vtkMRMLRTProtonBeamNode::HGS:
      d->comboBox_Algorithm->setCurrentIndex(3);
      break;
    default: // Ray Tracer or any other mistake
      d->comboBox_Algorithm->setCurrentIndex(0);
      break;
    }
    d->doubleSpinBox_PBResolution->setValue(protonNode->GetPBResolution());
    d->doubleSpinBox_SourceSize->setValue(protonNode->GetSourceSize());
    d->doubleSpinBox_EnergyResolution->setValue(protonNode->GetEnergyResolution());
    d->doubleSpinBox_EnergySpread->setValue(protonNode->GetEnergySpread());
    d->doubleSpinBox_StepLength->setValue(protonNode->GetStepLength());
    d->checkBox_WEDApproximation->setChecked(protonNode->GetLateralSpreadHomoApprox());
    d->checkBox_RangeCompensatorHighland->setChecked(protonNode->GetRangeCompensatorHighland());
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateWidgetFromParameterNode()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = this->getExternalBeamPlanningNode();
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
  if (!paramNode)
  {
    /* GCS FIX TODO: Still need to wipe the GUI in this case */
    return;
  }

  // If no plan node, try to find one in the scene
  // Otherwise, create a new one
  vtkMRMLRTPlanNode* planNode = paramNode->GetRTPlanNode ();
  if (!planNode)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
    if (node)
    {
      planNode = vtkMRMLRTPlanNode::SafeDownCast(node);
    }
    else
    {
      vtkSmartPointer<vtkMRMLRTPlanNode> newNode = vtkSmartPointer<vtkMRMLRTPlanNode>::New();
      planNode = newNode;
      this->mrmlScene()->AddNode(planNode);
    }
  }
  d->MRMLNodeComboBox_RtPlan->setCurrentNode(planNode);
  d->MRMLNodeComboBox_PlanPOIs->setCurrentNode(planNode->GetMarkupsFiducialNode());

  // if plan items exist, set GUI to plan items.  
  // else if GUI widgets have items selected, set them to plan
  // else set GUI widgets to empty
  if (planNode->GetRTPlanReferenceVolumeNode())
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(planNode->GetRTPlanReferenceVolumeNode());
  }
  else
  {
    this->referenceVolumeNodeChanged(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
  }
  if (planNode->GetRTPlanSegmentationNode())
  {
    d->MRMLSegmentSelectorWidget_PlanContours->setCurrentNode(planNode->GetRTPlanSegmentationNode());
  }
  else
  {
    this->planContoursNodeChanged(d->MRMLSegmentSelectorWidget_PlanContours->currentNode());
  }
  if (planNode->GetRTPlanDoseVolumeNode())
  {
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(planNode->GetRTPlanDoseVolumeNode());
  }
  else
  {
    this->rtDoseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
  }

  /* GCS FIX TODO: Wipe beam-specific UI items */

  return;

  /* GCS TODO: Here I still need to set the beam-specific widgets */
#if defined (commentout)

  d->SegmentSelectorWidget_TargetSegment->setCurrentNode(0);

  d->MRMLNodeComboBox_MLCPositionDoubleArray->setCurrentNode(0);
  //
  d->MRMLNodeComboBox_MLCPositionDoubleArray->setCurrentNode(paramNode->GetMLCPositionDoubleArrayNode());

#endif

#if defined (commentout)
  if (planNode->GetRTPlanReferenceVolumeNode())
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(planNode->GetRTPlanReferenceVolumeNode());
  }
  else
  {
    this->referenceVolumeNodeChanged(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
  }

  if (paramNode->GetMLCPositionDoubleArrayNode())
  {
    d->MRMLNodeComboBox_MLCPositionDoubleArray->setCurrentNode(paramNode->GetMLCPositionDoubleArrayNode());
  }
  else
  {
    this->MLCPositionDoubleArrayNodeChanged(d->MRMLNodeComboBox_MLCPositionDoubleArray->currentNode());
  }

#endif

#if defined (commentout)
  if (paramNode->GetBeamName())
  {
    d->lineEdit_BeamName->setText(paramNode->GetBeamName());
  }
#endif
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::setup()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  vtkSlicerExternalBeamPlanningModuleLogic* externalBeamPlanningModuleLogic =
    vtkSlicerExternalBeamPlanningModuleLogic::SafeDownCast(this->logic());

  qSlicerAbstractCoreModule* matlabDoseCalculationModule =
    qSlicerCoreApplication::application()->moduleManager()->module("MatlabDoseCalculation");
  if (matlabDoseCalculationModule) {
    vtkSlicerCLIModuleLogic* matlabDoseCalculationModuleLogic =
      vtkSlicerCLIModuleLogic::SafeDownCast(matlabDoseCalculationModule->logic());
    externalBeamPlanningModuleLogic->SetMatlabDoseCalculationModuleLogic(matlabDoseCalculationModuleLogic);
  } else {
    qWarning() << "MatlabDoseCalculation module is not found!";
  }

  // Make connections
  this->connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(externalBeamPlanningNodeChanged(vtkMRMLNode*)) );

  // RT plan page
  this->connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLSegmentSelectorWidget_PlanContours, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planContoursNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_RtPlan, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(rtPlanNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_PlanPOIs, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(planPOIsNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(rtDoseVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_DoseROI, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(rtDoseROINodeChanged(vtkMRMLNode*)) );
  this->connect( d->lineEdit_DoseGridSpacing, SIGNAL(textChanged(const QString &)), this, SLOT(doseGridSpacingChanged(const QString &)) );
  this->connect( d->comboBox_DoseEngineType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(doseEngineTypeChanged(const QString &)) );

  // RT Beams page
  this->connect( d->tableWidget_Beams, SIGNAL(cellClicked(int, int)), this, SLOT(tableWidgetCellClicked(int, int)) );
  this->connect( d->pushButton_AddBeam, SIGNAL(clicked()), this, SLOT(addBeamClicked()) );
  this->connect( d->pushButton_RemoveBeam, SIGNAL(clicked()), this, SLOT(removeBeamClicked()) );

  /* Beam global parameters */
  this->connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  this->connect( d->comboBox_RadiationType, SIGNAL(currentIndexChanged(int)), this, SLOT(radiationTypeChanged(int)) );

  /* Prescription page */
  this->connect( d->comboBox_BeamType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamTypeChanged(const QString &)) );
  this->connect( d->MRMLSegmentSelectorWidget_TargetVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(targetVolumeNodeChanged(vtkMRMLNode*)) );
  this->connect( d->MRMLSegmentSelectorWidget_TargetVolume, SIGNAL(currentSegmentChanged(QString)), this, SLOT(targetVolumeSegmentChanged(const QString&)) );
  this->connect( d->doubleSpinBox_RxDose, SIGNAL(valueChanged(double)), this, SLOT(RxDoseChanged(double)) );
  this->connect( d->comboBox_IsocenterSpec, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(isocenterSpecChanged(const QString &)));
  this->connect( d->MRMLCoordinatesWidget_IsocenterCoordinates, SIGNAL(coordinatesChanged(double*)), this, SLOT(isocenterCoordinatesChanged(double *)));

  /* Energy page */
  this->connect( d->doubleSpinBox_ProximalMargin, SIGNAL(valueChanged(double)), this, SLOT(proximalMarginChanged(double)) );
  this->connect( d->doubleSpinBox_DistalMargin, SIGNAL(valueChanged(double)), this, SLOT(distalMarginChanged(double)) );
    //Proton widgets
  this->connect( d->comboBox_BeamLineType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(beamLineTypeChanged(const QString &)) );
  this->connect( d->checkBox_EnergyPrescription, SIGNAL(clicked(bool)), this, SLOT(manualEnergyPrescriptionChanged(bool)) );
  this->connect( d->doubleSpinBox_MinimumEnergy, SIGNAL(valueChanged(double)), this, SLOT(minimumEnergyChanged(double)) );
  this->connect( d->doubleSpinBox_MaximumEnergy, SIGNAL(valueChanged(double)), this, SLOT(maximumEnergyChanged(double)) );

  /* Geometry page */
  this->connect( d->MRMLNodeComboBox_MLCPositionDoubleArray, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(MLCPositionDoubleArrayNodeChanged(vtkMRMLNode*)) );
  this->connect (d->doubleSpinBox_SAD, SIGNAL(valueChanged(double)), this, SLOT(sourceDistanceChanged(double)) );
  this->connect( d->RangeWidget_XJawsPosition, SIGNAL(valuesChanged(double, double)), this, SLOT(XJawsPositionValuesChanged(double, double)) );
  this->connect( d->RangeWidget_YJawsPosition, SIGNAL(valuesChanged(double, double)), this, SLOT(YJawsPositionValuesChanged(double, double)) );
  this->connect( d->SliderWidget_CollimatorAngle, SIGNAL(valueChanged(double)), this, SLOT(collimatorAngleChanged(double)) );
  this->connect( d->SliderWidget_GantryAngle, SIGNAL(valueChanged(double)), this, SLOT(gantryAngleChanged(double)) );
  this->connect( d->SliderWidget_CouchAngle, SIGNAL(valueChanged(double)), this, SLOT(couchAngleChanged(double)) );
  this->connect( d->doubleSpinBox_Smearing, SIGNAL(valueChanged(double)), this, SLOT(smearingChanged(double)) );
  this->connect( d->doubleSpinBox_BeamWeight, SIGNAL(valueChanged(double)), this, SLOT(beamWeightChanged(double)) );

  /* Proton beam model */
  this->connect( d->doubleSpinBox_ApertureDistance, SIGNAL(valueChanged(double)), this, SLOT(apertureDistanceChanged(double)) );
  this->connect( d->comboBox_Algorithm, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(algorithmChanged(const QString &)) );
  this->connect( d->doubleSpinBox_PBResolution, SIGNAL(valueChanged(double)), this, SLOT(PBResolutionChanged(double)) );
  this->connect( d->doubleSpinBox_SourceSize, SIGNAL(valueChanged(double)), this, SLOT(sourceSizeChanged(double)) );
  this->connect( d->doubleSpinBox_EnergyResolution, SIGNAL(valueChanged(double)), this, SLOT(energyResolutionChanged(double)) );
  this->connect( d->doubleSpinBox_EnergySpread, SIGNAL(valueChanged(double)), this, SLOT(energySpreadChanged(double)) );
  this->connect( d->doubleSpinBox_StepLength, SIGNAL(valueChanged(double)), this, SLOT(stepLengthChanged(double)) );
  this->connect( d->checkBox_WEDApproximation, SIGNAL(clicked(bool)), this, SLOT(WEDApproximationChanged(bool)) );
  this->connect( d->checkBox_RangeCompensatorHighland, SIGNAL(clicked(bool)), this, SLOT(rangeCompensatorHighlandChanged(bool)) );

  /* Beam visualization */
  this->connect( d->pushButton_UpdateDRR, SIGNAL(clicked()), this, SLOT(updateDRRClicked()) );
  this->connect( d->checkBox_BeamEyesView, SIGNAL(clicked(bool)), this, SLOT(beamEyesViewClicked(bool)) );
  this->connect( d->checkBox_ContoursInBEW, SIGNAL(clicked(bool)), this, SLOT(contoursInBEWClicked(bool)) );

  /* Calculation buttons */
  this->connect( d->pushButton_CalculateDose, SIGNAL(clicked()), this, SLOT(calculateDoseClicked()) );
  this->connect( d->pushButton_CalculateWED, SIGNAL(clicked()), this, SLOT(calculateWEDClicked()) );
  this->connect( d->pushButton_ClearDose, SIGNAL(clicked()), this, SLOT(clearDoseClicked()) );

  /* Disable unused buttons in prescription task */
  //this->radiationTypeChanged(0);

  /* Remove all tabs in Beam TabWidget */
  d->tabWidget->clear ();

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::externalBeamPlanningNodeChanged(vtkMRMLNode *node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(d->logic()->GetExternalBeamPlanningNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(externalBeamPlanningNodeModified()));

  d->logic()->SetAndObserveExternalBeamPlanningNode(paramNode);
  this->updateWidgetFromParameterNode();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onExternalBeamPlanningNodeModified()
{
  this->updateWidgetFromParameterNode();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onLogicModified()
{
  this->updateRTBeamTableWidget();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::onRTBeamNodeModifiedEvent()
{
  qDebug() << "Got onRTBeamNodeModifiedEvent";
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid parameter node!";
    return;
  }

  // Clear the beam table
  d->tableWidget_Beams->setRowCount(0);
  d->tableWidget_Beams->setColumnCount(0);
  d->tableWidget_Beams->clearContents();

  // Get rt plan node for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* planNode = this->getRTPlanNode ();
  if (!planNode)
  { 
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid rtplan node!";
    return;
  }

  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetRTBeamNodes (beams);

  // Set up the table
  d->tableWidget_Beams->setColumnCount(4);
  QStringList headerLabels;
  headerLabels << "Beam#" << "Name" << "Type" << "Radiation";

  d->tableWidget_Beams->setColumnWidth(0, 24);
  d->tableWidget_Beams->setHorizontalHeaderLabels(headerLabels);
  d->tableWidget_Beams->setRowCount(beams.size());

  // Fill the table
  std::vector<vtkMRMLRTBeamNode*>::iterator it;
  int i;
  for (it = beams.begin(), i = 0; it != beams.end(); ++it, ++i)
  {
    vtkMRMLRTBeamNode* beamNode = *it;
    if (beamNode)
    {
      d->tableWidget_Beams->setItem(i, 0, new QTableWidgetItem( QString::number(beamNode->GetBeamNumber()) ) );
      d->tableWidget_Beams->setItem(i, 1, new QTableWidgetItem( QString(beamNode->GetBeamName()) ) );
      d->tableWidget_Beams->setItem(i, 2, new QTableWidgetItem( QString::number(beamNode->GetGantryAngle()) ) );
    }
  }
  if (d->currentBeamRow >= 0)
  {
    d->tableWidget_Beams->selectRow(d->currentBeamRow);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::referenceVolumeNodeChanged: Invalid scene!";
    return;
  }
  if (!node) {
    return;
  }

  vtkMRMLRTPlanNode* planNode = this->getRTPlanNode ();
  if (!planNode) {
    /* GCS FIX TODO *** Probably I should create plan node here. ***/
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetAndObserveRTPlanReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::planContoursNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::planContoursNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLRTPlanNode* planNode = this->getRTPlanNode ();
  if (!planNode)
  {
    return;
  }

  planNode->DisableModifiedEventOn();
  planNode->SetAndObserveRTPlanSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  planNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rtPlanNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::rtPlanNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveRTPlanNode(vtkMRMLRTPlanNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::planPOIsNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::planPOIsNodeChanged: Invalid scene!";
    return;
  }

  // Update in plan node
  vtkMRMLRTPlanNode* planNode = this->getRTPlanNode();
  if (!planNode)
  {
    return;
  }
  planNode->SetAndObserveMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode::SafeDownCast(node));

  /* GCS FIX: Update beams isocenters */
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rtDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::rtDoseVolumeNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = paramNode->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::rtDoseVolumeNodeChanged: Invalid rtplan node!";
    return;
  }

  rtPlanNode->DisableModifiedEventOn();
  rtPlanNode->SetAndObserveRTPlanDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  rtPlanNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rtDoseROINodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::rtDoseROINodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  // paramNode->DisableModifiedEventOn();
  // paramNode->SetAndObservePlanContourSetNode(node);
  // paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::doseGridSpacingChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  // TODO: to be implemented
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::doseEngineTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = paramNode->GetRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::doseEngineTypeChanged: Invalid rtplan node!";
    return;
  }

  if (text.compare("Plastimatch") == 0)
  {
    rtPlanNode->SetRTPlanDoseEngine(vtkMRMLRTPlanNode::Plastimatch);
  }
  else if (text.compare("PMH") == 0)
  {
    rtPlanNode->SetRTPlanDoseEngine(vtkMRMLRTPlanNode::PMH);
  }
  else if (text.compare("Matlab") == 0)
  {
    rtPlanNode->SetRTPlanDoseEngine(vtkMRMLRTPlanNode::Matlab);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::addBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::addBeamClicked Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = this->getExternalBeamPlanningNode();
  if (!paramNode)
  {
    qWarning() << "qSlicerExternalBeamPlanningModuleWidget::addBeamClicked without parameter node";
    return;
  }

  // Create new beam node by replicating currently selected beam
  vtkMRMLRTBeamNode *beamNode = d->logic()->AddBeam(this->getCurrentBeamNode ());

  // Disconnect events for currently selected row
  if (this->getCurrentBeamNode() != NULL)
  {
    qvtkDisconnect (this->getCurrentBeamNode(), vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));
  }
  
  // Make new beam current in the table
  d->currentBeamRow = d->totalBeamRows;
  d->totalBeamRows ++;
  d->tableWidget_Beams->selectRow(d->currentBeamRow);

  QString new_name = "New beam";
  new_name.append(QString::number(d->totalBeamRows));
  beamNode->SetBeamName(new_name.toStdString().c_str());
  beamNameChanged(new_name);

  /* GCS TODO FIX -- this should be called when logic is modified, maybe 
     it gets called twice ? */
  this->updateRTBeamTableWidget();

  // Update UI
  this->updateWidgetFromRTBeam (beamNode);

  // Update beam visualization
  this->UpdateBeamTransform();

  // Update beam visualization
  this->UpdateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::removeBeamClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    return;
  }

  // Disconnect events for currently selected row
  qvtkDisconnect (beamNode, vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));
  
  // Remove beam
  d->logic()->RemoveBeam(beamNode);

  // Select a different beam as current
  d->totalBeamRows--;
  if (d->currentBeamRow >= d->totalBeamRows)
  {
    d->currentBeamRow = d->totalBeamRows-1;
  }

  // Update UI table
  d->tableWidget_Beams->selectRow(d->currentBeamRow);
  this->updateRTBeamTableWidget();

  // Update lower UI tabs
  beamNode = this->getCurrentBeamNode();
  this->updateWidgetFromRTBeam (beamNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::tableWidgetCellClicked(int row, int column)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(column);

  // Don't do anything when clicking on already selected beam
  if (row == d->currentBeamRow)
  {
    return;
  }

  // Disconnect events for currently selected row
  qvtkDisconnect (this->getCurrentBeamNode(), vtkCommand::ModifiedEvent, this, SLOT(onRTBeamNodeModifiedEvent()));

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  // Make sure inputs are initialized
  d->currentBeamRow = row;
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    qCritical() << "tableWidgetItemClicked: Inputs are not initialized!";
    return;
  }

  // Copy data from beam into tabs UI
  this->updateWidgetFromRTBeam (beamNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamNameChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamNameChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    return;
  }
  beamNode->SetBeamName(text.toStdString().c_str());

  // Update in table
  this->updateRTBeamTableWidget();
  
  // Update beam visualization
  this->UpdateBeamTransform();

  // Update beam visualization
  this->UpdateBeamGeometryModel();

}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::radiationTypeChanged(int index)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (index == -1)
  {
    return;
  }

  // GCS FIX: This needs to make changes to the node, then set 
  // values from node into UI

#if defined (commentout)
  QString text = d->comboBox_RadiationType->currentText();

  if (text == "Photon")
  {
    // Make Photon pages visible and others invisible
    int index =-1;
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index >= 0)
    {
      d->tabWidget->removeTab(index);
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageRx);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageRx, "Prescription");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageBeams, "Beam Model");
    }
  }
  else if (text == "Proton")
  {
    // Make Photon pages visible and others invisible
    int index =-1;
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index >=0)
    {
      d->tabWidget->removeTab(index);
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index >= 0)
    {
      d->tabWidget->removeTab(index);
    }
    index = 0;
    index = d->tabWidget->indexOf(d->tabWidgetPageRx);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageRx, "Prescription");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageGeometry);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageGeometry, "Geometry");
    }
    index = d->tabWidget->indexOf(d->tabWidgetPageBeams);
    if (index == -1)
    {
      d->tabWidget->addTab(d->tabWidgetPageBeams, "Beam Model");
    }

  }
  else if (text == "Electron")
  {
  }
#endif
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamTypeChanged: Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamTypeChanged: No current beam node.";
    return;
  }

  if (text.compare("Dynamic") == 0)
  {
    beamNode->SetBeamType(vtkMRMLRTBeamNode::Dynamic);
  }
  else
  {
    beamNode->SetBeamType(vtkMRMLRTBeamNode::Static);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::targetVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::targetVolumeNodeChanged: Invalid scene!";
    return;
  }

  // Update in plan node
  vtkMRMLRTPlanNode* planNode = this->getRTPlanNode ();
  if (!planNode)
  {
    return;
  }
  planNode->DisableModifiedEventOn();
  planNode->SetAndObserveRTPlanSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  planNode->DisableModifiedEventOff();

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    return;
  }
  beamNode->SetAndObserveTargetSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::targetVolumeSegmentChanged(const QString& segment)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::targetVolumeSegmentChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    return;
  }
  std::string s = segment.toUtf8().constData();
  beamNode->SetTargetSegmentID (s.c_str());

  if (beamNode->GetIsocenterSpec() == vtkMRMLRTBeamNode::CenterOfTarget)
  {
    d->logic()->SetBeamIsocenterToTargetCenter(beamNode);

    // GCS FIX: Testing...I copied this from above.  I should be getting
    // a markup moved event, why didn't I?
    double iso[3];
    beamNode->GetIsocenterPosition (iso);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(true);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->setCoordinates(iso);
    d->MRMLCoordinatesWidget_IsocenterCoordinates->blockSignals(false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::RxDoseChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::RxDoseChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTPlanNode* rtPlanNode = this->getRTPlanNode();
  if (!rtPlanNode) {
    return;
  }
  rtPlanNode->SetRxDose(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterSpecChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::doseEngineTypeChanged: Invalid rtplan node!";
    return;
  }

  if (text.compare("Center of Target") == 0)
  {
    qDebug() << "Setting isocenter spec to center of target";
    d->logic()->SetBeamIsocenterToTargetCenter(beamNode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterCoordinatesChanged(double *coords)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::isocenterCoordinatesChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::isocenterCoordinatesChanged: Invalid beam node!";
    return;
  }

  beamNode->SetIsocenterPosition(coords);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::isocenterFiducialNodeChangedfromCoordinates(double* coordinates) //MD Fix
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(coordinates);

}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::dosePointFiducialNodeChangedfromCoordinates(double* coordinates) // Md Fix
{
    Q_D(qSlicerExternalBeamPlanningModuleWidget);
    UNUSED_VARIABLE(coordinates);

    if (!this->mrmlScene()) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::dosePointFiducialNodeChangedfromCoordinates: Invalid scene!";
    return;
  }

    // TODO: to be implemented
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::proximalMarginChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::proximalMarginChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::proximalMarginChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetProximalMargin(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::distalMarginChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::distalMarginChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::distalMarginChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetDistalMargin(value);
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamLineTypeChanged(const QString &text)
{
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamLineTypeChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamLineTypeChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  
  if (text.compare("Active scanning") == 0)
  {
    protonNode->SetBeamLineType("active");
  }
  else
  {
    protonNode->SetBeamLineType("passive");
  }
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::manualEnergyPrescriptionChanged(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::manualEnergyPrescriptionChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::manualEnergyPrescriptionChanged: No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetHavePrescription(checked);

  if (protonNode->GetHavePrescription())
  {
      d->doubleSpinBox_MinimumEnergy->setEnabled(true);
      d->doubleSpinBox_MaximumEnergy->setEnabled(true);
      d->label_MinimumEnergy->setEnabled(true);
      d->label_MaximumEnergy->setEnabled(true);
  }
  else
  {
      d->doubleSpinBox_MinimumEnergy->setEnabled(false);
      d->doubleSpinBox_MaximumEnergy->setEnabled(false);
      d->label_MinimumEnergy->setEnabled(false);
      d->label_MaximumEnergy->setEnabled(false);
  }
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::minimumEnergyChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::minimumEnergyChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::minimumEnergyChanged: No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetMinimumEnergy(value);
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::maximumEnergyChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::maximumEnergyChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::maximumEnergyChanged: No current beam node.";
    return;
  }

  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetMaximumEnergy(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::MLCPositionDoubleArrayNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::MLCPositionDoubleArrayNodeChanged: Invalid scene!";
    return;
  }

  // GCS FIX TODO *** Come back to this later ***
#if defined (commentout)

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode || !node)
  {
    return;
  }

  // Get rt plan node for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* rtPlanNode = d->logic()->GetExternalBeamPlanningNode()->GetRtPlanNode();
  if (!rtPlanNode)
  { 
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateRTBeamTableWidget: Invalid rtplan node!";
    return;
  }

  paramNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode(paramNode);

  // Make sure inputs are initialized
  if (!beamNode)
  {
    qCritical() << "MLCPositionDoubleArrayNodeChanged: Inputs are not initialized!";
    return;
  }

  beamNode->SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode::SafeDownCast(node));
#endif
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::XJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::XJawPositionsValueChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::XJawPositionsValueChanged: No current beam node.";
    return;
  }
  beamNode->SetX1Jaw(-minVal);
  beamNode->SetX2Jaw( maxVal);

  // Update beam visualization
  this->UpdateBeamTransform();
  this->UpdateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::YJawsPositionValuesChanged(double minVal, double maxVal)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::YJawPositionsValueChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::YJawPositionsValueChanged: No current beam node.";
    return;
  }
  beamNode->SetY1Jaw(-minVal);
  beamNode->SetY2Jaw( maxVal);

  // Update beam visualization
  this->UpdateBeamTransform();
  this->UpdateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::gantryAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::gantryAngleChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::gantryAngleChanged: No current beam selected!";
    return;
  }
  beamNode->SetGantryAngle(value);

  // Update beam visualization
  this->UpdateBeamTransform();

  // Update the table
  this->updateRTBeamTableWidget();

  // Update beam visualization
  this->UpdateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::collimatorAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::collimatorAngleChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::collimatorAngleChanged: No current beam selected!";
    return;
  }
  beamNode->SetCollimatorAngle(value);

  // Update beam visualization
  this->UpdateBeamTransform();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::couchAngleChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::couchAngleChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (!beamNode) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::couchAngleChanged: No current beam selected!";
    return;
  }
  beamNode->SetCouchAngle(value);

  // Update beam visualization

  this->UpdateBeamTransform();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::smearingChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::smearingChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::smearingChanged: No current beam node.";
    return;
  }
  
  beamNode->SetSmearing(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamWeightChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamWeightChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamWeightChanged: No current beam node.";
    return;
  }
  beamNode->SetBeamWeight(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::sourceDistanceChanged(double value)
{
   Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::sourceDistanceChanged: Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::sourceDistanceChanged: No current beam node.";
    return;
  }
  beamNode->SetSAD(value);

  // Update beam visualization
  this->UpdateBeamGeometryModel();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::apertureDistanceChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::apertureDistanceChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::apertureDistanceChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetApertureOffset(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::algorithmChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::algorithmChanged: Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::algorithmChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);

  if(text == "Ray tracer algorithm")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
    d->label_CalculateDoseStatus->setText("Ray Tracer Algorithm");
  }
  else if(text == "Cartesian geometry dose calculation")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::CGS);
    d->label_CalculateDoseStatus->setText("Cartesian Geometry Algorithm");
  }
  else if(text == "Divergent geometry dose calculation")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::DGS);
    d->label_CalculateDoseStatus->setText("Divergent Geometry Algorithm");
  }
  else if(text == "Hong dose calculation")
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::HGS);
    d->label_CalculateDoseStatus->setText("Hong Algorithm");
  }
  else
  {
    protonNode->SetAlgorithm(vtkMRMLRTProtonBeamNode::RayTracer);
    d->label_CalculateDoseStatus->setText("Algorithm error, Ray Tracer chosen by default");
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::PBResolutionChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::PBResolutionChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  // GCS FIX TODO: Why are the function names different?
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::PBResolutionChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetPBResolution(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::sourceSizeChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::sourceSizeChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::sourceSizeChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetSourceSize(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::energyResolutionChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::energyResolutionChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::energyResolutionChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetEnergyResolution(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::energySpreadChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::energySpreadChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::energySpreadChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetEnergySpread(value);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::stepLengthChanged(double value)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::stepLengthChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::stepLengthChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetStepLength(value);
}


//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::WEDApproximationChanged(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::WEDApproximationChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::WEDApproximationChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetLateralSpreadHomoApprox(checked);
}

//---------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::rangeCompensatorHighlandChanged(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::rangeCompensatorHighlandChanged: Invalid scene!";
    return;
  }

  // Set in proton beam node
  vtkMRMLRTBeamNode* beamNode = this->getCurrentBeamNode();
  if (beamNode == NULL) {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::rangeCompensatorHighlandChanged: No current beam node.";
    return;
  }
  vtkMRMLRTProtonBeamNode* protonNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  protonNode->SetRangeCompensatorHighland(checked);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::beamEyesViewClicked(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::beamEyesViewClicked: Invalid scene!";
    return;
  }

  // Set in parameter node
  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode) 
  {
    return;
  }

  if (checked)
  {
    qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutTwoOverTwoView);
  }
  else
  {
    qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutFourUpView);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::contoursInBEWClicked(bool checked)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::contoursInBEWClicked: Invalid scene!";
    return;
  }

  // Set in parameter node
  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode) 
  {
    return;
  }

  // Todo: add the logic to check if contours should be included in the DRR view
  // right now the contours are included always. 
  if (checked)
  {
  }
  else
  {
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateDRRClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::updateDRRClicked: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);

  if (item)
  {
    strcpy(beamName, item->text().toStdString().c_str());
    d->logic()->UpdateDRR(beamName);
  }
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateWEDClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting WED calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::calculateWEDClicked: Invalid scene!";
    return;
  }

  /// GCS FIX TODO *** Implementation needs work ***

#if defined (commentout)

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }

  /* Make sure inputs were specified - only CT needed*/
  vtkMRMLScalarVolumeNode* referenceVolume = planNode->GetReferenceVolumeNode();
  if (!referenceVolume)
  {
    d->label_CalculateDoseStatus->setText("No reference image");
    return;
  }

  /* Copy pertinent variable values from GUI to logic */
  /* Is this the right place for this? */
  d->logic()->GetExternalBeamPlanningNode()->SetGantryAngle(d->SliderWidget_GantryAngle->value());

  /* OK, we're good to go (well, not really, but let's pretend). 
     Do the actual computation in the logic object */
  d->logic()->ComputeWED();

  d->label_CalculateDoseStatus->setText("WED calculation done.");
#endif
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->label_CalculateDoseStatus->setText("Starting dose calculation...");

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked: Invalid paramNode!";
    return;
  }

  /* Make sure inputs were specified */
  vtkMRMLRTPlanNode* planNode = this->getRTPlanNode ();
  if (!planNode) {
    d->label_CalculateDoseStatus->setText("No RT Plan node found");
    return;
  }
  if (planNode->GetRTPlanDoseEngine() != vtkMRMLRTPlanNode::Plastimatch){
    d->label_CalculateDoseStatus->setText("Dose calculation is not available for this dose calculation engine");
    return;
  }

  vtkMRMLScalarVolumeNode* referenceVolume = planNode->GetRTPlanReferenceVolumeNode();
  if (!referenceVolume) {
    d->label_CalculateDoseStatus->setText("No reference image");
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = planNode->GetRTPlanSegmentationNode();
  if (!segmentationNode)
  {
    d->label_CalculateDoseStatus->setText("No proton target volume"); // MD Fix -> dose could be computer without target
    return;
  }

  /* Make sure segmentation has a labelmap form */
  /* GCS FIX: Maybe this explicit conversion is not even needed */
#if defined (commentout)
  if ( !segmentationNode->HasMergedLabelmap() )
  {
    d->label_CalculateDoseStatus->setText("Target volume must be labelmap");
    return;
  }
#endif
  
  /* The last verifications were fine so we can compute the dose */
  /* Dose Calculation - loop on all the beam and sum in a global dose matrix */

  // Get rt plan node for ExternalBeamPlanning node
  vtkMRMLRTPlanNode* rtPlanNode = this->getRTPlanNode();
  if (!rtPlanNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::calculateDoseClicked: Invalid rtplan node!";
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  if (!beams) 
  {
    d->label_CalculateDoseStatus->setText("no beam found in the plan");
    return;
  }
  vtkMRMLRTProtonBeamNode* beamNode = NULL;

  d->logic()->InitializeAccumulatedDose();
  d->label_CalculateDoseStatus->setText("Dose calculation in progress"); //+ beamNode->GetBeamName();

  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTProtonBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if (beamNode->GetRadiationType() != vtkMRMLRTBeamNode::Proton){
          d->label_CalculateDoseStatus->setText("Dose calculation is not available for this particle");
          return;
      }
      d->logic()->ComputeDose(beamNode);
      // sum in the final dose matrix with weights externalbeam for RxDose, and beamNode for beam wieght
    }
    else
    {
      d->label_CalculateDoseStatus->setText("beam not found"); // + beamNode->GetBeamName();
    }
  }
  
  d->logic()->RegisterAccumulatedDose();
  d->label_CalculateDoseStatus->setText("Dose calculation done.");
  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::clearDoseClicked()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::clearDoseClicked: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::clearDoseClicked: Invalid paramNode!";
    return;
  }

  d->logic()->RemoveDoseNodes();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::updateBeamParameters()
{
  //Q_D(qSlicerExternalBeamPlanningModuleWidget);

  // TODO:
  std::string errorMessage;
  //d->logic()->UpdateBeam();
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::UpdateBeamTransform(vtkMRMLRTBeamNode *beamNode)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->logic()->UpdateBeamTransform(beamNode);
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::UpdateBeamTransform()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  d->logic()->UpdateBeamTransform(this->getCurrentBeamNode());
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::UpdateBeamGeometryModel()
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);

  QTableWidgetItem *item = NULL;
  char beamName[100];
  item = d->tableWidget_Beams->item(d->currentBeamRow, 1);
  if (!item)
  {
    return;
  }
  else 
  {
    strcpy(beamName, item->text().toStdString().c_str());
    d->logic()->UpdateBeamGeometryModel(beamName);
  }
  return;
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModuleWidget::collimatorTypeChanged(const QString &text)
{
  Q_D(qSlicerExternalBeamPlanningModuleWidget);
  UNUSED_VARIABLE(text);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerExternalBeamPlanningModuleWidget::collimatorTypeChanged: Invalid scene!";
    return;
  }

  vtkMRMLExternalBeamPlanningNode* paramNode = d->logic()->GetExternalBeamPlanningNode();
  if (!paramNode)
  {
    return;
  }
  
  //TODO:
}
