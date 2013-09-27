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
#include <QDebug> 
#include <QtPlugin>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <vtkSlicerVolumesLogic.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerDicomRtExportModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerDicomRtExportModule.h"
#include "qSlicerDicomRtExportModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDicomRtExportModule, qSlicerDicomRtExportModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_DicomRtExport
class qSlicerDicomRtExportModulePrivate
{
public:
  qSlicerDicomRtExportModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtExportModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModulePrivate::qSlicerDicomRtExportModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomRtExportModule methods

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModule::qSlicerDicomRtExportModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDicomRtExportModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtExportModule::~qSlicerDicomRtExportModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtExportModule::helpText()const
{
  QString help = 
    "The DicomRtExport module enables exporting the contour node as DICOM-RT files"
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/DicomRtExport\">%1/Documentation/%2.%3/Modules/Models</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtExportModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program "
         "and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomRtExportModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (PMH, Toronto)");
  moduleContributors << QString("Greg Sharp (MGH, Boston)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDicomRtExportModule::icon()const
{
  return this->Superclass::icon();
  //return QIcon(":/Icons/DicomRtExport.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomRtExportModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtExportModule::setup()
{
  this->Superclass::setup();

  vtkSlicerDicomRtExportModuleLogic* DicomRtExportLogic = vtkSlicerDicomRtExportModuleLogic::SafeDownCast(this->logic());

  qSlicerAbstractCoreModule* volumesModule = qSlicerCoreApplication::application()->moduleManager()->module("Volumes");
  if (volumesModule)
  {
    vtkSlicerVolumesLogic* volumesLogic = vtkSlicerVolumesLogic::SafeDownCast(volumesModule->logic());
    //DicomRtExportLogic->SetVolumesLogic(volumesLogic);
  }
  else
  {
    qWarning() << "Volumes module is not found";
  } 
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDicomRtExportModule::createWidgetRepresentation()
{
  return new qSlicerDicomRtExportModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDicomRtExportModule::createLogic()
{
  return vtkSlicerDicomRtExportModuleLogic::New();
}
