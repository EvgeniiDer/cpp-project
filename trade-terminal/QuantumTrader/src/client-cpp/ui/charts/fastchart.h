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
class FastChart : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	enum class RenderMode
	{
		StrethToWindow,
		FreePanAdZoom
	};
	explicit FastChart(QWidget* parent = nullptr);
	~FastChart() override;
protected:
	void wheelEvent(QWheelEvent* event) override;
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;
	void contextMenuEvent(QContextMenuEvent* event) override;
private:
	float m_zoomFactor = 1.0f;
	float m_aspectRatio = 1.0f;
	void initShaders();
	void generationGridData();
	void generationGraphData();
	RenderMode m_renderMode = RenderMode::FreePanAdZoom;

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