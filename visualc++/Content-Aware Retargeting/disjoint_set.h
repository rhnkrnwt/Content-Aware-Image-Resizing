#pragma once

#include <vector>

class DisjointSet {

public:

  DisjointSet() {
  }

  DisjointSet(size_t group_count) : group_count(group_count) {
    group_of_element_at.resize(group_count);
    group_size.resize(group_count);
    for (size_t i = 0; i < group_count; ++i) {
      group_of_element_at[i] = i;
      group_size[i] = 1;
    }
  }

  size_t GroupCount() {
    return group_count;
  }

  size_t SizeOfGroup(size_t x) {
    size_t group_of_x = FindGroup(x);
    return group_size[group_of_x];
  }

  size_t FindGroup(size_t x) {
    return (x == group_of_element_at[x]) ? x : (group_of_element_at[x] = FindGroup(group_of_element_at[x]));
  }

  void UnionGroup(size_t x, size_t y) {
    size_t group_of_x = FindGroup(x);
    size_t group_of_y = FindGroup(y);
    if (group_of_x == group_of_y) {
      return;
    }
    group_size[group_of_y] += group_size[group_of_x];
    group_size[group_of_x] = 0;
    group_of_element_at[x] = group_of_y;
  }

private:

  size_t group_count;
  std::vector<size_t> group_of_element_at;
  std::vector<size_t> group_size;
};