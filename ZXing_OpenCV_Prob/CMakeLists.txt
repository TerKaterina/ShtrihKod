cmake_minimum_required(VERSION 3.14)
project(FrameScanner)

# Настройка C++ стандарта
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции для тестирования
option(BUILD_TESTS "Build tests" ON)

# Определяем NOMINMAX для всех таргетов
add_compile_definitions(NOMINMAX)

# Поиск зависимостей
find_package(OpenCV REQUIRED)
find_package(ZXing REQUIRED)

# Основной исполняемый файл (без тестов)
# frame.cpp компилируется без TESTING_MODE, поэтому main() будет включен
add_executable(frame_scanner frame.cpp)

# Подключаем библиотеки для основной программы
target_link_libraries(frame_scanner
    ${OpenCV_LIBS}
    ZXing::ZXing
)

# На Windows добавляем библиотеку для GDI
if(WIN32)
    target_link_libraries(frame_scanner gdi32)
endif()

# Если нужно собрать тесты
if(BUILD_TESTS)
    enable_testing()
    
    find_package(GTest REQUIRED)
    
    # Создаем тестовый исполняемый файл
    # frame_tests.cpp будет включать frame.cpp с TESTING_MODE
    add_executable(frame_tests tests/test_frame.cpp)
    
    # Подключаем библиотеки для тестов
    target_link_libraries(frame_tests
        ${OpenCV_LIBS}
        ZXing::ZXing
        GTest::gtest
        GTest::gtest_main
    )
    
    # На Windows добавляем библиотеку для GDI
    if(WIN32)
        target_link_libraries(frame_tests gdi32)
    endif()
    
    # Определяем TESTING_MODE для тестов (это отключит main() в frame.cpp)
    target_compile_definitions(frame_tests PRIVATE TESTING_MODE)
    
    # Добавляем тесты
    add_test(NAME FrameTests COMMAND frame_tests)
    
    # Опционально: добавить зависимость, чтобы тесты собирались после основной программы
    add_dependencies(frame_tests frame_scanner)
endif()

# Установка
install(TARGETS frame_scanner RUNTIME DESTINATION bin)

# Вывод информации о сборке
message(STATUS "Build configuration:")
message(STATUS "  - Main executable: frame_scanner.exe")
if(BUILD_TESTS)
    message(STATUS "  - Tests executable: frame_tests.exe")
    message(STATUS "  - Testing mode: ENABLED")
else()
    message(STATUS "  - Tests: DISABLED")
endif()