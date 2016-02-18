import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin
from SegmentEditorEffects import *

#
# SegmentEditor
# 
class SegmentEditor(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Segment Editor"
    self.parent.categories = ["Segmentation"]
    self.parent.dependencies = ["Segmentations"]
    self.parent.contributors = ["Csaba Pinter (Queen's University)"]
    self.parent.helpText = """This module allows editing segmentation objects by directly drawing and using segmentaiton tools on the contained segments.\n\nRepresentations other than the labelmap one (which is used for editing) are automatically updated real-time, so for example the closed surface can be visualized as edited in the 3D view."""
    self.parent.acknowledgementText = """ """
    
#
# SegmentEditorWidget
#
class SegmentEditorWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
  def __init__(self, parent):
    ScriptedLoadableModuleWidget.__init__(self, parent)
    VTKObservationMixin.__init__(self) 

    # Members
    self.parameterSetNode = None
    self.editor = None

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

    # Register scripted segment editor effects
    from qSlicerSegmentationsEditorEffectsPythonQt import *
    #TODO:
    thresholdEffect = qSlicerSegmentEditorScriptedEffect(None)
    thresholdEffect.setPythonSource(SegmentEditorThresholdEffect.filePath)

    # Add margin to the sides
    self.layout.setContentsMargins(4,0,4,0)

    #
    # Parameter Combobox
    #
    self.parameterSelector = slicer.qMRMLNodeComboBox()
    self.parameterLabel = qt.QLabel("Parameter set: ")
    self.parameterSelector.nodeTypes = ["vtkMRMLSegmentEditorNode"]
    self.parameterSelector.removeEnabled = False
    self.parameterSelector.showHidden = True
    self.parameterSelector.setMRMLScene( slicer.mrmlScene )
    self.parameterLayout = qt.QHBoxLayout()
    self.parameterLayout.addWidget(self.parameterLabel)
    self.parameterLayout.addWidget(self.parameterSelector)
    self.layout.addLayout(self.parameterLayout)
    self.parameterSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.parameterNodeChanged)

    #
    # Segment editor widget
    #
    import qSlicerSegmentationsModuleWidgetsPythonQt
    self.editor = qSlicerSegmentationsModuleWidgetsPythonQt.qMRMLSegmentEditorWidget()
    # Set parameter node first so that the automatic selections made when the scene is set are saved
    self.selectParameterNode()
    self.editor.setMRMLScene(slicer.mrmlScene)
    self.layout.addWidget(self.editor)
    
    # Connect observers to scene events
    self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
    self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)
    self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndImportEvent, self.onSceneEndImport)

  def parameterNodeChanged(self, node):
    self.editor.setMRMLSegmentEditorNode(node)

  def selectParameterNode(self):
    # Select parameter set node if one is found in the scene, and create one otherwise
    import vtkSlicerSegmentationsModuleMRML
    node = slicer.mrmlScene.GetNthNodeByClass(0, "vtkMRMLSegmentEditorNode")
    if self.parameterSetNode is None:
      if node is not None:
        self.parameterSetNode = node
      else:
        self.parameterSetNode = vtkSlicerSegmentationsModuleMRML.vtkMRMLSegmentEditorNode()
        slicer.mrmlScene.AddNode(self.parameterSetNode)
      self.parameterSelector.setCurrentNode(self.parameterSetNode)

  def enter(self):
    """Runs whenever the module is reopened
    """
    self.turnOffLightboxes()
    self.installShortcutKeys()

    # Set parameter set node if absent
    self.selectParameterNode()
    self.editor.updateWidgetFromMRML()

  def exit(self):
    self.removeShortcutKeys()
    self.editor.removeSliceObservations()

  def onSceneStartClose(self, caller, event):
    self.parameterSetNode = None
    self.editor.setSegmentationNode(None)
    self.editor.removeSliceObservations()

  def onSceneEndClose(self, caller, event):
    self.enter()
  
  def onSceneEndImport(self, caller, event):
    self.selectParameterNode()
    self.enter()
    
  def cleanup(self):
    self.removeObservers()

  def installShortcutKeys(self):
    """Turn on editor-wide shortcuts.  These are active independent
    of the currently selected effect."""
    #TODO: Deal with commented out shortcuts
    Key_Escape = 0x01000000 # not in PythonQt
    Key_Space = 0x20 # not in PythonQt
    self.shortcuts = []
    keysAndCallbacks = (
        # ('e', EditUtil.toggleLabel),
        # ('z', self.toolsBox.undoRedo.undo),
        # ('y', self.toolsBox.undoRedo.redo),
        ('h', self.toggleCrosshair),
        # ('o', EditUtil.toggleLabelOutline),
        # ('t', EditUtil.toggleForegroundBackground),
        (Key_Escape, lambda : self.editor.setActiveEffect(None)),
        ('p', lambda : self.editor.setActiveEffect(self.editor.effectByName('Paint'))),
        ('d', lambda : self.editor.setActiveEffect(self.editor.effectByName('Draw'))),
        ('w', lambda : self.editor.setActiveEffect(self.editor.effectByName('Wand'))),
        ('r', lambda : self.editor.setActiveEffect(self.editor.effectByName('Rectangle'))),
        # ('c', self.toolsColor.showColorBox),
        # (Key_Space, self.toolsBox.toggleFloatingMode),
        )
    for key,callback in keysAndCallbacks:
      shortcut = qt.QShortcut(slicer.util.mainWindow())
      shortcut.setKey( qt.QKeySequence(key) )
      shortcut.connect( 'activated()', callback )
      self.shortcuts.append(shortcut)

  def removeShortcutKeys(self):
    for shortcut in self.shortcuts:
      shortcut.disconnect('activated()')
      shortcut.setParent(None)
    self.shortcuts = []

  def turnOffLightboxes(self):
    """Since the editor effects can't be used in lightbox mode,
    be sure to turn these off and warn the user about it"""
    warned = False
    layoutManager = slicer.app.layoutManager()
    if layoutManager is not None:
      sliceLogics = layoutManager.mrmlSliceLogics()
      for i in xrange(sliceLogics.GetNumberOfItems()):
        sliceLogic = sliceLogics.GetItemAsObject(i)
        if sliceLogic:
          sliceNode = sliceLogic.GetSliceNode()
          if sliceNode.GetLayoutGridRows() != 1 or sliceNode.GetLayoutGridColumns() != 1:
            if not warned:
              slicer.util.warningDisplay('The Segment Editor module is not compatible with slice viewers in light box mode.\n'
                                         'Views are being reset.', windowTitle='Segment Editor')
              warned = True
            sliceNode.SetLayoutGrid(1,1)

  def toggleCrosshair():
    """Turn on or off the crosshair and enable navigation mode
    by manipulating the scene's singleton crosshair node.
    """
    crosshairNode = slicer.util.getNode('vtkMRMLCrosshairNode*')
    if crosshairNode:
      if crosshairNode.GetCrosshairMode() == 0:
        crosshairNode.SetCrosshairMode(1)
      else:
        crosshairNode.SetCrosshairMode(0)

class SegmentEditorTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Currently no testing functionality.
    """
    self.setUp()
    self.test_SegmentEditor1()

  def test_SegmentEditor1(self):
    """Add test here later.
    """
    self.delayDisplay("Starting the test")
    self.delayDisplay('Test passed!')
