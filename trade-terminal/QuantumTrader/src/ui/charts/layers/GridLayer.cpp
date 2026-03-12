#include"GridLayer.h"
#include<cmath>


GridLayer::GridLayer() : m_vbo(QOpenGLBuffer::VertexBuffer),
                         m_gridColor(100, 100, 100) {}
GridLayer::~GridLayer()
{
	delete m_program;
}

void GridLayer::initializeGL()
{
	initializeOpenGLFunctions();
	initShaders();
	m_vao.create();
    m_vao.bind();
    
    m_vbo.create();
    m_vbo.bind();

    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, sizeof(GridVertexData));
    
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 1, sizeof(GridVertexData));

    m_vao.release();
    m_vbo.release();
    
}
void GridLayer::paintGL(const chart::ChartContext& context)
{
    updateGrid(context);
    
    if (m_vertexCount == 0 || !m_program || !m_program->isLinked()) return;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_program->bind();
    m_program->setUniformValue("mvp_matrix", context.mvpMatrix);
    m_program->setUniformValue("gridColor", m_gridColor);

    m_vao.bind();
    glDrawArrays(GL_LINES, 0, m_vertexCount);

    m_vao.release();
    m_program->release();
    glDisable(GL_BLEND);

}
void GridLayer::updateGrid(const chart::ChartContext& context)
{
    const auto& vp = context.viewport;
    if (qFuzzyCompare(m_lastPriceMin, vp.priceMin) &&
        qFuzzyCompare(m_lastPriceMax, vp.priceMax) &&
        qFuzzyCompare(m_lastIndexMin, vp.candleIndexMin) &&
        qFuzzyCompare(m_lastIndexMax, vp.candleIndexMax))
    {
        return;
    }

    m_lastPriceMin = vp.priceMin;
    m_lastPriceMax = vp.priceMax;
    m_lastIndexMin = vp.candleIndexMin;
    m_lastIndexMax = vp.candleIndexMax;

    std::vector<GridVertexData> vertices;

    float rangeY = vp.height();// vp.priceMax - vp.priceMin
    if (rangeY <= 0.00001f) rangeY = 1.0f; 
    float stepY = std::pow(10.0f, std::floor(std::log10(rangeY / 5.0f)));
    if (stepY <= 0.00001f) stepY = 1.0f;

    float startY = std::floor(vp.priceMin / stepY) * stepY;

    for (float y = startY; y <= vp.priceMax; y += stepY)
    {
        vertices.push_back({ vp.candleIndexMin, y, 0.2f });
        vertices.push_back({ vp.candleIndexMax, y, 0.2f });
    }

    float rangeX = vp.width();
    float stepX = std::max(1.0f, std::pow(10.0f, std::floor(std::log10(rangeX / 8.0f))));

    float startX = std::floor(vp.candleIndexMin / stepX) * stepX;

    for (float x = startX; x <= vp.candleIndexMax; x += stepX)
    {
        vertices.push_back({ x, vp.priceMin, 0.2f });
        vertices.push_back({ x, vp.priceMax, 0.2f });
    }

    m_vbo.bind();
    m_vbo.allocate(vertices.data(), vertices.size() * sizeof(GridVertexData));
    m_vbo.release();

    m_vertexCount = static_cast<int>(vertices.size());
}
void GridLayer::setGridColor(const QColor& color)
{
    m_gridColor = color;
}
void GridLayer::initShaders()
{
	m_program = new QOpenGLShaderProgram();
    const char* vShader = R"(
        #version 330 core
        layout(location = 0) in vec2 vertexPosition;
        layout(location = 1) in float alpha;
        out float vAlpha;
        uniform mat4 mvp_matrix;
        void main() {
            gl_Position = mvp_matrix * vec4(vertexPosition, 0.0, 1.0);
            vAlpha = alpha;
        }
    )";
    const char* fShader = R"(
        #version 330 core
        in float vAlpha;
        uniform vec3 gridColor;
        out vec4 color;
        void main() {
            color = vec4(gridColor, vAlpha); 
        }
    )";
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vShader))
    {
        qDebug() << "GridLayer Vertex shader error:" << m_program->log();
    }
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fShader))
    {
		qDebug() << "GridLayer Fragment shader error:" << m_program->log();
    }
    if (!m_program->link())
    {
		qDebug() << "GridLayer Shader program link error:" << m_program->log();
    }
}