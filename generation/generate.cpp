#include "generate.hpp"

void generate(std::string data) {
	try {
		ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::Code128);

		writer.setEncoding(ZXing::CharacterSet::UTF8);
		writer.setMargin(20);

		//std::string data = "1234567890";

		auto matrix = writer.encode(data, 300, 100);

		const int SCALE = 20;

		int imgWidth = matrix.width() * SCALE;
        	int imgHeight = matrix.height() * SCALE;
        	cv::Mat barcodeImage(imgHeight, imgWidth, CV_8UC3, cv::Scalar(255, 255, 255)); // Белый фон
        
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
		std::string outputFilename = "barcode.png";
		cv::imwrite(outputFilename, barcodeImage);
		std::cout << "Штрихкод успешно сохранен в файл: " << outputFilename << std::endl;

		// Опционально: показать изображение в окне
		// cv::imshow("Сгенерированный штрихкод", barcodeImage);
		// cv::waitKey(0);


	} catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
}
