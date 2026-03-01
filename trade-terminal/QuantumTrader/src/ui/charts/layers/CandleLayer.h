#pragma once

#include "IChartLayer.h"
#include "core/models/Candle.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>

class CandleLayer : public IChartLayer
{
public:
	CandleLayer();
	~CandleLayer() override;
	const std::vector<Candle>& getCandles()const;
	void initializeGL() override;
	void paintGL(const ChartContext& context) override;
	void resizeGL(int w, int h) override {};

	void setCandles(const std::vector<Candle>& candles);
private:
	void initShaders();
	void rebuildVBO();

	QOpenGLShaderProgram* m_program = nullptr;
	QOpenGLVertexArrayObject m_vao;
	QOpenGLBuffer m_vbo;


	int m_vertexCount = 0;
	std::vector<Candle> m_candles;
};