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

#ifndef __qSlicerSubjectHierarchyRegisterPlugin_h
#define __qSlicerSubjectHierarchyRegisterPlugin_h

// SlicerRt includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerSubjectHierarchyModulePluginsExport.h"

class qSlicerSubjectHierarchyRegisterPluginPrivate;
class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLSubjectHierarchyNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class Q_SLICER_SUBJECTHIERARCHY_PLUGINS_EXPORT qSlicerSubjectHierarchyRegisterPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyRegisterPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyRegisterPlugin();

public:
  /// Get node context menu item actions to add to tree view
  virtual QList<QAction*> nodeContextMenuActions()const;

  /// Show context menu actions valid for  given subject hierarchy node.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node);

protected slots:
  /// Start registration process by selecting the current node as the 'from' node.
  /// Saves node in \sa m_RegisterFromNode and shows "Register * to this using..."
  /// context menu option offering the possible registration methods,
  void registerCurrentNodeTo();

  /// Register saved 'from' node to current node using image based rigid registration.
  /// (Switch to registration module corresponding to selected method, set chosen
  /// input nodes, offer a best guess parameter set based on modalities etc.)
  void registerImageBasedRigid();

  /// Register saved 'from' node to current node using image based BSpline registration
  /// (Switch to registration module corresponding to selected method, set chosen
  /// input nodes, offer a best guess parameter set based on modalities etc.)
  void registerImageBasedBSpline();

  /// Register saved 'from' node to current node using interactive landmark registration
  /// (Switch to registration module corresponding to selected method, set chosen
  /// input nodes, offer a best guess parameter set based on modalities etc.)
  void registerInteractiveLandmark();

protected:
  vtkMRMLSubjectHierarchyNode* m_RegisterFromNode;

protected:
  QScopedPointer<qSlicerSubjectHierarchyRegisterPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyRegisterPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyRegisterPlugin);
};

//ETX

#endif
