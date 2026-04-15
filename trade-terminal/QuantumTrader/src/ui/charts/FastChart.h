#pragma once
#include <QOpenGLWidget>
#include <QOpenGlFunctions>
#include <vector>
#include <memory>
#include <atomic>
#include "ChartTypes.h" 

class CandleLayer;
class GridLayer;
class AxisLayer;
class CrosshairLayer;
class IChartLayer;
class LinkManager;
struct Candle;

class FastChart : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	explicit FastChart(QWidget* parent = nullptr);
	~FastChart() override;
	CandleLayer* getCandleLayer()const
	{
		return m_candleLayer;
	}
	void loadData(const std::vector<Candle>& dat);
protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;

	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;

	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	void autoScaleY();
	chart::DragState getZoneAt(const QPointF& pos);
	chart::DragState m_dragState = chart::DragState::None;
	QMatrix4x4 calculateMvpMatrix();

	chart::ChartSettings m_settings;
	chart::CameraStat m_cam;
	QPointF m_mousePixelPos;
	bool m_isMouseInside = false;

	QPointF m_lastMousePos;
	bool m_isDragging = false;

	std::vector<std::unique_ptr<IChartLayer>> m_layers;
	
	std::atomic<bool> m_isInitialized{ false };
	std::atomic<bool> m_isLocked{ false };


	//LAYERS!!! 
	CandleLayer* m_candleLayer = nullptr;
	GridLayer* m_gridLayer = nullptr;
	AxisLayer* m_axisLayer = nullptr;
	CrosshairLayer* m_crosshairLayer = nullptr;
};



