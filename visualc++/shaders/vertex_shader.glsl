#version 410

attribute vec3 vertex_position;
attribute vec3 vertex_color;
attribute vec2 vertex_uv;

uniform mat4 modelview_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

varying vec3 fragment_color;
varying vec2 fragment_vertex_uv;

void main () {
  fragment_color = vertex_color;
  fragment_vertex_uv = vec2(vertex_uv.x, vertex_uv.y);

  gl_Position = projection_matrix * view_matrix * modelview_matrix * vec4(vertex_position, 1.0);
}
