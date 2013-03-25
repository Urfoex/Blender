
set(INC
	.
	../Converter
	../Expressions
	../GameLogic
	../Ketsji
	../Network
	../Network/LoopBackNetwork
	../Physics/Bullet
	../Physics/common
	../Rasterizer
	../Rasterizer/RAS_OpenGLRasterizer
	../SceneGraph
	../../blender
	../../blender/blenfont
	../../blender/blenkernel
	../../blender/blenlib
	../../blender/blenloader
	../../blender/editors/include
	../../blender/gpu
	../../blender/imbuf
	../../blender/makesdna
	../../blender/makesrna
	../../blender/windowmanager
	../../../intern/container
	../../../intern/guardedalloc
	../../../intern/moto/include
	../../../intern/string
)

set(INC_SYS
	${PTHREADS_INCLUDE_DIRS}
	${}
	${BOOST_INCLUDE_DIR}
)

set(SRC
	 qgamewindow.cpp
	BL_KetsjiEmbedStart.cpp
	BL_System.cpp
	KX_BlenderCanvas.cpp
	KX_BlenderGL.cpp
	KX_BlenderInputDevice.cpp
	KX_BlenderKeyboardDevice.cpp
	KX_BlenderMouseDevice.cpp
	KX_BlenderRenderTools.cpp
	KX_BlenderSystem.cpp
	 

	 qgamewindow.h
	BL_System.h
	KX_BlenderCanvas.h
	KX_BlenderGL.h
	KX_BlenderInputDevice.h
	KX_BlenderKeyboardDevice.h
	KX_BlenderMouseDevice.h
	KX_BlenderRenderTools.h
	KX_BlenderSystem.h
	 
)

add_definitions(-DGLEWMX_STATIC)

if(WITH_AUDASPACE)
	list(APPEND INC
		../../../intern/audaspace/intern
	)
	add_definitions(-DWITH_AUDASPACE)
endif()

if(WITH_CODEC_FFMPEG)
	add_definitions(-DWITH_FFMPEG)
endif()

if(WITH_BULLET)
	list(APPEND INC_SYS
		${BULLET_INCLUDE_DIRS}
	)
	add_definitions(-DWITH_BULLET)
endif()


set(CMAKE_AUTOMOC ON)

# find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Concurrent)

# set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")
# set_target_properties(Qt5::Gui PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

# include_directories(${Qt5Gui_INCLUDE_DIRS})
include_directories(${Qt5Concurrent_INCLUDE_DIRS})
# add_definitions(${Qt5Gui_DEFINITIONS})
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5ui_EXECUTABLE_COMPILE_FLAGS}")


#//blender_include_dirs(${Qt5Gui_INCLUDE_DIRS})
blender_add_lib(ge_blen_routines "${SRC}" "${INC}" "${INC_SYS}")


# target_link_libraries(ge_blen_routines ${Qt5Gui_LIBRARIES})
qt5_use_modules(ge_blen_routines Gui Concurrent)