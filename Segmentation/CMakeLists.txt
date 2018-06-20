
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

find_package(ITK REQUIRED)
find_package(VTK REQUIRED)
include(${ITK_USE_FILE})
include(${VTK_USE_FILE})


set( Srcs 
treeitem.cpp
treemodel.cpp
qVusionRoiModule.cpp

FastGrowCutSegmenter.hxx  
fibheap.cpp 
FastGrowCutSegWrapperITK.cxx
    )

set( Hdrs 
treeitem.h
treemodel.h
qVusionRoiModule.h

FastGrowCutSegmenter.h
fibheap.h
utilities.h
FastGrowCutSegWrapperITK.h
   )

set( MOC_Hdrs qVusionRoiModule.h)
set( UIs qVusionRoiModule.ui)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ".."
  "../DisplayCore"
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})
add_library(Segmentation STATIC ${Srcs} ${Hdrs} ${UI_Srcs} ${MOC_Hdrs} )
qt5_use_modules(Segmentation Core Gui Widgets Sql UiTools Concurrent Charts) #Xml XmlPatterns
target_link_libraries(Segmentation ${VTK_LIBRARIES} ${ITK_LIBRARIES} DisplayCore)



