#include"fastchart.h"

FastChart::FastChart(QWidget* parent)
	: QOpenGLWidget(parent)
{

}
FastChart::~FastChart()
{
	makeCurrent();

	m_vboGraph.destroy();
	m_vaoGraph.destroy();

	m_vboGrid.destroy();
	m_vaoGrid.destroy();
	
	delete m_program;
}
//Initialize Shaders
void FastChart::initShaders()
{
	m_program = new QOpenGLShaderProgram(this);
	if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/grid.vert"))
	{
		qCritical() << "Vertes shader compilation failed" << m_program->log();
		return;
	}
	if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/grid.frag"))
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
void FastChart::generationGraphData() //load Veretex Buffer Object Graph data
{
	std::vector<float> vertices;
	const int numberPoints = 200;
	vertices.reserve(numberPoints * 2);
	for (size_t i = 0; i < numberPoints; ++i)
	{
		float x = -1.0f + (i / (float)(numberPoints - 1)) * 2.0f;
		float y = -1.0f + (i / (float)(numberPoints - 1)) * 2.0f;

		vertices.push_back(x);
		vertices.push_back(y);
	}
	m_graphVertexCount = numberPoints;

	m_vboGraph.bind();
	m_vboGraph.allocate(vertices.data(), vertices.size() * sizeof(float));
	m_vboGraph.release();
}
void FastChart::generationGridData()
{
	std::vector<float> vertices;
	const int lines = 20;
	const float step = 2.0f / lines;
	float pos = -1.0f;

	for (int i = 0; i <= lines; ++i)
	{
		vertices.push_back(-1.0f);
		vertices.push_back(pos);
		vertices.push_back(1.0f);
		vertices.push_back(pos);
		
		vertices.push_back(pos);
		vertices.push_back(-1.0f);
		vertices.push_back(pos);
		vertices.push_back(1.0f);

		pos += step;
	}
	m_gridVertexCount = vertices.size() / 2;

	m_vboGrid.bind();
	m_vboGrid.allocate(vertices.data(), vertices.size() * sizeof(float));
	m_vboGrid.release();
}
void FastChart::initializeGL()
{
	initializeOpenGLFunctions();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	initShaders();

	m_vaoGrid.create();
	m_vaoGrid.bind();
	
	m_vboGrid.create();
	m_vboGrid.bind();

	m_program->enableAttributeArray(0);
	m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 0); //


	generationGridData();
	m_vaoGrid.release();
	

	m_vaoGraph.create();
	m_vaoGraph.bind();

	m_vboGraph.create();
	m_vboGraph.bind();

	m_program->enableAttributeArray(0);
	m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 0);

	
	generationGraphData();
	m_vaoGraph.release();

}
void FastChart::wheelEvent(QWheelEvent* event)
{
	int delta = event->angleDelta().y();//Vertical + -
	if (delta > 0)
	{
		m_zoomFactor *= 0.9f;
	}
	else if(delta < 0)
	{
		m_zoomFactor *= 1.1f;
	}
	generationGridData();
	update();

}
void FastChart::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	if (!m_program->isLinked() ||!m_program->bind())
	{
		return; 
	}
	
	QMatrix4x4 matrix;

	matrix.ortho(
		-1.0f * m_zoomFactor,
		1.0f * m_zoomFactor,
		-1.0f * m_zoomFactor,
		1.0f * m_zoomFactor,
		-1.0f,
		1.0f
	);
	m_program->setUniformValue("mvp_matrix", matrix);

	m_vaoGrid.bind();
	glDrawArrays(GL_LINES, 0, m_gridVertexCount);
	m_vaoGrid.release();

	if (m_graphVertexCount > 0)
	{
		m_vaoGraph.bind();
		glDrawArrays(GL_LINE_STRIP, 0, m_graphVertexCount);
		m_vaoGraph.release();
	}
}
void FastChart::resizeGL(int w, int h)
{
	Q_UNUSED(w);
	Q_UNUSED(h);
}
