cmake_minimum_required(VERSION 3.21)

project(ZXingOpenCVProb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui) # ищем  OpenCV
find_package(ZXing CONFIG REQUIRED) # ищем ZXing 

add_executable(frame frame.cpp)     # файл frame.exe - каким может быть окно интерфейса

target_link_libraries(frame PRIVATE # подключаем библиотеки к frame
    ${OpenCV_LIBS}
    ZXing::ZXing
)

target_include_directories(frame PRIVATE ${OpenCV_INCLUDE_DIRS})