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

// .NAME vtkSlicerDoseVolumeHistogramLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseVolumeHistogramLogic_h
#define __vtkSlicerDoseVolumeHistogramLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// VTK includes
#include "vtkImageAccumulate.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLChartNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLChartViewNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_MODULE_LOGIC_EXPORT vtkSlicerDoseVolumeHistogramLogic :
  public vtkSlicerModuleLogic
{
public:
  static const std::string DVH_TYPE_ATTRIBUTE_NAME;
  static const std::string DVH_TYPE_ATTRIBUTE_VALUE;
  static const std::string DVH_STRUCTURE_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_STRUCTURE_PLOTS_NAME_ATTRIBUTE_NAME;
  static const std::string DVH_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  static const std::string DVH_MEAN_DOSE_GY_ATTRIBUTE_NAME;
  static const std::string DVH_MAX_DOSE_GY_ATTRIBUTE_NAME;
  static const std::string DVH_MIN_DOSE_GY_ATTRIBUTE_NAME;
  static const std::string DVH_VOXEL_COUNT_ATTRIBUTE_NAME;

public:
  static vtkSlicerDoseVolumeHistogramLogic *New();
  vtkTypeMacro(vtkSlicerDoseVolumeHistogramLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Compute DVH for the selected structure set based on the selected dose volume
  void ComputeDvh();

  /// Add dose volume histogram of a structure (ROI) to the selected chart given its name and the corresponding DVH double array node ID
  void AddDvhToSelectedChart(const char* structureName, const char* dvhArrayId);

  /// Remove dose volume histogram of a structure from the selected chart
  void RemoveDvhFromSelectedChart(const char* dvhArrayId);

public:
  void SetDoseVolumeNode( vtkMRMLVolumeNode* );
  void SetStructureSetModelNode( vtkMRMLNode* );
  void SetChartNode( vtkMRMLChartNode* );

  vtkGetObjectMacro( DoseVolumeNode, vtkMRMLVolumeNode );
  vtkGetObjectMacro( StructureSetModelNode, vtkMRMLNode );
  vtkGetObjectMacro( ChartNode, vtkMRMLChartNode );
  vtkGetObjectMacro( DvhDoubleArrayNodes, vtkCollection );

  vtkSetMacro( SceneChanged, bool );
  vtkGetMacro( SceneChanged, bool );
  vtkBooleanMacro( SceneChanged, bool );

protected:
  vtkSlicerDoseVolumeHistogramLogic();
  virtual ~vtkSlicerDoseVolumeHistogramLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Compute DVH and return statistics for the given volume (which is the selected dose volume stenciled with a structure) with the given structure name
  void ComputeDvh(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, char* structureName);

  /// Get stenciled dose volume for a structure (ROI)
  virtual void GetStenciledDoseVolumeForStructure(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModel);

  /// Return the chart view node object from the layout
  vtkMRMLChartViewNode* GetChartViewNode();

protected:
  vtkSetObjectMacro( DvhDoubleArrayNodes, vtkCollection );

private:
  vtkSlicerDoseVolumeHistogramLogic(const vtkSlicerDoseVolumeHistogramLogic&); // Not implemented
  void operator=(const vtkSlicerDoseVolumeHistogramLogic&);               // Not implemented

protected:
  /// Selected dose volume MRML node object
  vtkMRMLVolumeNode* DoseVolumeNode;

  /// Selected structure set MRML node object. Can be model node or model hierarchy node
  vtkMRMLNode* StructureSetModelNode;

  /// Selected chart MRML node object
  vtkMRMLChartNode* ChartNode;

  /// List of all the DVH double array MRML nodes that are present in the scene
  vtkCollection* DvhDoubleArrayNodes;

  /// Flag indicating if the scene has recently changed (update of the module GUI needed)
  bool SceneChanged;
};

#endif
