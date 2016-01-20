/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// MRMLBeams includes
#include "vtkMRMLBeamsNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* ISOCENTER_MARKUPS_REFERENCE_ROLE = "isocenterMarkupsRef";
static const char* BEAM_MODEL_REFERENCE_ROLE = "beamModelRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBeamsNode);

//----------------------------------------------------------------------------
vtkMRMLBeamsNode::vtkMRMLBeamsNode()
{
  this->BeamModelOpacity = 0.08;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLBeamsNode::~vtkMRMLBeamsNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->BeamModelOpacity )
      {
      ss << this->BeamModelOpacity;
      of << indent << " BeamModelOpacity=\"" << ss.str() << "\"";
      }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "BeamModelOpacity")) 
      {
      std::stringstream ss;
      ss << attValue;
      double beamModelOpacity;
      ss >> beamModelOpacity;
      this->BeamModelOpacity = beamModelOpacity;
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLBeamsNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLBeamsNode *node = (vtkMRMLBeamsNode *) anode;

  this->SetBeamModelOpacity(node->GetBeamModelOpacity());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "BeamModelOpacity:   " << this->BeamModelOpacity << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLBeamsNode::GetIsocenterMarkupsNode()
{
  return vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetNodeReference(ISOCENTER_MARKUPS_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::SetAndObserveIsocenterMarkupsNode(vtkMRMLMarkupsFiducialNode* node)
{
  this->SetNodeReferenceID(ISOCENTER_MARKUPS_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLBeamsNode::GetBeamModelNode()
{
  return vtkMRMLModelNode::SafeDownCast( this->GetNodeReference(BEAM_MODEL_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::SetAndObserveBeamModelNode(vtkMRMLModelNode* node)
{
  this->SetNodeReferenceID(BEAM_MODEL_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}
