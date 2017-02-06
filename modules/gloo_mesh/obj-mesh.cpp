#include "obj-mesh.h"

#include <iostream>

#define LOG_OUTPUT_ON 1

namespace gloo
{

void ObjMesh::Clear()
{
  mVertices.clear();
  mNormals.clear();
  mUVs.clear();

  for (auto & group : mGroups )
  {
    group.mFaces.clear();
  }
}

size_t ObjMesh::GetNumFaces() const
{
  size_t count = 0;
  for (auto & group : mGroups)
  {
    count += group.mFaces.size();
  }
  return count;
}

void ObjMesh::LogData() const
{
  std::cout << "#vertices = " << mVertices.size() << std::endl;
  std::cout << "#normals  = " << mNormals.size() << std::endl;
  std::cout << "#uvs      = " << mUVs.size() << std::endl;
  std::cout << "#faces    = " << ObjMesh::GetNumFaces() << std::endl;

  std::cout << "#groups   = " << mGroups.size() << std::endl;
  for (auto & group : mGroups)
  {
    std::cout << "\t" << group.mName << std::endl;
  }
}

MeshGroup<Batch>* ObjMesh::ExportToMeshGroup(int groupIndex, bool smoothShading) 
{  
  if ((groupIndex < 0) && (groupIndex > mGroups.size())) 
  {
    // Error: invalid index.
    std::cerr << "ERROR Invalid groupIndex. " << std::endl;
    return nullptr;
  }
  const ObjGroup & objGroup = mGroups[groupIndex];

  // Check attribute list.
  bool hasNormals = (mNormals.size() > 0);
  bool hasUVs     = (mUVs.size()   > 0);

  if (!hasNormals && smoothShading)
  {
#if LOG_OUTPUT_ON == 1
    std::cout << "No normals availabe. Computing per vertex normals ... " << std::endl;
#endif
    ObjMesh::ComputeVertexNormals(true);  
  }
  else if (!smoothShading)
  {
#if LOG_OUTPUT_ON == 1
    std::cout << "No flat normals availabe. Computing per face normals ... " << std::endl;
#endif
    ObjMesh::ComputeFaceNormals(true);
  }

  // Count number of vertices.
  size_t numVertices = 3 * ObjMesh::GetNumFacesOnGroup(groupIndex);
  size_t numElements = numVertices;

  // Copy all data referenced on face indices into an auxiliary buffer.
  std::vector<float> groupPositions;
  std::vector<float> groupNormals;
  std::vector<float> groupUVs;

  groupPositions.reserve(3*numVertices);
  groupNormals.reserve(3*numVertices);

  if (hasUVs)
    groupUVs.reserve(2*numVertices);

  for (int i = 0; i < objGroup.mFaces.size(); i++)
  {
    for (int vtx = 0; vtx < 3; vtx++)
    {
      // Retrieve v, vt and vn.
      int pos_index = objGroup.mFaces[i][vtx][0];
      int  uv_index = objGroup.mFaces[i][vtx][1];
      int nor_index = objGroup.mFaces[i][vtx][2];

      groupPositions.push_back(mVertices[pos_index].x);
      groupPositions.push_back(mVertices[pos_index].y);
      groupPositions.push_back(mVertices[pos_index].z);

      if (smoothShading)
      {
        groupNormals.push_back(mNormals[nor_index].x);
        groupNormals.push_back(mNormals[nor_index].y);
        groupNormals.push_back(mNormals[nor_index].z);  
      }
      else  // Flat shading: use face normals.
      {
        glm::vec3 n = objGroup.mFaces[i].GetNormal();
        groupNormals.push_back(n.x);
        groupNormals.push_back(n.y);
        groupNormals.push_back(n.z); 
      }

      if (hasUVs)
      {
        groupUVs.push_back(mUVs[uv_index].x);
        groupUVs.push_back(mUVs[uv_index].y);
      }
    }
  }

  // Build up MeshGroup.
  std::vector<GLuint> vertexAttribList;
  std::vector<GLfloat*> bufferList;

  // Add position attrib info.
  vertexAttribList.push_back(3);
  bufferList.push_back(groupPositions.data());

  vertexAttribList.push_back(3);
  bufferList.push_back(groupNormals.data());

  if (hasUVs)
  {
    vertexAttribList.push_back(2);
    bufferList.push_back(groupUVs.data());
  }

  MeshGroup<Batch>* meshGroup = new MeshGroup<Batch>(numVertices, numElements, GL_TRIANGLES);
  meshGroup->SetVertexAttribList(vertexAttribList);

  if (!meshGroup->Load(bufferList, nullptr))
  {
    return nullptr;
  }
  else
  {
    return meshGroup;
  }
}

void ObjMesh::TriangulateQuads()
{
  for (auto & group : mGroups)
  {
    size_t originalNumFaces = group.mFaces.size();
    for (int i = 0; i < originalNumFaces; i++)
    {
      // If the current face is a quad, then tesselate it.
      if (group.mFaces[i].VertexList().size() == 4)
      {
        // Remove vertex 3 from current face.
        std::vector<int> vtx3 = group.mFaces[i][3];
        group.mFaces[i].VertexList().pop_back();

        //  Create a new triangle with vertices 0, 2, 3.
        std::vector<int> & vtx2 = group.mFaces[i][2];
        std::vector<int> & vtx1 = group.mFaces[i][0];

        group.mFaces.push_back({vtx1, vtx2, vtx3});
      }
    }
  }
}

void ObjMesh::ComputeVertexNormals(bool normalize)
{
  // For each vtx, stores a list of faces that share it.
  std::vector<std::vector<Face*>> mVertexFaces(mVertices.size());

  // Build up the adjacency list (between vertices and faces).
  for (auto & group : mGroups)
  {
    for (auto & face : group.mFaces)
    {
      for (int vtx = 0; vtx < face.VertexList().size(); vtx++)
      {
        mVertexFaces[face[vtx][0]].push_back(&face);
      }
    }
  }

  // Compute face normals (but don't normalize them).
  ObjMesh::ComputeFaceNormals(false);
  mNormals.reserve(mVertices.size());

  // Once the adjacency list is built, then compute the weighted average of all normals whose
  // face share the current vertex.
  for (int i = 0; i < mVertexFaces.size(); i++)
  {
    glm::vec3 n(0.0f);
    float total_weight = 0.0f;

    for (int j = 0; j < mVertexFaces[i].size(); j++)
      total_weight += glm::length(mVertexFaces[i][j]->GetNormal());

    for (int j = 0; j < mVertexFaces[i].size(); j++)
    {
      float weight = glm::length(mVertexFaces[i][j]->GetNormal());
      n += mVertexFaces[i][j]->GetNormal() * (weight/total_weight);
    }

    if (normalize)
      n = glm::normalize(n);

    mNormals.push_back(n);

    for (int j = 0; j < mVertexFaces[i].size(); j++)
    {
      Face & face = * mVertexFaces[i][j];
      for (int vtx = 0; vtx < face.VertexList().size(); vtx++)
      {
        // Point vertex normal to mNormal[i].
        if (face[vtx][0] == i)
          face[vtx][2] = i; 
      }
    }

  }
}

void ObjMesh::ComputeFaceNormals(bool normalize)
{
  for (auto & group : mGroups)
  {
    for (auto & face : group.mFaces)
    {
      if (face.VertexList().size() == 3)
      {
        glm::vec3 & v0 = mVertices[face[0][0]];
        glm::vec3 & v1 = mVertices[face[1][0]];
        glm::vec3 & v2 = mVertices[face[2][0]];
        glm::vec3 n = glm::cross(v1-v0, v2-v0);

        if (normalize)
          n = glm::normalize(n);

        face.SetNormal(n);
      }
    }
  }
}

}  // namespace gloo.