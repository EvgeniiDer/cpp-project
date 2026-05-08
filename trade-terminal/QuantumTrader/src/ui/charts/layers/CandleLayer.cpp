
#include "CandleLayer.h"
#include <algorithm>


struct CandleVertexData
{
	float x, y;
	float r, g, b;
};

CandleLayer::CandleLayer()
{

}
CandleLayer::~CandleLayer()
{
	if (m_program)
	{
		delete m_program;
	}
}
const std::vector<Candle>& CandleLayer::getCandles()const
{
	return m_candles;
}
void CandleLayer::initializeGL()
{
	initializeOpenGLFunctions();
	initShaders();

	m_vao.create();
	m_vao.bind();


	m_vbo.create();
	m_vbo.bind();

	m_program->enableAttributeArray(0);
	m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, sizeof(CandleVertexData));

	m_program->enableAttributeArray(1);
	m_program->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 3, sizeof(CandleVertexData));

	m_vao.release();
	m_vbo.release();
	rebuildVBO();

}
void CandleLayer::paintGL(const chart::ChartContext& context)
{
	if (m_needRebuild)
	{
		rebuildVBO();
		m_needRebuild = false;
	}
	//----------------------------------------------------------
	if (m_vertexCount == 0 || !m_program || !m_program->isLinked()) return;

	m_program->bind();
	m_program->setUniformValue("mvp_matrix", context.mvpMatrix);

	m_vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
	m_vao.release();
	m_program->release();
}
void CandleLayer::setCandles(const std::vector<Candle>& candles)
{
	m_candles = candles;
	rebuildVBO();
}

void CandleLayer::updateLiveCnadle(const Candle& liveCandle)
{
	if (m_candles.empty())
	{
		m_candles.push_back(liveCandle);
	} else{
		if (m_candles.back().timestamp == liveCandle.timestamp)
		{
			m_candles.back() = liveCandle;
		} else
		{
			m_candles.push_back(liveCandle);
		}
	}
	m_needRebuild = true;
}

void CandleLayer::rebuildVBO()
{
	if (!m_vbo.isCreated())
	{
		return;
	}
	if (m_candles.empty())
	{
		m_vertexCount = 0;
		return;
	}

	std::vector<CandleVertexData> vertices;
	vertices.reserve(m_candles.size() * 18);

	const float bodyWidth = 0.8f;//ширина свечи
	const float wickWidth = 0.2f;// ширина фитиля

	for (size_t i = 0; i < m_candles.size(); ++i)
	{
		const Candle& candle = m_candles[i];
		bool isUp = candle.close >= candle.open;
		float r = isUp ? 0.0f : 1.0f;
		float g = isUp ? 1.0f : 0.0f;
		float b = 0.2f;

		float x = static_cast<float>(i);

		float yOpen = static_cast<float>(candle.open);
		float yClose = static_cast<float>(candle.close);
		float yHigh = static_cast<float>(candle.high);
		float yLow = static_cast<float>(candle.low);

		float yBodyTop = std::max(yOpen, yClose);
		float yBodyBottom = std::min(yOpen, yClose);

		if (yBodyTop == yBodyBottom)
		{
			yBodyTop += 0.05f;
		}

		auto addRect = [&](float left, float right, float bottom, float top)
			{
				vertices.push_back({ left,  bottom, r, g, b });
				vertices.push_back({ right, bottom, r, g, b });
				vertices.push_back({ left,  top,    r, g, b });

				vertices.push_back({ right, bottom, r, g, b });
				vertices.push_back({ right, top,    r, g, b });
				vertices.push_back({ left  ,top,    r, g, b });
			};
		addRect(x - bodyWidth / 2, x + bodyWidth / 2, yBodyBottom, yBodyTop);	
		addRect(x - wickWidth / 2, x + wickWidth / 2, yBodyTop, yHigh);       	
		addRect(x - wickWidth / 2, x + wickWidth / 2, yLow, yBodyBottom);
	}

	m_vao.bind();
	m_vbo.bind();
	m_vbo.allocate(vertices.data(), vertices.size() * sizeof(CandleVertexData));
	m_vbo.release();
	m_vao.release();

	m_vertexCount = vertices.size();
}
void CandleLayer::initShaders()
{
	m_program = new QOpenGLShaderProgram();
	const char* vShader = R"(
        #version 330 core
        layout(location = 0) in vec2 vertexPosition;
        layout(location = 1) in vec3 vertexColor;
        out vec3 fragmentColor;
        uniform mat4 mvp_matrix;
        void main() {
            gl_Position = mvp_matrix * vec4(vertexPosition, 0.0, 1.0);
            fragmentColor = vertexColor;
        }
    )";

	const char* fShader = R"(
        #version 330 core
        in vec3 fragmentColor;
        out vec4 color;
        void main() {
            color = vec4(fragmentColor, 1.0);
        }
    )";
	if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vShader))
	{
		qDebug() << "Vertex shader error:" << m_program->log();
	}
	if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fShader))
	{
		qDebug() << "Fragment shader error: " << m_program->log();
	}
	if (!m_program->link())
	{
		qDebug() << "Shader link error: " << m_program->log();
	}
}

