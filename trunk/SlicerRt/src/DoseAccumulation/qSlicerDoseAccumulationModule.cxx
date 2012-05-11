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

// Qt includes
#include <QtPlugin>

// DoseAccumulation Logic includes
#include <vtkSlicerDoseAccumulationLogic.h>

// DoseAccumulation includes
#include "qSlicerDoseAccumulationModule.h"
#include "qSlicerDoseAccumulationModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDoseAccumulationModule, qSlicerDoseAccumulationModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoseAccumulation
class qSlicerDoseAccumulationModulePrivate
{
public:
  qSlicerDoseAccumulationModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDoseAccumulationModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModulePrivate::qSlicerDoseAccumulationModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDoseAccumulationModule methods

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModule::qSlicerDoseAccumulationModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDoseAccumulationModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseAccumulationModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModule::~qSlicerDoseAccumulationModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDoseAccumulationModule::helpText()const
{
  QString help = 
    "This template module is meant to be used with the"
    "with the ModuleWizard.py script distributed with the"
    "Slicer source code (starting with version 4)."
    "Developers can generate their own source code using the"
    "wizard and then customize it to fit their needs.";
  return help;
}

//-----------------------------------------------------------------------------
QString qSlicerDoseAccumulationModule::acknowledgementText()const
{
  return "This work was supported by NAMIC, NAC, and the Slicer Community...";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseAccumulationModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (Organization)");
  // moduleContributors << QString("Richard Roe (Organization2)");
  // ...
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDoseAccumulationModule::icon()const
{
  return QIcon(":/Icons/DoseAccumulation.png");
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDoseAccumulationModule::createWidgetRepresentation()
{
  return new qSlicerDoseAccumulationModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDoseAccumulationModule::createLogic()
{
  return vtkSlicerDoseAccumulationLogic::New();
}
