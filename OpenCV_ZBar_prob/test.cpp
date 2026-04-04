#include <opencv2/opencv.hpp>
#include <zbar.h>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    cout << "=== Проверка OpenCV ===" << endl;
    cout << "Версия: " << CV_VERSION << endl;
    
    // Создаём и сохраняем картинку
    Mat img = Mat::zeros(200, 300, CV_8UC3);
    rectangle(img, Point(50,50), Point(250,150), Scalar(0,255,0), 3);
    putText(img, "OpenCV OK", Point(100, 180), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255,255,255), 2);
    imwrite("test.jpg", img);
    cout << "Создан файл: test.jpg" << endl;
    
    cout << "\n=== Проверка Zbar ===" << endl;
    
    // Получаем версию Zbar правильно
    unsigned int major, minor, patch;
    zbar::zbar_version(&major, &minor, &patch);
    cout << "Версия Zbar: " << major << "." << minor << "." << patch << endl;
    
    // Проверяем создание сканера
    zbar::ImageScanner scanner;
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
    cout << "Zbar работает!" << endl;
    
    cout << "\n✅ ВСЁ РАБОТАЕТ!" << endl;
    
    return 0;
}
