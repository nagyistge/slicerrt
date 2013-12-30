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

// SubjectHierarchy includes
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSubjectHierarchyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerSubjectHierarchyModuleLogic::vtkSlicerSubjectHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerSubjectHierarchyModuleLogic::~vtkSlicerSubjectHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSubjectHierarchyModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
  vtkMRMLScene *scene, const char* patientId, const char* studyInstanceUID, const char* seriesInstanceUID )
{
  //TODO: Move this function to the DICOM plugin???
  if ( !scene || !patientId || !studyInstanceUID || !seriesInstanceUID )
  {
    std::cerr << "vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy: Invalid input arguments!" << std::endl;
    return NULL;
  }

  vtkMRMLSubjectHierarchyNode* patientNode = NULL;
  vtkMRMLSubjectHierarchyNode* studyNode = NULL;
  vtkMRMLSubjectHierarchyNode* seriesNode = NULL;

  std::vector<vtkMRMLNode*> subjectHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLHierarchyNode", subjectHierarchyNodes);

  // Find referenced nodes
  for (unsigned int i=0; i<numberOfNodes; i++)
  {
    vtkMRMLSubjectHierarchyNode *node = vtkMRMLSubjectHierarchyNode::SafeDownCast(subjectHierarchyNodes[i]);
    if ( node && node->IsA("vtkMRMLSubjectHierarchyNode") )
    {
      std::string nodeDicomUIDStr = node->GetUID(vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME);
      const char* nodeDicomUID = nodeDicomUIDStr.c_str();
      if (!nodeDicomUID)
      {
        // Having a UID is not mandatory
        //TODO: Isn't it?
        continue;
      }
      if (!strcmp(patientId, nodeDicomUID))
      {
        patientNode = node;
      }
      else if (!strcmp(studyInstanceUID, nodeDicomUID))
      {
        studyNode = node;
      }
      else if (!strcmp(seriesInstanceUID, nodeDicomUID))
      {
        seriesNode = node;
      }
    }
  }

  if (!seriesNode)
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerSubjectHierarchyModuleLogic::InsertDicomSeriesInHierarchy: Subject hierarchy node with DICOM UID '"
      << seriesInstanceUID << "' cannot be found!");
    return NULL;
  }

  // Create patient and study nodes if they do not exist yet
  if (!patientNode)
  {
    patientNode = vtkMRMLSubjectHierarchyNode::New();
    patientNode->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_PATIENT);
    patientNode->AddUID(vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, patientId);
    scene->AddNode(patientNode);
    patientNode->Delete(); // Return ownership to the scene only
  }

  if (!studyNode)
  {
    studyNode = vtkMRMLSubjectHierarchyNode::New();
    studyNode->SetLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY);
    studyNode->AddUID(vtkSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, studyInstanceUID);
    studyNode->SetParentNodeID(patientNode->GetID());
    scene->AddNode(studyNode);
    studyNode->Delete(); // Return ownership to the scene only
  }

  seriesNode->SetParentNodeID(studyNode->GetID());

  return seriesNode;
}

//---------------------------------------------------------------------------
bool vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch(vtkMRMLNode* node1, vtkMRMLNode* node2,
                                                                const char* lowestCommonLevel)
{
  if ( !node1 || !node2 || node1->GetScene() != node2->GetScene() )
  {
    std::cerr << "vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch: Invalid input nodes or they are not in the same scene!" << std::endl;
    return false;
  }

  if (!lowestCommonLevel)
  {
    vtkErrorWithObjectMacro(node1, "vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch: Invalid lowest common level!");
    return false;
  }

  // If not hierarchy nodes, get the associated subject hierarchy node
  vtkMRMLSubjectHierarchyNode* hierarchyNode1 = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(node1);
  vtkMRMLSubjectHierarchyNode* hierarchyNode2 = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(node2);

  // Check if valid nodes are found
  if (!hierarchyNode1 || !hierarchyNode2)
  {
    return false;
  }

  // Walk the hierarchy up until we reach the lowest common level
  while (true)
  {
    hierarchyNode1 = vtkMRMLSubjectHierarchyNode::SafeDownCast(hierarchyNode1->GetParentNode());
    if (!hierarchyNode1)
    {
      hierarchyNode1 = NULL;
      vtkWarningWithObjectMacro(node1, "Subject hierarchy node ('" << hierarchyNode1->GetName() << "') has no ancestor with DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    const char* node1Level = hierarchyNode1->GetLevel();
    if (!node1Level)
    {
      hierarchyNode1 = NULL;
      vtkWarningWithObjectMacro(node1, "Subject hierarchy node ('" << hierarchyNode1->GetName() << "') has no DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    if (!strcmp(node1Level, lowestCommonLevel))
    {
      break;
    }
  }

  while (true)
  {
    hierarchyNode2 = vtkMRMLSubjectHierarchyNode::SafeDownCast(hierarchyNode2->GetParentNode());
    if (!hierarchyNode2)
    {
      hierarchyNode2 = NULL;
      vtkWarningWithObjectMacro(node1, "Subject hierarchy node ('" << hierarchyNode2->GetName() << "') has no ancestor with DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    const char* node2Level = hierarchyNode2->GetLevel();
    if (!node2Level)
    {
      hierarchyNode2 = NULL;
      vtkWarningWithObjectMacro(node1, "Subject hierarchy node ('" << hierarchyNode2->GetName() << "') has no DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    if (!strcmp(node2Level, lowestCommonLevel))
    {
      break;
    }
  }

  return (hierarchyNode1 == hierarchyNode2);
}
