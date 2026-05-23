#pragma once

#include "IChartLayer.h"
#include "core/models/Candle.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>

struct CandleVertexData;
class CandleLayer : public IChartLayer
{
public:
	CandleLayer();
	~CandleLayer() override;
	const std::vector<Candle>& getCandles()const;
	void initializeGL() override;
	void paintGL(const chart::ChartContext& context) override;
	void resizeGL(int w, int h) override {};

	void setCandles(const std::vector<Candle>& candles);
	void updateLiveCandle(const Candle& liveCandle);
private:
	void initShaders();
	//void rebuildVBO();
	void prepareVisibleVertices(const chart::ChartContext& context, std::vector<CandleVertexData>& outVertices);

	QOpenGLShaderProgram* m_program = nullptr;
	QOpenGLVertexArrayObject m_vao;
	QOpenGLBuffer m_vbo;


	int m_vertexCount = 0;
	std::vector<Candle> m_candles;
	bool m_needRebuild = false;
};