#pragma once

#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include <GL\glew.h>

class GLShader {

public:

  GLShader();

  GLShader(const std::string &vertex_shader_file_path,
    const std::string &fragment_shader_file_path,

    const std::string &shader_attribute_vertex_position_name,
    const std::string &shader_attribute_vertex_color_name,
    const std::string &shader_attribute_vertex_uv_name,

    const std::string &shader_uniform_modelview_matrix_name,
    const std::string &shader_uniform_view_matrix_name,
    const std::string &shader_uniform_projection_matrix_name,

    const std::string &shader_uniform_texture_name,
    const std::string &shader_uniform_texture_flag_name);

  void CreateDefaultShader();

  bool ParseFileIntoString(const std::string &file_path, std::string &file_string);

  GLint shader_program_id_;
  GLint shader_attribute_vertex_position_id_;
  GLint shader_attribute_vertex_color_id_;
  GLint shader_attribute_vertex_uv_id_;
  GLint shader_uniform_modelview_matrix_id_;
  GLint shader_uniform_view_matrix_id_;
  GLint shader_uniform_projection_matrix_id_;
  GLint shader_uniform_texture_id_;
  GLint shader_uniform_texture_flag_id_;
};