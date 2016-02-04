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

#ifndef __qSlicerSegmentEditorAbstractEffect_p_h
#define __qSlicerSegmentEditorAbstractEffect_p_h

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Slicer API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// Segmentations Editor Effects includes
#include "qSlicerSegmentationsEditorEffectsExport.h"

#include "qSlicerSegmentEditorAbstractEffect.h"

// VTK includes
#include <vtkWeakPointer.h>

// Qt includes
#include <QObject>
#include <QCursor>
#include <QMap>

class vtkMRMLScene;
class vtkMRMLSegmentEditorNode;
class qMRMLWidget;
class vtkProp;
class QFrame;

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorAbstractEffectPrivate: public QObject
{
  Q_OBJECT
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorAbstractEffect);
protected:
  qSlicerSegmentEditorAbstractEffect* const q_ptr;
public:
  qSlicerSegmentEditorAbstractEffectPrivate(qSlicerSegmentEditorAbstractEffect& object);
  ~qSlicerSegmentEditorAbstractEffectPrivate();
  void emitApplySignal() { emit applySignal(); };
signals:
  void applySignal();
public:
  /// Segment editor parameter set node
  vtkWeakPointer<vtkMRMLSegmentEditorNode> ParameterSetNode;

  /// MRML scene
  vtkMRMLScene* Scene;
  
  /// Cursor to restore after custom cursor is not needed any more
  QCursor SavedCursor;

  /// List of actors used by the effect. Removed when effect is deactivated
  QMap<qMRMLWidget*, QList<vtkProp*> > Actors;

  /// Frame containing the effect options UI.
  /// Populating the frame is possible using the \sa addOptionsWidget method from the base classes
  QFrame* OptionsFrame;
};

#endif
