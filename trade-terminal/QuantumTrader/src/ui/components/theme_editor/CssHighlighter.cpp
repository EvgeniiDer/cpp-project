#include "CssHighlighter.h"
#include<QDebug>

CssHighlighter::CssHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) 
{
	HighlightingRule rule;

	keywordFormat.setForeground(QColor("#569CD6"));//blue
	keywordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression(R"(\bads--[A-Za-z0-9_]+\b)");
    rule.format = keywordFormat;
    highlightingRules.append(rule);

    // 2. Формат для CSS свойств (все, что до двоеточия)
    propertyFormat.setForeground(QColor("#9CDCFE")); // Светло-голубой
    rule.pattern = QRegularExpression(R"(\b[A-Za-z0-9-]+\s*(?=:))");
    rule.format = propertyFormat;
    highlightingRules.append(rule);

    // 3. Формат для значений (цвета #hex)
    valueFormat.setForeground(QColor("#CE9178")); // Оранжевый/Терракотовый
    rule.pattern = QRegularExpression(R"(#[A-Fa-f0-9]{6}|#[A-Fa-f0-9]{3})");
    rule.format = valueFormat;
    highlightingRules.append(rule);

    // 4. Формат для значений (числа с px, %)
    rule.pattern = QRegularExpression(R"(\b\d+(?:px|%|em|rem)\b)");
    rule.format = valueFormat;
    highlightingRules.append(rule);

    // 5. Формат для комментариев /* ... */
    commentFormat.setForeground(QColor("#6A9955")); // Зеленый
    rule.pattern = QRegularExpression(R"(/\*.*\*/)");
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void CssHighlighter::highlightBlock(const QString& text)  
{
    for (const HighlightingRule& rule : qAsConst(highlightingRules))
    {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        // берет текст с помощью globalMatch(text) сравнивает условия Регулярного выражения rule.pattern и закидывает в итератор
        while (matchIterator.hasNext())
        {
        // Условия понятно!! Обратить внимание что итератор в начале указывает на начало а не на первое вхождение!!! поэтому следующее инструкци передвигает каретку на один
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
