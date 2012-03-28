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
#include <QFileDialog>
#include <QCheckBox>
#include <QTimer>

// SlicerQt includes
#include "qSlicerDoseVolumeHistogramModuleWidget.h"
#include "ui_qSlicerDoseVolumeHistogramModule.h"

// ModuleTemplate includes
#include "vtkSlicerDoseVolumeHistogramLogic.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLDoubleArrayNode.h>

// VTK includes
#include <vtkStringArray.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerDoseVolumeHistogramModuleWidgetPrivate: public Ui_qSlicerDoseVolumeHistogramModule
{
  Q_DECLARE_PUBLIC(qSlicerDoseVolumeHistogramModuleWidget);
protected:
  qSlicerDoseVolumeHistogramModuleWidget* const q_ptr;
public:
  qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget &object);
  vtkSlicerDoseVolumeHistogramLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget& object)
 : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic*
qSlicerDoseVolumeHistogramModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseVolumeHistogramModuleWidget);
  return vtkSlicerDoseVolumeHistogramLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::qSlicerDoseVolumeHistogramModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseVolumeHistogramModuleWidgetPrivate(*this) )
{
  m_ChartCheckboxToStructureSetNameMap.clear();
  m_ShowInChartCheckStates.clear();
  m_ShowHideAllClicked = false;

  m_CheckSceneChangeTimer = new QTimer(this); 
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::~qSlicerDoseVolumeHistogramModuleWidget()
{
  m_ChartCheckboxToStructureSetNameMap.clear();

  if (m_CheckSceneChangeTimer != NULL)
  {
    m_CheckSceneChangeTimer->stop();
		delete m_CheckSceneChangeTimer;
		m_CheckSceneChangeTimer = NULL;
	}
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setup()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Hide widgets whose functions have not been implemented yet
  d->pushButton_ExportStatisticsToCsv->setVisible(false);
  d->label_ImportCSV->setVisible(false);
  d->pushButton_ImportCSV->setVisible(false);

  d->tableWidget_ChartStatistics->setSortingEnabled(false);
  d->tableWidget_ChartStatistics->setSelectionMode(QAbstractItemView::NoSelection);

  // Make connections
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_StructureSet, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( structureSetNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_Chart, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( chartNodeChanged(vtkMRMLNode*) ) );

  connect( d->pushButton_ComputeDVH, SIGNAL( clicked() ), this, SLOT( computeDvhClicked() ) );
  connect( d->pushButton_ExportDvhToCsv, SIGNAL( clicked() ), this, SLOT( exportDvhToCsvClicked() ) );
  connect( d->pushButton_ExportStatisticsToCsv, SIGNAL( clicked() ), this, SLOT( exportStatisticsToCsv() ) );
  connect( d->checkBox_ShowHideAll, SIGNAL( stateChanged(int) ), this, SLOT( showHideAllCheckedStateChanged(int) ) );

  connect( m_CheckSceneChangeTimer, SIGNAL( timeout() ), this, SLOT( checkSceneChange() ) );

  // Start timer
	m_CheckSceneChangeTimer->start(100); 

  updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  bool dvhCanBeComputed=d->logic()->GetDoseVolumeNode() && d->logic()->GetStructureSetModelNode();
  d->pushButton_ComputeDVH->setEnabled(dvhCanBeComputed);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateChartCheckboxesState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator it;

  // If there is no chart node selected, disable all checkboxes
  if (d->logic()->GetChartNode() == NULL)
  {
    for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
    {
      it->first->setEnabled(false);
    }
    d->checkBox_ShowHideAll->setEnabled(false);

    return;
  }

  vtkStringArray* arraysInSelectedChart = d->logic()->GetChartNode()->GetArrays();
  m_ShowInChartCheckStates.clear();

  for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
  {
    it->first->setEnabled(true);
    it->first->blockSignals(true); // block signals for the checkboxes so that changing it do not toggle the visibility of the plot
    it->first->setChecked(false);

    for (int i=0; i<arraysInSelectedChart->GetNumberOfValues(); ++i)
    {
      if (arraysInSelectedChart->GetValue(i).compare(it->second.second) == 0)
      {
        it->first->setChecked(true);
        break;
      }
    }

    m_ShowInChartCheckStates.push_back(it->first->isChecked());

    it->first->blockSignals(false); // unblock signal for the checkbox in question
  }

  // Change show/hide all checkbox state
  d->checkBox_ShowHideAll->setEnabled(true);
  d->checkBox_ShowHideAll->blockSignals(true);
  if (arraysInSelectedChart->GetNumberOfValues() == 0)
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::Unchecked);
  }
  else if (arraysInSelectedChart->GetNumberOfValues() == m_ChartCheckboxToStructureSetNameMap.size())
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
  }
  else
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::PartiallyChecked);
  }
  d->checkBox_ShowHideAll->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  vtkMRMLVolumeNode* volumeNode = dynamic_cast<vtkMRMLVolumeNode*>(node);
  if (volumeNode)
  {
    d->logic()->SetDoseVolumeNode(volumeNode);
    updateButtonsState();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::structureSetNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->logic()->SetStructureSetModelNode(node);
  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::chartNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  vtkMRMLChartNode* chartNode = dynamic_cast<vtkMRMLChartNode*>(node);
  if (chartNode)
  {
    d->logic()->SetChartNode(chartNode);
    updateButtonsState();
    updateChartCheckboxesState();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::checkSceneChange()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (d->logic()->GetSceneChanged())
  {
    refreshDvhTable();

    d->logic()->SceneChangedOff();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::refreshDvhTable()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkCollection* dvhNodes = d->logic()->GetDvhDoubleArrayNodes();

  // Clear the table
  d->tableWidget_ChartStatistics->clear();

  std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator it;
  for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
  {
    QCheckBox* checkbox = it->first;
    disconnect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );
    delete checkbox;
  }

  m_ChartCheckboxToStructureSetNameMap.clear();

  if (dvhNodes->GetNumberOfItems() < 1)
  {
    return;
  }

  // Set up the table
  d->tableWidget_ChartStatistics->setColumnCount(6);
  QString doseUnit( d->logic()->GetDoseVolumeNode()->GetAttribute("DoseUnits") );
  QStringList headerLabels;
  headerLabels << "Structure" << "Total volume (cc)" << QString("Mean dose (%1)").arg(doseUnit) << QString("Max dose (%1)").arg(doseUnit) << QString("Min dose (%1)").arg(doseUnit) << " ";
  d->tableWidget_ChartStatistics->setHorizontalHeaderLabels(headerLabels);
  d->tableWidget_ChartStatistics->setRowCount(dvhNodes->GetNumberOfItems());

  // Fill the table
  for (int i=0; i<dvhNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast( dvhNodes->GetItemAsObject(i) );
    if (!dvhNode)
    {
      continue;
    }

    d->tableWidget_ChartStatistics->setItem(i, 0, new QTableWidgetItem( 
      QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ) );
    d->tableWidget_ChartStatistics->setItem(i, 1, new QTableWidgetItem( 
      QString::number( QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str())).toDouble(),'f',4) ) );
    d->tableWidget_ChartStatistics->setItem(i, 2, new QTableWidgetItem( 
      QString::number( QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_MEAN_DOSE_GY_ATTRIBUTE_NAME.c_str())).toDouble(),'f',4) ) );
    d->tableWidget_ChartStatistics->setItem(i, 3, new QTableWidgetItem( 
      QString::number( QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_MAX_DOSE_GY_ATTRIBUTE_NAME.c_str())).toDouble(),'f',4) ) );
    d->tableWidget_ChartStatistics->setItem(i, 4, new QTableWidgetItem( 
      QString::number( QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_MIN_DOSE_GY_ATTRIBUTE_NAME.c_str())).toDouble(),'f',4) ) );

    // Create checkbox
    QCheckBox* checkbox = new QCheckBox(d->tableWidget_ChartStatistics);
    checkbox->setToolTip(tr("Show/hide DVH plot of structure '%1' in selected chart").arg( QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ));
    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );

    // Store checkbox with the augmented structure set name and the double array ID
    std::string plotName( dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str()) );
    plotName.append( QString("(%1)").arg(i+1).toAscii().data() );
    dvhNode->SetAttribute(vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_PLOTS_NAME_ATTRIBUTE_NAME.c_str(), plotName.c_str());
    m_ChartCheckboxToStructureSetNameMap[checkbox] = std::pair<std::string, std::string>(plotName, dvhNode->GetID());

    d->tableWidget_ChartStatistics->setCellWidget(i, 5, checkbox);
  }

  updateButtonsState();
  updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::computeDvhClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the DVH for the selected structure set using the selected dose volume
  d->logic()->ComputeDvh();

  refreshDvhTable();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showInChartCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());

  if (!senderCheckbox)
  {
    std::cerr << "Error: Invalid sender checkbox for show/hide in chart checkbox state change" << std::endl;
    return;
  }

  if (aState)
  {
    d->logic()->AddDvhToSelectedChart(m_ChartCheckboxToStructureSetNameMap[senderCheckbox].first.c_str(), m_ChartCheckboxToStructureSetNameMap[senderCheckbox].second.c_str());
  }
  else
  {
    d->logic()->RemoveDvhFromSelectedChart(m_ChartCheckboxToStructureSetNameMap[senderCheckbox].first.c_str());
  }

  if (!m_ShowHideAllClicked)
  {
    // Update states vector
    std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator checkboxIt;
    std::vector<bool>::iterator stateIt;
    for (checkboxIt=m_ChartCheckboxToStructureSetNameMap.begin(), stateIt=m_ShowInChartCheckStates.begin(); checkboxIt!=m_ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt, ++stateIt)
    {
      if (checkboxIt->first == senderCheckbox)
      {
        (*stateIt) = (bool)aState;
      }
    }

    // Change show/hide all checkbox state
    d->checkBox_ShowHideAll->blockSignals(true);
    bool isThereChecked = false;
    bool isThereUnchecked = false;
    std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator it;
    for (it=m_ChartCheckboxToStructureSetNameMap.begin(); it!=m_ChartCheckboxToStructureSetNameMap.end(); ++it)
    {
      if (it->first->isChecked())
      {
        isThereChecked = true;
      }
      else
      {
        isThereUnchecked = true;
      }
    }
    if (isThereChecked && isThereUnchecked)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::PartiallyChecked);
    }
    else if (isThereChecked && !isThereUnchecked)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
    }
    else
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Unchecked);
    }
    d->checkBox_ShowHideAll->blockSignals(false);
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showHideAllCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  std::map<QCheckBox*, std::pair<std::string, std::string>>::iterator checkboxIt;

  m_ShowHideAllClicked = true;

  if (aState == Qt::PartiallyChecked)
  {
    std::vector<bool>::iterator stateIt;
    for (checkboxIt=m_ChartCheckboxToStructureSetNameMap.begin(), stateIt=m_ShowInChartCheckStates.begin(); checkboxIt!=m_ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt, ++stateIt)
    {
      checkboxIt->first->setChecked(*stateIt);
    }
  }
  else
  {
    bool state = (aState==Qt::Checked ? true : false);
    for (checkboxIt=m_ChartCheckboxToStructureSetNameMap.begin(); checkboxIt!=m_ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt)
    {
      checkboxIt->first->setChecked(state);
    }
  }

  m_ShowHideAllClicked = false;
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportDvhToCsvClicked()
{
  QString filter = QString( tr( "Comma Separated Values (*.csv);;" ) );
  QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save DVH to CSV file"), QString("."), filter);

	if (! fileName.isNull() )
  {
    // TODO
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportStatisticsToCsv()
{
  QString filter = QString( tr( "Comma Separated Values (*.csv);;" ) );
  QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save statistics to CSV file"), QString("."), filter);

	if (! fileName.isNull() )
  {
    // print comma separated value file with header keys in quotes

    //csv = ""
    //header = ""
    //for k in self.keys[:-1]:
    //  header += "\"%s\"" % k + ","
    //header += "\"%s\"" % self.keys[-1] + "\n"
    //csv = header
    //for i in self.labelStats["Labels"]:
    //  line = ""
    //  for k in self.keys[:-1]:
    //    line += str(self.labelStats[i,k]) + ","
    //  line += str(self.labelStats[i,self.keys[-1]]) + "\n"
    //  csv += line

    //fp = open(fileName, "w")
    //fp.write(csv)
    //fp.close()
  }
}
