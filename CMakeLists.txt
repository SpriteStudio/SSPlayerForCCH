CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(PlayerConverter)



ADD_LIBRARY(commonlib 
src/common/BinaryDataWriter.cpp
src/common/BinaryDataWriter.h
src/common/Canvas2dSaver.cpp
src/common/Canvas2dSaver.h
src/common/Cocos2dSaver.cpp
src/common/Cocos2dSaver.h
src/common/CoronaSaver.cpp
src/common/CoronaSaver.h
src/common/DebugUtil.cpp
src/common/DebugUtil.h
src/common/FileUtil.cpp
src/common/FileUtil.h
src/common/MathUtil.cpp
src/common/MathUtil.h
src/common/SaverUtil.cpp
src/common/SaverUtil.h
src/common/SsaxLoader.cpp
src/common/SsaxLoader.h
src/common/SsfLoader.cpp
src/common/SsfLoader.h
src/common/SsMotion.cpp
src/common/SsMotion.h
src/common/SsMotionDecoder.cpp
src/common/SsMotionDecoder.h
src/common/SsMotionUtil.cpp
src/common/SsMotionUtil.h
src/common/TextEncoding.cpp
src/common/TextEncoding.h
src/common/XmlUtil.cpp
src/common/XmlUtil.h
)


ADD_EXECUTABLE(SsToCocos2d
src/converter/SsToCocos2d.cpp 
xcode4.6/SsPlayerConverter/SsToCocos2d/SsToCocos2d/main.cpp
src/converter/ConverterShared.cpp
src/converter/ConverterShared.h
)

ADD_EXECUTABLE(SsToCorona
src/converter/SsToCorona.cpp 
xcode4.6/SsPlayerConverter/SsToCorona/SsToCorona/main.cpp
src/converter/ConverterShared.cpp
src/converter/ConverterShared.h
)

ADD_EXECUTABLE(SsToHtml5
src/converter/SsToHtml5.cpp 
xcode4.6/SsPlayerConverter/SsToHtml5/SsToHtml5/main.cpp
src/converter/ConverterShared.cpp
src/converter/ConverterShared.h
)


IF(APPLE)
	SET(SEARCH_DIR "/usr/local/include;/opt/local/include;src/common" )
	SET(SEARCH_LIB "/usr/local/lib;/opt/local/lib" )
	SET(LINK_LIB "commonlib /opt/local/lib/libboost_filesystem-mt.a /opt/local/lib/libboost_program_options-mt.a /opt/local/lib/libboost_system-mt.a /usr/local/lib/libiconv.2.dylib" )
    ADD_DEFINITIONS(-APPLE)
ENDIF()


include_directories(${SEARCH_DIR})
link_directories(${SEARCH_LIB})
TARGET_LINK_LIBRARIES(SsToCocos2d ${LINK_LIB})

include_directories(${SEARCH_DIR})
link_directories(${SEARCH_LIB})
TARGET_LINK_LIBRARIES(SsToCorona ${LINK_LIB})

include_directories(${SEARCH_DIR})
link_directories(${SEARCH_LIB})
TARGET_LINK_LIBRARIES(SsToHtml5 ${LINK_LIB})




