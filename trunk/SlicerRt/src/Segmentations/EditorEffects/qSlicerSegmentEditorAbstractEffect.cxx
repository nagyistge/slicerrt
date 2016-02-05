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
#include "qSlicerSegmentEditorAbstractEffect.h"
#include "qSlicerSegmentEditorAbstractEffect_p.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentEditorNode.h"
#include "vtkOrientedImageData.h"

// Qt includes
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QPaintDevice>
#include <QFrame>
#include <QVBoxLayout>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLThreeDView.h"
#include "vtkMRMLSliceLogic.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLViewNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMatrix4x4.h>
#include <vtkProp.h>

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractEffectPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::qSlicerSegmentEditorAbstractEffectPrivate(qSlicerSegmentEditorAbstractEffect& object)
  : q_ptr(&object)
  , Scene(NULL)
  , ParameterSetNode(NULL)
  , SavedCursor(QCursor(Qt::ArrowCursor))
  , OptionsFrame(NULL)
{
  this->OptionsFrame = new QFrame();
  this->OptionsFrame->setFrameShape(QFrame::NoFrame);
  this->OptionsFrame->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  QVBoxLayout* layout = new QVBoxLayout(this->OptionsFrame);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::~qSlicerSegmentEditorAbstractEffectPrivate()
{
  if (this->OptionsFrame)
  {
    delete this->OptionsFrame;
    this->OptionsFrame = NULL;
  }
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorAbstractEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::qSlicerSegmentEditorAbstractEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorAbstractEffectPrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::~qSlicerSegmentEditorAbstractEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::activate()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Show options frame
  d->OptionsFrame->setVisible(true);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::deactivate()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Hide options frame
  d->OptionsFrame->setVisible(false);

  // Remove actors from container
  QMapIterator<qMRMLWidget*, QList<vtkProp*> > actorsIterator(d->Actors);
  while (actorsIterator.hasNext())
  {
    actorsIterator.next();
    qMRMLWidget* viewWidget = actorsIterator.key();
    foreach (vtkProp* actor, actorsIterator.value())
    {
      vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(viewWidget);
      if (!renderer)
      {
        qCritical() << "qSlicerSegmentEditorAbstractEffect::deactivate: Failed to get renderer for view widget!";
        continue;
      }

      // Call both actor removal functions to support both slice and 3D views
      // (the 2D actors are members of vtkViewport, base class of vtkRenderer,
      // while vtkRenderer has another, general actor list)
      renderer->RemoveActor(actor);
      renderer->RemoveActor2D(actor);
    }

    // Schedule render
    qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
    qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
    if (sliceWidget)
    {
      sliceWidget->sliceView()->scheduleRender();
    }
    else if (threeDWidget)
    {
      threeDWidget->threeDView()->scheduleRender();
    }
    else
    {
      qCritical() << "qSlicerSegmentEditorAbstractEffect::deactivate: Unsupported view widget!";
      continue;
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::connectApply(QObject* receiver, const char* method)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Connect apply signal to commit changes to selected segment
  QObject::connect(d, SIGNAL(applySignal()), receiver, method);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::apply()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  emit d->applySignal();
}

//-----------------------------------------------------------------------------
QCursor qSlicerSegmentEditorAbstractEffect::createCursor(qMRMLWidget* viewWidget)
{
  Q_UNUSED(viewWidget); // The default cursor is not view-specific, but this method can be overridden

  QImage baseImage(":Icons/CursorBaseArrow.png");
  QIcon effectIcon(this->icon());
  QImage effectImage(effectIcon.pixmap(effectIcon.availableSizes()[0]).toImage());
  int width = qMax(baseImage.width(), effectImage.width());
  int pad = -9;
  int height = pad + baseImage.height() + effectImage.height();
  width = height = qMax(width,height);
  int center = width/2;
  QImage cursorImage(width, height, QImage::Format_ARGB32);
  QPainter painter;
  cursorImage.fill(0);
  painter.begin(&cursorImage);
  QPoint point(center - (baseImage.width()/2), 0);
  painter.drawImage(point, baseImage);
  point.setX(center - (effectImage.width()/2));
  point.setY(cursorImage.height() - effectImage.height());
  painter.drawImage(point, effectImage);
  painter.end();
  QPixmap cursorPixmap = QPixmap::fromImage(cursorImage);
  return QCursor(cursorPixmap, center, 0);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOff(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  d->SavedCursor = QCursor(viewWidget->cursor());
  viewWidget->setCursor(QCursor(Qt::BlankCursor));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOn(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  viewWidget->setCursor(d->SavedCursor);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addActor(qMRMLWidget* viewWidget, vtkProp* actor)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  if (d->Actors.contains(viewWidget))
  {
    d->Actors[viewWidget] << actor;
  }
  else
  {
    QList<vtkProp*> actorList;
    actorList << actor;
    d->Actors[viewWidget] = actorList;
  }
}

//-----------------------------------------------------------------------------
QFrame* qSlicerSegmentEditorAbstractEffect::optionsFrame()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  return d->OptionsFrame;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addOptionsWidget(QWidget* newOptionsWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  newOptionsWidget->setParent(d->OptionsFrame);
  newOptionsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));
  d->OptionsFrame->layout()->addWidget(newOptionsWidget);
}

//-----------------------------------------------------------------------------
vtkMRMLScene* qSlicerSegmentEditorAbstractEffect::scene()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  if (!d->ParameterSetNode)
  {
    return NULL;
  }

  return d->ParameterSetNode->GetScene();
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentEditorNode* qSlicerSegmentEditorAbstractEffect::parameterSetNode()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  return d->ParameterSetNode.GetPointer();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameterSetNode(vtkMRMLSegmentEditorNode* node)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  d->ParameterSetNode = node;
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorAbstractEffect::parameter(QString name)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    return QString();
  }

  QString attributeName = QString("%1.%2").arg(this->name()).arg(name);
  const char* value = d->ParameterSetNode->GetAttribute(attributeName.toLatin1().constData());
  if (!value)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::parameter: Parameter named " << name << " cannot be found for effect " << this->name();
    return QString();
  }

  return QString(value);
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorAbstractEffect::integerParameter(QString name)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    return 0;
  }

  QString parameterStr = this->parameter(name);
  bool ok = false;
  int parameterInt = parameterStr.toInt(&ok);
  if (!ok)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::integerParameter: Parameter named " << name << " cannot be converted to integer!";
    return 0;
  }

  return parameterInt;
}

//-----------------------------------------------------------------------------
double qSlicerSegmentEditorAbstractEffect::doubleParameter(QString name)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    return 0.0;
  }

  QString parameterStr = this->parameter(name);
  bool ok = false;
  double parameterDouble = parameterStr.toDouble(&ok);
  if (!ok)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::doubleParameter: Parameter named " << name << " cannot be converted to floating point number!";
    return 0.0;
  }

  return parameterDouble;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, QString value, bool emitParameterModifiedEvent/*=false*/)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);
  if (!d->ParameterSetNode)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::setParameter: Invalid segment editor parameter set node set to effect " << this->name();
    return;
  }

  // Disable full modified events in all cases (observe EffectParameterModified instead if necessary)
  int disableState = d->ParameterSetNode->GetDisableModifiedEvent();
  d->ParameterSetNode->SetDisableModifiedEvent(1);

  // Set parameter as attribute
  QString attributeName = QString("%1.%2").arg(this->name()).arg(name);
  d->ParameterSetNode->SetAttribute(attributeName.toLatin1().constData(), value.toLatin1().constData());

  // Re-enable full modified events for parameter node
  d->ParameterSetNode->SetDisableModifiedEvent(disableState);

  // Emit parameter modified event if requested
  if (emitParameterModifiedEvent)
  {
    d->ParameterSetNode->InvokeCustomModifiedEvent(vtkMRMLSegmentEditorNode::EffectParameterModified, (void*)(attributeName.toLatin1().constData()));
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, int value, bool emitParameterModifiedEvent/*=false*/)
{
  this->setParameter(name, QString::number(value), emitParameterModifiedEvent);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, double value, bool emitParameterModifiedEvent/*=false*/)
{
  this->setParameter(name, QString::number(value), emitParameterModifiedEvent);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::abortEvent(vtkRenderWindowInteractor* interactor, unsigned long eventId, qMRMLWidget* viewWidget)
{
  if (!interactor || !viewWidget)
  {
    return;
  }

  QVariant tagsVariant = viewWidget->property(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier());
  QList<QVariant> tagsList = tagsVariant.toList();
  foreach(QVariant tagVariant, tagsList)
  {
    unsigned long tag = tagVariant.toULongLong();
    vtkCommand* command = interactor->GetCommand(tag);
    command->SetAbortFlag(1);
  }
}

//-----------------------------------------------------------------------------
vtkRenderWindow* qSlicerSegmentEditorAbstractEffect::renderWindow(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return NULL;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    return sliceWidget->sliceView()->renderWindow();
  }
  else if (threeDWidget)
  {
    return threeDWidget->threeDView()->renderWindow();
  }

  qCritical() << "qSlicerSegmentEditorAbstractEffect::renderWindow: Unsupported view widget type!";
  return NULL;
}

//-----------------------------------------------------------------------------
vtkRenderer* qSlicerSegmentEditorAbstractEffect::renderer(qMRMLWidget* viewWidget)
{
  vtkRenderWindow* renderWindow = qSlicerSegmentEditorAbstractEffect::renderWindow(viewWidget);
  if (!renderWindow)
  {
    return NULL;
  }

  return vtkRenderer::SafeDownCast(renderWindow->GetRenderers()->GetItemAsObject(0));
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractViewNode* qSlicerSegmentEditorAbstractEffect::viewNode(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return NULL;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    return sliceWidget->sliceLogic()->GetSliceNode();
  }
  else if (threeDWidget)
  {
    return threeDWidget->mrmlViewNode();
  }

  qCritical() << "qSlicerSegmentEditorAbstractEffect::renderWindow: Unsupported view widget type!";
  return NULL;
}

//-----------------------------------------------------------------------------
QPoint qSlicerSegmentEditorAbstractEffect::rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget)
{
  QPoint xy(0,0);

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
    qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget) );
  if (!sliceNode)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::rasToXy: Failed to get slice node!";
    return xy;
  }

  double rast[4] = {ras[0], ras[1], ras[2], 1.0};
  double xyzw[4] = {0.0, 0.0, 0.0, 1.0};
  vtkSmartPointer<vtkMatrix4x4> rasToXyMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToXyMatrix->DeepCopy(sliceNode->GetXYToRAS());
  rasToXyMatrix->Invert();
  rasToXyMatrix->MultiplyPoint(rast, xyzw);

  xy.setX(xyzw[0]);
  xy.setY(xyzw[1]);
  return xy;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  outputRas[0] = outputRas[1] = outputRas[2] = 0.0;

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
    qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget) );
  if (!sliceNode)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::xyToRas: Failed to get slice node!";
    return;
  }
  
  // x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  // slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  // coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  double xyzw[4] = {inputXyz[0], inputXyz[1], inputXyz[2], 1.0};
  double rast[4] = {0.0, 0.0, 0.0, 1.0};
  sliceNode->GetXYToRAS()->MultiplyPoint(xyzw, rast);
  outputRas[0] = rast[0];
  outputRas[1] = rast[1];
  outputRas[2] = rast[2];
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  double xyz[3] = {xy.x(), xy.y(), 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceWidget);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToIjk(double inputXyz[3], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  outputIjk[0] = outputIjk[1] = outputIjk[2] = 0;

  if (!sliceWidget || !image)
  {
    return;
  }

  // Convert from XY to RAS first
  double ras[3] = {0.0, 0.0, 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(inputXyz, ras, sliceWidget);

  // Convert RAS to image IJK
  double rast[4] = {ras[0], ras[1], ras[2], 1.0};
  double ijkl[4] = {0.0, 0.0, 0.0, 1.0};
  vtkSmartPointer<vtkMatrix4x4> rasToIjkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image->GetImageToWorldMatrix(rasToIjkMatrix);
  rasToIjkMatrix->Invert();
  rasToIjkMatrix->MultiplyPoint(rast, ijkl);

  outputIjk[0] = (int)(ijkl[0] + 0.5);
  outputIjk[1] = (int)(ijkl[1] + 0.5);
  outputIjk[2] = (int)(ijkl[2] + 0.5);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint xy, int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  double xyz[3] = {xy.x(), xy.y(), 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceWidget, image);
}
