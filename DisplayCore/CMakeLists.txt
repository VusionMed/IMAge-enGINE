find_package(VTK REQUIRED)
find_package(CTK REQUIRED)
include(${VTK_USE_FILE})
include(${CTK_USE_FILE})


# Set your files and resources here
set( Srcs 
	displayport.cpp
    #vtkUDInteractorStyleImage.cxx
	#vtkVusionSliceView.cxx
	vtkVusionMultiView.cxx
	vtkMultiViewInteractorStyleImage.cxx
	vtkMultiViewInteractorStyleImagePaint.cxx
	#vtkVusionContourLineInterpolator.cxx
	vtkVusionImageMapToWindowLevelColors.cxx
	vuWindowWidget.cpp
    )
set( Hdrs 
    displayport.h
	#vtkUDInteractorStyleImage.h
	#vtkVusionSliceView.h
	vtkVusionMultiView.h
	vtkMultiViewInteractorStyleImage.h
	vtkMultiViewInteractorStyleImagePaint.h
	vtkVusionPainter.h
	vuWindowWidget.h
	vtkVusionImageMapToWindowLevelColors.h
    )

set( MOC_Hdrs vuWindowWidget.h)
set( UIs  vuWindowWidget.ui)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_library(DisplayCore STATIC ${Srcs} ${Hdrs} ${UI_Srcs} ${MOC_Hdrs})
qt5_use_modules(DisplayCore Core Gui Widgets opengl UiTools Concurrent)
target_link_libraries(DisplayCore ${QT_LIBRARIES} ${VTK_LIBRARIES} CTKWidgets)
