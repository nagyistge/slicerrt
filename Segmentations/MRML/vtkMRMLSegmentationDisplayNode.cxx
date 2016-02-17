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

  This file was originally developed by Andras Lasso, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLColorTableNode.h>

// SegmentationCore includes
#include "vtkSegmentation.h"
#include "vtkOrientedImageData.h"
#include "vtkTopologicalHierarchy.h"
#include "vtkSegmentationConverterFactory.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkVersion.h>
#include <vtkLookupTable.h>
#include <vtkVector.h>

// STD includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentationDisplayNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentationDisplayNode::vtkMRMLSegmentationDisplayNode()
{
  this->PreferredDisplayRepresentationName2D = NULL;
  this->PreferredDisplayRepresentationName3D = NULL;
  this->SliceIntersectionVisibility = true;

  this->SegmentationDisplayProperties.clear();
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationDisplayNode::~vtkMRMLSegmentationDisplayNode()
{
  this->SetPreferredDisplayRepresentationName2D(NULL);
  this->SetPreferredDisplayRepresentationName3D(NULL);
  this->SegmentationDisplayProperties.clear();
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);

  of << indent << " PreferredDisplayRepresentationName2D=\"" << (this->PreferredDisplayRepresentationName2D ? this->PreferredDisplayRepresentationName2D : "NULL") << "\"";
  of << indent << " PreferredDisplayRepresentationName3D=\"" << (this->PreferredDisplayRepresentationName3D ? this->PreferredDisplayRepresentationName3D : "NULL") << "\"";

  of << indent << " SegmentationDisplayProperties=\"";
  for (SegmentDisplayPropertiesMap::iterator propIt = this->SegmentationDisplayProperties.begin();
    propIt != this->SegmentationDisplayProperties.end(); ++propIt)
  {
    of << vtkMRMLNode::URLEncodeString(propIt->first.c_str()) << " "
      << propIt->second.Color[0] << " " << propIt->second.Color[1] << " " << propIt->second.Color[2] << " "
      << (propIt->second.Visible3D ? "true" : "false") << " "
      << (propIt->second.Visible2DFill ? "true" : "false") << " "
      << (propIt->second.Visible2DOutline ? "true" : "false") << " "
      << propIt->second.Opacity3D << " " << propIt->second.Opacity2DFill << " " << propIt->second.Opacity2DOutline << "|";
  }
  of << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::ReadXMLAttributes(const char** atts)
{
  // Read all MRML node attributes from two arrays of names and values
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "PreferredDisplayRepresentationName2D")) 
    {
      this->SetPreferredDisplayRepresentationName2D(attValue);
    }
    else if (!strcmp(attName, "PreferredDisplayRepresentationName3D")) 
    {
      this->SetPreferredDisplayRepresentationName3D(attValue);
    }
    else if (!strcmp(attName, "SegmentationDisplayProperties")) 
    {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->SegmentationDisplayProperties.clear();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
      {
        std::stringstream segmentProps;
        segmentProps << valueStr.substr(0, separatorPosition);
        std::string id("");
        SegmentDisplayProperties props;
        std::string visible3DStr("");
        std::string visible2DFillStr("");
        std::string visible2DOutlineStr("");
        segmentProps >> id >> props.Color[0] >> props.Color[1] >> props.Color[2]
          >> visible3DStr >> visible2DFillStr >> visible2DOutlineStr
          >> props.Opacity3D >> props.Opacity2DFill >> props.Opacity2DOutline;
        props.Visible3D = (visible3DStr.compare("true") ? false : true);
        props.Visible2DFill = (visible2DFillStr.compare("true") ? false : true);
        props.Visible2DOutline = (visible2DOutlineStr.compare("true") ? false : true);
        this->SetSegmentDisplayProperties(vtkMRMLNode::URLDecodeString(id.c_str()), props);

        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
      }
      if (!valueStr.empty())
      {
        std::stringstream segmentProps;
        segmentProps << valueStr.substr(0, separatorPosition);
        std::string id("");
        SegmentDisplayProperties props;
        std::string visible3DStr("");
        std::string visible2DFillStr("");
        std::string visible2DOutlineStr("");
        segmentProps >> id >> props.Color[0] >> props.Color[1] >> props.Color[2]
          >> visible3DStr >> visible2DFillStr >> visible2DOutlineStr
          >> props.Opacity3D >> props.Opacity2DFill >> props.Opacity2DOutline;
        props.Visible3D = (visible3DStr.compare("true") ? false : true);
        props.Visible2DFill = (visible2DFillStr.compare("true") ? false : true);
        props.Visible2DOutline = (visible2DOutlineStr.compare("true") ? false : true);
        this->SetSegmentDisplayProperties(id, props);
      }
    }
  }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::Copy(vtkMRMLNode *anode)
{
  bool wasModifying = this->StartModify();
  this->Superclass::Copy(anode);

  vtkMRMLSegmentationDisplayNode *node = vtkMRMLSegmentationDisplayNode::SafeDownCast(anode);
  if (node)
  {
    this->SetPreferredDisplayRepresentationName2D(node->GetPreferredDisplayRepresentationName2D());
    this->SetPreferredDisplayRepresentationName3D(node->GetPreferredDisplayRepresentationName3D());
    this->SegmentationDisplayProperties = node->SegmentationDisplayProperties;
  }

  this->EndModify(wasModifying);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " PreferredDisplayRepresentationName2D:   " << (this->PreferredDisplayRepresentationName2D ? this->PreferredDisplayRepresentationName2D : "NULL") << "\n";
  os << indent << " PreferredDisplayRepresentationName3D:   " << (this->PreferredDisplayRepresentationName3D ? this->PreferredDisplayRepresentationName3D : "NULL") << "\n";

  os << indent << " SegmentationDisplayProperties:\n";
  for (SegmentDisplayPropertiesMap::iterator propIt = this->SegmentationDisplayProperties.begin();
    propIt != this->SegmentationDisplayProperties.end(); ++propIt)
  {
    os << indent << "   SegmentID=" << propIt->first << ", Color=("
       << propIt->second.Color[0] << "," << propIt->second.Color[1] << "," << propIt->second.Color[2]
       << "), Visible3D=" << (propIt->second.Visible3D ? "true" : "false") << ", Visible2DFill=" << (propIt->second.Visible2DFill ? "true" : "false") << ", Visible2DOutline=" << (propIt->second.Visible2DOutline ? "true" : "false")
       << ", Opacity3D=" << propIt->second.Opacity3D << ", Opacity2DFill=" << propIt->second.Opacity2DFill << ", Opacity2DOutline=" << propIt->second.Opacity2DOutline << "\n";
  }
}

//---------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLSegmentationDisplayNode::CreateColorTableNode(const char* segmentationNodeName)
{
  if (!this->Scene)
  {
    vtkErrorMacro("CreateColorTableNode: Invalid MRML scene!");
    return NULL;
  }

  vtkSmartPointer<vtkMRMLColorTableNode> segmentationColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string segmentationColorTableNodeName = std::string(segmentationNodeName ? segmentationNodeName : "Segmentation") + GetColorTableNodeNamePostfix();
  segmentationColorTableNodeName = this->Scene->GenerateUniqueName(segmentationColorTableNodeName);
  segmentationColorTableNode->SetName(segmentationColorTableNodeName.c_str());
  segmentationColorTableNode->HideFromEditorsOff();
  segmentationColorTableNode->SetTypeToUser();
  segmentationColorTableNode->NamesInitialisedOn();
  segmentationColorTableNode->SetAttribute("Category", this->GetNodeTagName());
  this->Scene->AddNode(segmentationColorTableNode);

  // Initialize color table
  segmentationColorTableNode->SetNumberOfColors(1);
  segmentationColorTableNode->GetLookupTable()->SetTableRange(0,0);
  segmentationColorTableNode->AddColor(this->GetSegmentationColorNameBackground(), 0.0, 0.0, 0.0, 0.0); // Black background

  // Set reference to color table node
  this->SetAndObserveColorNodeID(segmentationColorTableNode->GetID());

  return segmentationColorTableNode;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationDisplayNode::SetSegmentColor(std::string segmentId, double r, double g, double b, double a/* = 1.0*/)
{
  // Set color in display properties
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentId);
  if (propsIt != this->SegmentationDisplayProperties.end())
  {
    propsIt->second.Color[0] = r;
    propsIt->second.Color[1] = g;
    propsIt->second.Color[2] = b;
    propsIt->second.Opacity2DFill = a;
  }

  // Set color in color table too
  this->SetSegmentColorTableEntry(segmentId, r, g, b, a);

  return true;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationDisplayNode::SetSegmentColorTableEntry(std::string segmentId, double r, double g, double b, double a)
{
  // Set color in color table (for merged labelmap)
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetColorNode());
  if (!colorTableNode)
  {
    if (this->Scene && !this->Scene->IsImporting())
    {
      vtkErrorMacro("SetSegmentColorTableEntry: No color table node associated with segmentation. Maybe CreateColorTableNode was not called?");
    }
    return false;
  }

  // Look up segment color in color table node (-1 if not found)
  int colorIndex = colorTableNode->GetColorIndexByName(segmentId.c_str());
  if (colorIndex < 0)
  {
    vtkWarningMacro("SetSegmentColorTableEntry: No color table entry found for segment " << segmentId);
    return false;
  }
  // Do not support opacity in color table. If advanced display is needed then use the displayable manager
  colorTableNode->SetColor(colorIndex, r, g, b, 1.0);
  return true;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationDisplayNode::GetSegmentDisplayProperties(std::string segmentId, SegmentDisplayProperties &properties)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentId);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    return false;
  }

  properties.Color[0] = propsIt->second.Color[0];
  properties.Color[1] = propsIt->second.Color[1];
  properties.Color[2] = propsIt->second.Color[2];
  properties.Visible3D = propsIt->second.Visible3D;
  properties.Visible2DFill = propsIt->second.Visible2DFill;
  properties.Visible2DOutline = propsIt->second.Visible2DOutline;
  properties.Opacity3D = propsIt->second.Opacity3D;
  properties.Opacity2DFill = propsIt->second.Opacity2DFill;
  properties.Opacity2DOutline = propsIt->second.Opacity2DOutline;

  return true;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentDisplayProperties(std::string segmentId, SegmentDisplayProperties &properties)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentId);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    // If not found then add
    SegmentDisplayProperties newPropertiesEntry;
    newPropertiesEntry.Color[0] = properties.Color[0];
    newPropertiesEntry.Color[1] = properties.Color[1];
    newPropertiesEntry.Color[2] = properties.Color[2];
    newPropertiesEntry.Visible3D = properties.Visible3D;
    newPropertiesEntry.Visible2DFill = properties.Visible2DFill;
    newPropertiesEntry.Visible2DOutline = properties.Visible2DOutline;
    newPropertiesEntry.Opacity3D = properties.Opacity3D;
    newPropertiesEntry.Opacity2DFill = properties.Opacity2DFill;
    newPropertiesEntry.Opacity2DOutline = properties.Opacity2DOutline;
    this->SegmentationDisplayProperties[segmentId] = newPropertiesEntry;
  }
  else
  {
    // If found then replace values
    propsIt->second.Color[0] = properties.Color[0];
    propsIt->second.Color[1] = properties.Color[1];
    propsIt->second.Color[2] = properties.Color[2];
    propsIt->second.Visible3D = properties.Visible3D;
    propsIt->second.Visible2DFill = properties.Visible2DFill;
    propsIt->second.Visible2DOutline = properties.Visible2DOutline;
    propsIt->second.Opacity3D = properties.Opacity3D;
    propsIt->second.Opacity2DFill = properties.Opacity2DFill;
    propsIt->second.Opacity2DOutline = properties.Opacity2DOutline;
  }

  // Set color in color table too
  this->SetSegmentColorTableEntry(segmentId,
    properties.Color[0], properties.Color[1], properties.Color[2], properties.Opacity2DFill);

  this->Modified();
}

//---------------------------------------------------------------------------
vtkVector3d vtkMRMLSegmentationDisplayNode::GetSegmentColor(std::string segmentID)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentID);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    vtkErrorMacro("GetSegmentColor: No display properties found for segment with ID " << segmentID);
    vtkVector3d color(vtkSegment::SEGMENT_COLOR_VALUE_INVALID[0], vtkSegment::SEGMENT_COLOR_VALUE_INVALID[1], vtkSegment::SEGMENT_COLOR_VALUE_INVALID[2]);
    return color;
  }

  vtkVector3d color(propsIt->second.Color[0], propsIt->second.Color[1], propsIt->second.Color[2]);
  return color;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentColor(std::string segmentID, vtkVector3d color)
{
  SegmentDisplayProperties properties;
  if (!this->GetSegmentDisplayProperties(segmentID, properties))
    {
    return;
    }
  properties.Color[0] = color.GetX();
  properties.Color[1] = color.GetY();
  properties.Color[2] = color.GetZ();
  this->SetSegmentDisplayProperties(segmentID, properties);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationDisplayNode::GetSegmentVisibility3D(std::string segmentID)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentID);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    vtkErrorMacro("GetSegmentVisibility3D: No display properties found for segment with ID " << segmentID);
    return false;
  }
  return propsIt->second.Visible3D;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentVisibility3D(std::string segmentID, bool visible)
{
  SegmentDisplayProperties properties;
  if (!this->GetSegmentDisplayProperties(segmentID, properties))
    {
    return;
    }
  properties.Visible3D = visible;
  this->SetSegmentDisplayProperties(segmentID, properties);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationDisplayNode::GetSegmentVisibility2DFill(std::string segmentID)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentID);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    vtkErrorMacro("GetSegmentVisibility2DFill: No display properties found for segment with ID " << segmentID);
    return false;
  }
  return propsIt->second.Visible2DFill;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentVisibility2DFill(std::string segmentID, bool visible)
{
  SegmentDisplayProperties properties;
  if (!this->GetSegmentDisplayProperties(segmentID, properties))
    {
    return;
    }
  properties.Visible2DFill = visible;
  this->SetSegmentDisplayProperties(segmentID, properties);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationDisplayNode::GetSegmentVisibility2DOutline(std::string segmentID)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentID);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    vtkErrorMacro("GetSegmentVisibility2DOutline: No display properties found for segment with ID " << segmentID);
    return false;
  }
  return propsIt->second.Visible2DOutline;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentVisibility2DOutline(std::string segmentID, bool visible)
{
  SegmentDisplayProperties properties;
  if (!this->GetSegmentDisplayProperties(segmentID, properties))
    {
    return;
    }
  properties.Visible2DOutline = visible;
  this->SetSegmentDisplayProperties(segmentID, properties);
}

//---------------------------------------------------------------------------
double vtkMRMLSegmentationDisplayNode::GetSegmentOpacity3D(std::string segmentID)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentID);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    vtkErrorMacro("GetSegmentOpacity3D: No display properties found for segment with ID " << segmentID);
    return 0.0;
  }
  return propsIt->second.Opacity3D;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentOpacity3D(std::string segmentID, double opacity)
{
  SegmentDisplayProperties properties;
  if (!this->GetSegmentDisplayProperties(segmentID, properties))
    {
    return;
    }
  properties.Opacity3D = opacity;
  this->SetSegmentDisplayProperties(segmentID, properties);
}

//---------------------------------------------------------------------------
double vtkMRMLSegmentationDisplayNode::GetSegmentOpacity2DFill(std::string segmentID)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentID);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    vtkErrorMacro("GetSegmentOpacity2DFill: No display properties found for segment with ID " << segmentID);
    return 0.0;
  }
  return propsIt->second.Opacity2DFill;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentOpacity2DFill(std::string segmentID, double opacity)
{
  SegmentDisplayProperties properties;
  if (!this->GetSegmentDisplayProperties(segmentID, properties))
    {
    return;
    }
  properties.Opacity2DFill = opacity;
  this->SetSegmentDisplayProperties(segmentID, properties);
}

//---------------------------------------------------------------------------
double vtkMRMLSegmentationDisplayNode::GetSegmentOpacity2DOutline(std::string segmentID)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentID);
  if (propsIt == this->SegmentationDisplayProperties.end())
  {
    vtkErrorMacro("GetSegmentOpacity2DOutline: No display properties found for segment with ID " << segmentID);
    return 0.0;
  }
  return propsIt->second.Opacity2DOutline;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::SetSegmentOpacity2DOutline(std::string segmentID, double opacity)
{
  SegmentDisplayProperties properties;
  if (!this->GetSegmentDisplayProperties(segmentID, properties))
    {
    return;
    }
  properties.Opacity2DOutline = opacity;
  this->SetSegmentDisplayProperties(segmentID, properties);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::RemoveSegmentDisplayProperties(std::string segmentId)
{
  SegmentDisplayPropertiesMap::iterator propsIt = this->SegmentationDisplayProperties.find(segmentId);
  if (propsIt != this->SegmentationDisplayProperties.end())
  {
    this->SegmentationDisplayProperties.erase(propsIt);
  }

  this->Modified();
}


//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::ClearSegmentDisplayProperties()
{
  this->SegmentationDisplayProperties.clear();
  this->Modified();
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationDisplayNode::CalculateAutoOpacitiesForSegments()
{
  // Get segmentation node
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(this->GetDisplayableNode());
  if (!segmentationNode)
  {
    vtkErrorMacro("CalculateAutoOpacitiesForSegments: No segmentation node associated to this display node!");
    return false;
  }

  // Make sure the requested representation exists
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation->CreateRepresentation(this->PreferredDisplayRepresentationName3D))
  {
    return false;
  }

  // Get displayed 3D representation (always poly data)
  std::string displayedPolyDataRepresentationName = this->GetDisplayRepresentationName3D();

  // Assemble segment poly datas into a collection that can be fed to topological hierarchy algorithm
  vtkSmartPointer<vtkPolyDataCollection> segmentPolyDataCollection = vtkSmartPointer<vtkPolyDataCollection>::New();
  for (SegmentDisplayPropertiesMap::iterator propIt = this->SegmentationDisplayProperties.begin();
    propIt != this->SegmentationDisplayProperties.end(); ++propIt)
  {
    // Get segment
    vtkSegment* currentSegment = segmentation->GetSegment(propIt->first);
    if (!currentSegment)
    {
      vtkErrorMacro("CalculateAutoOpacitiesForSegments: Mismatch in display properties and segments!");
      continue;
    }

    // Get poly data from segment
    vtkPolyData* currentPolyData = vtkPolyData::SafeDownCast(
      currentSegment->GetRepresentation(displayedPolyDataRepresentationName.c_str()) );
    if (!currentPolyData)
    {
      continue;
    }

    segmentPolyDataCollection->AddItem(currentPolyData);
  }
    
  // Set opacities according to topological hierarchy levels
  vtkSmartPointer<vtkTopologicalHierarchy> topologicalHierarchy = vtkSmartPointer<vtkTopologicalHierarchy>::New();
  topologicalHierarchy->SetInputPolyDataCollection(segmentPolyDataCollection);
  topologicalHierarchy->Update();
  vtkIntArray* levels = topologicalHierarchy->GetOutputLevels();

  // Determine number of levels
  int numberOfLevels = 0;
  for (int i=0; i<levels->GetNumberOfTuples(); ++i)
  {
    if (levels->GetValue(i) > numberOfLevels)
    {
      numberOfLevels = levels->GetValue(i);
    }
  }
  // Sanity check
  if (static_cast<vtkIdType>(this->SegmentationDisplayProperties.size()) != levels->GetNumberOfTuples())
  {
    vtkErrorMacro("CalculateAutoOpacitiesForSegments: Number of poly data colors does not match number of segment display properties!");
    return false;
  }

  // Set opacities into lookup table
  int idx = 0;
  SegmentDisplayPropertiesMap::iterator propIt;
  for (idx=0, propIt=this->SegmentationDisplayProperties.begin(); idx < levels->GetNumberOfTuples(); ++idx, ++propIt)
  {
    int level = levels->GetValue(idx);

    // The opacity level is set evenly distributed between 0 and 1 (excluding 0)
    // according to the topological hierarchy level of the segment
    double opacity = 1.0 - ((double)level) / (numberOfLevels+1);
    propIt->second.Opacity3D = opacity;
  }

  this->Modified();
  return true;
}

//---------------------------------------------------------------------------
std::string vtkMRMLSegmentationDisplayNode::GetDisplayRepresentationName3D()
{
  // Get segmentation node
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(this->GetDisplayableNode());
  if (!segmentationNode)
  {
    return "";
  }
  // If segmentation is empty then we cannot show poly data
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation || segmentation->GetNumberOfSegments() == 0)
  {
    return "";
  }

  // Assume the first segment contains the same name of representations as all segments (this should be the case by design)
  vtkSegment* firstSegment = segmentation->GetSegments().begin()->second;

  // If preferred representation is defined and exists then use that (double check it is poly data)
  if (this->PreferredDisplayRepresentationName3D)
  {
    vtkDataObject* preferredRepresentation = firstSegment->GetRepresentation(this->PreferredDisplayRepresentationName3D);
    if (vtkPolyData::SafeDownCast(preferredRepresentation))
    {
      return std::string(this->PreferredDisplayRepresentationName3D);
    }
  }

  // Otherwise if master representation is poly data then use that
  char* masterRepresentationName = segmentation->GetMasterRepresentationName();
  vtkDataObject* masterRepresentation = firstSegment->GetRepresentation(masterRepresentationName);
  if (vtkPolyData::SafeDownCast(masterRepresentation))
  {
    return std::string(masterRepresentationName);
  }

  // Otherwise return first poly data representation if any
  std::vector<std::string> containedRepresentationNames;
  segmentation->GetContainedRepresentationNames(containedRepresentationNames);
  for (std::vector<std::string>::iterator reprIt = containedRepresentationNames.begin();
    reprIt != containedRepresentationNames.end(); ++reprIt)
  {
    vtkDataObject* currentRepresentation = firstSegment->GetRepresentation(*reprIt);
    if (vtkPolyData::SafeDownCast(currentRepresentation))
    {
      return (*reprIt);
    }
  }
  
  // If no poly data representations are available, then return empty string
  // meaning there is no poly data representation to display
  return "";
}

//---------------------------------------------------------------------------
std::string vtkMRMLSegmentationDisplayNode::GetDisplayRepresentationName2D()
{
  // Get segmentation node
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(this->GetDisplayableNode());
  if (!segmentationNode)
  {
    return "";
  }
  // If segmentation is empty then return empty string
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation || segmentation->GetNumberOfSegments() == 0)
  {
    return "";
  }

  // If preferred 2D representation exists, then return that
  if (this->PreferredDisplayRepresentationName2D)
  {
    if (segmentation->ContainsRepresentation(this->PreferredDisplayRepresentationName2D))
    {
      return std::string(this->PreferredDisplayRepresentationName2D);
    }
  }

  // Otherwise return master representation
  return std::string(segmentation->GetMasterRepresentationName());
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationDisplayNode::GetPolyDataRepresentationNames(std::set<std::string> &representationNames)
{
  representationNames.clear();

  // Note: This function used to collect existing poly data representations from the segmentation
  // It was then decided that a preferred poly data representation can be selected regardless
  // its existence, thus the list is populated based on supported poly data representations.

  // Traverse converter rules to find supported poly data representations
  vtkSegmentationConverterFactory::RuleListType rules = vtkSegmentationConverterFactory::GetInstance()->GetConverterRules();
  for (vtkSegmentationConverterFactory::RuleListType::iterator ruleIt = rules.begin(); ruleIt != rules.end(); ++ruleIt)
  {
    vtkSegmentationConverterRule* currentRule = (*ruleIt);

    vtkSmartPointer<vtkDataObject> sourceObject = vtkSmartPointer<vtkDataObject>::Take(
      currentRule->ConstructRepresentationObjectByRepresentation(currentRule->GetSourceRepresentationName()) );
    vtkPolyData* sourcePolyData = vtkPolyData::SafeDownCast(sourceObject);
    if (sourcePolyData)
    {
      representationNames.insert(std::string(currentRule->GetSourceRepresentationName()));
    }

    vtkSmartPointer<vtkDataObject> targetObject = vtkSmartPointer<vtkDataObject>::Take(
      currentRule->ConstructRepresentationObjectByRepresentation(currentRule->GetTargetRepresentationName()) );
    vtkPolyData* targetPolyData = vtkPolyData::SafeDownCast(targetObject);
    if (targetPolyData)
    {
      representationNames.insert(std::string(currentRule->GetTargetRepresentationName()));
    }
  }
}
