#pragma once

#include "base.h"
#include "error.h"

#include <GL/gl3w.h>

class GLManager {
public:
  void Resize(int width, int height);
  Error Initialize(int width, int height);

  void UpdateTextureData(const void *data);
  void Draw();
private:
  struct ShaderId {
    void operator()(GLuint id) { glDeleteShader(id); }
  };

  struct ProgramId {
    void operator()(GLuint id) { glDeleteProgram(id); }
  };

  struct VertexArrayId {
    void operator()(GLuint id) { glDeleteVertexArrays(1, &id); }
  };

  struct BufferId {
    void operator()(GLuint id) { glDeleteBuffers(1, &id); }
  };

  struct TextureId {
    void operator()(GLuint id) { glDeleteTextures(1, &id); }
  };

  template <typename C>
  class IdWrapper {
  public:
    ~IdWrapper() {
      if (m_id != std::numeric_limits<GLuint>::max()) {
        m_clear(m_id);
      }
    }

    GLuint id() const { return m_id; }
    GLuint *id_ptr() { return &m_id; }
  private:
    GLuint m_id{std::numeric_limits<GLuint>::max()};
    C m_clear;
  };

  int m_width{-1}, m_height{-1};

  IdWrapper<ShaderId> m_vertex, m_fragment;
  IdWrapper<ProgramId> m_program;
  IdWrapper<VertexArrayId> m_vao;
  IdWrapper<BufferId> m_vbo, m_ebo;
  IdWrapper<TextureId> m_texture;
};
