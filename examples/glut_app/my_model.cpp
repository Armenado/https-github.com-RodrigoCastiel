#include "my_model.h"

#include <gloo/transform.h>
#include <gloo/mouse_event.h>
#include <gloo/group.h>

#include <cstdio>
#include <iostream>


GLfloat squareVertices[] = {-0.5f, 0.0f,  0.5f,
                             0.5f, 0.0f,  0.5f,
                            -0.5f, 0.0f, -0.5f,
                             0.5f, 0.0f, -0.5f};

GLfloat squareColors[] = {1.0f, 0.0f, 0.0f, 
                          0.0f, 1.0f, 0.0f,
                          0.0f, 0.0f, 1.0f,
                          0.4f, 0.4f, 0.4f};

GLfloat squareNormals[] = {1.0f, 0.0f, 0.0f, 
                           0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 1.0f,
                           0.4f, 0.4f, 0.4f};


GLfloat squareBuffer[] = {-0.5f, 0.0f,  0.5f,
                           0.5f, 0.0f,  0.5f,
                          -0.5f, 0.0f, -0.5f,
                           0.5f, 0.0f, -0.5f,

                          1.0f, 0.0f, 0.0f,
                          0.0f, 1.0f, 0.0f,
                          0.0f, 0.0f, 1.0f,
                          0.4f, 0.4f, 0.4f
                           };


GLfloat squareBufferInterleaved[] =  {-0.5f, 0.0f,  0.5f,   1.0f, 0.0f, 0.0f,
                                       0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,
                                      -0.5f, 0.0f, -0.5f,   0.0f, 0.0f, 1.0f,
                                       0.5f, 0.0f, -0.5f,   0.4f, 0.4f, 0.4f};

GLuint indices[] = {0, 2, 1, 3};

GLuint program; 

MyModel::MyModel()
{

}

MyModel::~MyModel()
{
  delete mCamera;
  delete mShaderProgram;
  delete mMeshGroup;
  delete mMeshGroup2;

  delete mAxis;
}

bool MyModel::Init()
{
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glPointSize(2);
  
  mShaderProgram = new gloo::ShaderProgram();
  mShaderProgram->BuildFromFiles("phong_no_shadow/vertex_shader.glsl", 
                                 "phong_no_shadow/fragment_shader.glsl");
  mShaderProgram->PrintCompilationLog();
  gloo::CompilationStatus status = mShaderProgram->GetCompilationStatus();
  // std::cout << "Compilation Status = " << status << std::endl;

  if (status != gloo::CompilationStatus::kSuccess) 
  {
    delete mShaderProgram;
    return false;
  }

  program = mShaderProgram->GetHandle();
  mShaderProgram->Bind();

  mCamera = new Camera();
  mCamera->SetPosition(0, 0, 3.0f);

  GLint posAttribLoc = glGetAttribLocation(program, "in_position");
  GLint colAttribLoc = glGetAttribLocation(program, "in_color");
  GLint normAttribLoc = glGetAttribLocation(program, "in_normal");
  GLint uvAttribLoc   = glGetAttribLocation(program, "in_uv");

  mAxis = new AxisMesh(posAttribLoc, colAttribLoc);
  mBoundingBox = new BoundingBoxMesh(posAttribLoc, colAttribLoc);

  mMeshGroup = new MeshGroup<Batch>(4, 4);
  mMeshGroup2 = new MeshGroup<Interleave>(4, 4);
  
  mMeshGroup->SetVertexAttribList({3, 3, 3, 2});

  mMeshGroup->AddRenderingPass({gloo::kNoAttrib, {posAttribLoc, true}, {colAttribLoc, true}, {uvAttribLoc, false}});

  // mMeshGroup->AddRenderingPass({{0, posAttribLoc}, {2, colAttribLoc}});
  // mMeshGroup->AddRenderingPass({{0, posAttribLoc}, {2, colAttribLoc}});

  // TODO: correct disabled attributes.

  mMeshGroup->Load( {nullptr, squareVertices, squareColors, nullptr}, nullptr);
  // mMeshGroup->Load(squareBuffer, indices);


  mMeshGroup2->SetVertexAttribList({3, 3});
  mMeshGroup2->AddRenderingPass({{posAttribLoc, true}, {colAttribLoc, true}});
  // mMeshGroup2->AddRenderingPass({{0, posAttribLoc}, {1, colAttribLoc}});
  mMeshGroup2->Load( {squareVertices, squareColors}, nullptr);
  
  // mMeshGroup2->Load(squareBufferInterleaved, indices);

  return true;
}

void MyModel::Idle()
{
  glutPostRedisplay();
}

void MyModel::Display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  static float blah_angle = 0.0;
  blah_angle += 0.01;

  mCamera->SetOnRendering();
  mCamera->SetUniformViewMatrix( mShaderProgram->GetVariableHandle("V") );

  gloo::Transform M;
  M.LoadIdentity();
  M.Translate(1.2f, 0.0f, 0.0);

  GLuint uniformLoc = glGetUniformLocation(mShaderProgram->GetHandle(), "M");
  M.SetUniform(uniformLoc);


  M.LoadIdentity();
  M.Translate(-1.2f, 0.0f, 0.0f);
  M.Rotate(blah_angle, 0, 0, 1);

  M.SetUniform(uniformLoc);

  mBoundingBox->Render();

  mMeshGroup2->Render();
  
  M.LoadIdentity();
  // M.Rotate(blah_angle, -1, 0, 0);
  M.SetUniform(uniformLoc);

  // squareVertices[5] *= 0.999;

  // mMeshGroup2->Update({squareVertices, squareColors, nullptr});

  // mAxis->Update(5*cos(blah_angle), 2*sin(3.1*blah_angle), 0.3*cos(blah_angle));
  mAxis->Render();

  // mMeshGroup->Render();

  glutSwapBuffers();
}

void MyModel::Reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  mCamera->SetOnReshape(0, 0, w, h);
  mCamera->SetUniformProjMatrix( mShaderProgram->GetVariableHandle("P") );
}

void MyModel::ActiveMouseMotion(const MouseEvent & mouseEvent)
{
  switch (mouseEvent.mMouseState.mModifier)
  {
    case MouseState::kCTRL:

    break;

    case MouseState::kSHIFT:

    break;

    case MouseState::kALT:

    break;
  
    default:
      if (mouseEvent.mMouseState.mLftButton == MouseState::kDown)
      {
        // std::cout << "Left button dragged.\n";
      }
      if (mouseEvent.mMouseState.mRgtButton == MouseState::kDown)
      {
        mCamera->Rotate(mouseEvent.mMouseState.mVelY/100.0f, 
                        mouseEvent.mMouseState.mVelX/100.0f,
                        0.0f);
        // std::cout << "Right button dragged.\n";
      }
      if (mouseEvent.mMouseState.mMidButton == MouseState::kDown)
      {
        // std::cout << "dy = " << mouseEvent.mMouseState.mVelY * 1e-2 << std::endl;
        mCamera->Translate(0.0f, 0.0f, -mouseEvent.mMouseState.mVelY * 1e-2);
        // std::cout << "Middle button dragged.\n";
      }
  }  // end switch.

}

void MyModel::PassiveMouseMotion(const MouseEvent & mouseEvent)
{
  // std::cout << "Coordinates: " << mouseState.mPosX << ", " << mouseState.mPosY << std::endl;
}

void MyModel::MouseButtonChange(const MouseEvent & mouseEvent)
{
  if (mouseEvent.mMouseState.mModifier == MouseState::kCTRL)
  {
    std::cout << "CTRL.\n";
  }
  if (mouseEvent.mMouseState.mModifier == MouseState::kSHIFT)
  {
    std::cout << "SHIFT.\n";
  }
  if (mouseEvent.mMouseState.mModifier == MouseState::kALT)
  {
    std::cout << "ALT.\n";
  }

  if (mouseEvent.mButton == MouseState::kLeft && mouseEvent.mButtonState == MouseState::kUp)
  {
    std::cout << "Left button pressed.\n";
  }
  if (mouseEvent.mButton == MouseState::kRight && mouseEvent.mButtonState == MouseState::kUp)
  {
    std::cout << "Right button pressed.\n";
  }
  if (mouseEvent.mButton == MouseState::kMiddle && mouseEvent.mButtonState == MouseState::kUp)
  {
    std::cout << "Middle button pressed.\n";
  }
}

void MyModel::KeyboardChange(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0);
    break;
  }
}

void MyModel::SpecialKeyboardChange(unsigned char key, int x, int y)
{

}