// .NAME vtkMRMLContourHierarchyNode - MRML node to represent hierarchy of Contours.
// .SECTION Description
// n/a
//

#ifndef __vtkMRMLContourHierarchyNode_h
#define __vtkMRMLContourHierarchyNode_h

#include "vtkMRMLHierarchyNode.h"

#include "vtkSlicerContoursModuleMRMLExport.h"

/// \ingroup Slicer_QtModules_Contour
class VTK_SLICER_CONTOURS_MODULE_MRML_EXPORT vtkMRMLContourHierarchyNode : public vtkMRMLHierarchyNode
{
public:
  static vtkMRMLContourHierarchyNode *New();
  vtkTypeMacro(vtkMRMLContourHierarchyNode,vtkMRMLHierarchyNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Propagate events generated in contour nodes
  virtual void ProcessMRMLEvents(vtkObject* caller, unsigned long event, void *callData);

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Get node XML tag name (like Volume, Contour)
  virtual const char* GetNodeTagName();

  /// Find all child contour nodes in the hierarchy
  virtual void GetChildrenContourNodes(vtkCollection *contours);

protected:
  vtkMRMLContourHierarchyNode();
  ~vtkMRMLContourHierarchyNode();
  vtkMRMLContourHierarchyNode(const vtkMRMLContourHierarchyNode&);
  void operator=(const vtkMRMLContourHierarchyNode&);

};

#endif
