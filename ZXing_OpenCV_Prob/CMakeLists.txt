cmake_minimum_required(VERSION 3.21)

project(ZXingOpenCVProb)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)       # ищем  OpenCV

find_package(ZXing CONFIG REQUIRED) # ищем ZXing 

add_executable(test test.cpp)       # файл test.exe для проверки работы библиотек

add_executable(scan scan.cpp)       # файл scan.exe для работы с камерой

target_link_libraries(test PRIVATE  # подключаем библиотеки к test
    ${OpenCV_LIBS}  
    ZXing::ZXing
)
target_link_libraries(scan PRIVATE  # подключаем библиотеки к scan
    ${OpenCV_LIBS}
    ZXing::ZXing
)

target_include_directories(test PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(scan PRIVATE ${OpenCV_INCLUDE_DIRS})