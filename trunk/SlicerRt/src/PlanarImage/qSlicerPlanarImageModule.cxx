/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Qt includes
#include <QtPlugin>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// PlanarImage Logic includes
#include <vtkSlicerPlanarImageModuleLogic.h>

// PlanarImage includes
#include "qSlicerPlanarImageModule.h"
#include "qSlicerPlanarImageModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerPlanarImageModule, qSlicerPlanarImageModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PlanarImage
class qSlicerPlanarImageModulePrivate
{
public:
  qSlicerPlanarImageModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPlanarImageModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPlanarImageModulePrivate::qSlicerPlanarImageModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlanarImageModule methods

//-----------------------------------------------------------------------------
qSlicerPlanarImageModule::qSlicerPlanarImageModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlanarImageModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlanarImageModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerPlanarImageModule::~qSlicerPlanarImageModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPlanarImageModule::helpText()const
{
  QString help = 
    "This module displays and handles planar images (single-slice volumes) as textured models. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/PlanarImage\">%1/Documentation/%2.%3/Modules/PlanarImage</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerPlanarImageModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlanarImageModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
void qSlicerPlanarImageModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerPlanarImageModule::createWidgetRepresentation()
{
  return new qSlicerPlanarImageModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlanarImageModule::createLogic()
{
  return vtkSlicerPlanarImageModuleLogic::New();
}
