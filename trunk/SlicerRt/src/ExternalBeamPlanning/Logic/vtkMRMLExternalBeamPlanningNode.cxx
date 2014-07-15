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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// MRMLDoseAccumulation includes
#include "vtkMRMLExternalBeamPlanningNode.h"

// SlicerRT includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLContourNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLDoubleArrayNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* RFERENCE_VOLUME_REFERENCE_ROLE = "referenceVolumeRef";
static const char* RT_PLAN_REFERENCE_ROLE = "rtPlanRef";
static const char* PLAN_CONTOUR_SET_REFERENCE_ROLE = "planContourSetRef";
static const char* ISOCENTER_FIDUCIAL_REFERENCE_ROLE = "isocenterFiducialRef";
static const char* TARGET_CONTOUR_REFERENCE_ROLE = "targetContourRef";
static const char* MLCPOSITION_REFERENCE_ROLE = "MLCPositionRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLExternalBeamPlanningNode);

//----------------------------------------------------------------------------
vtkMRMLExternalBeamPlanningNode::vtkMRMLExternalBeamPlanningNode()
{
  this->BeamName = NULL;
  this->SetBeamName("RTBeam");
  this->BeamNumber = 0;
  this->BeamDescription = NULL;
  this->RadiationType = Proton;

  this->BeamType = Static;
  this->NominalEnergy = 80.0;
  this->NominalmA = 1.0;
  this->RxDose = 1.0;
  this->BeamOnTime = 0.0;
  this->X1Jaw = 100;
  this->X2Jaw = 100;
  this->Y1Jaw = 100;
  this->Y2Jaw = 100;

  this->GantryAngle = 0;
  this->CollimatorAngle = 0;
  this->CouchAngle = 0;
  this->Isocenter[0] = 0.0;
  this->Isocenter[1] = 0.0;
  this->Isocenter[2] = 0.0;

  this->CollimatorType = SquareHalfMM;

  this->SAD = 2000.0;
  
  this->BeamWeight = 1.0;
  this->EnergyResolution = 2.0;

  this->BeamFlavor = 'a';

  this->HideFromEditors = false;

  this->ApertureOffset = 1500.0;
  
  this->ApertureSpacingAtIso[0] = 2.0;
  this->ApertureSpacingAtIso[1] = 2.0;
  this->ApertureSpacing[0] = this->ApertureSpacingAtIso[0] * this->ApertureOffset / this->SAD;
  this->ApertureSpacing[1] = this->ApertureSpacingAtIso[1] * this->ApertureOffset / this->SAD;
  this->ApertureOrigin[0] = this->X1Jaw * this->ApertureOffset / this->SAD;
  this->ApertureOrigin[1] = this->Y1Jaw * this->ApertureOffset / this->SAD;
  this->ApertureDim[0] = (int) ((this->X2Jaw + this->X1Jaw) / this->ApertureSpacingAtIso[0] + 1);
  this->ApertureDim[1] = (int) ((this->Y2Jaw + this->Y1Jaw) / this->ApertureSpacingAtIso[1] + 1);

  this->SourceSize = 0.0;
}

//----------------------------------------------------------------------------
vtkMRMLExternalBeamPlanningNode::~vtkMRMLExternalBeamPlanningNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLExternalBeamPlanningNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLExternalBeamPlanningNode *node = (vtkMRMLExternalBeamPlanningNode *)anode;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLExternalBeamPlanningNode::GetReferenceVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetNodeReference(RFERENCE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(RFERENCE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode* vtkMRMLExternalBeamPlanningNode::GetRtPlanNode()
{
  return vtkMRMLRTPlanNode::SafeDownCast( this->GetNodeReference(RT_PLAN_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveRtPlanNode(vtkMRMLRTPlanNode* node)
{
  this->SetNodeReferenceID(RT_PLAN_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLExternalBeamPlanningNode::GetPlanContourSetNode()
{
  return vtkMRMLNode::SafeDownCast( this->GetNodeReference(PLAN_CONTOUR_SET_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObservePlanContourSetNode(vtkMRMLNode* node)
{
  this->SetNodeReferenceID(PLAN_CONTOUR_SET_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLExternalBeamPlanningNode::GetIsocenterFiducialNode()
{
  return vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetNodeReference(ISOCENTER_FIDUCIAL_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode* node)
{
  this->SetNodeReferenceID(ISOCENTER_FIDUCIAL_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLExternalBeamPlanningNode::GetTargetContourNode()
{
  return vtkMRMLContourNode::SafeDownCast( this->GetNodeReference(TARGET_CONTOUR_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveTargetContourNode(vtkMRMLContourNode* node)
{
  this->SetNodeReferenceID(TARGET_CONTOUR_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLDoubleArrayNode* vtkMRMLExternalBeamPlanningNode::GetMLCPositionDoubleArrayNode()
{
  return vtkMRMLDoubleArrayNode::SafeDownCast( this->GetNodeReference(MLCPOSITION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode* node)
{
  this->SetNodeReferenceID(MLCPOSITION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
const double* vtkMRMLExternalBeamPlanningNode::GetIsocenterPosition ()
{
  return this->Isocenter;
}

//----------------------------------------------------------------------------
double vtkMRMLExternalBeamPlanningNode::GetIsocenterPosition (int dim)
{
  return this->Isocenter[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetIsocenterPosition (const float* position)
{
  for (int d = 0; d < 3; d++) 
  {
    this->Isocenter[d] = position[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetIsocenterPosition (const double* position)
{
    for (int d = 0; d < 3; d++) {
        this->Isocenter[d] = position[d];
    }
}

//----------------------------------------------------------------------------
const double* vtkMRMLExternalBeamPlanningNode::GetApertureSpacing ()
{
  return this->ApertureSpacing;
}

//----------------------------------------------------------------------------
double vtkMRMLExternalBeamPlanningNode::GetApertureSpacing (int dim)
{
  return this->ApertureSpacing[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetApertureSpacing (const float* spacing)
{
  for (int d = 0; d < 2; d++) 
  {
  this->ApertureSpacing[d] = spacing[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetApertureSpacing (const double* spacing)
{
    for (int d = 0; d < 2; d++) {
    this->ApertureSpacing[d] = spacing[d];
    }
}

//----------------------------------------------------------------------------
const double* vtkMRMLExternalBeamPlanningNode::GetApertureSpacingAtIso ()
{
  return this->ApertureSpacingAtIso;
}

//----------------------------------------------------------------------------
double vtkMRMLExternalBeamPlanningNode::GetApertureSpacingAtIso (int dim)
{
  return this->ApertureSpacingAtIso[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetApertureSpacingAtIso (const float* spacing)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureSpacingAtIso[d] = spacing[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetApertureSpacingAtIso (const double* spacing)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureSpacingAtIso[d] = spacing[d];
  }
}

//----------------------------------------------------------------------------
const double* vtkMRMLExternalBeamPlanningNode::GetApertureOrigin ()
{
  return this->ApertureOrigin;
}

//----------------------------------------------------------------------------
double vtkMRMLExternalBeamPlanningNode::GetApertureOrigin (int dim)
{
  return this->ApertureOrigin[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetApertureOrigin (const float* position)
{
  for (int d = 0; d < 2; d++) {
  this->ApertureOrigin[d] = position[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetApertureOrigin (const double* position)
{
  for (int d = 0; d < 2; d++) {
    this->ApertureOrigin[d] = position[d];
  }
}

//----------------------------------------------------------------------------
const int* vtkMRMLExternalBeamPlanningNode::GetApertureDim ()
{
  return this->ApertureDim;
}

//----------------------------------------------------------------------------
int vtkMRMLExternalBeamPlanningNode::GetApertureDim (int dim)
{
  return this->ApertureDim[dim];
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetApertureDim (const int* dim)
{
  for (int d = 0; d < 2; d++) 
  {
    this->ApertureDim[d] = dim[d];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::UpdateApertureParameters()
{
  double origin[2] = {-this->X1Jaw * this->ApertureOffset / this->SAD , -this->Y1Jaw * this->ApertureOffset / this->SAD };
  this->SetApertureOrigin(origin);

  double spacing_at_aperture[2] = {this->ApertureSpacingAtIso[0] * this->ApertureOffset / this->SAD, this->ApertureSpacingAtIso[1] * this->ApertureOffset / this->SAD};
  this->SetApertureSpacing(spacing_at_aperture);

  int dim[2] = { (int) ((this->X2Jaw + this->X1Jaw) / this->ApertureSpacingAtIso[0] +1 ), (int) ((this->X2Jaw + this->X1Jaw) / this->ApertureSpacingAtIso[0] +1 )};
  this->SetApertureDim(dim);
}
