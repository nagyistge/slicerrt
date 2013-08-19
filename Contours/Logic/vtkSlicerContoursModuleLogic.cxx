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

// Contours includes
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursPatientHierarchyPlugin.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"
#include "vtkSlicerPatientHierarchyPluginHandler.h"

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkLookupTable.h>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContoursModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic::vtkSlicerContoursModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic::~vtkSlicerContoursModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());

  // Create default structure set node
  this->CreateDefaultStructureSetNode();
}

//-----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourNode>::New());

  // Register Patient Hierarchy plugin
  vtkSlicerPatientHierarchyPluginHandler::GetInstance()->RegisterPlugin(vtkSmartPointer<vtkSlicerContoursPatientHierarchyPlugin>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode"))
  {
    // Create empty ribbon model
    this->CreateEmptyRibbonModelForContour(node);

    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (contourNode)
  {
    // Delete ribbon model representation if it is the default empty model
    if (contourNode->RibbonModelContainsEmptyPolydata())
    {
      this->GetMRMLScene()->RemoveNode(contourNode->GetRibbonModelNode());
    }

    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneEndClose()
{
  assert(this->GetMRMLScene() != 0);

  this->CreateDefaultStructureSetNode();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneEndImport()
{
  assert(this->GetMRMLScene() != 0);

  vtkSmartPointer<vtkCollection> contourNodes = vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLContourNode") );
  vtkObject* nextObject = NULL;
  for (contourNodes->InitTraversal(); (nextObject = contourNodes->GetNextItemAsObject()); )
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(nextObject);
    if (contourNode)
    {
      contourNode->UpdateRepresenations();
    }
  }
  
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::CreateDefaultStructureSetNode()
{
  assert(this->GetMRMLScene() != 0);

  std::string defaultStructureSetNodeName;
  defaultStructureSetNodeName = SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NAME + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  vtkSmartPointer<vtkCollection> defaultStructureSetNodes = vtkSmartPointer<vtkCollection>::Take(
    this->GetMRMLScene()->GetNodesByClassByName("vtkMRMLHierarchyNode", defaultStructureSetNodeName.c_str()) );
  if (defaultStructureSetNodes->GetNumberOfItems() > 0)
  {
    vtkDebugMacro("CreateDefaultStructureSetNode: Default structure set node already exists");
    return;
  }

  vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> structureSetPatientHierarchyNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
  structureSetPatientHierarchyNode->SetName(defaultStructureSetNodeName.c_str());
  structureSetPatientHierarchyNode->AllowMultipleChildrenOn();
  structureSetPatientHierarchyNode->HideFromEditorsOff();
  structureSetPatientHierarchyNode->SetSaveWithScene(0);
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME, SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  std::string defaultStructureSetDicomUid = SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NAME + SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME;
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME, defaultStructureSetDicomUid.c_str());
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  this->GetMRMLScene()->AddNode(structureSetPatientHierarchyNode);

  // A hierarchy node needs a display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> structureSetPatientHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  std::string contourHierarchyDisplayNodeName = defaultStructureSetNodeName + "_Display";
  structureSetPatientHierarchyDisplayNode->SetName(contourHierarchyDisplayNodeName.c_str());
  structureSetPatientHierarchyDisplayNode->SetVisibility(1);

  this->GetMRMLScene()->AddNode(structureSetPatientHierarchyDisplayNode);
  structureSetPatientHierarchyNode->SetAndObserveDisplayNodeID(structureSetPatientHierarchyDisplayNode->GetID());

  // Add color table node and default colors
  vtkSmartPointer<vtkMRMLColorTableNode> structureSetColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string structureSetColorTableNodeName;
  structureSetColorTableNodeName = SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NAME + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  structureSetColorTableNodeName = this->GetMRMLScene()->GenerateUniqueName(structureSetColorTableNodeName);
  structureSetColorTableNode->SetName(structureSetColorTableNodeName.c_str());
  structureSetColorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  structureSetColorTableNode->HideFromEditorsOff();
  structureSetColorTableNode->SetSaveWithScene(0);
  structureSetColorTableNode->SetTypeToUser();
  this->GetMRMLScene()->AddNode(structureSetColorTableNode);

  structureSetColorTableNode->SetNumberOfColors(2);
  structureSetColorTableNode->GetLookupTable()->SetTableRange(0,1);
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_BACKGROUND, 0.0, 0.0, 0.0, 0.0); // Black background
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_INVALID,
    SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
    SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] ); // Color indicating invalid index

  // Add color table in patient hierarchy
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchyColorTableNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  std::string phColorTableNodeName;
  phColorTableNodeName = structureSetColorTableNodeName + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  phColorTableNodeName = this->GetMRMLScene()->GenerateUniqueName(phColorTableNodeName);
  patientHierarchyColorTableNode->SetName(phColorTableNodeName.c_str());
  patientHierarchyColorTableNode->HideFromEditorsOff();
  patientHierarchyColorTableNode->SetSaveWithScene(0);
  patientHierarchyColorTableNode->SetAssociatedNodeID(structureSetColorTableNode->GetID());
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME, SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
  patientHierarchyColorTableNode->SetParentNodeID(structureSetPatientHierarchyNode->GetID());
  this->GetMRMLScene()->AddNode(patientHierarchyColorTableNode);
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::CreateEmptyRibbonModelForContour(vtkMRMLNode* node)
{
  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("CreateEmptyRibbonModelForContour: Invalid MRML scene!");
    return;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (!contourNode)
  {
    vtkErrorMacro("CreateEmptyRibbonModelForContour: Argument node is not a contour node!");
    return;
  }

  // If a scene is being loaded then don't create empty model, because the node will be updated
  // to the proper representation when importing has finished. \sa vtkMRMLContourNode::ReadXMLAttributes
  if (contourNode->RibbonModelNodeId && !contourNode->RibbonModelNode)
  {
    return;
  }

  vtkSmartPointer<vtkPolyData> emptyPolyData = vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkMRMLModelDisplayNode> emptyRibbonModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  mrmlScene->AddNode(emptyRibbonModelDisplayNode);

  vtkSmartPointer<vtkMRMLModelNode> emptyRibbonModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  mrmlScene->AddNode(emptyRibbonModelNode);
  emptyRibbonModelNode->SetAndObserveDisplayNodeID(emptyRibbonModelDisplayNode->GetID());
  emptyRibbonModelNode->SetAndObservePolyData(emptyPolyData);

  std::string emptyRibbonModelName(contourNode->GetName());
  emptyRibbonModelName.append(SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX);
  emptyRibbonModelNode->SetName(emptyRibbonModelName.c_str());

  contourNode->SetAndObserveRibbonModelNodeId(emptyRibbonModelNode->GetID());
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::PaintLabelmapForeground(vtkMRMLScalarVolumeNode* volumeNode, unsigned char newColor)
{
  if (!volumeNode)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::PaintLabelmapForeground: volumeNode argument is null!" << std::endl;
    return;
  }
  if (newColor <= SlicerRtCommon::COLOR_INDEX_INVALID)
  {
    vtkErrorWithObjectMacro(volumeNode, "PaintLabelmapForeground: Invalid color index given! Color index must be greater than the invalid color index (" << SlicerRtCommon::COLOR_INDEX_INVALID << ")");
    return;
  }

  vtkImageData* imageData = volumeNode->GetImageData();
  if (!imageData || imageData->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    vtkErrorWithObjectMacro(volumeNode, "PaintLabelmapForeground: Invalid image data! Scalar type has to be unsigned char instead of '" << (imageData?imageData->GetScalarTypeAsString():"None") << "'");
    return;
  }

  unsigned char* imagePtr = (unsigned char*)imageData->GetScalarPointer();
  for (long i=0; i<imageData->GetNumberOfPoints(); ++i)
  {
    if ( (*imagePtr) != 0 )
    {
      (*imagePtr) = newColor;
    }
    ++imagePtr;
  }
  imageData->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode(vtkMRMLNode* node, std::vector<vtkMRMLContourNode*>& contours)
{
  if (!node)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode: node argument is null!" << std::endl;
    return;
  }

  contours.clear();

  // Create list of selected contour nodes
  if (node->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
    if (contourNode)
    {
      contours.push_back(contourNode);
    }
  }
  else if ( node->IsA("vtkMRMLDisplayableHierarchyNode")
    && SlicerRtCommon::IsPatientHierarchyNode(node) && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLDisplayableHierarchyNode::SafeDownCast(node)->GetChildrenDisplayableNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      vtkWarningWithObjectMacro(node, "GetContourNodesFromSelectedNode: Selected contour hierarchy node has no children contour nodes!");
      return;
    }

    // Collect contour nodes in the hierarchy and determine whether their active representation types are the same
    for (int i=0; i<childContourNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(childContourNodes->GetItemAsObject(i));
      if (contourNode)
      {
        contours.push_back(contourNode);
      }
    }
  }
  else
  {
    vtkErrorWithObjectMacro(node, "GetContourNodesFromSelectedNode: Invalid node type for contour!");
  }
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType vtkSlicerContoursModuleLogic::GetRepresentationTypeOfContours(std::vector<vtkMRMLContourNode*>& contours)
{
  bool sameRepresentationTypes = true;
  vtkMRMLContourNode::ContourRepresentationType representationType = vtkMRMLContourNode::None;

  for (std::vector<vtkMRMLContourNode*>::iterator it = contours.begin(); it != contours.end(); ++it)
  {
    if (representationType == vtkMRMLContourNode::None)
    {
      representationType = (*it)->GetActiveRepresentationType();
    }
    else if ((*it)->GetActiveRepresentationType() == vtkMRMLContourNode::None) // Sanity check
    {
      vtkWarningWithObjectMacro((*it), "getRepresentationTypeOfSelectedContours: Invalid representation type (None) found for the contour node '" << (*it)->GetName() << "'!")
    }
    else if (representationType != (*it)->GetActiveRepresentationType())
    {
      sameRepresentationTypes = false;
    }
  }

  if (sameRepresentationTypes)
  {
    return representationType;
  }
  else
  {
    return vtkMRMLContourNode::None;
  }
}

//-----------------------------------------------------------------------------
const char* vtkSlicerContoursModuleLogic::GetRasterizationReferenceVolumeOfContours(std::vector<vtkMRMLContourNode*>& contours, bool &sameReferenceVolumeInContours)
{
  sameReferenceVolumeInContours = true;
  std::string rasterizationReferenceVolumeNodeId("");

  for (std::vector<vtkMRMLContourNode*>::iterator it = contours.begin(); it != contours.end(); ++it)
  {
    if (rasterizationReferenceVolumeNodeId.empty())
    {
      rasterizationReferenceVolumeNodeId = std::string((*it)->GetRasterizationReferenceVolumeNodeId());
    }
    else if ( ((*it)->GetRasterizationReferenceVolumeNodeId() == NULL && !rasterizationReferenceVolumeNodeId.empty())
           || ((*it)->GetRasterizationReferenceVolumeNodeId() != NULL && STRCASECMP(rasterizationReferenceVolumeNodeId.c_str(), (*it)->GetRasterizationReferenceVolumeNodeId())) )
    {
      sameReferenceVolumeInContours = false;
    }
  }

  if (sameReferenceVolumeInContours)
  {
    return rasterizationReferenceVolumeNodeId.c_str();
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerContoursModuleLogic::GetReferencedVolumeByDicomForContours(std::vector<vtkMRMLContourNode*>& contours)
{
  if (contours.empty())
  {
    return NULL;
  }

  // Get series hierarchy node from first contour
  vtkMRMLHierarchyNode* contourHierarchySeriesNode = vtkSlicerPatientHierarchyModuleLogic::GetAncestorAtLevel(
    *contours.begin(), vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  if (!contourHierarchySeriesNode)
  {
    std::cerr << "vtkSlicerContoursModuleLogic::GetReferencedSeriesForContours: Failed to find series hierarchy node for contour '"
      << ((*contours.begin())==NULL ? "NULL" : (*contours.begin())->GetName()) << "'!" << std::endl;
    return NULL;
  }

  // Traverse contour nodes from parent
  std::string commonReferencedSeriesUid("");
  std::vector<vtkMRMLHierarchyNode*> contourPatientHierarchyNodes = contourHierarchySeriesNode->GetChildrenNodes();
  for (std::vector<vtkMRMLHierarchyNode*>::iterator contourIt=contourPatientHierarchyNodes.begin(); contourIt!=contourPatientHierarchyNodes.end(); ++contourIt)
  {
    vtkMRMLHierarchyNode* contourPatientHierarchyNode = (*contourIt);

    // Get referenced series UID for contour
    const char* referencedSeriesUid = contourPatientHierarchyNode->GetAttribute(
      SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str() );

    if (!referencedSeriesUid)
    {
      vtkWarningWithObjectMacro(contourHierarchySeriesNode, "No referenced series UID found for contour '" << contourPatientHierarchyNode->GetName() << "'");
    }
    // Set a candidate common reference UID in the first loop
    else if (commonReferencedSeriesUid.empty())
    {
      commonReferencedSeriesUid = std::string(referencedSeriesUid);
    }
    // Return NULL if referenced UIDs are different
    else if (STRCASECMP(referencedSeriesUid, commonReferencedSeriesUid.c_str()))
    {
      return NULL;
    }
  }

  // Return NULL if no referenced UID has been found for any of the contours
  if (commonReferencedSeriesUid.empty())
  {
    return NULL;
  }

  // Common referenced series UID found, get corresponding patient hierarchy node
  vtkMRMLHierarchyNode* referencedSeriesNode = vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(
    contourHierarchySeriesNode->GetScene(), commonReferencedSeriesUid.c_str());

  // Get and return referenced volume
  return vtkMRMLScalarVolumeNode::SafeDownCast(referencedSeriesNode->GetAssociatedNode());
}

//-----------------------------------------------------------------------------
bool vtkSlicerContoursModuleLogic::ContoursContainRepresentation(std::vector<vtkMRMLContourNode*>& contours, vtkMRMLContourNode::ContourRepresentationType representationType, bool allMustContain/*=true*/)
{
  if (contours.size() == 0)
  {
    return false;
  }

  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = contours.begin(); contourIt != contours.end(); ++contourIt)
  {
    if (allMustContain && !(*contourIt)->RepresentationExists(representationType))
    {
      // At least one misses the requested representation
      return false;
    }
    else if (!allMustContain && (*contourIt)->RepresentationExists(representationType))
    {
      // At least one has the requested representation
      return true;
    }
  }

  if (allMustContain)
  {
    // All contours have the requested representation
    return true;
  }
  else
  {
    // None of the contours have the requested representation
    return false;
  }
}

//-----------------------------------------------------------------------------
bool vtkSlicerContoursModuleLogic::IsReferenceVolumeValidForAllContours(std::vector<vtkMRMLContourNode*>& contours, vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = contours.begin(); contourIt != contours.end(); ++contourIt)
  {
    // If (target is indexed labelmap OR an intermediate labelmap is needed for closed surface conversion BUT missing)
    // AND (the selected reference node is empty AND was not created from labelmap), then reference volume selection is invalid
    if ( ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
      || ( targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel
      && !(*contourIt)->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap) ) )
      && ( !(*contourIt)->GetRasterizationReferenceVolumeNodeId()
      && !(*contourIt)->HasBeenCreatedFromIndexedLabelmap() ) )
    {
      return false;
    }
  }

  return true;
}
