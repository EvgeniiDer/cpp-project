#pragma once

#include"IChartLayer.h"
#include<QOpenGLShaderProgram>
#include<QOpenGLBuffer>
#include<QOpenGLVertexArrayObject>
#include<QColor>

struct GridVertexData
{
	float x, y;
	float alph; 
};
class GridLayer : public IChartLayer
{
public:
	GridLayer();
	~GridLayer() override;

	void initializeGL() override;
	void paintGL(const ChartContext& context) override;

	void setGridColor(const QColor& color);
private:
	void initShaders();
	void updateGrid(const ChartContext& context);

	QOpenGLShaderProgram* m_program = nullptr;
	QOpenGLVertexArrayObject m_vao;
	QOpenGLBuffer m_vbo;

	int m_vertexCount = 0;

	float m_lastPriceMin = 0, m_lastPriceMax = 0;
	float m_lastIndexMin = 0, m_lastIndexMax = 0;
	
	QColor m_gridColor;
};