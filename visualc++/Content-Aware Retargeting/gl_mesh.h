#pragma once

#include <cstdlib>

#include <memory>
#include <string>
#include <vector>

#include <GL\glew.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <opencv\cv.hpp>

#include "gl_shader.h"

namespace ContentAwareRetargeting {

  class GLMesh {

  public:

    GLMesh();

    GLMesh(const GLMesh &other);

    ~GLMesh();

    GLMesh &operator=(const GLMesh &other);

    void DeepCopy(const GLMesh &other);

    void Translate(const glm::vec3 &translation_vector);

    void Upload();

    void Clear();

    void Draw(const GLShader &gl_shader) const;

    void Draw(const GLShader &gl_shader, const glm::mat4 &parent_modelview_matrix) const;

    std::vector<glm::vec3> vertices_;
    std::vector<glm::vec3> colors_;
    std::vector<glm::vec2> uvs_;

    GLuint texture_id_;
    GLuint vertices_type;

    bool texture_flag_;

  private:

    GLuint vbo_vertices_;
    GLuint vbo_colors_;
    GLuint vbo_uvs_;

    glm::mat4 local_modelview_matrix_;
  };

}