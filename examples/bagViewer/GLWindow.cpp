#include "GLWindow.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>


GLWindow::GLWindow(QWindow* parent): QWindow(parent)
    ,updatePending(false),animating(false),context(0),device(0)
{
    setSurfaceType(QWindow::QSurface::OpenGLSurface);
}

GLWindow::~GLWindow()
{
    delete device;
}

void GLWindow::initialize()
{

}


void GLWindow::render(QPainter *painter)
{
    Q_UNUSED(painter)
}

void GLWindow::render()
{
    if (!device)
        device = new QOpenGLPaintDevice;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    device->setSize(size());
    
    QPainter painter(device);
    render(&painter);
}

void GLWindow::renderLater()
{
    if(!updatePending)
    {
        updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool GLWindow::event(QEvent* event)
{
    switch(event->type()){
        case QEvent::UpdateRequest:
            updatePending = false;
            renderNow();
            return true;
        default:
            return QWindow::event(event);
    }
}

void GLWindow::exposeEvent(QExposeEvent* event)
{
    Q_UNUSED(event);

    if(isExposed())
        renderNow();
}

void GLWindow::renderNow()
{
    if(!isExposed())
        return;
    
    bool needInit = false;
    
    if(!context){
        context = new QOpenGLContext(this);
        context->setFormat(requestedFormat());
        context->create();
        needInit = true;
    }
    
    context->makeCurrent(this);
    
    if(needInit){
        initializeOpenGLFunctions();
        initialize();
    }
    
    render();
    
    context->swapBuffers(this);
    if(animating)
        renderLater();
}

void GLWindow::setAnimating(bool a)
{
    animating = a;
    if(animating)
        renderLater();
}

