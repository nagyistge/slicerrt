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

  This file was originally developed by Kevin Wang, RMP, PMH
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care
  
==============================================================================*/

// Qt includes
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "qSlicerIsodoseModuleWidget.h"
#include "ui_qSlicerIsodoseModule.h"

// qMRMLWidget includes
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDWidget.h"

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>

// STD includes
#include <sstream>
#include <string>


//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Isodose
class qSlicerIsodoseModuleWidgetPrivate: public Ui_qSlicerIsodoseModule
{
  Q_DECLARE_PUBLIC(qSlicerIsodoseModuleWidget);
protected:
  qSlicerIsodoseModuleWidget* const q_ptr;
public:
  qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget &object);
  ~qSlicerIsodoseModuleWidgetPrivate();
  vtkSlicerIsodoseModuleLogic* logic() const;
  void setDefaultColorNode();

  vtkScalarBarWidget* ScalarBarWidget;
};

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::qSlicerIsodoseModuleWidgetPrivate(qSlicerIsodoseModuleWidget& object)
  : q_ptr(&object)
{
  this->ScalarBarWidget = vtkScalarBarWidget::New();
  this->ScalarBarWidget->GetScalarBarActor()->SetOrientationToVertical();
  this->ScalarBarWidget->GetScalarBarActor()->SetNumberOfLabels(11);
  this->ScalarBarWidget->GetScalarBarActor()->SetTitle("(mm)");
  this->ScalarBarWidget->GetScalarBarActor()->SetLabelFormat(" %#8.3f");
  
  // it's a 2d actor, position it in screen space by percentages
  this->ScalarBarWidget->GetScalarBarActor()->SetPosition(0.1, 0.1);
  this->ScalarBarWidget->GetScalarBarActor()->SetWidth(0.1);
  this->ScalarBarWidget->GetScalarBarActor()->SetHeight(0.8);
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidgetPrivate::~qSlicerIsodoseModuleWidgetPrivate()
{
  if (this->ScalarBarWidget)
    {
    this->ScalarBarWidget->Delete();
    this->ScalarBarWidget = 0;
    }
}

//-----------------------------------------------------------------------------
vtkSlicerIsodoseModuleLogic*
qSlicerIsodoseModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerIsodoseModuleWidget);
  return vtkSlicerIsodoseModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerIsodoseModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidget::qSlicerIsodoseModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerIsodoseModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModuleWidget::~qSlicerIsodoseModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerIsodoseModuleWidget);

  this->Superclass::setMRMLScene(scene);

  //qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetIsodoseNode() == 0)
    {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
      {
      this->setIsodoseNode( vtkMRMLIsodoseNode::SafeDownCast(node) );
      }
    d->setDefaultColorNode();
    }
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onEnter()
{
  if (!this->mrmlScene())
    {
    return;
    }

  Q_D(qSlicerIsodoseModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
    {
    return;
    }
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
    {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLIsodoseNode");
    if (node)
      {
      paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);
      d->logic()->SetAndObserveIsodoseNode(paramNode);
      return;
      }
    else 
      {
      vtkSmartPointer<vtkMRMLIsodoseNode> newNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveIsodoseNode(newNode);
      }
    }

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (paramNode->GetDoseVolumeNodeId() && stricmp(paramNode->GetDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNodeId());
    }
    else
    {
      this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }
  }

}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onLogicModified()
{
  Q_D(qSlicerIsodoseModuleWidget);

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidgetPrivate::setDefaultColorNode()
{
  Q_Q(qSlicerIsodoseModuleWidget);
  if (!q->mrmlScene())
  {
    return;
  }
  const char *defaultID = this->logic()->GetDefaultLabelMapColorNodeID();
  vtkMRMLColorTableNode *defaultNode = vtkMRMLColorTableNode::SafeDownCast(
    q->mrmlScene()->GetNodeByID(defaultID));
  this->MRMLColorTableView->setMRMLColorNode(defaultNode);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setup()
{
  Q_D(qSlicerIsodoseModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setIsodoseNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->spinBox_NumberOfLevels, SIGNAL(valueChanged(int)), this, SLOT(setNumberOfLevels(int)));

  connect( d->checkBox_Isoline, SIGNAL(toggled(bool)), this, SLOT( setIsolineVisibility(bool) ) );
  connect( d->checkBox_Isosurface, SIGNAL(toggled(bool)), this, SLOT( setIsosurfaceVisibility(bool) ) );
  connect( d->checkBox_ScalarBar, SIGNAL(toggled(bool)), this, SLOT( setScalarbarVisibility(bool) ) );

  //connect( d->lineEdit_DoseLevels, SIGNAL(textEdited(QString)), this, SLOT(onTextEdited(QString)) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  //connect( d->MRMLNodeComboBox_OutputHierarchy, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( outputHierarchyNodeChanged(vtkMRMLNode*) ) );

  qSlicerApplication * app = qSlicerApplication::application();
  if (app && app->layoutManager())
    {
    qMRMLThreeDView* threeDView = app->layoutManager()->threeDWidget(0)->threeDView();
    vtkRenderer* activeRenderer = app->layoutManager()->activeThreeDRenderer();
    if (activeRenderer)
      {
      d->ScalarBarWidget->SetInteractor(activeRenderer->GetRenderWindow()->GetInteractor());
      }
    //connect(d->VTKScalarBar, SIGNAL(modified()), threeDView, SLOT(scheduleRender()));
    }

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  // Select the default color node
  d->setDefaultColorNode();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsodoseNode(vtkMRMLNode *node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = vtkMRMLIsodoseNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetIsodoseNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveIsodoseNode(paramNode);
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  if (d->logic()->DoseVolumeContainsDose())
  {
    d->label_NotDoseVolumeWarning->setText("");
  }
  else
  {
    d->label_NotDoseVolumeWarning->setText(tr(" Selected volume is not a dose"));
  }

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setNumberOfLevels(int newNumber)
{
  Q_D(qSlicerIsodoseModuleWidget);
  if (!d->spinBox_NumberOfLevels->isEnabled())
  {
    return;
  }
  const char *defaultID = d->logic()->GetDefaultLabelMapColorNodeID();
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(defaultID));
  colorTableNode->SetNumberOfColors(newNumber);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::outputHierarchyNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerIsodoseModuleWidget);

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputHierarchyNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem)
{
  Q_D(qSlicerIsodoseModuleWidget);

}

//-----------------------------------------------------------------------------
QString qSlicerIsodoseModuleWidget::generateNewIsodoseLevel() const
{
  Q_D(const qSlicerIsodoseModuleWidget);

  QString newIsodoseLevelBase("New level");
  QString newIsodoseLevel(newIsodoseLevelBase);
  int i=0;
  //while (d->InspectedNode->GetAttribute(newAttributeName.toLatin1()))
  //  {
  //  newAttributeName = QString("%1%2").arg(newAttributeNameBase).arg(++i);
  //  }
  return newIsodoseLevel;
}

//-----------------------------------------------------------------------------
//void qSlicerIsodoseModuleWidget::addClicked()
//{
//  Q_D(qSlicerIsodoseModuleWidget);
//
//  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
//  if (!paramNode || !this->mrmlScene())
//  {
//    return;
//  }
//
//  int wasModifying = paramNode->StartModify();
//
//  int rowCountBefore = d->tableWidget_DoseLevels->rowCount();
//
//  QString rowCountBeforeString = QString::number(rowCountBefore);
//  d->tableWidget_DoseLevels->insertRow( rowCountBefore );
//
//  // Block signals so that onAttributeChanged function is not called when populating
//  d->tableWidget_DoseLevels->blockSignals(true);
//
//  d->tableWidget_DoseLevels->setItem( rowCountBefore, 0, new QTableWidgetItem( rowCountBeforeString ) );
//  d->tableWidget_DoseLevels->setItem( rowCountBefore, 1, new QTableWidgetItem( rowCountBeforeString ) );
//
//  // Unblock signals
//  d->tableWidget_DoseLevels->blockSignals(false);
//
//  std::vector<DoseLevelStruct>* isodoseLevelVector = paramNode->GetIsodoseLevelVector();
//  DoseLevelStruct tempLevel;
//  tempLevel.DoseLevelName = std::string( rowCountBeforeString.toLatin1() );
//  tempLevel.DoseLevelValue = rowCountBefore;
//  (*isodoseLevelVector).push_back(tempLevel);
//
//  paramNode->Modified();
//  paramNode->EndModify(wasModifying);
//
//  updateButtonsState();
//}

//-----------------------------------------------------------------------------
//void qSlicerIsodoseModuleWidget::removeClicked()
//{
//  Q_D(qSlicerIsodoseModuleWidget);
//
//  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
//  if (!paramNode || !this->mrmlScene())
//  {
//    return;
//  }
//
//  // Extract selected row indices out of the selected table widget items list
//  // (there may be more items selected in a row)
//  QList<QTableWidgetItem*> selectedItems = d->tableWidget_DoseLevels->selectedItems();
//  QSet<int> affectedRowNumbers;
//  foreach (QTableWidgetItem* item, selectedItems)
//  {
//    affectedRowNumbers.insert(item->row());
//  }
//
//  int wasModifying = paramNode->StartModify();
//
//  for (QSet<int>::iterator it = affectedRowNumbers.begin(); it != affectedRowNumbers.end(); ++it)
//  {
//    QString attributeNameToDelete( d->tableWidget_DoseLevels->item((*it), 0)->text() );
//    d->tableWidget_DoseLevels->removeRow((*it));
//    paramNode->GetIsodoseLevelVector()->erase( (*(paramNode->GetIsodoseLevelVector())).begin() + (*it) );
//  }
//
//  paramNode->Modified();
//  paramNode->EndModify(wasModifying);
//
//  updateButtonsState();
//}

////-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::onTextEdited(QString changedString)
{
  Q_D(qSlicerIsodoseModuleWidget);

  // Do nothing if unselected
  if (changedString.isEmpty())
  {
    return;
  }

  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  QStringList list = changedString.split(",");
  if (list.size() >=1) 
  {
    list.sort();
    int wasModifying = paramNode->StartModify();
    paramNode->GetIsodoseLevelVector()->clear();

    for (int i = 0; i<list.size(); i++)
    {
      bool ok;
      double level = list.at(i).toDouble(&ok);
      // Sanity check
      if (ok && level > 0.0 && level < 10000)
      {
        paramNode->GetIsodoseLevelVector()->push_back(level);
      }
    }
    paramNode->Modified();
    paramNode->EndModify(wasModifying);
  }
  else
  {
    return;
  }

  updateButtonsState();
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsolineVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  vtkMRMLModelHierarchyNode* modelHierarchyNode = NULL;
  modelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetOutputHierarchyNodeId()));
  if(!modelHierarchyNode)
  {
    return;
  }

  vtkMRMLModelNode* modelNode;
  vtkCollection* collection = vtkCollection::New();
  modelHierarchyNode->GetChildrenModelNodes(collection);
  vtkCollectionSimpleIterator it;
  int i = 0;
  for (collection->InitTraversal(it); (modelNode = vtkMRMLModelNode::SafeDownCast(collection->GetNextItemAsObject(it))) ; ++i)
  {
    modelNode->GetDisplayNode()->SetSliceIntersectionVisibility(visible);
  }
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setIsosurfaceVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);
  vtkMRMLIsodoseNode* paramNode = d->logic()->GetIsodoseNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  vtkMRMLModelHierarchyNode* modelHierarchyNode = NULL;
  modelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetOutputHierarchyNodeId()));
  if(!modelHierarchyNode)
  {
    return;
  }
  
  vtkMRMLModelNode* modelNode;
  vtkCollection* collection = vtkCollection::New();
  modelHierarchyNode->GetChildrenModelNodes(collection);
  vtkCollectionSimpleIterator it;
  int i = 0;
  for (collection->InitTraversal(it); (modelNode = vtkMRMLModelNode::SafeDownCast(collection->GetNextItemAsObject(it))) ; ++i)
  {
    modelNode->GetDisplayNode()->SetVisibility(visible);
  }
}

//------------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::setScalarbarVisibility(bool visible)
{
  Q_D(qSlicerIsodoseModuleWidget);
  //if (!d->MRMLDisplayNode.GetPointer())
  //  {
  //  return;
  //  }
  //d->MRMLDisplayNode->SetVisibility(visible);
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::applyClicked()
{
  Q_D(qSlicerIsodoseModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the iso dose surface for the selected dose volume
  d->logic()->ComputeIsodose();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModuleWidget::updateButtonsState()
{
  Q_D(qSlicerIsodoseModuleWidget);
  if (!this->mrmlScene())
  {
    return;
  }

  const char *defaultID = d->logic()->GetDefaultLabelMapColorNodeID();
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(defaultID));
  bool applyEnabled = d->logic()->GetIsodoseNode()
                   && d->logic()->GetIsodoseNode()->GetDoseVolumeNodeId()
                   && strcmp(d->logic()->GetIsodoseNode()->GetDoseVolumeNodeId(), "")
                   && colorTableNode->GetNumberOfColors() > 0;
  d->pushButton_Apply->setEnabled(applyEnabled);
}