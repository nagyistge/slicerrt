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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// ExternalBeamPlanning includes
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"
#include "vtkMRMLExternalBeamPlanningNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanHierarchyNode.h"

// SlicerRT includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"

// Plastimatch includes
#include "plm_image.h"
#include "ion_beam.h"
#include "ion_plan.h"
#include "itk_image_save.h"
#include "itk_image_stats.h"
#include "rpl_volume.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSliceCompositeNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkConeSource.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkPiecewiseFunction.h>
#include <vtkProperty.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkVolumeRayCastMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkVolumeRayCastCompositeFunction.h>
#include <vtkVolumeTextureMapper3D.h>
#include <vtkVolumeTextureMapper2D.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageShiftScale.h>
#include <vtkImageExtractComponents.h>
#include <vtkTransform.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerExternalBeamPlanningModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->ExternalBeamPlanningNode = NULL;
  this->DRRImageSize[0] = 256;
  this->DRRImageSize[1] = 256;
}

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::~vtkSlicerExternalBeamPlanningModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetAndObserveExternalBeamPlanningNode(vtkMRMLExternalBeamPlanningNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerExternalBeamPlanningModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLExternalBeamPlanningNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node!");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLExternalBeamPlanningNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
  if (node)
  {
    paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeamTransform(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("UpdateBeamTransform: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();

  // Make sure inputs are initialized
  if (!rtPlanNode || !referenceVolumeNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Inputs are not initialized!")
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
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

  // Make sure inputs are initialized
  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!")
    return;
  }
  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = beamNode->GetIsocenterFiducialNode();
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = beamNode->GetMLCPositionDoubleArrayNode();
  if (!isocenterMarkupsNode || !MLCPositionDoubleArrayNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!")
    return;
  }

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(beamNode->GetGantryAngle());
  transform->RotateY(beamNode->GetCollimatorAngle());
  transform->RotateX(-90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  isocenterMarkupsNode->GetNthFiducialPosition(0,isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();

  vtkMRMLLinearTransformNode *transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(beamModelNode->GetTransformNodeID()) );
  if (transformNode)
  {
    transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeamGeometryModel(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();

  // Make sure inputs are initialized
  if (!rtPlanNode || !referenceVolumeNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!")
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
  if (!beams) 
  {
    return;
  }
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamname) == 0)
      {
        break;
      }
    }
  }

  // Make sure inputs are initialized
  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!")
    return;
  }
  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = beamNode->GetIsocenterFiducialNode();
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = beamNode->GetMLCPositionDoubleArrayNode();
  if (!isocenterMarkupsNode || !MLCPositionDoubleArrayNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!")
    return;
  }

  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();

  vtkSmartPointer<vtkPolyData> beamModelPolyData = this->CreateBeamPolyData(
	  beamNode->GetX1Jaw(),
	  beamNode->GetX2Jaw(), 
	  beamNode->GetY1Jaw(),
	  beamNode->GetY2Jaw(),
	  beamNode->GetMLCPositionDoubleArrayNode()->GetArray());

  beamModelNode->SetAndObservePolyData(beamModelPolyData);
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerExternalBeamPlanningModuleLogic::CreateBeamPolyData(
	double X1, double X2, double Y1, double Y2, vtkDoubleArray* doubleArray)
{
  int n = 4;

  // First we extract the shape of the mlc
  int X2count = X2/10;
  int X1count = X1/10;
  int Y2count = Y2/10;
  int Y1count = Y1/10;
  int numLeavesVisible = X2count - (-X1count); // Calculate the number of leaves visible
  int numPointsEachSide = numLeavesVisible *2;

  double Y2LeavePosition[40];
  double Y1LeavePosition[40];

  // Calculate Y2 first
  for (int i = X2count; i >= -X1count; i--)
  {
    double leafPosition = doubleArray->GetComponent(-(i-20), 1);
    if (-leafPosition>-Y2)
    {
      Y2LeavePosition[-(i-20)] = leafPosition;
    }
    else
    {
      Y2LeavePosition[-(i-20)] = Y2;
    }
  }
  // Calculate Y1 next
  for (int i = X2count; i >= -X1count; i--)
  {
    double leafPosition = doubleArray->GetComponent(-(i-20), 0);
    if (leafPosition<Y1)
    {
      Y1LeavePosition[-(i-20)] = leafPosition;
    }
    else
    {
      Y1LeavePosition[-(i-20)] = Y1;
    }
  }

  // Create beam model
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertPoint(0,0,0,1000);

  int count = 1;
  for (int i = X2count; i > -X1count; i--)
  {
    points->InsertPoint(count,-Y2LeavePosition[-(i-20)]*2, i*10*2, -1000 );
    count ++;
    points->InsertPoint(count,-Y2LeavePosition[-(i-20)]*2, (i-1)*10*2, -1000 );
    count ++;
  }

  for (int i = -X1count; i < X2count; i++)
  {
    points->InsertPoint(count,Y1LeavePosition[-(i-20)]*2, i*10*2, -1000 );
    count ++;
    points->InsertPoint(count,Y1LeavePosition[-(i-20)]*2, (i+1)*10*2, -1000 );
    count ++;
  }

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  for (int i = 1; i <numPointsEachSide; i++)
  {
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(i);
    cellArray->InsertCellPoint(i+1);
  }
  // Add connection between Y2 and Y1
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(numPointsEachSide);
  cellArray->InsertCellPoint(numPointsEachSide+1);
  for (int i = numPointsEachSide+1; i <2*numPointsEachSide; i++)
  {
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(i);
    cellArray->InsertCellPoint(i+1);
  }

  // Add connection between Y2 and Y1
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(2*numPointsEachSide);
  cellArray->InsertCellPoint(1);

  // Add the cap to the bottom
  cellArray->InsertNextCell(2*numPointsEachSide);
  for (int i = 1; i <= 2*numPointsEachSide; i++)
  {
    cellArray->InsertCellPoint(i);
  }

  vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);

  return beamModelPolyData;
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::AddBeam()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("AddBeam: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();
  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = this->ExternalBeamPlanningNode->GetIsocenterFiducialNode();
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = this->ExternalBeamPlanningNode->GetMLCPositionDoubleArrayNode();
  
  // Make sure inputs are initialized
  if (!rtPlanNode || !isocenterMarkupsNode || !MLCPositionDoubleArrayNode)
  {
    vtkErrorMacro("AddBeam: Inputs are not initialized!")
    return;
  }

  // Get RT plan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkPolyData> beamModelPolyData = this->CreateBeamPolyData(
      this->ExternalBeamPlanningNode->GetX1Jaw(),
      this->ExternalBeamPlanningNode->GetX2Jaw(), 
      this->ExternalBeamPlanningNode->GetY1Jaw(),
      this->ExternalBeamPlanningNode->GetY2Jaw(),
      MLCPositionDoubleArrayNode->GetArray());

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(this->ExternalBeamPlanningNode->GetGantryAngle());
  transform->RotateY(this->ExternalBeamPlanningNode->GetCollimatorAngle());
  transform->RotateX(-90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  isocenterMarkupsNode->GetNthFiducialPosition(0,isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  transformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(transformNode));
  transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  
  std::string rtBeamNodeName;
  std::string rtBeamModelNodeName;
  rtBeamNodeName = std::string("RTBeam");
  rtBeamNodeName = this->GetMRMLScene()->GenerateUniqueName(rtBeamNodeName);
  rtBeamModelNodeName = rtBeamNodeName + "_SurfaceModel";

  // Create rtbeam model node
  vtkSmartPointer<vtkMRMLModelNode> RTBeamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  RTBeamModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelNode));
  RTBeamModelNode->SetName(rtBeamModelNodeName.c_str());
  RTBeamModelNode->SetAndObservePolyData(beamModelPolyData);
  RTBeamModelNode->SetAndObserveTransformNodeID(transformNode->GetID());
  RTBeamModelNode->HideFromEditorsOff();

  // Create model node for beam
  vtkSmartPointer<vtkMRMLModelDisplayNode> RTBeamModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  RTBeamModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelDisplayNode));
  RTBeamModelDisplayNode->SetColor(0.0, 1.0, 0.0);
  RTBeamModelDisplayNode->SetOpacity(0.3);

  // Disable backface culling to make the back side of the contour visible as well
  RTBeamModelDisplayNode->SetBackfaceCulling(0);
  RTBeamModelDisplayNode->HideFromEditorsOff();
  RTBeamModelDisplayNode->VisibilityOn(); 
  RTBeamModelNode->SetAndObserveDisplayNodeID(RTBeamModelDisplayNode->GetID());
  RTBeamModelDisplayNode->SliceIntersectionVisibilityOn();  

  // Create rtbeam node
  vtkSmartPointer<vtkMRMLRTBeamNode> RTBeamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  RTBeamNode = vtkMRMLRTBeamNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamNode));
  RTBeamNode->SetBeamName(rtBeamNodeName.c_str());
  RTBeamNode->SetAndObserveBeamModelNodeId(RTBeamModelNode->GetID());
  RTBeamNode->HideFromEditorsOff();

  // Put the RTBeam node in the hierarchy
  rtPlanNode->AddRTBeamNode(RTBeamNode);

  //RTBeamNode->SetBeamName(this->ExternalBeamPlanningNode->GetBeamName());
  this->ExternalBeamPlanningNode->SetBeamName(RTBeamNode->GetBeamName());

  RTBeamNode->SetX1Jaw(this->ExternalBeamPlanningNode->GetX1Jaw());
  RTBeamNode->SetX2Jaw(this->ExternalBeamPlanningNode->GetX2Jaw());
  RTBeamNode->SetY1Jaw(this->ExternalBeamPlanningNode->GetY1Jaw());
  RTBeamNode->SetY2Jaw(this->ExternalBeamPlanningNode->GetY2Jaw());
  RTBeamNode->SetAndObserveMLCPositionDoubleArrayNode(this->ExternalBeamPlanningNode->GetMLCPositionDoubleArrayNode());
  RTBeamNode->SetAndObserveIsocenterFiducialNode(this->ExternalBeamPlanningNode->GetIsocenterFiducialNode());
  // RTBeamNode->SetAndObserveProtonTargetContourNode(this->ExternalBeamPlanningNode->GetProtonTargetContourNode());

  RTBeamNode->SetGantryAngle(this->ExternalBeamPlanningNode->GetGantryAngle());
  RTBeamNode->SetCollimatorAngle(this->ExternalBeamPlanningNode->GetCollimatorAngle());
  RTBeamNode->SetCouchAngle(this->ExternalBeamPlanningNode->GetCouchAngle());

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveBeam(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();
  // Make sure inputs are initialized
  if (!rtPlanNode)
  {
    vtkErrorMacro("RemoveBeam: Inputs are not initialized!")
    return;
  }

  // Get RT plan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  // Find RT beam node in RT plan hierarchy node
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);

  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    vtkWarningMacro("RemoveBeam: Selected RTPlan node has no children contour nodes!");
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
        rtPlanNode->RemoveRTBeamNode(beamNode);
        this->GetMRMLScene()->RemoveNode(beamModelNode);
        this->GetMRMLScene()->RemoveNode(beamModelDisplayNode);
        break;
      }
    }
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateDRR(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLRTPlanNode* rtPlanNode = this->ExternalBeamPlanningNode->GetRtPlanNode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();
  vtkMRMLMarkupsFiducialNode* isocenterMarkupsNode = this->ExternalBeamPlanningNode->GetIsocenterFiducialNode();
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = this->ExternalBeamPlanningNode->GetMLCPositionDoubleArrayNode();

  // Make sure inputs are initialized
  if (!rtPlanNode || !isocenterMarkupsNode || !referenceVolumeNode)// || !MLCPositionDoubleArrayNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModel: Inputs are not initialized!")
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  rtPlanNode->GetRTBeamNodes(beams);
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

  // Cast image data to uchar for faster rendering (this is for CT data only now)
  vtkSmartPointer<vtkImageShiftScale> cast = vtkSmartPointer<vtkImageShiftScale>::New();
  cast->SetInput(referenceVolumeNode->GetImageData());
  cast->SetOutputScalarTypeToUnsignedChar();
  cast->SetShift(1000);
  cast->SetScale(255./2000.);
  cast->SetClampOverflow(1);
  cast->Update();

  // Create the renderer, render window 
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetOffScreenRendering(1);
  renWin->AddRenderer(renderer);

  // Create our volume and mapper
  // Currently use cpu ray casting as gpu ray casting and texturemapping 3d are not working on my computer
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  vtkSmartPointer<vtkVolumeRayCastMapper> mapper = vtkSmartPointer<vtkVolumeRayCastMapper>::New();
  vtkSmartPointer<vtkVolumeRayCastCompositeFunction> compositeFunction = vtkSmartPointer<vtkVolumeRayCastCompositeFunction>::New();
  mapper->SetVolumeRayCastFunction(compositeFunction);
  //vtkSmartPointer<vtkVolumeTextureMapper3D> mapper = vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
  //vtkSmartPointer<vtkVolumeTextureMapper2D> mapper = vtkSmartPointer<vtkVolumeTextureMapper2D>::New();
  //vtkSmartPointer<vtkGPUVolumeRayCastMapper> mapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
  mapper->SetInput( cast->GetOutput() );
  mapper->SetBlendModeToComposite();
  volume->SetMapper( mapper );

  // Create our transfer function
  vtkSmartPointer<vtkColorTransferFunction> colorFun = vtkSmartPointer<vtkColorTransferFunction>::New();
  vtkSmartPointer<vtkPiecewiseFunction> opacityFun = vtkSmartPointer<vtkPiecewiseFunction>::New();

  // Create the property and attach the transfer functions
  vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor( colorFun );
  volumeProperty->SetScalarOpacity( opacityFun );
  volumeProperty->SetInterpolationTypeToLinear();

  colorFun->AddRGBPoint( 0, 0, 0, 0);
  colorFun->AddRGBPoint( 128, 0, 0, 0);
  colorFun->AddRGBPoint( 255, 1, 1, 1);

  opacityFun->AddPoint(0, 0 );
  opacityFun->AddPoint(128, 0 );
  opacityFun->AddPoint(255, 0.1);

  volumeProperty->ShadeOff();
  volumeProperty->SetScalarOpacityUnitDistance(0.8919);

  // connect up the volume to the property and the mapper
  volume->SetProperty( volumeProperty );

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateY(this->ExternalBeamPlanningNode->GetGantryAngle());
  transform->RotateX(90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  isocenterMarkupsNode->GetNthFiducialPosition(0,isoCenterPosition);
  transform2->Translate(-isoCenterPosition[0], -isoCenterPosition[1], -isoCenterPosition[2]);

  vtkSmartPointer<vtkTransform> transform3 = vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkMatrix4x4> IJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceVolumeNode->GetIJKToRASMatrix(IJK2RASMatrix);
  transform3->SetMatrix(IJK2RASMatrix);

  transform->PreMultiply();
  transform->Concatenate(transform2);
  transform->Concatenate(transform3);
  volume->SetUserTransform( transform );

  // The usual rendering stuff.
  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
  // fixed camera parameters for now
  camera->SetPosition(0, 0, 1000);
  camera->SetFocalPoint(0, 0, 0);
  camera->SetViewAngle(14.68);
  camera->ParallelProjectionOff();

  // Add the volume to the scene
  renderer->AddVolume( volume );
  renderer->SetActiveCamera(camera);

  renWin->SetSize(this->DRRImageSize[0], this->DRRImageSize[1]);
  renWin->Render();

  // Capture and convert to 2D image
  vtkSmartPointer<vtkWindowToImageFilter> windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
  windowToImage->SetInput( renWin );

  vtkSmartPointer<vtkImageExtractComponents> extract = vtkSmartPointer<vtkImageExtractComponents>::New();
  extract->SetInputConnection( windowToImage->GetOutputPort() );
  extract->SetComponents(0);
  extract->Update();

  // Add the drr image to mrml scene
  vtkSmartPointer<vtkMRMLScalarVolumeNode> DRRImageNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  DRRImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->AddNode(DRRImageNode));
  std::string DRRImageNodeName = std::string(beamname) + std::string("_DRRImage");
  DRRImageNodeName = this->GetMRMLScene()->GenerateUniqueName(DRRImageNodeName);
  DRRImageNode->SetName(DRRImageNodeName.c_str());
  //DRRImageNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  DRRImageNode->SetAndObserveImageData(extract->GetOutput());
  DRRImageNode->SetOrigin(-128,-128,0);
  DRRImageNode->SetSelectable(1);

  vtkSmartPointer<vtkMRMLSliceLogic> sliceLogic = this->GetApplicationLogic()->GetSliceLogicByLayoutName("Slice4");
  if (!sliceLogic)
  {
    vtkErrorMacro("UpdateDRR: Invalid sliceLogic for DRR viewer!");
    return;
  }

  //  
  vtkSmartPointer<vtkTransform> transformDRR = vtkSmartPointer<vtkTransform>::New();
  transformDRR->Identity();
  transformDRR->RotateZ(this->ExternalBeamPlanningNode->GetGantryAngle());
  transformDRR->RotateX(-90);

  vtkSmartPointer<vtkTransform> transformDRR2 = vtkSmartPointer<vtkTransform>::New();
  transformDRR2->Identity();
  transformDRR2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transformDRR->PostMultiply();
  transformDRR->Concatenate(transformDRR2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> DRRImageTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  DRRImageTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(DRRImageTransformNode));
  std::string DRRImageTransformNodeName = std::string(beamname) + std::string("_DRRImage_Transform");
  DRRImageTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(DRRImageTransformNodeName);
  DRRImageTransformNode->SetName(DRRImageTransformNodeName.c_str());
  DRRImageTransformNode->SetAndObserveMatrixTransformToParent(transformDRR->GetMatrix());
  DRRImageNode->SetAndObserveTransformNodeID(DRRImageTransformNode->GetID());

  vtkSmartPointer<vtkMRMLSliceNode> sliceNode = sliceLogic->GetSliceNode();
  vtkSmartPointer<vtkMRMLSliceCompositeNode> compositeSliceNode = sliceLogic->GetSliceCompositeNode();
  compositeSliceNode->SetBackgroundVolumeID(DRRImageNode->GetID());

  vtkSmartPointer<vtkTransform> transformSlice = vtkSmartPointer<vtkTransform>::New();
  transformSlice->Identity();
  transformSlice->RotateZ(this->ExternalBeamPlanningNode->GetGantryAngle());
  transformSlice->RotateX(-90);
  transformSlice->Update();

  sliceNode->SetOrientationToReformat();
  sliceNode->SetSliceToRAS(transformSlice->GetMatrix());
  sliceNode->UpdateMatrices();

  sliceLogic->FitSliceToAll();
}

//---------------------------------------------------------------------------
template<class T> 
static void 
itk_rectify_volume_hack (T image)
{
  typename T::ObjectType::RegionType rg = image->GetLargestPossibleRegion ();
  typename T::ObjectType::PointType og = image->GetOrigin();
  typename T::ObjectType::SpacingType sp = image->GetSpacing();
  typename T::ObjectType::SizeType sz = rg.GetSize();
  typename T::ObjectType::DirectionType dc = image->GetDirection();

  og[0] = og[0] - (sz[0] - 1) * sp[0];
  og[1] = og[1] - (sz[1] - 1) * sp[1];
  dc[0][0] = 1.;
  dc[1][1] = 1.;

  image->SetOrigin(og);
  image->SetDirection(dc);
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeDose()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("ComputeDose: Invalid MRML scene or parameter set node!");
    return;
  }

  Plm_image::Pointer plmRef = PlmCommon::ConvertVolumeNodeToPlmImage(
    this->ExternalBeamPlanningNode->GetReferenceVolumeNode());
  plmRef->print ();

  vtkMRMLContourNode* targetContourNode = this->ExternalBeamPlanningNode->GetProtonTargetContourNode();
  Plm_image::Pointer plmTgt = PlmCommon::ConvertVolumeNodeToPlmImage(
    targetContourNode->GetIndexedLabelmapVolumeNode());
  plmTgt->print ();

#if defined (commentout)
  double min_val, max_val, avg;
  int non_zero, num_vox;
  itk_image_stats (plmRef->m_itk_int32, &min_val, &max_val, &avg, &non_zero, &num_vox);
  printf ("MIN %f AVE %f MAX %f NONZERO %d NUMVOX %d\n", 
    (float) min_val, (float) avg, (float) max_val, non_zero, num_vox);
#endif

  itk::Image<short, 3>::Pointer referenceVolumeItk = plmRef->itk_short();
  itk::Image<unsigned char, 3>::Pointer targetVolumeItk = plmTgt->itk_uchar();

  // Ray tracing code expects identity direction cosines.  This is a hack.
#if defined (commentout)
  printf ("(cd 5)\n");
  itk_rectify_volume_hack (referenceVolumeItk);
  itk_rectify_volume_hack (targetVolumeItk);
#endif

  Ion_plan ion_plan;

  try
  {
    // Assign inputs to dose calc logic
    printf ("Setting reference volume\n");
    ion_plan.set_patient (referenceVolumeItk);
    printf ("Setting target volume\n");
    ion_plan.set_target (targetVolumeItk);
    printf ("Done.\n");

    printf ("Gantry angle is: %g\n",
            this->ExternalBeamPlanningNode->GetGantryAngle());

    float src_dist = 2000;
    float src[3];
    float isocenter[3] = { 0, 0, 0 };

    /* Adjust src according to gantry angle */
    float ga_radians = 
      this->ExternalBeamPlanningNode->GetGantryAngle() * M_PI / 180.;
    src[0] = - src_dist * sin(ga_radians);
    src[1] = src_dist * cos(ga_radians);
    src[2] = 0.f;

    ion_plan.beam->set_source_position (src);
    ion_plan.beam->set_isocenter_position (isocenter);

    float ap_offset = 1500;
    int ap_dim[2] = { 30, 30 };
//    float ap_origin[2] = { -19, -19 };
    float ap_spacing[2] = { 2, 2 };
    ion_plan.get_aperture()->set_distance (ap_offset);
    ion_plan.get_aperture()->set_dim (ap_dim);
//    ion_plan.get_aperture()->set_origin (ap_origin);
    ion_plan.get_aperture()->set_spacing (ap_spacing);
    ion_plan.set_step_length (1);
    ion_plan.set_smearing (this->ExternalBeamPlanningNode->GetSmearing());
    if (!ion_plan.init ()) {
      /* Failure.  How to notify the user?? */
      std::cerr << "Sorry, ion_plan.init() failed.\n";
      return;
    }

    /* A little warm fuzzy for the developers */
    ion_plan.debug ();
    printf ("Working...\n");
    fflush(stdout);

    /* Compute the aperture and range compensator */
    vtkWarningMacro ("Computing beam modifier\n");
    ion_plan.compute_beam_modifiers ();
    vtkWarningMacro ("Computing beam modifier done!\n");

  }
  catch (std::exception& ex)
  {
    vtkErrorMacro("ComputeDose: Plastimatch exception: " << ex.what());
    return;
  }

  /* Get aperture as itk image */
  Rpl_volume *rpl_vol = ion_plan.rpl_vol;
  Plm_image::Pointer& ap =
    rpl_vol->get_aperture()->get_aperture_image();
  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk =
    ap->itk_uchar();

  /* Get range compensator as itk image */
  Plm_image::Pointer& rc =
    rpl_vol->get_aperture()->get_range_compensator_image();
  itk::Image<float, 3>::Pointer rcVolumeItk =
    rc->itk_float();

  /* Convert range compensator image to vtk */
  vtkSmartPointer<vtkImageData> rcVolume = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(rcVolumeItk, rcVolume, VTK_FLOAT);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> rcVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  rcVolumeNode->SetAndObserveImageData (rcVolume);
  rcVolumeNode->SetSpacing (
    rcVolumeItk->GetSpacing()[0],
    rcVolumeItk->GetSpacing()[1],
    rcVolumeItk->GetSpacing()[2]);
  rcVolumeNode->SetOrigin (
    rcVolumeItk->GetOrigin()[0],
    rcVolumeItk->GetOrigin()[1],
    rcVolumeItk->GetOrigin()[2]);

  std::string rcVolumeNodeName = this->GetMRMLScene()
    ->GenerateUniqueName(std::string ("range_compensator_"));
  rcVolumeNode->SetName(rcVolumeNodeName.c_str());

  rcVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(rcVolumeNode);

  /* Convert aperture image to vtk */
  vtkSmartPointer<vtkImageData> apertureVolume = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<unsigned char>(apertureVolumeItk, apertureVolume, VTK_UNSIGNED_CHAR);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> apertureVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  apertureVolumeNode->SetAndObserveImageData (apertureVolume);
  apertureVolumeNode->SetSpacing (
    apertureVolumeItk->GetSpacing()[0],
    apertureVolumeItk->GetSpacing()[1],
    apertureVolumeItk->GetSpacing()[2]);
  apertureVolumeNode->SetOrigin (
    apertureVolumeItk->GetOrigin()[0],
    apertureVolumeItk->GetOrigin()[1],
    apertureVolumeItk->GetOrigin()[2]);

  std::string apertureVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(std::string ("aperture_"));
  apertureVolumeNode->SetName(apertureVolumeNodeName.c_str());

  apertureVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(apertureVolumeNode);

  /* Compute the dose */
  try
  {
    vtkWarningMacro("ComputeDose: Applying beam modifiers");
    ion_plan.apply_beam_modifiers ();

    vtkWarningMacro ("Optimizing SOBP\n");
    ion_plan.beam->set_proximal_margin (
      this->ExternalBeamPlanningNode->GetProximalMargin());
    ion_plan.beam->set_distal_margin (
      this->ExternalBeamPlanningNode->GetDistalMargin());
    ion_plan.beam->set_sobp_prescription_min_max (
      rpl_vol->get_min_wed(), rpl_vol->get_max_wed());
    ion_plan.beam->optimize_sobp ();

    ion_plan.compute_dose ();
  }
  catch (std::exception& ex)
  {
    vtkErrorMacro("ComputeDose: Plastimatch exception: " << ex.what());
    return;
  }

  /* Get dose as itk image */
  itk::Image<float, 3>::Pointer doseVolumeItk = ion_plan.get_dose_itk();

  /* Convert dose image to vtk */
  vtkSmartPointer<vtkImageData> doseVolume = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(doseVolumeItk, doseVolume, VTK_FLOAT);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  doseVolumeNode->SetAndObserveImageData (doseVolume);
  doseVolumeNode->SetSpacing (
    doseVolumeItk->GetSpacing()[0],
    doseVolumeItk->GetSpacing()[1],
    doseVolumeItk->GetSpacing()[2]);
  doseVolumeNode->SetOrigin (
    doseVolumeItk->GetOrigin()[0],
    doseVolumeItk->GetOrigin()[1],
    doseVolumeItk->GetOrigin()[2]);

  std::string doseVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(std::string("proton_dose_"));
  doseVolumeNode->SetName(doseVolumeNodeName.c_str());

  doseVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(doseVolumeNode);

  /* Testing .. */
  doseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  /* More testing .. */
  if (doseVolumeNode->GetVolumeDisplayNode() == NULL)
  {
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    displayNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(displayNode);
    doseVolumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  }
  if (doseVolumeNode->GetVolumeDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(doseVolumeNode->GetVolumeDisplayNode());
    doseScalarVolumeDisplayNode->SetAutoWindowLevel(0);
    doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, 16.0);

    /* Set colormap to rainbow */
    doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    doseScalarVolumeDisplayNode->SetLowerThreshold (1.0);
    doseScalarVolumeDisplayNode->ApplyThresholdOn ();
  }
  else
  {
    vtkWarningMacro("ComputeDose: Display node is not available for gamma volume node. The default color table will be used.");
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeWED()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    vtkErrorMacro("ComputeWED: Invalid MRML scene or parameter set node!");
    return;
  }

  // Convert input images to ITK format for Plastimatch
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ExternalBeamPlanningNode->GetReferenceVolumeNode();
  //  itk::Image<short, 3>::Pointer referenceVolumeItk = itk::Image<short, 3>::New();
  itk::Image<short, 3>::Pointer referenceVolumeItk = itk::Image<short, 3>::New();

  SlicerRtCommon::ConvertVolumeNodeToItkImage<short>(referenceVolumeNode, referenceVolumeItk);

  // Ray tracing code expects identity direction cosines.  This is a hack.
  itk_rectify_volume_hack (referenceVolumeItk);

  Ion_plan ion_plan;

  try
  {
    // Assign inputs to dose calc logic
    ion_plan.set_patient (referenceVolumeItk);

    vtkDebugMacro("ComputeWED: Gantry angle is: " << this->ExternalBeamPlanningNode->GetGantryAngle());

    float src_dist = 2000;
    float src[3];
    float isocenter[3] = { 0, 0, 0 };

    /* Adjust src according to gantry angle */
    float ga_radians = 
      this->ExternalBeamPlanningNode->GetGantryAngle() * M_PI / 180.;
    src[0] = - src_dist * sin(ga_radians);
    src[1] = src_dist * cos(ga_radians);
    src[2] = 0.f;

    ion_plan.beam->set_source_position (src);
    ion_plan.beam->set_isocenter_position (isocenter);

    float ap_offset = 1500;
    int ap_dim[2] = { 30, 30 };
//    float ap_origin[2] = { -19, -19 };
    float ap_spacing[2] = { 2, 2 };
    ion_plan.get_aperture()->set_distance (ap_offset);
    ion_plan.get_aperture()->set_dim (ap_dim);
//    ion_plan.get_aperture()->set_origin (ap_origin);
    ion_plan.get_aperture()->set_spacing (ap_spacing);
    ion_plan.set_step_length (1);
    if (!ion_plan.init ())
    {
      vtkErrorMacro("SComputeWED: ion_plan.init() failed!");
      return;
    }

    /* A little warm fuzzy for the developers */
    ion_plan.debug ();
    printf ("Working...\n");
    fflush(stdout);
  }
  catch (std::exception& ex)
  {
    vtkErrorMacro("ComputeWED: Plastimatch exception: " << ex.what());
    return;
  }

  // Get wed as itk image 
  Rpl_volume *rpl_vol = ion_plan.rpl_vol;

  Plm_image::Pointer patient = Plm_image::New();
  patient->set_itk (referenceVolumeItk);
  Volume::Pointer patient_vol = patient->get_volume_float();
  // Volume* wed = rpl_vol->create_wed_volume (&ion_plan); //TODO: this line broke the build, needs to be fixed
  Volume* wed = NULL; // Creating dummy variable to ensure compilation
  return; // TODO: remove this return statement once the 

  // Volume* wed = create_wed_volume (0,&ion_plan);

  // Feed in reference volume, as wed output is the warped reference image.
  rpl_vol->compute_wed_volume(wed,patient_vol.get(),-1000);

  Plm_image::Pointer wed_image = Plm_image::New (new Plm_image (wed));

  itk::Image<float, 3>::Pointer wedVolumeItk =
    wed_image->itk_float();

  /* Convert aperture image to vtk */
  vtkSmartPointer<vtkImageData> wedVolume = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(wedVolumeItk, wedVolume, VTK_FLOAT);

  /* Create the MRML node for the volume */
  vtkSmartPointer<vtkMRMLScalarVolumeNode> wedVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

  wedVolumeNode->SetAndObserveImageData (wedVolume);
  wedVolumeNode->SetSpacing (
    wedVolumeItk->GetSpacing()[0],
    wedVolumeItk->GetSpacing()[1],
    wedVolumeItk->GetSpacing()[2]);
  wedVolumeNode->SetOrigin (
    wedVolumeItk->GetOrigin()[0],
    wedVolumeItk->GetOrigin()[1],
    wedVolumeItk->GetOrigin()[2]);

  std::string wedVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(std::string ("wed_"));
  wedVolumeNode->SetName(wedVolumeNodeName.c_str());

  wedVolumeNode->SetScene(this->GetMRMLScene());
  this->GetMRMLScene()->AddNode(wedVolumeNode);
}