#-----------------------------------------------------------------------------
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/SegmentEditorEffects.__init__.py.in
	${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/SegmentEditorEffects/__init__.py
	)

#-----------------------------------------------------------------------------
set(SegmentEditorEffects_PYTHON_SCRIPTS
  AbstractScriptedSegmentEditorEffect
  SegmentEditorThresholdEffect
  )

set(SegmentEditorEffects_PYTHON_RESOURCES
  )

ctkMacroCompilePythonScript(
  TARGET_NAME SegmentEditorEffects
  SCRIPTS "${SegmentEditorEffects_PYTHON_SCRIPTS}"
  RESOURCES "${SegmentEditorEffects_PYTHON_RESOURCES}"
  DESTINATION_DIR ${CMAKE_BINARY_DIR}/${Slicer_QTSCRIPTEDMODULES_LIB_DIR}/SegmentEditorEffects
  INSTALL_DIR ${Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR}/SegmentEditorEffects
  )
