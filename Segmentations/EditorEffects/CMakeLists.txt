project(qSlicer${MODULE_NAME}EditorEffects)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "Q_SLICER_${MODULE_NAME_UPPER}_EFFECTS_EXPORT")

set(${KIT}_INCLUDE_DIRECTORIES
  ${vtkSlicerSegmentationsModuleMRML_INCLUDE_DIRS}
  ${vtkSlicerSegmentationsModuleLogic_INCLUDE_DIRS}
  ${MRMLCore_INCLUDE_DIRS}
  ${qMRMLWidgets_INCLUDE_DIRS}
  ${MRMLLogic_INCLUDE_DIRS}
  ${Slicer_Base_INCLUDE_DIRS}
  ${vtkSlicerEditorLibModuleLogic_INCLUDE_DIRS} # For the slice painter
  )

set(${KIT}_SRCS
  qSlicerSegmentEditorEffectFactory.cxx
  qSlicerSegmentEditorEffectFactory.h
  qSlicerSegmentEditorAbstractEffect.cxx
  qSlicerSegmentEditorAbstractEffect.h
  qSlicerSegmentEditorAbstractEffect_p.h
  qSlicerSegmentEditorLabelEffect.cxx
  qSlicerSegmentEditorLabelEffect.h
  qSlicerSegmentEditorLabelEffect_p.h
  qSlicerSegmentEditorPaintEffect.cxx
  qSlicerSegmentEditorPaintEffect.h
  qSlicerSegmentEditorPaintEffect_p.h
  qSlicerSegmentEditorRectangleEffect.cxx
  qSlicerSegmentEditorRectangleEffect.h
  qSlicerSegmentEditorScriptedEffect.cxx
  qSlicerSegmentEditorScriptedEffect.h
  )

set(${KIT}_MOC_SRCS
  qSlicerSegmentEditorEffectFactory.h
  qSlicerSegmentEditorAbstractEffect.h
  qSlicerSegmentEditorAbstractEffect_p.h
  qSlicerSegmentEditorLabelEffect.h
  qSlicerSegmentEditorLabelEffect_p.h
  qSlicerSegmentEditorPaintEffect.h
  qSlicerSegmentEditorPaintEffect_p.h
  qSlicerSegmentEditorRectangleEffect.h
  qSlicerSegmentEditorScriptedEffect.h
  )

set(${KIT}_UI_SRCS
  )

set(${KIT}_RESOURCES
  Resources/${KIT}.qrc
  )

SET (${KIT}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
set(${KIT}_TARGET_LIBRARIES
  vtkSlicerSegmentationsModuleMRML
  vtkSlicerSegmentationsModuleLogic
  MRMLCore
  MRMLLogic
  qMRMLWidgets
  vtkSlicerEditorLibModuleLogic
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleQtLibrary(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  MOC_SRCS ${${KIT}_MOC_SRCS}
  UI_SRCS ${${KIT}_UI_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  RESOURCES ${${KIT}_RESOURCES}
  WRAP_PYTHONQT
  )

#-----------------------------------------------------------------------------
if(Slicer_USE_PYTHONQT)
  add_subdirectory(Python)
endif()
