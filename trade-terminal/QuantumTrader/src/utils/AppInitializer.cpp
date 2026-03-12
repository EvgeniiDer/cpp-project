#include"AppInitializer.h"
#include<QCoreApplication>
#include<QSurfaceFormat>


void AppInitializer::setupGraphiccs()
{
   QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  // QCoreApplication::setAttribute(Qt::AA_NativeWindows);
   QSurfaceFormat format;
   format.setDepthBufferSize(24);
   format.setStencilBufferSize(8);
   format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
   format.setRenderableType(QSurfaceFormat::OpenGL);
   QSurfaceFormat::setDefaultFormat(format);
}
