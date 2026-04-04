cmake_minimum_required(VERSION 3.28.3)
project(TestOpenCVZbar)

set(CMAKE_CXX_STANDARD 11)

# Находим OpenCV
find_package(OpenCV REQUIRED)

# Находим Zbar
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZBAR REQUIRED zbar)

# Создаём программу
add_executable(test test.cpp)

# Подключаем библиотеки
target_include_directories(test PRIVATE ${OpenCV_INCLUDE_DIRS} ${ZBAR_INCLUDE_DIRS})
target_link_libraries(test ${OpenCV_LIBS} ${ZBAR_LIBRARIES})
