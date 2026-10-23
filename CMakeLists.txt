cmake_minimum_required(VERSION 3.13)
project(noctis-iv-plus CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
find_package(OpenGL REQUIRED)

option(NOCTIS_X87_MATH "Use x87 floating point like the original Borland build (x86 only)" ON)

set(GAME_SOURCES
  src/main.cpp
  src/dos/dos.cpp
  src/platform/platform_sdl.cpp
  src/game/noctis-0.cpp
  src/game/noctis-1.cpp
  src/game/noctis.cpp
  src/goes/goes.cpp
  src/goes/cast.cpp
  src/goes/cat.cpp
  src/goes/clean.cpp
  src/goes/dele.cpp
  src/goes/dl.cpp
  src/goes/inbox.cpp
  src/goes/outbox.cpp
  src/goes/par.cpp
  src/goes/pri.cpp
  src/goes/rep.cpp
  src/goes/sl.cpp
  src/goes/st.cpp
  src/goes/where.cpp
)

add_executable(noctis ${GAME_SOURCES})
target_include_directories(noctis PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_link_libraries(noctis PRIVATE ${SDL2_LIBRARIES} OpenGL::GL m)

# The original was 16-bit Borland C++: char is signed, integer overflow
# wraps around and the code type-puns freely.
target_compile_options(noctis PRIVATE
  -fsigned-char -fwrapv -fno-strict-aliasing
  -Wall -Wno-write-strings -Wno-unused-label -Wno-unused-variable
  -Wno-unused-but-set-variable -Wno-parentheses -Wno-misleading-indentation
  -Wno-char-subscripts -Wno-unused-function)

if(NOCTIS_X87_MATH AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
  target_compile_options(noctis PRIVATE -mfpmath=387)
endif()

include(GNUInstallDirs)
set(NOCTIS_SHARE "${CMAKE_INSTALL_FULL_DATADIR}/noctis-iv-plus")
target_compile_definitions(noctis PRIVATE NOCTIS_DATADIR="${NOCTIS_SHARE}")

install(TARGETS noctis RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY data manual DESTINATION ${NOCTIS_SHARE})
install(FILES WPL.HTM DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES dist/noctis-iv-plus.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications OPTIONAL)
