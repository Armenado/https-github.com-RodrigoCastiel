// + ======================================== +
// |         gl-oo-interface library          |
// |            Module: GLOO Mesh.            |
// |        Author: Rodrigo Castiel, 2016.    |
// + ======================================== +

// ============================================================================== //
// Group
// ============================================================================== //
// 3D-surface meshes are made up by groups, which are parts that share the same
// materials or textures.
//
// Each group has a single buffer in GPU, which can follow two kinds of storage:
// 1. Interleaved (Tighly Packed):  (P N T) (P N T) ... (P N T)
// 2. Batched (Sub-Buffered):       (P P ... P) (N N ... N) (T T ... T)
// Where P is the vertex positions array, N is the vertex normals array and so on.
// 
// 

#pragma once

#include "gl_header.h"

namespace gloo
{

enum StorageFormat
{
  Interleave,
  Batch,
};

template <StorageFormat F>
class StaticGroup
{
public:
  StaticGroup()  { }
  ~StaticGroup() { }

  // 1. Methods for loading geometry.

  // Loads specified geometry into GPU buffers, without storing it in client-side memory.
  bool Load(GLuint programHandle,       // ShaderProgram Handle.
            const GLfloat* positions,   // Format: { (x, y, z) }
            const GLfloat* colors,      // Format: { (r, g, b) }
            const GLfloat* normals,     // Format: { (nx, ny, nz) }
            const GLfloat* uv,          // Format: { (u, v) }
            const GLuint* indices,      // Format: { i0, i1, i2, ... }.
            int numVertices,            // Number of vertices.
            int numIndices,             // Number of indices/elements.
            GLenum drawMode  // GL_LINES, GL_LINE_STRIP, GL_POINTS, GL_TRIANGLES, ...
  );


private:
  // OpenGL buffer IDs.
  GLuint mEab { 0 };
  GLuint mVao { 0 };
  GLuint mVbo { 0 };

  // Geometry and rendering options.
  GLenum mDrawMode { GL_TRIANGLES };
};

// template <>
// bool StaticGroup<Interleave>::Load(GLuint programHandle,
//                                    const GLfloat* positions, 
//                                    const GLfloat* colors,
//                                    const GLfloat* normals,
//                                    const GLfloat* uv,
//                                    const GLuint* indices,
//                                    int numVertices,
//                                    int numIndices,
//                                    GLenum drawMode)
// {

// }

template <>
bool StaticGroup<Interleave>::Load(GLuint programHandle,
                                   const GLfloat* positions, 
                                   const GLfloat* colors,
                                   const GLfloat* normals,
                                   const GLfloat* uv,
                                   const GLuint* indices,
                                   int numVertices,
                                   int numIndices,
                                   GLenum drawMode);


template <>
bool StaticGroup<Batch>::Load(GLuint programHandle,
                              const GLfloat* positions, 
                              const GLfloat* colors,
                              const GLfloat* normals,
                              const GLfloat* uv,
                              const GLuint* indices,
                              int numVertices,
                              int numIndices,
                              GLenum drawMode);


}  // namespace gloo.