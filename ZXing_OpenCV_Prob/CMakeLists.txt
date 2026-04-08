cmake_minimum_required(VERSION 3.21)

project(ZXingOpenCVProb)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)       # ищем  OpenCV

find_package(ZXing CONFIG REQUIRED) # ищем ZXing 

add_executable(test test.cpp)       # создаём файл test.exe

target_link_libraries(test PRIVATE
    ${OpenCV_LIBS}  # подключаем библиотеки OpenCV
    ZXing::ZXing    # подключаем библиотеку ZXing
)

target_include_directories(test PRIVATE ${OpenCV_INCLUDE_DIRS})