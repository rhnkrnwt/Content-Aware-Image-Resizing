#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <queue>
#include <vector>

#include <opencv\cv.hpp>

namespace ContentAwareRetargeting {

  const std::string SEGMENT_TREE_PROGRAM_PATH = "..\\program\\video_segmentation\\seg_tree_sample.exe";
  const std::string SEGMENT_CONVERTER_PROGRAM_PATH = "..\\program\\video_segmentation\\segment_converter.exe";

  void GroundTruthData() {
    //for (int i = 1; i <= 132; ++i) {
    //  std::ostringstream oss;
    //  oss << "..\\temp\\svg\\source-cut-10-S" << i << ".svg";
    //  std::string file_name = oss.str();
    //  std::fstream fs(file_name);
    //  size_t n;
    //  fs >> n;

    //  cv::Mat result(cv::Size(688, 286), CV_8UC3);

    //  for (int r = 0; r < 286; ++r) {
    //    for (int c = 0; c < 688; ++c) {
    //      fs >> n;

    //      result.at<cv::Vec3b>(r, c).val[0] = n % 256;
    //      result.at<cv::Vec3b>(r, c).val[1] = (n / 256) % 256;
    //      result.at<cv::Vec3b>(r, c).val[2] = n / 65536;
    //    }
    //  }

    //  std::ostringstream ross;
    //  ross << "..\\temp\\svg\\segmentation_frame" << std::setw(5) << std::setfill('0') << i - 1 << "_morto.avi.png";
    //  cv::imwrite(ross.str(), result);
    //}

    for (int i = 1; i <= 132; ++i) {
      std::ostringstream oss;
      oss << "..\\temp\\saliency\\" << i << ".jpg";
      std::cout << oss.str() << "\n";
      cv::Mat input;
      input = cv::imread(oss.str());
      cv::resize(input, input, cv::Size(688, 286));

      std::ostringstream ross;
      ross << "..\\temp\\saliency\\res\\saliency_frame" << std::setw(5) << std::setfill('0') << i - 1 << "_morto.avi.png";
      std::cout << ross.str() << "\n";
      cv::imwrite(ross.str(), input);
    }

  }

  //void VideoSegmentation(const std::string &video_file_path) {
  //}

  void VideoSegmentation(const std::string &video_file_directory, const std::string &video_file_name) {

    if (!std::fstream(video_file_directory + video_file_name + ".pb").good()) {
      std::string segment_tree_program_command = SEGMENT_TREE_PROGRAM_PATH;
      segment_tree_program_command += " --input_file=\"" + video_file_directory + video_file_name + "\"";
      segment_tree_program_command += " --run_on_server";
      segment_tree_program_command += " --write_to_file";
      std::cout << segment_tree_program_command << "\n";
      system(segment_tree_program_command.c_str());
    }

    std::string segment_converter_command = SEGMENT_CONVERTER_PROGRAM_PATH;
    segment_converter_command += " --input=\"" + video_file_directory + video_file_name + ".pb\"";
    segment_converter_command += " --bitmap_ids=0.0";
    //segment_converter_command += " --bitmap_color=0.1";
    //segment_converter_command += " --text_format";
    if (video_file_directory[video_file_directory.length() - 1] == '\\') {
      segment_converter_command += " --output_dir=\"" + video_file_directory.substr(0, video_file_directory.length() - 1) + "\"";
    } else {
      segment_converter_command += " --output_dir=\"" + video_file_directory + "\"";
    }
    std::cout << segment_converter_command << "\n";
    system(segment_converter_command.c_str());

    for (size_t frame_count = 0; ; ++frame_count) {
      std::ostringstream old_frame_segmentation_name_oss;
      old_frame_segmentation_name_oss << "frame" << std::setw(5) << std::setfill('0') << frame_count << ".png";

      if (!std::fstream(video_file_directory + old_frame_segmentation_name_oss.str()).good()) {
        break;
      }

      std::ostringstream video_frame_segmentation_name_oss;
      video_frame_segmentation_name_oss << "segmentation_frame" << std::setw(5) << std::setfill('0') << frame_count << "_" + video_file_name + ".png";

      std::string rename_old_frame_file_command = "rename \"" + video_file_directory + old_frame_segmentation_name_oss.str() + "\" \"" + video_frame_segmentation_name_oss.str() + "\"";
      system(rename_old_frame_file_command.c_str());
    }
  }

  // Impossible to save all frames of a normal length video
  // Just a brute force method for testing
  size_t CalculateGroupFromSegmentedVideo(const std::vector<cv::Mat> &segmented_frames, std::vector<std::vector<std::vector<int> > > &group_of_pixel) {
    if (!segmented_frames.size()) {
      return 0;
    }

    group_of_pixel = std::vector<std::vector<std::vector<int> > >(segmented_frames.size(), std::vector<std::vector<int> >(segmented_frames[0].rows, std::vector<int>(segmented_frames[0].cols, -1)));

    struct State {

      State(size_t t, size_t r, size_t c) : t_(t), r_(r), c_(c) {
      }

      size_t t_, r_, c_;
    };

    // BFS, pixels with the same color are the same object

    size_t group_count = 0;
    for (size_t t = 0; t < segmented_frames.size(); ++t) {
      for (size_t r = 0; r < segmented_frames[t].rows; ++r) {
        for (size_t c = 0; c < segmented_frames[t].cols; ++c) {
          if (group_of_pixel[t][r][c] >= 0) {
            continue;
          }

          std::queue<State> q;
          q.push(State(t, r, c));
          group_of_pixel[t][r][c] = group_count;

          while (!q.empty()) {
            State current_state = q.front();
            q.pop();

            for (size_t dt = 0; dt <= 1; ++dt) {
              for (size_t dr = 0; dr <= 1; ++dr) {
                for (size_t dc = 0; dc <= 1; ++dc) {
                  if ((dt + dr + dc) != 1) {
                    continue;
                  }
                  State next_state = State(current_state.t_ + dt, current_state.r_ + dr, current_state.c_ + dc);
                  if (next_state.t_ < segmented_frames.size() && next_state.r_ < segmented_frames[next_state.t_].rows && next_state.c_ < segmented_frames[next_state.t_].cols) {
                    if (group_of_pixel[next_state.t_][next_state.r_][next_state.c_] >= 0) {
                      continue;
                    }
                    if (segmented_frames[next_state.t_].at<cv::Vec3b>(next_state.r_, next_state.c_) != segmented_frames[current_state.t_].at<cv::Vec3b>(current_state.r_, current_state.c_)) {
                      continue;
                    }
                    group_of_pixel[next_state.t_][next_state.r_][next_state.c_] = group_count;
                    q.push(next_state);
                  }
                }
              }

            }

          }

          ++group_count;
        }
      }
    }

    return group_count;
  }

}