#include"fastchart.h"

FastChart::FastChart(QWidget* parent)
	: QOpenGLWidget(parent)
{
}
void FastChart::initShaders()
{
	m_program = new QOpenGLShaderProgram(this);
	if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/grid.vert"))
	{
		qCritical() << "Vertes shader compilation failed" << m_program->log();
		return;
	}
	if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ";/shaders/grid.ver"));
	{
		qCritical() << "Fragment shader compilation failed" << m_program->log();
		return;
	}
	if (!m_program->link())
	{
		qCritical() << "Shader program linking failed: " << m_program->log();
		return;
	}
}
