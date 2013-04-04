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

#ifndef __vtkMRMLRTPlanHierarchyNode_h
#define __vtkMRMLRTPlanHierarchyNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLScene.h>

#include "vtkSlicerRTPlanModuleMRMLExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLColorTableNode;

class VTK_SLICER_RTPLAN_MODULE_MRML_EXPORT vtkMRMLRTPlanHierarchyNode : public vtkMRMLDisplayableHierarchyNode
{
public:
  static vtkMRMLRTPlanHierarchyNode *New();
  vtkTypeMacro(vtkMRMLRTPlanHierarchyNode,vtkMRMLHierarchyNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Updates the referenced nodes from the updated scene
  virtual void UpdateScene(vtkMRMLScene *scene);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "RTPlanHierarchy";};

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// Updates this node if it depends on other nodes 
  /// when the node is deleted in the scene
  void UpdateReferences();

  /// Handles events registered in the observer manager
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

protected:
  vtkMRMLRTPlanHierarchyNode();
  ~vtkMRMLRTPlanHierarchyNode();
  vtkMRMLRTPlanHierarchyNode(const vtkMRMLRTPlanHierarchyNode&);
  void operator=(const vtkMRMLRTPlanHierarchyNode&);

protected:
  char* StructureName;
};

#endif // __vtkMRMLRTPlanHierarchyNode_h
