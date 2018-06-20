
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
find_package(CTK REQUIRED)
include(${CTK_USE_FILE})

find_package(boost 1.62.0 COMPONENTS math)


set( Srcs 
    itkAdcMapFilter.hxx 
    itkComputedAdcFilter.hxx
    itkComputedEadcFilter.hxx
    itkMaskVectorImageFilter.hxx    
    DiffusionCore.cpp

	#itkDwiIVIMFilterMultiThread.hxx
    #itkDiffTensorFilter.hxx
	#itkComputedDwiFilter.hxx 
    )

set( Hdrs 
     itkAdcMapFilter.h 
     itkComputedAdcFilter.h
     itkComputedEadcFilter.h
     itkMaskVectorImageFilter.h

     #itkDisplayOptimizer.h     
	 #itkComputedDwiFilter.h    
	 #itkDwiIVIMFilterMultiThread.h
     #itkDiffTensorFilter.h
     #itkTensorCDwiImageFilter.h
     #itkDwiIVIMCostFunction4.h
	 #NODDI headers 
	 #itkNODDIFitVoxelTensor.h
	 #itkNODDIProtocolModel.h
	 #itkNODDISingleValuedCostFunction.h
	 #itkLegendre.h
	 #erfiFaddeeva.h
	 #itkMaskVectorImageFilter.h
   )

set( MOC_Hdrs DiffusionCore.h)
set( UIs DiffusionCore.ui)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  "../Utility"
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)


find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
qt5_wrap_ui(UI_Srcs ${UIs})
add_library(DiffusionCore STATIC ${Srcs} ${Hdrs} ${UI_Srcs} ${MOC_Hdrs} )
qt5_use_modules(DiffusionCore Core Gui Widgets Sql UiTools Concurrent)
target_link_libraries(DiffusionCore ${ITK_LIBRARIES} ${VTK_LIBRARIES} ${Glue} Utility CTKWidgets)
# if(VTK_LIBRARIES)
  # target_link_libraries(DiffusionCore ${VTK_LIBRARIES})
# else()
  # target_link_libraries(DiffusionCore vtkHybrid vtkWidgets)
# endif()


