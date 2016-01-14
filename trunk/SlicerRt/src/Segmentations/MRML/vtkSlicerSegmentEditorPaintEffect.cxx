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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "vtkSlicerSegmentEditorPaintEffect.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTrivialProducer.h>
#include <vtkMatrix4x4.h>
#include <vtkCallbackCommand.h>
#include <vtkPolyData.h>
#include <vtkCollection.h>
//#include <vtkRenderWindowInteractor.h>
//#include <vtkInteractorObserver.h>
//#include <vtkRenderWindow.h>
//#include <vtkRendererCollection.h>
//#include <vtkRenderer.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>
//#include <vtkMRMLSliceNode.h>

// Slicer includes
//#include "vtkMRMLSliceLogic.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSegmentEditorPaintEffect);

//----------------------------------------------------------------------------
vtkSlicerSegmentEditorPaintEffect::vtkSlicerSegmentEditorPaintEffect()
{
  this->Brush = vtkPolyData::New();
  /*
    self.createGlyph(self.brush)
    self.mapper = vtk.vtkPolyDataMapper2D()
    self.actor = vtk.vtkActor2D()
    self.mapper.SetInputData(self.brush)
    self.actor.SetMapper(self.mapper)
    self.actor.VisibilityOff()

    self.renderer.AddActor2D(self.actor)
    self.actors.append(self.actor)
  */
}

//----------------------------------------------------------------------------
vtkSlicerSegmentEditorPaintEffect::~vtkSlicerSegmentEditorPaintEffect()
{
  if (this->Brush)
  {
    this->Brush->Delete();
    this->Brush = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerSegmentEditorPaintEffect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentEditorPaintEffect::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSegmentEditorPaintEffect::RegisterNodes()
{
}

//-----------------------------------------------------------------------------
void vtkSlicerSegmentEditorPaintEffect::Activate()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("Activate: Invalid MRML scene!");
    return;
  }

}

//-----------------------------------------------------------------------------
void vtkSlicerSegmentEditorPaintEffect::CreateGlyph(vtkPolyData* polyData)
{
  /*
    sliceNode = self.sliceWidget.sliceLogic().GetSliceNode()
    self.rasToXY.DeepCopy(sliceNode.GetXYToRAS())
    self.rasToXY.Invert()
    maximum, maxIndex = 0,0
    for index in range(3):
      if abs(self.rasToXY.GetElement(0, index)) > maximum:
        maximum = abs(self.rasToXY.GetElement(0, index))
        maxIndex = index
    point = [0, 0, 0, 0]
    point[maxIndex] = self.radius
    xyRadius = self.rasToXY.MultiplyPoint(point)
    import math
    xyRadius = math.sqrt( xyRadius[0]**2 + xyRadius[1]**2 + xyRadius[2]**2 )

    if self.pixelMode:
      xyRadius = 0.01

    # make a circle paint brush
    points = vtk.vtkPoints()
    lines = vtk.vtkCellArray()
    polyData.SetPoints(points)
    polyData.SetLines(lines)
    PI = 3.1415926
    TWOPI = PI * 2
    PIoverSIXTEEN = PI / 16
    prevPoint = -1
    firstPoint = -1
    angle = 0
    while angle <= TWOPI:
      x = xyRadius * math.cos(angle)
      y = xyRadius * math.sin(angle)
      p = points.InsertNextPoint( x, y, 0 )
      if prevPoint != -1:
        idList = vtk.vtkIdList()
        idList.InsertNextId(prevPoint)
        idList.InsertNextId(p)
        polyData.InsertNextCell( vtk.VTK_LINE, idList )
      prevPoint = p
      if firstPoint == -1:
        firstPoint = p
      angle = angle + PIoverSIXTEEN

    # make the last line in the circle
    idList = vtk.vtkIdList()
    idList.InsertNextId(p)
    idList.InsertNextId(firstPoint)
    polyData.InsertNextCell( vtk.VTK_LINE, idList )
  */
}
  
//-----------------------------------------------------------------------------
void vtkSlicerSegmentEditorPaintEffect::Apply()
{
}

//---------------------------------------------------------------------------
//void vtkSlicerSegmentEditorPaintEffect::ProcessEvents(vtkObject* caller,
//                                        unsigned long eid,
//                                        void* clientData,
//                                        void* vtkNotUsed(callData))
//{
//}
