#include <iostream>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/BitMatrix.h>
#include <ZXing/CharacterSet.h>

int main() {
	try {
		//ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::Code128);
		/*
		ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::QRCode);

		writer.setEncoding(ZXing::CharacterSet::UTF8);
		writer.setMargin(1);

		std::string text = "https://lichess.org";
		//auto matrix = writer.encode("1234567890", 300, 80);
		auto matrix = writer.encode(text, 0, 0);

		for (int y = 0; y < matrix.height(); ++y) {
			for (int x = 0; x < matrix.width(); ++x) {
				std::cout << (matrix.get(x, y) ? "  " : "██");
			}
			std::cout << std::endl;
		}*/

		ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::Code128);

		writer.setEncoding(ZXing::CharacterSet::UTF8);
		writer.setMargin(10);

		std::string data = "1234567890";

		auto matrix = writer.encode(data, 0, 0);

		const int REPEAT_ROW = 5; // Печатаем каждую строку 5 раз
        
        	for (int y = 0; y < matrix.height(); ++y) {
            		// Печатаем одну и ту же строку несколько раз, делая полосы "жирнее" по вертикали
            		for (int r = 0; r < REPEAT_ROW; ++r) {
                		for (int x = 0; x < matrix.width(); ++x) {
                    			// Используем пробел и решетку. 
                    			// Важно: Для линейного кода достаточно ОДНОГО символа на модуль.
                    			std::cout << (matrix.get(x, y) ? '#' : ' ');
                		}
                		std::cout << std::endl;
            		}
        	}		


	} catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
	return 0;
}
