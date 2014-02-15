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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerSubjectHierarchyAbstractPlugin_h
#define __qSlicerSubjectHierarchyAbstractPlugin_h

// Qt includes
#include <QObject>
#include <QMap>
#include <QStringList>

// SlicerRt includes
#include "qSlicerSubjectHierarchyModulePluginsExport.h"

class vtkObject;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;
class QStandardItem;
class QAction;

/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
/// \brief Abstract plugin for handling Subject Hierarchy nodes
///
/// This class provides an interface and some default implementations for the common operations on
/// subject hierarchy nodes. To exercise the default implementations, a Default plugin \sa qSlicerSubjectHierarchyDefaultPlugin
/// has to be created.
///
/// Note about confidence values (\sa canAddNodeToSubjectHierarchy \sa canReparentNodeInsideSubjectHierarchy \sa canOwnSubjectHierarchyNode):
/// The confidence value is a floating point number between 0.0 and 1.0. Meaning of some typical values:
/// 0.0 = The plugin cannot handle the node in question at all
/// 0.3 = It is likely that other plugins will be able to handle the node in question better (typical value for plugins for generic types, such as Volumes)
/// 0.5 = The plugin has equal chance to handle this node as others (an example can be color table node)
/// 0.7 = The plugin is likely be the only one that can handle the node in question, but there is a chance that other plugins can do that too
/// 1.0 = The node in question can only be handled by the plugin (by node type or identifier attribute)
///
class Q_SLICER_SUBJECTHIERARCHY_PLUGINS_EXPORT qSlicerSubjectHierarchyAbstractPlugin : public QObject
{
  Q_OBJECT

  /// This property stores the name of the plugin
  /// Cannot be empty.
  /// \sa name()
  Q_PROPERTY(QString name READ name)

  /// This property holds the name list of the plugin dependencies.
  /// There is no dependency cycle check, so special care must be taken to
  /// avoid infinite loop.
  /// By default, there is no dependency
  Q_PROPERTY(QStringList dependencies READ dependencies)

public:
  typedef QObject Superclass;
  qSlicerSubjectHierarchyAbstractPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyAbstractPlugin();

// Parenting related pure virtual methods
public:
  /// Determines if a non subject hierarchy node can be placed in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// Most plugins do not perform steps additional to the default, so the default implementation returns a 0
  /// confidence value, which can be overridden in plugins that do handle special cases.
  /// \param node Node to be added to the hierarchy
  /// \param parent Prospective parent of the node to add.
  ///   Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canAddNodeToSubjectHierarchy(vtkMRMLNode* node , vtkMRMLSubjectHierarchyNode* parent=NULL);

  /// Add a node to subject hierarchy under a specified parent node. If added non subject hierarchy nodes
  ///   have certain steps to perform when adding them in Subject Hierarchy, those steps take place here.
  /// \return True if added successfully, false otherwise
  virtual bool addNodeToSubjectHierarchy(vtkMRMLNode* node, vtkMRMLSubjectHierarchyNode* parent);

  /// Determines if a subject hierarchy node can be reparented in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// Most plugins do not perform steps additional to the default, so the default implementation returns a 0
  /// confidence value, which can be overridden in plugins that do handle special cases.
  /// \param node Node to be reparented in the hierarchy
  /// \param parent Prospective parent of the node to reparent.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent);

  /// Reparent a node that was already in the subject hierarchy under a new parent.
  /// \return True if reparented successfully, false otherwise
  virtual bool reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent);

// General (ownable) pure virtual methods
public:
  /// Determines if the actual plugin can handle a subject hierarchy node. The plugin with
  /// the highest confidence number will "own" the node in the subject hierarchy (set icon, tooltip,
  /// set context menu etc.)
  /// \param node Note to handle in the subject hierarchy tree
  /// \param role Output argument for the role that the plugin assigns to the subject hierarchy node
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node, QString &role=QString()) = 0;

  /// Set icon of a owned subject hierarchy node
  /// \return Flag indicating whether setting an icon was successful
  virtual bool setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item) = 0;

  /// Set visibility icon of a owned subject hierarchy node
  virtual void setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item) = 0;

// General (ownable) virtual methods with default implementation
public:
  /// Generate displayed name for the owned subject hierarchy node corresponding to its role.
  /// The default implementation removes the '_SubjectHierarchy' ending from the node's name.
  virtual QString displayedName(vtkMRMLSubjectHierarchyNode* node);

  /// Generate tooltip for a owned subject hierarchy node
  virtual QString tooltip(vtkMRMLSubjectHierarchyNode* node);

  /// Set display visibility of a owned subject hierarchy node
  virtual void setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible);

  /// Get display visibility of a owned subject hierarchy node
  /// \return Display visibility (0: hidden, 1: shown, 2: partially shown)
  virtual int getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node);

  /// Get node context menu item actions to add to tree view
  virtual QList<QAction*> nodeContextMenuActions()const;

  /// Get scene context menu item actions to add to tree view
  /// Separate method is needed for the scene, as its actions are set to the
  /// tree by a different method \sa nodeContextMenuActions
  virtual QList<QAction*> sceneContextMenuActions()const;

  /// Hide all context menu actions
  virtual void hideAllContextMenuActions() { };

  /// Show context menu actions valid for handling a given subject hierarchy node.
  /// "Handling" includes features that are applied to the node (e.g. transform, convert, etc.)
  /// This function is only called for a node's owner plugin and its dependent plugins.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForHandlingNode(vtkMRMLSubjectHierarchyNode* node) { Q_UNUSED(node); };

  /// Show context menu actions valid for creating a child for a given subject hierarchy node.
  /// This function is called for all plugins, not just a node's owner plugin and its dependents,
  /// because it's not the node's ownership that determines what kind of children can be created
  /// to it, but the properties (level etc.) of the node
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForCreatingChildForNode(vtkMRMLSubjectHierarchyNode* node) { Q_UNUSED(node); };

  /// Export data associated to the owned subject hierarhcy node to DICOM
  virtual void exportNodeToDicom(vtkMRMLSubjectHierarchyNode* node);

// Utility functions
public:
  vtkMRMLSubjectHierarchyNode* createChildNode(vtkMRMLSubjectHierarchyNode* parentNode, QString nodeName, vtkMRMLNode* associatedNode=NULL);

  void emitOwnerPluginChanged(vtkObject* node, void* callData);

public:
  /// Get the name of the plugin
  virtual QString name()const;

  /// Get the list of plugin dependencies
  virtual QStringList dependencies()const;

  /// Get child level map
  QMap<QString, QString> childLevelMap() { return m_ChildLevelMap; };

signals:
  /// Signal requesting expanding of the subject hierarchy tree item belonging to a node
  void requestExpandNode(vtkMRMLSubjectHierarchyNode* node);

  /// Signal that is emitted when a node changes owner plugin
  /// \param node Subject hierarchy node changing owner plugin
  /// \callData Name of the old plugin (the name of the new plugin can be get from the node)
  void ownerPluginChanged(vtkObject* node, void* callData);

protected:
  /// Get child level according to child level map of the current plugin
  /// (no search is done in the dependencies)
  virtual QString childLevel(QString parentLevel);

protected slots:
  /// Create supported child for the current node (which is selected in the tree)
  void createChildForCurrentNode();

protected:
  /// Name of the plugin
  QString m_Name;

  /// Map assigning a child level to a parent level for the plugin
  QMap<QString, QString> m_ChildLevelMap;

private:
  qSlicerSubjectHierarchyAbstractPlugin(const qSlicerSubjectHierarchyAbstractPlugin&); // Not implemented
  void operator=(const qSlicerSubjectHierarchyAbstractPlugin&); // Not implemented  
};

#endif
