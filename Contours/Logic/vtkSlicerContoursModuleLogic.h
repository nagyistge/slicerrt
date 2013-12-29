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

// .NAME vtkSlicerContoursModuleLogic - Logic class for contour handling
// .SECTION Description
// This class manages the logic associated with converting and handling
// contour node objects.


#ifndef __vtkSlicerContoursModuleLogic_h
#define __vtkSlicerContoursModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerContoursModuleLogicExport.h"

// SlicerRt includes
#include "vtkMRMLContourNode.h"

// MRML includes
class vtkMRMLScalarVolumeNode;

/// \ingroup Slicer_QtModules_Contours
class VTK_SLICER_CONTOURS_LOGIC_EXPORT vtkSlicerContoursModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerContoursModuleLogic *New();
  vtkTypeMacro(vtkSlicerContoursModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Paint the foreground of a specified labelmap to a certain label.
  /// This makes sure that a labelmap whose color table has changed has the same color afterwards
  static void PaintLabelmapForeground(vtkMRMLScalarVolumeNode* volumeNode, unsigned char newColor);

  /// Collect contour nodes from selected nodes
  /// \param node Selected node that contains either a vtkMRMLContourNode or a vtkMRMLDisplayableHierarchyNode
  ///        being a parent of multiple contour nodes in patient hierarchy
  /// \param contours Output list of collected contour nodes
  static void GetContourNodesFromSelectedNode(vtkMRMLNode* node, std::vector<vtkMRMLContourNode*>& contours);

  /// Get the common representation in contour list.
  /// \param contours Input list of collected contour nodes
  /// \return The common representation in the input contours or 'None' when they are not the same
  static vtkMRMLContourNode::ContourRepresentationType GetRepresentationTypeOfContours(std::vector<vtkMRMLContourNode*>& contours);

  /// Get the common rasterization reference volume in contour list.
  /// \param contours Input list of collected contour nodes
  /// \param sameReferenceVolumeInContours Output parameter: True if the reference volumes in the contours are the same, false otherwise
  /// \return The common rasterization reference volume in the input contours or NULL when they are not the same
  static const char* GetRasterizationReferenceVolumeOfContours(std::vector<vtkMRMLContourNode*>& contours, bool &sameReferenceVolumeInContours);

  /// Get referenced volume for list of contours according to patient hierarchy attributes
  /// \return The common reference volume for the contours if any, NULL otherwise
  static vtkMRMLScalarVolumeNode* GetReferencedVolumeByDicomForContours(std::vector<vtkMRMLContourNode*>& contours);

  /// Determine if the selected contours contain a certain representation
  /// \param contours List of contours to examine
  /// \param representationType The required representation type
  /// \param allMustContain If set to true, this function returns true only if all the selected
  ///        contours contain the representation. Otherwise it returns true even if only one contains it
  /// \return True if every selected node has the given type of representation, false otherwise
  static bool ContoursContainRepresentation(std::vector<vtkMRMLContourNode*>& contours, vtkMRMLContourNode::ContourRepresentationType representationType, bool allMustContain=true);

  /// Get the indexed labelmap representation of a contour with a certain geometry (grid, lattice)
  /// Note: The contour node will not be changed. If the result indexed labelmap needs to be set to the contour, it has to be done manually
  /// \param Input contour object
  /// \param referenceVolumeNode Input volume node object that has the desired geometry
  /// \param outputIndexedLabelmap Output volume node in which the result is copied
  static void GetIndexedLabelmapWithGivenGeometry(vtkMRMLContourNode* contour, vtkMRMLScalarVolumeNode* referenceVolumeNode, vtkMRMLScalarVolumeNode* outputIndexedLabelmap);

  /// Create a contour node from a representation. Does not handle color tables
  /// \param representationNode Representation to create the contour from. Can be labelmap or model
  static vtkMRMLContourNode* CreateContourFromRepresentation(vtkMRMLDisplayableNode* representationNode);

protected:
  /// Creates an empty ribbon model as a default representation for the new contours
  void CreateEmptyRibbonModelForContour(vtkMRMLNode* node);

protected:
  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();

  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndClose();
  virtual void OnMRMLSceneEndImport();

protected:
  vtkSlicerContoursModuleLogic();
  virtual ~vtkSlicerContoursModuleLogic();

private:
  vtkSlicerContoursModuleLogic(const vtkSlicerContoursModuleLogic&); // Not implemented
  void operator=(const vtkSlicerContoursModuleLogic&);               // Not implemented
};

#endif