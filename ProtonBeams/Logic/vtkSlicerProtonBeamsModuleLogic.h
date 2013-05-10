/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// .NAME vtkSlicerBeamsModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerProtonBeamsModuleLogic_h
#define __vtkSlicerProtonBeamsModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerBeamsModuleLogicExport.h"

class vtkTransform;
class vtkMRMLVolumeNode;
class vtkMRMLProtonBeamsNode;

/// \ingroup Slicer_QtModules_Beams
class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerProtonBeamsModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerProtonBeamsModuleLogic *New();
  vtkTypeMacro(vtkSlicerProtonBeamsModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Compute and set source fiducial position from isocenter position and beam parameters
  void ComputeSourceFiducialPosition(std::string &errorMessage, vtkTransform* isocenterToSourceTransform=NULL);

  /// Create beam geometry model from isocenter and source fiducials
  void CreateProtonBeamModel(std::string &errorMessage);

public:
  /// Set and observe dose accumulation parameter node 
  void SetAndObserveProtonBeamsNode(vtkMRMLProtonBeamsNode* node);

  /// Get dose accumulation parameter node 
  vtkGetObjectMacro(ProtonBeamsNode, vtkMRMLProtonBeamsNode);

protected:
  vtkSlicerProtonBeamsModuleLogic();
  virtual ~vtkSlicerProtonBeamsModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerProtonBeamsModuleLogic(const vtkSlicerProtonBeamsModuleLogic&); // Not implemented
  void operator=(const vtkSlicerProtonBeamsModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLProtonBeamsNode* ProtonBeamsNode;
};

#endif

