#pragma once

#include <GL\glew.h>
#include <opencv\cv.hpp>

namespace ContentAwareRetargeting {

  namespace GLTexture {

    void SetGLTexture(void *image_data_pointer, int width, int height, GLuint *texture_id) {
      glDeleteTextures(1, texture_id);

      glGenTextures(1, texture_id);
      glBindTexture(GL_TEXTURE_2D, *texture_id);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data_pointer);
    }

    void SetGLTexture(const cv::Mat &cv_image, GLuint *texture_id) {
      cv::Mat image_for_gl_texture;
      cv::cvtColor(cv_image, image_for_gl_texture, CV_BGR2RGB);
      cv::flip(image_for_gl_texture, image_for_gl_texture, 0);
      SetGLTexture(image_for_gl_texture.data, image_for_gl_texture.size().width, image_for_gl_texture.size().height, texture_id);
    }

  };

}