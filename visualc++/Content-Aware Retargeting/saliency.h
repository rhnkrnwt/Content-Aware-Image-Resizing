#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <opencv\cv.hpp>

#include "omp.h"

namespace ContentAwareRetargeting {

  const std::string SALIENCY_PROGRAM_PATH = "..\\program\\saliency\\saliency.exe";

  cv::Mat CalculateContextAwareSaliencyMapWithMatlabProgram(const cv::Mat &image, std::vector<std::vector<double> > &saliency_map, const std::string &input_image_name, const std::string &output_image_path) {
    std::string saliency_program_command = SALIENCY_PROGRAM_PATH;
    saliency_program_command += std::string(" ");
    saliency_program_command += "\"" + input_image_name + "\"";
    saliency_program_command += std::string(" ");
    saliency_program_command += "\"" + output_image_path + "\"";

    if (std::fstream(output_image_path).good()) {
      std::cout << "The saliency image " + output_image_path + " was already generated.\n";
    } else {
      std::cout << saliency_program_command << "\n";
      system(saliency_program_command.c_str());
    }

    //cv::Mat saliency_map_test;
    //cv::Ptr<cv::saliency::Saliency> saliency_algorithm = cv::saliency::Saliency::create("SPECTRAL_RESIDUAL");
    //saliency_algorithm->computeSaliency(image, saliency_map_test);

    //cv::Mat saliency_image_test;
    //cv::saliency::StaticSaliencySpectralResidual spec;
    //spec.computeSaliency(saliency_map_test, saliency_image_test);
    //cv::imshow("Saliency Map", saliency_map_test);
    //cv::imshow("Saliency image", saliency_image_test);

    cv::Mat saliency_image = cv::imread(output_image_path);
    cv::resize(saliency_image, saliency_image, image.size());

    cv::imwrite(output_image_path, saliency_image);

    saliency_map = std::vector<std::vector<double> >(image.size().height, std::vector<double>(image.size().width, 0));

#pragma omp parallel for
    for (int r = 0; r < image.size().height; ++r) {
#pragma omp parallel for
      for (int c = 0; c < image.size().width; ++c) {
        for (size_t pixel_index = 0; pixel_index < 3; ++pixel_index) {
          saliency_map[r][c] += saliency_image.at<cv::Vec3b>(r, c).val[pixel_index];
        }
        saliency_map[r][c] /= 3;
        saliency_map[r][c] /= 255.0;

        // Enlarge difference
        //saliency_map[r][c] = pow(saliency_map[r][c], 1.5);
      }
    }

    return saliency_image;
  }

}