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

#ifndef __qSlicerPatientHierarchyModuleWidgetsPlugin_h
#define __qSlicerPatientHierarchyModuleWidgetsPlugin_h

// Qt includes
#include <QDesignerCustomWidgetCollectionInterface>

// PatientHierarchy includes
#include "qMRMLPatientHierarchyTreeViewPlugin.h"

// \class Group the plugins in one library
class Q_SLICER_MODULE_PATIENTHIERARCHY_WIDGETS_PLUGINS_EXPORT qSlicerPatientHierarchyModuleWidgetsPlugin
  : public QObject
  , public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface);

public:
  QList<QDesignerCustomWidgetInterface*> customWidgets() const
    {
    QList<QDesignerCustomWidgetInterface *> plugins;
    plugins << new qMRMLPatientHierarchyTreeViewPlugin;
    return plugins;
    }
};

#endif
