// Qt includes
#include <QDebug>
#include <QtCore>
#include <QtGui>

// SlicerQt includes
#include "qSlicerDeformationFieldVisualizerModuleWidget.h"
#include "ui_qSlicerDeformationFieldVisualizerModule.h"
#include <qSlicerApplication.h>

// DeformationVisualizer includes
#include "vtkSlicerDeformationFieldVisualizerLogic.h"
#include "vtkMRMLDeformationFieldVisualizerNode.h"

// MMRL includes
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLBSplineTransformNode.h>
#include <vtkMRMLGridTransformNode.h>
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkGeneralTransform.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DeformationFieldVisualizer
// TODO: Keeping private for now until after fixes and enhancements
class qSlicerDeformationFieldVisualizerModuleWidgetPrivate: public Ui_qSlicerDeformationFieldVisualizerModule{
  Q_DECLARE_PUBLIC(qSlicerDeformationFieldVisualizerModuleWidget);
protected:
  qSlicerDeformationFieldVisualizerModuleWidget* const q_ptr;
public:
  qSlicerDeformationFieldVisualizerModuleWidgetPrivate(qSlicerDeformationFieldVisualizerModuleWidget& object);
  vtkSlicerDeformationFieldVisualizerLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerDeformationFieldVisualizerModuleWidgetPrivate methods
//-----------------------------------------------------------------------------
qSlicerDeformationFieldVisualizerModuleWidgetPrivate::qSlicerDeformationFieldVisualizerModuleWidgetPrivate(qSlicerDeformationFieldVisualizerModuleWidget& object) : q_ptr(&object)
{
}

vtkSlicerDeformationFieldVisualizerLogic* qSlicerDeformationFieldVisualizerModuleWidgetPrivate::logic() const {
  Q_Q( const qSlicerDeformationFieldVisualizerModuleWidget );
  return vtkSlicerDeformationFieldVisualizerLogic::SafeDownCast( q->logic() );
}


//-----------------------------------------------------------------------------
// qSlicerDeformationFieldVisualizerModuleWidget methods
//-----------------------------------------------------------------------------
qSlicerDeformationFieldVisualizerModuleWidget::qSlicerDeformationFieldVisualizerModuleWidget(QWidget* _parent) : Superclass( _parent ) , d_ptr(new qSlicerDeformationFieldVisualizerModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerDeformationFieldVisualizerModuleWidget::~qSlicerDeformationFieldVisualizerModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  this->Superclass::setMRMLScene(scene);

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetDeformationFieldVisualizerNode() == 0){
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLDeformationFieldVisualizerNode");
    if (node){
      this->setDeformationFieldVisualizerParametersNode(vtkMRMLDeformationFieldVisualizerNode::SafeDownCast(node));
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::onEnter()
{
  if (!this->mrmlScene()){return;}

  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  if (d->logic() == NULL){return;}

  //Check for existing parameter node
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();

  if (pNode == NULL){
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLDeformationFieldVisualizerNode");
    if (node){
      pNode = vtkMRMLDeformationFieldVisualizerNode::SafeDownCast(node);
      d->logic()->SetAndObserveDeformationFieldVisualizerNode(pNode);
      return;
    }
    else{
      vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode> newNode = vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveDeformationFieldVisualizerNode(newNode);
    }
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setDeformationFieldVisualizerParametersNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  vtkMRMLDeformationFieldVisualizerNode* pNode = vtkMRMLDeformationFieldVisualizerNode::SafeDownCast(node);

  qvtkReconnect( d->logic()->GetDeformationFieldVisualizerNode(), pNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  d->logic()->SetAndObserveDeformationFieldVisualizerNode(pNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();

  if (pNode && this->mrmlScene()){
    d->ParameterComboBox->setCurrentNode(pNode);

    if (pNode->GetInputVolumeNodeID()){
      d->InputFieldComboBox->setCurrentNode(pNode->GetInputVolumeNodeID());
    }
    else{
      this->inputVolumeChanged(d->InputFieldComboBox->currentNode());
    }

    if (pNode->GetReferenceVolumeNodeID()){
      d->InputReferenceComboBox->setCurrentNode(pNode->GetReferenceVolumeNodeID());
    }
    else{
      this->referenceVolumeChanged(d->InputReferenceComboBox->currentNode());
    }    

    if (pNode->GetOutputModelNodeID()){
      d->OutputModelComboBox->setCurrentNode(pNode->GetOutputModelNodeID());
    }
    else{
      this->outputModelChanged(d->OutputModelComboBox->currentNode());
    }  

    //Update Visualization Parameters

    // Glyph Parameters
    d->InputGlyphPointMax->setValue(pNode->GetGlyphPointMax());
    d->InputGlyphSeed->setValue(pNode->GetGlyphSeed());
    d->InputGlyphScale->setValue(pNode->GetGlyphScale());
    d->InputGlyphScaleDirectional->setChecked(pNode->GetGlyphScaleDirectional());
    d->InputGlyphScaleIsotropic->setChecked(pNode->GetGlyphScaleIsotropic());
    d->InputGlyphThreshold->setMaximumValue(pNode->GetGlyphThresholdMax());
    d->InputGlyphThreshold->setMinimumValue(pNode->GetGlyphThresholdMin());
    d->GlyphSourceComboBox->setCurrentIndex(pNode->GetGlyphSourceOption());
    // Arrow Parameters
    d->InputGlyphArrowTipLength->setValue(pNode->GetGlyphArrowTipLength());
    d->InputGlyphArrowTipRadius->setValue(pNode->GetGlyphArrowTipRadius());
    d->InputGlyphArrowShaftRadius->setValue(pNode->GetGlyphArrowShaftRadius());
    d->InputGlyphArrowResolution->setValue(pNode->GetGlyphArrowResolution());
    // Cone Parameters
    d->InputGlyphConeHeight->setValue(pNode->GetGlyphConeHeight());
    d->InputGlyphConeRadius->setValue(pNode->GetGlyphConeRadius());
    d->InputGlyphConeResolution->setValue(pNode->GetGlyphConeResolution());
    // Sphere Parameters
    d->InputGlyphSphereResolution->setValue(pNode->GetGlyphSphereResolution());

    // Grid Parameters
    d->InputGridScale->setValue(pNode->GetGridScale());
    d->InputGridDensity->setValue(pNode->GetGridDensity());

    // Block Parameters
    d->InputBlockScale->setValue(pNode->GetBlockScale());
    d->InputBlockDisplacementCheck->setChecked(pNode->GetBlockDisplacementCheck());

    // Contour Parameters
    d->InputContourNumber->setValue(pNode->GetContourNumber());
    d->InputContourRange->setMaximumValue(pNode->GetContourMax());
    d->InputContourRange->setMinimumValue(pNode->GetContourMin());

    // Glyph Slice Parameters
    if (pNode->GetGlyphSliceNodeID()){
      d->GlyphSliceComboBox->setCurrentNode(pNode->GetGlyphSliceNodeID());
    }
    else{
      this->setGlyphSliceNode(d->GlyphSliceComboBox->currentNode());
    }  
    d->InputGlyphSlicePointMax->setValue(pNode->GetGlyphSlicePointMax());
    d->InputGlyphSliceThreshold->setMaximumValue(pNode->GetGlyphSliceThresholdMax());
    d->InputGlyphSliceThreshold->setMinimumValue(pNode->GetGlyphSliceThresholdMin());
    d->InputGlyphSliceScale->setValue(pNode->GetGlyphSliceScale());
    d->InputGlyphSliceSeed->setValue(pNode->GetGlyphSliceSeed());

    // Grid Slice Parameters
    if (pNode->GetGridSliceNodeID()){
      d->GridSliceComboBox->setCurrentNode(pNode->GetGridSliceNodeID());
    }
    else{
      this->setGridSliceNode(d->GridSliceComboBox->currentNode());
    }  
    d->InputGridSliceScale->setValue(pNode->GetGridSliceScale());
    d->InputGridSliceDensity->setValue(pNode->GetGridSliceDensity());
  }
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::inputVolumeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  // TODO: Move into updatefrommrml?
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene() || !node){
    d->ApplyButton->setEnabled(false);
    d->VolumeDisabledLabel->show();
    return;
  }

  d->ApplyButton->setEnabled(true);
  d->VolumeDisabledLabel->hide();  

  pNode->DisableModifiedEventOn();
  pNode->SetAndObserveInputVolumeNodeID(node->GetID());
  pNode->DisableModifiedEventOff();

  double maxNorm = 0;

  // What to do if there is more than one array? Would there be more than one array?
  if (strcmp(node->GetClassName(), "vtkMRMLVectorVolumeNode") == 0){
    d->InputReferenceComboBox->setEnabled(false);
    maxNorm = vtkMRMLVectorVolumeNode::SafeDownCast(node)->GetImageData()->GetPointData()->GetArray(0)->GetMaxNorm();
  }
  else if (strcmp(node->GetClassName(), "vtkMRMLLinearTransformNode") == 0 || 
    strcmp(node->GetClassName(), "vtkMRMLBSplineTransformNode") == 0 ||
    strcmp(node->GetClassName(), "vtkMRMLGridTransformNode") == 0){
    d->InputReferenceComboBox->setEnabled(true);

    vtkSmartPointer<vtkMRMLVolumeNode> referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->mrmlScene()->GetNodeByID(pNode->GetReferenceVolumeNodeID()));
    if (referenceVolumeNode == NULL){return;}

	//TODO: Remake progress dialog and add detail (update progress from actual steps occurring in logic)
    QProgressDialog *convertProgress =  new QProgressDialog(qSlicerApplication::application()->mainWindow());
    convertProgress->setModal(true);
    convertProgress->setMinimumDuration(100); //will matter a bit more after progress dialog is remade
    convertProgress->show();
	convertProgress->setLabelText("Converting transform to vector volume...");
	
    convertProgress->setValue(20);
    d->logic()->GenerateTransformField();
	
	convertProgress->setValue(80);
    maxNorm = d->logic()->GetFieldMaxNorm() + 1;
	
	convertProgress->setValue(100);
	delete convertProgress;
  }

  pNode->SetGlyphThresholdMax(maxNorm);
  d->InputGlyphThreshold->setMaximum(maxNorm);
  d->InputGlyphThreshold->setMaximumValue(maxNorm);

  pNode->SetContourMax(maxNorm);
  d->InputContourRange->setMaximum(maxNorm);
  d->InputContourRange->setMaximumValue(maxNorm);

  pNode->SetGlyphSliceThresholdMax(maxNorm);
  d->InputGlyphSliceThreshold->setMaximum(maxNorm);
  d->InputGlyphSliceThreshold->setMaximumValue(maxNorm);    
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::referenceVolumeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  //TODO: Move into updatefrommrml?
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene() || !node){return;}

  pNode->DisableModifiedEventOn();
  pNode->SetAndObserveReferenceVolumeNodeID(node->GetID());
  pNode->DisableModifiedEventOff();

  double maxNorm = 0;

  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNode = vtkMRMLTransformNode::SafeDownCast(this->mrmlScene()->GetNodeByID(pNode->GetInputVolumeNodeID()));
  if (inputVolumeNode == NULL){return;}

  if (strcmp(inputVolumeNode->GetClassName(), "vtkMRMLLinearTransformNode") == 0 || 
    strcmp(inputVolumeNode->GetClassName(), "vtkMRMLBSplineTransformNode") == 0 ||
    strcmp(inputVolumeNode->GetClassName(), "vtkMRMLGridTransformNode") == 0){
	
	//TODO: Remake progress dialog and add detail (update progress from actual steps occurring in logic)
    QProgressDialog *convertProgress =  new QProgressDialog(qSlicerApplication::application()->mainWindow());
    convertProgress->setModal(true);
    convertProgress->setMinimumDuration(100); //will matter a bit more after progress dialog is remade
    convertProgress->show();
	convertProgress->setLabelText("Converting transform to vector volume...");
	
	convertProgress->setValue(20);
    d->logic()->GenerateTransformField();
	
	convertProgress->setValue(80);
    maxNorm = d->logic()->GetFieldMaxNorm() + 1;
	
	convertProgress->setValue(100);
	delete convertProgress;
  }

  pNode->SetGlyphThresholdMax(maxNorm);
  d->InputGlyphThreshold->setMaximum(maxNorm);
  d->InputGlyphThreshold->setMaximumValue(maxNorm);

  pNode->SetContourMax(maxNorm);
  d->InputContourRange->setMaximum(maxNorm);
  d->InputContourRange->setMaximumValue(maxNorm);

  pNode->SetGlyphSliceThresholdMax(maxNorm);
  d->InputGlyphSliceThreshold->setMaximum(maxNorm);
  d->InputGlyphSliceThreshold->setMaximumValue(maxNorm);    
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::outputModelChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  //TODO: Move into updatefrommrml?
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene() || !node){
    d->ApplyButton->setEnabled(false);
    d->ModelDisabledLabel->show();
    return;
  }

  d->ApplyButton->setEnabled(true);
  d->ModelDisabledLabel->hide();

  pNode->DisableModifiedEventOn();
  pNode->SetAndObserveOutputModelNodeID(node->GetID());
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::updateSourceOptions(int option)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();

  //TODO: Move into updatefrommrml?
  if (option == 0){
    d->ArrowSourceOptions->setEnabled(true);
    d->ArrowSourceOptions->setVisible(true);
    d->ConeSourceOptions->setEnabled(false);
    d->ConeSourceOptions->setVisible(false);
    d->SphereSourceOptions->setEnabled(false);
    d->SphereSourceOptions->setVisible(false);

    if (!pNode || !this->mrmlScene()){return;}
    pNode->DisableModifiedEventOn();
    pNode->SetGlyphScaleDirectional(true);
    pNode->SetGlyphScaleIsotropic(false);
    pNode->DisableModifiedEventOff();
  }
  else if (option == 1){
    d->ArrowSourceOptions->setEnabled(false);
    d->ArrowSourceOptions->setVisible(false);
    d->ConeSourceOptions->setEnabled(true);
    d->ConeSourceOptions->setVisible(true);
    d->SphereSourceOptions->setEnabled(false);
    d->SphereSourceOptions->setVisible(false);  

    if (!pNode || !this->mrmlScene()){return;}
    pNode->DisableModifiedEventOn();
    pNode->SetGlyphScaleDirectional(true);
    pNode->SetGlyphScaleIsotropic(false);
    pNode->DisableModifiedEventOff();
  }
  else if (option == 2){
    d->ArrowSourceOptions->setEnabled(false);
    d->ArrowSourceOptions->setVisible(false);
    d->ConeSourceOptions->setEnabled(false);
    d->ConeSourceOptions->setVisible(false);
    d->SphereSourceOptions->setEnabled(true);
    d->SphereSourceOptions->setVisible(true);

    if (!pNode || !this->mrmlScene()){return;}
    pNode->DisableModifiedEventOn();
    pNode->SetGlyphScaleDirectional(false);
    pNode->SetGlyphScaleIsotropic(true);
    pNode->DisableModifiedEventOff();
  }

  d->InputGlyphScaleDirectional->setChecked(pNode->GetGlyphScaleDirectional());
  d->InputGlyphScaleIsotropic->setChecked(pNode->GetGlyphScaleIsotropic());
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::visualize()
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  if (d->InputFieldComboBox->currentNodeId() != NULL && d->OutputModelComboBox->currentNodeId() != NULL){
	//TODO: Remake progress dialog and add detail (update progress from actual steps occurring in logic)
    QProgressDialog *visualizeProgress =  new QProgressDialog(qSlicerApplication::application()->mainWindow());
    visualizeProgress->setModal(true);
    visualizeProgress->setMinimumDuration(100); //will matter a bit more after progress dialog is remade
    visualizeProgress->show();
	visualizeProgress->setLabelText("Processing...");
    visualizeProgress->setValue(0);
	
    if (d->GlyphToggle->isChecked()){
		visualizeProgress->setLabelText("Creating glyphs...");
		visualizeProgress->setValue(20);
		d->logic()->CreateVisualization(1);
    }
    else if (d->GridToggle->isChecked()){
		visualizeProgress->setLabelText("Creating grid...");
		visualizeProgress->setValue(20);
		d->logic()->CreateVisualization(2);
    }
    else if (d->ContourToggle->isChecked()){
		visualizeProgress->setLabelText("Creating contours...");
		visualizeProgress->setValue(20);
		d->logic()->CreateVisualization(3);
    }
    else if (d->BlockToggle->isChecked()){
		visualizeProgress->setLabelText("Creating block...");
		visualizeProgress->setValue(20);
		d->logic()->CreateVisualization(4);
    }
    else if (d->GlyphSliceToggle->isChecked()){
		visualizeProgress->setLabelText("Creating glyphs for slice view...");
		visualizeProgress->setValue(20);
		d->logic()->CreateVisualization(5);
    }
    else if (d->GridSliceToggle->isChecked()){
		visualizeProgress->setLabelText("Creating grid for slice view...");
		visualizeProgress->setValue(20);
		d->logic()->CreateVisualization(6);
    }
	visualizeProgress->setValue(100);
	delete visualizeProgress;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setup()
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  connect(d->ParameterComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setDeformationFieldVisualizerParametersNode(vtkMRMLNode*)));

  connect(d->InputFieldComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(inputVolumeChanged(vtkMRMLNode*)));
  connect(d->InputReferenceComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeChanged(vtkMRMLNode*)));
  connect(d->OutputModelComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(outputModelChanged(vtkMRMLNode*)));

  // Glyph Parameters
  connect(d->InputGlyphPointMax, SIGNAL(valueChanged(double)), this, SLOT(setGlyphPointMax(double)));
  connect(d->InputGlyphScale, SIGNAL(valueChanged(double)), this, SLOT(setGlyphScale(double)));
  connect(d->InputGlyphScaleDirectional, SIGNAL(toggled(bool)), this, SLOT(setGlyphScaleDirectional(bool)));
  connect(d->InputGlyphScaleIsotropic, SIGNAL(toggled(bool)), this, SLOT(setGlyphScaleIsotropic(bool)));
  connect(d->InputGlyphThreshold, SIGNAL(valuesChanged(double, double)), this, SLOT(setGlyphThreshold(double, double)));
  connect(d->GenerateSeedButton, SIGNAL(clicked()), this, SLOT(setSeed()));
  connect(d->InputGlyphSeed, SIGNAL(valueChanged(int)), this, SLOT(setGlyphSeed(int)));  
  connect(d->GlyphSourceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setGlyphSourceOption(int)));
  // Arrow Parameters
  connect(d->InputGlyphArrowTipLength, SIGNAL(valueChanged(double)), this, SLOT(setGlyphArrowTipLength(double)));
  connect(d->InputGlyphArrowTipRadius, SIGNAL(valueChanged(double)), this, SLOT(setGlyphArrowTipRadius(double)));
  connect(d->InputGlyphArrowShaftRadius, SIGNAL(valueChanged(double)), this, SLOT(setGlyphArrowShaftRadius(double)));  
  connect(d->InputGlyphArrowResolution, SIGNAL(valueChanged(double)), this, SLOT(setGlyphArrowResolution(double)));
  // Cone Parameters
  connect(d->InputGlyphConeHeight, SIGNAL(valueChanged(double)), this, SLOT(setGlyphConeHeight(double)));
  connect(d->InputGlyphConeRadius, SIGNAL(valueChanged(double)), this, SLOT(setGlyphConeRadius(double)));
  connect(d->InputGlyphConeResolution, SIGNAL(valueChanged(double)), this, SLOT(setGlyphConeResolution(double)));
  // Sphere Parameters
  connect(d->InputGlyphSphereResolution, SIGNAL(valueChanged(double)), this, SLOT(setGlyphSphereResolution(double)));

  // Grid Parameters
  connect(d->InputGridScale, SIGNAL(valueChanged(double)), this, SLOT(setGridScale(double)));
  connect(d->InputGridDensity, SIGNAL(valueChanged(double)), this, SLOT(setGridDensity(double)));

  // Block Parameters  
  connect(d->InputBlockScale, SIGNAL(valueChanged(double)), this, SLOT(setBlockScale(double)));
  connect(d->InputBlockDisplacementCheck, SIGNAL(stateChanged(int)), this, SLOT(setBlockDisplacementCheck(int)));

  // Contour Parameters
  connect(d->InputContourNumber, SIGNAL(valueChanged(double)), this, SLOT(setContourNumber(double)));
  connect(d->InputContourRange, SIGNAL(valuesChanged(double, double)), this, SLOT(setContourRange(double, double)));

  // Glyph Slice Parameters
  connect(d->GlyphSliceComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setGlyphSliceNode(vtkMRMLNode*)));
  connect(d->InputGlyphSlicePointMax, SIGNAL(valueChanged(double)), this, SLOT(setGlyphSlicePointMax(double)));  
  connect(d->InputGlyphSliceThreshold, SIGNAL(valuesChanged(double, double)), this, SLOT(setGlyphSliceThreshold(double, double)));
  connect(d->InputGlyphSliceScale, SIGNAL(valueChanged(double)), this, SLOT(setGlyphSliceScale(double)));
  connect(d->InputGlyphSliceSeed, SIGNAL(valueChanged(int)), this, SLOT(setGlyphSliceSeed(int)));  
  connect(d->GenerateSeedButton2, SIGNAL(clicked()), this, SLOT(setSeed2()));

  // Grid Slice Parameters
  connect(d->GridSliceComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setGridSliceNode(vtkMRMLNode*)));
  connect(d->InputGridSliceScale, SIGNAL(valueChanged(double)), this, SLOT(setGridSliceScale(double)));
  connect(d->InputGridSliceDensity, SIGNAL(valueChanged(double)), this, SLOT(setGridSliceDensity(double)));

  connect(d->ApplyButton, SIGNAL(clicked()), this, SLOT(visualize()));
}

//-----------------------------------------------------------------------------
// Glyph parameters
//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphPointMax(double pointMax)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphPointMax(pointMax);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setSeed()
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSeed((RAND_MAX+1)*(long)rand()+rand());
  d->InputGlyphSeed->setValue(pNode->GetGlyphSeed());
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSeed(int seed)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSeed(seed);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphScale(double scale)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphScale(scale);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphScaleDirectional(bool state)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphScaleDirectional(state);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphScaleIsotropic(bool state)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphScaleIsotropic(state);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphThreshold(double min, double max)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphThresholdMin(min);
  pNode->SetGlyphThresholdMax(max);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSourceOption(int option)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSourceOption(option);
  pNode->DisableModifiedEventOff();
  this->updateSourceOptions(option);
}

//-----------------------------------------------------------------------------
// Arrow parameters
//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphArrowTipLength(double length)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphArrowTipLength(length);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphArrowTipRadius(double radius)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphArrowTipRadius(radius);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphArrowShaftRadius(double radius)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphArrowShaftRadius(radius);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphArrowResolution(double resolution)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphArrowResolution(resolution);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
// Cone Parameters
//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphConeHeight(double height)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphConeHeight(height);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphConeRadius(double radius)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphConeRadius(radius);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphConeResolution(double resolution)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphConeResolution(resolution);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
// Sphere Parameters
//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSphereResolution(double resolution)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSphereResolution(resolution);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGridScale(double scale)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGridScale(scale);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGridDensity(double density)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGridDensity(density);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setBlockScale(double scale)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetBlockScale(scale);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setBlockDisplacementCheck(int state)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetBlockDisplacementCheck(state);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setContourNumber(double number)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetContourNumber(number);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setContourRange(double min, double max)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetContourMin(min);
  pNode->SetContourMax(max);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSliceNode(vtkMRMLNode* node)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetAndObserveGlyphSliceNodeID(node->GetID());
  pNode->DisableModifiedEventOff();  
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSlicePointMax(double pointMax)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSlicePointMax(pointMax);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSliceThreshold(double min, double max)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSliceThresholdMin(min);
  pNode->SetGlyphSliceThresholdMax(max);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSliceScale(double scale)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSliceScale(scale);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGlyphSliceSeed(int seed)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSliceSeed(seed);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setSeed2()
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGlyphSliceSeed((RAND_MAX+1)*(long)rand()+rand());
  d->InputGlyphSliceSeed->setValue(pNode->GetGlyphSliceSeed());
  pNode->DisableModifiedEventOff();
}


//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGridSliceNode(vtkMRMLNode* node)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);

  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetAndObserveGridSliceNodeID(node->GetID());
  pNode->DisableModifiedEventOff();  
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGridSliceScale(double scale)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGridSliceScale(scale);
  pNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDeformationFieldVisualizerModuleWidget::setGridSliceDensity(double density)
{
  Q_D(qSlicerDeformationFieldVisualizerModuleWidget);
  vtkMRMLDeformationFieldVisualizerNode* pNode = d->logic()->GetDeformationFieldVisualizerNode();
  if (!pNode || !this->mrmlScene()){return;}
  pNode->DisableModifiedEventOn();
  pNode->SetGridSliceDensity(density);
  pNode->DisableModifiedEventOff();
}
