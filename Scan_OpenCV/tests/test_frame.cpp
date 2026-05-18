// Определяем NOMINMAX перед любыми включениями Windows.h
#define NOMINMAX

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <windows.h>

// Определяем TESTING_MODE перед включением frame.cpp
#define TESTING_MODE

// Включаем тестируемый файл
#include "../frame.cpp"

using namespace cv;
using namespace std;

//------------------------------------------------------------
// Тесты для структуры Button
//------------------------------------------------------------
TEST(ButtonTest, ContainsPointInside)
{
    Button btn;
    btn.rect = Rect(10, 20, 100, 50);
    
    EXPECT_TRUE(btn.contains(10, 20));   // Левый верхний угол
    EXPECT_TRUE(btn.contains(109, 69));  // Правый нижний угол
    EXPECT_TRUE(btn.contains(50, 40));   // Середина
    EXPECT_TRUE(btn.contains(10, 69));   // Левый нижний
    EXPECT_TRUE(btn.contains(109, 20));  // Правый верхний
}

TEST(ButtonTest, ContainsPointOutside)
{
    Button btn;
    btn.rect = Rect(10, 20, 100, 50);
    
    EXPECT_FALSE(btn.contains(9, 40));    // Левее
    EXPECT_FALSE(btn.contains(50, 19));   // Выше
    EXPECT_FALSE(btn.contains(110, 40));  // Правее
    EXPECT_FALSE(btn.contains(50, 70));   // Ниже
    EXPECT_FALSE(btn.contains(-1, -1));   // Далеко за пределами
}

TEST(ButtonTest, DrawDoesNotCrash)
{
    Button btn;
    btn.rect = Rect(10, 20, 100, 50);
    btn.text = L"Test Button";
    
    Mat frame(480, 640, CV_8UC3, Scalar(0, 0, 0));
    
    EXPECT_NO_THROW(btn.draw(frame));
}

TEST(ButtonTest, DrawOnEmptyFrame)
{
    Button btn;
    btn.rect = Rect(0, 0, 100, 50);
    btn.text = L"Button";
    
    Mat frame;
    EXPECT_NO_THROW(btn.draw(frame));
}

//------------------------------------------------------------
// Тесты для функции utf8ToWide
//------------------------------------------------------------
TEST(Utf8ToWideTest, EmptyString)
{
    wstring result = utf8ToWide("");
    EXPECT_TRUE(result.empty());
}

TEST(Utf8ToWideTest, AsciiString)
{
    string utf8 = "Hello World";
    wstring expected = L"Hello World";
    wstring result = utf8ToWide(utf8);
    EXPECT_EQ(result, expected);
}

TEST(Utf8ToWideTest, RussianText)
{
    string utf8 = u8"Привет мир";
    wstring result = utf8ToWide(utf8);
    
    EXPECT_EQ(result.length(), 9);  // "Привет мир" - 9 символов
    EXPECT_EQ(result[0], L'П');
    EXPECT_EQ(result[1], L'р');
    EXPECT_EQ(result[2], L'и');
}

TEST(Utf8ToWideTest, MixedUnicode)
{
    string utf8 = u8"Hello Привет 123";
    wstring result = utf8ToWide(utf8);
    EXPECT_FALSE(result.empty());
    EXPECT_GT(result.length(), 0);
}

TEST(Utf8ToWideTest, SpecialCharacters)
{
    string utf8 = u8"!@#$%^&*()";
    wstring result = utf8ToWide(utf8);
    EXPECT_EQ(result.length(), 10);
}

//------------------------------------------------------------
// Тесты для функции isRectInside
//------------------------------------------------------------
TEST(IsRectInsideTest, FullyInside)
{
    Rect outer(0, 0, 100, 100);
    Rect inner(10, 10, 80, 80);
    
    EXPECT_TRUE(isRectInside(outer, inner));
}

TEST(IsRectInsideTest, ExactlyInside)
{
    Rect outer(0, 0, 100, 100);
    Rect inner(0, 0, 100, 100);
    
    EXPECT_TRUE(isRectInside(outer, inner));
}

TEST(IsRectInsideTest, PartiallyOutside)
{
    Rect outer(0, 0, 100, 100);
    Rect inner(-10, 0, 100, 100);
    
    EXPECT_FALSE(isRectInside(outer, inner));
}

TEST(IsRectInsideTest, CompletelyOutside)
{
    Rect outer(0, 0, 100, 100);
    Rect inner(200, 200, 50, 50);
    
    EXPECT_FALSE(isRectInside(outer, inner));
}

TEST(IsRectInsideTest, LargerThanOuter)
{
    Rect outer(0, 0, 100, 100);
    Rect inner(-50, -50, 200, 200);
    
    EXPECT_FALSE(isRectInside(outer, inner));
}

TEST(IsRectInsideTest, TouchingEdges)
{
    Rect outer(0, 0, 100, 100);
    Rect inner(0, 0, 100, 50);
    
    EXPECT_TRUE(isRectInside(outer, inner));
}

TEST(IsRectInsideTest, ZeroSizeInner)
{
    Rect outer(0, 0, 100, 100);
    Rect inner(50, 50, 0, 0);
    
    EXPECT_TRUE(isRectInside(outer, inner));
}

//------------------------------------------------------------
// Тесты для класса GDIFrameRenderer
//------------------------------------------------------------




///////////////////////////////////СЮДА ВСТАВИТЬ КАТЯ!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!








//------------------------------------------------------------
// Тесты для класса FrameScreen
//------------------------------------------------------------
class FrameScreenTest : public ::testing::Test {
protected:
    void SetUp() override {
        screen = new FrameScreen();
    }
    
    void TearDown() override {
        delete screen;
    }
    
    FrameScreen* screen;
};

TEST_F(FrameScreenTest, ConstructorInitializesCorrectly)
{
    EXPECT_EQ(screen->windowName, "Frame Scanner");
    EXPECT_FALSE(screen->shouldExit);
    EXPECT_FALSE(screen->codeInsideMainFrame);
    EXPECT_TRUE(screen->detectedText.empty());
    EXPECT_EQ(screen->screenWidth, 0);
    EXPECT_EQ(screen->screenHeight, 0);
    EXPECT_EQ(screen->scaleX, 1.0);
    EXPECT_EQ(screen->scaleY, 1.0);
    EXPECT_TRUE(screen->cameraFrame.empty());
    EXPECT_TRUE(screen->gray.empty());
    EXPECT_TRUE(screen->displayFrame.empty());
}

TEST_F(FrameScreenTest, DrawFrameCornersDoesNotCrash)
{
    Mat frame(480, 640, CV_8UC3, Scalar(0, 0, 0));
    Rect rect(100, 100, 200, 150);
    
    EXPECT_NO_THROW(screen->drawFrameCorners(frame, rect));
}

TEST_F(FrameScreenTest, DrawFrameCornersWithSmallRectangle)
{
    Mat frame(480, 640, CV_8UC3, Scalar(0, 0, 0));
    Rect rect(10, 10, 20, 20);
    
    EXPECT_NO_THROW(screen->drawFrameCorners(frame, rect));
}

TEST_F(FrameScreenTest, DrawFrameCornersWithZeroSize)
{
    Mat frame(480, 640, CV_8UC3, Scalar(0, 0, 0));
    Rect rect(100, 100, 0, 0);
    
    EXPECT_NO_THROW(screen->drawFrameCorners(frame, rect));
}

TEST_F(FrameScreenTest, DrawFrameCornersWithNegativeSize)
{
    Mat frame(480, 640, CV_8UC3, Scalar(0, 0, 0));
    Rect rect(100, 100, -50, -50);
    
    EXPECT_NO_THROW(screen->drawFrameCorners(frame, rect));
}

TEST_F(FrameScreenTest, DrawFrameCornersWithEmptyFrame)
{
    Mat frame;
    Rect rect(100, 100, 200, 150);
    
    EXPECT_NO_THROW(screen->drawFrameCorners(frame, rect));
}

TEST_F(FrameScreenTest, OnMouseEventHandling)
{
    screen->backButton.rect = Rect(10, 10, 100, 50);
    screen->shouldExit = false;
    
    // Клик по кнопке должен установить флаг выхода
    screen->onMouse(EVENT_LBUTTONDOWN, 50, 30);
    EXPECT_TRUE(screen->shouldExit);
    
    // Сброс флага
    screen->shouldExit = false;
    
    // Клик вне кнопки не должен устанавливать флаг
    screen->onMouse(EVENT_LBUTTONDOWN, 200, 200);
    EXPECT_FALSE(screen->shouldExit);
    
    // Другие события мыши не должны влиять
    screen->onMouse(EVENT_RBUTTONDOWN, 50, 30);
    EXPECT_FALSE(screen->shouldExit);
    
    screen->onMouse(EVENT_MOUSEMOVE, 50, 30);
    EXPECT_FALSE(screen->shouldExit);
}

TEST_F(FrameScreenTest, UpdateLayoutCalculations)
{
    // Создаем тестовый displayFrame
    screen->displayFrame = Mat(1080, 1920, CV_8UC3);
    screen->screenWidth = 1920;
    screen->screenHeight = 1080;
    
    screen->updateLayout();
    
    // Проверяем, что baseFrame был вычислен
    EXPECT_GT(screen->baseFrame.width, 0);
    EXPECT_GT(screen->baseFrame.height, 0);
    EXPECT_GT(screen->baseFrame.x, 0);
    EXPECT_GT(screen->baseFrame.y, 0);
    
    // Проверяем размеры области сканирования
    int expectedWidth = static_cast<int>(1920 * 0.65);
    int expectedHeight = static_cast<int>(1080 * 0.50);
    EXPECT_EQ(screen->baseFrame.width, expectedWidth);
    EXPECT_EQ(screen->baseFrame.height, expectedHeight);
    
    // Проверяем, что кнопка была создана
    EXPECT_GT(screen->backButton.rect.width, 0);
    EXPECT_GT(screen->backButton.rect.height, 0);
    EXPECT_FALSE(screen->backButton.text.empty());
    EXPECT_EQ(screen->backButton.text, L"Назад");
}

TEST_F(FrameScreenTest, PrepareDisplayFrame)
{
    // Создаем тестовый кадр
    screen->cameraFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->screenWidth = 1920;
    screen->screenHeight = 1080;
    
    screen->prepareDisplayFrame();
    
    // Проверяем, что displayFrame был создан
    EXPECT_FALSE(screen->displayFrame.empty());
    EXPECT_EQ(screen->displayFrame.cols, screen->screenWidth);
    EXPECT_EQ(screen->displayFrame.rows, screen->screenHeight);
    
    // Проверяем коэффициенты масштабирования
    EXPECT_EQ(screen->scaleX, 1.0);
    EXPECT_EQ(screen->scaleY, 1.0);
    
    // Проверяем grayscale изображение
    EXPECT_FALSE(screen->gray.empty());
    EXPECT_EQ(screen->gray.channels(), 1);
}

TEST_F(FrameScreenTest, CaptureFrameWithoutCamera)
{
    // Без реальной камеры captureFrame должен вернуть false
    // и cameraFrame должен быть пустым
    bool result = screen->captureFrame();
    // Может вернуть false, так как камера не открыта
    // или cameraFrame может быть пустым
    if (!result) {
        EXPECT_TRUE(screen->cameraFrame.empty());
    }
}

TEST_F(FrameScreenTest, DrawBackgroundMask)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->baseFrame = Rect(100, 100, 500, 300);
    
    Mat original = screen->displayFrame.clone();
    
    EXPECT_NO_THROW(screen->drawBackgroundMask());
    
    // Проверяем, что изображение изменилось
    bool isDifferent = false;
    for (int i = 0; i < original.rows && !isDifferent; i++) {
        for (int j = 0; j < original.cols && !isDifferent; j++) {
            if (original.at<Vec3b>(i, j) != screen->displayFrame.at<Vec3b>(i, j)) {
                isDifferent = true;
            }
        }
    }
    EXPECT_TRUE(isDifferent);
}

TEST_F(FrameScreenTest, DrawBaseFrame)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->baseFrame = Rect(100, 100, 500, 300);
    
    EXPECT_NO_THROW(screen->drawBaseFrame());
}

TEST_F(FrameScreenTest, DrawBarcodeHighlight)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->detectedRect = Rect(200, 200, 100, 50);
    
    // Когда код внутри области
    screen->codeInsideMainFrame = true;
    EXPECT_NO_THROW(screen->drawBarcodeHighlight());
    
    // Когда код не внутри области
    screen->codeInsideMainFrame = false;
    EXPECT_NO_THROW(screen->drawBarcodeHighlight());
}

TEST_F(FrameScreenTest, DrawButtons)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->backButton.rect = Rect(100, 100, 200, 50);
    
    EXPECT_NO_THROW(screen->drawButtons());
}

TEST_F(FrameScreenTest, DrawTexts)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->baseFrame = Rect(100, 100, 500, 300);
    screen->backButton.rect = Rect(100, 100, 200, 50);
    
    EXPECT_NO_THROW(screen->drawTexts());
}

TEST_F(FrameScreenTest, DrawHintText)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->baseFrame = Rect(100, 100, 500, 300);
    
    EXPECT_NO_THROW(screen->drawHintText());
}

TEST_F(FrameScreenTest, DrawButtonTexts)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->backButton.rect = Rect(100, 100, 200, 50);
    screen->backButton.text = L"Test";
    
    EXPECT_NO_THROW(screen->drawButtonTexts());
}

TEST_F(FrameScreenTest, DrawDetectedCodeText)
{
    screen->displayFrame = Mat(1080, 1920, CV_8UC3, Scalar(100, 100, 100));
    screen->detectedText = "123456789";
    screen->codeInsideMainFrame = true;
    
    EXPECT_NO_THROW(screen->drawDetectedCodeText());
    
    // Когда код не внутри области
    screen->codeInsideMainFrame = false;
    EXPECT_NO_THROW(screen->drawDetectedCodeText());
}

//------------------------------------------------------------
// Тесты для проверки математических вычислений
//------------------------------------------------------------
TEST(MathTest, ScaleCalculations)
{
    int cameraWidth = 1920;
    int cameraHeight = 1080;
    int screenWidth = 1920;
    int screenHeight = 1080;
    
    double scaleX = static_cast<double>(screenWidth) / cameraWidth;
    double scaleY = static_cast<double>(screenHeight) / cameraHeight;
    
    EXPECT_DOUBLE_EQ(scaleX, 1.0);
    EXPECT_DOUBLE_EQ(scaleY, 1.0);
    
    screenWidth = 3840;
    scaleX = static_cast<double>(screenWidth) / cameraWidth;
    EXPECT_DOUBLE_EQ(scaleX, 2.0);
}

TEST(MathTest, LayoutCalculations)
{
    int frameWidth = 1920;
    int frameHeight = 1080;
    
    int frameBoxWidth = static_cast<int>(frameWidth * 0.65);
    int frameBoxHeight = static_cast<int>(frameHeight * 0.50);
    int frameBoxX = (frameWidth - frameBoxWidth) / 2;
    int frameBoxY = static_cast<int>(frameHeight * 0.16);
    
    EXPECT_EQ(frameBoxWidth, 1248);
    EXPECT_EQ(frameBoxHeight, 540);
    EXPECT_EQ(frameBoxX, 336);
    EXPECT_EQ(frameBoxY, 172);
}

TEST(MathTest, ButtonLayoutCalculations)
{
    int frameWidth = 1920;
    int frameHeight = 1080;
    
    int buttonWidth = static_cast<int>(frameWidth * 0.20);
    int buttonHeight = static_cast<int>(frameHeight * 0.08);
    int buttonY = static_cast<int>(frameHeight * 0.85);
    int sideMargin = static_cast<int>(frameWidth * 0.10);
    
    EXPECT_EQ(buttonWidth, 384);
    EXPECT_EQ(buttonHeight, 86);
    EXPECT_EQ(buttonY, 918);
    EXPECT_EQ(sideMargin, 192);
}

TEST(MathTest, FontHeightCalculations)
{
    int frameHeight = 1080;
    
    int hintFontHeight = static_cast<int>(frameHeight * 0.035);
    int buttonFontHeight = static_cast<int>(frameHeight * 0.035);
    int codeFontHeight = static_cast<int>(frameHeight * 0.04);
    
    EXPECT_EQ(hintFontHeight, 37);
    EXPECT_EQ(buttonFontHeight, 37);
    EXPECT_EQ(codeFontHeight, 43);
}

//------------------------------------------------------------
// Интеграционные тесты
//------------------------------------------------------------
TEST(IntegrationTest, ButtonAndRectInteraction)
{
    Button btn;
    btn.rect = Rect(10, 10, 200, 50);
    btn.text = L"Click Me";
    
    EXPECT_TRUE(btn.contains(10, 10));
    EXPECT_TRUE(btn.contains(209, 59));
    EXPECT_FALSE(btn.contains(5, 5));
    
    Mat frame(480, 640, CV_8UC3);
    EXPECT_NO_THROW(btn.draw(frame));
}

TEST(IntegrationTest, IsRectInsideVariousCases)
{
    Rect outer(0, 0, 100, 100);
    
    EXPECT_TRUE(isRectInside(outer, Rect(10, 10, 80, 80)));
    EXPECT_TRUE(isRectInside(outer, Rect(0, 0, 100, 100)));
    EXPECT_FALSE(isRectInside(outer, Rect(-10, 0, 100, 100)));
    EXPECT_FALSE(isRectInside(outer, Rect(200, 200, 50, 50)));
}

TEST(IntegrationTest, Utf8ConversionChain)
{
    string original = u8"Тест123";
    wstring wide = utf8ToWide(original);
    EXPECT_FALSE(wide.empty());
    EXPECT_EQ(wide.length(), 7); // "Тест123" - 7 символов
}

TEST(IntegrationTest, FullPipelineWithoutCamera)
{
    FrameScreen screen;
    
    // Инициализация не должна вызвать исключений даже без камеры
    // (может вернуть false, но не должно падать)
    EXPECT_NO_THROW(screen.init());
    
    // Ручная установка флага выхода
    screen.shouldExit = true;
    EXPECT_NO_THROW(screen.run());
}

//------------------------------------------------------------
// Тесты для проверки граничных условий
//------------------------------------------------------------
TEST(EdgeCasesTest, EmptyButtonText)
{
    Button btn;
    btn.rect = Rect(0, 0, 100, 50);
    btn.text = L"";
    
    Mat frame(480, 640, CV_8UC3);
    EXPECT_NO_THROW(btn.draw(frame));
}

TEST(EdgeCasesTest, ButtonWithZeroSize)
{
    Button btn;
    btn.rect = Rect(0, 0, 0, 0);
    btn.text = L"Test";
    
    Mat frame(480, 640, CV_8UC3);
    EXPECT_NO_THROW(btn.draw(frame));
}

TEST(EdgeCasesTest, DetectBarcodeWithEmptyGray)
{
    FrameScreen screen;
    screen.gray = Mat();
    
    EXPECT_NO_THROW(screen.detectBarcode());
    EXPECT_FALSE(screen.codeInsideMainFrame);
    EXPECT_TRUE(screen.detectedText.empty());
}

//------------------------------------------------------------
// Тесты для проверки работы с памятью
//------------------------------------------------------------
TEST(MemoryTest, MultipleGDIRenderers)
{
    Mat frame1(480, 640, CV_8UC3);
    Mat frame2(480, 640, CV_8UC3);
    
    EXPECT_NO_THROW({
        GDIFrameRenderer renderer1(frame1);
        GDIFrameRenderer renderer2(frame2);
        renderer1.drawTextFit(L"Test1", Rect(10, 10, 100, 50), 20, RGB(255, 0, 0), DT_CENTER);
        renderer2.drawTextFit(L"Test2", Rect(10, 10, 100, 50), 20, RGB(0, 255, 0), DT_CENTER);
    });
}

TEST(MemoryTest, FrameScreenCopy)
{
    FrameScreen screen1;
    FrameScreen screen2 = screen1; // Проверка копирования
    
    EXPECT_EQ(screen2.windowName, screen1.windowName);
    EXPECT_EQ(screen2.shouldExit, screen1.shouldExit);
}

//------------------------------------------------------------
// Главная функция запуска тестов
//------------------------------------------------------------
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}