#include <opencv2/opencv.hpp>
#include <zbar.h>
#include <iostream>
#include <vector> // Обязательно для std::vector

// Рекомендуется избегать using namespace в заголовках и больших проектах,
// но для простого примера оставим как есть.
using namespace cv;
using namespace std;
using namespace zbar;

int main() {
    // 1. Создаём сканер и включаем распознавание всех типов кодов
    ImageScanner scanner;
    scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);

    // 2. Загружаем изображение (путь исправлен на корректный)
    Mat img = imread("C:/Users/neval/OneDrive/Изображения/stankin2.png", IMREAD_COLOR);
    if (img.empty()) { // Используем img.empty() как более современный способ проверки
        cout << "Не удалось загрузить изображение!" << endl;
        return -1;
    }

    // 3. Преобразуем изображение в градации серого (формат, который понимает ZBar)
    Mat imgGray;
    cvtColor(img, imgGray, COLOR_BGR2GRAY);

    // 4. Получаем параметры изображения
    int width = imgGray.cols;
    int height = imgGray.rows;

    // 5. Создаем КОПИЮ данных изображения для безопасной передачи в ZBar.
    // Это критически важно, так как ZBar не копирует данные, а использует указатель.
    // Если Mat будет уничтожен или изменен, ZBar обратится к несуществующей памяти.
    std::vector<uchar> buffer(imgGray.data, imgGray.data + imgGray.total());

    // 6. Оборачиваем изображение в объект ZBar
    // Используем данные из нашего буфера (buffer.data()), а не сырой указатель (imgGray.data).
    Image imageZbar(width, height, "Y800", buffer.data(), width * height);

    // 7. Сканируем изображение на наличие символов
    scanner.scan(imageZbar);

    // 8. Выводим результаты поиска
    for (Image::SymbolIterator symbol = imageZbar.symbol_begin();
        symbol != imageZbar.symbol_end(); ++symbol) {
        cout << "Тип: " << symbol->get_type_name() << endl;
        cout << "Данные: " << symbol->get_data() << endl << endl;
    }

    // 9. Отображаем исходное изображение (опционально)
    imshow("Display window", img);
    waitKey(0); // Ждем нажатия клавиши

    return 0;
}