# -----------------------------------------------------------------------------
# Copyright 2023 filament_glfw_imgui Library Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# -----------------------------------------------------------------------------

CC=clang++
OPTS="-std=c++20 -fno-exceptions -O3 -g"
CPUNAME=x86_64
OUT=build   # Folder in which to place outputs.

# Filament renderer.
FILAMENT_PATH=3p/precompiled/${OSTYPE}_${CPUNAME}/filament
FILAMENT_INCLUDES="-I$FILAMENT_PATH/include/"
FILAMENT_LIBS="-L$FILAMENT_PATH/lib/$CPUNAME \
  -lfilament \
  -lbackend \
  -lutils \
  -lvkshaders \
  -lsmol-v \
  -lbluevk \
  -lbluegl \
  -lfilaflat \
  -lfilabridge \
  -libl \
  -lfilament-iblprefilter \
  -lcamutils \
  -lktxreader \
  -limage"
FILAMENT_BIN="./$FILAMENT_PATH/bin"

# GLFW for window and input management.
GLFW_PATH=3p/precompiled/${OSTYPE}_${CPUNAME}/glfw3
GLFW_INCLUDES="-I$GLFW_PATH/include/"
GLFW_LIBS="-L$GLFW_PATH/lib/ -lglfw3"

#-------------------------------------------------------------------------------
# Attempts to build project dependencies.
#-------------------------------------------------------------------------------
if [[ "$1" = "deps" ]]; then
  # Set these to the locations of your filament and glfw source repos.
  FILAMENT_SRC=../filament
  GLFW_SRC=../glfw

  if [[ "$2" = "release" ]]; then
    CONFIG=$2
  elif [[ "$2" = "debug" ]]; then
    CONFIG=$2
  elif [[ "$2" = "clean" ]]; then
    set -x
    rm -rf $FILAMENT_SRC/out
    rm -rf $GLFW_SRC/build
    exit 0
  elif [[ "$2" = "" ]]; then
    CONFIG="release"
  else
    echo "unknown config $2; expected {release, debug}"
    exit 1
  fi
  BUILD_FILAMENT="(cd $FILAMENT_SRC && \
    CC=clang CXX=clang++ CXXFLAGS=-stdlib=libc++ ./build.sh $CONFIG) && \
    cmake --install $FILAMENT_SRC/out/cmake-$CONFIG/ --prefix $FILAMENT_PATH"

  BUILD_GLFW="(cd $GLFW_SRC && mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=$CONFIG -DBUILD_SHARED_LIBS=OFF \
      -DBUILD_GLFW_SAMPLES=OFF -DBUILD_GLFW_TESTS=OFF && \
    cmake --build . --config $CONFIG --parallel) && \
    cmake --install $GLFW_SRC/build --prefix $GLFW_PATH --config $CONFIG"

  (eval $BUILD_FILAMENT && eval $BUILD_GLFW)

#-------------------------------------------------------------------------------
# Builds resources (e.g. materials)
#-------------------------------------------------------------------------------
elif [[ "$1" = "resources" ]]; then
  # The -a option may be overkill for your platform.
  $FILAMENT_BIN/matc -p all -a all -o \
    $OUT/vertex_color.filamat demo/materials/vertex_color.mat && \
  $FILAMENT_BIN/matc -p all -a all -o \
    $OUT/lit_vertex_color.filamat demo/materials/lit_vertex_color.mat && \
  $FILAMENT_BIN/matc -p all -a all -o \
    $OUT/filament_imgui.filamat filament_glfw_imgui/filament_imgui.mat && \
  $FILAMENT_BIN/resgen --deploy=demo/ --json \
    $OUT/vertex_color.filamat \
    $OUT/lit_vertex_color.filamat \
    $OUT/filament_imgui.filamat \
    demo/fonts/Roboto_Regular.ttf \
    demo/fonts/Inconsolata_Regular.ttf
  # Copy environments to the output folder. These aren't compiled as resources
  # but are loaded at runtime.
  cp -r demo/environments $OUT/

#-------------------------------------------------------------------------------
# Builds the demo.
#-------------------------------------------------------------------------------
elif [[ "$1" = "demo" ]]; then
  # Detect OS and set platform-specific variables.
  if [[ "$OSTYPE" =~ ^darwin ]]; then
    # NOTE(ambrus): the macos version thing shuts the linker up about version mismatch.
    # Feels like a hack...
    FILAMENT_INCLUDES="-mmacosx-version-min=11.7 $FILAMENT_INCLUDES"
    FILAMENT_LIBS="$FILAMENT_LIBS -lobjc -liconv \
      -framework GameController -framework CoreHaptics -framework Cocoa \
      -framework QuartzCore -framework Metal -framework CoreFoundation \
      -framework CoreVideo -framework CoreAudio -framework IOKit \
      -framework AudioToolbox -framework ForceFeedback -framework Carbon"
    FILAMENT_NATIVE_SRCS="filament_native/filament_native_glfw_metal.mm"
    RESOURCES="demo/resources.apple.S"

  elif [[ "$OSTYPE" =~ ^linux ]]; then
    CC="$CC -stdlib=libc++"
    FILAMENT_LIBS="$FILAMENT_LIBS -lvulkan"
    GLFW_LIBS="$GLFW_LIBS -lX11 -lXxf86vm -lXrandr -lXi"
    FILAMENT_NATIVE_SRCS="filament_native/filament_native_glfw_x11.cpp"
    RESOURCES="demo/resources.S"

  else
    echo "unknown platform"
    exit 1
  fi

  # App inputs.
  SRCS="\
    3p/stb/stb_image.cpp \
    3p/imgui/imgui.cpp \
    3p/imgui/imgui_demo.cpp \
    3p/imgui/imgui_draw.cpp \
    3p/imgui/imgui_tables.cpp \
    3p/imgui/imgui_widgets.cpp \
    3p/imgui/backends/imgui_impl_glfw.cpp \
    $FILAMENT_NATIVE_SRCS \
    $RESOURCES \
    demo/demo.cpp"
  INCLUDES="-I. -I3p/ -Idemo/ $FILAMENT_INCLUDES $GLFW_INCLUDES"
  LIBS="$FILAMENT_LIBS $GLFW_LIBS"

  # Build the app.
  $CC $OPTS $INCLUDES $SRCS -o build/demo $LIBS

else
  echo "unknown command: $1"
  echo ""
  echo "Usage: build.sh [command] [options]"
  echo ""
  echo "  build.sh deps {debug,release,clean}"
  echo "    Compiles Filament and GLFW static libraries, placing results in"
  echo "    3p/precompiled/. Default is 'release'."
  echo ""
  echo "  build.sh resources"
  echo "    Bundles demo app resources with matc and resgen, placing results in"
  echo "    demo/, and copies environments to build/."
  echo ""
  echo "  build.sh demo"
  echo "    Builds the demo app"
  echo ""
  exit 1
fi
