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

#ifndef __qSlicerSubjectHierarchyVolumesPlugin_h
#define __qSlicerSubjectHierarchyVolumesPlugin_h

// SlicerRt includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerSubjectHierarchyModulePluginsExport.h"

class qSlicerSubjectHierarchyVolumesPluginPrivate;
class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLSubjectHierarchyNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class Q_SLICER_SUBJECTHIERARCHY_PLUGINS_EXPORT qSlicerSubjectHierarchyVolumesPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyVolumesPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyVolumesPlugin();

public:
  /// Determines if the actual plugin can handle a subject hierarchy node. The plugin with
  /// the highest confidence number will "own" the node in the subject hierarchy (set icon, tooltip,
  /// set context menu etc.)
  /// \param node Note to handle in the subject hierarchy tree
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node);

  /// Get role that the plugin assigns to the subject hierarchy node.
  ///   Each plugin should provide only one role.
  virtual const QString roleForPlugin()const;

  /// Set icon of a owned subject hierarchy node
  /// \return Flag indicating whether setting an icon was successful
  virtual bool setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item);

  /// Set visibility icon of a owned subject hierarchy node
  virtual void setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item);

  /// Open module belonging to node and set inputs in opened module
  virtual void editProperties(vtkMRMLSubjectHierarchyNode* node);

  /// Set display visibility of a owned subject hierarchy node
  virtual void setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible);

  /// Get display visibility of a owned subject hierarchy node
  /// \return Display visibility (0: hidden, 1: shown, 2: partially shown)
  virtual int getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node);

  /// Get node context menu item actions to add to tree view
  virtual QList<QAction*> nodeContextMenuActions()const;

  /// Show context menu actions valid for  given subject hierarchy node.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node);

protected:
  /// Show volume in slice viewers. The argument node becomes the background, and the previous
  /// background becomes the foreground with 50% transparency.
  void showVolume(vtkMRMLScalarVolumeNode* node, int visible=1);

  /// Update selection node based on current volumes visibility (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  void updateSelectionNodeBasedOnCurrentVolumesVisibility();
  /// Determine labelmap selection (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string getSelectedLabelmapVolumeNodeID();
  /// Determine background volume selection (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string getSelectedBackgroundVolumeNodeID();
  /// Determine foreground volume selection (if the selection is different in the slice viewers, then the first one is set)
  /// TODO: This is a workaround (http://www.na-mic.org/Bug/view.php?id=3551)
  std::string getSelectedForegroundVolumeNodeID();

protected slots:
  /// Toggle between labelmap outline display in the slice views
  void toggleLabelmapOutlineDisplay(bool checked);

  /// Show volumes in study. The first two regular volumes and the first labelmap is shown if there are more.
  /// Hides other volumes if there are less in the current study.
  void showVolumesInStudy();

protected:
  QScopedPointer<qSlicerSubjectHierarchyVolumesPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyVolumesPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyVolumesPlugin);
};

//ETX

#endif
