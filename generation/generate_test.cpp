#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <filesystem>

#include "generate.hpp"

class GenerateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Удаляем тестовый файл перед каждым тестом, если он существует
        if (std::filesystem::exists("barcode.png")) {
            std::filesystem::remove("barcode.png");
        }
    }

    void TearDown() override {
        // Очищаем тестовый файл после каждого теста
        if (std::filesystem::exists("barcode.png")) {
            std::filesystem::remove("barcode.png");
        }
    }

    // Перенаправляем вывод для проверки сообщений
    void redirectStdout() {
        old_cout_buffer_ = std::cout.rdbuf();
        std::cout.rdbuf(test_cout_buffer_.rdbuf());
    }

    void restoreStdout() {
        std::cout.rdbuf(old_cout_buffer_);
    }

    std::string getStdoutContent() {
        return test_cout_buffer_.str();
    }

    // Перенаправляем stderr для проверки ошибок
    void redirectStderr() {
        old_cerr_buffer_ = std::cerr.rdbuf();
        std::cerr.rdbuf(test_cerr_buffer_.rdbuf());
    }

    void restoreStderr() {
        std::cerr.rdbuf(old_cerr_buffer_);
    }

    std::string getStderrContent() {
        return test_cerr_buffer_.str();
    }

private:
    std::streambuf* old_cout_buffer_;
    std::streambuf* old_cerr_buffer_;
    std::stringstream test_cout_buffer_;
    std::stringstream test_cerr_buffer_;
};

TEST_F(GenerateTest, CreatesBarcodeFileWithValidData) {
    redirectStdout();
    
    std::string testData = "1234567890";
    generate(testData);
    
    restoreStdout();
    
    // Проверяем, что файл был создан
    EXPECT_TRUE(std::filesystem::exists("barcode.png"));
    
    // Проверяем, что файл не пустой
    EXPECT_GT(std::filesystem::file_size("barcode.png"), 0);
    
    // Проверяем вывод в консоль
    std::string output = getStdoutContent();
    EXPECT_THAT(output, testing::HasSubstr("The barcode has been successfully created"));
    EXPECT_THAT(output, testing::HasSubstr("barcode.png"));
    
    // Проверяем, что сгенерированное изображение имеет правильные размеры
    cv::Mat image = cv::imread("barcode.png");
    ASSERT_FALSE(image.empty());
    
    // Ожидаемые размеры: ширина = matrix.width() * SCALE, высота = matrix.height() * SCALE
    // SCALE = 20, ширина = 300/масштаб? Фактически encode использует 300 и 100 как желаемые размеры
    EXPECT_GT(image.rows, 0);
    EXPECT_GT(image.cols, 0);
    
    // Проверяем, что изображение не полностью белое (есть черные пиксели)
    bool hasBlackPixels = false;
    for (int y = 0; y < image.rows; ++y) {
        for (int x = 0; x < image.cols; ++x) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(y, x);
            if (pixel[0] < 200 && pixel[1] < 200 && pixel[2] < 200) {
                hasBlackPixels = true;
                break;
            }
        }
        if (hasBlackPixels) break;
    }
    EXPECT_TRUE(hasBlackPixels);
}

TEST_F(GenerateTest, HandlesEmptyString) {
    redirectStderr();
    
    std::string emptyData = "";
    generate(emptyData);
    
    restoreStderr();
    
    // Должна быть ошибка при попытке кодировать пустую строку
    std::string errorOutput = getStderrContent();
    EXPECT_FALSE(errorOutput.empty());
}

TEST_F(GenerateTest, HandlesSpecialCharacters) {
    redirectStdout();
    
    std::string specialData = "ABC-123_$%^&*";
    generate(specialData);
    
    restoreStdout();
    
    EXPECT_TRUE(std::filesystem::exists("barcode.png"));
    EXPECT_GT(std::filesystem::file_size("barcode.png"), 0);
    
    // Проверяем, что изображение было создано
    cv::Mat image = cv::imread("barcode.png");
    EXPECT_FALSE(image.empty());
}

TEST_F(GenerateTest, OverwritesExistingFile) {
    // Сначала создаем фиктивный файл
    cv::Mat dummyImage(100, 100, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::imwrite("barcode.png", dummyImage);
    auto originalSize = std::filesystem::file_size("barcode.png");
    
    redirectStdout();
    
    std::string testData = "TEST_DATA";
    generate(testData);
    
    restoreStdout();
    
    // Файл должен быть перезаписан
    EXPECT_TRUE(std::filesystem::exists("barcode.png"));
    auto newSize = std::filesystem::file_size("barcode.png");
    
    // Размер должен измениться, так как новое изображение имеет другие размеры
    EXPECT_NE(newSize, originalSize);
}

TEST_F(GenerateTest, HandlesNumericData) {
    redirectStdout();
    
    std::string numericData = "9876543210";
    generate(numericData);
    
    restoreStdout();
    
    EXPECT_TRUE(std::filesystem::exists("barcode.png"));
    
    // Проверяем, что изображение имеет корректный формат (3 канала RGB)
    cv::Mat image = cv::imread("barcode.png");
    ASSERT_FALSE(image.empty());
    EXPECT_EQ(image.type(), CV_8UC3);
}

TEST_F(GenerateTest, HandlesAlphanumericData) {
    redirectStdout();
    
    std::string alnumData = "ABC123XYZ";
    generate(alnumData);
    
    restoreStdout();
    
    EXPECT_TRUE(std::filesystem::exists("barcode.png"));
    
    // Проверяем, что изображение не повреждено
    cv::Mat image = cv::imread("barcode.png");
    EXPECT_FALSE(image.empty());
}

// Тест с перенаправлением вывода ошибок при нормальном выполнении
TEST_F(GenerateTest, NoErrorOutputForValidData) {
    redirectStderr();
    
    std::string testData = "VALID_DATA_123";
    generate(testData);
    
    restoreStderr();
    
    // При успешной генерации не должно быть сообщений об ошибках
    std::string errorOutput = getStderrContent();
    EXPECT_TRUE(errorOutput.empty());
}

// Интеграционный тест: проверка возможности прочитать сгенерированный штрихкод
TEST_F(GenerateTest, GeneratedBarcodeIsReadable) {
    std::string testData = "INTEGRATION_TEST";
    generate(testData);
    
    // Проверяем, что изображение можно загрузить обратно
    cv::Mat barcodeImage = cv::imread("barcode.png");
    ASSERT_FALSE(barcodeImage.empty());
    
    // Изображение должно быть цветным (3 канала)
    EXPECT_EQ(barcodeImage.channels(), 3);
    
    // Проверяем, что изображение имеет разумные размеры
    EXPECT_GT(barcodeImage.rows, 50);
    EXPECT_GT(barcodeImage.cols, 100);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}