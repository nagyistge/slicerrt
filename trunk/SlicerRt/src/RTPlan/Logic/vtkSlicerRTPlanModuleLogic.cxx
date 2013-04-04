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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// RTPlan Logic includes
#include "vtkSlicerRTPlanModuleLogic.h"
#include "vtkMRMLRTPlanModuleNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanHierarchyNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLContourNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageMarchingCubes.h>
#include <vtkImageChangeInformation.h>
#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>
#include <vtkLookupTable.h>
#include <vtkColorTransferFunction.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkImageContinuousDilate3D.h>
#include <vtkImageContinuousErode3D.h>
#include <vtkImageLogic.h>
#include <vtkImageAccumulate.h>
#include <vtkImageReslice.h>
#include <vtkGeneralTransform.h>
#include <vtkConeSource.h>
#include <vtkTransformPolyDataFilter.h>

// STD includes
#include <cassert>

#define THRESHOLD 0.001

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerRTPlanModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerRTPlanModuleLogic::vtkSlicerRTPlanModuleLogic()
{
  this->RTPlanModuleNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerRTPlanModuleLogic::~vtkSlicerRTPlanModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->RTPlanModuleNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::SetAndObserveRTPlanModuleNode(vtkMRMLRTPlanModuleNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->RTPlanModuleNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanModuleNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->RTPlanModuleNode)
    {
    return;
    }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLRTPlanModuleNode"))
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->RTPlanModuleNode)
    {
    return;
    }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLRTPlanModuleNode"))
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLRTPlanModuleNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanModuleNode");
  if (node)
    {
    paramNode = vtkMRMLRTPlanModuleNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->RTPlanModuleNode, paramNode);
    }
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::CreateBeamPolyData()
{
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::UpdateBeam(char *beamname, double gantryAngle)
{
  if ( !this->GetMRMLScene() || !this->RTPlanModuleNode )
  {
    return;
  }
  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetReferenceVolumeNodeID()));
  vtkMRMLAnnotationFiducialNode* ISOCenterFiducialNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetISOCenterNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode || !ISOCenterFiducialNode )
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  RTPlanNode->GetRTBeamNodes(beams);
  // Fill the table
  if (!beams) return;
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamname) == 0)
      {
        //RTPlanNode->RemoveRTBeamNode(beamNode);
        break;
      }
    }
  }

  //this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  //vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
  //double baseRadius = 2;
  //coneSource->SetRadius(baseRadius*2.0*sqrt(2.0));
  //coneSource->SetHeight(100*2.0);
  //coneSource->SetResolution(4);
  //coneSource->Update();

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(gantryAngle);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  ISOCenterFiducialNode->GetFiducialCoordinates(isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  //vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  //transformFilter->SetInput(coneSource->GetOutput());
  //transformFilter->SetTransform(transform);
  //transformFilter->Update();
  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();

  //beamNode->SetAndObservePolyData(transformFilter->GetOutput());
  vtkMRMLLinearTransformNode *transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
             this->GetMRMLScene()->GetNodeByID(beamModelNode->GetTransformNodeID()));
  if (transformNode)
  {
    transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  }
 // this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::AddBeam()
{
  if ( !this->GetMRMLScene() || !this->RTPlanModuleNode )
  {
    return;
  }

  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetReferenceVolumeNodeID()));
  vtkMRMLAnnotationFiducialNode* ISOCenterFiducialNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetISOCenterNodeID()));
  // Make sure inputs are initialized
  // if (!referenceVolumeNode || !RTPlanNode)
  if (!RTPlanNode || !ISOCenterFiducialNode )
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  // get rtplan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  // Create root contour hierarchy node for the series, if it has not been created yet
  //if (RTPlanHierarchyRootNode.GetPointer()==NULL)
  //{
  //  RTPlanHierarchyRootNode = vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New();
  //  RTPlanHierarchyRootNode->SetName(RTPlanHierarchyRootNodeName.c_str());
  //  RTPlanHierarchyRootNode->AllowMultipleChildrenOn();
  //  RTPlanHierarchyRootNode->HideFromEditorsOff();
  //  this->GetMRMLScene()->AddNode(RTPlanHierarchyRootNode);
  //  RTPlanHierarchyRootNode->SetAssociatedNodeID( RTPlanNode->GetID() );

  //   A hierarchy node needs a display node
  //  vtkSmartPointer<vtkMRMLDisplayNode> RTPlanHierarchyRootDisplayNode = vtkSmartPointer<vtkMRMLDisplayNode>::New();
  //  RTPlanHierarchyRootDisplayNodeName.append("Display");
  //  RTPlanHierarchyRootDisplayNode->SetName(RTPlanHierarchyRootDisplayNodeName.c_str());
  //  RTPlanHierarchyRootDisplayNode->SetVisibility(1);
  //  this->GetMRMLScene()->AddNode(RTPlanHierarchyRootDisplayNode);
  //  RTPlanHierarchyRootNode->SetAndObserveDisplayNodeID( RTPlanHierarchyRootDisplayNode->GetID() );
  //}

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
  double baseRadius = 2;
  coneSource->SetRadius(baseRadius*2.0*sqrt(2.0));
  coneSource->SetHeight(100*2.0);
  coneSource->SetResolution(4);
  coneSource->SetDirection(0,1,0);
  coneSource->Update();

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(0);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  ISOCenterFiducialNode->GetFiducialCoordinates(isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  transformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(transformNode));
  transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  
  // Create rtbeam model node
  vtkSmartPointer<vtkMRMLModelNode> RTBeamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  RTBeamModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelNode));
  //RTBeamNode->SetBeamName("BeamName");
  RTBeamModelNode->SetAndObservePolyData(coneSource->GetOutput());
  RTBeamModelNode->SetAndObserveTransformNodeID(transformNode->GetID());
  RTBeamModelNode->HideFromEditorsOff();

  // create model node for beam
  vtkSmartPointer<vtkMRMLModelDisplayNode> RTBeamModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  RTBeamModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelDisplayNode));
  //RTBeamModelDisplayNode->SetName(RTBeamModelDisplayNodeName.c_str());
  RTBeamModelDisplayNode->SetColor(0.0, 1.0, 0.0);
  RTBeamModelDisplayNode->SetOpacity(0.3);
  // Disable backface culling to make the back side of the contour visible as well
  RTBeamModelDisplayNode->SetBackfaceCulling(0);
  RTBeamModelDisplayNode->HideFromEditorsOff();
  RTBeamModelDisplayNode->VisibilityOn(); 
  //RTBeamNode->SetAndObserveDisplayNodeID(RTBeamModelDisplayNode->GetID());
  RTBeamModelNode->SetAndObserveDisplayNodeID(RTBeamModelDisplayNode->GetID());
  RTBeamModelDisplayNode->SliceIntersectionVisibilityOn();  

  // Create rtbeam node
  vtkSmartPointer<vtkMRMLRTBeamNode> RTBeamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  RTBeamNode = vtkMRMLRTBeamNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamNode));
  RTBeamNode->SetBeamName("BeamName");
  RTBeamNode->SetAndObserveBeamModelNodeId(RTBeamModelNode->GetID());
  RTBeamNode->HideFromEditorsOff();

  //// Put the RTBeam node in the hierarchy
  RTPlanNode->AddRTBeamNode(RTBeamNode);

  //vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyNode = vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New();
  //std::string RTPlanHierarchyNode(contourNodeName);
  ////RTPlanHierarchyNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
  //RTPlanHierarchyNode = this->GetMRMLScene()->GenerateUniqueName(phContourNodeName);
  //RTPlanHierarchyNode->SetName(phContourNodeName.c_str());
  //RTPlanHierarchyNode->SetParentNodeID( RTPlanHierarchyRootNode->GetID() );
  //RTPlanHierarchyNode->SetAssociatedNodeID( RTBeamNode->GetID() );
  //RTPlanHierarchyNode->HideFromEditorsOff();
  //this->GetMRMLScene()->AddNode(RTPlanHierarchyNode);

  //displayNodeCollection->AddItem( vtkMRMLModelNode::SafeDownCast(addedDisplayableNode)->GetModelDisplayNode() );

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerRTPlanModuleLogic::RemoveBeam(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->RTPlanModuleNode )
  {
    return;
  }

  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->RTPlanModuleNode->GetReferenceVolumeNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode)
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  // get rtplan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  // find rtbeam node in rtplan hierarchy node

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  RTPlanNode->GetRTBeamNodes(beams);

  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    std::cerr << "Warning: Selected RTPlan node has no children contour nodes!" << std::endl;
    return;
  }
  // Fill the table
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamname) == 0)
      {
        vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();
        vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelDisplayNode = beamModelNode->GetModelDisplayNode();
        RTPlanNode->RemoveRTBeamNode(beamNode);
        this->GetMRMLScene()->RemoveNode(beamModelNode);
        this->GetMRMLScene()->RemoveNode(beamModelDisplayNode);
        break;
      }
    }
  }

  this->Modified();
}
