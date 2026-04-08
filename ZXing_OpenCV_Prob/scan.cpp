#include <iostream>
#include <opencv2/opencv.hpp>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ImageView.h>

using namespace std;
using namespace cv;

int main()
{
    cout << "=== Camera barcode scanner ===" << endl;

    VideoCapture cap(0); // открываем первую камеру

    if (!cap.isOpened()) {
        cout << "Failed to open camera." << endl;
        return 1;
    }

    cout << "Camera opened successfully." << endl;
    cout << "Press ESC to exit." << endl;

    Mat frame;
    Mat gray;
    string lastText = "";

    while (true) {
        cap >> frame; // читаем кадр с камеры

        if (frame.empty()) {
            cout << "Empty frame received." << endl;
            break;
        }

        // переводим кадр в grayscale для распознавания
        cvtColor(frame, gray, COLOR_BGR2GRAY);

        ZXing::ImageView imageView(
            gray.data,              // данные изображения
            gray.cols,              // ширина
            gray.rows,              // высота
            ZXing::ImageFormat::Lum // grayscale-формат
        );

        auto result = ZXing::ReadBarcode(imageView); // пытаемся найти код

        if (result.isValid()) {
            string text = result.text();

            // чтобы не спамить одинаковым кодом каждый кадр
            if (text != lastText) {
                cout << "Barcode found: " << text << endl;
                lastText = text;
            }

            // выводим найденный текст поверх изображения
            putText(frame, text, Point(30, 40),
                FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
        }

        imshow("Barcode Scanner", frame); // показываем окно

        // выход только по клавише ESC
        if (waitKey(1) == 27) {
            break;
        }
    }

    cap.release();
    destroyAllWindows();

    return 0;
}