/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada and
  Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// ModuleTemplate includes
#include "vtkSlicerDicomRtReader.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// VTK includes
#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkRibbonFilter.h>
#include <vtkSmartPointer.h>

// DCMTK includes
#include <dcmtk/config/osconfig.h>    /* make sure OS specific configuration is included first */

#include <dcmtk/ofstd/ofconapp.h>

#include <dcmtk/dcmrt/drtdose.h>
#include <dcmtk/dcmrt/drtimage.h>
#include <dcmtk/dcmrt/drtplan.h>
#include <dcmtk/dcmrt/drtstrct.h>
#include <dcmtk/dcmrt/drttreat.h>
#include <dcmtk/dcmrt/drtionpl.h>
#include <dcmtk/dcmrt/drtiontr.h>

// Qt includes
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QVariant>
#include <QStringList>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

// CTK includes
#include <ctkDICOMDatabase.h>

//----------------------------------------------------------------------------
namespace
{
  template<class T>
  vtkVector3<T> operator -(const vtkVector3<T>& a, const vtkVector3<T>& b)
  {
    vtkVector3<T> result;
    result.SetX(a.GetX() - b.GetX());
    result.SetY(a.GetY() - b.GetY());
    result.SetZ(a.GetZ() - b.GetZ());
    return result;
  }

  template<class T>
  bool operator ==(const vtkVector3<T>& a, const vtkVector3<T>& b)
  {
    return a.GetX() == b.GetX() && a.GetY() == b.GetY() && a.GetZ() == b.GetZ();
  }

  template<class T>
  bool operator !=(const vtkVector3<T>& a, const vtkVector3<T>& b)
  {
    return !(a==b);
  }

  bool AreEqualWithTolerance(double a, double b)
  {
    return fabs(a - b) < EPSILON;
  }

  double MajorityValue(const std::vector<double>& spacingValues, std::string &outputMessage)
  {
    std::map<double, int> spacingValueFrequencies;
    double averageTotal(0.0);
    double averageCount(0.0);
    for (std::vector<double>::const_iterator it = spacingValues.begin(); it != spacingValues.end(); ++it)
    {
      double roundedSpacingValue = vtkMath::Round((*it) / EPSILON) * EPSILON; // Round value to consider spacings within tolerance the same
      averageTotal += *it;
      averageCount++;
      spacingValueFrequencies[roundedSpacingValue]++;
    }
    double majorityValue(0.0);
    int majorityCount(-1);
    std::stringstream outputStringStream;
    for (std::map<double, int>::iterator it = spacingValueFrequencies.begin(); it != spacingValueFrequencies.end(); ++it)
    {
      outputStringStream << "  Planar spacing: " << it->first << ". Frequency: " << it->second << "." << std::endl;
      if (it->second > majorityCount)
      {
        majorityValue = it->first;
        majorityCount = it->second;
      }
    }
    outputMessage = outputStringStream.str();
    return majorityValue;
  }

} // namespace

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::RoiEntry::RoiEntry()
{
  this->Number = 0;
  this->SliceThickness = 0.0;
  this->DisplayColor[0] = 1.0;
  this->DisplayColor[1] = 0.0;
  this->DisplayColor[2] = 0.0;
  this->PolyData = NULL;
  this->ContourPlaneNormalVector[0] = 0.0;
  this->ContourPlaneNormalVector[1] = 0.0;
  this->ContourPlaneNormalVector[2] = 1.0;
}

vtkSlicerDicomRtReader::RoiEntry::~RoiEntry()
{
  this->SetPolyData(NULL);
}

vtkSlicerDicomRtReader::RoiEntry::RoiEntry(const RoiEntry& src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor[0] = src.DisplayColor[0];
  this->DisplayColor[1] = src.DisplayColor[1];
  this->DisplayColor[2] = src.DisplayColor[2];
  this->PolyData = NULL;
  this->SetPolyData(src.PolyData);
  this->SliceThickness = src.SliceThickness;
  this->ContourPlaneNormalVector[0] = src.ContourPlaneNormalVector[0];
  this->ContourPlaneNormalVector[1] = src.ContourPlaneNormalVector[1];
  this->ContourPlaneNormalVector[2] = src.ContourPlaneNormalVector[2];
  this->ReferencedSeriesUID = src.ReferencedSeriesUID;
  this->ReferencedFrameOfReferenceUID = src.ReferencedFrameOfReferenceUID;
  this->ContourIndexToSOPInstanceUIDMap = src.ContourIndexToSOPInstanceUIDMap;
}

vtkSlicerDicomRtReader::RoiEntry& vtkSlicerDicomRtReader::RoiEntry::operator=(const RoiEntry &src)
{
  this->Number = src.Number;
  this->Name = src.Name;
  this->Description = src.Description;
  this->DisplayColor[0] = src.DisplayColor[0];
  this->DisplayColor[1] = src.DisplayColor[1];
  this->DisplayColor[2] = src.DisplayColor[2];
  this->SetPolyData(src.PolyData);
  this->SliceThickness = src.SliceThickness;
  this->ContourPlaneNormalVector[0] = src.ContourPlaneNormalVector[0];
  this->ContourPlaneNormalVector[1] = src.ContourPlaneNormalVector[1];
  this->ContourPlaneNormalVector[2] = src.ContourPlaneNormalVector[2];
  this->ReferencedSeriesUID = src.ReferencedSeriesUID;
  this->ReferencedFrameOfReferenceUID = src.ReferencedFrameOfReferenceUID;
  this->ContourIndexToSOPInstanceUIDMap = src.ContourIndexToSOPInstanceUIDMap;

  return (*this);
}

void vtkSlicerDicomRtReader::RoiEntry::SetPolyData(vtkPolyData* roiPolyData)
{
  if (roiPolyData == this->PolyData)
  {
    // not changed
    return;
  }
  if (this->PolyData != NULL)
  {
    this->PolyData->UnRegister(NULL);
  }

  this->PolyData = roiPolyData;

  if (this->PolyData != NULL)
  {
    this->PolyData->Register(NULL);
  }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const std::string vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_DATABASE_FILENAME = "/ctkDICOM.sql";
const std::string vtkSlicerDicomRtReader::DICOMRTREADER_DICOM_CONNECTION_NAME = "SlicerRt";

vtkStandardNewMacro(vtkSlicerDicomRtReader);

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::vtkSlicerDicomRtReader()
{
  this->FileName = NULL;

  this->RoiSequenceVector.clear();
  this->RTStructureSetReferencedSOPInstanceUIDs = NULL;
  this->BeamSequenceVector.clear();

  this->SetPixelSpacing(0.0,0.0);
  this->DoseUnits = NULL;
  this->DoseGridScaling = NULL;
  this->RTDoseReferencedRTPlanSOPInstanceUID = NULL;

  this->SOPInstanceUID = NULL;

  this->ImageType = NULL;
  this->RTImageLabel = NULL;
  this->RTImageReferencedRTPlanSOPInstanceUID = NULL;
  this->ReferencedBeamNumber = -1;
  this->SetRTImagePosition(0.0,0.0);
  this->RTImageSID = 0.0;
  this->WindowCenter = 0.0;
  this->WindowWidth = 0.0;

  this->PatientName = NULL;
  this->PatientId = NULL;
  this->PatientSex = NULL;
  this->PatientBirthDate = NULL;
  this->PatientComments = NULL;
  this->StudyInstanceUid = NULL;
  this->StudyDescription = NULL;
  this->StudyDate = NULL;
  this->StudyTime = NULL;
  this->SeriesInstanceUid = NULL;
  this->SeriesDescription = NULL;
  this->SeriesModality = NULL;
  this->SeriesNumber = NULL;

  this->DatabaseFile = NULL;

  this->LoadRTStructureSetSuccessful = false;
  this->LoadRTDoseSuccessful = false;
  this->LoadRTPlanSuccessful = false;
  this->LoadRTImageSuccessful = false;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::~vtkSlicerDicomRtReader()
{
  this->RoiSequenceVector.clear();
  this->BeamSequenceVector.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::Update()
{
  if ((this->FileName != NULL) && (strlen(this->FileName) > 0))
  {
    // Set DICOM database file name
    QSettings settings;
    QString databaseDirectory = settings.value("DatabaseDirectory").toString();
    QString databaseFile = databaseDirectory + DICOMRTREADER_DICOM_DATABASE_FILENAME.c_str();
    this->SetDatabaseFile(databaseFile.toLatin1().constData());

    // Load DICOM file or dataset
    DcmFileFormat fileformat;

    OFCondition result = EC_TagNotFound;
    result = fileformat.loadFile(this->FileName, EXS_Unknown);
    if (result.good())
    {
      DcmDataset *dataset = fileformat.getDataset();

      // Check SOP Class UID for one of the supported RT objects
      //   TODO: One series can contain composite information, e.g, an RTPLAN series can contain structure sets and plans as well
      OFString sopClass("");
      if (dataset->findAndGetOFString(DCM_SOPClassUID, sopClass).good() && !sopClass.empty())
      {
        if (sopClass == UID_RTDoseStorage)
        {
          this->LoadRTDose(dataset);
        }
        else if (sopClass == UID_RTImageStorage)
        {
          this->LoadRTImage(dataset);
        }
        else if (sopClass == UID_RTPlanStorage)
        {
          this->LoadRTPlan(dataset);  
        }
        else if (sopClass == UID_RTStructureSetStorage)
        {
          this->LoadRTStructureSet(dataset);
        }
        else if (sopClass == UID_RTTreatmentSummaryRecordStorage)
        {
          //result = dumpRTTreatmentSummaryRecord(out, *dataset);
        }
        else if (sopClass == UID_RTIonPlanStorage)
        {
          //result = dumpRTIonPlan(out, *dataset);
        }
        else if (sopClass == UID_RTIonBeamsTreatmentRecordStorage)
        {
          //result = dumpRTIonBeamsTreatmentRecord(out, *dataset);
        }
        else
        {
          //OFLOG_ERROR(drtdumpLogger, "unsupported SOPClassUID (" << sopClass << ") in file: " << ifname);
        }
      } 
      else 
      {
        //OFLOG_ERROR(drtdumpLogger, "SOPClassUID (0008,0016) missing or empty in file: " << ifname);
      }
    } 
    else 
    {
      //OFLOG_FATAL(drtdumpLogger, OFFIS_CONSOLE_APPLICATION << ": error (" << result.text() << ") reading file: " << ifname);
    }
  } 
  else 
  {
    //OFLOG_FATAL(drtdumpLogger, OFFIS_CONSOLE_APPLICATION << ": invalid filename: <empty string>");
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTImage(DcmDataset* dataset)
{
  this->LoadRTImageSuccessful = false;

  DRTImageIOD rtImageObject;
  if (rtImageObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to read RT Image object!");
    return;
  }

  vtkDebugMacro("LoadRTImage: Load RT Image object");

  // ImageType
  OFString imageType("");
  if (rtImageObject.getImageType(imageType).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to get Image Type for RT Image object");
    return; // mandatory DICOM value
  }
  this->SetImageType(imageType.c_str());

  // RTImageLabel
  OFString rtImagelabel("");
  if (rtImageObject.getRTImageLabel(rtImagelabel).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to get RT Image Label for RT Image object");
    return; // mandatory DICOM value
  }
  this->SetRTImageLabel(imageType.c_str());

  // RTImagePlane (confirm it is NORMAL)
  OFString rtImagePlane("");
  if (rtImageObject.getRTImagePlane(rtImagePlane).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to get RT Image Plane for RT Image object");
    return; // mandatory DICOM value
  }
  if (rtImagePlane.compare("NORMAL"))
  {
    vtkErrorMacro("LoadRTImage: Only value 'NORMAL' is supported for RTImagePlane tag for RT Image objects!");
    return;
  }

  // ReferencedRTPlanSequence
  DRTReferencedRTPlanSequenceInRTImageModule &rtReferencedRtPlanSequenceObject = rtImageObject.getReferencedRTPlanSequence();
  if (rtReferencedRtPlanSequenceObject.gotoFirstItem().good())
  {
    DRTReferencedRTPlanSequenceInRTImageModule::Item &currentReferencedRtPlanSequenceObject = rtReferencedRtPlanSequenceObject.getCurrentItem();

    OFString referencedSOPClassUID("");
    currentReferencedRtPlanSequenceObject.getReferencedSOPClassUID(referencedSOPClassUID);
    if (referencedSOPClassUID.compare(UID_RTPlanStorage))
    {
      vtkErrorMacro("LoadRTImage: Referenced RT Plan SOP class has to be RTPlanStorage!");
    }
    else
    {
      // Read Referenced RT Plan SOP instance UID
      OFString referencedSOPInstanceUID("");
      currentReferencedRtPlanSequenceObject.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
      this->SetRTImageReferencedRTPlanSOPInstanceUID(referencedSOPInstanceUID.c_str());
    }

    if (rtReferencedRtPlanSequenceObject.getNumberOfItems() > 1)
    {
      vtkErrorMacro("LoadRTImage: Referenced RT Plan sequence object can contain one item! It contains " << rtReferencedRtPlanSequenceObject.getNumberOfItems());
    }
  }

  // ReferencedBeamNumber
  Sint32 referencedBeamNumber = -1;
  if (rtImageObject.getReferencedBeamNumber(referencedBeamNumber).good())
  {
    this->ReferencedBeamNumber = (int)referencedBeamNumber;
  }
  else if (rtReferencedRtPlanSequenceObject.getNumberOfItems() == 1)
  {
    // Type 3
    vtkDebugMacro("LoadRTImage: Unable to get referenced beam number in referenced RT Plan for RT image!");
  }

  // XRayImageReceptorTranslation
  OFVector<vtkTypeFloat64> xRayImageReceptorTranslation;
  if (rtImageObject.getXRayImageReceptorTranslation(xRayImageReceptorTranslation).good())
  {
    if (xRayImageReceptorTranslation.size() == 3)
    {
      if ( xRayImageReceptorTranslation[0] != 0.0
        || xRayImageReceptorTranslation[1] != 0.0
        || xRayImageReceptorTranslation[2] != 0.0 )
      {
        vtkErrorMacro("LoadRTImage: Non-zero XRayImageReceptorTranslation vectors are not supported!");
        return;
      }
    }
    else
    {
      vtkErrorMacro("LoadRTImage: XRayImageReceptorTranslation tag should contain a vector of 3 elements (it has " << xRayImageReceptorTranslation.size() << "!");
    }
  }

  // XRayImageReceptorAngle
  vtkTypeFloat64 xRayImageReceptorAngle = 0.0;
  if (rtImageObject.getXRayImageReceptorAngle(xRayImageReceptorAngle).good())
  {
    if (xRayImageReceptorAngle != 0.0)
    {
      vtkErrorMacro("LoadRTImage: Non-zero XRayImageReceptorAngle spacingValues are not supported!");
      return;
    }
  }

  // ImagePlanePixelSpacing
  OFVector<vtkTypeFloat64> imagePlanePixelSpacing;
  if (rtImageObject.getImagePlanePixelSpacing(imagePlanePixelSpacing).good())
  {
    if (imagePlanePixelSpacing.size() == 2)
    {
      //this->SetImagePlanePixelSpacing(imagePlanePixelSpacing[0], imagePlanePixelSpacing[1]);
    }
    else
    {
      vtkErrorMacro("LoadRTImage: ImagePlanePixelSpacing tag should contain a vector of 2 elements (it has " << imagePlanePixelSpacing.size() << "!");
    }
  }

  // RTImagePosition
  OFVector<vtkTypeFloat64> rtImagePosition;
  if (rtImageObject.getRTImagePosition(rtImagePosition).good())
  {
    if (rtImagePosition.size() == 2)
    {
      this->SetRTImagePosition(rtImagePosition[0], rtImagePosition[1]);
    }
    else
    {
      vtkErrorMacro("LoadRTImage: RTImagePosition tag should contain a vector of 2 elements (it has " << rtImagePosition.size() << ")!");
    }
  }

  // RTImageOrientation
  OFVector<vtkTypeFloat64> rtImageOrientation;
  if (rtImageObject.getRTImageOrientation(rtImageOrientation).good())
  {
    if (rtImageOrientation.size() > 0)
    {
      vtkErrorMacro("LoadRTImage: RTImageOrientation is specified but not supported yet!");
    }
  }

  // GantryAngle
  vtkTypeFloat64 gantryAngle = 0.0;
  if (rtImageObject.getGantryAngle(gantryAngle).good())
  {
    this->SetGantryAngle(gantryAngle);
  }

  // GantryPitchAngle
  vtkTypeFloat32 gantryPitchAngle = 0.0;
  if (rtImageObject.getGantryPitchAngle(gantryPitchAngle).good())
  {
    if (gantryPitchAngle != 0.0)
    {
      vtkErrorMacro("LoadRTImage: Non-zero GantryPitchAngle tag spacingValues are not supported yet!");
      return;
    }
  }

  // BeamLimitingDeviceAngle
  vtkTypeFloat64 beamLimitingDeviceAngle = 0.0;
  if (rtImageObject.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle).good())
  {
    this->SetBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
  }

  // PatientSupportAngle
  vtkTypeFloat64 patientSupportAngle = 0.0;
  if (rtImageObject.getPatientSupportAngle(patientSupportAngle).good())
  {
    this->SetPatientSupportAngle(patientSupportAngle);
  }

  // RadiationMachineSAD
  vtkTypeFloat64 radiationMachineSAD = 0.0;
  if (rtImageObject.getRadiationMachineSAD(radiationMachineSAD).good())
  {
    this->SetRadiationMachineSAD(radiationMachineSAD);
  }

  // RadiationMachineSSD
  vtkTypeFloat64 radiationMachineSSD = 0.0;
  if (rtImageObject.getRadiationMachineSSD(radiationMachineSSD).good())
  {
    //this->SetRadiationMachineSSD(radiationMachineSSD);
  }

  // RTImageSID
  vtkTypeFloat64 rtImageSID = 0.0;
  if (rtImageObject.getRTImageSID(rtImageSID).good())
  {
    this->SetRTImageSID(rtImageSID);
  }

  // SourceToReferenceObjectDistance
  vtkTypeFloat64 sourceToReferenceObjectDistance = 0.0;
  if (rtImageObject.getSourceToReferenceObjectDistance(sourceToReferenceObjectDistance).good())
  {
    //this->SetSourceToReferenceObjectDistance(sourceToReferenceObjectDistance);
  }

  // WindowCenter
  vtkTypeFloat64 windowCenter = 0.0;
  if (rtImageObject.getWindowCenter(windowCenter).good())
  {
    this->SetWindowCenter(windowCenter);
  }

  // WindowWidth
  vtkTypeFloat64 windowWidth = 0.0;
  if (rtImageObject.getWindowWidth(windowWidth).good())
  {
    this->SetWindowWidth(windowWidth);
  }

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtImageObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorMacro("LoadRTImage: Failed to get SOP instance UID for RT image!");
    return; // mandatory DICOM value
  }
  this->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtImageObject);

  this->LoadRTImageSuccessful = true;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTPlan(DcmDataset* dataset)
{
  this->LoadRTPlanSuccessful = false; 

  DRTPlanIOD rtPlanObject;
  if (rtPlanObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTPlan: Failed to read RT Plan object!");
    return;
  }

  vtkDebugMacro("LoadRTPlan: Load RT Plan object");

  DRTBeamSequence &rtPlaneBeamSequenceObject = rtPlanObject.getBeamSequence();
  if (rtPlaneBeamSequenceObject.gotoFirstItem().good())
  {
    do
    {
      DRTBeamSequence::Item &currentBeamSequenceObject = rtPlaneBeamSequenceObject.getCurrentItem();  
      if (!currentBeamSequenceObject.isValid())
      {
        vtkDebugMacro("LoadRTPlan: Found an invalid beam sequence in dataset");
        continue;
      }

      // Read item into the BeamSequenceVector
      BeamEntry beamEntry;

      OFString beamName("");
      currentBeamSequenceObject.getBeamName(beamName);
      beamEntry.Name=beamName.c_str();

      OFString beamDescription("");
      currentBeamSequenceObject.getBeamDescription(beamDescription);
      beamEntry.Description=beamDescription.c_str();

      OFString beamType("");
      currentBeamSequenceObject.getBeamType(beamType);
      beamEntry.Type=beamType.c_str();

      Sint32 beamNumber = -1;
      currentBeamSequenceObject.getBeamNumber( beamNumber );        
      beamEntry.Number = beamNumber;

      vtkTypeFloat64 sourceAxisDistance = 0.0;
      currentBeamSequenceObject.getSourceAxisDistance(sourceAxisDistance);
      beamEntry.SourceAxisDistance = sourceAxisDistance;

      DRTControlPointSequence &rtControlPointSequenceObject = currentBeamSequenceObject.getControlPointSequence();
      if (rtControlPointSequenceObject.gotoFirstItem().good())
      {
        // do // TODO: comment out for now since only first control point is loaded (as isocenter)
        {
          DRTControlPointSequence::Item &controlPointItem = rtControlPointSequenceObject.getCurrentItem();
          if (controlPointItem.isValid())
          {
            OFVector<vtkTypeFloat64> isocenterPositionDataLps;
            controlPointItem.getIsocenterPosition(isocenterPositionDataLps);

            // Convert from DICOM LPS -> Slicer RAS
            beamEntry.IsocenterPositionRas[0] = -isocenterPositionDataLps[0];
            beamEntry.IsocenterPositionRas[1] = -isocenterPositionDataLps[1];
            beamEntry.IsocenterPositionRas[2] = isocenterPositionDataLps[2];

            vtkTypeFloat64 gantryAngle = 0.0;
            controlPointItem.getGantryAngle(gantryAngle);
            beamEntry.GantryAngle = gantryAngle;

            vtkTypeFloat64 patientSupportAngle = 0.0;
            controlPointItem.getPatientSupportAngle(patientSupportAngle);
            beamEntry.PatientSupportAngle = patientSupportAngle;

            vtkTypeFloat64 beamLimitingDeviceAngle = 0.0;
            controlPointItem.getBeamLimitingDeviceAngle(beamLimitingDeviceAngle);
            beamEntry.BeamLimitingDeviceAngle = beamLimitingDeviceAngle;

            DRTBeamLimitingDevicePositionSequence &currentCollimatorPositionSequenceObject =
              controlPointItem.getBeamLimitingDevicePositionSequence();
            if (currentCollimatorPositionSequenceObject.gotoFirstItem().good())
            {
              do 
              {
                DRTBeamLimitingDevicePositionSequence::Item &collimatorPositionItem =
                  currentCollimatorPositionSequenceObject.getCurrentItem();
                if (collimatorPositionItem.isValid())
                {
                  OFString rtBeamLimitingDeviceType("");
                  collimatorPositionItem.getRTBeamLimitingDeviceType(rtBeamLimitingDeviceType);

                  OFVector<vtkTypeFloat64> leafJawPositions;
                  OFCondition getJawPositionsCondition = collimatorPositionItem.getLeafJawPositions(leafJawPositions);

                  if ( !rtBeamLimitingDeviceType.compare("ASYMX") || !rtBeamLimitingDeviceType.compare("X") )
                  {
                    if (getJawPositionsCondition.good())
                    {
                      beamEntry.LeafJawPositions[0][0] = leafJawPositions[0];
                      beamEntry.LeafJawPositions[0][1] = leafJawPositions[1];
                    }
                    else
                    {
                      vtkDebugMacro("LoadRTPlan: No jaw position found in collimator entry");
                    }
                  }
                  else if ( !rtBeamLimitingDeviceType.compare("ASYMY") || !rtBeamLimitingDeviceType.compare("Y") )
                  {
                    if (getJawPositionsCondition.good())
                    {
                      beamEntry.LeafJawPositions[1][0] = leafJawPositions[0];
                      beamEntry.LeafJawPositions[1][1] = leafJawPositions[1];
                    }
                    else
                    {
                      vtkDebugMacro("LoadRTPlan: No jaw position found in collimator entry");
                    }
                  }
                  else if ( !rtBeamLimitingDeviceType.compare("MLCX") || !rtBeamLimitingDeviceType.compare("MLCY") )
                  {
                    vtkWarningMacro("LoadRTPlan: Multi-leaf collimator entry found. This collimator type is not yet supported!");
                  }
                  else
                  {
                    vtkErrorMacro("LoadRTPlan: Unsupported collimator type: " << rtBeamLimitingDeviceType);
                  }
                }
              }
              while (currentCollimatorPositionSequenceObject.gotoNextItem().good());
            }
          } // endif controlPointItem.isValid()
        }
        // while (rtControlPointSequenceObject.gotoNextItem().good());
      }

      this->BeamSequenceVector.push_back(beamEntry);
    }
    while (rtPlaneBeamSequenceObject.gotoNextItem().good());
  }
  else
  {
    vtkErrorMacro("LoadRTPlan: No beams found in RT plan!");
    return;
  }

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtPlanObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorMacro("LoadRTPlan: Failed to get SOP instance UID for RT plan!");
    return; // mandatory DICOM value
  }
  this->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtPlanObject);

  this->LoadRTPlanSuccessful = true;
}

//----------------------------------------------------------------------------
DRTRTReferencedSeriesSequence* vtkSlicerDicomRtReader::GetReferencedSeriesSequence(DRTStructureSetIOD &rtStructureSetObject)
{
  DRTReferencedFrameOfReferenceSequence &rtReferencedFrameOfReferenceSequenceObject = rtStructureSetObject.getReferencedFrameOfReferenceSequence();
  if (!rtReferencedFrameOfReferenceSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: No referenced frame of reference sequence object item is available");
    return NULL;
  }

  DRTReferencedFrameOfReferenceSequence::Item &currentReferencedFrameOfReferenceSequenceItem = rtReferencedFrameOfReferenceSequenceObject.getCurrentItem();
  if (!currentReferencedFrameOfReferenceSequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: Frame of reference sequence object item is invalid");
    return NULL;
  }

  DRTRTReferencedStudySequence &rtReferencedStudySequenceObject = currentReferencedFrameOfReferenceSequenceItem.getRTReferencedStudySequence();
  if (!rtReferencedStudySequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: No referenced study sequence object item is available");
    return NULL;
  }

  DRTRTReferencedStudySequence::Item &rtReferencedStudySequenceItem = rtReferencedStudySequenceObject.getCurrentItem();
  if (!rtReferencedStudySequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: Referenced study sequence object item is invalid");
    return NULL;
  }

  DRTRTReferencedSeriesSequence &rtReferencedSeriesSequenceObject = rtReferencedStudySequenceItem.getRTReferencedSeriesSequence();
  if (!rtReferencedSeriesSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesSequence: No referenced series sequence object item is available");
    return NULL;
  }

  return &rtReferencedSeriesSequenceObject;
}

//----------------------------------------------------------------------------
OFString vtkSlicerDicomRtReader::GetReferencedSeriesInstanceUID(DRTStructureSetIOD rtStructureSetObject)
{
  OFString invalidUid("");
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequenceObject = this->GetReferencedSeriesSequence(rtStructureSetObject);
  if (!rtReferencedSeriesSequenceObject || !rtReferencedSeriesSequenceObject->gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedSeriesInstanceUID: No referenced series sequence object item is available");
    return invalidUid;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject->getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedSeriesInstanceUID: Referenced series sequence object item is invalid");
    return invalidUid;
  }

  OFString referencedSeriesInstanceUID("");
  rtReferencedSeriesSequenceItem.getSeriesInstanceUID(referencedSeriesInstanceUID);
  return referencedSeriesInstanceUID;
}

//----------------------------------------------------------------------------
DRTContourImageSequence* vtkSlicerDicomRtReader::GetReferencedFrameOfReferenceContourImageSequence(DRTStructureSetIOD &rtStructureSetObject)
{
  DRTRTReferencedSeriesSequence* rtReferencedSeriesSequenceObject = this->GetReferencedSeriesSequence(rtStructureSetObject);
  if (!rtReferencedSeriesSequenceObject || !rtReferencedSeriesSequenceObject->gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedFrameOfReferenceContourImageSequence: No referenced series sequence object item is available");
    return NULL;
  }

  DRTRTReferencedSeriesSequence::Item &rtReferencedSeriesSequenceItem = rtReferencedSeriesSequenceObject->getCurrentItem();
  if (!rtReferencedSeriesSequenceItem.isValid())
  {
    vtkErrorMacro("GetReferencedFrameOfReferenceContourImageSequence: Referenced series sequence object item is invalid");
    return NULL;
  }

  DRTContourImageSequence &rtContourImageSequenceObject = rtReferencedSeriesSequenceItem.getContourImageSequence();
  if (!rtContourImageSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("GetReferencedFrameOfReferenceContourImageSequence: No contour image sequence object item is available");
    return NULL;
  }

  return &rtContourImageSequenceObject;
}

//----------------------------------------------------------------------------
// Variables for calculating the distance between contour planes.
double vtkSlicerDicomRtReader::CalculateDistanceBetweenContourPlanes(DRTROIContourSequence &rtROIContourSequenceObject)
{
  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("CalculateDistanceBetweenContourPlanes: No structure sets were found. Returning default of 1.0.");
    return 1.0;
  }

  // Iterate over each ROI then each contour in the set until you find one that gives you a result
  std::vector<double> planeSpacingValues;
  bool consistentPlaneSpacing = true;
  double distanceBetweenContourPlanes = -1.0; // The found distance between planes if consistent
  int roiIndex = 0;

  do
  {
    DRTROIContourSequence::Item &currentRoiObject = rtROIContourSequenceObject.getCurrentItem();
    if (!currentRoiObject.isValid())
    {
      continue;
    }
    // Get contour sequence
    DRTContourSequence &rtContourSequenceObject = currentRoiObject.getContourSequence();
    if (!rtContourSequenceObject.gotoFirstItem().good())
    {
      vtkErrorMacro("CalculateDistanceBetweenContourPlanes: Contour sequence for ROI is empty!");
      continue;
    }

    if (rtContourSequenceObject.isEmpty())
    {
      vtkErrorMacro("Empty contour. Skipping.");
      continue;
    }
    if (rtContourSequenceObject.getNumberOfItems() < 2)
    {
      vtkDebugMacro("CalculateDistanceBetweenContourPlanes: Unable to calculate distance between contour planes, less than two planes detected. Skipping.");
      continue;
    }
    if (!rtContourSequenceObject.gotoFirstItem().good())
    {
      vtkErrorMacro("CalculateDistanceBetweenContourPlanes: Contour sequence object is invalid. Skipping.");
      continue;
    }

    // All contour planes in the current ROI, with their distances from the first plane
    std::map< vtkSmartPointer<vtkPlane>, double > contourPlanes;
    double firstNormal[3] = {0.0,0.0,0.0};
    double firstOrigin[3] = {0.0,0.0,0.0};

    // Iterate over each plane in the contour
    do 
    {
      DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();
      if ( !contourItem.isValid())
      {
        continue;
      }

      OFString numberofpoints("");
      contourItem.getNumberOfContourPoints(numberofpoints);

      std::stringstream ss;
      ss << numberofpoints;
      int number;
      ss >> number;
      if (number < 3)
      {
        Sint32 contourNumber(-1);
        contourItem.getContourNumber(contourNumber);
        vtkWarningMacro("CalculateDistanceBetweenContourPlanes: Contour does not contain enough points to extract a planar equation. Skipping contour number: " << contourNumber << ".");
        continue;
      }

      OFVector<vtkTypeFloat64> contourData_LPS;
      contourItem.getContourData(contourData_LPS);
      vtkVector3<vtkTypeFloat64> firstPlanePoint(0.0,0.0,0.0);
      vtkVector3<vtkTypeFloat64> secondPlanePoint(0.0,0.0,0.0);
      vtkVector3<vtkTypeFloat64> thirdPlanePoint(0.0,0.0,0.0);

      vtkVector3<vtkTypeFloat64> currentPlaneIVector(0.0,0.0,0.0);
      vtkVector3<vtkTypeFloat64> currentPlaneJVector(0.0,0.0,0.0);
      vtkVector3<vtkTypeFloat64> currentPlaneKVector(0.0,0.0,0.0);

      for (unsigned int i = 0; i < contourData_LPS.size(); i+=3)
      {
        if (i+8 >= contourData_LPS.size())
        {
          // We reached the end of the data (comes in coordinate groups of 9)
          break;
        }
        // We want to compute the normal vector in RAS, not LPS, so we negate the first two coordinates
        firstPlanePoint.Set(-contourData_LPS[i], -contourData_LPS[i+1], contourData_LPS[i+2]);
        secondPlanePoint.Set(-contourData_LPS[i+3], -contourData_LPS[i+4], contourData_LPS[i+5]);
        thirdPlanePoint.Set(-contourData_LPS[i+6], -contourData_LPS[i+7], contourData_LPS[i+8]);

        currentPlaneIVector = secondPlanePoint - firstPlanePoint;
        currentPlaneJVector = thirdPlanePoint - firstPlanePoint;
        currentPlaneKVector = currentPlaneIVector.Cross(currentPlaneJVector);

        if (!(currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0))
        {
          break;
        }
      }

      if (currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0)
      {
        Sint32 planeNumber;
        contourItem.getContourNumber(planeNumber);
        vtkErrorMacro("CalculateDistanceBetweenContourPlanes: All points in contour plane " << planeNumber << " in contour " << this->RoiSequenceVector[roiIndex].Name << " produce co-linear vectors. Unable to determine equation of the plane.");
        break;
      }

      currentPlaneKVector.Normalize();
      vtkSmartPointer<vtkPlane> currentContourPlane = vtkSmartPointer<vtkPlane>::New();
      currentContourPlane->SetNormal(currentPlaneKVector.GetX(), currentPlaneKVector.GetY(), currentPlaneKVector.GetZ());
      currentContourPlane->SetOrigin(firstPlanePoint.GetX(), firstPlanePoint.GetY(), firstPlanePoint.GetZ());

      // Store first plane parameters to compute distances
      if (contourPlanes.empty())
      {
        currentContourPlane->GetNormal(firstNormal);
        currentContourPlane->GetOrigin(firstOrigin);
        contourPlanes[currentContourPlane] = 0.0;
      }
      // Compute distance from first plane (also check if they are parallel)
      else
      {
        double normal[3] = {0.0,0.0,0.0};
        currentContourPlane->GetNormal(normal);
        // We accept normal vectors with the exact opposite direction, so we accept if their dot product is 1 or -1 (normal vectors have magnitude of 1)
        double dotProduct = (normal[0]*firstNormal[0]) + (normal[1]*firstNormal[1]) + (normal[2]*firstNormal[2]);
        if (!AreEqualWithTolerance(fabs(dotProduct), 1.0))
        {
          vtkErrorMacro("CalculateDistanceBetweenContourPlanes: Contour planes in structures set are not parallel!");
        }

        // Store distance of current plane from first plane
        double distanceFromFirstPlane = vtkPlane::DistanceToPlane(firstOrigin, normal, currentContourPlane->GetOrigin());
        contourPlanes[currentContourPlane] = distanceFromFirstPlane;
      }
    }
    while (rtContourSequenceObject.gotoNextItem().good()); // For all contour planes

    // Order planes for current ROI
    std::map< double, vtkSmartPointer<vtkPlane> > orderedContourPlanes;
    std::map< vtkSmartPointer<vtkPlane>, double >::iterator planesIt;
    for (planesIt=contourPlanes.begin(); planesIt != contourPlanes.end(); ++planesIt)
    {
      // Swap map to have the ordering by distance from first plane
      orderedContourPlanes[planesIt->second] = planesIt->first;
    }

    // Save contour plane normal and ordered planes to ROI entry
    Sint32 referencedRoiNumber = -1;
    currentRoiObject.getReferencedROINumber(referencedRoiNumber);
    RoiEntry* roiEntry = this->FindRoiByNumber(referencedRoiNumber);
    if (roiEntry == NULL)
    {
      vtkErrorMacro("CalculateDistanceBetweenContourPlanes: ROI with number " << referencedRoiNumber << " is not found!");
    }
    else
    {
      roiEntry->ContourPlaneNormalVector[0] = firstNormal[0];
      roiEntry->ContourPlaneNormalVector[1] = firstNormal[1];
      roiEntry->ContourPlaneNormalVector[2] = firstNormal[2];

      roiEntry->OrderedContourPlanes = orderedContourPlanes;
    }

    // Compute distances between adjacent planes
    std::map< double, vtkSmartPointer<vtkPlane> >::iterator orderedPlanesIt;
    double previousDistance = 0.0;
    for (orderedPlanesIt=orderedContourPlanes.begin(); orderedPlanesIt != orderedContourPlanes.end(); ++orderedPlanesIt)
    {
      if (orderedPlanesIt != orderedContourPlanes.begin()) // We skip the first one, just save its distance as previous
      {
        double currentDistance = fabs(orderedPlanesIt->first - previousDistance);
        if (!AreEqualWithTolerance(currentDistance, 0.0))
        {
          // Only add spacing value if it's not 0 - multiple contours may be drawn on the same plane and it's not considered for slice thickness computation
          planeSpacingValues.push_back(currentDistance);
          
          // Store current spacing as found spacing if has not been set
          if (distanceBetweenContourPlanes == -1.0)
          {
            distanceBetweenContourPlanes = currentDistance;
          }

          // Check for inconsistency
          if ( !AreEqualWithTolerance(currentDistance, distanceBetweenContourPlanes)
            && consistentPlaneSpacing ) // Only prompt the warning once for each ROI
          {
            vtkWarningMacro("CalculateDistanceBetweenContourPlanes: Contour '" << this->RoiSequenceVector[roiIndex].Name << "' does not have consistent plane spacing (" << currentDistance << " != " << distanceBetweenContourPlanes << "). Using majority spacing distance.");
            consistentPlaneSpacing = false;
          }
        } // If non-zero
      }
      previousDistance = orderedPlanesIt->first;
    }

    ++roiIndex;
  }
  while(rtROIContourSequenceObject.gotoNextItem().good()); // For all ROIs

  // Calculate the majority value for the plane spacing from plane spacing values if inconsistent spacing was found
  if (!consistentPlaneSpacing)
  {
    std::string message("");
    distanceBetweenContourPlanes = MajorityValue(planeSpacingValues, message);
    vtkWarningMacro("CalculateDistanceBetweenContourPlanes: Inconsistent plane spacing. Details:\n" << message << "Used contour spacing: " << distanceBetweenContourPlanes);
  }

  return distanceBetweenContourPlanes;
} 


//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTStructureSet(DcmDataset* dataset)
{
  this->LoadRTStructureSetSuccessful = false;

  DRTStructureSetIOD rtStructureSetObject;
  if (rtStructureSetObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTStructureSet: Could not load strucure set object from dataset");
    return;
  }

  vtkDebugMacro("LoadRTStructureSet: RT Structure Set object");

  // Read ROI name, description, and number into the ROI contour sequence vector (StructureSetROISequence)
  DRTStructureSetROISequence &rtStructureSetROISequenceObject = rtStructureSetObject.getStructureSetROISequence();
  if (!rtStructureSetROISequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("LoadRTStructureSet: No structure sets were found");
    return;
  }
  do
  {
    DRTStructureSetROISequence::Item &currentROISequenceObject = rtStructureSetROISequenceObject.getCurrentItem();
    if (!currentROISequenceObject.isValid())
    {
      continue;
    }
    RoiEntry roiEntry;

    OFString roiName("");
    currentROISequenceObject.getROIName(roiName);
    roiEntry.Name = roiName.c_str();

    OFString roiDescription("");
    currentROISequenceObject.getROIDescription(roiDescription);
    roiEntry.Description = roiDescription.c_str();                   

    OFString referencedFrameOfReferenceUid("");
    currentROISequenceObject.getReferencedFrameOfReferenceUID(referencedFrameOfReferenceUid);
    roiEntry.ReferencedFrameOfReferenceUID = referencedFrameOfReferenceUid.c_str();

    Sint32 roiNumber = -1;
    currentROISequenceObject.getROINumber(roiNumber);
    roiEntry.Number=roiNumber;

    // Save to vector          
    this->RoiSequenceVector.push_back(roiEntry);
  }
  while (rtStructureSetROISequenceObject.gotoNextItem().good());

  // Get referenced anatomical image
  OFString referencedSeriesInstanceUID = this->GetReferencedSeriesInstanceUID(rtStructureSetObject);

  // Get the slice thickness from the referenced anatomical image
  OFString firstReferencedSOPInstanceUID("");

  DRTROIContourSequence &rtROIContourSequenceObject = rtStructureSetObject.getROIContourSequence();
  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("LoadRTStructureSet: No ROIContourSequence found!");
    return;
  }

  // Get distance between contour planes
  // Ordered planes and normal vectors are set in this function to the ROI entries
  double sliceThickness = this->CalculateDistanceBetweenContourPlanes(rtROIContourSequenceObject);

  // Reset the ROI contour sequence to the start
  if (!rtROIContourSequenceObject.gotoFirstItem().good())
  {
    vtkErrorMacro("LoadRTStructureSet: No ROIContourSequence found!");
    return;
  }

  // Used for connection from one planar contour ROI to the corresponding anatomical volume slice instance
  std::map<int, std::string> contourToSliceInstanceUIDMap;
  std::set<std::string> referencedSopInstanceUids;

  // Read ROIs (ROIContourSequence)
  do 
  {
    DRTROIContourSequence::Item &currentRoiObject = rtROIContourSequenceObject.getCurrentItem();
    if (!currentRoiObject.isValid())
    {
      continue;
    }

    // Create containers for vtkPolyData
    vtkSmartPointer<vtkPoints> tempPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> tempCellArray = vtkSmartPointer<vtkCellArray>::New();
    vtkIdType pointId = 0;

    // Get ROI entry created for the referenced ROI
    Sint32 referencedRoiNumber = -1;
    currentRoiObject.getReferencedROINumber(referencedRoiNumber);
    RoiEntry* roiEntry = this->FindRoiByNumber(referencedRoiNumber);
    if (roiEntry == NULL)
    {
      vtkErrorMacro("LoadRTStructureSet: ROI with number " << referencedRoiNumber << " is not found!");      
      continue;
    } 

    // Get contour sequence
    DRTContourSequence &rtContourSequenceObject = currentRoiObject.getContourSequence();
    if (!rtContourSequenceObject.gotoFirstItem().good())
    {
      vtkErrorMacro("LoadRTStructureSet: Contour sequence for ROI named '"
        << roiEntry->Name << "' with number " << referencedRoiNumber << " is empty!");
      continue;
    }

    // Read contour data
    do
    {
      // Get contour
      DRTContourSequence::Item &contourItem = rtContourSequenceObject.getCurrentItem();
      if (!contourItem.isValid())
      {
        continue;
      }

      // Get number of contour points
      OFString numberOfPointsString("");
      contourItem.getNumberOfContourPoints(numberOfPointsString);
      std::stringstream ss;
      ss << numberOfPointsString;
      int numberOfPoints;
      ss >> numberOfPoints;

      // Get contour point data
      OFVector<vtkTypeFloat64> contourData_LPS;
      contourItem.getContourData(contourData_LPS);

      unsigned int contourIndex = tempCellArray->InsertNextCell(numberOfPoints+1);
      for (int k=0; k<numberOfPoints; k++)
      {
        // Convert from DICOM LPS -> Slicer RAS
        tempPoints->InsertPoint(pointId, -contourData_LPS[3*k], -contourData_LPS[3*k+1], contourData_LPS[3*k+2]);
        tempCellArray->InsertCellPoint(pointId);
        pointId++;
      }

      // Close the contour
      tempCellArray->InsertCellPoint(pointId-numberOfPoints);

      // Add map to the referenced slice instance UID
      // This is not a mandatory field so no error logged if not found. The reason why
      // it is still read and stored is that it references the contours individually
      DRTContourImageSequence &rtContourImageSequenceObject = contourItem.getContourImageSequence();
      if (rtContourImageSequenceObject.gotoFirstItem().good())
      {
        DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject.getCurrentItem();
        if (rtContourImageSequenceItem.isValid())
        {
          OFString referencedSOPInstanceUID("");
          rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
          contourToSliceInstanceUIDMap[contourIndex] = referencedSOPInstanceUID.c_str();
          referencedSopInstanceUids.insert(referencedSOPInstanceUID.c_str());

          // Check if multiple SOP instance UIDs are referenced
          if (rtContourImageSequenceObject.getNumberOfItems() > 1)
          {
            vtkWarningMacro("LoadRTStructureSet: Contour in ROI " << roiEntry->Number << ": " << roiEntry->Name << " contains multiple referenced instances. This is not yet supported!");
          }
        }
        else
        {
          vtkErrorMacro("LoadRTStructureSet: Contour image sequence object item is invalid");
        }
      }
    }
    while (rtContourSequenceObject.gotoNextItem().good());

    // Read slice reference UIDs from referenced frame of reference sequence if it was not included in the ROIContourSequence above
    if (contourToSliceInstanceUIDMap.empty())
    {
      DRTContourImageSequence* rtContourImageSequenceObject = this->GetReferencedFrameOfReferenceContourImageSequence(rtStructureSetObject);
      if (rtContourImageSequenceObject && rtContourImageSequenceObject->gotoFirstItem().good())
      {
        int currentSliceNumber = -1; // Use negative keys to indicate that the slice instances cannot be directly mapped to the ROI planar contours
        do 
        {
          DRTContourImageSequence::Item &rtContourImageSequenceItem = rtContourImageSequenceObject->getCurrentItem();
          if (rtContourImageSequenceItem.isValid())
          {
            OFString referencedSOPInstanceUID("");
            rtContourImageSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID);
            contourToSliceInstanceUIDMap[currentSliceNumber] = referencedSOPInstanceUID.c_str();
            referencedSopInstanceUids.insert(referencedSOPInstanceUID.c_str());
          }
          else
          {
            vtkErrorMacro("LoadRTStructureSet: Contour image sequence object item in referenced frame of reference sequence is invalid");
          }
          currentSliceNumber--;
        }
        while (rtContourImageSequenceObject->gotoNextItem().good());
      }
      else
      {
        vtkErrorMacro("LoadRTStructureSet: No items in contour image sequence object item in referenced frame of reference sequence!");
      }
    }

    // Save just loaded contour data into ROI entry
    vtkSmartPointer<vtkPolyData> tempPolyData = vtkSmartPointer<vtkPolyData>::New();
    tempPolyData->SetPoints(tempPoints);
    if (tempPoints->GetNumberOfPoints() == 1)
    {
      // Point ROI
      tempPolyData->SetVerts(tempCellArray);
    }
    else if (tempPoints->GetNumberOfPoints() > 1)
    {
      // Contour ROI
      tempPolyData->SetLines(tempCellArray);
    }
    roiEntry->SetPolyData(tempPolyData);

    // Get structure color
    Sint32 roiDisplayColor = -1;
    for (int j=0; j<3; j++)
    {
      currentRoiObject.getROIDisplayColor(roiDisplayColor,j);
      roiEntry->DisplayColor[j] = roiDisplayColor/255.0;
    }

    // Set referenced series UID
    roiEntry->ReferencedSeriesUID = (std::string)referencedSeriesInstanceUID.c_str();

    // Set slice thickness
    roiEntry->SliceThickness = sliceThickness;

    // Set referenced SOP instance UIDs
    roiEntry->ContourIndexToSOPInstanceUIDMap = contourToSliceInstanceUIDMap;

    // Serialize referenced SOP instance UID set
    std::set<std::string>::iterator uidIt;
    std::string serializedUidList("");
    for (uidIt = referencedSopInstanceUids.begin(); uidIt != referencedSopInstanceUids.end(); ++uidIt)
    {
      serializedUidList.append(*uidIt);
      serializedUidList.append(" ");
    }
    // Strip last space
    serializedUidList = serializedUidList.substr(0, serializedUidList.size()-1);
    this->SetRTStructureSetReferencedSOPInstanceUIDs(serializedUidList.c_str());
  }
  while (rtROIContourSequenceObject.gotoNextItem().good());

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtStructureSetObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorMacro("LoadRTStructureSet: Failed to get SOP instance UID for RT structure set!");
    return; // mandatory DICOM value
  }
  this->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtStructureSetObject);

  this->LoadRTStructureSetSuccessful = true;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfRois()
{
  return this->RoiSequenceVector.size();
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiName(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiName: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return (this->RoiSequenceVector[internalIndex].Name.empty() ? SlicerRtCommon::DICOMRTIMPORT_NO_NAME : this->RoiSequenceVector[internalIndex].Name).c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetRoiDisplayColor(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiDisplayColor: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].DisplayColor;
}

//----------------------------------------------------------------------------
vtkPolyData* vtkSlicerDicomRtReader::GetRoiPolyData(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiPolyData: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].PolyData;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetRoiReferencedSeriesUid(unsigned int internalIndex)
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiReferencedSeriesUid: Cannot get ROI with internal index: " << internalIndex);
    return NULL;
  }
  return this->RoiSequenceVector[internalIndex].ReferencedSeriesUID.c_str();
}

//---------------------------------------------------------------------------
std::map<double, vtkSmartPointer<vtkPlane> > vtkSlicerDicomRtReader::GetRoiOrderedContourPlanes( unsigned int internalIndex )
{
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("GetRoiOrderedContourPlanes: Cannot get ROI with internal index: " << internalIndex);
    std::map<double, vtkSmartPointer<vtkPlane> > noPlanes;
    return noPlanes;
  }
  return this->RoiSequenceVector[internalIndex].OrderedContourPlanes;
}

//----------------------------------------------------------------------------
int vtkSlicerDicomRtReader::GetNumberOfBeams()
{
  return this->BeamSequenceVector.size();
}

//----------------------------------------------------------------------------
unsigned int vtkSlicerDicomRtReader::GetBeamNumberForIndex(unsigned int index)
{
  return this->BeamSequenceVector[index].Number;
}

//----------------------------------------------------------------------------
const char* vtkSlicerDicomRtReader::GetBeamName(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }
  return (beam->Name.empty() ? SlicerRtCommon::DICOMRTIMPORT_NO_NAME : beam->Name).c_str();
}

//----------------------------------------------------------------------------
double* vtkSlicerDicomRtReader::GetBeamIsocenterPositionRas(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    return NULL;
  }  
  return beam->IsocenterPositionRas;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamSourceAxisDistance(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamSourceAxisDistance: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->SourceAxisDistance;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamGantryAngle(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamGantryAngle: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->GantryAngle;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamPatientSupportAngle(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamPatientSupportAngle: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->PatientSupportAngle;
}

//----------------------------------------------------------------------------
double vtkSlicerDicomRtReader::GetBeamBeamLimitingDeviceAngle(unsigned int beamNumber)
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    vtkErrorMacro("GetBeamBeamLimitingDeviceAngle: Unable to find beam of number" << beamNumber);
    return 0.0;
  }  
  return beam->BeamLimitingDeviceAngle;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::GetBeamLeafJawPositions(unsigned int beamNumber, double jawPositions[2][2])
{
  BeamEntry* beam=this->FindBeamByNumber(beamNumber);
  if (beam==NULL)
  {
    jawPositions[0][0]=jawPositions[0][1]=jawPositions[1][0]=jawPositions[1][1]=0.0;
    return;
  }  
  jawPositions[0][0]=beam->LeafJawPositions[0][0];
  jawPositions[0][1]=beam->LeafJawPositions[0][1];
  jawPositions[1][0]=beam->LeafJawPositions[1][0];
  jawPositions[1][1]=beam->LeafJawPositions[1][1];
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::LoadRTDose(DcmDataset* dataset)
{
  this->LoadRTDoseSuccessful = false;

  DRTDoseIOD rtDoseObject;
  if (rtDoseObject.read(*dataset).bad())
  {
    vtkErrorMacro("LoadRTDose: Failed to read RT Dose dataset!");
    return;
  }

  vtkDebugMacro("LoadRTDose: Load RT Dose object");

  OFString doseGridScaling("");
  if (rtDoseObject.getDoseGridScaling(doseGridScaling).bad())
  {
    vtkErrorMacro("LoadRTDose: Failed to get Dose Grid Scaling for dose object");
    return; // mandatory DICOM value
  }
  else if (doseGridScaling.empty())
  {
    vtkWarningMacro("LoadRTDose: Dose grid scaling tag is missing or empty! Using default dose grid scaling 0.0001.");
    doseGridScaling = "0.0001";
  }

  this->SetDoseGridScaling(doseGridScaling.c_str());

  OFString doseUnits("");
  if (rtDoseObject.getDoseUnits(doseUnits).bad())
  {
    vtkErrorMacro("LoadRTDose: Failed to get Dose Units for dose object");
    return; // mandatory DICOM value
  }
  this->SetDoseUnits(doseUnits.c_str());

  OFVector<vtkTypeFloat64> pixelSpacingOFVector;
  if (rtDoseObject.getPixelSpacing(pixelSpacingOFVector).bad() || pixelSpacingOFVector.size() < 2)
  {
    vtkErrorMacro("LoadRTDose: Failed to get Pixel Spacing for dose object");
    return; // mandatory DICOM value
  }
  this->SetPixelSpacing(pixelSpacingOFVector[0], pixelSpacingOFVector[1]);
  vtkDebugMacro("Pixel Spacing: (" << pixelSpacingOFVector[0] << ", " << pixelSpacingOFVector[1] << ")");

  // Get referenced RTPlan instance UID
  DRTReferencedRTPlanSequence &referencedRTPlanSequence = rtDoseObject.getReferencedRTPlanSequence();
  if (referencedRTPlanSequence.gotoFirstItem().good())
  {
    DRTReferencedRTPlanSequence::Item &referencedRTPlanSequenceItem = referencedRTPlanSequence.getCurrentItem();
    if (referencedRTPlanSequenceItem.isValid())
    {
      OFString referencedSOPInstanceUID("");
      if (referencedRTPlanSequenceItem.getReferencedSOPInstanceUID(referencedSOPInstanceUID).good())
      {
        this->SetRTDoseReferencedRTPlanSOPInstanceUID(referencedSOPInstanceUID.c_str());
      }
    }
  }

  // SOP instance UID
  OFString sopInstanceUid("");
  if (rtDoseObject.getSOPInstanceUID(sopInstanceUid).bad())
  {
    vtkErrorMacro("LoadRTDose: Failed to get SOP instance UID for RT dose!");
    return; // mandatory DICOM value
  }
  this->SetSOPInstanceUID(sopInstanceUid.c_str());

  // Get and store patient, study and series information
  this->GetAndStoreHierarchyInformation(&rtDoseObject);

  this->LoadRTDoseSuccessful = true;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::BeamEntry* vtkSlicerDicomRtReader::FindBeamByNumber(unsigned int beamNumber)
{
  for (unsigned int i=0; i<this->BeamSequenceVector.size(); i++)
  {
    if (this->BeamSequenceVector[i].Number == beamNumber)
    {
      return &this->BeamSequenceVector[i];
    }
  }

  // Not found
  vtkErrorMacro("FindBeamByNumber: Beam cannot be found for number " << beamNumber);
  return NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtReader::RoiEntry* vtkSlicerDicomRtReader::FindRoiByNumber(unsigned int roiNumber)
{
  for (unsigned int i=0; i<this->RoiSequenceVector.size(); i++)
  {
    if (this->RoiSequenceVector[i].Number == roiNumber)
    {
      return &this->RoiSequenceVector[i];
    }
  }

  // Not found
  vtkErrorMacro("FindBeamByNumber: ROI cannot be found for number " << roiNumber);
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtReader::CreateRibbonModelForRoi(unsigned int internalIndex, vtkPolyData* ribbonModelPolyData)
{
  if (ribbonModelPolyData == NULL)
  {
    vtkErrorMacro("CreateRibbonModelForRoi: Input ribbon model poly data is NULL!");
    return;
  }

  // Get ROI entry
  if (internalIndex >= this->RoiSequenceVector.size())
  {
    vtkErrorMacro("CreateRibbonModelForRoi: Cannot get ROI with internal index: " << internalIndex);
    return;
  }

  vtkPolyData* roiPolyData = this->RoiSequenceVector[internalIndex].PolyData;
  if (!roiPolyData || roiPolyData->GetNumberOfPoints() == 0)
  {
    vtkErrorMacro("CreateRibbonModelForRoi: Invalid ROI with internal index: " << internalIndex);
    return;
  }
  else if (roiPolyData->GetNumberOfPoints() == 1)
  {
    vtkWarningMacro("CreateRibbonModelForRoi: Point ROI does not need to be ribbonized with internal index: " << internalIndex);
    ribbonModelPolyData->DeepCopy(roiPolyData);
    return;
  }

  // Remove coincident points (if there are multiple contour points at the same position then the ribbon filter fails)
  vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
#if (VTK_MAJOR_VERSION <= 5)
  cleaner->SetInput(roiPolyData);
#else
  cleaner->SetInputData(roiPolyData);
#endif


  // Convert to ribbon using vtkRibbonFilter
  vtkSmartPointer<vtkRibbonFilter> ribbonFilter = vtkSmartPointer<vtkRibbonFilter>::New();
  ribbonFilter->SetInputConnection(cleaner->GetOutputPort());
  // It is better to allow per-contour normal vectors in case the planes within a structure are not parallel
  //ribbonFilter->SetDefaultNormal(this->RoiSequenceVector[internalIndex].ContourPlaneNormalVector);
  //ribbonFilter->UseDefaultNormalOn();
  ribbonFilter->SetWidth(this->RoiSequenceVector[internalIndex].SliceThickness / 2.0);
  ribbonFilter->SetAngle(90.0);
  ribbonFilter->Update();

  vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalFilter->SetInputConnection(ribbonFilter->GetOutputPort());
  normalFilter->ConsistencyOn();
  normalFilter->Update();

  ribbonModelPolyData->DeepCopy(normalFilter->GetOutput());
}
