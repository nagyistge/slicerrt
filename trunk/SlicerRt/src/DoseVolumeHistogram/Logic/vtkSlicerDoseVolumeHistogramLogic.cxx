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

==============================================================================*/

// ModuleTemplate includes
#include "vtkSlicerDoseVolumeHistogramLogic.h"

#include "vtkPolyDataToLabelmapFilter.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLChartNode.h"
#include "vtkMRMLLayoutNode.h"
#include "vtkMRMLChartViewNode.h"
#include "vtkMRMLDoubleArrayNode.h"

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageAccumulate.h>
#include <vtkImageThreshold.h>
#include <vtkImageToImageStencil.h>
#include <vtkDoubleArray.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseVolumeHistogramLogic);

vtkCxxSetObjectMacro(vtkSlicerDoseVolumeHistogramLogic, DoseVolumeNode, vtkMRMLVolumeNode);
vtkCxxSetObjectMacro(vtkSlicerDoseVolumeHistogramLogic, StructureSetModelNode, vtkMRMLModelNode);
vtkCxxSetObjectMacro(vtkSlicerDoseVolumeHistogramLogic, ChartNode, vtkMRMLChartNode);

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic::vtkSlicerDoseVolumeHistogramLogic()
{
  this->DoseVolumeNode = NULL;
  this->StructureSetModelNode = NULL;
  this->ChartNode = NULL;

  this->CurrentLabelValue = 2;
}

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic::~vtkSlicerDoseVolumeHistogramLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------

vtkMRMLVolumeNode* vtkSlicerDoseVolumeHistogramLogic
::GetLabelmapVolumeNodeForSelectedStructureSet()
{
  std::string labelmapNodeName( this->StructureSetModelNode->GetName() );
  labelmapNodeName.append( "_Labelmap" );

  vtkCollection* labelmapNodeCollection = this->GetMRMLScene()->GetNodesByName( labelmapNodeName.c_str() );

  if (labelmapNodeCollection->GetNumberOfItems() > 1)
  {
    std::cerr << "Error: More nodes found with the name " << labelmapNodeName << std::endl;
    return NULL;
  }
  
  if (labelmapNodeCollection->GetNumberOfItems() == 1)
  {
    vtkMRMLVolumeNode* labelmapNode = dynamic_cast<vtkMRMLVolumeNode*>( labelmapNodeCollection->GetItemAsObject(0) );
    if (labelmapNode)
    {
      return labelmapNode;
    }
  }

  // Create a node by converting the structure set model poly data to labelmap
  vtkNew<vtkMatrix4x4> doseRasToIjkTransformMatrix;
  this->DoseVolumeNode->GetRASToIJKMatrix( doseRasToIjkTransformMatrix.GetPointer() );

  vtkNew<vtkTransform> doseRasToIjkTransform;
  doseRasToIjkTransform->SetMatrix( doseRasToIjkTransformMatrix.GetPointer() );

  vtkNew<vtkTransformPolyDataFilter> transformPolyDataFilter;
  transformPolyDataFilter->SetInput( this->StructureSetModelNode->GetPolyData() );
  transformPolyDataFilter->SetTransform( doseRasToIjkTransform.GetPointer() );

  vtkNew<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter;
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataFilter->GetOutput() );
  polyDataToLabelmapFilter->SetReferenceImage( this->DoseVolumeNode->GetImageData() );
  polyDataToLabelmapFilter->SetLabelValue( this->CurrentLabelValue++ );
  polyDataToLabelmapFilter->Update();

  vtkMRMLScalarVolumeNode* structureSetLabelmapVolumeNode = vtkMRMLScalarVolumeNode::New();
  structureSetLabelmapVolumeNode->CopyOrientation( this->DoseVolumeNode );
  structureSetLabelmapVolumeNode->SetName( labelmapNodeName.c_str() );
  structureSetLabelmapVolumeNode->SetAndObserveImageData( polyDataToLabelmapFilter->GetOutput() );
  structureSetLabelmapVolumeNode->LabelMapOn();

  this->GetMRMLScene()->AddNode( structureSetLabelmapVolumeNode );

  return structureSetLabelmapVolumeNode;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeStatistics(std::vector<double> &indices, std::vector<double> &counts, std::vector<double> &meanDoses, std::vector<double> &totalVolumeCCs, std::vector<double> &maxDoses, std::vector<double> &minDoses)
{
  indices.clear();
  counts.clear();
  meanDoses.clear();
  totalVolumeCCs.clear();
  maxDoses.clear();
  minDoses.clear();

  vtkMRMLVolumeNode* structureSetLabelmapVolumeNode = GetLabelmapVolumeNodeForSelectedStructureSet();
  double* structureSetLabelmapSpacing = structureSetLabelmapVolumeNode->GetSpacing();

  double cubicMMPerVoxel = structureSetLabelmapSpacing[0] * structureSetLabelmapSpacing[1] * structureSetLabelmapSpacing[2];
  double ccPerCubicMM = 0.001;

  vtkNew<vtkImageAccumulate> stataccum;
  stataccum->SetInput( structureSetLabelmapVolumeNode->GetImageData() );
  stataccum->Update();

  int lo = (int)(stataccum->GetMin()[0]);
  // don't compute DVH the background (voxels)
  if (lo == 0)
  {
    lo = 1;
  }
  int hi = (int)(stataccum->GetMax()[0]);

  // prevent long computations for non-labelmap images
  if (hi-lo > 50)
  {
    return;
  }

  for (int i=lo; i<=hi; ++i)
  {
    // logic copied from slicer3 DoseVolumeHistogram to create the binary volume of the label
    vtkNew<vtkImageThreshold> thresholder;
    thresholder->SetInput(structureSetLabelmapVolumeNode->GetImageData());
    thresholder->SetInValue(1);
    thresholder->SetOutValue(0);
    thresholder->ReplaceOutOn();
    thresholder->ThresholdBetween(i,i);
    thresholder->SetOutputScalarType(this->DoseVolumeNode->GetImageData()->GetScalarType());
    thresholder->Update();

    // use vtk's statistics class with the binary labelmap as a stencil
    vtkNew<vtkImageToImageStencil> stencil;
    stencil->SetInput(thresholder->GetOutput());
    stencil->ThresholdBetween(1,1);

    vtkNew<vtkImageAccumulate> stat;
    stat->SetInput(this->DoseVolumeNode->GetImageData());
    stat->SetStencil(stencil->GetOutput());
    stat->Update();

    if (stat->GetVoxelCount() > 0)
    {
      // add an entry to each list
      indices.push_back(i);
      counts.push_back( stat->GetVoxelCount() );
      meanDoses.push_back( stat->GetMean()[0] );
      totalVolumeCCs.push_back( stat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM );
      maxDoses.push_back( stat->GetMax()[0] );
      minDoses.push_back( stat->GetMin()[0] );
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::AddDvhToSelectedChart()
{
  vtkCollection* layoutNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLLayoutNode");
  layoutNodes->InitTraversal();
  vtkObject* layoutNodeVtkObject = layoutNodes->GetNextItemAsObject();
  vtkMRMLLayoutNode* layoutNode = dynamic_cast<vtkMRMLLayoutNode*>(layoutNodeVtkObject);
  if (layoutNode == NULL)
  {
    std::cerr << "Error: unable to get layout node!" << std::endl;
    return;
  }
  layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView );
  
  vtkCollection* chartViewNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLChartViewNode");
  chartViewNodes->InitTraversal();
  vtkObject* chartViewNodeVtkObject = chartViewNodes->GetNextItemAsObject();
  vtkMRMLChartViewNode* chartViewNode = dynamic_cast<vtkMRMLChartViewNode*>(chartViewNodeVtkObject);
  if (chartViewNode == NULL)
  {
    std::cerr << "Error: unable to get chart view node!" << std::endl;
    return;
  }

  vtkMRMLVolumeNode* structureSetLabelmapVolumeNode = GetLabelmapVolumeNodeForSelectedStructureSet();

  vtkNew<vtkImageAccumulate> stataccum;
  stataccum->SetInput( structureSetLabelmapVolumeNode->GetImageData() );
  stataccum->Update();

  int lo = (int)(stataccum->GetMin()[0]);
  // don't compute DVH the background (voxels)
  if (lo == 0)
  {
    lo = 1;
  }
  int hi = (int)(stataccum->GetMax()[0]);

  vtkMRMLChartNode* chartNode = (vtkMRMLChartNode*)( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLChartNode") );
  chartNode = (vtkMRMLChartNode*)( this->GetMRMLScene()->AddNode( chartNode ) );

  for (int i=lo; i<=hi; ++i)
  {
    // logic copied from slicer3 DoseVolumeHistogram to create the binary volume of the label
    vtkNew<vtkImageThreshold> thresholder;
    thresholder->SetInput(structureSetLabelmapVolumeNode->GetImageData());
    thresholder->SetInValue(1);
    thresholder->SetOutValue(0);
    thresholder->ReplaceOutOn();
    thresholder->ThresholdBetween(i,i);
    thresholder->SetOutputScalarType(this->DoseVolumeNode->GetImageData()->GetScalarType());
    thresholder->Update();

    // use vtk's statistics class with the binary labelmap as a stencil
    vtkNew<vtkImageToImageStencil> stencil;
    stencil->SetInput(thresholder->GetOutput());
    stencil->ThresholdBetween(1,1);

    vtkNew<vtkImageAccumulate> stat;
    stat->SetInput(this->DoseVolumeNode->GetImageData());
    stat->SetStencil(stencil->GetOutput());
    stat->Update();

    int numBins = 100;
    double rangeMin = stat->GetMin()[0];
    double rangeMax = stat->GetMax()[0];
    double spacing = (rangeMax - rangeMin) / (double)numBins;

    stat->SetComponentExtent(0,numBins-1,0,0,0,0);
    stat->SetComponentOrigin(rangeMin,0,0);
    stat->SetComponentSpacing(spacing,1,1);
    stat->Update();

    // TODO set stat object as member?

    if (stat->GetVoxelCount() > 0)
    {
      vtkMRMLDoubleArrayNode* arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLDoubleArrayNode") );
      arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->AddNode( arrayNode ) );
      vtkDoubleArray* doubleArray = arrayNode->GetArray();
      doubleArray->SetNumberOfTuples( numBins+1 ); // +1 because there is a fixed point at (0.0, 1.0)

      unsigned long totalVoxels = stat->GetVoxelCount();
      unsigned long voxelBelowDose = 0;

      char numberBuffer[16];
      itoa(i, numberBuffer, 10);
      std::string arrayName("DVH ");
      arrayName.append( numberBuffer );

      doubleArray->SetComponent(0, 0, 0.0);
      doubleArray->SetComponent(0, 1, 1.0);
      doubleArray->SetComponent(0, 2, 0);

      for (int sampleIndex=0; sampleIndex<numBins; ++sampleIndex)
      {
        unsigned long voxelsInBin = stat->GetOutput()->GetScalarComponentAsDouble(sampleIndex,0,0,0);

        doubleArray->SetComponent(sampleIndex+1, 0, rangeMin+sampleIndex*spacing);
        doubleArray->SetComponent(sampleIndex+1, 1, 1.0-voxelBelowDose/totalVoxels);
        doubleArray->SetComponent(sampleIndex+1, 2, 0);
        voxelBelowDose += voxelsInBin;
      }

      chartNode->AddArray( arrayName.c_str(), arrayNode->GetID() );
    }
  }

  chartViewNode->SetChartNodeID( chartNode->GetID() ); // TODO: Is it needed here?

  chartNode->SetProperty("default", "title", "DVH");
  chartNode->SetProperty("default", "xAxisLabel", "Dose"); // TODO: compute in Gy
  chartNode->SetProperty("default", "yAxisLabel", "Fractional volume");
  chartNode->SetProperty("default", "type", "Line");

  chartViewNode->SetChartNodeID( chartNode->GetID() );
}
