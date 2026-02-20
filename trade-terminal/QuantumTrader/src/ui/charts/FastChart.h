#pragma

#include<QOpenGLWidget>
#include<QOpenGlFunctions>
#include<vector>
#include<memory>
#include "layers/CandleLayer.h"
#include "layers/GridLayer.h"
#include "layers/AxisLayer.h"




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

		
protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;

	void wheelEvent(QWheelEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	QMatrix4x4 calculateMvpMatrix();

	struct CameraStat
	{
		float x = 0.0f;
		float y = 0.0f;
		float zoomX = 50.0f;
		float zoomY = 100.0f;
	} m_cam;
	
	QPointF m_lastMousePos;
	bool m_isDragging = false;

	std::vector<std::unique_ptr<IChartLayer>> m_layers;

	//LAYERS!!! 
	CandleLayer* m_candleLayer = nullptr;
	GridLayer* m_gridLayer = nullptr;
	AxisLayer* m_axisLayer = nullptr;
};



