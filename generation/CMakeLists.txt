cmake_minimum_required(VERSION 3.14)
project(BarcodeGenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции сборки
option(BUILD_TESTS "Build unit tests" ON)

# Поиск OpenCV
find_package(OpenCV REQUIRED)

# Поиск установленного ZXing
# Пробуем разные варианты имен пакета
find_package(ZXing QUIET)
find_package(nu-book-zxing-cpp QUIET)

if(ZXing_FOUND OR nu-book-zxing-cpp_FOUND)
    message(STATUS "ZXing found via find_package")
    if(ZXing_FOUND)
        set(ZXING_TARGET ZXing::ZXing)
    else()
        set(ZXING_TARGET nu-book-zxing-cpp::ZXing)
    endif()
else()
    message(FATAL_ERROR "ZXing library not found. Please install ZXing first.")
endif()

# Основной исполняемый файл
add_executable(barcode_generator
    main.cpp
    generate.cpp
    generate.hpp
)

target_link_libraries(barcode_generator
    ${OpenCV_LIBS}
    ${ZXING_TARGET}
)

target_include_directories(barcode_generator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

set_property(TARGET barcode_generator PROPERTY CXX_STANDARD 20)


# Юнит-тесты
if(BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    # find_package(GTest REQUIRED)
    
    add_executable(barcode_tests
        generate_test.cpp
        generate.cpp
        generate.hpp
    )
    
    target_link_libraries(barcode_tests
        ${OpenCV_LIBS}
        ${ZXING_TARGET}
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
    )
    
    target_include_directories(barcode_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OpenCV_INCLUDE_DIRS}
    )
    
    set_property(TARGET barcode_tests PROPERTY CXX_STANDARD 20)
    
    # Добавляем тесты в CTest
    include(GoogleTest)
    gtest_discover_tests(barcode_tests)
    
    # Удобные цели для запуска тестов
    add_custom_target(run_tests
        COMMAND barcode_tests
        DEPENDS barcode_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests..."
    )
    
    add_custom_target(check
        COMMAND ctest --output-on-failure
        DEPENDS barcode_tests
        COMMENT "Running all tests with CTest..."
    )
endif()

# Информация о сборке
message(STATUS "========================================")
message(STATUS "Build configuration:")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - OpenCV: ${OpenCV_VERSION}")
message(STATUS "  - ZXing: ${ZXING_TARGET}")
message(STATUS "  - Build tests: ${BUILD_TESTS}")
message(STATUS "========================================")