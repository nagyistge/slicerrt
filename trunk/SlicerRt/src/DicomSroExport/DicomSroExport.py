import os
import unittest
from __main__ import vtk, qt, ctk, slicer

#
# DicomSroExport
#

class DicomSroExport:
  def __init__(self, parent):
    parent.title = "DICOM Registration Export"
    parent.categories = ["Plastimatch"]
    parent.dependencies = []
    parent.contributors = ["Gregory Sharp (MGH)"]
    parent.helpText = """
    This is an example of scripted loadable module bundled in an extension.
    """
    parent.acknowledgementText = """
    This file was originally developed by Greg Sharp, Massachusetts General Hospital, and was partially funded by NIH grant 2-U54-EB005149.
    """ # replace with organization, grant and thanks.
    self.parent = parent

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['DicomSroExport'] = self.runTest

  def runTest(self):
    tester = DicomSroExportTest()
    tester.runTest()

#
# qDicomSroExportWidget
#

class DicomSroExportWidget:
  def __init__(self, parent = None):
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.layout = self.parent.layout()
    if not parent:
      self.setup()
      self.parent.show()

  def setup(self):
    # Instantiate and connect widgets ...

    ### Reload and Test area
    reloadCollapsibleButton = ctk.ctkCollapsibleButton()
    reloadCollapsibleButton.text = "Reload && Test"
    self.layout.addWidget(reloadCollapsibleButton)
    reloadFormLayout = qt.QFormLayout(reloadCollapsibleButton)

    # reload button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadButton = qt.QPushButton("Reload")
    self.reloadButton.toolTip = "Reload this module."
    self.reloadButton.name = "DicomSroExport Reload"
    reloadFormLayout.addWidget(self.reloadButton)
    self.reloadButton.connect('clicked()', self.onReload)

    # reload and test button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadAndTestButton = qt.QPushButton("Reload and Test")
    self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
    reloadFormLayout.addWidget(self.reloadAndTestButton)
    self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)

    ### Parameters Area
    parametersCollapsibleButton = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton.text = "Parameters"
    self.layout.addWidget(parametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton)

    # fixed image (mrml input)
    self.fixedMRMLSelector = slicer.qMRMLNodeComboBox()
    self.fixedMRMLSelector.nodeTypes = ( ("vtkMRMLScalarVolumeNode"), "" )
    self.fixedMRMLSelector.addAttribute (
      "vtkMRMLScalarVolumeNode", "LabelMap", 0 )
    self.fixedMRMLSelector.selectNodeUponCreation = True
    self.fixedMRMLSelector.addEnabled = False
    self.fixedMRMLSelector.removeEnabled = False
    self.fixedMRMLSelector.noneEnabled = False
    self.fixedMRMLSelector.showHidden = False
    self.fixedMRMLSelector.showChildNodeTypes = False
    self.fixedMRMLSelector.setMRMLScene( slicer.mrmlScene )
    self.fixedMRMLSelector.setToolTip( "Pick the input to the algorithm." )
    parametersFormLayout.addRow("Fixed image: ", self.fixedMRMLSelector)

    # fixed image (directory input)
    self.fixedInputDirectory = ctk.ctkDirectoryButton()
    self.fixedInputDirectory.directory = qt.QDir.homePath()
    parametersFormLayout.addRow("", self.fixedInputDirectory)

    # moving image (mrml input)
    self.movingMRMLSelector = slicer.qMRMLNodeComboBox()
    self.movingMRMLSelector.nodeTypes = ( ("vtkMRMLScalarVolumeNode"), "" )
    self.movingMRMLSelector.addAttribute(
      "vtkMRMLScalarVolumeNode", "LabelMap", 0 )
    self.movingMRMLSelector.selectNodeUponCreation = True
    self.movingMRMLSelector.addEnabled = False
    self.movingMRMLSelector.removeEnabled = False
    self.movingMRMLSelector.noneEnabled = False
    self.movingMRMLSelector.showHidden = False
    self.movingMRMLSelector.showChildNodeTypes = False
    self.movingMRMLSelector.setMRMLScene( slicer.mrmlScene )
    self.movingMRMLSelector.setToolTip( "Pick the input to the algorithm." )
    parametersFormLayout.addRow("Fixed image: ", self.movingMRMLSelector)

    # moving image (directory input)
    self.movingInputDirectory = ctk.ctkDirectoryButton()
    self.movingInputDirectory.directory = qt.QDir.homePath()
    parametersFormLayout.addRow("", self.movingInputDirectory)

    # transform (mrml input)
    self.xformMRMLSelector = slicer.qMRMLNodeComboBox()
    self.xformMRMLSelector.nodeTypes = ( ("vtkMRMLLinearTransformNode"), "" )
    self.xformMRMLSelector.selectNodeUponCreation = True
    self.xformMRMLSelector.addEnabled = False
    self.xformMRMLSelector.removeEnabled = False
    self.xformMRMLSelector.noneEnabled = False
    self.xformMRMLSelector.showHidden = False
    self.xformMRMLSelector.showChildNodeTypes = False
    self.xformMRMLSelector.setMRMLScene( slicer.mrmlScene )
    self.xformMRMLSelector.setToolTip( "Pick the input to the algorithm." )
    parametersFormLayout.addRow("Transform: ", self.xformMRMLSelector)

    # output directory selector
    self.outputDirectory = ctk.ctkDirectoryButton()
    self.outputDirectory.directory = qt.QDir.homePath()
    parametersFormLayout.addRow("Output Directory: ", self.outputDirectory)
    
    # check box to trigger taking screen shots for later use in tutorials
    self.enableScreenshotsFlagCheckBox = qt.QCheckBox()
    self.enableScreenshotsFlagCheckBox.checked = 0
    self.enableScreenshotsFlagCheckBox.setToolTip("If checked, take screen shots for tutorials. Use Save Data to write them to disk.")
    #    parametersFormLayout.addRow("Enable Screenshots", self.enableScreenshotsFlagCheckBox)

    # scale factor for screen shots
    self.screenshotScaleFactorSliderWidget = ctk.ctkSliderWidget()
    self.screenshotScaleFactorSliderWidget.singleStep = 1.0
    self.screenshotScaleFactorSliderWidget.minimum = 1.0
    self.screenshotScaleFactorSliderWidget.maximum = 50.0
    self.screenshotScaleFactorSliderWidget.value = 1.0
    self.screenshotScaleFactorSliderWidget.setToolTip("Set scale factor for the screen shots.")
    #    parametersFormLayout.addRow("Screenshot scale factor", self.screenshotScaleFactorSliderWidget)

    # Apply Button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run the algorithm."
    self.applyButton.enabled = False
    parametersFormLayout.addRow(self.applyButton)

    # connections
    self.applyButton.connect('clicked(bool)', self.onApplyButton)
    self.fixedMRMLSelector.connect(
      "currentNodeChanged(vtkMRMLNode*)", self.onSelect)

    # Add vertical spacer
    self.layout.addStretch(1)

  def cleanup(self):
    pass

  def onSelect(self):
    if (self.fixedMRMLSelector.currentNode()
        and self.movingMRMLSelector.currentNode()
        and self.xformMRMLSelector.currentNode()):
      self.applyButton.enabled = True
    else:
      self.applyButton.enabled = False

  def onApplyButton(self):
    logic = DicomSroExportLogic()
    enableScreenshotsFlag = self.enableScreenshotsFlagCheckBox.checked
    screenshotScaleFactor = int(self.screenshotScaleFactorSliderWidget.value)
    print("Run the algorithm")
    logic.run(
      self.fixedMRMLSelector.currentNode(),
      self.movingMRMLSelector.currentNode(),
      self.xformMRMLSelector.currentNode(),
      self.outputDirectory.text)

  def onReload(self,moduleName="DicomSroExport"):
    """Generic reload method for any scripted module.
    ModuleWizard will subsitute correct default moduleName.
    """
    import imp, sys, os, slicer

    widgetName = moduleName + "Widget"

    # reload the source code
    # - set source file path
    # - load the module to the global space
    filePath = eval('slicer.modules.%s.path' % moduleName.lower())
    p = os.path.dirname(filePath)
    if not sys.path.__contains__(p):
      sys.path.insert(0,p)
    fp = open(filePath, "r")
    globals()[moduleName] = imp.load_module(
        moduleName, fp, filePath, ('.py', 'r', imp.PY_SOURCE))
    fp.close()

    # rebuild the widget
    # - find and hide the existing widget
    # - create a new widget in the existing parent
    parent = slicer.util.findChildren(name='%s Reload' % moduleName)[0].parent().parent()
    for child in parent.children():
      try:
        child.hide()
      except AttributeError:
        pass
    # Remove spacer items
    item = parent.layout().itemAt(0)
    while item:
      parent.layout().removeItem(item)
      item = parent.layout().itemAt(0)

    # delete the old widget instance
    if hasattr(globals()['slicer'].modules, widgetName):
      getattr(globals()['slicer'].modules, widgetName).cleanup()

    # create new widget inside existing parent
    globals()[widgetName.lower()] = eval(
        'globals()["%s"].%s(parent)' % (moduleName, widgetName))
    globals()[widgetName.lower()].setup()
    setattr(globals()['slicer'].modules, widgetName, globals()[widgetName.lower()])

  def onReloadAndTest(self,moduleName="DicomSroExport"):
    try:
      self.onReload()
      evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
      tester = eval(evalString)
      tester.runTest()
    except Exception, e:
      import traceback
      traceback.print_exc()
      qt.QMessageBox.warning(slicer.util.mainWindow(), 
          "Reload and Test", 'Exception!\n\n' + str(e) + "\n\nSee Python Console for Stack Trace")


#
# DicomSroExportLogic
#

class DicomSroExportLogic:
  """This class should implement all the actual 
  computation done by your module.  The interface 
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """
  def __init__(self):
    pass

  def hasImageData(self,volumeNode):
    """This is a dummy logic method that 
    returns true if the passed in volume
    node has valid image data
    """
    if not volumeNode:
      print('no volume node')
      return False
    if volumeNode.GetImageData() == None:
      print('no image data')
      return False
    return True

  def delayDisplay(self,message,msec=1000):
    #
    # logic version of delay display
    #
    print(message)
    self.info = qt.QDialog()
    self.infoLayout = qt.QVBoxLayout()
    self.info.setLayout(self.infoLayout)
    self.label = qt.QLabel(message,self.info)
    self.infoLayout.addWidget(self.label)
    qt.QTimer.singleShot(msec, self.info.close)
    self.info.exec_()

  def takeScreenshot(self,name,description,type=-1):
    # show the message even if not taking a screen shot
    self.delayDisplay(description)

    if self.enableScreenshots == 0:
      return

    lm = slicer.app.layoutManager()
    # switch on the type to get the requested window
    widget = 0
    if type == -1:
      # full window
      widget = slicer.util.mainWindow()
    elif type == slicer.qMRMLScreenShotDialog().FullLayout:
      # full layout
      widget = lm.viewport()
    elif type == slicer.qMRMLScreenShotDialog().ThreeD:
      # just the 3D window
      widget = lm.threeDWidget(0).threeDView()
    elif type == slicer.qMRMLScreenShotDialog().Red:
      # red slice window
      widget = lm.sliceWidget("Red")
    elif type == slicer.qMRMLScreenShotDialog().Yellow:
      # yellow slice window
      widget = lm.sliceWidget("Yellow")
    elif type == slicer.qMRMLScreenShotDialog().Green:
      # green slice window
      widget = lm.sliceWidget("Green")

    # grab and convert to vtk image data
    qpixMap = qt.QPixmap().grabWidget(widget)
    qimage = qpixMap.toImage()
    imageData = vtk.vtkImageData()
    slicer.qMRMLUtils().qImageToVtkImageData(qimage,imageData)

    annotationLogic = slicer.modules.annotations.logic()
    annotationLogic.CreateSnapShot(name, description, type, self.screenshotScaleFactor, imageData)

  def run (self,fixedNode,movingNode,xformNode,outputDir):
    """
    Run the actual algorithm
    """
    import os, sys, vtk
    print ("Hello from the Apply button")
    import vtkSlicerPlastimatchPyModuleLogicPython
    loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
    if loadablePath not in sys.path:
      sys.path.append(loadablePath)
    reg = vtkSlicerPlastimatchPyModuleLogicPython.vtkSlicerPlastimatchPyModuleLogic()
    reg.SetMRMLScene(slicer.mrmlScene)

    print ("Did the import work?")

    # Set input/output images
    #reg.SetFixedImageID(self.volumeSelectors[fixedDataName].currentNode().GetID())
    #reg.SetMovingImageID(self.volumeSelectors[movingDataName].currentNode().GetID())
    #reg.SetOutputVolumeID(self.volumeSelectors['Transformed'].currentNode().GetID())

    print ("Did that work???")

    return True


class DicomSroExportTest(unittest.TestCase):
  """
  This is the test case for your scripted module.
  """

  def delayDisplay(self,message,msec=1000):
    """This utility method displays a small dialog and waits.
    This does two things: 1) it lets the event loop catch up
    to the state of the test so that rendering and widget updates
    have all taken place before the test continues and 2) it
    shows the user/developer/tester the state of the test
    so that we'll know when it breaks.
    """
    print(message)
    self.info = qt.QDialog()
    self.infoLayout = qt.QVBoxLayout()
    self.info.setLayout(self.infoLayout)
    self.label = qt.QLabel(message,self.info)
    self.infoLayout.addWidget(self.label)
    qt.QTimer.singleShot(msec, self.info.close)
    self.info.exec_()

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_DicomSroExport1()

  def test_DicomSroExport1(self):
    """ Ideally you should have several levels of tests.  At the lowest level
    tests sould exercise the functionality of the logic with different inputs
    (both valid and invalid).  At higher levels your tests should emulate the
    way the user would interact with your code and confirm that it still works
    the way you intended.
    One of the most important features of the tests is that it should alert other
    developers when their changes will have an impact on the behavior of your
    module.  For example, if a developer removes a feature that you depend on,
    your test should break so they know that the feature is needed.
    """

    self.delayDisplay("Starting the test")
    #
    # first, get some data
    #
    import urllib
    downloads = (
        ('http://slicer.kitware.com/midas3/download?items=5767', 'FA.nrrd', slicer.util.loadVolume),
        )

    for url,name,loader in downloads:
      filePath = slicer.app.temporaryPath + '/' + name
      if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
        print('Requesting download %s from %s...\n' % (name, url))
        urllib.urlretrieve(url, filePath)
      if loader:
        print('Loading %s...\n' % (name,))
        loader(filePath)
    self.delayDisplay('Finished with download and loading\n')

    volumeNode = slicer.util.getNode(pattern="FA")
    logic = DicomSroExportLogic()
    self.assertTrue( logic.hasImageData(volumeNode) )
    self.delayDisplay('Test passed!')
