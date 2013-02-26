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

// .NAME vtkSlicerPatientHierarchyModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerPatientHierarchyModuleLogic_h
#define __vtkSlicerPatientHierarchyModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerPatientHierarchyModuleLogicExport.h"

class vtkMRMLPatientHierarchyNode;

/// \ingroup Slicer_QtModules_PatientHierarchy
class VTK_SLICER_PATIENTHIERARCHY_LOGIC_EXPORT vtkSlicerPatientHierarchyModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerPatientHierarchyModuleLogic *New();
  vtkTypeMacro(vtkSlicerPatientHierarchyModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Find patient hierarchy node according to an instance UID and database
  static vtkMRMLPatientHierarchyNode* GetPatientHierarchyNodeByUid(
    vtkMRMLScene *scene, const char* dicomDatabaseFileName, const char* uid );

  /// Place series in patient hierarchy. Create patient and study node if needed
  static void InsertDicomSeriesInHierarchy(
    vtkMRMLScene *scene, const char* dicomDatabaseFileName, 
    const char* patientId, const char* studyInstanceUid, const char* seriesInstanceUid );

  /// Determine if two patient hierarchy nodes are in the same branch (share the same parent)
  /// \param nodeId1 ID of the first node to check. Can be patient hierarchy node or a node
  ///   associated with one
  /// \param nodeId2 ID of the second node to check
  /// \param lowestCommonLevel Lowest level on which they have to share an ancestor
  /// \return True if the two nodes or their associated hierarchy nodes share a parent on the
  ///   specified level, false otherwise
  static bool AreNodesInSameBranch( vtkMRMLScene *scene,
    const char* nodeId1, const char* nodeId2, int lowestCommonLevel );

protected:
  vtkSlicerPatientHierarchyModuleLogic();
  virtual ~vtkSlicerPatientHierarchyModuleLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();

private:
  vtkSlicerPatientHierarchyModuleLogic(const vtkSlicerPatientHierarchyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerPatientHierarchyModuleLogic&);               // Not implemented
};

#endif
