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

// .NAME vtkSlicerDicomRtImportModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtImportLogic_h
#define __vtkSlicerDicomRtImportLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include <vector>

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkSlicerVolumesLogic;
class vtkSlicerIsodoseModuleLogic;
class vtkMRMLModelNode;
class vtkMRMLHierarchyNode;
class vtkMRMLAnnotationFiducialNode;
class vtkDICOMImportInfo;
class vtkPolyData;
class vtkSlicerDicomRtReader;

/// \ingroup SlicerRt_DicomRtImportLogic
class VTK_SLICER_DICOMRTIMPORT_LOGIC_EXPORT vtkSlicerDicomRtImportModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDicomRtImportModuleLogic *New();
  vtkTypeMacro(vtkSlicerDicomRtImportModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Examine a list of file lists and determine what objects can be loaded from them
  void Examine(vtkDICOMImportInfo *importInfo);

  /// Load DICOM RT series from file name
  /// /return True if loading successful
  bool LoadDicomRT(vtkDICOMImportInfo *loadInfo);

  /// Set Volumes module logic
  void SetVolumesLogic(vtkSlicerVolumesLogic* volumesLogic);

  /// Set Isodose module logic
  void SetIsodoseLogic(vtkSlicerIsodoseModuleLogic* isodoseLogic);

public:
  vtkSetMacro(AutoContourOpacity, bool);
  vtkGetMacro(AutoContourOpacity, bool);
  vtkBooleanMacro(AutoContourOpacity, bool);

  vtkSetMacro(BeamModelsInSeparateBranch, bool);
  vtkGetMacro(BeamModelsInSeparateBranch, bool);
  vtkBooleanMacro(BeamModelsInSeparateBranch, bool);

  vtkGetStringMacro(DefaultDoseColorTableNodeId);
  vtkSetStringMacro(DefaultDoseColorTableNodeId);

protected:
  vtkSlicerDicomRtImportModuleLogic();
  virtual ~vtkSlicerDicomRtImportModuleLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  /// Load RT Structure Set and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtStructureSet(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo* loadInfo);

  /// Add an ROI point to the scene
  vtkMRMLAnnotationFiducialNode* AddRoiPoint(double* roiPosition, std::string baseName, double* roiColor);

  /// Add an ROI contour to the scene
  vtkMRMLModelNode* AddRoiContour(vtkPolyData* roiPoly, std::string baseName, double* roiColor);

  /// Load RT Dose and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtDose(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo* loadInfo);

  /// Load RT Plan and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtPlan(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo* loadInfo);

  /// Load RT Image and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtImage(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo* loadInfo);

  /// Insert currently loaded series in the proper place in patient hierarchy
  void InsertSeriesInPatientHierarchy(vtkSlicerDicomRtReader* rtReader);

  /// Creates default dose color table.
  /// Should not be called, except when updating the default dose color table file manually, or when the file cannot be found (\sa LoadDefaultDoseColorTable)
  void CreateDefaultDoseColorTable();

private:
  vtkSlicerDicomRtImportModuleLogic(const vtkSlicerDicomRtImportModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDicomRtImportModuleLogic&);              // Not implemented

private:
  /// Volumes logic instance
  vtkSlicerVolumesLogic* VolumesLogic;

  /// Isodose logic instance
  vtkSlicerIsodoseModuleLogic* IsodoseLogic;

  /// Flag indicating whether opacity values for the loaded contours are automatically determined
  bool AutoContourOpacity;

  /// Flag determining whether the generated beam models are arranged in a separate patient hierarchy
  /// branch, or each beam model is added under its corresponding isocenter fiducial
  bool BeamModelsInSeparateBranch;

  /// Default dose color table ID. Loaded on Slicer startup.
  char* DefaultDoseColorTableNodeId;
};

#endif
