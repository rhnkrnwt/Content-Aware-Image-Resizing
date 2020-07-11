#pragma once

#include <utility>
#include <vector>

class Edge {

public:

  Edge() {
  }

  Edge(const std::pair<size_t, size_t> &edge_indices_pair) : edge_indices_pair_(edge_indices_pair) {
  }

  Edge(const std::pair<size_t, size_t> &edge_indices_pair, double weight) : edge_indices_pair_(edge_indices_pair), weight_(weight) {
  }

  const bool operator <(const Edge &other) {
    if (weight_ != other.weight_) {
      return weight_ < other.weight_;
    }
    if ((edge_indices_pair_.first != other.edge_indices_pair_.first)) {
      return  edge_indices_pair_.first < other.edge_indices_pair_.first;
    }
    return edge_indices_pair_.second < other.edge_indices_pair_.second;
  }

  std::pair<size_t, size_t> edge_indices_pair_;
  double weight_;
};

template <class T>
class Graph {

public:

  Graph() {
  }

  Graph(std::vector<T> &vertices, std::vector<Edge> &edges) : vertices_(vertices), edges_(edges) {
  }

  std::vector<T> vertices_;
  std::vector<Edge> edges_;
};