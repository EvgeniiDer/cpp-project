#pragma once

#include<QSyntaxHighlighter>
#include<QTextCharFormat>
#include<QRegularExpression>

class CssHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT
public:
	explicit CssHighlighter(QTextDocument* parent = nullptr);
protected:
	void highlightBlock(const QString& text) override;
private:
	struct HighlightingRule
	{
		QRegularExpression pattern;
		QTextCharFormat format;
	};

	QVector<HighlightingRule> highlightingRules;

	QTextCharFormat keywordFormat;
	QTextCharFormat propertyFormat;
	QTextCharFormat valueFormat;
	QTextCharFormat commentFormat;
	
};
