#ifndef FASTCHART_H
#define FASTCHART_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QWidget>
#include <vector>

class FastChart : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	explicit FastChart(QWidget* parent = nullptr);
	~FastChart() override;
protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;
private:
	void initShaders();
	void generationGridData();
	void generationGraphData();

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