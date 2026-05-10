#include "generate.hpp"

cv::Mat render_code(ZXing::MultiFormatWriter& writer, std::string& data)
{
    auto matrix = writer.encode(data, 300, 100);
    
    int SCALE = 20;
    int imgWidth = matrix.width() * SCALE;
    int imgHeight = matrix.height() * SCALE;
    
    cv::Mat barcodeImage(imgHeight, imgWidth, CV_8UC3, cv::Scalar(255, 255, 255));
    
    for (int y = 0; y < matrix.height(); ++y) {
        for (int x = 0; x < matrix.width(); ++x) {
            if (matrix.get(x, y)) {
                cv::rectangle(barcodeImage,
                    cv::Point(x * SCALE, y * SCALE),
                    cv::Point((x + 1) * SCALE, (y + 1) * SCALE),
                    cv::Scalar(0, 0, 0),
                    cv::FILLED);
            }
        }
    }
    
    return barcodeImage;
}

void output_file(cv::Mat& barcodeImage, std::string& data)
{
    std::string outputFilename = "barcode.png";
    cv::imwrite(outputFilename, barcodeImage);
    std::cout << "Штрихкод успешно сохранен в файл: " << outputFilename << std::endl;
}

void generate(std::string& data) 
{
    try {
		// конструктор
        ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::Code128);
        
        // генерация и рендеринг
        cv::Mat barcodeImage = render_code(writer, data);
        
        // сохранение
        output_file(barcodeImage, data);
        
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
}