#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ImageView.h>
#include <ZXing/Barcode.h>

using namespace std;
using namespace cv;

//------------------------------------------------------------
// Структура кнопки для интерфейса.
// Назначение:
// 1) Хранение области, где кнопка отображается
// 2) Проверка попадания координат мыши в кнопку
//------------------------------------------------------------
struct Button {
    Rect rect;
    wstring text;

    bool contains(int x, int y) const
    {
        return x >= rect.x && x <= rect.x + rect.width &&
            y >= rect.y && y <= rect.y + rect.height;
    }

    void draw(Mat& frame) const
    {
        rectangle(frame, rect, Scalar(255, 255, 255), FILLED);
        rectangle(frame, rect, Scalar(0, 0, 0), 2);
    }
};

//------------------------------------------------------------
// Преобразование строки из UTF-8 в wide string.
// Нужно для того, чтобы отображать русскоязычный текст через WinAPI/GDI.
//------------------------------------------------------------
wstring utf8ToWide(const string& text)
{
    if (text.empty())
        return L"";

    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 0)
        return L"";

    wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &result[0], size);
    return result;
}

//------------------------------------------------------------
// Функция: проверка, находится ли один прямоугольник внутри другого.
// Используется для определения, попал ли распознанный штрих-код в целевую область.
//------------------------------------------------------------
bool isRectInside(const Rect& outer, const Rect& inner)
{
    return inner.x >= outer.x &&
        inner.y >= outer.y &&
        inner.x + inner.width <= outer.x + outer.width &&
        inner.y + inner.height <= outer.y + outer.height;
}

//------------------------------------------------------------
// Класс для отрисовки текста через GDI поверх cv::Mat.
// Причины использования:
// 1) OpenCV putText не умеет отображать русский текст с нормальным рендерингом,
// 2) Вместо этого используем возможности Windows GDI.
//
// Как работает:
// 1) Создаем временный GDI-битмап из изображения OpenCV
// 2) Рисуем текст через GDI с поддержкой русских шрифтов
// 3) Копируем изменения обратно в Mat
//------------------------------------------------------------
class GDIFrameRenderer
{
public:
    //--------------------------------------------------------
    // Конструктор:
    // Принимает ссылку на изображение OpenCV и создает
    // временный GDI bitmap/context для последующей отрисовки.
    //--------------------------------------------------------
    explicit GDIFrameRenderer(Mat& frame) : targetBGR(frame)
    {
        cvtColor(targetBGR, bgra, COLOR_BGR2BGRA);

        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = bgra.cols;
        bmi.bmiHeader.biHeight = -bgra.rows;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hdc = CreateCompatibleDC(nullptr);
        hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &dibPixels, nullptr, 0);

        if (dibPixels) {
            memcpy(dibPixels, bgra.data, bgra.total() * bgra.elemSize());
        }

        oldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);
        SetBkMode(hdc, TRANSPARENT);
    }

    //--------------------------------------------------------
    // Деструктор:
    // Копирует измененный GDI bitmap обратно в Mat
    // и освобождает все GDI-ресурсы.
    //--------------------------------------------------------
    ~GDIFrameRenderer()
    {
        if (dibPixels) {
            memcpy(bgra.data, dibPixels, bgra.total() * bgra.elemSize());
            cvtColor(bgra, targetBGR, COLOR_BGRA2BGR);
        }

        if (oldBitmap)
            SelectObject(hdc, oldBitmap);
        if (hBitmap)
            DeleteObject(hBitmap);
        if (hdc)
            DeleteDC(hdc);
    }

    //--------------------------------------------------------
    // Отрисовка текста с автоматическим подбором размера.
    //
    // Параметры:
    // text          - строка
    // area          - прямоугольная область, куда помещается текст
    // maxFontHeight - максимальная высота шрифта
    // color         - цвет текста
    // format        - флаги DrawTextW (выравнивание и т.д.)
    //
    // Возвращает:
    // ничего, результат отрисовывается в переданный Mat.
    //--------------------------------------------------------
    void drawTextFit(const wstring& text, const Rect& area, int maxFontHeight, COLORREF color, UINT format)
    {
        int fontHeight = maxFontHeight;

        while (fontHeight >= 12) {
            HFONT font = createUiFont(fontHeight);
            HFONT oldFont = (HFONT)SelectObject(hdc, font);

            RECT rcMeasure{};
            rcMeasure.left = 0;
            rcMeasure.top = 0;
            rcMeasure.right = area.width;
            rcMeasure.bottom = area.height;

            DrawTextW(hdc, text.c_str(), -1, &rcMeasure, format | DT_CALCRECT);

            int textWidth = rcMeasure.right - rcMeasure.left;
            int textHeight = rcMeasure.bottom - rcMeasure.top;

            SelectObject(hdc, oldFont);
            DeleteObject(font);

            if (textWidth <= area.width - 10 && textHeight <= area.height - 10) {
                HFONT drawFont = createUiFont(fontHeight);
                HFONT oldDrawFont = (HFONT)SelectObject(hdc, drawFont);

                SetTextColor(hdc, color);

                RECT rcDraw{};
                rcDraw.left = area.x;
                rcDraw.top = area.y;
                rcDraw.right = area.x + area.width;
                rcDraw.bottom = area.y + area.height;

                DrawTextW(hdc, text.c_str(), -1, &rcDraw, format);

                SelectObject(hdc, oldDrawFont);
                DeleteObject(drawFont);
                return;
            }

            fontHeight -= 2;
        }

        HFONT font = createUiFont(12);
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        SetTextColor(hdc, color);

        RECT rcDraw{};
        rcDraw.left = area.x;
        rcDraw.top = area.y;
        rcDraw.right = area.x + area.width;
        rcDraw.bottom = area.y + area.height;

        DrawTextW(hdc, text.c_str(), -1, &rcDraw, format);

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

public:
    //--------------------------------------------------------
    // Создание шрифта для интерфейса.
    // Характеристики шрифта:
    // - полужирный
    // - сглаженный
    // - Segoe UI
    //--------------------------------------------------------
    HFONT createUiFont(int fontHeight)
    {
        return CreateFontW(
            -fontHeight,
            0, 0, 0,
            FW_SEMIBOLD,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_TT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_NATURAL_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
    }

public:  // Сделали все поля public для тестирования
    //--------------------------------------------------------
    // Поля GDIFrameRenderer:
    //
    // targetBGR  - ссылка на исходное OpenCV-изображение, в которое будет отрисован результат
    // bgra       - временное изображение с порядком BGRA для совместимости с GDI
    // bmi        - структура bitmap для DIB section
    // dibPixels  - указатель на память bitmap
    // hdc        - device context для рисования
    // hBitmap    - bitmap, с которым работает GDI
    // oldBitmap  - предыдущий bitmap контекста, чтобы восстановить при удалении
    //--------------------------------------------------------
    Mat& targetBGR;
    Mat bgra;

    BITMAPINFO bmi{};
    void* dibPixels = nullptr;

    HDC hdc = nullptr;
    HBITMAP hBitmap = nullptr;
    HBITMAP oldBitmap = nullptr;
};

//------------------------------------------------------------
// Главный класс приложения.
// Задачи:
// 1) Захват видео с камеры
// 2) Распознавание штрих-кодов
// 3) Отрисовка интерфейса
// 4) Обработка нажатия кнопок
// 5) Вывод результата распознавания
//------------------------------------------------------------
class FrameScreen
{
public:
    //--------------------------------------------------------
    // Поля FrameScreen (сделаны public для тестирования):
    //
    // cap                 - объект видеозахвата
    // windowName          - имя окна OpenCV
    //
    // cameraFrame         - исходный кадр с камеры
    // gray                - grayscale версия кадра для ZXing
    // displayFrame        - кадр, отображаемый на экране
    //
    // screenWidth         - ширина экрана
    // screenHeight        - высота экрана
    //
    // scaleX, scaleY      - коэффициенты масштабирования cameraFrame в displayFrame
    //
    // backButton          - кнопка "Закрыть"
    //
    // baseFrame           - прямоугольная область сканирования
    // detectedRect        - прямоугольник обнаруженного штрих-кода
    //
    // shouldExit          - флаг выхода из приложения
    // codeInsideMainFrame - флаг: найден ли штрих-код внутри области сканирования
    //
    // detectedText        - текст распознанного штрих-кода
    //--------------------------------------------------------
    VideoCapture cap;
    string windowName = "Frame Scanner";

    Mat cameraFrame;
    Mat gray;
    Mat displayFrame;

    int screenWidth = 0;
    int screenHeight = 0;

    double scaleX = 1.0;
    double scaleY = 1.0;

    Button backButton;

    Rect baseFrame;
    Rect detectedRect;

    bool shouldExit = false;
    bool codeInsideMainFrame = false;

    string detectedText;

public:
    //--------------------------------------------------------
    // Инициализация приложения:
    // - открытие камеры
    // - настройка параметров кадра
    // - создание окна и установка обработчиков
    //--------------------------------------------------------
    bool init()
    {
        cap.open(0);
        cap.set(CAP_PROP_FRAME_WIDTH, 1920);
        cap.set(CAP_PROP_FRAME_HEIGHT, 1080);

        if (!cap.isOpened()) {
            cout << "Failed to open camera." << endl;
            return false;
        }

        cout << "Camera width: " << cap.get(CAP_PROP_FRAME_WIDTH) << endl;
        cout << "Camera height: " << cap.get(CAP_PROP_FRAME_HEIGHT) << endl;

        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);

        namedWindow(windowName, WINDOW_NORMAL);
        setWindowProperty(windowName, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
        setMouseCallback(windowName, FrameScreen::onMouseStatic, this);

        return true;
    }

    //--------------------------------------------------------
    // Основной цикл программы:
    // 1) захват кадра с камеры
    // 2) подготовка displayFrame
    // 3) обновление layout
    // 4) детектирование штрих-кода
    // 5) отрисовка фона
    // 6) отрисовка рамки
    // 7) отрисовка подсветки штрих-кода
    // 8) отрисовка кнопок
    // 9) отрисовка текстов
    //--------------------------------------------------------
    void run()
    {
        while (!shouldExit) {
            if (!captureFrame()) {
                break;
            }

            prepareDisplayFrame();
            updateLayout();
            detectBarcode();
            drawBackgroundMask();
            drawBaseFrame();
            drawBarcodeHighlight();
            drawButtons();
            drawTexts();

            imshow(windowName, displayFrame);
            waitKey(1);
        }

        cap.release();
        destroyAllWindows();
    }

public:
    //--------------------------------------------------------
    // Захват кадра из видеопотока.
    // Возвращает false, если кадр не получен или поток остановлен.
    //--------------------------------------------------------
    bool captureFrame()
    {
        cap >> cameraFrame;

        if (cameraFrame.empty()) {
            cout << "Empty frame received." << endl;
            return false;
        }

        return true;
    }

    //--------------------------------------------------------
    // Подготовка кадра для отображения:
    // - масштабирование изображения под размер экрана
    // - вычисление коэффициентов scaleX/scaleY
    // - создание grayscale-версии для ZXing
    //--------------------------------------------------------
    void prepareDisplayFrame()
    {
        resize(cameraFrame, displayFrame, Size(screenWidth, screenHeight), 0, 0, INTER_LINEAR);

        scaleX = static_cast<double>(displayFrame.cols) / static_cast<double>(cameraFrame.cols);
        scaleY = static_cast<double>(displayFrame.rows) / static_cast<double>(cameraFrame.rows);

        cvtColor(cameraFrame, gray, COLOR_BGR2GRAY);
    }

    //--------------------------------------------------------
    // Обновление расположения элементов интерфейса:
    // - вычисление области сканирования
    // - позиционирование кнопок
    //
    // Все координаты вычисляются относительно displayFrame.
    //--------------------------------------------------------
    void updateLayout()
    {
        int frameWidth = displayFrame.cols;
        int frameHeight = displayFrame.rows;

        int frameBoxWidth = static_cast<int>(frameWidth * 0.65);
        int frameBoxHeight = static_cast<int>(frameHeight * 0.50);
        int frameBoxX = (frameWidth - frameBoxWidth) / 2;
        int frameBoxY = static_cast<int>(frameHeight * 0.16);

        baseFrame = Rect(frameBoxX, frameBoxY, frameBoxWidth, frameBoxHeight);

        int buttonWidth = static_cast<int>(frameWidth * 0.20);
        int buttonHeight = static_cast<int>(frameHeight * 0.08);
        int buttonY = static_cast<int>(frameHeight * 0.85);
        int sideMargin = static_cast<int>(frameWidth * 0.10);

        backButton = { Rect(sideMargin, buttonY, buttonWidth, buttonHeight), L"Закрыть" };
    }

    //--------------------------------------------------------
    // Детектирование штрих-кода на изображении.
    // Что происходит:
    // - распознавание через ZXing
    // - получение координат detectedRect
    // - проверка, попадает ли detectedRect в baseFrame
    // - сохранение обнаруженного текста detectedText
    //--------------------------------------------------------
    void detectBarcode()
    {
        codeInsideMainFrame = false;
        detectedText.clear();
        detectedRect = Rect();

        ZXing::ImageView imageView(
            gray.data,
            gray.cols,
            gray.rows,
            ZXing::ImageFormat::Lum
        );

        auto result = ZXing::ReadBarcode(imageView);

        if (!result.isValid()) {
            return;
        }

        detectedText = result.text();

        auto pos = result.position();
        auto p1 = pos.topLeft();
        auto p2 = pos.topRight();
        auto p3 = pos.bottomRight();
        auto p4 = pos.bottomLeft();

        float minX = static_cast<float>(min(min(p1.x, p2.x), min(p3.x, p4.x)));
        float minY = static_cast<float>(min(min(p1.y, p2.y), min(p3.y, p4.y)));
        float maxX = static_cast<float>(max(max(p1.x, p2.x), max(p3.x, p4.x)));
        float maxY = static_cast<float>(max(max(p1.y, p2.y), max(p3.y, p4.y)));

        int rectX = static_cast<int>(minX * scaleX);
        int rectY = static_cast<int>(minY * scaleY);
        int rectW = static_cast<int>((maxX - minX) * scaleX);
        int rectH = static_cast<int>((maxY - minY) * scaleY);

        if (rectW <= 0 || rectH <= 0) {
            return;
        }

        detectedRect = Rect(rectX, rectY, rectW, rectH);

        if (isRectInside(baseFrame, detectedRect)) {
            codeInsideMainFrame = true;
        }
    }

    //--------------------------------------------------------
    // Отрисовка затемненного фона за пределами области сканирования.
    // Принцип работы: копируем displayFrame, затемняем все,
    // и возвращаем обратно область baseFrame без затемнения.
    //--------------------------------------------------------
    void drawBackgroundMask()
    {
        Mat darkened;
        displayFrame.copyTo(darkened);
        darkened.convertTo(darkened, -1, 0.35, 0);
        displayFrame(baseFrame).copyTo(darkened(baseFrame));
        displayFrame = darkened;
    }

    //--------------------------------------------------------
    // Отрисовка рамки области сканирования с уголками.
    //--------------------------------------------------------
    void drawBaseFrame()
    {
        rectangle(displayFrame, baseFrame, Scalar(0, 0, 0), 2);
        drawFrameCorners(displayFrame, baseFrame);
    }

    //--------------------------------------------------------
    // Рисуем подсветку для корректно распознанного кода,
    // который находится внутри области сканирования, если он есть.
    //--------------------------------------------------------
    void drawBarcodeHighlight()
    {
        if (codeInsideMainFrame) {
            rectangle(displayFrame, detectedRect, Scalar(0, 255, 0), 3);
        }
    }

    //--------------------------------------------------------
    // Отрисовка всех кнопок интерфейса.
    // Использует метод draw структуры Button.
    //--------------------------------------------------------
    void drawButtons()
    {
        backButton.draw(displayFrame);
    }

    //--------------------------------------------------------
    // Рисует все текстовые надписи:
    // - подсказка
    // - текст на кнопках
    // - результат распознавания
    //--------------------------------------------------------
    void drawTexts()
    {
        drawHintText();
        drawButtonTexts();
        drawDetectedCodeText();
    }

    //--------------------------------------------------------
    // Отрисовка подсказки для пользователя внутри области сканирования.
    //--------------------------------------------------------
    void drawHintText()
    {
        GDIFrameRenderer textRenderer(displayFrame);

        int frameHeight = displayFrame.rows;
        int hintFontHeight = static_cast<int>(frameHeight * 0.035);

        Rect hintRect(
            baseFrame.x,
            baseFrame.y + baseFrame.height - hintFontHeight - 10,
            baseFrame.width,
            hintFontHeight + 20
        );

        textRenderer.drawTextFit(
            L"Поместите штрих-код в эту область",
            hintRect,
            hintFontHeight,
            RGB(0, 0, 0),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }

    //--------------------------------------------------------
    // Рисует текст на кнопках интерфейса.
    //--------------------------------------------------------
    void drawButtonTexts()
    {
        GDIFrameRenderer textRenderer(displayFrame);

        int frameHeight = displayFrame.rows;
        int buttonFontHeight = static_cast<int>(frameHeight * 0.035);

        textRenderer.drawTextFit(
            backButton.text,
            backButton.rect,
            buttonFontHeight,
            RGB(0, 0, 0),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }

    //--------------------------------------------------------
    // Рисует результат распознанного штрих-кода после успешного сканирования.
    // Результат отображается прямо по центру области сканирования.
    //--------------------------------------------------------
    void drawDetectedCodeText()
    {
        if (!codeInsideMainFrame) {
            return;
        }

        wstring detectedWide = utf8ToWide(detectedText);
        int frameWidth = displayFrame.cols;
        int frameHeight = displayFrame.rows;
        int codeFontHeight = static_cast<int>(frameHeight * 0.04);

        Rect textBg(
            frameWidth / 2 - static_cast<int>(frameWidth * 0.28),
            frameHeight / 2 - codeFontHeight - 14,
            static_cast<int>(frameWidth * 0.56),
            codeFontHeight + 36
        );

        rectangle(displayFrame, textBg, Scalar(0, 0, 0), FILLED);
        rectangle(displayFrame, textBg, Scalar(0, 255, 0), 2);

        GDIFrameRenderer textRenderer(displayFrame);
        textRenderer.drawTextFit(
            detectedWide,
            textBg,
            codeFontHeight,
            RGB(0, 255, 0),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }

    //--------------------------------------------------------
    // Обработчик событий мыши:
    // - нажатие на кнопку -> завершение программы
    // - нажатие на поле ввода -> переход к ручному вводу
    //--------------------------------------------------------
    void onMouse(int event, int x, int y)
    {
        if (event != EVENT_LBUTTONDOWN) {
            return;
        }

        if (backButton.contains(x, y)) {
            shouldExit = true;
        }
    }

    //--------------------------------------------------------
    // Статическая функция для OpenCV mouse callback.
    // OpenCV требует статическую функцию-обработчик, поэтому передаем userdata
    // для получения указателя на текущий объект FrameScreen.
    //--------------------------------------------------------
    static void onMouseStatic(int event, int x, int y, int flags, void* userdata)
    {
        FrameScreen* self = static_cast<FrameScreen*>(userdata);
        if (self) {
            self->onMouse(event, x, y);
        }
    }

    //--------------------------------------------------------
    // Отрисовка уголков вокруг прямоугольной области.
    //--------------------------------------------------------
    void drawFrameCorners(Mat& frame, const Rect& rect)
    {
        int cornerLen = static_cast<int>(min(rect.width, rect.height) * 0.10);
        int cornerThickness = 6;

        line(frame, Point(rect.x, rect.y), Point(rect.x + cornerLen, rect.y), Scalar(0, 0, 0), cornerThickness);
        line(frame, Point(rect.x, rect.y), Point(rect.x, rect.y + cornerLen), Scalar(0, 0, 0), cornerThickness);

        line(frame, Point(rect.x + rect.width, rect.y), Point(rect.x + rect.width - cornerLen, rect.y), Scalar(0, 0, 0), cornerThickness);
        line(frame, Point(rect.x + rect.width, rect.y), Point(rect.x + rect.width, rect.y + cornerLen), Scalar(0, 0, 0), cornerThickness);

        line(frame, Point(rect.x, rect.y + rect.height), Point(rect.x + cornerLen, rect.y + rect.height), Scalar(0, 0, 0), cornerThickness);
        line(frame, Point(rect.x, rect.y + rect.height), Point(rect.x, rect.y + rect.height - cornerLen), Scalar(0, 0, 0), cornerThickness);

        line(frame, Point(rect.x + rect.width, rect.y + rect.height), Point(rect.x + rect.width - cornerLen, rect.y + rect.height), Scalar(0, 0, 0), cornerThickness);
        line(frame, Point(rect.x + rect.width, rect.y + rect.height), Point(rect.x + rect.width, rect.y + rect.height - cornerLen), Scalar(0, 0, 0), cornerThickness);
    }
};

//------------------------------------------------------------
// Точка входа в программу.
// Создаёт объект экрана, инициализирует его и запускает цикл.
//------------------------------------------------------------

//------------------------------------------------------------
// Точка входа в программу.
// Создаёт объект экрана, инициализирует его и запускает цикл.
//------------------------------------------------------------
#ifndef TESTING_MODE  // Не компилируем main при тестировании
int main()
{
    FrameScreen screen;

    if (!screen.init()) {
        return 1;
    }

    screen.run();
    return 0;
}
#endif // TESTING_MODE