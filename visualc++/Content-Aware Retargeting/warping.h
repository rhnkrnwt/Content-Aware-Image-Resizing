#pragma once

#define IL_STD

#include <vector>
#include <algorithm>
#include <set>
#include <map>

#include <ilcplex\ilocplex.h>
#include <ilconcert\iloexpression.h>
#include <opencv\cv.hpp>

#include "graph.h"

namespace ContentAwareRetargeting {

  size_t Vec3bToValue(const cv::Vec3b &color) {
    return color.val[0] * 256 * 256 + color.val[1] * 256 + color.val[2];
  }

  double SignifanceColorToSaliencyValue(cv::Vec3b &signifance_color) {

    if (signifance_color[2] >= 255) {
      return (1.0 - (signifance_color[1] / 255.0)) / 3.0 + (2 / 3.0);
    }

    if (signifance_color[1] >= 255) {
      return (signifance_color[2] / 255.0) / 3.0 + (1 / 3.0);
    }

    return (signifance_color[1] / 255.0) / 3.0;
  }

  void ObjectPreservingVideoWarping(const std::string &source_video_file_directory, const std::string &source_video_file_name, std::vector<Graph<glm::vec2> > &target_graphs, int target_video_width, const int target_video_height, const double mesh_width, const double mesh_height) {
    if (target_video_width <= 0 || target_video_height <= 0) {
      std::cout << "Wrong target video size (" << target_video_width << " x " << target_video_height << ")\n";
      return;
    }

    const double DST_WEIGHT = 5.5;
    const double DLT_WEIGHT = 0.8;
    const double ORIENTATION_WEIGHT = 12.0;

    const double OBJECT_COHERENCE_WEIGHT = 4.0;
    const double LINE_COHERENCE_WEIGHT = 2.0;

    //const double DST_WEIGHT = 0.7;
    //const double DLT_WEIGHT = 0.3;
    //const double ORIENTATION_WEIGHT = 1.0;

    //const double OBJECT_COHERENCE_WEIGHT = 0.7;
    //const double LINE_COHERENCE_WEIGHT = 0.3;

    std::map<size_t, double> object_saliency;
    std::map<size_t, Edge> object_representive_edge;

    std::map<size_t, glm::vec2> object_horizontal_average_deformation_in_the_first_appear_frame;
    std::map<size_t, glm::vec2> object_vertical_average_deformation_in_the_first_appear_frame;

    std::map<size_t, glm::vec2> object_average_deformation_in_the_first_appear_frame;

    for (size_t t = 0; t < target_graphs.size(); ++t) {
      //for (int t = target_graphs.size() - 1; t >= 0; --t) {

      std::ostringstream video_frame_segmentation_name_oss;
      video_frame_segmentation_name_oss << "segmentation_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";

      std::ostringstream video_frame_significance_name_oss;
      video_frame_significance_name_oss << "significance_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";

      if (!std::fstream(source_video_file_directory + video_frame_segmentation_name_oss.str()).good()) {
        break;
      }

      if (!std::fstream(source_video_file_directory + video_frame_significance_name_oss.str()).good()) {
        break;
      }

      cv::Mat segmentation_video_frame = cv::imread(source_video_file_directory + video_frame_segmentation_name_oss.str());

      cv::Mat significance_video_frame = cv::imread(source_video_file_directory + video_frame_significance_name_oss.str());

      const double WIDTH_RATIO = target_video_width / (double)segmentation_video_frame.size().width;
      const double HEIGHT_RATIO = target_video_height / (double)segmentation_video_frame.size().height;

      Graph<glm::vec2> &target_graph = target_graphs[t];

      IloEnv env;

      IloNumVarArray x(env);
      IloExpr expr(env);
      IloRangeArray hard_constraint(env);

      for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
        x.add(IloNumVar(env, -IloInfinity, IloInfinity));
        x.add(IloNumVar(env, -IloInfinity, IloInfinity));
      }

      size_t mesh_column_count = (size_t)(segmentation_video_frame.size().width / mesh_width) + 1;
      size_t mesh_row_count = (size_t)(segmentation_video_frame.size().height / mesh_height) + 1;

      // Boundary constraint
      for (size_t row = 0; row < mesh_row_count; ++row) {
        size_t vertex_index = row * mesh_column_count;
        hard_constraint.add(x[vertex_index * 2] == target_graph.vertices_[0].x);

        vertex_index = row * mesh_column_count + mesh_column_count - 1;
        hard_constraint.add(x[vertex_index * 2] == target_graph.vertices_[0].x + target_video_width);
      }

      for (size_t column = 0; column < mesh_column_count; ++column) {
        size_t vertex_index = column;
        hard_constraint.add(x[vertex_index * 2 + 1] == target_graph.vertices_[0].y);

        vertex_index = (mesh_row_count - 1) * mesh_column_count + column;
        hard_constraint.add(x[vertex_index * 2 + 1] == target_graph.vertices_[0].y + target_video_height);
      }

      // Avoid flipping
      for (size_t row = 0; row < mesh_row_count; ++row) {
        for (size_t column = 1; column < mesh_column_count; ++column) {
          size_t vertex_index_right = row * mesh_column_count + column;
          size_t vertex_index_left = row * mesh_column_count + column - 1;
          //hard_constraint.add((x[vertex_index_right * 2] - x[vertex_index_left * 2]) >= 1e-4);
        }
      }

      for (size_t row = 1; row < mesh_row_count; ++row) {
        for (size_t column = 0; column < mesh_column_count; ++column) {
          size_t vertex_index_down = row * mesh_column_count + column;
          size_t vertex_index_up = (row - 1) * mesh_column_count + column;
          //hard_constraint.add((x[vertex_index_down * 2 + 1] - x[vertex_index_up * 2 + 1]) >= 1e-4);
        }
      }

      // Line coherence
      if (t) {
        for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
          expr += LINE_COHERENCE_WEIGHT * IloPower(x[vertex_index * 2] - target_graphs[t - 1].vertices_[vertex_index].x, 2.0);
          expr += LINE_COHERENCE_WEIGHT * IloPower(x[vertex_index * 2 + 1] - target_graphs[t - 1].vertices_[vertex_index].y, 2.0);
        }
      }

      //if (t < target_graphs.size() - 1) {
      //  for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
      //    expr += LINE_COHERENCE_WEIGHT * IloPower(x[vertex_index * 2] - target_graphs[t + 1].vertices_[vertex_index].x, 2.0);
      //    expr += LINE_COHERENCE_WEIGHT * IloPower(x[vertex_index * 2 + 1] - target_graphs[t + 1].vertices_[vertex_index].y, 2.0);
      //  }
      //}


      // For the boundary pixel
      cv::resize(segmentation_video_frame, segmentation_video_frame, segmentation_video_frame.size() + cv::Size(1, 1));
      cv::resize(significance_video_frame, significance_video_frame, significance_video_frame.size() + cv::Size(1, 1));

      std::map<size_t, std::vector<size_t> > edge_index_list_of_object;
      std::set<size_t> first_appear_group_list;

      // Create edge index list
      for (size_t edge_index = 0; edge_index < target_graph.edges_.size(); ++edge_index) {
        size_t vertex_index_1 = target_graph.edges_[edge_index].edge_indices_pair_.first;
        size_t vertex_index_2 = target_graph.edges_[edge_index].edge_indices_pair_.second;
        size_t group_1 = Vec3bToValue(segmentation_video_frame.at<cv::Vec3b>(target_graph.vertices_[vertex_index_1].y, target_graph.vertices_[vertex_index_1].x));
        size_t group_2 = Vec3bToValue(segmentation_video_frame.at<cv::Vec3b>(target_graph.vertices_[vertex_index_2].y, target_graph.vertices_[vertex_index_2].x));
        
        edge_index_list_of_object[group_1].push_back(edge_index);

        if (object_saliency.find(group_1) == object_saliency.end()) {
          first_appear_group_list.insert(group_1);

          object_saliency[group_1] = SignifanceColorToSaliencyValue(significance_video_frame.at<cv::Vec3b>(target_graph.vertices_[vertex_index_1].y, target_graph.vertices_[vertex_index_1].x));

          object_representive_edge[group_1] = target_graph.edges_[edge_index];
        }

        if (group_1 != group_2) {
          edge_index_list_of_object[group_2].push_back(edge_index);

          if (object_saliency.find(group_2) == object_saliency.end()) {
            first_appear_group_list.insert(group_2);

            object_saliency[group_2] = SignifanceColorToSaliencyValue(significance_video_frame.at<cv::Vec3b>(target_graph.vertices_[vertex_index_2].y, target_graph.vertices_[vertex_index_2].x));

            object_representive_edge[group_2] = target_graph.edges_[edge_index];
          }
        }
      }

      // Average deformation
      for (auto edge_index_list_of_object_iterator = edge_index_list_of_object.begin(); edge_index_list_of_object_iterator != edge_index_list_of_object.end(); ++edge_index_list_of_object_iterator) {
        const std::size_t object_index = edge_index_list_of_object_iterator->first;
        const std::vector<size_t> &edge_index_list = edge_index_list_of_object_iterator->second;

        const double OBJECT_SIZE_WEIGHT = sqrt(1.0 / (double)edge_index_list.size());
        //const double OBJECT_SIZE_WEIGHT = 1.0;

        if (first_appear_group_list.find(object_index) == first_appear_group_list.end()) {
          for (const auto &edge_index : edge_index_list) {
            const Edge &edge = target_graph.edges_[edge_index];
            size_t vertex_index_1 = edge.edge_indices_pair_.first;
            size_t vertex_index_2 = edge.edge_indices_pair_.second;

            const glm::vec2 &horizontal_average_deformation = object_horizontal_average_deformation_in_the_first_appear_frame[object_index];
            const glm::vec2 &vertical_average_deformation = object_vertical_average_deformation_in_the_first_appear_frame[object_index];

            const glm::vec2 &average_deformation = object_average_deformation_in_the_first_appear_frame[object_index];

            float delta_x = target_graph.vertices_[vertex_index_1].x - target_graph.vertices_[vertex_index_2].x;
            float delta_y = target_graph.vertices_[vertex_index_1].y - target_graph.vertices_[vertex_index_2].y;
            if (std::abs(delta_x) > std::abs(delta_y)) { // Horizontal
              expr += OBJECT_SIZE_WEIGHT * OBJECT_COHERENCE_WEIGHT * IloPower((x[vertex_index_1 * 2] - x[vertex_index_2 * 2]) - horizontal_average_deformation.x, 2.0);
              expr += OBJECT_SIZE_WEIGHT * OBJECT_COHERENCE_WEIGHT * IloPower((x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1]) - horizontal_average_deformation.y, 2.0);

              //expr += OBJECT_SIZE_WEIGHT * OBJECT_COHERENCE_WEIGHT * IloPower((x[vertex_index_1 * 2] - x[vertex_index_2 * 2]) - average_deformation.x, 2.0);
            } else {
              expr += OBJECT_SIZE_WEIGHT * OBJECT_COHERENCE_WEIGHT * IloPower((x[vertex_index_1 * 2] - x[vertex_index_2 * 2]) - vertical_average_deformation.x, 2.0);
              expr += OBJECT_SIZE_WEIGHT * OBJECT_COHERENCE_WEIGHT * IloPower((x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1]) - vertical_average_deformation.y, 2.0);

              //expr += OBJECT_SIZE_WEIGHT * OBJECT_COHERENCE_WEIGHT * IloPower((x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1]) - average_deformation.y, 2.0);
            }
          }
        }
      }

      // Object transformation constraint
      for (auto edge_index_list_of_object_iterator = edge_index_list_of_object.begin(); edge_index_list_of_object_iterator != edge_index_list_of_object.end(); ++edge_index_list_of_object_iterator) {
        const std::size_t object_index = edge_index_list_of_object_iterator->first;
        const std::vector<size_t> &edge_index_list = edge_index_list_of_object_iterator->second;

        const double OBJECT_SIZE_WEIGHT = sqrt(1.0 / (double)edge_index_list.size());
        //const double OBJECT_SIZE_WEIGHT = 1.0;

        if (!edge_index_list.size()) {
          continue;
        }

        const Edge &representive_edge = target_graph.edges_[edge_index_list[0]];
        //const Edge &representive_edge = object_representive_edge[object_index];

        double c_x = target_graph.vertices_[representive_edge.edge_indices_pair_.first].x - target_graph.vertices_[representive_edge.edge_indices_pair_.second].x;
        double c_y = target_graph.vertices_[representive_edge.edge_indices_pair_.first].y - target_graph.vertices_[representive_edge.edge_indices_pair_.second].y;

        double original_matrix_a = c_x;
        double original_matrix_b = c_y;
        double original_matrix_c = c_y;
        double original_matrix_d = -c_x;

        double matrix_rank = original_matrix_a * original_matrix_d - original_matrix_b * original_matrix_c;

        if (fabs(matrix_rank) <= 1e-9) {
          matrix_rank = (matrix_rank > 0 ? 1 : -1) * 1e-9;
        }

        double matrix_a = original_matrix_d / matrix_rank;
        double matrix_b = -original_matrix_b / matrix_rank;
        double matrix_c = -original_matrix_c / matrix_rank;
        double matrix_d = original_matrix_a / matrix_rank;

        for (const auto &edge_index : edge_index_list) {
          const Edge &edge = target_graph.edges_[edge_index];

          double e_x = target_graph.vertices_[edge.edge_indices_pair_.first].x - target_graph.vertices_[edge.edge_indices_pair_.second].x;
          double e_y = target_graph.vertices_[edge.edge_indices_pair_.first].y - target_graph.vertices_[edge.edge_indices_pair_.second].y;

          double t_s = matrix_a * e_x + matrix_b * e_y;
          double t_r = matrix_c * e_x + matrix_d * e_y;

          // DST
          expr += OBJECT_SIZE_WEIGHT * DST_WEIGHT * object_saliency[object_index] *
            IloPower((x[edge.edge_indices_pair_.first * 2] - x[edge.edge_indices_pair_.second * 2]) -
              (t_s * (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_r * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
              2);
          expr += OBJECT_SIZE_WEIGHT * DST_WEIGHT * object_saliency[object_index] *
            IloPower((x[edge.edge_indices_pair_.first * 2 + 1] - x[edge.edge_indices_pair_.second * 2 + 1]) -
              (-t_r * (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_s * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
              2);

          // DLT
          expr += OBJECT_SIZE_WEIGHT * DLT_WEIGHT * (1 - object_saliency[object_index]) *
            IloPower((x[edge.edge_indices_pair_.first * 2] - x[edge.edge_indices_pair_.second * 2]) -
              WIDTH_RATIO * (t_s * (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_r * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
              2);
          expr += OBJECT_SIZE_WEIGHT * DLT_WEIGHT * (1 - object_saliency[object_index]) *
            IloPower((x[edge.edge_indices_pair_.first * 2 + 1] - x[edge.edge_indices_pair_.second * 2 + 1]) -
              HEIGHT_RATIO * (-t_r *  (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_s * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
              2);
        }
      }

      // Grid orientation constraint
      for (const auto &edge : target_graph.edges_) {
        size_t vertex_index_1 = edge.edge_indices_pair_.first;
        size_t vertex_index_2 = edge.edge_indices_pair_.second;
        float delta_x = target_graph.vertices_[vertex_index_1].x - target_graph.vertices_[vertex_index_2].x;
        float delta_y = target_graph.vertices_[vertex_index_1].y - target_graph.vertices_[vertex_index_2].y;
        if (std::abs(delta_x) > std::abs(delta_y)) { // Horizontal
          expr += ORIENTATION_WEIGHT * IloPower(x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1], 2);
        } else {
          expr += ORIENTATION_WEIGHT * IloPower(x[vertex_index_1 * 2] - x[vertex_index_2 * 2], 2);
        }
      }

      IloModel model(env);

      model.add(IloMinimize(env, expr));

      model.add(hard_constraint);

      IloCplex cplex(model);

      cplex.setOut(env.getNullStream());

      if (!cplex.solve()) {
        std::cout << "Failed to optimize the model. (frame : " << t << ")\n";
      } else {
        std::cout << "Done. (frame : " << (t + 1) << " / " << target_graphs.size() << ")\n";
      }

      IloNumArray result(env);

      cplex.getValues(result, x);

      for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
        //std::cout << t << " : (" << target_graph.vertices_[vertex_index].x << ", " << target_graph.vertices_[vertex_index].y << ") -> (" << cplex.getValue(x[vertex_index * 2 + variable_index_offset]) << ", " << cplex.getValue(x[vertex_index * 2 + 1 + variable_index_offset]) << ")\n";
        target_graph.vertices_[vertex_index].x = result[vertex_index * 2];
        target_graph.vertices_[vertex_index].y = result[vertex_index * 2 + 1];
      }

      model.end();
      cplex.end();
      env.end();

      // Calculate average deformation for the group appearing first
      for (const size_t first_appear_group_index : first_appear_group_list) {
        glm::vec2 &horizontal_average_deformation = object_horizontal_average_deformation_in_the_first_appear_frame[first_appear_group_index];
        glm::vec2 &vertical_average_deformation = object_vertical_average_deformation_in_the_first_appear_frame[first_appear_group_index];

        glm::vec2 &average_deformation = object_average_deformation_in_the_first_appear_frame[first_appear_group_index];

        horizontal_average_deformation = glm::vec2(0, 0);
        vertical_average_deformation = glm::vec2(0, 0);

        average_deformation = glm::vec2(0, 0);

        size_t horizontal_edge_count = 0;

        for (const size_t edge_index : edge_index_list_of_object[first_appear_group_index]) {
          const Edge &edge = target_graph.edges_[edge_index];
          size_t vertex_index_1 = edge.edge_indices_pair_.first;
          size_t vertex_index_2 = edge.edge_indices_pair_.second;
          float delta_x = target_graph.vertices_[vertex_index_1].x - target_graph.vertices_[vertex_index_2].x;
          float delta_y = target_graph.vertices_[vertex_index_1].y - target_graph.vertices_[vertex_index_2].y;
          if (std::abs(delta_x) > std::abs(delta_y)) { // Horizontal
            horizontal_average_deformation.x += delta_x;
            horizontal_average_deformation.y += delta_y;

            average_deformation.x += delta_x;

            ++horizontal_edge_count;
          } else {
            vertical_average_deformation.x += delta_x;
            vertical_average_deformation.y += delta_y;

            average_deformation.y += delta_y;
          }
        }

        if (horizontal_edge_count) {
          horizontal_average_deformation.x /= (double)horizontal_edge_count;
          horizontal_average_deformation.y /= (double)horizontal_edge_count;

          average_deformation.x /= (double)horizontal_edge_count;
        }

        size_t vertical_edge_count = edge_index_list_of_object[first_appear_group_index].size() - horizontal_edge_count;

        if (vertical_edge_count) {
          vertical_average_deformation.x /= (double)vertical_edge_count;
          vertical_average_deformation.y /= (double)vertical_edge_count;

          average_deformation.y /= (double)vertical_edge_count;
        }
      }
    }

    std::cout << "Optimization done.\n";
  }

  void PatchBasedWarping(const cv::Mat &image, Graph<glm::vec2> &target_graph, const std::vector<std::vector<int> > &group_of_pixel, const std::vector<double> &saliency_of_patch, const int target_image_width, const int target_image_height, const double mesh_width, const double mesh_height) {
    if (target_image_width <= 0 || target_image_height <= 0) {
      std::cout << "Wrong target image size (" << target_image_width << " x " << target_image_height << ")\n";
      return;
    }

    // Build the edge list of each patch
    std::vector<std::vector<size_t> > edge_index_list_of_patch(image.size().width * image.size().height);
    for (size_t edge_index = 0; edge_index < target_graph.edges_.size(); ++edge_index) {
      size_t vertex_index_1 = target_graph.edges_[edge_index].edge_indices_pair_.first;
      size_t vertex_index_2 = target_graph.edges_[edge_index].edge_indices_pair_.second;

      int group_of_x = group_of_pixel[target_graph.vertices_[vertex_index_1].y][target_graph.vertices_[vertex_index_1].x];
      int group_of_y = group_of_pixel[target_graph.vertices_[vertex_index_2].y][target_graph.vertices_[vertex_index_2].x];

      if (group_of_x == group_of_y) {
        edge_index_list_of_patch[group_of_x].push_back(edge_index);
      } else {
        edge_index_list_of_patch[group_of_x].push_back(edge_index);
        edge_index_list_of_patch[group_of_y].push_back(edge_index);
      }
    }

    IloEnv env;

    IloNumVarArray x(env);
    IloExpr expr(env);

    for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
      x.add(IloNumVar(env, -IloInfinity, IloInfinity));
      x.add(IloNumVar(env, -IloInfinity, IloInfinity));
    }

    const double DST_WEIGHT = 5.5;
    const double DLT_WEIGHT = 0.8;
    const double ORIENTATION_WEIGHT = 12.0;

    const double WIDTH_RATIO = target_image_width / (double)image.size().width;
    const double HEIGHT_RATIO = target_image_height / (double)image.size().height;

    // Patch transformation constraint
    for (size_t patch_index = 0; patch_index < edge_index_list_of_patch.size(); ++patch_index) {
      const std::vector<size_t> &edge_index_list = edge_index_list_of_patch[patch_index];
      const double PATCH_SIZE_WEIGHT = sqrt(1.0 / (double)edge_index_list.size());
      //const double PATCH_SIZE_WEIGHT = 1.0;

      if (!edge_index_list.size()) {
        continue;
      }

      //const Edge &representive_edge = target_graph.edges_[edge_index_list[edge_index_list.size() / 2]];
      const Edge &representive_edge = target_graph.edges_[edge_index_list[0]];

      double c_x = target_graph.vertices_[representive_edge.edge_indices_pair_.first].x - target_graph.vertices_[representive_edge.edge_indices_pair_.second].x;
      double c_y = target_graph.vertices_[representive_edge.edge_indices_pair_.first].y - target_graph.vertices_[representive_edge.edge_indices_pair_.second].y;

      double original_matrix_a = c_x;
      double original_matrix_b = c_y;
      double original_matrix_c = c_y;
      double original_matrix_d = -c_x;

      double matrix_rank = original_matrix_a * original_matrix_d - original_matrix_b * original_matrix_c;

      if (fabs(matrix_rank) <= 1e-9) {
        matrix_rank = (matrix_rank > 0 ? 1 : -1) * 1e-9;
      }

      double matrix_a = original_matrix_d / matrix_rank;
      double matrix_b = -original_matrix_b / matrix_rank;
      double matrix_c = -original_matrix_c / matrix_rank;
      double matrix_d = original_matrix_a / matrix_rank;

      for (const auto &edge_index : edge_index_list) {
        const Edge &edge = target_graph.edges_[edge_index];
        double e_x = target_graph.vertices_[edge.edge_indices_pair_.first].x - target_graph.vertices_[edge.edge_indices_pair_.second].x;
        double e_y = target_graph.vertices_[edge.edge_indices_pair_.first].y - target_graph.vertices_[edge.edge_indices_pair_.second].y;

        double t_s = matrix_a * e_x + matrix_b * e_y;
        double t_r = matrix_c * e_x + matrix_d * e_y;

        // DST
        expr += PATCH_SIZE_WEIGHT * DST_WEIGHT * saliency_of_patch[patch_index] *
          IloPower((x[edge.edge_indices_pair_.first * 2] - x[edge.edge_indices_pair_.second * 2]) -
            (t_s * (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_r * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
            2);
        expr += PATCH_SIZE_WEIGHT * DST_WEIGHT * saliency_of_patch[patch_index] *
          IloPower((x[edge.edge_indices_pair_.first * 2 + 1] - x[edge.edge_indices_pair_.second * 2 + 1]) -
            (-t_r * (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_s * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
            2);

        // DLT
        expr += PATCH_SIZE_WEIGHT * DLT_WEIGHT * (1 - saliency_of_patch[patch_index]) *
          IloPower((x[edge.edge_indices_pair_.first * 2] - x[edge.edge_indices_pair_.second * 2]) -
            WIDTH_RATIO * (t_s * (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_r * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
            2);
        expr += PATCH_SIZE_WEIGHT * DLT_WEIGHT * (1 - saliency_of_patch[patch_index]) *
          IloPower((x[edge.edge_indices_pair_.first * 2 + 1] - x[edge.edge_indices_pair_.second * 2 + 1]) -
            HEIGHT_RATIO * (-t_r *  (x[representive_edge.edge_indices_pair_.first * 2] - x[representive_edge.edge_indices_pair_.second * 2]) + t_s * (x[representive_edge.edge_indices_pair_.first * 2 + 1] - x[representive_edge.edge_indices_pair_.second * 2 + 1])),
            2);
      }
    }

    // Grid orientation constraint
    for (const Edge &edge : target_graph.edges_) {
      size_t vertex_index_1 = edge.edge_indices_pair_.first;
      size_t vertex_index_2 = edge.edge_indices_pair_.second;
      float delta_x = target_graph.vertices_[vertex_index_1].x - target_graph.vertices_[vertex_index_2].x;
      float delta_y = target_graph.vertices_[vertex_index_1].y - target_graph.vertices_[vertex_index_2].y;
      if (std::abs(delta_x) > std::abs(delta_y)) { // Horizontal
        expr += ORIENTATION_WEIGHT * IloPower(x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1], 2);
      } else {
        expr += ORIENTATION_WEIGHT * IloPower(x[vertex_index_1 * 2] - x[vertex_index_2 * 2], 2);
      }
    }

    IloModel model(env);

    model.add(IloMinimize(env, expr));

    IloRangeArray hard_constraint(env);

    size_t mesh_column_count = (int)(image.size().width / mesh_width) + 1;
    size_t mesh_row_count = (int)(image.size().height / mesh_height) + 1;

    // Boundary constraint
    for (size_t row = 0; row < mesh_row_count; ++row) {
      size_t vertex_index = row * mesh_column_count;
      hard_constraint.add(x[vertex_index * 2] == target_graph.vertices_[0].x);

      vertex_index = row * mesh_column_count + mesh_column_count - 1;
      hard_constraint.add(x[vertex_index * 2] == target_graph.vertices_[0].x + target_image_width);
    }

    for (size_t column = 0; column < mesh_column_count; ++column) {
      size_t vertex_index = column;
      hard_constraint.add(x[vertex_index * 2 + 1] == target_graph.vertices_[0].y);

      vertex_index = (mesh_row_count - 1) * mesh_column_count + column;
      hard_constraint.add(x[vertex_index * 2 + 1] == target_graph.vertices_[0].y + target_image_height);
    }

    // Avoid flipping
    for (size_t row = 0; row < mesh_row_count; ++row) {
      for (size_t column = 1; column < mesh_column_count; ++column) {
        size_t vertex_index_right = row * mesh_column_count + column;
        size_t vertex_index_left = row * mesh_column_count + column - 1;
        hard_constraint.add((x[vertex_index_right * 2] - x[vertex_index_left * 2]) >= 1e-4);
      }
    }

    for (size_t row = 1; row < mesh_row_count; ++row) {
      for (size_t column = 0; column < mesh_column_count; ++column) {
        size_t vertex_index_down = row * mesh_column_count + column;
        size_t vertex_index_up = (row - 1) * mesh_column_count + column;
        hard_constraint.add((x[vertex_index_down * 2 + 1] - x[vertex_index_up * 2 + 1]) >= 1e-4);
      }
    }

    model.add(hard_constraint);

    IloCplex cplex(model);

    cplex.setOut(env.getNullStream());
    if (!cplex.solve()) {
      std::cout << "Failed to optimize the model.\n";
    }

    IloNumArray result(env);

    cplex.getValues(result, x);

    for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
      target_graph.vertices_[vertex_index].x = result[vertex_index * 2];
      target_graph.vertices_[vertex_index].y = result[vertex_index * 2 + 1];
    }

    model.end();
    cplex.end();
    env.end();
  }

  void FocusWarping(const cv::Mat &image, Graph<glm::vec2> &target_graph, const std::vector<std::vector<int> > &group_of_pixel, const std::vector<double> &saliency_of_patch, const int target_image_width, const int target_image_height, const double mesh_width, const double mesh_height, const double max_mesh_scale, const double focus_x, const double focus_y) {
    if (target_image_width <= 0 || target_image_height <= 0) {
      std::cout << "Wrong target image size (" << target_image_width << " x " << target_image_height << ")\n";
      return;
    }

    double width_scale = target_image_width / (double)image.cols;
    double height_scale = target_image_height / (double)image.rows;

    IloEnv env;

    IloNumVarArray x(env);
    IloExpr expr(env);

    for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
      x.add(IloNumVar(env, -IloInfinity, IloInfinity));
      x.add(IloNumVar(env, -IloInfinity, IloInfinity));
    }

    int mesh_column_count = (int)(image.size().width / mesh_width) + 1;
    int mesh_row_count = (int)(image.size().height / mesh_height) + 1;

    const double FOCUS_WEIGHT = 5.0;
    const double ORIENTATION_WEIGHT = 10.0;
    const double DISTORTION_WEIGHT = 5.0;

    for (size_t edge_index = 0; edge_index < target_graph.edges_.size(); ++edge_index) {
      int vertex_index_1 = target_graph.edges_[edge_index].edge_indices_pair_.first;
      int vertex_index_2 = target_graph.edges_[edge_index].edge_indices_pair_.second;
      float delta_x = target_graph.vertices_[vertex_index_1].x - target_graph.vertices_[vertex_index_2].x;
      float delta_y = target_graph.vertices_[vertex_index_1].y - target_graph.vertices_[vertex_index_2].y;

      double distance_to_focus_point = 0;
      distance_to_focus_point += pow((target_graph.vertices_[vertex_index_1].x + target_graph.vertices_[vertex_index_2].x) / 2.0 - focus_x, 2.0);
      distance_to_focus_point += pow((target_graph.vertices_[vertex_index_1].y + target_graph.vertices_[vertex_index_2].y) / 2.0 - focus_y, 2.0);
      distance_to_focus_point = sqrt(distance_to_focus_point);

      // Normalize distance value to [0, 1]
      distance_to_focus_point /= sqrt(std::max(pow(focus_x, 2.0), pow(image.size().width - focus_x, 2.0)) + std::max(pow(focus_y, 2.0), pow(image.size().height - focus_y, 2.0)));

      double distance_weight = 1 - distance_to_focus_point;
      distance_weight = pow(distance_weight, 4.0);

      if (std::abs(delta_x) > std::abs(delta_y)) { // Horizontal
        expr += FOCUS_WEIGHT * distance_weight * IloPower((x[vertex_index_1 * 2] - x[vertex_index_2 * 2]) - max_mesh_scale * width_scale * delta_x, 2);
      } else {
        expr += FOCUS_WEIGHT * distance_weight * IloPower((x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1]) - max_mesh_scale * height_scale * delta_y, 2);
      }
    }

    // Grid orientation & distortion constraint
    for (const auto &edge : target_graph.edges_) {
      int vertex_index_1 = edge.edge_indices_pair_.first;
      int vertex_index_2 = edge.edge_indices_pair_.second;
      float delta_x = target_graph.vertices_[vertex_index_1].x - target_graph.vertices_[vertex_index_2].x;
      float delta_y = target_graph.vertices_[vertex_index_1].y - target_graph.vertices_[vertex_index_2].y;
      if (std::abs(delta_x) > std::abs(delta_y)) { // Horizontal
        expr += ORIENTATION_WEIGHT * IloPower(x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1], 2);
        expr += DISTORTION_WEIGHT * IloPower((x[vertex_index_1 * 2] - x[vertex_index_2 * 2]) - width_scale * delta_x, 2);
      } else {
        expr += ORIENTATION_WEIGHT * IloPower(x[vertex_index_1 * 2] - x[vertex_index_2 * 2], 2);
        expr += DISTORTION_WEIGHT * IloPower((x[vertex_index_1 * 2 + 1] - x[vertex_index_2 * 2 + 1]) - height_scale * delta_y, 2);
      }
    }


    IloModel model(env);

    model.add(IloMinimize(env, expr));

    IloRangeArray c(env);

    // Boundary constraint
    for (int row = 0; row < mesh_row_count; ++row) {
      int vertex_index = row * mesh_column_count;
      c.add(x[vertex_index * 2] == target_graph.vertices_[0].x);

      vertex_index = row * mesh_column_count + mesh_column_count - 1;
      c.add(x[vertex_index * 2] == target_image_width);
    }

    for (int column = 0; column < mesh_column_count; ++column) {
      int vertex_index = column;
      c.add(x[vertex_index * 2 + 1] == target_graph.vertices_[0].y);

      vertex_index = (mesh_row_count - 1) * mesh_column_count + column;
      c.add(x[vertex_index * 2 + 1] == target_image_height);
    }

    // Avoid flipping
    //for (int row = 0; row < mesh_row_count; ++row) {
    //  for (int column = 1; column < mesh_column_count; ++column) {
    //    int vertex_index_right = row * mesh_column_count + column;
    //    int vertex_index_left = row * mesh_column_count + column - 1;
    //    hard_constraint.add((x[vertex_index_right * 2] - x[vertex_index_left * 2]) >= 1e-4);
    //  }
    //}

    //for (int row = 1; row < mesh_row_count; ++row) {
    //  for (int column = 0; column < mesh_column_count; ++column) {
    //    int vertex_index_down = row * mesh_column_count + column;
    //    int vertex_index_up = (row - 1) * mesh_column_count + column;
    //    hard_constraint.add((x[vertex_index_down * 2 + 1] - x[vertex_index_up * 2 + 1]) >= 1e-4);
    //  }
    //}

    model.add(c);

    IloCplex cplex(model);

    cplex.setOut(env.getNullStream());

    if (!cplex.solve()) {
      std::cout << "Failed to optimize the model.\n";
    }

    IloNumArray result(env);

    cplex.getValues(result, x);

    for (size_t vertex_index = 0; vertex_index < target_graph.vertices_.size(); ++vertex_index) {
      target_graph.vertices_[vertex_index].x = result[vertex_index * 2];
      target_graph.vertices_[vertex_index].y = result[vertex_index * 2 + 1];
    }

    model.end();
    cplex.end();
    env.end();
  }

}