#ifndef FASTCHART_H
#define FASTCHART_H

#include<QContextMenuEvent>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QWidget>
#include <vector>
#include <QWheelEvent>
#include <cmath>
#include<QPointF>
class FastChart : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	enum class RenderMode
	{
		StretchToWindow,
		FreePanAndZoom
	};
	explicit FastChart(QWidget* parent = nullptr);
	~FastChart() override;
protected:
	void wheelEvent(QWheelEvent* event) override;
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;
	void contextMenuEvent(QContextMenuEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
private:
	float m_zoomFactor = 1.0f;
	float m_aspectRatio = 1.0f;
	float m_panX = 0.0f;
	float m_panY = 0.0f;
	QPointF m_lastMousePos;
	void initShaders();
	void generationGridData();
	void generationGraphData();
	RenderMode m_renderMode = RenderMode::FreePanAndZoom;

	QOpenGLShaderProgram* m_program;

	QOpenGLBuffer m_vboGrid;
	QOpenGLVertexArrayObject m_vaoGrid;
	int m_gridVertexCount = 0;

	QOpenGLBuffer m_vboGraph;
	QOpenGLVertexArrayObject m_vaoGraph;
	int m_graphVertexCount = 0;

	void generateTestGraphData();
};

#endif // FASTCHART_H