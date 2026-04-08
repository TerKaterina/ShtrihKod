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
// Структура кнопки интерфейса.
// Хранит прямоугольную область кнопки и её текст.
// Умеет:
// 1) проверять, был ли клик внутри кнопки
// 2) рисовать саму кнопку на кадре
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
// Перевод строки из UTF-8 в wide string.
// Нужен для нормального отображения русского текста через WinAPI/GDI.
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
// Проверка: лежит ли внутренний прямоугольник полностью
// внутри внешнего прямоугольника.
// Используется для проверки, что найденный штрих-код
// действительно находится внутри основной рамки сканирования.
//------------------------------------------------------------
bool isRectInside(const Rect& outer, const Rect& inner)
{
    return inner.x >= outer.x &&
        inner.y >= outer.y &&
        inner.x + inner.width <= outer.x + outer.width &&
        inner.y + inner.height <= outer.y + outer.height;
}

//------------------------------------------------------------
// Класс для рисования текста через GDI поверх cv::Mat.
// Зачем нужен:
// обычный OpenCV putText плохо работает с кириллицей,
// поэтому текст рисуется через Windows GDI.
//
// Что делает:
// 1) создаёт временный GDI-контекст для текущего кадра
// 2) позволяет нарисовать текст с автоподбором размера
// 3) после завершения переносит результат обратно в Mat
//------------------------------------------------------------
class GDIFrameRenderer
{
public:
    //--------------------------------------------------------
    // Конструктор:
    // получает ссылку на кадр OpenCV и создаёт поверх него
    // временный GDI bitmap/context для рисования текста.
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
    // копирует изменённый GDI bitmap обратно в Mat
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
    // Рисование текста с подбором размера шрифта.
    //
    // Параметры:
    // text          - текст
    // area          - прямоугольная область, куда надо вписать текст
    // maxFontHeight - максимальная высота шрифта
    // color         - цвет текста
    // format        - флаги DrawTextW (выравнивание и т.д.)
    //
    // Логика:
    // если текст не помещается, размер шрифта постепенно уменьшается.
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

private:
    //--------------------------------------------------------
    // Создание шрифта интерфейса.
    // Здесь настраивается:
    // - гарнитура
    // - сглаживание
    // - толщина
    // - размер
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

private:
    //--------------------------------------------------------
    // Поля GDIFrameRenderer:
    //
    // targetBGR  - исходный кадр OpenCV, в который потом вернётся текст
    // bgra       - временная копия кадра в формате BGRA для GDI
    // bmi        - описание bitmap для DIB section
    // dibPixels  - указатель на память bitmap
    // hdc        - device context для рисования
    // hBitmap    - bitmap, в который рисует GDI
    // oldBitmap  - старый bitmap контекста, чтобы вернуть его назад
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
// Главный класс экрана сканирования.
// Отвечает за:
// 1) запуск камеры
// 2) основной цикл работы экрана
// 3) распознавание штрих-кода
// 4) построение интерфейса
// 5) обработку кнопок
//------------------------------------------------------------
class FrameScreen
{
public:
    //--------------------------------------------------------
    // Инициализация экрана:
    // - открытие камеры
    // - установка желаемого разрешения
    // - получение размеров экрана
    // - создание полноэкранного окна
    // - привязка обработчика мыши
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
    // Основной цикл экрана.
    // Каждый проход:
    // 1) берёт кадр с камеры
    // 2) подготавливает displayFrame
    // 3) обновляет layout
    // 4) ищет штрих-код
    // 5) рисует интерфейс
    // 6) показывает результат
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

private:
    //--------------------------------------------------------
    // Получение нового кадра с камеры.
    // Возвращает false, если кадр получить не удалось.
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
    // - масштабируем исходный кадр до размера экрана
    // - считаем коэффициенты scaleX/scaleY
    // - создаём grayscale-кадр для ZXing
    //--------------------------------------------------------
    void prepareDisplayFrame()
    {
        resize(cameraFrame, displayFrame, Size(screenWidth, screenHeight), 0, 0, INTER_LINEAR);

        scaleX = static_cast<double>(displayFrame.cols) / static_cast<double>(cameraFrame.cols);
        scaleY = static_cast<double>(displayFrame.rows) / static_cast<double>(cameraFrame.rows);

        cvtColor(cameraFrame, gray, COLOR_BGR2GRAY);
    }

    //--------------------------------------------------------
    // Пересчёт расположения интерфейса:
    // - основной рамки
    // - кнопки Назад
    // - кнопки Ввод
    //
    // Всё считается относительно текущего размера displayFrame.
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

        backButton = { Rect(sideMargin, buttonY, buttonWidth, buttonHeight), L"Назад" };
        inputButton = { Rect(frameWidth - sideMargin - buttonWidth, buttonY, buttonWidth, buttonHeight), L"Ввод" };
    }

    //--------------------------------------------------------
    // Поиск штрих-кода на текущем кадре.
    // Если код найден:
    // - вычисляется прямоугольник detectedRect
    // - проверяется, лежит ли он внутри baseFrame
    // - сохраняется текст detectedText
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
    // Затемнение области вне основной рамки.
    // Делает экран визуально понятнее: пользователь должен
    // смотреть в зону сканирования, а не по краям.
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
    // Рисование основной рамки сканирования и её уголков.
    //--------------------------------------------------------
    void drawBaseFrame()
    {
        rectangle(displayFrame, baseFrame, Scalar(0, 0, 0), 2);
        drawFrameCorners(displayFrame, baseFrame);
    }

    //--------------------------------------------------------
    // Если код находится внутри основной рамки,
    // рисуется зелёная рамка по его границам.
    //--------------------------------------------------------
    void drawBarcodeHighlight()
    {
        if (codeInsideMainFrame) {
            rectangle(displayFrame, detectedRect, Scalar(0, 255, 0), 3);
        }
    }

    //--------------------------------------------------------
    // Рисование прямоугольников кнопок.
    // Только форма кнопок, без текста.
    //--------------------------------------------------------
    void drawButtons()
    {
        backButton.draw(displayFrame);
        inputButton.draw(displayFrame);
    }

    //--------------------------------------------------------
    // Общий метод рисования текстов интерфейса.
    // Вызывает отдельные методы для:
    // - подсказки
    // - текста кнопок
    // - текста найденного кода
    // - верхнего сообщения
    //--------------------------------------------------------
    void drawTexts()
    {
        drawHintText();
        drawButtonTexts();
        drawDetectedCodeText();
        drawTopMessage();
    }

    //--------------------------------------------------------
    // Подсказка внутри нижней части основной рамки.
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
            L"Разместите код внутри рамки",
            hintRect,
            hintFontHeight,
            RGB(0, 0, 0),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }

    //--------------------------------------------------------
    // Текст кнопок Назад и Ввод.
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

        textRenderer.drawTextFit(
            inputButton.text,
            inputButton.rect,
            buttonFontHeight,
            RGB(0, 0, 0),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }

    //--------------------------------------------------------
    // Текст найденного штрих-кода по центру экрана.
    // Показывается только если код внутри основной рамки.
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
    // Верхнее временное сообщение.
    // Сейчас используется для реакции на кнопку "Ввод".
    //--------------------------------------------------------
    void drawTopMessage()
    {
        if (topMessageFrames <= 0) {
            return;
        }

        int frameWidth = displayFrame.cols;
        int frameHeight = displayFrame.rows;
        int msgFontHeight = static_cast<int>(frameHeight * 0.03);

        Rect msgBg(
            frameWidth / 2 - static_cast<int>(frameWidth * 0.22),
            static_cast<int>(frameHeight * 0.05),
            static_cast<int>(frameWidth * 0.44),
            msgFontHeight + 30
        );

        rectangle(displayFrame, msgBg, Scalar(0, 0, 0), FILLED);
        rectangle(displayFrame, msgBg, Scalar(255, 255, 255), 2);

        GDIFrameRenderer textRenderer(displayFrame);
        textRenderer.drawTextFit(
            topMessage,
            msgBg,
            msgFontHeight,
            RGB(255, 255, 255),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );

        topMessageFrames--;
    }

    //--------------------------------------------------------
    // Обработка клика мыши по элементам интерфейса.
    // Логика:
    // - если нажата кнопка Назад -> выходим из экрана
    // - если нажата кнопка Ввод -> показываем сообщение
    //--------------------------------------------------------
    void onMouse(int event, int x, int y)
    {
        if (event != EVENT_LBUTTONDOWN) {
            return;
        }

        if (backButton.contains(x, y)) {
            shouldExit = true;
        }
        else if (inputButton.contains(x, y)) {
            topMessage = L"Ручной ввод пока не реализован";
            topMessageFrames = 120;
            cout << "Manual input is not implemented yet" << endl;
        }
    }

    //--------------------------------------------------------
    // Статический адаптер для OpenCV mouse callback.
    // OpenCV требует обычную функцию, поэтому через userdata
    // мы передаём указатель на текущий объект FrameScreen.
    //--------------------------------------------------------
    static void onMouseStatic(int event, int x, int y, int flags, void* userdata)
    {
        FrameScreen* self = static_cast<FrameScreen*>(userdata);
        if (self) {
            self->onMouse(event, x, y);
        }
    }

    //--------------------------------------------------------
    // Рисование декоративных уголков основной рамки.
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

private:
    //--------------------------------------------------------
    // Поля FrameScreen:
    //
    // cap                 - объект камеры
    // windowName          - имя окна OpenCV
    //
    // cameraFrame         - сырой кадр с камеры
    // gray                - grayscale версия кадра для ZXing
    // displayFrame        - кадр, который реально показывается на экране
    //
    // screenWidth         - ширина экрана
    // screenHeight        - высота экрана
    //
    // scaleX, scaleY      - коэффициенты масштабирования от cameraFrame к displayFrame
    //
    // backButton          - кнопка "Назад"
    // inputButton         - кнопка "Ввод"
    //
    // baseFrame           - основная зона сканирования
    // detectedRect        - прямоугольник найденного штрих-кода
    //
    // shouldExit          - флаг завершения экрана
    // codeInsideMainFrame - флаг: код находится внутри основной рамки
    //
    // detectedText        - текст распознанного штрих-кода
    // topMessage          - верхнее временное сообщение
    // topMessageFrames    - сколько кадров ещё показывать topMessage
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
    Button inputButton;

    Rect baseFrame;
    Rect detectedRect;

    bool shouldExit = false;
    bool codeInsideMainFrame = false;

    string detectedText;
    wstring topMessage = L"";
    int topMessageFrames = 0;
};

//------------------------------------------------------------
// Точка входа в программу.
// Создаёт объект экрана, инициализирует его и запускает цикл.
//------------------------------------------------------------
int main()
{
    FrameScreen screen;

    if (!screen.init()) {
        return 1;
    }

    screen.run();
    return 0;
}