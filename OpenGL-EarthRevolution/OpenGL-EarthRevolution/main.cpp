//
//  main.cpp
//  OpenGL-EarthRevolution
//
//  Created by 王海龙 on 2021/2/26.
//

#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

GLShaderManager shaderManager; //着色器管理器
GLMatrixStack   modelViewMatrixStack; //模型视图矩阵
GLMatrixStack   projectionMatrixStack; //投影矩阵
GLFrustum       viewFrustum;   //视景体
GLGeometryTransform transformPipeline; //几何图形变换管道
GLTriangleBatch  torusBath; //大球批次类
GLTriangleBatch  sphereBath;//小球批次类
GLBatch          floorBath; //地板

GLFrame    cameraFrame; //角色帧，照相机角色帧

//添加附加随机球
#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];

void SetupRC() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    shaderManager.InitializeStockShaders();
    //开启深度测试
    glEnable(GL_DEPTH_TEST);
    //设置地板的顶点数据
    floorBath.Begin(GL_LINES, 324);
    for (GLfloat x = -20.f; x<=20.0; x+=0.5) {
        floorBath.Vertex3f(x, -0.55f, 20.0f);
        floorBath.Vertex3f(x, -0.55f, -20.0f);
        floorBath.Vertex3f(20.0f, -0.55f, x);
        floorBath.Vertex3f(-20.0f, -0.55f, x);
    }
    floorBath.End();
    
    //设置大球模型
    gltMakeSphere(torusBath, 0.4, 40.0, 80.0);
    //设置小球
    gltMakeSphere(sphereBath, 0.1f, 13, 26);
    //6. 随机位置放置小球球
    for (int i = 0; i < NUM_SPHERES; i++) {
        
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        
        //在y方向，将球体设置为0.0的位置，这使得它们看起来是飘浮在眼睛的高度
        //对spheres数组中的每一个顶点，设置顶点数据
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
}

//进行调用以绘制场景
void RenderScene () {
    //1.颜色值(地板,大球,小球颜色)
    static GLfloat vFloorColor[] = {0.0f,1.0f,0.0f,1.0f};
    static GLfloat vTorusColor[] = {1.0f,0.0f,0.0f,1.0f};
    static GLfloat vSphereColor[] = { 0.0f, 0.0f, 1.0f, 1.0f};
    //基于时间动画
    static CStopWatch rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60;
    
    //2.清除颜色缓冲区和深度缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //压栈
    modelViewMatrixStack.PushMatrix();
    
    //.加入观察者 平移10步(地板,大球,小球,小小球)
    M3DMatrix44f mCamrea;
    cameraFrame.GetCameraMatrix(mCamrea);
    modelViewMatrixStack.PushMatrix(mCamrea);
    
    //3.绘制地面
    shaderManager.UseStockShader(GLT_SHADER_FLAT,transformPipeline.GetModelViewProjectionMatrix(),vFloorColor);
    floorBath.Draw();
    
    //获取光源位置
    M3DVector4f vLightPos = {0.0f,10.0f,5.0f,1.0f};
    //.使得大球位置平移(3.0)向屏幕里面
    modelViewMatrixStack.Translate(0.0f, 0.0f, -3.0f);
    //压栈(复制栈顶)
    modelViewMatrixStack.PushMatrix();
    //大球沿着Y轴自传
    modelViewMatrixStack.Rotate(yRot, 0.0f, 1.0f, 0.0f);
    //指定合适的着色器(点光源着色器)
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vTorusColor);
    torusBath.Draw();
    modelViewMatrixStack.PopMatrix();
    
    //画小球
    for (int i = 0; i<NUM_SPHERES; i++) {
        modelViewMatrixStack.PushMatrix();
        modelViewMatrixStack.MultMatrix(spheres[i]);//模型矩阵和小球的位置相乘
        //绘制小球
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vSphereColor);
        sphereBath.Draw();
        modelViewMatrixStack.PopMatrix();
    }
    
    //篮球围绕大红球旋转
    modelViewMatrixStack.Rotate(yRot * -2, 0, 1, 0);
    modelViewMatrixStack.Translate(0.8f, 0.0, 0.0);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vSphereColor);
    sphereBath.Draw();
    
    modelViewMatrixStack.PopMatrix(); //个人理解：每PushMatrix 一次其实往矩阵堆栈里压一个矩阵去绘制一个图形，该图形绘制完了就要PopMatrix，否则在绘制新的图形就会受到影响
    modelViewMatrixStack.PopMatrix();
    
    
    //4.执行缓冲区交换
    glutSwapBuffers();
    glutPostRedisplay();
    
}

//屏幕更改大小或已初始化
void ChangeSize (int width, int height) {
    glViewport(0, 0, width, height);
    //创建投影矩阵
    viewFrustum.SetPerspective(35.0f, float(width)/float(height), 1.0f, 100.0f);
    //viewFrustum.GetProjectionMatrix()  获取viewFrustum投影矩阵
    //并将其加载到投影矩阵堆栈上
    projectionMatrixStack.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    
    //3.设置变换管道以使用两个矩阵堆栈（变换矩阵modelViewMatrix ，投影矩阵projectionMatrix）
    //初始化GLGeometryTransform 的实例transformPipeline.通过将它的内部指针设置为模型视图矩阵堆栈 和 投影矩阵堆栈实例，来完成初始化
    //当然这个操作也可以在SetupRC 函数中完成，但是在窗口大小改变时或者窗口创建时设置它们并没有坏处。而且这样可以一次性完成矩阵和管线的设置。
    transformPipeline.SetMatrixStacks(modelViewMatrixStack, projectionMatrixStack);
}

void SpeacialKeys(int key,int x,int y){
    //移动步长
    float linear = 0.1f;
    //旋转度数
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        //MoveForward 平移
        cameraFrame.MoveForward(linear);
    }
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        //RotateWorld 旋转
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
    }
    

}


int main(int argc, char* argv[]){
    gltSetWorkingDirectory(argv[0]);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
    
    glutCreateWindow("OpenGL SphereWorld");
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutSpecialFunc(SpeacialKeys);
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    
    SetupRC();
    glutMainLoop();
    return 0;
}
