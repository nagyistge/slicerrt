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

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyDefaultPlugin.h"
#include "qSlicerSubjectHierarchyPluginHandler.h"

// SubjectHierarchy MRML includes
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// Qt includes
#include <QDebug>
#include <QAction>
#include <QIcon>
#include <QStandardItem>

// MRMLWidgets includes
#include <qMRMLSceneModel.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

//----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class qSlicerSubjectHierarchyDefaultPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyDefaultPlugin);
protected:
  qSlicerSubjectHierarchyDefaultPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyDefaultPluginPrivate(qSlicerSubjectHierarchyDefaultPlugin& object);
  ~qSlicerSubjectHierarchyDefaultPluginPrivate();
  void init();
public:
  QIcon StudyIcon;
  QIcon SubjectIcon;

  QIcon VisibleIcon;
  QIcon HiddenIcon;
  QIcon PartiallyVisibleIcon;

  QAction* CreateSubjectAction;
  QAction* CreateStudyAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyDefaultPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDefaultPluginPrivate::qSlicerSubjectHierarchyDefaultPluginPrivate(qSlicerSubjectHierarchyDefaultPlugin& object)
: q_ptr(&object)
{
  this->StudyIcon = QIcon(":Icons/Study.png");
  this->SubjectIcon = QIcon(":Icons/Subject.png");

  this->CreateSubjectAction = NULL;
  this->CreateStudyAction = NULL;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyDefaultPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyDefaultPlugin);

  this->CreateSubjectAction = new QAction("Create new subject",q);
  QObject::connect(this->CreateSubjectAction, SIGNAL(triggered()), q, SLOT(createChildForCurrentNode()));

  this->CreateStudyAction = new QAction("Create new study",q);
  QObject::connect(this->CreateStudyAction, SIGNAL(triggered()), q, SLOT(createChildForCurrentNode()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDefaultPluginPrivate::~qSlicerSubjectHierarchyDefaultPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDefaultPlugin::qSlicerSubjectHierarchyDefaultPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyDefaultPluginPrivate(*this) )
{
  this->m_Name = QString("Default");

  // Scene -> Subject
  this->m_ChildLevelMap.insert( QString(),
    vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT );
  // Subject -> Study
  this->m_ChildLevelMap.insert( vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT,
    vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY );

  Q_D(qSlicerSubjectHierarchyDefaultPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyDefaultPlugin::~qSlicerSubjectHierarchyDefaultPlugin()
{
}

//----------------------------------------------------------------------------
void qSlicerSubjectHierarchyDefaultPlugin::setDefaultVisibilityIcons(QIcon visibleIcon, QIcon hiddenIcon, QIcon partiallyVisibleIcon)
{
  Q_D(qSlicerSubjectHierarchyDefaultPlugin);

  d->VisibleIcon = visibleIcon;
  d->HiddenIcon = hiddenIcon;
  d->PartiallyVisibleIcon = partiallyVisibleIcon;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyDefaultPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_UNUSED(node);

  // The default Subject Hierarchy plugin is never selected by confidence number it returns
  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyDefaultPlugin::roleForPlugin()const
{
  return "Generic";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyDefaultPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyDefaultPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyDefaultPlugin);

  // Subject and Study icons
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT))
  {
    item->setIcon(d->SubjectIcon);
    return true;
  }
  else if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    item->setIcon(d->StudyIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDefaultPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  Q_D(qSlicerSubjectHierarchyDefaultPlugin);

  // Default is the eye icon that shows the visibility of the whole branch
  int visible = this->getDisplayVisibility(node);

  // It should be fine to set the icon even if it is the same, but due
  // to a bug in Qt (http://bugreports.qt.nokia.com/browse/QTBUG-20248),
  // it would fire a superflous itemChanged() signal.
  if (item->data(qMRMLSceneModel::VisibilityRole).isNull()
    || item->data(qMRMLSceneModel::VisibilityRole).toInt() != visible)
  {
    item->setData(visible, qMRMLSceneModel::VisibilityRole);
    switch (visible)
    {
    case 0:
      item->setIcon(d->HiddenIcon);
      break;
    case 1:
      item->setIcon(d->VisibleIcon);
      break;
    case 2:
      item->setIcon(d->PartiallyVisibleIcon);
      break;
    default:
      break;
    }
  }
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyDefaultPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyDefaultPlugin);

  QList<QAction*> actions;
  actions << d->CreateStudyAction;
  return actions;
}

//-----------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyDefaultPlugin::sceneContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyDefaultPlugin);

  QList<QAction*> actions;
  actions << d->CreateSubjectAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyDefaultPlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyDefaultPlugin);

  d->CreateSubjectAction->setVisible(false);
  d->CreateStudyAction->setVisible(false);

  // Scene
  if (!node)
  {
    d->CreateSubjectAction->setVisible(true);
  }
  // Subject
  else if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT))
  {
    d->CreateStudyAction->setVisible(true);
  }
}
