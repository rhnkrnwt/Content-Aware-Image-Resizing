#pragma once

#include <algorithm>

#include <glm\glm.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>

#include "disjoint_set.h"
#include "graph.h"
#include "omp.h"

namespace ContentAwareRetargeting {

  void BuildGraphFromImage(const cv::Mat &image, Graph<glm::vec2> &target_graph) {
    target_graph = Graph<glm::vec2>();

    target_graph.vertices_.reserve(image.size().width * image.size().height);
    target_graph.edges_.reserve(image.size().width * image.size().height * 4);

    const int DELTA_R[4] = {0, 1, 1, -1};
    const int DELTA_C[4] = {1, 0, 1, 1};

    for (int r = 0; r < image.size().height; ++r) {
      for (int c = 0; c < image.size().width; ++c) {
        target_graph.vertices_.push_back(glm::vec2(c, r));

        size_t index = r * image.size().width + c;

        for (size_t direction = 0; direction < 4; ++direction) {
          int neighbor_r = r + DELTA_R[direction];
          int neighbor_c = c + DELTA_C[direction];
          if (neighbor_r >= 0 && neighbor_r < image.size().height && neighbor_c >= 0 && neighbor_c < image.size().width) {
            size_t neighbor_index = neighbor_r * image.size().width + neighbor_c;
            std::pair<size_t, size_t> e(index, neighbor_index);

            double w[3];
            for (size_t pixel_index = 0; pixel_index < 3; ++pixel_index) {
              w[pixel_index] = image.at<cv::Vec3b>(r, c).val[pixel_index] - image.at<cv::Vec3b>(neighbor_r, neighbor_c).val[pixel_index];
            }

            double edge_weight = sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);

            target_graph.edges_.push_back(Edge(e, edge_weight));
          }
        }
      }
    }
  }

  cv::Mat Segmentation(const cv::Mat &image, Graph<glm::vec2> &target_graph, std::vector<std::vector<int> > &group_of_pixel, const double k, const int min_patch_size, const double similar_color_patch_merge_threshold) {

    BuildGraphFromImage(image, target_graph);

    sort(target_graph.edges_.begin(), target_graph.edges_.end());

    DisjointSet vertex_disjoint_set(target_graph.vertices_.size());

    std::vector<double> thresholds(target_graph.vertices_.size(), 1 / k);

    // Segmentation
    for (const auto &edge : target_graph.edges_) {
      size_t group_of_x = vertex_disjoint_set.FindGroup(edge.edge_indices_pair_.first);
      size_t group_of_y = vertex_disjoint_set.FindGroup(edge.edge_indices_pair_.second);
      if (group_of_x == group_of_y) {
        continue;
      }
      if (edge.weight_ <= std::min(thresholds[group_of_x], thresholds[group_of_y])) {
        vertex_disjoint_set.UnionGroup(group_of_x, group_of_y);
        thresholds[group_of_x] = edge.weight_ + (k / vertex_disjoint_set.SizeOfGroup(edge.edge_indices_pair_.first));
      }
    }

    // Deal with the smaller set
    for (const auto &edge : target_graph.edges_) {
      size_t group_of_x = vertex_disjoint_set.FindGroup(edge.edge_indices_pair_.first);
      size_t group_of_y = vertex_disjoint_set.FindGroup(edge.edge_indices_pair_.second);
      if (group_of_x == group_of_y) {
        continue;
      }
      if (min_patch_size > std::min(vertex_disjoint_set.SizeOfGroup(group_of_x), vertex_disjoint_set.SizeOfGroup(group_of_y))) {
        vertex_disjoint_set.UnionGroup(group_of_x, group_of_y);
      }
    }

    // Calculate the color of each group
    std::vector<cv::Vec3d> group_color(target_graph.vertices_.size());
    for (int r = 0; r < image.size().height; ++r) {
      for (int c = 0; c < image.size().width; ++c) {
        size_t index = r * image.size().width + c;
        size_t group = vertex_disjoint_set.FindGroup(index);
        size_t group_size = vertex_disjoint_set.SizeOfGroup(group);
        if (group_size) {
          for (size_t pixel_index = 0; pixel_index < 3; ++pixel_index) {
            group_color[group].val[pixel_index] += image.at<cv::Vec3b>(r, c).val[pixel_index] / (double)group_size;
          }
        }
      }
    }

    // Deal with the similar color set
    for (const auto &edge : target_graph.edges_) {
      size_t group_of_x = vertex_disjoint_set.FindGroup(edge.edge_indices_pair_.first);
      size_t group_of_y = vertex_disjoint_set.FindGroup(edge.edge_indices_pair_.second);
      if (group_of_x == group_of_y) {
        continue;
      }
      double difference[3];
      for (size_t pixel_index = 0; pixel_index < 3; ++pixel_index) {
        difference[pixel_index] = group_color[group_of_x].val[pixel_index] - group_color[group_of_y].val[pixel_index];
      }
      double color_difference = sqrt(difference[0] * difference[0] + difference[1] * difference[1] + difference[2] * difference[2]);
      if (color_difference < similar_color_patch_merge_threshold) {
        vertex_disjoint_set.UnionGroup(group_of_x, group_of_y);
      }
    }

    // Calculate the color of each group again
    for (auto &color : group_color) {
      for (size_t pixel_index = 0; pixel_index < 3; ++pixel_index) {
        color.val[pixel_index] = 0;
      }
    }

    for (int r = 0; r < image.size().height; ++r) {
      for (int c = 0; c < image.size().width; ++c) {
        size_t index = r * image.size().width + c;
        size_t group = vertex_disjoint_set.FindGroup(index);
        size_t group_size = vertex_disjoint_set.SizeOfGroup(group);
        if (group_size) {
          for (size_t pixel_index = 0; pixel_index < 3; ++pixel_index) {
            group_color[group].val[pixel_index] += image.at<cv::Vec3b>(r, c).val[pixel_index] / (double)group_size;
          }
        }
      }
    }

    cv::Mat image_after_segmentation = image.clone();

    // Write the pixel value
    group_of_pixel = std::vector<std::vector<int> >(image.size().height, std::vector<int>(image.size().width));

#pragma omp parallel for
    for (int r = 0; r < image.size().height; ++r) {
#pragma omp parallel for
      for (int c = 0; c < image.size().width; ++c) {
        size_t index = r * image.size().width + c;
        size_t group = vertex_disjoint_set.FindGroup(index);
        group_of_pixel[r][c] = group;
        for (size_t pixel_index = 0; pixel_index < 3; ++pixel_index) {
          image_after_segmentation.at<cv::Vec3b>(r, c).val[pixel_index] = (unsigned char)group_color[group].val[pixel_index];
        }
      }
    }

    return image_after_segmentation;
  }

}