/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Adam Rankin, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "ctkTest.h"
#include "qSlicerContoursModuleWidget.h"
#include "ui_qSlicerContoursModule.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerQt includes
#include <QApplication>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtksys/SystemTools.hxx>

// ITK includes
#include "itkFactoryRegistration.h"

// ----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Contours
class qSlicerContoursModuleWidgetTester: public QObject
{
  Q_OBJECT

public:
  qSlicerContoursModuleWidget* SlicerContoursModuleWidget;

public:
  int argc;
  char** argv;

protected:
  bool m_Initialized;
  vtkMRMLScene* m_Scene;

private slots:
  void init();
  void cleanup();

  void testUI();
};

// ----------------------------------------------------------------------------
void qSlicerContoursModuleWidgetTester::init()
{
  m_Initialized = false;

  int argIndex = 1;
  std::ostream& outputStream = std::cout;
  std::ostream& errorStream = std::cerr;

  // TestSceneFile
  const char *testSceneFileName  = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TestSceneFile") == 0)
    {
      testSceneFileName = argv[argIndex+1];
      outputStream << "Test MRML scene file name: " << testSceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      testSceneFileName = "";
    }
  }
  else
  {
    errorStream << "Invalid arguments!" << std::endl;
    delete this->SlicerContoursModuleWidget;
    this->SlicerContoursModuleWidget = NULL;
    return;
  }

  const char *temporarySceneFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TemporarySceneFile") == 0)
    {
      temporarySceneFileName = argv[argIndex+1];
      outputStream << "Temporary scene file name: " << temporarySceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      temporarySceneFileName = "";
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    delete this->SlicerContoursModuleWidget;
    this->SlicerContoursModuleWidget = NULL;
    return;
  }

  // Make sure NRRD reading works
  itk::itkFactoryRegistration();

  // Create scene
  this->m_Scene = vtkMRMLScene::New();

  this->SlicerContoursModuleWidget = new qSlicerContoursModuleWidget;

  // Create logic
  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> shLogic =
    vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  shLogic->SetMRMLScene(this->m_Scene);

  // Create logic
  vtkSmartPointer<vtkSlicerContoursModuleLogic> contoursLogic =
    vtkSmartPointer<vtkSlicerContoursModuleLogic>::New();
  contoursLogic->SetMRMLScene(this->m_Scene);

  // Load test scene into temporary scene
  this->m_Scene->SetURL(testSceneFileName);
  this->m_Scene->Import();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  this->m_Scene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  this->m_Scene->SetURL(temporarySceneFileName);
  this->m_Scene->Commit();

  this->SlicerContoursModuleWidget->testInit();
  this->SlicerContoursModuleWidget->setMRMLScene(this->m_Scene);
  this->SlicerContoursModuleWidget->enter();

  m_Initialized = true;
}

// ----------------------------------------------------------------------------
void qSlicerContoursModuleWidgetTester::cleanup()
{
  this->SlicerContoursModuleWidget->exit();
  QVERIFY(this->SlicerContoursModuleWidget != NULL);
  delete this->SlicerContoursModuleWidget;
  this->SlicerContoursModuleWidget = NULL;
  this->m_Scene->Delete();
}

// ----------------------------------------------------------------------------
void qSlicerContoursModuleWidgetTester::testUI()
{
  if (this->m_Initialized)
  {
    this->SlicerContoursModuleWidget->show();

    vtkMRMLContourNode* bodyNode = vtkMRMLContourNode::SafeDownCast(this->m_Scene->GetFirstNodeByName("BODY_Contour"));
    this->SlicerContoursModuleWidget->testSetContourNode(bodyNode);
    if (this->SlicerContoursModuleWidget->testGetCurrentContourNode() == NULL)
    {
      QFAIL("Unable to set contour node.");
    }

    // See if things work for index label map
    this->SlicerContoursModuleWidget->testSetTargetRepresentationType(vtkMRMLContourNode::IndexedLabelmap);

    if (!this->SlicerContoursModuleWidget->testGetDPointer()->MRMLNodeComboBox_ReferenceVolume->isVisible())
    {
      QFAIL("Reference volume combobox is invisible when it should be visible.");
    }
    if (!this->SlicerContoursModuleWidget->testGetDPointer()->horizontalSlider_OversamplingFactor->isVisible())
    {
      QFAIL("Oversampling factor widget is not visible.");
    }
    if (this->SlicerContoursModuleWidget->testGetDPointer()->pushButton_ApplyChangeRepresentation->isEnabled())
    {
      QFAIL("Apply button is enabled when it shouldn't be.");
    }

    vtkMRMLScalarVolumeNode* doseNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->m_Scene->GetFirstNodeByName("Dose"));
    this->SlicerContoursModuleWidget->testSetReferenceVolumeNode(doseNode);
    if (this->SlicerContoursModuleWidget->testGetCurrentReferenceVolumeNode() != doseNode)
    {
      QFAIL("Selected reference volume is not \"Dose\".");
    }

    if (!this->SlicerContoursModuleWidget->testGetDPointer()->pushButton_ApplyChangeRepresentation->isEnabled())
    {
      QFAIL("Apply button should be enabled after reference volume is set. It is not.");
    }

    this->SlicerContoursModuleWidget->hide();
  }
}

// ----------------------------------------------------------------------------
int qSlicerContoursModuleWidgetTest(int argc, char *argv[])
{
  QApplication app(argc, argv);
  qSlicerContoursModuleWidgetTester tc;
  tc.argc = argc;
  tc.argv = argv;
  argc = 1; // Truncate all remaining entries, run all tests
  return QTest::qExec(&tc, argc, argv);
}

#include "moc_qSlicerContoursModuleWidgetTest.cxx"
