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

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDICOMPlugin.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Qt includes
#include <QDebug>
#include <QAction>
#include <QStandardItem>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLStorableNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class qSlicerSubjectHierarchyDICOMPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyDICOMPlugin);
protected:
  qSlicerSubjectHierarchyDICOMPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyDICOMPluginPrivate(qSlicerSubjectHierarchyDICOMPlugin& object);
  ~qSlicerSubjectHierarchyDICOMPluginPrivate();
  void init();
public:
  QIcon PatientIcon;

  QAction* CreateGenericSeriesAction;
  QAction* CreateGenericSubseriesAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyDICOMPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPluginPrivate::qSlicerSubjectHierarchyDICOMPluginPrivate(qSlicerSubjectHierarchyDICOMPlugin& object)
: q_ptr(&object)
{
  this->PatientIcon = QIcon(":Icons/Patient.png");

  this->CreateGenericSeriesAction = NULL;
  this->CreateGenericSubseriesAction = NULL;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyDICOMPlugin);

  this->CreateGenericSeriesAction = new QAction("Create child generic series",q);
  QObject::connect(this->CreateGenericSeriesAction, SIGNAL(triggered()), q, SLOT(createChildForCurrentNode()));

  this->CreateGenericSubseriesAction = new QAction("Create child generic subseries",q);
  QObject::connect(this->CreateGenericSubseriesAction, SIGNAL(triggered()), q, SLOT(createChildForCurrentNode()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPluginPrivate::~qSlicerSubjectHierarchyDICOMPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPlugin::qSlicerSubjectHierarchyDICOMPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyDICOMPluginPrivate(*this) )
{
  this->m_Name = QString("DICOM");

  // Scene (empty level) -> Subject
  this->m_ChildLevelMap.insert( "", vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT );
  // Subject -> Study
  this->m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT,
    vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY );
  // Study -> Series
  this->m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY,
    vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES );
  // Series -> Subseries
  this->m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES,
    vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES );
  // Subseries -> Subseries
  this->m_ChildLevelMap.insert( vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES,
    vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES );

  Q_D(qSlicerSubjectHierarchyDICOMPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDICOMPlugin::~qSlicerSubjectHierarchyDICOMPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyDICOMPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyDICOMPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  // Subject level
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT))
  {
    return 0.7;
  }
  // Study level (so that creation of a generic series is possible)
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    return 0.3;
  }
  // Series level
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES))
  {
    return 0.3;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyDICOMPlugin::roleForPlugin()const
{
  // Get current node to determine tole
  //TODO: This is a workaround, needs to be fixed (each plugin should provide only one role!)
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyDICOMPlugin::roleForPlugin: Invalid current node!";
    return "Error!";
  }

  // Subject level
  if (currentNode->IsLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT))
  {
    return "Patient";
  }
  // Study level (so that creation of a generic series is possible)
  if (currentNode->IsLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    return "Study";
  }
  // Series level
  if (currentNode->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES))
  {
    return "Generic series";
  }

  return QString("Error!");
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyDICOMPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyDICOMPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyDICOMPlugin);

  // Patient icon
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT))
  {
    item->setIcon(d->PatientIcon);
    return true;
  }
  // Study icon
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setIcon(node, item);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  // Have the default plugin (which is not registered) take care of this
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyDICOMPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyDICOMPlugin);

  QList<QAction*> actions;
  actions << d->CreateGenericSeriesAction << d->CreateGenericSubseriesAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyDICOMPlugin);
  this->hideAllContextMenuActions();

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  // Study
  if (node->IsLevel(vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    d->CreateGenericSeriesAction->setVisible(true);
  }
  // Series or Subseries
  else if ( node->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    || node->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES) )
  {
    d->CreateGenericSubseriesAction->setVisible(true);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDICOMPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  //TODO: Show DICOM tag editor when implemented
}
