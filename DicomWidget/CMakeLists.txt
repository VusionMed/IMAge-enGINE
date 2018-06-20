

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

find_package(CTK REQUIRED)
include(${CTK_USE_FILE})

# Set your files and resources here
set( Srcs vuDICOMBrowser.cpp )

set( Hdrs vuDICOMBrowser.h)

set( MOC_Hdrs vuDICOMBrowser.h)

set( UIs vuDICOMBrowser.ui)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)


find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_library(DicomWidget STATIC ${Srcs} ${Hdrs} ${UI_Srcs} ${MOC_Hdrs})
qt5_use_modules(DicomWidget Core Gui Widgets Sql Xml UiTools XmlPatterns Concurrent)
target_link_libraries(DicomWidget ${QT_LIBRARIES} CTKWidgets CTKDICOMWidgets)

install(TARGETS DicomWidget DESTINATION bin)
install(FILES Hdrs DESTINATION include)