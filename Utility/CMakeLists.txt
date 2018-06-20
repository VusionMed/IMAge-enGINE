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

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
# find_package(CTK REQUIRED)
# include(${CTK_USE_FILE})
# find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)


# Set your files and resources here
set( Srcs DicomHelper.cpp
    bvalueDialog.cpp
	vtkDICOMVUSIONGenerator.cxx 
)

set( Hdrs DicomHelper.h 
      DKIHelper.h
	  bvalueDialog.h
	  vtkDICOMVUSIONGenerator.h
	  SharedFunctions.h
)

set( MOC_Hdrs DicomHelper.h)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(CMAKE_AUTOMOC ON)

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_library(Utility STATIC ${Srcs} ${Hdrs} ${MOC_Hdrs})
qt5_use_modules(Utility Core Gui Widgets Concurrent)
target_link_libraries(Utility ${ITK_LIBRARIES} ${VTK_LIBRARIES} ${Glue})
