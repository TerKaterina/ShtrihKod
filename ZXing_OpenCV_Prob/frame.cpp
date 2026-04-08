#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ImageView.h>
#include <ZXing/Barcode.h>

using namespace std;
using namespace cv;

struct Button {
    Rect rect;
    string text;
};

Button backButton;
Button inputButton;
bool shouldExit = false;

string topMessage = "";
int topMessageFrames = 0;

// функция для проверки: попала ли точка внутрь прямоугольника
bool isInsideRect(const Rect& rect, int x, int y)
{
    return x >= rect.x && x <= rect.x + rect.width &&
        y >= rect.y && y <= rect.y + rect.height;
}

// функция для проверки: лежит ли весь прямоугольник внутри другого
bool isRectInside(const Rect& outer, const Rect& inner)
{
    return inner.x >= outer.x &&
        inner.y >= outer.y &&
        inner.x + inner.width <= outer.x + outer.width &&
        inner.y + inner.height <= outer.y + outer.height;
}

// обработчик кликов мыши
void onMouse(int event, int x, int y, int flags, void* userdata)
{
    if (event != EVENT_LBUTTONDOWN)
        return;

    if (isInsideRect(backButton.rect, x, y)) {
        shouldExit = true;
    }
    else if (isInsideRect(inputButton.rect, x, y)) {
        topMessage = "Manual input is not implemented yet";
        topMessageFrames = 120;
        cout << topMessage << endl;
    }
}

// рисование кнопки
void drawButton(Mat& frame, const Button& button, double fontScale, int thickness)
{
    rectangle(frame, button.rect, Scalar(255, 255, 255), FILLED);
    rectangle(frame, button.rect, Scalar(0, 0, 0), 2);

    int baseline = 0;
    Size textSize = getTextSize(button.text, FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseline);

    int textX = button.rect.x + (button.rect.width - textSize.width) / 2;
    int textY = button.rect.y + (button.rect.height + textSize.height) / 2;

    putText(frame, button.text, Point(textX, textY),
        FONT_HERSHEY_SIMPLEX, fontScale, Scalar(0, 0, 0), thickness);
}

// рисование декоративных уголков базовой рамки
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

int main()
{
    cout << "=== Frame scanner UI ===" << endl;

    VideoCapture cap(0);

    if (!cap.isOpened()) {
        cout << "Failed to open camera." << endl;
        return 1;
    }

    string windowName = "Frame Scanner";
    namedWindow(windowName, WINDOW_NORMAL);
    setWindowProperty(windowName, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    setMouseCallback(windowName, onMouse);

    Mat frame;
    Mat gray;

    while (true) {
        cap >> frame;

        if (frame.empty()) {
            cout << "Empty frame received." << endl;
            break;
        }

        int frameWidth = frame.cols;
        int frameHeight = frame.rows;

        int frameBoxWidth = static_cast<int>(frameWidth * 0.65);
        int frameBoxHeight = static_cast<int>(frameHeight * 0.50);
        int frameBoxX = (frameWidth - frameBoxWidth) / 2;
        int frameBoxY = static_cast<int>(frameHeight * 0.16);

        Rect baseFrame(frameBoxX, frameBoxY, frameBoxWidth, frameBoxHeight);

        int buttonWidth = static_cast<int>(frameWidth * 0.20);
        int buttonHeight = static_cast<int>(frameHeight * 0.08);
        int buttonY = static_cast<int>(frameHeight * 0.85);
        int sideMargin = static_cast<int>(frameWidth * 0.10);

        backButton = { Rect(sideMargin, buttonY, buttonWidth, buttonHeight), "Back" };
        inputButton = { Rect(frameWidth - sideMargin - buttonWidth, buttonY, buttonWidth, buttonHeight), "Input" };

        cvtColor(frame, gray, COLOR_BGR2GRAY);

        ZXing::ImageView imageView(
            gray.data,
            gray.cols,
            gray.rows,
            ZXing::ImageFormat::Lum
        );

        auto result = ZXing::ReadBarcode(imageView);

        // затемнение вне базовой рамки
        Mat darkened;
        frame.copyTo(darkened);
        darkened.convertTo(darkened, -1, 0.35, 0);
        frame(baseFrame).copyTo(darkened(baseFrame));
        frame = darkened;

        // базовая рамка
        rectangle(frame, baseFrame, Scalar(0, 0, 0), 2);
        drawFrameCorners(frame, baseFrame);

        string hintText = "Place the code inside the frame";
        double hintScale = max(0.7, frameHeight / 900.0);
        int hintThickness = 2;
        int hintBaseline = 0;
        Size hintSize = getTextSize(hintText, FONT_HERSHEY_SIMPLEX, hintScale, hintThickness, &hintBaseline);

        int hintX = (frameWidth - hintSize.width) / 2;
        int hintY = baseFrame.y + baseFrame.height - 20;

        putText(frame, hintText, Point(hintX, hintY),
            FONT_HERSHEY_SIMPLEX, hintScale, Scalar(0, 0, 0), hintThickness);

        bool codeInsideMainFrame = false;
        string detectedText = "";
        Rect detectedRect;

        if (result.isValid()) {
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

            int rectX = static_cast<int>(minX);
            int rectY = static_cast<int>(minY);
            int rectW = static_cast<int>(maxX - minX);
            int rectH = static_cast<int>(maxY - minY);

            if (rectW > 0 && rectH > 0) {
                detectedRect = Rect(rectX, rectY, rectW, rectH);

                if (isRectInside(baseFrame, detectedRect)) {
                    codeInsideMainFrame = true;
                }
            }
        }

        if (codeInsideMainFrame) {
            rectangle(frame, detectedRect, Scalar(0, 255, 0), 3);

            double codeScale = max(0.8, frameHeight / 700.0);
            int codeThickness = 2;
            int codeBaseline = 0;
            Size codeSize = getTextSize(detectedText, FONT_HERSHEY_SIMPLEX, codeScale, codeThickness, &codeBaseline);

            int codeX = (frameWidth - codeSize.width) / 2;
            int codeY = frameHeight / 2;

            Rect textBg(
                codeX - 20,
                codeY - codeSize.height - 15,
                codeSize.width + 40,
                codeSize.height + 30
            );

            rectangle(frame, textBg, Scalar(0, 0, 0), FILLED);
            rectangle(frame, textBg, Scalar(0, 255, 0), 2);

            putText(frame, detectedText, Point(codeX, codeY),
                FONT_HERSHEY_SIMPLEX, codeScale, Scalar(0, 255, 0), codeThickness);
        }

        if (topMessageFrames > 0) {
            double msgScale = max(0.7, frameHeight / 850.0);
            int msgThickness = 2;
            int msgBaseline = 0;
            Size msgSize = getTextSize(topMessage, FONT_HERSHEY_SIMPLEX, msgScale, msgThickness, &msgBaseline);

            int msgX = (frameWidth - msgSize.width) / 2;
            int msgY = static_cast<int>(frameHeight * 0.08);

            Rect msgBg(
                msgX - 20,
                msgY - msgSize.height - 15,
                msgSize.width + 40,
                msgSize.height + 30
            );

            rectangle(frame, msgBg, Scalar(0, 0, 0), FILLED);
            rectangle(frame, msgBg, Scalar(255, 255, 255), 2);

            putText(frame, topMessage, Point(msgX, msgY),
                FONT_HERSHEY_SIMPLEX, msgScale, Scalar(255, 255, 255), msgThickness);

            topMessageFrames--;
        }

        double buttonFontScale = max(0.8, frameHeight / 900.0);
        int buttonThickness = 2;

        drawButton(frame, backButton, buttonFontScale, buttonThickness);
        drawButton(frame, inputButton, buttonFontScale, buttonThickness);

        imshow(windowName, frame);

        if (shouldExit) {
            break;
        }

        waitKey(1);
    }

    cap.release();
    destroyAllWindows();

    return 0;
}