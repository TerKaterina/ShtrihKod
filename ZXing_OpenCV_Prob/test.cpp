#include <iostream>
#include <opencv2/opencv.hpp>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ImageView.h>

using namespace std;
using namespace cv;

int main() // вывод на английском, тк на русском почему-то не получилось
{
    cout << "=== OpenCV check ===" << endl;
    cout << "OpenCV version: " << CV_VERSION << endl;

    // создаём чёрную картинку 400x200 с 3 цветовыми каналами и сохраняем
    Mat img = Mat::zeros(200, 400, CV_8UC3);
    putText(img, "OpenCV OK", Point(60, 100),
        FONT_HERSHEY_SIMPLEX, 1.2, Scalar(255, 255, 255), 2);
    imwrite("test.jpg", img);
    cout << "Created file: test.jpg" << endl;

    cout << "\n=== ZXing check ===" << endl;

    // создаём пустую чёрно-белую картинку 100x100
    Mat gray = Mat::zeros(100, 100, CV_8UC1);
    ZXing::ImageView imageView(
        gray.data,              // данные изображения
        gray.cols,              // ширина изображения
        gray.rows,              // высота изображения
        ZXing::ImageFormat::Lum // формат изображения: яркость
    );

    auto result = ZXing::ReadBarcode(imageView); // пытаемся найти штрих-код на картинке

    cout << "ZXing connected, test call completed." << endl;

    if (result.isValid()) {
        // код найден
        cout << "Barcode found: " // выводим найденный текст
            << result.text() << endl;
    }
    else {
        // код не найден
        cout << "No barcode found." << endl;
    }

    cout << "\nEverything is connected and working." << endl;

    return 0;
}