#include"fastchart.h"
#include <QMenu>
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
		//to NDC
}
void FastChart::contextMenuEvent(QContextMenuEvent* event)
{
	QMenu contextMenu(this);

	QAction* stretchToWindowAction = contextMenu.addAction("Streth To Window");
	QAction* freeToWindowAction = contextMenu.addAction("Free to Window");

	stretchToWindowAction->setCheckable(true);
	freeToWindowAction->setCheckable(true);

	if (m_renderMode == RenderMode::StretchToWindow)
	{
		stretchToWindowAction->setChecked(true);
	} else
	{
		freeToWindowAction->setChecked(true);
	}

	connect(stretchToWindowAction, &QAction::triggered, this, [this]()
		{
			m_renderMode = RenderMode::StretchToWindow;
			generationGridData();
			update();
		});
	connect(freeToWindowAction, &QAction::triggered, this, [this]()
		{
			m_renderMode = RenderMode::FreePanAndZoom;
			generationGridData();
			update();
		});
	contextMenu.exec(event->globalPos());
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

	const int targetLineCount = 20;

	float visibleHeight = 2.0f * m_zoomFactor;

	float roughStep = visibleHeight / targetLineCount;
	float stepPower = std::pow(10.0f, std::floor(std::log10(roughStep)));
	float step = roughStep / stepPower;
	if (step < 1.5f) step = 1.0f;
	else if (step < 3.5f) step = 2.0f;
	else if (step < 7.5f) step = 5.0f;
	else step = 10.0f;
	step *= stepPower;

	float right, left, top, bottom;
	
	if (m_renderMode == RenderMode::FreePanAndZoom)
	{
		right = m_panX + m_zoomFactor * m_aspectRatio;
		left = m_panX - m_zoomFactor * m_aspectRatio;
		top = m_panY + m_zoomFactor;
		bottom = m_panY - m_zoomFactor;
	} 
	else//(m_renderMode == RenderMode::StrethToWindow)
	{
		right = m_zoomFactor * m_aspectRatio;
		left = -right;
		top = m_zoomFactor;
		bottom = -top;
	}

	float startX = std::floor(left / step) * step;
	float startY = std::floor(bottom / step) * step;

	for (float x = startX; x <= right; x += step)
	{
		vertices.push_back(x); vertices.push_back(bottom);
		vertices.push_back(x); vertices.push_back(top);
	}
	for (float y = startY; y <= top; y += step)
	{
		vertices.push_back(left);  vertices.push_back(y);
		vertices.push_back(right); vertices.push_back(y);
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


	m_vaoGrid.release();
	m_vboGrid.release();

	m_vaoGraph.create();
	m_vaoGraph.bind();

	m_vboGraph.create();
	m_vboGraph.bind();

	m_program->enableAttributeArray(0);
	m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 0);

	
	generationGraphData();
	m_vaoGraph.release();


	generationGridData();
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
	
	if (m_renderMode == RenderMode::FreePanAndZoom)
	{
		QMatrix4x4 grid_matrix;
		grid_matrix.ortho(-1.0f * m_zoomFactor * m_aspectRatio, 1.0f * m_zoomFactor * m_aspectRatio, -1.0f * m_zoomFactor, 1.0f * m_zoomFactor, -1.0f, 1.0f);
		grid_matrix.translate(-m_panX, -m_panY, 0);

		QMatrix4x4 chart_matrix;
		chart_matrix.ortho(-1.0f * m_zoomFactor, 1.0f * m_zoomFactor, -1.0f * m_zoomFactor, 1.0f * m_zoomFactor, -1.0f, 1.0f);
		chart_matrix.translate(-m_panX, -m_panY, 0);

		m_program->setUniformValue("mvp_matrix", grid_matrix);
		m_vaoGrid.bind();
		glDrawArrays(GL_LINES, 0, m_gridVertexCount);
		m_vaoGrid.release();

		if (m_graphVertexCount > 0)
		{
			m_program->setUniformValue("mvp_matrix", chart_matrix);
			m_vaoGraph.bind();
			glDrawArrays(GL_LINE_STRIP, 0, m_graphVertexCount);
			m_vaoGraph.release();
		}
	} else 	{
		QMatrix4x4 matrix;
		matrix.ortho(-1.0f * m_zoomFactor * m_aspectRatio, 1.0f * m_zoomFactor * m_aspectRatio, -1.0f * m_zoomFactor, 1.0f * m_zoomFactor, -1.0f, 1.0f);
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
	m_program->release();
}
void FastChart::mousePressEvent(QMouseEvent* event)
{
	if (m_renderMode != RenderMode::FreePanAndZoom)
	{
		return;
	}
	if (event->buttons() & Qt::LeftButton)
	{
		m_lastMousePos = event->position();
	}
}
void FastChart::mouseMoveEvent(QMouseEvent* event)
{
	if (m_renderMode != RenderMode::FreePanAndZoom)
	{
		return;
	}
	if (event->buttons() & Qt::LeftButton)
	{
		QPointF delta = event->position() - m_lastMousePos;

		float worldDeltaX = delta.x() * 2.0f * m_zoomFactor * m_aspectRatio / width();
		float worldDeltaY = delta.y() * 2.0f * m_zoomFactor / height();

		m_panX -= worldDeltaX;
		m_panY += worldDeltaY; 
		
		m_lastMousePos = event->position();
		generationGridData();
		update();
	}
}
void FastChart::resizeGL(int w, int h)
{
	if (h == 0)
	{
		h = 1;
	}
	m_aspectRatio = static_cast<float>(w) / static_cast<float>(h);
	generationGridData();
	update();
}
