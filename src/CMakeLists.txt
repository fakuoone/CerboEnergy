
# cmake DCMAKE_BUILD_TYPE:STRING=Debug -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_TOOLCHAIN_FILE="B:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-mingw-static -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_C_COMPILER:FILEPATH=C:\msys64\ucrt64\bin\gcc.exe -DCMAKE_CXX_COMPILER:FILEPATH=C:\msys64\ucrt64\bin\g++.exe --no-warn-unused-cli  -SB:/Programmieren/C/CerboEnergy/src  -BB:/Programmieren/C/CerboEnergy/build -G "MinGW Makefiles"

# cmake DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_TOOLCHAIN_FILE="B:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_C_COMPILER:FILEPATH=C:\msys64\ucrt64\bin\gcc.exe -DCMAKE_CXX_COMPILER:FILEPATH=C:\msys64\ucrt64\bin\g++.exe --no-warn-unused-cli  -SB:/Programmieren/C/CerboEnergy/src  -BB:/Programmieren/C/CerboEnergy/build -G "MinGW Makefiles"

# cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# vcpkg install curl:x64-mingw-static --host-triplet=x64-mingw-static

cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER "C:/msys64/ucrt64/bin/gcc.exe" )
set(CMAKE_CXX_COMPILER "C:/msys64/ucrt64/bin/g++.exe" )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LINKTYPE "STATIC")

# Set flags for static linking
IF(LINKTYPE STREQUAL STATIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -lcrypt32")
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lcrypt32 -lws2_32 -lcrypt32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
    set(BUILD_SHARED_LIBS OFF)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lcrypt32")
    set(CMAKE_CXX_STANDARD_LIBRARIES "-limgui -limplot -lwsock32 -lcrypt32 -lws2_32 -lcrypt32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--whole-archive -lwinpthread -Wl,--no-whole-archive")
    set(BUILD_SHARED_LIBS OFF)
endif()

project(SSHEnergy VERSION 1.0)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Set where the imgui files are stored
set(IMGUI_PATH  "B:/Programmieren/C/Libs/imgui")
set(CMAKE_TOOLCHAIN_FILE "B:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake")

set(SOURCES main.cpp)
set(HEADERS ${CMAKE_SOURCE_DIR}/include/cerboGui.h
            ${CMAKE_SOURCE_DIR}/include/cerboLogger.h 
            ${CMAKE_SOURCE_DIR}/include/cerboPlots.h
            ${CMAKE_SOURCE_DIR}/include/dataHandler.h
            ${CMAKE_SOURCE_DIR}/include/cerboSSH.h
            ${CMAKE_SOURCE_DIR}/include/cerboModbus.h
            ${CMAKE_SOURCE_DIR}/include/helperFunctions.h
            ${CMAKE_SOURCE_DIR}/include/main.h)

add_executable(SSHEnergy WIN32 ${SOURCES} ${HEADERS})
target_include_directories(SSHEnergy PRIVATE ${CMAKE_SOURCE_DIR}/include)

configure_file(mainConfig.h.in ${CMAKE_BINARY_DIR}/../src/mainConfig.h)

# Compile as static library 
file(GLOB IMGUI_GLOB
    ${IMGUI_PATH}/imgui.h
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imconfig.h
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_internal.h
    ${IMGUI_PATH}/imstb_rectpack.h
    ${IMGUI_PATH}/imstb_textedit.h
    ${IMGUI_PATH}/imstb_truetype.h
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp

    # specific bindings...
    ${IMGUI_PATH}/backends/imgui_impl_win32.h
    ${IMGUI_PATH}/backends/imgui_impl_win32.cpp
    ${IMGUI_PATH}/backends/imgui_impl_dx11.cpp
    ${IMGUI_PATH}/backends/imgui_impl_dx11.h)

set(IMPLOT_PATH  "B:/Programmieren/C/Libs/implot")
file(GLOB IMPLOT_GLOB
    ${IMPLOT_PATH}/implot.h
    ${IMPLOT_PATH}/implot.cpp
    ${IMPLOT_PATH}/implot_internal.h
    ${IMPLOT_PATH}/implot_items.cpp)

file(GLOB IMPLOTWRAPPER_GLOB
    ${CMAKE_SOURCE_DIR}/cerboPlots.h
    ${CMAKE_SOURCE_DIR}/cerboPlots.cpp)

add_library(imgui ${LINKTYPE} ${IMGUI_GLOB})
target_compile_definitions(imgui PRIVATE IMGUI_EXPORTS)
target_include_directories(imgui PUBLIC ${IMGUI_PATH})
target_include_directories(imgui PUBLIC ${IMGUI_PATH}/backends)

add_library(implot ${LINKTYPE} ${IMPLOT_GLOB})
target_compile_definitions(implot PRIVATE IMPLOT_EXPORTS)
target_include_directories(implot PUBLIC ${IMPLOT_PATH})
target_include_directories(implot PUBLIC ${IMGUI_PATH})

find_package(libssh CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(libmodbus CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(SSHEnergy PRIVATE OpenSSL::SSL)
target_link_libraries(SSHEnergy PRIVATE OpenSSL::Crypto)
target_link_libraries(SSHEnergy PRIVATE ws2_32) 
target_link_libraries(SSHEnergy PRIVATE crypt32) 
target_link_libraries(SSHEnergy PRIVATE z) 
target_link_libraries(SSHEnergy PRIVATE ssh)

target_link_libraries(SSHEnergy PRIVATE CURL::libcurl)
target_link_libraries(SSHEnergy PRIVATE modbus)
target_link_libraries(SSHEnergy PRIVATE nlohmann_json::nlohmann_json)

target_link_libraries(SSHEnergy PRIVATE imgui)
target_link_libraries(SSHEnergy PRIVATE implot)
target_link_libraries(SSHEnergy PRIVATE d3d11 d3dcompiler)
target_link_libraries(SSHEnergy PRIVATE Dwmapi) 

target_include_directories(SSHEnergy PRIVATE "${PROJECT_BINARY_DIR}" "B:/Programmieren/C/Libs/imgui")

target_include_directories(SSHEnergy PRIVATE "B:/Programmieren/C/Libs/stbimage/include")
target_include_directories(SSHEnergy PRIVATE "B:/Programmieren/C/Libs/modbus/include")
target_include_directories(SSHEnergy PRIVATE "B:/Programmieren/C/Libs/libssh/include")
target_include_directories(SSHEnergy PRIVATE "B:/Programmieren/C/Libs/implot/include")
target_include_directories(SSHEnergy PRIVATE "B:/Programmieren/C/Libs/imgui/include")
target_include_directories(SSHEnergy PRIVATE "B:/Programmieren/C/Libs/curl/include")
