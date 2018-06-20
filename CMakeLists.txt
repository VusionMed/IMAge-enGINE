cmake_minimum_required(VERSION 2.8.8)

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

project(MR_Diffusion)
set(CMAKE_CXX_FLAGS_RELEASE "/MD")
set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
add_subdirectory(DicomWidget)
add_subdirectory(DiffusionCore)
add_subdirectory(DisplayCore)
add_subdirectory(Segmentation)
add_subdirectory(Utility)

find_package(VTK REQUIRED)
find_package(CTK REQUIRED)
find_package(ITK REQUIRED)


include(${VTK_USE_FILE})
include(${CTK_USE_FILE})
include(${ITK_USE_FILE})


if("${VTK_QT_VERSION}" STREQUAL "")
  message(FATAL_ERROR "VTK was not built with Qt")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "/MD")
set(CMAKE_CXX_FLAGS_DEBUG "/MDd")

# Set your files and resources here
set( Srcs main.cpp 
     mainwindow.cpp
	 vusionAboutDialog.cxx
	 settingDialog.cpp
    )

set( Hdrs mainwindow.h 
     vusionAboutDialog.h
	 settingDialog.h
	)

set( MOC_Hdrs 
     mainwindow.h 
     vusionAboutDialog.h
	 settingDialog.h
    )

set( UIs mainwindow.ui 
     vusionAboutDialog.ui
	 settingDialog.ui
	)

set( QRCs darkstyle/style.qrc)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  "DicomWidget"
  "DiffusionCore"
  "DisplayCore"
  "Utility"
  "Segmentation"
  "ThirdParty"
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})
qt5_add_resources(QRC_Srcs ${QRCs} )

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_executable(MR_Diffusion ${Srcs} ${Hdrs} ${UI_Srcs} ${MOC_Hdrs} ${QRC_Srcs})
qt5_use_modules(MR_Diffusion Core Gui Widgets Sql UiTools  Concurrent Test) #Xml XmlPatterns

target_link_libraries(MR_Diffusion ${VTK_LIBRARIES} ${QT_LIBRARIES}  DicomWidget DiffusionCore DisplayCore Segmentation Utility CTKWidgets CTKDICOMWidgets)


