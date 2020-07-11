#version 410

uniform sampler2D texture;

uniform lowp float texture_flag;

varying vec3 fragment_color;
varying vec2 fragment_vertex_uv;

void main () {
  gl_FragColor = texture_flag * texture2D(texture, fragment_vertex_uv) + (1.0 - texture_flag) * vec4(fragment_color, 1.0);
}