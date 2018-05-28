#include "gl_manager.h"

#include <sstream>

static Error ExtendErrorWithLog(GLchar *log, Error err) {
  std::stringstream ss;
  ss << log;

  string line;
  while (std::getline(ss, line)) {
    err = err.Extend(fmt::format("[GL log] {}", line));
  }

  return err;
}

static Error CompileShader(GLuint shader, const char *source) {
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLchar log[1024];
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    return ExtendErrorWithLog(log, Error::New("Failed to compile shader."));
  }

  return Error::New();
}

static Error LinkProgram(GLuint program, GLuint vertex_shader, GLuint fragment_shader) {
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    GLchar log[1024];
    glGetProgramInfoLog(program, sizeof(log), NULL, log);
    return ExtendErrorWithLog(log, Error::New("Failed to link shader."));
  }

  return Error::New();
}

static const char vertex_source[] =
  "#version 330 core\n"

  "layout (location = 0) in vec2 in_position;"
  "layout (location = 1) in vec2 in_texpos;"

  "out vec2 texpos;"

  "void main() {"
    "gl_Position = vec4(in_position, 0.0, 1.0);"
    "texpos = in_texpos;"
  "}"
;

static const char fragment_source[] =
  "#version 330 core\n"

  "uniform sampler2D tex;"

  "in vec2 texpos;"

  "out vec4 out_color;"

  "void main() {"
    "out_color = texture(tex, texpos);"
  "}"
;

void GLManager::Resize(int width, int height) {
  m_width = width;
  m_height = height;

  glViewport(0, 0, m_width, m_height);

  glBindTexture(GL_TEXTURE_2D, m_texture.id());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
}

Error GLManager::Initialize(int width, int height) {
  glClearColor(1, 1, 1, 0);

  *m_vertex.id_ptr() = glCreateShader(GL_VERTEX_SHADER);
  if (auto err = CompileShader(m_vertex.id(), vertex_source)) {
    return err.Extend("while compiling vertex shader");
  }

  *m_fragment.id_ptr() = glCreateShader(GL_FRAGMENT_SHADER);
  if (auto err = CompileShader(m_fragment.id(), fragment_source)) {
    return err.Extend("while compiling fragment shader");
  }

  *m_program.id_ptr() = glCreateProgram();
  if (auto err = LinkProgram(m_program.id(), m_vertex.id(), m_fragment.id())) {
    return err;
  }

  float vertices[] = {
    // vec2 position  vec2 textpos
    -1.0,  1.0,       0.0, 0.0,  // Top-left.
     1.0,  1.0,       1.0, 0.0,  // Top-right.
     1.0, -1.0,       1.0, 1.0,  // Bottom-right.
    -1.0, -1.0,       0.0, 1.0,  // Bottom-left.
  };

  GLuint indices[] = {
    1, 2, 0,
    2, 3, 0,
  };

  constexpr GLsizei stride = sizeof(float) * 4;

  glGenVertexArrays(1, m_vao.id_ptr());
  glGenBuffers(1, m_vbo.id_ptr());
  glGenBuffers(1, m_ebo.id_ptr());

  glBindVertexArray(m_vao.id());

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo.id());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                        reinterpret_cast<void*>(sizeof(float) * 2));
  glEnableVertexAttribArray(1);

  glGenTextures(1, m_texture.id_ptr());
  glBindTexture(GL_TEXTURE_2D, m_texture.id());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  Resize(width, height);

  glUseProgram(m_program.id());
  glUniform1i(glGetUniformLocation(m_program.id(), "tex"), 0);

  return Error::New();
}

void GLManager::UpdateTextureData(const void *data) {
  glBindTexture(GL_TEXTURE_2D, m_texture.id());
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE,
                  data);
}

void GLManager::Draw() {
  glClear(GL_COLOR_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_texture.id());

  glUseProgram(m_program.id());
  glBindVertexArray(m_vao.id());
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
