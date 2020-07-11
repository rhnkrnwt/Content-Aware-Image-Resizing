#pragma once

#include "gl_mesh.h"

namespace ContentAwareRetargeting {

  GLMesh::GLMesh() : vbo_vertices_(0), vbo_colors_(0), vbo_uvs_(0), vertices_type(GL_TRIANGLES), local_modelview_matrix_(glm::mat4(1.0)), texture_id_(0), texture_flag_(false) {
  }

  GLMesh::GLMesh(const GLMesh &other) {
    DeepCopy(other);
  }

  GLMesh::~GLMesh()  {
    if (vbo_vertices_) {
      glDeleteBuffers(1, &vbo_vertices_);
    }
    if (vbo_colors_) {
      glDeleteBuffers(1, &vbo_colors_);
    }
    if (vbo_uvs_) {
      glDeleteBuffers(1, &vbo_uvs_);
    }
  }

  GLMesh &GLMesh::operator=(const GLMesh &other) {
    DeepCopy(other);
    return *this;
  }

  void GLMesh::DeepCopy(const GLMesh &other) {
    vertices_ = other.vertices_;
    colors_ = other.colors_;
    uvs_ = other.uvs_;

    texture_flag_ = other.texture_flag_;

    texture_id_ = other.texture_id_;

    local_modelview_matrix_ = other.local_modelview_matrix_;

    vbo_vertices_ = vbo_colors_ = vbo_uvs_ = 0;

    this->Upload();
  }

  void GLMesh::Translate(const glm::vec3 &translation_vector) {
    local_modelview_matrix_ = glm::translate(local_modelview_matrix_, translation_vector);
  }

  void GLMesh::Upload() {
    if (vertices_.size()) {
      if (!vbo_vertices_) {
        glGenBuffers(1, &vbo_vertices_);
      }
      glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices_);
      glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(vertices_[0]), vertices_.data(), GL_STATIC_DRAW);
    }

    if (colors_.size()) {
      if (!vbo_colors_) {
        glGenBuffers(1, &vbo_colors_);
      }
      glBindBuffer(GL_ARRAY_BUFFER, vbo_colors_);
      glBufferData(GL_ARRAY_BUFFER, colors_.size() * sizeof(colors_[0]), colors_.data(), GL_STATIC_DRAW);
    }

    if (uvs_.size()) {
      if (!vbo_uvs_) {
        glGenBuffers(1, &vbo_uvs_);
      }
      glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs_);
      glBufferData(GL_ARRAY_BUFFER, uvs_.size() * sizeof(uvs_[0]), uvs_.data(), GL_STATIC_DRAW);
    }

    texture_flag_ = uvs_.size();
  }

  void GLMesh::Clear() {
    vertices_.clear();
    colors_.clear();
    uvs_.clear();
    local_modelview_matrix_ = glm::mat4(1.0);
    texture_id_ = 0;

    vbo_vertices_ = vbo_colors_ = vbo_uvs_ = 0;

    Upload();
  }

  void GLMesh::Draw(const GLShader &gl_shader) const {
    Draw(gl_shader, glm::mat4(1.0f));
  }

  void GLMesh::Draw(const GLShader &gl_shader, const glm::mat4 &parent_modelview_matrix) const {

    if (vbo_vertices_) {
      glEnableVertexAttribArray(gl_shader.shader_attribute_vertex_position_id_);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices_);
      glVertexAttribPointer(gl_shader.shader_attribute_vertex_position_id_, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (vbo_colors_) {
      glEnableVertexAttribArray(gl_shader.shader_attribute_vertex_color_id_);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_colors_);
      glVertexAttribPointer(gl_shader.shader_attribute_vertex_color_id_, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (vbo_uvs_) {
      glEnableVertexAttribArray(gl_shader.shader_attribute_vertex_uv_id_);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs_);
      glVertexAttribPointer(gl_shader.shader_attribute_vertex_uv_id_, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (texture_flag_) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_id_);
      glUniform1i(gl_shader.shader_uniform_texture_id_, 0);
    }

    glUniform1f(gl_shader.shader_uniform_texture_flag_id_, texture_flag_ ? 1.0 : 0.0);

    glm::mat4 modelview_matrix = parent_modelview_matrix * local_modelview_matrix_;

    glUniformMatrix4fv(gl_shader.shader_uniform_modelview_matrix_id_, 1, GL_FALSE, glm::value_ptr(modelview_matrix));

    glDrawArrays(vertices_type, 0, vertices_.size());

    if (vbo_vertices_) {
      glDisableVertexAttribArray(gl_shader.shader_attribute_vertex_position_id_);
    }

    if (vbo_colors_) {
      glDisableVertexAttribArray(gl_shader.shader_attribute_vertex_color_id_);
    }

    if (vbo_uvs_) {
      glDisableVertexAttribArray(gl_shader.shader_attribute_vertex_uv_id_);
    }

  }

}