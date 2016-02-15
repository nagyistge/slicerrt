/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLSegmentationDisplayNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/

#ifndef __vtkMRMLSegmentationDisplayNode_h
#define __vtkMRMLSegmentationDisplayNode_h

#include "vtkMRMLLabelMapVolumeDisplayNode.h"

#include "vtkSlicerSegmentationsModuleMRMLExport.h"

#include <set>

class vtkMRMLColorTableNode;
class vtkVector3d;

/// \ingroup Segmentations
/// \brief MRML node for representing segmentation display attributes.
///
/// vtkMRMLSegmentationDisplayNode nodes describe how volume is displayed.
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkMRMLSegmentationDisplayNode : public vtkMRMLLabelMapVolumeDisplayNode
{
public:
  // Define constants
  static const std::string GetColorTableNodeNamePostfix() { return "_ColorTable"; };
  static const std::string GetColorIndexTag() { return "ColorIndex"; };
  static const char* GetSegmentationColorNameBackground() { return "Background"; };
  static const char* GetSegmentationColorNameRemoved() { return "Removed"; };
  static unsigned short GetSegmentationColorIndexBackground() { return 0; };

  /// Display properties per segment
  struct SegmentDisplayProperties
  {
    /// Displayed segment color (may be different than default color stored in segment)
    double Color[3];
    /// Visibility
    bool Visible3D;
    bool Visible2DFill; // This one is used for labelmap volume related operations (color table, merged labelmap)
    bool Visible2DOutline;
    /// Opacity
    double Opacity3D;
    double Opacity2DFill; // This one is used for labelmap volume related operations (color table, merged labelmap)
    double Opacity2DOutline;
  };

  typedef std::map<std::string, SegmentDisplayProperties> SegmentDisplayPropertiesMap;

public:
  static vtkMRMLSegmentationDisplayNode *New();
  vtkTypeMacro(vtkMRMLSegmentationDisplayNode,vtkMRMLLabelMapVolumeDisplayNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() { return "SegmentationDisplay"; };

public:
  /// Get name of representation that is displayed in the 2D view if exists
  /// To get the actually displayed 2D representation call \sa 
  vtkGetStringMacro(PreferredDisplayRepresentationName2D);
  /// Set name of representation that is displayed in the 2D view if exists
  vtkSetStringMacro(PreferredDisplayRepresentationName2D);
  /// Get name of representation that is displayed in the 3D view if exists
  vtkGetStringMacro(PreferredDisplayRepresentationName3D);
  /// Set name of representation that is displayed in the 3D view if exists
  vtkSetStringMacro(PreferredDisplayRepresentationName3D);

public:
  /// Create color table node for segmentation
  /// First two values are fixed: 0=Background, 1=Invalid
  /// The subsequent colors correspond to the segments in order of segment indices.
  /// \param segmentationNodeName Name of the segmentation node that is set to the color node with a postfix
  virtual vtkMRMLColorTableNode* CreateColorTableNode(const char* segmentationNodeName);

  /// Convenience function to set color to a segment.
  /// Makes changes in the color table node associated to the segmentation node.
  bool SetSegmentColor(std::string segmentId, double r, double g, double b, double a = 1.0);

  /// Get segment display properties for a specific segment
  /// \param segmentID Identifier of segment of which the properties are queried
  /// \param color Output argument for segment color
  /// \param polyDataOpacity Output argument for poly data opacity
  /// \return Success flag
  bool GetSegmentDisplayProperties(std::string segmentID, SegmentDisplayProperties &properties);

  /// Set segment display properties. Add new entry if not in list already
  void SetSegmentDisplayProperties(std::string segmentID, SegmentDisplayProperties &properties);

  /// Remove segment display properties
  void RemoveSegmentDisplayProperties(std::string segmentID);

  /// Clear segment display properties
  void ClearSegmentDisplayProperties();

  /// Determine and set automatic opacities for segments using topological hierarchies.
  /// Stores value in opacity component of \sa SegmentDisplayProperties.
  /// \return Success flag
  bool CalculateAutoOpacitiesForSegments();

  /// Collect representation names that are stored as poly data
  void GetPolyDataRepresentationNames(std::set<std::string> &representationNames);

  /// Decide which poly data representation to use for 3D display.
  /// If preferred representation exists \sa PreferredDisplayRepresentationName3D, then return that.
  /// Otherwise if master representation is a poly data then return master representation type.
  /// Otherwise return first poly data representation if any.
  /// Otherwise return empty string meaning there is no poly data representation to display.
  std::string GetDisplayRepresentationName3D();

  /// Decide which representation to use for 2D display.
  /// If preferred representation exists \sa PreferredDisplayRepresentationName2D, then return that.
  /// Otherwise return master representation.
  std::string GetDisplayRepresentationName2D();

// Python compatibility functions
public:
  /// Get segment color by segment ID. Convenience function for python compatibility.
  /// \return Segment color if segment found, otherwise the pre-defined invalid color
  vtkVector3d GetSegmentColor(std::string segmentID);
  /// Set segment color by segment ID. Convenience function for python compatibility.
  void SetSegmentColor(std::string segmentID, vtkVector3d color);

  /// Get segment 3D visibility by segment ID. Convenience function for python compatibility.
  /// \return Segment 3D visibility if segment found, otherwise false
  bool GetSegmentVisibility3D(std::string segmentID);
  /// Set segment 3D visibility by segment ID. Convenience function for python compatibility.
  void SetSegmentVisibility3D(std::string segmentID, bool visible);

  /// Get segment 2D fill visibility by segment ID. Convenience function for python compatibility.
  /// \return Segment 2D fill visibility if segment found, otherwise false
  bool GetSegmentVisibility2DFill(std::string segmentID);
  /// Set segment 2D fill visibility by segment ID. Convenience function for python compatibility.
  void SetSegmentVisibility2DFill(std::string segmentID, bool visible);

  /// Get segment 2D outline visibility by segment ID. Convenience function for python compatibility.
  /// \return Segment 2D outline visibility if segment found, otherwise false
  bool GetSegmentVisibility2DOutline(std::string segmentID);
  /// Set segment 2D outline visibility by segment ID. Convenience function for python compatibility.
  void SetSegmentVisibility2DOutline(std::string segmentID, bool visible);

  /// Get segment 3D opacity by segment ID. Convenience function for python compatibility.
  /// \return Segment 3D opacity if segment found, otherwise false
  double GetSegmentOpacity3D(std::string segmentID);
  /// Set segment 3D opacity by segment ID. Convenience function for python compatibility.
  void SetSegmentOpacity3D(std::string segmentID, double opacity);

  /// Get segment 2D fill opacity by segment ID. Convenience function for python compatibility.
  /// \return Segment 2D fill opacity if segment found, otherwise false
  double GetSegmentOpacity2DFill(std::string segmentID);
  /// Set segment 2D fill opacity by segment ID. Convenience function for python compatibility.
  void SetSegmentOpacity2DFill(std::string segmentID, double opacity);

  /// Get segment 2D outline opacity by segment ID. Convenience function for python compatibility.
  /// \return Segment 2D outline opacity if segment found, otherwise false
  double GetSegmentOpacity2DOutline(std::string segmentID);
  /// Set segment 2D outline opacity by segment ID. Convenience function for python compatibility.
  void SetSegmentOpacity2DOutline(std::string segmentID, double opacity);

protected:
  /// Set segment color in associated color table
  /// \return Success flag
  bool SetSegmentColorTableEntry(std::string segmentId, double r, double g, double b, double a);

protected:
  vtkMRMLSegmentationDisplayNode();
  virtual ~vtkMRMLSegmentationDisplayNode();
  vtkMRMLSegmentationDisplayNode(const vtkMRMLSegmentationDisplayNode&);
  void operator=(const vtkMRMLSegmentationDisplayNode&);

protected:
  /// Name of representation that is displayed in 2D views as outline or filled area
  /// if exists. If does not exist, then master representation is displayed.
  char* PreferredDisplayRepresentationName2D;

  /// Name of representation that is displayed as poly data in the 3D view.
  /// If does not exist, then master representation is displayed if poly data,
  /// otherwise the first poly data representation if any.
  char* PreferredDisplayRepresentationName3D;

  /// List of segment display properties for all segments in associated segmentation.
  /// Maps segment identifier string (segment name by default) to properties.
  SegmentDisplayPropertiesMap SegmentationDisplayProperties;
};

#endif
