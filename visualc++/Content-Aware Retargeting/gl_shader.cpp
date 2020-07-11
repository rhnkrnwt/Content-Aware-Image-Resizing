#pragma once

#include "gl_shader.h"

const std::string DEFAULT_VERTEX_SHADER_FILE_PATH = "..\\shaders\\vertex_shader.glsl";
const std::string DEFAULT_FRAGMENT_SHADER_FILE_PATH = "..\\shaders\\fragment_shader.glsl";

const std::string DEFAULT_SHADER_ATTRIBUTE_VERTEX_POSITION_NAME = "vertex_position";
const std::string DEFAULT_SHADER_ATTRIBUTE_VERTEX_COLOR_NAME = "vertex_color";
const std::string DEFAULT_SHADER_ATTRIBUTE_VERTEX_UV_NAME = "vertex_uv";

const std::string DEFAULT_SHADER_UNIFORM_MODELVIEW_MATRIX_NAME = "modelview_matrix";
const std::string DEFAULT_SHADER_UNIFORM_VIEW_MATRIX_NAME = "view_matrix";
const std::string DEFAULT_SHADER_UNIFORM_PROJECTION_MATRIX_NAME = "projection_matrix";

const std::string DEFAULT_SHADER_UNIFORM_TEXTURE_NAME = "texture";
const std::string DEFAULT_SHADER_UNIFORM_TEXTURE_FLAG_NAME = "texture_flag";

GLShader::GLShader() {
}

GLShader::GLShader(const std::string &vertex_shader_file_path,
  const std::string &fragment_shader_file_path,

  const std::string &shader_attribute_vertex_position_name,
  const std::string &shader_attribute_vertex_color_name,
  const std::string &shader_attribute_vertex_uv_name,

  const std::string &shader_uniform_modelview_matrix_name,
  const std::string &shader_uniform_view_matrix_name,
  const std::string &shader_uniform_projection_matrix_name,
  const std::string &shader_uniform_texture_name,
  const std::string &shader_uniform_texture_flag_name) {

  std::string vertex_shader_string;
  std::string fragment_shader_string;

  if (!ParseFileIntoString(vertex_shader_file_path, vertex_shader_string)) {
    return;
  }

  if (!ParseFileIntoString(fragment_shader_file_path, fragment_shader_string)) {
    return;
  }

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  const GLchar *vertex_shader_pointer = (const GLchar*)vertex_shader_string.c_str();
  glShaderSource(vertex_shader, 1, &vertex_shader_pointer, NULL);
  glCompileShader(vertex_shader);

  int shader_compile_status;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_compile_status);
  if (shader_compile_status != GL_TRUE) {
    std::cerr << "Could not compile the shader " << vertex_shader_file_path << " .\n";
  }

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  const GLchar *fragment_shader_pointer = (const GLchar*)fragment_shader_string.c_str();
  glShaderSource(fragment_shader, 1, &fragment_shader_pointer, NULL);
  glCompileShader(fragment_shader);

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_compile_status);
  if (shader_compile_status != GL_TRUE) {
    std::cerr << "Could not compile the shader " << fragment_shader_file_path << " .\n";
  }

  shader_program_id_ = glCreateProgram();
  glAttachShader(shader_program_id_, vertex_shader);
  glAttachShader(shader_program_id_, fragment_shader);
  glLinkProgram(shader_program_id_);

  int shader_link_status;
  glGetProgramiv(shader_program_id_, GL_LINK_STATUS, &shader_link_status);
  if (shader_link_status != GL_TRUE) {
    std::cerr << "Could not link the shader.\n";
  }

  int shader_validate_status;
  glValidateProgram(shader_program_id_);
  glGetProgramiv(shader_program_id_, GL_VALIDATE_STATUS, &shader_validate_status);
  if (shader_validate_status != GL_TRUE) {
    std::cerr << "Could not validate the shader.\n";
  }

  shader_attribute_vertex_position_id_ = glGetAttribLocation(shader_program_id_, shader_attribute_vertex_position_name.c_str());
  if (shader_attribute_vertex_position_id_ == -1) {
    std::cerr << "Could not bind attribute " << shader_attribute_vertex_position_name << ".\n";
  }

  shader_attribute_vertex_color_id_ = glGetAttribLocation(shader_program_id_, shader_attribute_vertex_color_name.c_str());
  if (shader_attribute_vertex_color_id_ == -1) {
    std::cerr << "Could not bind attribute " << shader_attribute_vertex_color_name << ".\n";
  }

  shader_attribute_vertex_uv_id_ = glGetAttribLocation(shader_program_id_, shader_attribute_vertex_uv_name.c_str());
  if (shader_attribute_vertex_uv_id_ == -1) {
    std::cerr << "Could not bind attribute " << shader_attribute_vertex_uv_name << ".\n";
  }

  shader_uniform_modelview_matrix_id_ = glGetUniformLocation(shader_program_id_, shader_uniform_modelview_matrix_name.c_str());
  if (shader_uniform_modelview_matrix_id_ == -1) {
    std::cerr << "Could not bind uniform " << shader_uniform_modelview_matrix_name << ".\n";
  }

  shader_uniform_view_matrix_id_ = glGetUniformLocation(shader_program_id_, shader_uniform_view_matrix_name.c_str());
  if (shader_uniform_view_matrix_id_ == -1) {
    std::cerr << "Could not bind uniform " << shader_uniform_view_matrix_name << ".\n";
  }

  shader_uniform_projection_matrix_id_ = glGetUniformLocation(shader_program_id_, shader_uniform_projection_matrix_name.c_str());
  if (shader_uniform_projection_matrix_id_ == -1) {
    std::cerr << "Could not bind uniform " << shader_uniform_projection_matrix_name << ".\n";
  }

  shader_uniform_texture_id_ = glGetUniformLocation(shader_program_id_, shader_uniform_texture_name.c_str());
  if (shader_uniform_texture_id_ == -1) {
    std::cerr << "Could not bind uniform " << shader_uniform_texture_name << ".\n";
  }

  shader_uniform_texture_flag_id_ = glGetUniformLocation(shader_program_id_, shader_uniform_texture_flag_name.c_str());
  if (shader_uniform_texture_flag_id_ == -1) {
    std::cerr << "Could not bind uniform " << shader_uniform_texture_flag_name << ".\n";
  }
}

void GLShader::CreateDefaultShader() {
  *this = GLShader(DEFAULT_VERTEX_SHADER_FILE_PATH,
    DEFAULT_FRAGMENT_SHADER_FILE_PATH,

    DEFAULT_SHADER_ATTRIBUTE_VERTEX_POSITION_NAME,
    DEFAULT_SHADER_ATTRIBUTE_VERTEX_COLOR_NAME,
    DEFAULT_SHADER_ATTRIBUTE_VERTEX_UV_NAME,

    DEFAULT_SHADER_UNIFORM_MODELVIEW_MATRIX_NAME,
    DEFAULT_SHADER_UNIFORM_VIEW_MATRIX_NAME,
    DEFAULT_SHADER_UNIFORM_PROJECTION_MATRIX_NAME,

    DEFAULT_SHADER_UNIFORM_TEXTURE_NAME,
    DEFAULT_SHADER_UNIFORM_TEXTURE_FLAG_NAME
  );
}

bool GLShader::ParseFileIntoString(const std::string &file_path, std::string &file_string) {
  std::ifstream file_stream(file_path);
  if (!file_stream.is_open()) {
    std::cerr << "File not found : " << file_path << "\n";
    return false;
  }
  file_string = std::string(std::istreambuf_iterator<char>(file_stream), std::istreambuf_iterator<char>());
  return true;
}