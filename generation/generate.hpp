#ifndef GENERATE_HPP
#define GENERATE_HPP

#include <iostream>
#include <opencv2/opencv.hpp>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/BitMatrix.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/CharacterSet.h>
#include <string>

cv::Mat render_code(ZXing::MultiFormatWriter& writer, std::string& data);
void output_file(cv::Mat& barcodeImage, std::string& data);
void generate(std::string& data);

#endif //GENERATE_HPP
