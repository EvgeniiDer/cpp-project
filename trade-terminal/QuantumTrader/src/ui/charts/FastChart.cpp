#include"FastChart.h" 
#include<qpainter>
#include<QMouseEvent>
#include<qmatrix4x4>
#include"layers/CandleLayer.h"
#include"layers/GridLayer.h"
#include"layers/AxisLayer.h"
#include"layers/CrosshairLayer.h"
#include"ChartTypes.h"
#include"../../core/managers/LinkManager.h"
#include"../src/core/managers/MarketDataManager.h"
#include"../src/core/network/common/NetworkTypes.h"
FastChart::FastChart(QWidget* parent) : QOpenGLWidget(parent)
{
	this->setMouseTracking(true);
	std::unique_ptr<GridLayer> gridLayer = std::make_unique<GridLayer>();
	m_gridLayer = gridLayer.get();// Корчое возвращает нам указатель на кучу где храниться этот обьект замена если бы мы обращались бы к векторму к которому пердаеться управление как m_layers[0]!!!!! Для упрощения пометка использвать в дальнешем
	m_layers.push_back(std::move(gridLayer));

	std::unique_ptr<CandleLayer> candleLayer = std::make_unique<CandleLayer>();
	m_candleLayer = candleLayer.get();
	m_layers.push_back(std::move(candleLayer));

	std::unique_ptr<AxisLayer> axisLayer = std::make_unique<AxisLayer>();
	m_axisLayer = axisLayer.get();
	m_layers.push_back(std::move(axisLayer));
	
	std::unique_ptr<CrosshairLayer> crosshairLayer = std::make_unique<CrosshairLayer>();
	m_crosshairLayer = crosshairLayer.get();
	m_layers.push_back(std::move(crosshairLayer));
}
FastChart::~FastChart()
{
	makeCurrent();
	m_layers.clear();
	doneCurrent();
}

void FastChart::initializeGL()
{
	initializeOpenGLFunctions();
	glClearColor(0.1f, 0.12f, 0.15f, 1.0f);

	for (const std::unique_ptr<IChartLayer>& layer : m_layers)
	{
		layer->initializeGL();
	}
	m_isInitialized = true;
}

void FastChart::paintGL()
{
	if (!m_isInitialized || !isValid() || !context())
	{
		return;
	}
	qDebug() << "[PaintGL Check]"
		<< "Cam(X,Y):" << m_cam.x << m_cam.y
		<< "Zoom(X,Y):" << m_cam.zoomX << m_cam.zoomY
		<< "Candles count:" << (m_candleLayer ? m_candleLayer->getCandles().size() : 0);
	glClear(GL_COLOR_BUFFER_BIT);
 
	QMatrix4x4 mvp = calculateMvpMatrix();

	float halfX = m_cam.zoomX / 2.0f;
	float halfY = m_cam.zoomY / 2.0f;

	chart::Viewport viewport;
    viewport.candleIndexMin = m_cam.x - halfX;
	viewport.candleIndexMax = m_cam.x + halfX;
	viewport.priceMin = m_cam.y - halfY;
	viewport.priceMax = m_cam.y + halfY;

	chart::ChartContext context = {
		mvp,	  //const QMatrix4x4
		viewport, //const Viewport
		m_mousePixelPos,//QPointF mouseChartPos
		m_isMouseInside,	  //bool isMouseOver
		nullptr,  //QPainter* painter
		width(),  //int widgetWidth
		height(),  //int widgetHeight
		m_settings.priceAxisWidth, //float priceAxisWidth
		m_settings.timeAxisHeight  //float timeAxisHeight
	};

	for (const std::unique_ptr<IChartLayer>& layer : m_layers)
	{
		layer->paintGL(context);
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	context.painter = &painter;

	for (const std::unique_ptr<IChartLayer>& layer : m_layers)
	{
		layer->paintUI(context);
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
	qDebug() << "[MVP Matrix Check]"
		<< "Ortho bounds: L:" << left << "R:" << right
		<< "B:" << bottom << "T:" << top;
	matrix.ortho(left, right, bottom, top, -1.0f, 1.0f);
	return matrix;
}
chart::DragState FastChart::getZoneAt(const QPointF& pos)
{
	float x = pos.x();
	float y = pos.y();

	float axisWidth = m_settings.priceAxisWidth;
	float axisHeight = m_settings.timeAxisHeight;
	
	float bordeX = this->width() - axisWidth;

	if(std::abs(x - bordeX) <= 5.0f)
	{
		return chart::DragState::ResizePriceAxis;
	}

	if (x >= this->width() - axisWidth) 
		return chart::DragState::PriceAxis;
	if (y >= this->height() - axisHeight) 
		return chart::DragState::DateAxis;
	return chart::DragState::ChartArea;
}
void FastChart::mouseReleaseEvent(QMouseEvent* evetn)
{
	m_dragState = chart::DragState::None;
}
void FastChart::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragState = getZoneAt(event->position());
		m_lastMousePos = event->position();
	}
}

void FastChart::mouseMoveEvent(QMouseEvent* event)
{

	m_mousePixelPos = event->position();
	update();
	if (m_dragState == chart::DragState::None)
	{
		chart::DragState hoverZone = getZoneAt(event->position());
		if (hoverZone == chart::DragState::ResizePriceAxis)
		{
			this->setCursor(Qt::SplitHCursor);
		} else
		{
			this->unsetCursor();
		}
		return;
	}
	
	QPointF delta = event->position() - m_lastMousePos;
	if (m_dragState == chart::DragState::ResizePriceAxis)
	{
		int newWidth = this->width() - event->position().x();

		if (newWidth < 40) newWidth = 40;
		if (newWidth > 200) newWidth = 200;
		
		m_settings.priceAxisWidth = static_cast<float>(newWidth);
	}
	if (m_dragState == chart::DragState::PriceAxis)
	{
		float zoomFactor = 1.0f + (delta.y() * 0.01f);
		m_cam.zoomY *= zoomFactor;
		
		if (m_cam.zoomY < 1.0f) m_cam.zoomY = 1.0f;
	}
	else if (m_dragState == chart::DragState::DateAxis)
	{
		float zoomFactor = 1.0f + (delta.x() * 0.01f);
		m_cam.zoomX *= zoomFactor;
		
		if (m_cam.zoomX < 5.0f) m_cam.zoomX = 5.0f;
	}
	else if (m_dragState == chart::DragState::ChartArea)
	{

		bool isCtrlPressed = event->modifiers() & Qt::ControlModifier;
		if (isCtrlPressed)
			{
				float zoomFactor = 1.0f + (delta.y() * 0.01f);
				m_cam.zoomY *= zoomFactor;
				if (m_cam.zoomY < 1.0f) m_cam.zoomY = 1.0f;
			}
		else
			{
				float candlesPerPixel = m_cam.zoomX / width();
				float pricePerPixel = m_cam.zoomY / height();

				m_cam.x -= delta.x() * candlesPerPixel; 
				m_cam.y += delta.y() * pricePerPixel;
			}
	}
	m_lastMousePos = event->position();
	update(); 	
}
void FastChart::wheelEvent(QWheelEvent* event)
{
	int delta = event->angleDelta().y();
	if (delta == 0) return;
	
	float zoomInFactor = 0.9f;
	float zoomOutFactor = 1.1f;
	float zoomFactor = (delta > 0) ? zoomInFactor : zoomOutFactor;
	
	chart::DragState hoverZone = getZoneAt(event->position());
	if (hoverZone == chart::DragState::PriceAxis)
	{
		m_cam.zoomY *= zoomFactor;
		if (m_cam.zoomY < 1.0f) m_cam.zoomY = 1.0f;
	}
	else if (hoverZone == chart::DragState::DateAxis)
	{
		m_cam.zoomX *= zoomFactor;
		if (m_cam.zoomX < 5.0f) m_cam.zoomX = 5.0f;
	}
	else if(hoverZone == chart::DragState::ChartArea)
	{
		bool isCtrlPressed = event->modifiers() & Qt::ControlModifier;
		if (isCtrlPressed)
		{
			if (delta > 0)
			{
				m_cam.zoomY *= zoomInFactor;
			}
			else
			{
				m_cam.zoomY *= zoomOutFactor;
			}
			if (m_cam.zoomY < 1.0f) m_cam.zoomY = 1.0f;
		} 
		else
		{
			if (delta > 0)
			{
				m_cam.zoomX *= zoomInFactor;
			}
			else
			{
				m_cam.zoomX *= zoomOutFactor;
			}
			if (m_cam.zoomX < 5.0f) m_cam.zoomX = 5.0f;
		}
	}

	update();
}

void FastChart::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		chart::DragState clickZone = getZoneAt(event->position());
		if (clickZone == chart::DragState::PriceAxis || clickZone == chart::DragState::ResizePriceAxis)
		{
			autoScaleY();
		}
	}
}
void FastChart::autoScaleY()
{
	const std::vector<Candle>& candles = m_candleLayer->getCandles();
	if (candles.empty()) return;

	float halfX = m_cam.zoomX / 2.0f;
	int startIndex = std::max(0, static_cast<int>(m_cam.x - halfX));
	int endIndex = std::min(static_cast<int>(candles.size()) - 1, static_cast<int>(m_cam.x + halfX));

	if (startIndex > endIndex)return;

	float minPrice = std::numeric_limits<float>::max();
	float maxPrice = std::numeric_limits<float>::min();

	for(int i = startIndex; i <= endIndex; ++i)
	{
		if (candles[i].low < minPrice) minPrice = candles[i].low;
		if (candles[i].high > maxPrice) maxPrice = candles[i].high;
	}

	if (minPrice == std::numeric_limits<float>::max()) return;

	m_cam.y = (maxPrice + minPrice) / 2.0f;

	float height = maxPrice - minPrice;
	if (height == 0.0f) height = 1.0f;
	m_cam.zoomY = height * 1.1f;
	
	qDebug() << "[AutoScale] Success!"
		<< "MinPrice:" << minPrice << "MaxPrice:" << maxPrice
		<< "New CamY:" << m_cam.y << "New ZoomY:" << m_cam.zoomY;
	update();
}

void FastChart::enterEvent(QEnterEvent* event)
{
	m_isMouseInside = true;
	QWidget::enterEvent(event);
}
void FastChart::leaveEvent(QEvent* event)
{
	m_isMouseInside = false;
	update();
	QWidget::leaveEvent(event);
}
void FastChart::loadData(const std::vector<Candle>& data)
{
	if (m_candleLayer)
	{
		m_candleLayer->setCandles(data);
		if (!data.empty() && !m_isHistoryLoaded)
		{
			m_cam.x = data.size() - 50.0f;
			autoScaleY();
			
			m_isHistoryLoaded = true;
		}
		update();
	}
}
void FastChart::setContext(MarketDataManager* manager, const QString& exchangeName, const QString& symbol)
{
	m_dataManager = manager;
	m_exchangeName = exchangeName;
	m_symbol = symbol;
	m_isHistoryLoaded = false;

	QObject::connect(m_dataManager, &MarketDataManager::candlesUpdated, this, &FastChart::onCandlesReceived);
	m_dataManager->requestHistory(m_exchangeName, m_symbol, ChartInterval(ChartInterval::Unit::Minute, 1), 90000);//RestApi request
	m_dataManager->subcribeToStream(m_exchangeName, m_symbol);//WebSocket request
}

void FastChart::onCandlesReceived(const QString& exchangeName, const QString& symbol, const std::vector<Candle>& candles)
{
	if (symbol != m_symbol || exchangeName != m_exchangeName)
	{
		return;
	}
	if (candles.size() == 1)
	{
		m_candleLayer->updateLiveCnadle(candles[0]);
	}
	else
	{
		qDebug() << "[FastChart] Painting: " << symbol;
		this->loadData(candles);
	}
	this->update();
}
