#include"FastChart.h" 
#include<QMouseEvent>
#include<qmatrix4x4>

FastChart::FastChart(QWidget* parent) : QOpenGLWidget(parent)
{
	std::unique_ptr<CandleLayer> layer = std::make_unique<CandleLayer>();
	m_candleLayer = layer.get();
	m_layers.push_back(std::move(layer));
}
FastChart::~FastChart()
{

}

void FastChart::initializeGL()
{
	initializeOpenGLFunctions();
	glClearColor(0.1f, 0.12f, 0.15f, 1.0f);

	for (const std::unique_ptr<IChartLayer>& layer : m_layers)
	{
		layer->initializeGL();
	}
}

void FastChart::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
 
	QMatrix4x4 mvp = calculateMvpMatrix();

	float halfX = m_cam.zoomX / 2.0f;
	float halfY = m_cam.zoomY / 2.0f;

	Viewport viewport;
    viewport.candleIndexMin = m_cam.x - halfX;
	viewport.candleIndexMax = m_cam.x + halfX;
	viewport.priceMin = m_cam.y - halfY;
	viewport.priceMax = m_cam.y + halfY;

	ChartContext context = { mvp, viewport, QPointF(), false };

	for (const std::unique_ptr<IChartLayer>& layer : m_layers)
	{
		layer->paintGL(context);
	}
} 
void FastChart::resizeGL(int w, int h)
{
	for (const std::unique_ptr<IChartLayer>& layer : m_layers)
	{
		layer->resizeGL(w, h);
	}
		
}

QMatrix4x4 FastChart::calculateMvpMatrix()
{
	QMatrix4x4 matrix;
	float halfX = m_cam.zoomX / 2.0f;
	float halfY = m_cam.zoomY / 2.0f;

	float left = m_cam.x - halfX;
	float right = m_cam.x + halfX;

	float bottom = m_cam.y - halfY;
	float top = m_cam.y + halfY;
	matrix.ortho(left, right, bottom, top, -1.0f, 1.0f);
	return matrix;
}


void FastChart::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_isDragging = true;
		m_lastMousePos = event->position();
	}
}

void FastChart::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isDragging)
	{
		QPointF delta = event->position() - m_lastMousePos;

		float candlesPerPixel = m_cam.zoomX / width();
		float pricePerPixel = m_cam.zoomY / height();

		m_cam.x -= delta.x() * candlesPerPixel; 
		m_cam.y += delta.y() * pricePerPixel;

		m_lastMousePos = event->position();
		update(); 	
	}
}
void FastChart::wheelEvent(QWheelEvent* event)
{
	if (event->angleDelta().y() > 0)
	{
		m_cam.zoomX *= 0.9f; 


	} else
	{
		m_cam.zoomX *= 1.1f; 	
	}

	if (m_cam.zoomX < 5.0f) m_cam.zoomX = 5.0f;

	update();
}

