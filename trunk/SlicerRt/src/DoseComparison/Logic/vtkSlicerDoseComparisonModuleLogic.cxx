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

// DoseComparison includes
#include "vtkSlicerDoseComparisonModuleLogic.h"
#include "vtkMRMLDoseComparisonNode.h"

#include "gamma_dose_comparison.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageExport.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseComparisonModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::vtkSlicerDoseComparisonModuleLogic()
{
  this->DoseComparisonNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::~vtkSlicerDoseComparisonModuleLogic()
{
  SetAndObserveDoseComparisonNode(NULL); // release the node object to avoid memory leaks
}

//----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::SetAndObserveDoseComparisonNode(vtkMRMLDoseComparisonNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseComparisonNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerDoseComparisonModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseComparisonNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLDoseComparisonNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDoseComparisonNode");
  if (node)
  {
    paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DoseComparisonNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseComparisonModuleLogic::DoseVolumeContainsDose(vtkMRMLNode* node)
{
  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(node);
  const char* doseUnitName = doseVolumeNode->GetAttribute("DoseUnitName");

  if (doseUnitName != NULL)
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic
::ConvertVtkImageToItkImage(vtkImageData* inVolume, itk::Image<float, 3>::Pointer outVolume)
{
  if ( inVolume == NULL )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - input image is NULL!"); 
    return; 
  }

  if ( outVolume.IsNull() )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - output image is NULL!"); 
    return; 
  }

  // convert vtkImageData to itkImage 
  vtkSmartPointer<vtkImageExport> imageExport = vtkSmartPointer<vtkImageExport>::New(); 
  imageExport->SetInput(inVolume); 
  imageExport->Update(); 

  int extent[6]={0,0,0,0,0,0}; 
  inVolume->GetExtent(extent); 

  itk::Image<float, 3>::SizeType size;
  size[0] = extent[1] - extent[0] + 1;
  size[1] = extent[3] - extent[2] + 1;
  size[2] = extent[5] - extent[4] + 1;

  itk::Image<float, 3>::IndexType start;
  //double* inputOrigin = inVolume->GetOrigin();
  //start[0]=inputOrigin[0];
  //start[1]=inputOrigin[1];
  //start[2]=inputOrigin[2];
  start[0]=0.0;
  start[1]=0.0;
  start[2]=0.0;

  itk::Image<float, 3>::RegionType region;
  region.SetSize(size);
  region.SetIndex(start);
  outVolume->SetRegions(region);

  //double* inputSpacing = inVolume->GetSpacing();
  //outVolume->SetSpacing(inputSpacing);

  try 
  {
    outVolume->Allocate();
  }
  catch(itk::ExceptionObject & err)
  {
    vtkErrorMacro("Failed to allocate memory for the image conversion: " << err.GetDescription() ); 
    return; 
  }

  imageExport->Export( outVolume->GetBufferPointer() ); 
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::ComputeGammaDoseDifference()
{
  // Convert input images to the format Plastimatch can use
  vtkMRMLVolumeNode* referenceDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseComparisonNode->GetReferenceDoseVolumeNodeId()));
  itk::Image<float, 3>::Pointer referenceDoseVolumeItk = itk::Image<float, 3>::New();

  vtkMRMLVolumeNode* compareDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseComparisonNode->GetCompareDoseVolumeNodeId()));
  itk::Image<float, 3>::Pointer compareDoseVolumeItk = itk::Image<float, 3>::New();

  ConvertVtkImageToItkImage(referenceDoseVolumeNode->GetImageData(), referenceDoseVolumeItk);
  ConvertVtkImageToItkImage(compareDoseVolumeNode->GetImageData(), compareDoseVolumeItk);

  // Compute gamma dose volume
  Gamma_dose_comparison gamma;
  gamma.set_reference_image(referenceDoseVolumeItk);
  gamma.set_compare_image(compareDoseVolumeItk);
  gamma.set_spatial_tolerance(this->DoseComparisonNode->GetDtaDistanceToleranceMm());
  gamma.set_dose_difference_tolerance(this->DoseComparisonNode->GetDoseDifferenceTolerancePercent());
  gamma.set_reference_dose(this->DoseComparisonNode->GetReferenceDoseGy());
  //gamma.set_analysis_threshold(this->DoseComparisonNode->GetAnalysisThresholdPercent());
  gamma.set_gamma_max(this->DoseComparisonNode->GetMaximumGamma());

  gamma.run();

  itk::Image<float, 3>::Pointer gammaVolumeItk = gamma.get_gamma_image_itk();

  // Convert output to VTK
  vtkSmartPointer<vtkImageData> gammaVolume = vtkSmartPointer<vtkImageData>::New();

  itk::Image<float, 3>::SpacingType spacing = gammaVolumeItk->GetSpacing();
  //gammaVolume->SetSpacing( spacing[0], spacing[1], spacing[2] );

  itk::Image<float, 3>::PointType origin = gammaVolumeItk->GetOrigin();
  //gammaVolume->SetOrigin( origin[0], origin[1], origin[2] );

  itk::Image<float, 3>::RegionType region = gammaVolumeItk->GetBufferedRegion();
  itk::Image<float, 3>::SizeType imageSize = region.GetSize();
  int extent[6]={0, imageSize[0]-1, 0, imageSize[1]-1, 0, imageSize[2]-1};
  gammaVolume->SetExtent(extent);

  // TODO: Use these instead of the ones from the reference when transition of spacing and origin is solved in Plastimatch
  double* referenceSpacing = referenceDoseVolumeNode->GetSpacing();
  double* referenceOrigin = referenceDoseVolumeNode->GetOrigin();

  gammaVolume->SetScalarType(VTK_FLOAT);
  gammaVolume->SetNumberOfScalarComponents(1);
  gammaVolume->AllocateScalars();

  float* gammaPtr = (float*)gammaVolume->GetScalarPointer();
  itk::ImageRegionIteratorWithIndex< itk::Image<float, 3> > itGammaItk(
    gammaVolumeItk, gammaVolumeItk->GetLargestPossibleRegion() );
  for ( itGammaItk.GoToBegin(); !itGammaItk.IsAtEnd(); ++itGammaItk )
  {
    itk::Image<float, 3>::IndexType i = itGammaItk.GetIndex();
    (*gammaPtr) = gammaVolumeItk->GetPixel(i);
    gammaPtr++;
  }

  vtkMRMLVolumeNode* gammaVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseComparisonNode->GetGammaVolumeNodeId()));

  if (gammaVolumeNode==NULL)
  {
    vtkErrorMacro("gammaVolumeNode is invalid");
    return;
  }

  gammaVolumeNode->CopyOrientation(referenceDoseVolumeNode);
  gammaVolumeNode->SetAndObserveImageData(gammaVolume);
  gammaVolumeNode->SetSpacing(referenceSpacing);
  gammaVolumeNode->SetOrigin(referenceOrigin);

  // Set default colormap to rainbow
  if (gammaVolumeNode->GetVolumeDisplayNode()==NULL)
  {
    // gammaVolumeNode->CreateDefaultDisplayNodes(); unfortunately this is not implemented for Scalar nodes
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> sdisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    sdisplayNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(sdisplayNode);
    gammaVolumeNode->SetAndObserveDisplayNodeID(sdisplayNode->GetID());
  }
  if (gammaVolumeNode->GetVolumeDisplayNode()!=NULL)
  {
    gammaVolumeNode->GetVolumeDisplayNode()->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRed");
  }
  else
  {
    vtkWarningMacro("Display node is not available for gamma volume node. The default color table will be used.");
  }
  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(gammaVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  } 

}
